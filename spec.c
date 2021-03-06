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

#include "spec.h"
#include "numeric.h"

int my_read(int fd, u_char *buf, int size)
{
    int re = 0;

    while ( (re = read(fd, buf, size))  < size){
//	fprintf(stdout,"Could not read data (%d) %s\n",re,strerror(errno));
    }
    return re;
}

void init_spec(specdata *spec)
{
    spec->freq = NULL;
    spec->pow = NULL;
    spec->alpha = 3.0;
    spec-> maxrun = 2000;
    spec->width = 0;
    spec->use_window = 0;
}

int init_specdata(specdata *spec, int width, int height,
		  double alpha, int maxrun, int use_window)
{
    int numspec = width;
    spec->use_window = use_window;
    if (maxrun) spec->maxrun = maxrun;
    if (alpha) spec->alpha = alpha;
    
    spec->width = width;
    
    if (!(spec->pow = (double *) malloc(spec->width*sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    return -1;
	}
    }

    if (!(spec->freq = (double *) malloc(spec->width*sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    return -1;
	}
    }

    return 0;
}

static long getutime(){
        struct timespec t0;
        clock_gettime(CLOCK_MONOTONIC_RAW,&t0);
        return t0.tv_sec * (int)1e9 + t0.tv_nsec;
}


#define RSIZE 100
int read_spec_data(int fdin, int8_t *bufx, int8_t *bufy, int size)
{
    int i,j;
    
    u_char ibuf[RSIZE*TS_SIZE+1];
    int c=0;
    int re = 0;

    while (c<size){
	int s = RSIZE;
	int d = 2*(size-c)/(TS_SIZE-4);
	fsync(fdin);
	if ( d < s ) s = d;
	re=my_read(fdin, ibuf, s*TS_SIZE);
      
	for (i=0; i < re; i+=TS_SIZE){
	    if (ibuf[0] != 0x47 || ibuf[1] != 0x02 || ibuf[2] !=0 ) {
		printf("unaligned buffer error\n");
		printf("0x%02x 0x%02x 0x%02x 0x%02x %d\n",ibuf[0],ibuf[1],ibuf[2],ibuf[3],s);
		//	exit(0);
	    }
	    for (j=4; j<TS_SIZE; j+=2){
		bufx[c] = ibuf[i+j];
		bufy[c] = ibuf[i+j+1];
               c++;
	    }
	}
    }
    return c;
}




int spec_read_fft_data(int fdin, fftw_complex *in, int numspec)
{
    int c;
    double isum = 0;
    double qsum = 0;
    int bnum = (numspec/(TS_SIZE-4)+1)*(TS_SIZE-4);    
    int8_t *bufx=(int8_t *) malloc(sizeof(int8_t)*bnum);
    int8_t *bufy=(int8_t *) malloc(sizeof(int8_t)*bnum);
    memset(bufx,0,numspec*sizeof(int8_t));
    memset(bufy,0,numspec*sizeof(int8_t));

    if ((c=read_spec_data(fdin, bufx, bufy, bnum))<0){
	free(bufx);
	free(bufy);
	return -1;
    }

    for (int i = 0; i < numspec; i++) {
	double real,img;
	real = (double)bufx[i];
	img = (double)bufy[i];
	in[i]= CMPLX(real,img);
	isum += real;
	qsum += img;
    }
    double dclevel = (isum+qsum)/(2.0*numspec);
    for (int i = 0; i < numspec; i++) {
	in[i] -= CMPLX(dclevel,dclevel);
    }    
    free(bufx);
    free(bufy);
    return 0;
}


int spec_fft(int fdin, specdata *spec, double *pow, int num)
{
    double alpha = spec->alpha;
    int maxrun= spec->maxrun;
    double *window = NULL;
    
    double max = 0;
    double min = 0;
        
    if (spec->use_window){
	window = KaiserWindow(num, alpha);
    }
    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * num);
    int count = 0;
    for (int i = 0; count < maxrun; i++){
	if ( spec_read_fft_data(fdin,in,num) < 0) return -1;
	do_fft(in, window, num);
	count += fft_power_log(in, pow, num);
    }
    
    fftw_free(in);
    free(window);
	
    for (int i = 0; i < num; i ++){
	pow[i] /=(double)count;
    }
    smooth(pow, num);
    return 0;
}

static void clear_buffer(int fdin)
{
    for (int b=0; b < 12000; b++){
	u_char ibuf[TS_SIZE];
	int re;
	re=my_read(fdin, ibuf, TS_SIZE);
    }
}

void spec_read_data (int fdin, specdata *spec)
{
    uint64_t maxd = 0;
    double *pow= spec->pow;

    clear_buffer(fdin);

    memset(pow,0,spec->width*sizeof(double));

    spec_fft(fdin, spec, pow, spec->width);
}    

void spec_write_pam (int fd, bitmap *bm, specdata *spec)
{    
    if ( bm == NULL) {
	int width = spec->width;
	int height = width*9/16;
	bm = init_bitmap(width, height, 3);
    }
    
    clear_bitmap(bm);
    display_array(bm, spec->pow, spec->width, 0, 0, 1.0, 0);
    coordinate_axes(bm, 200, 255,0);
    write_pam (fd, bm);
}

void spec_write_csv (int fd, specdata *spec, uint32_t freq, uint32_t fft_sr, int center, int64_t  str)
{
    uint32_t step = fft_sr/spec->width/1000;
    uint32_t freqstart = freq - fft_sr/2/1000;
    write_csv (fd, spec->width, step, freqstart, spec->pow, center, str );
}

void spec_set_freq(specdata *spec, uint32_t freq, uint32_t fft_sr)
{
    uint32_t step = fft_sr/spec->width/1000;
    uint32_t freqstart = freq - fft_sr/2/1000;
    
    for (int i = 0; i < spec->width; i++){
	spec->freq[i] = (double)(freqstart+step*i)/1000.0;
    }
}

void spec_write_graph (int fd, graph *g, specdata *spec)
{
    double *p = spec->pow;
    graph_range(g, spec->freq, p, spec->width);

    clear_bitmap(g->bm);
    display_array_graph( g, spec->freq, p, 50, spec->width);
    coordinate_axes(g->bm, 200, 255,0);
    write_pam (fd, g->bm);
}
