/*
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

    if (iod->lnb_type != UNIVERSAL) quick = 0;
    if (iod->pol == 2) quick = 1;
    if ( !quick ){
	tune_sat(iod->fe_fd, iod->lnb_type, iod->freq, 
		 iod->fft_sr, iod->delsys, iod->input,
		 iod->id, iod->pol, iod->hi,
		 iod->lnb, iod->lofs, iod->lof1, iod->lof2,
		 iod->scif_slot, iod->scif_freq);
    } else {
	if (iod->freq >MIN_FREQ && iod->freq < MAX_FREQ){
	    if (set_fe_input(iod->fe_fd, iod->freq, iod->fft_sr,
			     iod->delsys, iod->input, iod->id) < 0){
		return -1;
	    }
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

    if (iod->pol != 2) power_on_delay(iod->fe_fd, iod->delay);

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
// default LNBs
    iod->lofs = 11700000;
    iod->lof1 =  9750000;
    iod->lof2 = 10600000;
    iod->scif_slot = 0;
    iod->scif_freq = 1210;
}

void set_io_tune(io_data *iod, enum fe_delivery_system delsys,
		 int adapter, int input, int fe_num,
		 uint32_t freq, uint32_t sr, uint32_t pol, int lnb,
		 uint32_t hi, uint32_t id, int delay,  int lnb_type,
		 uint32_t lofs, uint32_t lof1, uint32_t lof2,
		 uint32_t scif_slot, uint32_t scif_freq)
{
    iod->adapter = adapter;
    iod->input = input;
    iod->fe_num = fe_num;
    iod->freq = freq;
    iod->fft_sr = sr;
    iod->id = id;
    iod->window = (sr/2/1000);
    iod->delay = delay;
    iod->pol = pol;
    iod->hi = hi;
    iod->lnb = lnb;
    iod->lnb_type = lnb_type;
    iod->delsys = delsys;

    if (lofs) iod->lofs = lofs;
    if (lof1) iod->lof1 = lof1;
    if (lof2) iod->lof2 = lof2;
    if (scif_slot) iod->scif_slot = scif_slot;
    if (scif_freq) iod->scif_freq = scif_freq;
}

void set_io(io_data *iod, uint32_t length, int full, uint32_t fstart,
	    uint32_t fstop,  int smooth)
{
    iod->fft_length = length;
    iod->full = full;
    iod->smooth = smooth;
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
	    " -I id        : set id (do not use if you don't know)\n"
	    " -l ls l1 l2  : set lofs lof1 lof2 \n"
	    "              : (default 11700000 9750000 10600000)\n"
 	    " -L n         : diseqc switch to LNB/SAT number n (default 0)\n"
	    " -p pol       : polarisation 0=vertical 1=horizontal\n"
	    "              : (must be set for any diseqc command to be send)\n"
	    " -s rate      : the symbol rate in Symbols/s\n"
	    " -u           : use hi band of LNB\n"
	    " -D           : use 1s delay to wait for LNB power up\n"
	    " -U type      : lnb is unicable type (1: EN 50494, 2: TS 50607\n"
	    " -j slot freq : slot s freqency f ( default slot 1 freq 1210 in MHz)\n"
	);
}

int parse_args_io_tune(int argc, char **argv, io_data *iod)
{
    enum fe_delivery_system delsys = SYS_DVBS2;
    int adapter = 0;
    int input = 0;
    int fe_num = 0;
    uint32_t freq = 0;
    uint32_t sr = FFT_SR;
    uint32_t id = DVB_UNDEF;
    int delay = 0;
    uint32_t pol = 2;
    uint32_t hi = 0;
    uint32_t lnb = 0;
    int lnb_type = UNIVERSAL;
    uint32_t lofs = 0;
    uint32_t lof1 = 0;
    uint32_t lof2 = 0;
    uint32_t scif_slot = 0;
    uint32_t scif_freq = 0;
    char *nexts= NULL;
    opterr = 0;
    optind = 1;
    char **myargv;

    myargv = malloc(argc*sizeof(char*));
    memcpy(myargv, argv, argc*sizeof(char*));
    
    while (1) {
	int option_index = 0;
	int c;
	static struct option long_options[] = {
	    {"adapter", required_argument, 0, 'a'},
	    {"delay", no_argument, 0, 'D'},
	    {"delsys", required_argument, 0, 'd'},
	    {"frequency", required_argument, 0, 'f'},
	    {"lofs", required_argument, 0, 'l'},
	    {"unicable", required_argument, 0, 'U'},
	    {"slot", required_argument, 0, 'j'},
	    {"input", required_argument, 0, 'i'},
	    {"id", required_argument, 0, 'I'},
	    {"frontend", required_argument, 0, 'e'},
	    {"lnb", required_argument, 0, 'L'},
	    {"polarisation", required_argument, 0, 'p'},
	    {"symbol_rate", required_argument, 0, 's'},
	    {"band", no_argument, 0, 'u'},
	    {0, 0, 0, 0}
	};

	c = getopt_long(argc, myargv, 
			"a:d:Df:I:i:j:e:L:p:s:ul:U:S:",
			long_options, &option_index);
	if (c==-1)
	    break;
	switch (c) {
	case 'a':
	    adapter = strtoul(optarg, NULL, 0);
	    break;

	case 'd':
	    if (!strcmp(optarg, "C"))
		delsys = SYS_DVBC_ANNEX_A;
	    if (!strcmp(optarg, "DVBC"))
		delsys = SYS_DVBC_ANNEX_A;
	    if (!strcmp(optarg, "S"))
		delsys = SYS_DVBS;
	    if (!strcmp(optarg, "DVBS"))
		delsys = SYS_DVBS;
	    if (!strcmp(optarg, "S2"))
		delsys = SYS_DVBS2;
	    if (!strcmp(optarg, "DVBS2"))
		delsys = SYS_DVBS2;
	    if (!strcmp(optarg, "T"))
		delsys = SYS_DVBT;
	    if (!strcmp(optarg, "DVBT"))
		delsys = SYS_DVBT;
	    if (!strcmp(optarg, "T2"))
		delsys = SYS_DVBT2;
	    if (!strcmp(optarg, "DVBT2"))
		delsys = SYS_DVBT2;
	    if (!strcmp(optarg, "J83B"))
		delsys = SYS_DVBC_ANNEX_B;
	    if (!strcmp(optarg, "ISDBC"))
		delsys = SYS_ISDBC;
	    if (!strcmp(optarg, "ISDBT"))
		delsys = SYS_ISDBT;
	    if (!strcmp(optarg, "ISDBS"))
		delsys = SYS_ISDBS;
	    break;

	case 'D':
	    delay = 1000000;
	    break;
	    
	case 'e':
	    fe_num = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'f':
	    freq = strtoul(optarg, NULL, 0);
	    break;

	case 'l':
	    lofs = strtoul(optarg, &nexts, 0);
	    if (nexts){
		nexts++;
		lof1 = strtoul(nexts, &nexts, 0);
	    } if (nexts) {
		nexts++;
		lof2 = strtoul(nexts, NULL, 0);
	    } else {
		fprintf(stderr, "Error Missing data in -l (--lofs)");
		return -1;
	    } 
	    break;
	    
	case 'U':
	    lnb_type = strtoul(optarg, NULL, 0);
	    break;

	case 'j':
	    nexts = NULL;
	    scif_slot = strtoul(optarg, &nexts, 0);
	    scif_slot = scif_slot-1;
	    nexts++;
	    scif_freq = strtoul(nexts, NULL, 0);
	    scif_freq = scif_freq;
	    break;
	    
	case 'i':
	    input = strtoul(optarg, NULL, 0);
	    break;

	case 'I':
	    id = strtoul(optarg, NULL, 0);
	    break;

	case 'L':
	    lnb = strtoul(optarg, NULL,0);	    
	    break;
	    
	case'p':
	    if (!strcmp(optarg, "h") || !strcmp(optarg, "H"))
	    {
		pol = 1;
	    } else if (!strcmp(optarg, "v") || !strcmp(optarg, "V"))
	    {
		pol = 0;
	    } else {
		pol =  strtoul(optarg, NULL, 0);
	    }
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

    set_io_tune(iod, delsys, adapter, input, fe_num, 
		freq, sr, pol, lnb, hi, id, delay, lnb_type,
		lofs, lof1, lof2, scif_slot, scif_freq);

    return 0;
}




