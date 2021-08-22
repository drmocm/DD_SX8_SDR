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
#include "spec.h"
#include "numeric.h"
#include "dvb.h"
#include "blindscan.h"

#define SINGLE_PAM 0
#define MULTI_PAM  1
#define CSV 2
#define BLINDSCAN 8

#define MIN_FREQ     950000  // kHz
#define MAX_FREQ    2150000  // kHz

#define FFT_LENGTH 1024
#define FFT_SR (50000*FFT_LENGTH) // 1 point 50kHz (in Hz)

typedef struct io_data_{
    int fe_fd;
    int fd_dmx;
    int fdin;
    int fd_out;
    int adapter;
    int input;
    int full;
    char *filename;
    uint32_t fstart;
    uint32_t fstop;
    uint32_t frange;
    uint32_t freq;
    uint32_t sat;
    uint32_t pol;
    uint32_t hi;
    uint32_t lnb;
    uint32_t lofs;
    uint32_t lof1;
    uint32_t lof2;
    uint32_t scif_slot;
    uint32_t scif_freq;
    uint32_t fft_sr;
    uint32_t fft_length;
    uint32_t window;
    uint32_t id;
    int step;
    int delay;
} io_data;

void open_io(io_data *iod)
{
    if (iod->filename){
	iod->fd_out = open(iod->filename, O_WRONLY | O_CREAT | O_TRUNC,
			   00644);
    }
    
    if ( (iod->fe_fd=open_fe(iod->adapter, 0)) < 0){
	exit(1);
    }

    if (iod->delay) power_on_delay(iod->fe_fd, iod->delay);
    if (iod->pol != 2) diseqc(iod->fe_fd, iod->lnb, iod->pol, iod->hi);

    if (!iod->full){
	if (iod->freq >MIN_FREQ && iod->freq < MAX_FREQ){
	    if (set_fe_input(iod->fe_fd, iod->freq, iod->fft_sr,
			     SYS_DVBS2, iod->input, iod->id) < 0){
		exit(1);
	    }
	}
    }
    
    if ( (iod->fd_dmx = open_dmx(iod->adapter, iod->input)) < 0){
	exit(1);
    }
    if ( (iod->fdin=open_dvr(iod->adapter, iod->input)) < 0){
	exit(1);
    }
}

void close_io(io_data *iod)
{
    close(iod->fe_fd);
    close(iod->fdin);
    close(iod->fd_dmx);
}

int next_freq_step(io_data *iod)
{
    uint32_t sfreq = iod->fstart+iod->window/2;
    uint32_t freq;
	
    if (!iod->full) return -1;

    if (iod->step < 0 && iod->id == AGC_OFF){
	freq = iod->fstart + iod->frange/2;
	if (set_fe_input(iod->fe_fd, sfreq, iod->fft_sr,
			 SYS_DVBS2, iod->input, iod->id) < 0){
	    exit(1);
	}
	//iod->id = AGC_OFF_C;  no longer necessary with latest driver
	iod->step = 0;
    }
    freq = sfreq+iod->window*iod->step;
    if (freq+iod->window/2 > iod->fstop) return -1;
    iod->freq = freq;
    fprintf(stderr,"Setting frequency %d step %d\n",freq,iod->step);
    if (set_fe_input(iod->fe_fd, iod->freq, iod->fft_sr,
		     SYS_DVBS2, iod->input, iod->id) < 0){
	exit(1);
    }
    while (!read_status(iod->fe_fd))
	usleep(1000);
    iod->step++;
    return iod->step-1;
}

void init_io(io_data *iod)
{
    iod->filename = NULL;
    iod->fd_out = fileno(stdout);
    iod->fe_fd = -1;
    iod->fdin = -1;
    iod->id = AGC_OFF;
    iod->adapter = 0;
    iod->input = 0;
    iod->full = 0;
    iod->freq = 0;
    iod->fft_sr = FFT_SR;
    iod->step = -1;
    iod->delay = 0;
    iod->pol = 2;
    iod->hi = 0;
    iod->fstart = MIN_FREQ;
    iod->fstop = MAX_FREQ;
    iod->frange = (MAX_FREQ - MIN_FREQ);
}

