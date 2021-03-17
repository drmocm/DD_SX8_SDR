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
#include <math.h>
#include <time.h>
#include <string.h>
#include <complex.h>
#include <fftw3.h>

#define TS_SIZE 188
#define MIN_FREQ     950000  // kHz
#define MAX_FREQ    2050000  // kHz

#define FREQ_RANGE (MAX_FREQ - MIN_FREQ)  

#define FFT_LENGTH 1024
#define FFT_SR (50000*FFT_LENGTH) // 1 point 50kHz (in MHz)
#define WINDOW (FFT_SR/2/1000)    //center window 
#define MAXWIN (FREQ_RANGE/WINDOW)

typedef struct specdata_
{
    unsigned char *data_points;
    double *pow;
    double alpha;
    int maxrun;
    int width;
    int height;
    int use_window;
    uint32_t freq;
} specdata;

int init_specdata(specdata *spec, uint32_t freq, int width, int height,
		  double alpha, int maxrun, int use_window);
void spec_read_data (int fdin, specdata *spec);
void spec_write_pam (int fd, specdata *spec);
void spec_write_csv (int fd, specdata *spec);

#endif /* _SPEC_H_*/
