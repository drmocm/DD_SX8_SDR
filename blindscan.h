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
#include <getopt.h>
#include <math.h>
#include <time.h>
#include <string.h>

typedef struct blindscan_ {
    double freq_start;
    double freq_end;
    double freq_step;
    double *spec;
    double *peaks;
    double *widths;
    int speclen;
    int numpeaks;
} blindscan;


void init_blindscan (blindscan *b, double *spec, double *freq, int speclen);

    
#endif /* _blindscan_H_*/
