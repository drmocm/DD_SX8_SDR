/*
ddsx8-spec is an **example** program written in C to show how to use 
the SDR mode of the DigitalDevices MAX SX8 to get IQ data.

Copyright (C) 2021  Marcus Metzler

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <unistd.h>
#include "iod.h"
#include "spec.h"
#include "numeric.h"
#include "dvb.h"
#include "blindscan.h"

// various outmodes

static int min= 0;
static int multi= 0;

#define MAXTRY 5
int check_tune(enum fe_delivery_system delsys, io_data *iod)
{
    int lock = 0;
    int t = 0;

    fprintf(stderr,"Trying to tune freq: %d sr: %d delsys: %s ",  iod->freq,
	    iod->fft_sr, delsys == SYS_DVBS ? "DVB-S" : "DVB-S2");
    iod->delsys = delsys;
    if (tune(iod, 0)){
	fprintf(stderr,"Tuning failed\n");
	exit(1);
    }
    
    while (!lock && t < MAXTRY){
	t++;
	fprintf(stderr,".");
	lock = read_status(iod->fe_fd);
	sleep(1);
    }
    fprintf(stderr,"%slock\n\n",lock ? " ": " no ");
    return lock;
}


int next_freq_step(io_data *iod)
{
    uint32_t sfreq = iod->fstart+iod->window/2;
    uint32_t freq;
    
    if (!iod->full) return -1;

    if (iod->step < 0){ 
	freq = iod->fstart + iod->frange/2;
	iod->freq = freq;
	if (tune(iod, 0)){
	    fprintf(stderr,"Tuning failed\n");
	    exit(1);
	}
	iod->step = 0;
    }
    freq = sfreq+iod->window*iod->step;
//    if (freq+iod->window/2 > iod->fstop) return -1;
    if (freq  > iod->fstop) return -1;
    iod->freq = freq;
    fprintf(stderr,"Setting frequency %d step %d\n",freq,iod->step);
    if (tune(iod, 1)){
	fprintf(stderr,"Tuning failed\n");
	exit(1);
    }
    while (!read_status(iod->fe_fd))
	usleep(1000);
    iod->step++;
    return iod->step-1;
}

void print_help(char *argv){
    fprintf(stderr,
	    " usage: %s <options> \n\n",
	    argv);
    file_options();
    print_tuning_options();
    print_spectrum_options();
    print_check_options();
    fprintf(stderr,
	    "\n -h           : this help message\n\n"
	);
}

int parse_args(int argc, char **argv, specdata *spec, io_data *iod)
{
    enum fe_delivery_system delsys = SYS_DVBS2;
    int use_window = 0;
    double alpha = 2.0;
    int nfft = 1000; //number of FFTs for average
    int full = 0;
    int width = FFT_LENGTH;
    int height = 9*FFT_LENGTH/16;
    int outmode = 0;
    int adapter = 0;
    int input = 0;
    int fe_num = 0;
    uint32_t freq = -1;
    uint32_t fstart = MIN_FREQ;
    uint32_t fstop = MAX_FREQ;
    uint32_t t =0;
    uint32_t sr = FFT_SR;
    uint32_t id = AGC_OFF;
    int delay = 0;
    uint32_t pol = 2;
    uint32_t hi = 0;
    int smooth = 0;
    char *nexts= NULL;
    uint32_t lnb = 0;
    int lnb_type = UNIVERSAL;
    
    if (argc < 2) {
	print_help(argv[0]);
	return -1;
    }
    
    while (1) {
	int cur_optind = optind ? optind : 1;
	int option_index = 0;
	int c;
	static struct option long_options[] = {
	    {"adapter", required_argument, 0, 'a'},
	    {"agc", no_argument, 0, 'b'},
	    {"continuous", no_argument, 0, 'c'},
	    {"check_tune", no_argument, 0, 'C'},
	    {"delay", no_argument, 0, 'D'},
	    {"frequency", required_argument, 0, 'f'},
	    {"blindscan", required_argument, 0, 'g'},
	    {"help", no_argument , 0, 'h'},
	    {"input", required_argument, 0, 'i'},
	    {"frontend", required_argument, 0, 'e'},
	    {"Kaiserwindow", no_argument, 0, 'k'},
	    {"lnb", required_argument, 0, 'L'},
	    {"alpha", required_argument, 0, 'l'},
	    {"nfft", required_argument, 0, 'n'},	    
	    {"output", required_argument , 0, 'o'},
	    {"polarisation", required_argument, 0, 'p'},
	    {"quick", no_argument, 0, 'q'},
	    {"symbol_rate", required_argument, 0, 's'},
	    {"csv", no_argument, 0, 't'},
	    {"csvmin", no_argument, 0, 'T'},
	    {"band", no_argument, 0, 'u'},
	    {"full_spectrum", required_argument, 0, 'x'},	    
	    {0, 0, 0, 0}
	};

	    c = getopt_long(argc, argv, 
			    "a:bcCDf:g:hi:e:kL:l:n:o:p:qs:tTux:",
			    long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {

	case 'a':
	    adapter = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'b':
	    id = AGC_ON;
	    break;
	    
	case 'c':
	    multi = 1;
	    break;

	case 'C':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t -g\n");
		exit(1);
	    }
	    outmode = CHECK_TUNE;
	    id = DVB_UNDEF;
	    break;
	    
	case 'd':
	    delay = 1000000;
	    break;
	    
	case 'e':
	    fe_num = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'f':
	    freq = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'g':
	    if (outmode == CSV){
		outmode = BLINDSCAN_CSV;
	    } else if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t -g\n");
		exit(1);
	    }
	    if (!outmode) outmode = BLINDSCAN;
	    full = 1;
	    smooth = strtoul(optarg, NULL, 0);
	    break;

	case 'h':
	    print_help(argv[0]);
	    return -1;

	case 'i':
	    input = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'k':
	    use_window = 1;
	    break;
	    
	case 'L':
	    lnb = strtoul(optarg, NULL,0);	    
	    break;
	    
	case 'l':
	    alpha = strtod(optarg, NULL);	    
	    break;
	    
	case 'o':
	    iod->filename = strdup(optarg);
	    break;
	    
	case 'n':
	    nfft = strtoul(optarg, NULL, 10);
	    break;

	case'p':
	    pol =  strtoul(optarg, NULL, 0);
	    break;
	    
	case 'q':
	    width = FFT_LENGTH/2;
	    break;

	case 's':
	    sr = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'T':
	    min = 1;
	case 't':
	    if (outmode == BLINDSCAN) outmode = BLINDSCAN_CSV;
	    else if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t\n");
		exit(1);
	    }
	    if (!outmode) outmode = CSV;
	    
	    break;
	    
	case 'u':
	    if (pol == 2) pol = 0;
	    hi  = 1;
	    break;
	    
	case 'x':
	    full = 1;
	    t = strtoul(optarg, &nexts, 0);
	    if (t) {
		fstart = t;
		if (nexts){
		    nexts++;
		    fstop = strtoul(nexts, NULL, 0);
		}
//		fprintf(stderr,"nexts: %s   %d %d\n",nexts, fstart, fstop);
		nexts = NULL;
	    }
	    break;

	default:
	    break;
	    
	}
    }
    /*
    if (optind < argc) {
	fprintf(stderr,"Warning: unused arguments\n");
    }
    */
    height = 9*width/16;
    set_io(iod, delsys, adapter, input, fe_num, freq, sr, pol, lnb, hi,
	   width, id, full, delay, fstart, fstop, lnb_type, smooth);
    if (init_specdata(spec, width, height, alpha,
		      nfft, use_window) < 0) {
	exit(1);
    }
    if (!outmode) outmode = SINGLE_PAM;

    return outmode;
}

