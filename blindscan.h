#ifndef _BLINDSCAN_H_
#define _BLINDSCAN_H_

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

typedef struct peak_ {
    int start;
    int startend;
    int stop;
    int stopstart;
    double freq;
    double width;
    double height;
    double slopestart;
    double slopestop;
} peak;

typedef struct blindscan_ {
    double freq_start;
    double freq_end;
    double freq_step;
    double *spec;
    peak *peaks;
    int speclen;
    int numpeaks;
} blindscan;


void init_blindscan (blindscan *b, double *spec, double *freq, int speclen);
int do_blindscan(blindscan *b, int smooth);
void write_peaks(int fd, peak *pk, int l, double lof, int pol);

    
#endif /* _blindscan_H_*/
