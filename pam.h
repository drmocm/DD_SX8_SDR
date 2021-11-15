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

typedef struct graph_{
    bitmap *bm;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double xrange;
    double yrange;
    double lastx;
    double lasty;
} graph;

void init_graph(graph *g, bitmap *bm, double xmin, double xmax,
	       double ymin, double ymax);

void plot(bitmap *bm, int x, int y, uint8_t R, uint8_t G, uint8_t B);
void plotline(bitmap *bm, int x, int y, int x2, int y2, 
	      uint8_t r, uint8_t g, uint8_t b);
void box(bitmap *bm, int x0, int y0, int x1, int y1,
	 uint8_t R,uint8_t G, uint8_t B);
void circle(bitmap *bm, int x0, int y0, int r, uint8_t R, uint8_t G, uint8_t B);
void squircle(bitmap *bm, int x0, int y0, int rx, int ry, double n,
	      uint8_t R,uint8_t G, uint8_t B);
void ellipse(bitmap *bm, int x0, int y0 ,int rx, int ry,
	     uint8_t R,uint8_t G, uint8_t B);
void coordinate_axes(bitmap *bm, uint8_t r, uint8_t g, uint8_t b);
void get_rgb(int val, uint8_t *R, uint8_t *G, uint8_t *B);
void write_pam(int fd, bitmap *bm);
void write_csv(int fd, int width, uint32_t freq,
	       uint32_t fft_sr, double *pow, int center, int64_t str, int min);
void clear_bitmap(bitmap *bm);
bitmap *init_bitmap(int width, int height, int depth);
void delete_bitmap(bitmap *bm);
void clear_range_bitmap(bitmap *bm, int first, int last);
void display_array(bitmap *bm, double *pow, int length,
		   int startpos, double ymin, double scale, double range);
void plot_to_graph(graph *g, double x2, double y2,
		   uint8_t R, uint8_t G, uint8_t B);
void plotline_graph(graph *g, double x, double y, double x2, double y2, 
		    uint8_t R, uint8_t G, uint8_t B);
void box_graph(graph *g, double x0, double y0, double x1, double y1, 
	       uint8_t R, uint8_t G, uint8_t B);
void ellipse_graph(graph *g, double x, double y, double rx, double ry,
		   uint8_t R, uint8_t G, uint8_t B);
void circle_graph(graph *g, double x, double y, double r,
		  uint8_t R, uint8_t G, uint8_t B);
void squircle_graph(graph *g, double x, double y, double rx, double ry,
		    double n, uint8_t R, uint8_t G, uint8_t B);
void display_array_graph(graph *g, double *x, double *y, int length, int first);
void display_array_graph_c(graph *g, double *x, double *y, int first,
			   int length, uint8_t R, uint8_t G, uint8_t B);
void graph_range(graph *g, double *x, double *y, int width);
void clear_range_graph(graph *g, double dfirst, double dlast);
#endif /* _PAM_H_*/