void spectrum_output( int mode, io_data *iod, specdata *spec)
{
    int full = iod->full;
    int run = 1;
    bitmap *bm = NULL;
    int64_t str;
    int width = 1920;
    int height = width*9/16;
    int steps = iod->frange/iod->window;
    int swidth = width/steps;
    struct dtv_fe_stats st;
    graph g;
    double *fullspec = NULL;
    double *fullfreq=NULL;
    int maxstep = (iod->fstop - iod->fstart)/iod->window+1;
    int fulllen =  spec->width/2*maxstep;
    int k = 0;
    blindscan blind;

    iod->step = -1;

    while (run){
	if (!multi) run = 0;
	if (min) min = 1;
	if (!full) {
	    spec_set_freq(spec, iod->freq, iod->fft_sr);
	    spec_read_data(iod->fdin, spec);
	    switch (mode){
	    
	    case SINGLE_PAM:
		if ( bm == NULL) {
		    bm = init_bitmap(width, height, 3);
		    clear_bitmap(bm);
		}
		init_graph(&g, bm, 0, 0, 0, 0);
		spec_write_graph (iod->fd_out, &g, spec);
		break;
		
	    case CSV:
		spec_write_csv(iod->fd_out, spec, iod->freq, iod->fft_sr,
			       0, 0, min);
		break;
	    }
	} else {
	    int step = 0;
	    if (maxstep<2){
		fprintf (stderr,"Range too small use single spectrum\n",
			 maxstep);
		exit(1);
	    }	    
	    k = 0;
	    if (!fullspec){
		iod->step = -1;
		int maxmem = sizeof(double) *spec->width/2*
		    ((MAX_FREQ - MIN_FREQ)/iod->window+1);

		if (!(fullspec = (double *) malloc(maxmem))){
		    {
			fprintf(stderr,"not enough memory\n");
			exit(1);
		    }
		}
		if (!(fullfreq = (double *) malloc(maxmem))){
		    {
			fprintf(stderr,"not enough memory\n");
			exit(1);
		    }
		}
		memset(fullspec, 0, maxmem);
		memset(fullfreq, 0, maxmem);
		if (mode != CSV){
		    bm = init_bitmap(width, height, 3);
		    clear_bitmap(bm);
		    init_graph(&g, bm, iod->fstart/1000.0,
			       iod->fstop/1000.0, 0, 0);
		}
	    } else iod->step = 0;

	    while ((step=next_freq_step(iod)) >= 0){
		spec_set_freq(spec, iod->freq, iod->fft_sr);
		spec_read_data(iod->fdin, spec);
/*
		get_stat(iod->fe_fd, DTV_STAT_SIGNAL_STRENGTH, &st);
		str = st.stat[0].svalue;
*/
		for (int i = 0; i <= spec->width/2; i++){
		    fullspec[i+k] = spec->pow[i+spec->width/4];
		    fullfreq[i+k] = spec->freq[i+spec->width/4];
		}

		if (mode == CSV && min){
		    write_csv (iod->fd_out, spec->width/2,
			       iod->fft_sr/spec->width/1000,
			       iod->fstart, &fullspec[k], 0, 0, min);
		    if (min) min = 2;
		}
		k += spec->width/2;
	    }

	    // from here use k instead of fullen, because of discrete steps
	    switch (mode){
	    case CSV:
		if (!min){
		    write_csv (iod->fd_out, k, iod->fft_sr/spec->width/1000,
			       iod->fstart, fullspec, 0, 0, min);
		}
		break;
		    
	    case SINGLE_PAM:
		if (g.yrange == 0) graph_range(&g, fullfreq, fullspec, k);
		display_array_graph( &g, fullfreq, fullspec, 0, k, 1);
		write_pam (iod->fd_out, bm);
		break; 
		
	    case BLINDSCAN_CSV:
		init_blindscan(&blind, fullspec, fullfreq, k+spec->width);
		do_blindscan(&blind, iod->smooth);
		write_csv (iod->fd_out, k, iod->fft_sr/spec->width/1000,
			   iod->fstart, fullspec, 0, 0, min);
		write_peaks(iod->fd_out, blind.peaks, blind.numpeaks);
		break;
		
	    case BLINDSCAN:
		init_blindscan(&blind, fullspec, fullfreq, k);
		do_blindscan(&blind, iod->smooth);
		graph_range(&g, fullfreq, fullspec, k);
		display_array_graph_c( &g, fullfreq, fullspec,
				       0, k, 0, 200, 0, 1);

		for (int p=0; p < blind.numpeaks; p++){ 
		    peak *pk = blind.peaks;
		    display_peak(&g, pk[p].freq, pk[p].width, pk[p].height,
				 255, 0, 0, 1 );
		}
/*
		graph_range(&g, fullfreq, blind.spec, blind.speclen);
		display_array_graph_c( &g, fullfreq, blind.spec,
				       0, fulllen,255,255,255, 0);
*/		
		
		write_pam (iod->fd_out, bm);
		break;
		
	    default:
		break;
	    }
	}
	if (bm) clear_bitmap(bm);
	iod->step = -1;
    }
    if (mode == SINGLE_PAM){
	delete_bitmap(bm);
    }

}

int main(int argc, char **argv){
    specdata spec;
    int outm=0;
    int full=0;
    io_data iod;

    init_io(&iod);
    init_spec(&spec);
    
    if ((outm = parse_args(argc, argv, &spec, &iod)) < 0)
	exit(1);

    open_io(&iod);
    if (outm != CHECK_TUNE) {
	if (!iod.full){
	    if (tune(&iod, 0)){
		fprintf(stderr,"Tuning failed\n");
		exit(1);
	    }
	}
	spectrum_output (outm,  &iod, &spec );
    } else {
	if (check_tune(SYS_DVBS, &iod)){
	    printf("Successfully tuned freq: %d sr: %d delsys: DVB-S\n",
		   iod.freq, iod.fft_sr);
	} else if (check_tune(SYS_DVBS2, &iod)){
	    printf("Successfully tuned freq: %d sr: %d delsys: DVB-S2\n",
		   iod.freq, iod.fft_sr);
	} else {
	    printf("Tuning failed freq: %d sr: %d\n",
		   iod.freq, iod.fft_sr);
	}
    }

}
