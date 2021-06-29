#ifndef _PAM_H_
#define _PAM_H_

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

typedef struct bitmap_{
    uint8_t *data;
    int width;
    int height;
    int depth;
} bitmap;


void plot(bitmap *bm, int x, int y,
			unsigned char R,
			unsigned char G,
			unsigned char B);
void plotline(bitmap *bm, int x, int y, int x2, int y2, 
	      unsigned char r,
	      unsigned char g,
	      unsigned char b);
void coordinate_axes(bitmap *bm, unsigned char r,
		     unsigned char g, unsigned char b);
void get_rgb(int val, uint8_t *R, uint8_t *G, uint8_t *B);
void write_pam(int fd, bitmap *bm);
void write_csv(int fd, int width, uint32_t freq,
	       uint32_t fft_sr, double *pow, int center, int64_t str);
void clear_bitmap(bitmap *bm);
bitmap *init_bitmap(int width, int height, int depth);
void delete_bitmap(bitmap *bm);
void display_array(bitmap *bm, double *pow, int length);

#endif /* _PAM_H_*/