void set_io(io_data *iod, int adapter, int num,
	    uint32_t freq, uint32_t sr, uint32_t pol, uint32_t hi,
	    uint32_t length, uint32_t id, int full, int delay,
	    uint32_t fstart, uint32_t fstop)
{
    iod->adapter = adapter;
    iod->input = num;
    iod->freq = freq;
    iod->fft_sr = sr;
    iod->fft_length = length;
    iod->id = id;
    iod->full = full;
    iod->window = (sr/2/1000);
    iod->delay = delay;
    iod->pol = pol;
    iod->hi = hi;
    if (fstart < MIN_FREQ || fstart > MAX_FREQ ||
	fstop < MIN_FREQ || fstop > MAX_FREQ){
	fprintf(stderr,"Frequencies out of range (%d %d ) using default: %d -  %d\n",
		fstart, fstop, MIN_FREQ, MAX_FREQ);
	return;
    }

    iod->fstart = fstart;
    iod->fstop = fstop;
    iod->frange = (fstop - fstart);
}

void print_help(char *argv){
	    fprintf(stderr," usage:\n\n"
                    " ddsx8-spec [-f frequency] [-p pol] [-s rate] [-u] "
		    "[-a adapter] [-i input]\n"
		    "            [-k] [-l alpha] [-b] [-c] [-x (f1 f2)]\n"
		    "            [-d] [-q] [-n number] [-t] [-h] "
		    "[-o filename]\n\n"
		    " -a adapter   : the number n of the DVB adapter, i.e. \n"
		    "                /dev/dvb/adapter[n] (default=0)\n"
		    " -b           : turn on agc\n"
		    " -c           : continuous PAM output\n"
		    " -d           : use 1s delay to wait for LNB power up\n"
		    " -f frequency : center frequency of the spectrum in kHz\n"
		    " -g           : do a blindscan\n"
		    " -i input     : the physical input of the SX8 (default=0)\n"
		    " -k           : use Kaiser window before FFT\n"
		    " -l alpha     : parameter of the Kaiser window\n"
		    " -n number    : number of FFTs averaging (default 1000)\n"
		    " -o filename  : output filename (default stdout)\n"
		    " -p pol       : polarisation 0=vertical 1=horizontal\n"
		    " -q           : faster FFT\n"
		    " -s rate      : the signal rate used for the FFT in Hz\n"
		    " -t           : output CSV \n"
		    " -u           : use hi band of LNB\n"
		    " -x f1 f2     : full spectrum scan from f1 to f2\n"
		    "                (default -x 0 : 950000 to 2150000 kHz)\n"
		    " -h           : this help message\n\n");
	    
}

int parse_args(int argc, char **argv, specdata *spec, io_data *iod)
{
    int use_window = 0;
    double alpha = 2.0;
    int nfft = 1000; //number of FFTs for average
    int full = 0;
    int width = FFT_LENGTH;
    int height = 9*FFT_LENGTH/16;
    int outmode = SINGLE_PAM;
    int adapter = 0;
    int input = 0;
    uint32_t freq = -1;
    uint32_t fstart = MIN_FREQ;
    uint32_t fstop = MAX_FREQ;
    uint32_t t =0;
    uint32_t sr = FFT_SR;
    uint32_t id = AGC_OFF;
    int delay = 0;
    uint32_t pol = 2;
    uint32_t hi = 0;
    char *nexts= NULL;
    
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
	    {"delay", no_argument, 0, 'd'},
	    {"frequency", required_argument, 0, 'f'},
	    {"blindscan", no_argument, 0, 'g'},
	    {"help", no_argument , 0, 'h'},
	    {"input", required_argument, 0, 'i'},
	    {"Kaiserwindow", no_argument, 0, 'k'},
	    {"alpha", required_argument, 0, 'l'},
	    {"nfft", required_argument, 0, 'n'},	    
	    {"output", required_argument , 0, 'o'},
	    {"polarisation", no_argument, 0, 'p'},
	    {"quick", no_argument, 0, 'q'},
	    {"signal_rate", required_argument, 0, 's'},
	    {"csv", no_argument, 0, 't'},
	    {"band", no_argument, 0, 'u'},
	    {"full_spectrum", required_argument, 0, 'x'},	    
	    {0, 0, 0, 0}
	};

	    c = getopt_long(argc, argv, 
			    "a:bcdf:ghi:kl:n:o:p:qs:tux:",
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
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t -g\n");
		exit(1);
	    }
	    outmode = MULTI_PAM;
	    break;
	    
	case 'd':
	    delay = 1000000;
	    break;
	    
	case 'f':
	    freq = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'g':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t -g\n");
		exit(1);
	    }
	    full = 1;
	    outmode = BLINDSCAN;
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
	    
	case 't':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t -g\n");
		exit(1);
	    }
	    outmode = CSV;
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
    set_io(iod, adapter, input, freq, sr, pol, hi, width, id, full, delay,
	   fstart, fstop);
    if (init_specdata(spec, width, height, alpha,
		      nfft, use_window) < 0) {
	exit(1);
    }

    return outmode;
}

