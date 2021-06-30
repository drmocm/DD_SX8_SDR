#include <unistd.h>
#include "spec.h"
#include "numeric.h"
#include "dvb.h"

#define SINGLE_PAM 0
#define MULTI_PAM  1
#define CSV 2
#define FULL_SPECTRUM 4

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



void print_help(char *argv){
	    fprintf(stderr,"usage:\n"
                    "%s [-f frequency] [-p pol] [-s rate] [-a adapter] "
		    "[-i input] [-k] [-l alpha] [-b] [-c] [-x] [-q] [-d] [-h]\n\n"
		    " -f frequency: center frequency of the spectrum in kHz\n\n"
		    " -p pol      : polarisation 0=vertical 1=horizontal\n\n"
		    " -s rate     : the signal rate used for the FFT in Hz\n"
		    " -a adapter  : the number n of the DVB adapter, i.e. \n"
		    "              /dev/dvb/adapter[n] (default=0)\n\n"
		    " -i input    : the physical input of the SX8 (default=0)\n\n"
		    " -u          : use hi band of LNB\n\n"
		    " -k          : use Kaiser window before FFT\n\n"
		    " -b          : turn on agc\n\n"
		    " -n          : number of FFTs for averaging (default 1000)\n\n"
		    " -c          : continuous PAM output\n\n"
		    " -t          : output CSV \n\n"
		    " -x f1 f2    : full spectrum scan from f1 to f2\n\n"
		    "               (default -x 0 : 950000 to 2150000 kHz) \n\n"
		    " -q          : faster FFT\n\n"
		    " -d          : use 1s delay to wait for LNB power up\n\n"
		    " -l alpha    : parameter of the Kaiser window\n\n", argv);
}

void open_io(io_data *iod)
{

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
    if (iod->step < 0) iod->step= 0;
    else iod->step+=1;

    if (iod->step == 0 && iod->id == AGC_OFF){
	fprintf(stderr,"Optimizing AGC\n",freq,iod->step);
	freq = iod->fstart + iod->frange/2;
	if (set_fe_input(iod->fe_fd, sfreq, iod->fft_sr,
			 SYS_DVBS2, iod->input, iod->id) < 0){
	    exit(1);
	}
	iod->id = AGC_OFF_C;
    }
    freq = sfreq+iod->window*iod->step;
    if (freq+iod->window/2 > iod->fstop) return -1;
    iod->freq = freq;
    fprintf(stderr,"Setting frequency %d step %d %d\n",freq,iod->step, iod->fft_length);
    if (set_fe_input(iod->fe_fd, iod->freq, iod->fft_sr,
		     SYS_DVBS2, iod->input, iod->id) < 0){
	exit(1);
    }
    while (!read_status(iod->fe_fd))
	usleep(1000);
    return iod->step;
}

void init_io(io_data *iod)
{
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


int parse_args(int argc, char **argv, specdata *spec, io_data *iod)
{
    int use_window = 0;
    double alpha = 2.0;
    int outm = 0;
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
	    {"frequency", required_argument, 0, 'f'},
	    {"adapter", required_argument, 0, 'a'},
	    {"Kaiserwindow", no_argument, 0, 'k'},
	    {"alpha", required_argument, 0, 'l'},
	    {"input", required_argument, 0, 'i'},
	    {"signal_rate", required_argument, 0, 's'},
	    {"agc", no_argument, 0, 'b'},
	    {"delay", no_argument, 0, 'd'},
	    {"band", no_argument, 0, 'u'},
	    {"continuous", no_argument, 0, 'c'},
	    {"polarisation", no_argument, 0, 'p'},
	    {"quick", no_argument, 0, 'q'},
	    {"nfft", required_argument, 0, 'n'},	    
	    {"full_spectrum", required_argument, 0, 'x'},	    
	    {"output", required_argument , 0, 'o'},
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	c = getopt_long(argc, argv, 
			"f:a:kl:i:bctn:ho:qdp:us:x:",
			long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {

	case 'f':
	    freq = strtoul(optarg, NULL, 0);
	    break;
	    
	case 's':
	    sr = strtoul(optarg, NULL, 0);
	    break;
	    
	case'p':
	    pol =  strtoul(optarg, NULL, 0);
	    break;
	    
	case 'a':
	    adapter = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'k':
	    use_window = 1;
	    break;
	    
	case 'l':
	    alpha = strtod(optarg, NULL);	    
	    break;
	    
	case 'i':
	    input = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'd':
	    delay = 1000000;
	    break;
	    
	case 'u':
	    if (pol == 2) pol = 0;
	    hi  = 1;
	    break;
	    
	case 'b':
	    id = AGC_ON;
	    break;
	    
	case 'c':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t\n");
		exit(1);
	    }
	    outmode = MULTI_PAM;
	    break;
	    
	case 't':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -c -t\n");
		exit(1);
	    }
	    outmode = CSV;
	    break;
	    
	case 'n':
	    nfft = strtoul(optarg, NULL, 10);
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

	case 'q':
	    width = FFT_LENGTH/2;
	    break;

	case 'h':
	    print_help(argv[0]);
	    return -1;
	default:
	    break;
	    
	}
    }
    /*
    if (optind < argc) {
	fprintf(stderr,"Warning: unused arguments\n");
    }
    */
    if (outm&FULL_SPECTRUM) full=1;
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
		init_grap(&g, bm, 0, 0, 0, 0);
		spec_write_graph (iod->fd_out, &g, spec);
		break;
		
	    case CSV:
		spec_write_csv(iod->fd_out, spec, iod->freq, iod->fft_sr, 0, 0);
		break;
	    }
	} else {
	    int step = 0;
	    iod->step = 0;
	    
	    while ((step=next_freq_step(iod)) >= 0){
		spec_set_freq(spec, iod->freq, iod->fft_sr);
		spec_read_data(iod->fdin, spec);

		switch (mode){

		case CSV: 
		    get_stat(iod->fe_fd, DTV_STAT_SIGNAL_STRENGTH, &st);
		    str = st.stat[0].svalue;
		    spec_write_csv(iod->fd_out, spec,
				   iod->freq, iod->fft_sr,2,str);
		    break;

		case MULTI_PAM:
		    run = 1;
		    
		case SINGLE_PAM:
		    if ( bm == NULL) {
			bm = init_bitmap(width, height, 3);
			clear_bitmap(bm);
			init_grap(&g, bm, iod->fstart, iod->fstop, 0, 60);
			g.lastx = spec->freq[0];
			g.lasty = spec->pow[0];
		    }
#if 0		    
		    display_array_graph( &g, spec->freq, spec->pow,
					 spec->width/2, spec->width/4);
#else
		    display_array(bm, spec->pow, spec->width/2,
				  spec->width/2*step,25,
				  (double)2.0*swidth/spec->width, 40);

#endif
		    write_pam (iod->fd_out, bm);
		    break;
		    
		default:
		    fprintf(stderr,"Full spectrum only works with -t option\n");
		    exit(1);
		    break;
		}
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
