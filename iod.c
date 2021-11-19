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

#include "iod.h"

int tune(io_data *iod, int quick)
{
    if (iod->pol != 2 && !quick)
	diseqc(iod->fe_fd, iod->lnb, iod->pol, iod->hi);

    if (iod->freq >MIN_FREQ && iod->freq < MAX_FREQ){
	if (set_fe_input(iod->fe_fd, iod->freq, iod->fft_sr,
			 iod->delsys, iod->input, iod->id) < 0){
	    return -1;
	}
    }
    return 0;
}

void open_io(io_data *iod)
{

    if (iod->filename){
	iod->fd_out = open(iod->filename, O_WRONLY | O_CREAT | O_TRUNC,
			   00644);
    }
    
    if ( (iod->fe_fd=open_fe(iod->adapter, iod->fe_num)) < 0){
	exit(1);
    }

    if (iod->delay) power_on_delay(iod->fe_fd, iod->delay);

    if ( (iod->fd_dmx = open_dmx(iod->adapter, iod->fe_num)) < 0){
	exit(1);
    }
    if ( (iod->fdin=open_dvr(iod->adapter,iod->fe_num)) < 0){
	exit(1);
    }
}

void close_io(io_data *iod)
{
    close(iod->fe_fd);
    close(iod->fdin);
    close(iod->fd_dmx);
}

void init_io(io_data *iod)
{
    iod->filename = NULL;
    iod->fd_out = fileno(stdout);
    iod->fe_fd = -1;
    iod->fdin = -1;
    iod->id = AGC_OFF;
    iod->adapter = 0;
    iod->lnb_type = UNIVERSAL;
    iod->input = 0;
    iod->fe_num = 0;
    iod->full = 0;
    iod->smooth = 6;
    iod->freq = 0;
    iod->fft_sr = FFT_SR;
    iod->step = -1;
    iod->delay = 0;
    iod->pol = 2;
    iod->hi = 0;
    iod->fstart = MIN_FREQ;
    iod->fstop = MAX_FREQ;
    iod->frange = (MAX_FREQ - MIN_FREQ);
    iod->lnb = 0;
}

void set_io_tune(io_data *iod, enum fe_delivery_system delsys,
		 int adapter, int num, int fe_num,
		 uint32_t freq, uint32_t sr, uint32_t pol, int lnb,
		 uint32_t hi, uint32_t id, int delay,  int lnb_type)
{
    iod->adapter = adapter;
    iod->input = num;
    iod->fe_num = fe_num;
    iod->freq = freq;
    iod->fft_sr = sr;
    iod->id = id;
    iod->window = (sr/2/1000);
    iod->delay = delay;
    iod->pol = pol;
    iod->hi = hi;
    iod->lnb = lnb;
    iod->delsys = delsys;
}

void set_io(io_data *iod, enum fe_delivery_system delsys,
	    int adapter, int num, int fe_num,
	    uint32_t freq, uint32_t sr, uint32_t pol, int lnb,
	    uint32_t hi, uint32_t length, uint32_t id, int full,
	    int delay, uint32_t fstart, uint32_t fstop, int lnb_type,
	    int smooth)
{
    set_io_tune(iod, delsys, adapter, num, fe_num, freq, sr, pol,
		lnb, hi, id, delay, lnb_type);
    iod->fft_length = length;
    iod->full = full;
    iod->smooth = smooth;
    iod->window = (sr/2/1000);
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

void file_options()
{
    fprintf(stderr,
	    " -o filename  : output filename (default stdout)\n"
	);
}

void print_tuning_options()
{
    fprintf(stderr,
	    "\n TUNING OPTIONS:\n"
	    " -d delsys    : the delivery system (not needed for sepctrum) \n"
	    " -a adapter   : the number n of the DVB adapter, i.e. \n"
	    "                /dev/dvb/adapter[n] (default=0)\n"
	    " -e frontend  : the frontend/dmx/dvr to be used (default=0)\n"
	    " -f frequency : center frequency in kHz\n"
	    " -i input     : the physical input of the SX8 (default=0)\n"
	    " -L n         : diseqc switch to LNB/SAT number n (default 0)\n"
	    " -p pol       : polarisation 0=vertical 1=horizontal\n"
	    "              : (must be set for any diseqc command to be send)\n"
	    " -s rate      : the symbol rate in Symbols/s\n"
	    " -u           : use hi band of LNB\n"
	    " -D           : use 1s delay to wait for LNB power up\n"
	);
}

void print_check_options()
{
    fprintf(stderr,
	    "\n CHECK OPTIONS:\n"
	    " -C           : try to tune the frequency and symbolrate\n"
    	    "              : determine delivery system\n");
}

void print_spectrum_options()
{
    fprintf(stderr,
	    "\n SPECTRUM OPTIONS:\n"
	    " -b           : turn on agc\n"
	    " -c           : continuous PAM output\n"
	    " -k           : use Kaiser window before FFT\n"
	    " -l alpha     : parameter of the Kaiser window\n"
	    " -n number    : number of FFTs averaging (default 1000)\n"
	    " -q           : faster FFT\n"
	    " -o filename  : output filename (default stdout)\n"
	    " -t           : output CSV \n"
	    " -T           : output minimal CSV\n"
	    " -x f1 f2     : full spectrum scan from f1 to f2\n"
	    "                (default -x 0 : 950000 to 2150000 kHz)\n"
	    " -g s         : blindscan, use s to improve scan (higher\n"
	    "                s can lead to less false positives,\n"
	    "                but may lead to missed peaks)\n"
	);
}




int parse_args_io_tune(int argc, char **argv, io_data *iod)
{
    enum fe_delivery_system delsys = SYS_DVBS2;
    int adapter = 0;
    int input = 0;
    int fe_num = 0;
    uint32_t freq = -1;
    uint32_t sr = FFT_SR;
    uint32_t id = AGC_OFF;
    int delay = 0;
    uint32_t pol = 2;
    uint32_t hi = 0;
    uint32_t lnb = 0;
    int lnb_type = UNIVERSAL;
    
    while (1) {
	int cur_optind = optind ? optind : 1;
	int option_index = 0;
	int c;
	static struct option long_options[] = {
	    {"adapter", required_argument, 0, 'a'},
	    {"delay", no_argument, 0, 'D'},
	    {"frequency", required_argument, 0, 'f'},
	    {"input", required_argument, 0, 'i'},
	    {"frontend", required_argument, 0, 'e'},
	    {"lnb", required_argument, 0, 'L'},
	    {"polarisation", required_argument, 0, 'p'},
	    {"symbol_rate", required_argument, 0, 's'},
	    {"band", no_argument, 0, 'u'},
	    {0, 0, 0, 0}
	};

	    c = getopt_long(argc, argv, 
			    "a:Df:i:e:L:p:s:u",
			    long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {

	case 'a':
	    adapter = strtoul(optarg, NULL, 0);
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

	case 'i':
	    input = strtoul(optarg, NULL, 0);
	    break;

	case 'L':
	    lnb = strtoul(optarg, NULL,0);	    
	    break;
	    
	case'p':
	    pol =  strtoul(optarg, NULL, 0);
	    break;
	    
	case 's':
	    sr = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'u':
	    if (pol == 2) pol = 0;
	    hi  = 1;
	    break;

	default:
	    break;
	    
	}
    }

    set_io_tune(iod, delsys, adapter, input, fe_num, freq, sr, pol, lnb,
		hi, id, delay,  lnb_type);

    return 0;
}