void spectrum_output( int mode, io_data *iod, specdata *spec)
{
    int full = iod->full;
    int run = 1;
    bitmap *bm=NULL;
    int64_t str;
    int width = 1920;
    int height = width*9/16;
    int steps = iod->frange/iod->window;
    int swidth = width/steps;
    struct dtv_fe_stats st;
    graph g;
    double *fullspec = NULL;
    double *fullfreq=NULL;
    int maxstep = (iod->fstop - iod->fstart)/iod->window;
    int fulllen =  spec->width/2*maxstep;
    int k = 0;
    blindscan blind;
    

    iod->step = -1;
    while (run){
	run = 0;
	if (!full) {
	    spec_set_freq(spec, iod->freq, iod->fft_sr);
	    spec_read_data(iod->fdin, spec);
	    switch (mode){
	    case MULTI_PAM:
		run = 1;
	    
	    case SINGLE_PAM:
		if ( bm == NULL) {
		    bm = init_bitmap(width, height, 3);
		    clear_bitmap(bm);
		}
		init_graph(&g, bm, 0, 0, 0, 0);
		spec_write_graph (iod->fd_out, &g, spec);
		break;
		
	    case CSV:
		spec_write_csv(iod->fd_out, spec, iod->freq, iod->fft_sr, 0, 0);
		break;
	    }
	} else {
	    int step = 0;
	    
	    k = 0;
	    if (!fullspec){
		iod->step = -1;
		if (!(fullspec = (double *) malloc(fulllen*
						   sizeof(double)))){
		    {
			fprintf(stderr,"not enough memory\n");
			exit(1);
		    }
		}
		if (!(fullfreq = (double *) malloc(fulllen*
						   sizeof(double)))){
		    {
			fprintf(stderr,"not enough memory\n");
			exit(1);
		    }
		}
		memset(fullspec, 0, fulllen*sizeof(double));
		memset(fullfreq, 0, fulllen*sizeof(double));
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
		k += spec->width/2;
	    }
	    switch (mode){
	    case CSV: 
		write_csv (iod->fd_out, fulllen,
			   iod->fft_sr/spec->width/1000,
			   iod->fstart, fullspec, 0, 0);
		break;
		    
	    case MULTI_PAM:
		run = 1;
		
	    case SINGLE_PAM:
		if (g.yrange == 0) graph_range(&g, fullfreq, fullspec, fulllen);
		g.lastx = fullspec[0];
		g.lasty = fullfreq[0];

		display_array_graph( &g, fullfreq, fullspec,
				     0, fulllen);
		write_pam (iod->fd_out, bm);
		break;
		
	    case BLINDSCAN:
		init_blindscan(&blind, fullspec, fullfreq, fulllen);
		do_blindscan(&blind);
		if (g.yrange == 0) graph_range(&g, fullfreq, blind.spec, blind.speclen);
		g.lastx = fullspec[0];
		g.lasty = fullfreq[0];
		display_array_graph( &g, fullfreq, blind.spec,
				     0, fulllen);
		write_csv (iod->fd_out, fulllen,
			   iod->fft_sr/spec->width/1000,
			   iod->fstart, blind.spec, 0, 0);
		//write_pam (iod->fd_out, bm);
		break;
		
	    default:
		break;
	    }
	}
	if (bm) clear_bitmap(bm);
	iod->step = -1;
    }
    if (mode == SINGLE_PAM || mode == MULTI_PAM){
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

    spectrum_output (outm,  &iod, &spec );

}
