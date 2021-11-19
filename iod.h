#ifndef _IOD_H_
#define _IOD_H_

#include "dvb.h"

#define SINGLE_PAM 1
#define CSV 2
#define BLINDSCAN 3
#define BLINDSCAN_CSV 4
#define CHECK_TUNE 5

#define UNICABLE 1
#define UNIVERSAL 2

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
    int fe_num;
    int full;
    int smooth;
    char *filename;
    int lnb_type;
    uint32_t fstart;
    uint32_t fstop;
    uint32_t frange;
    uint32_t freq;
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


int tune(enum fe_delivery_system delsys, io_data *iod, int quick);
void set_io(io_data *iod, int adapter, int num, int fe_num,
	    uint32_t freq, uint32_t sr, uint32_t pol, int lnb,
	    uint32_t hi, uint32_t length, uint32_t id, int full,
	    int delay, uint32_t fstart, uint32_t fstop, int lnb_type,
	    int smooth);
void init_io(io_data *iod);
void close_io(io_data *iod);
void open_io(io_data *iod);
void print_spectrum_options();
void print_check_options();
void print_tuning_options();

#endif
