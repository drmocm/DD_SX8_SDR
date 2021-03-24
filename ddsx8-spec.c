#include <unistd.h>
#include "spec.h"
#include "numeric.h"
#include "dvb.h"

#define SINGLE_PAM 0
#define MULTI_PAM  1
#define CSV 2


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
		    " -c       : continuous output\n\n"
		    " alpha    : parameter of the KAiser window\n\n", argv);
}



int parse_args(int argc, char **argv,
	       uint32_t *freq, int *adapter, int *input,
	       int *use_window, double *alpha, uint32_t *id)
{
    int outmode = SINGLE_PAM;
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
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	c = getopt_long(argc, argv, 
			"f:a:kl:i:bcth",
			long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {
	case 'f':
	    *freq = strtoul(optarg, NULL, 0);
	    break;
	case 'a':
	    *adapter = strtoul(optarg, NULL, 0);
	    break;
	case 'k':
	    *use_window = 1;
	    break;
	case 'l':
	    *alpha = strtod(optarg, NULL);	    
	    break;
	case 'i':
	    *input = strtoul(optarg, NULL, 0);
	    break;
	case 'b':
	    *id = AGC_ON;
	    break;
	case 'c':
	    outmode = MULTI_PAM;
	    break;
	case 't':
	    outmode = CSV;
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
    return outmode;
}

int main(int argc, char **argv){
    specdata spec;
    int adapter = 0;
    int input = 0;
    uint32_t freq = -1;
    int use_window = 0;
    double alpha = 2.0;
    int fe_fd = -1;
    int fdin = -1;
    int fd = -1;
    uint32_t id = AGC_OFF;
    int outm = 0;
    
    if ((outm = parse_args(argc, argv, &freq, &adapter,
			   &input, &use_window, &alpha, &id)) < 0) exit(1);
    if (init_specdata(&spec, freq, FFT_LENGTH, 9*FFT_LENGTH/16,
		      alpha, 1000, use_window) < 0) {
	exit(1);
    }
    if ( (fe_fd=open_fe(adapter, 0)) < 0){
	exit(1);
    }
    if (set_fe_input(fe_fd, freq, FFT_SR, SYS_DVBS2, input, id) < 0){
	exit(1);
    }
    if ( open_dmx(adapter, 0) < 0){
	exit(1);
    }
    if ( (fdin=open_dvr(adapter, 0)) < 0){
	exit(1);
    }

    switch (outm){
    case SINGLE_PAM:
	spec_read_data(fdin, &spec);
	spec_write_pam(fileno(stdout), &spec);
	break;

    case MULTI_PAM:
	while(1){
	    spec_read_data(fdin, &spec);
	    spec_write_pam(fileno(stdout), &spec);
	}
	break;

    case CSV:
	spec_read_data(fdin, &spec);
	spec_write_csv(fileno(stdout), &spec);
	break;
    }
}
