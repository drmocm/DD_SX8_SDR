#ifndef _SPEC_H_
#define _SPEC_H_

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <complex.h>
#include <fftw3.h>
#include "pam.h"

#define TS_SIZE 188
#define FFT_LENGTH 1024

typedef struct specdata_
{
    double *freq;
    double *pow;
    double alpha;
    int maxrun;
    int width;
    int use_window;
} specdata;

void init_spec(specdata *spec);
int init_specdata(specdata *spec, int width, int height,
		  double alpha, int maxrun, int use_window);
void spec_read_data (int fdin, specdata *spec);
void spec_write_pam (int fd, bitmap *bm, specdata *spec);
void spec_write_csv (int fd, specdata *spec,uint32_t freq,
		     uint32_t fft_sr, int center, int64_t str, int min);
void spec_set_freq(specdata *spec, uint32_t freq, uint32_t fft_sr);
void spec_write_graph (int fd, graph *g, specdata *spec);
void print_spectrum_options();
int parse_args_spectrum(int argc, char **argv, specdata *spec);

#endif /* _SPEC_H_*/
