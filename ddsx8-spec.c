#include <unistd.h>
#include "spec.h"
#include "numeric.h"
#include "dvb.h"

#define SINGLE_PAM 0
#define MULTI_PAM  1
#define CSV 2
#define FULL_SPECTRUM 4

typedef struct io_data_{
    int fe_fd;
    int fdin;
    int fd_out;
    int adapter;
    int input;
    char *filename;
    uint32_t freq;
    uint32_t fft_sr;
    uint32_t id;
} io_data;



void print_help(char *argv){
	    fprintf(stderr,"usage:\n"
                    "%s [-f frequency] [-a adapter] "
		    "[-i input] [-k] [-l alpha] [-b] [-c] [-h]\n\n"
		    " frequency: center frequency of the spectrum in KHz\n\n"
		    " adapter  : the number n of the DVB adapter, i.e. \n"
		    "            /dev/dvb/adapter[n] (default=0)\n\n"
		    " input    : the physical input of the SX8 (default=0)\n\n"
		    " -k       : use Kaiser window before FFT\n\n"
		    " -b       : turn on agc\n\n"
		    " -n       : number of FFTs for averaging (default 1000)\n\n"
		    " -c       : continuous PAM output\n\n"
		    " -t       : output CSV \n\n"
		    " -x       : full spectrum scan\n\n"
		    " alpha    : parameter of the KAiser window\n\n", argv);
}

void open_io(io_data *iod)
{

    if ( (iod->fe_fd=open_fe(iod->adapter, 0)) < 0){
	exit(1);
    }
	
    if (set_fe_input(iod->fe_fd, iod->freq, iod->fft_sr,
		     SYS_DVBS2, iod->input, iod->id) < 0){
	exit(1);
    }
    if ( open_dmx(iod->adapter, iod->input) < 0){
	exit(1);
    }
    if ( (iod->fdin=open_dvr(iod->adapter, iod->input)) < 0){
	exit(1);
    }
}

void init_io(io_data *iod)
{
    iod->fd_out = fileno(stdout);
    iod->fe_fd = -1;
    iod->fdin = -1;
    iod->id = AGC_OFF;
    iod->adapter = 0;
    iod->input = 0;
    iod->freq = -1;
    iod->fft_sr = FFT_SR;
}

void set_io(io_data *iod, int adapter, int num,
	    uint32_t freq, uint32_t sr, uint32_t id)
{
    iod->adapter = adapter;
    iod->input = num;
    iod->freq = freq;
    iod->fft_sr = sr;
    iod->id = id;
}

int parse_args(int argc, char **argv, specdata *spec, io_data *iod)
{
    int use_window = 0;
    double alpha = 2.0;
    int outm = 0;
    int nfft = 1000; //number of FFTs for average
    int full = 0;
    int width = FFT_LENGTH;
    int height = 9*width/16;
    int outmode = SINGLE_PAM;
    int adapter = 0;
    int input = 0;
    uint32_t freq = -1;
    uint32_t sr = FFT_SR;
    uint32_t id = AGC_OFF;
    
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
	    {"agc", no_argument, 0, 'b'},
	    {"continuous", no_argument, 0, 'c'},
	    {"nfft", required_argument, 0, 'n'},	    
	    {"full_spectrum", required_argument, 0, 'x'},	    
	    {"output", required_argument , 0, 'o'},
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	c = getopt_long(argc, argv, 
			"f:a:kl:i:bctn:ho:",
			long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {
	case 'f':
	    freq = strtoul(optarg, NULL, 0);
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
	case 'b':
	    id = AGC_ON;
	    break;
	case 'c':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -x -c -t\n");
		exit(1);
	    }
	    outmode = MULTI_PAM;
	    break;
	case 't':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -x -c -t\n");
		exit(1);
	    }
	    outmode = CSV;
	    break;
	case 'n':
	    nfft = strtoul(optarg, NULL, 0);
	    break;
	case 'x':
	    if (outmode) {
		fprintf(stderr, "Error conflicting options\n");
		fprintf(stderr, "chose only one of the options -x -c -t\n");
		exit(1);
	    }
	    outmode |= FULL_SPECTRUM;
	    break;
	case 'h':
	    print_help(argv[0]);
	    return -1;
	default:
	    break;
	    
	}
    }
    if (optind < argc) {
	fprintf(stderr,"Warning: unused arguments\n");
    }

    if (outm&FULL_SPECTRUM) full=1;
    
    set_io(iod, adapter, input, freq, FFT_SR, id);
    if (init_specdata(spec, width, height, alpha,
		      nfft, use_window) < 0) {
	exit(1);
    }

    return outmode;
}

void spectrum_output( int mode, io_data *iod, specdata *spec)
{
    switch (mode){
    case SINGLE_PAM:
	spec_read_data(iod->fdin, spec);
	spec_write_pam(iod->fd_out, spec);
	break;

    case MULTI_PAM:
	while(1){
	    spec_read_data(iod->fdin, spec);
	    spec_write_pam(iod->fd_out, spec);
	}
	break;

    case CSV:
	spec_read_data(iod->fdin, spec);
	spec_write_csv(iod->fd_out, spec, iod->freq, iod->fft_sr);
	break;
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

    spectrum_output (outm&3,  &iod, &spec );

}
