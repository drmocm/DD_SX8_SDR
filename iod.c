#include "iod.h"

#include <stdlib.h>

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

void set_io(io_data *iod, int adapter, int num, int fe_num,
	    uint32_t freq, uint32_t sr, uint32_t pol, int lnb,
	    uint32_t hi, uint32_t length, uint32_t id, int full,
	    int delay, uint32_t fstart, uint32_t fstop, int lnb_type,
	    int smooth)
{
    iod->adapter = adapter;
    iod->input = num;
    iod->fe_num = fe_num;
    iod->freq = freq;
    iod->fft_sr = sr;
    iod->fft_length = length;
    iod->id = id;
    iod->full = full;
    iod->smooth = smooth;
    iod->window = (sr/2/1000);
    iod->delay = delay;
    iod->pol = pol;
    iod->hi = hi;
    iod->lnb = lnb;
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
