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
		fprintf(stderr,"unaligned buffer error\n");
		fprintf(stderr,"0x%02x 0x%02x 0x%02x 0x%02x %d\n",ibuf[0],ibuf[1],ibuf[2],ibuf[3],s);
		exit(0x47);
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
    fftw_plan p;
    
    double max = 0;
    double min = 0;
        
    if (spec->use_window){
	window = KaiserWindow(num, alpha);
    }
    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * num);
    fftw_complex *out = in;
    p = fftw_plan_dft_1d(num, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    int count = 0;
    for (int i = 0; count < maxrun; i++){
	if ( spec_read_fft_data(fdin,in,num) < 0) return -1;
	if (window){
	    for (int i = 0; i < num; i += 1)
	    {
		in[i] = window[i] * in[i];
	    }
	}
	fftw_execute(p); /* repeat as needed */
	count += fft_power_log(in, pow, num);
    }
    fftw_destroy_plan(p);
    
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

void spec_write_csv (int fd, specdata *spec, uint32_t freq, uint32_t fft_sr, int center, int64_t  str, int min)
{
    uint32_t step = fft_sr/spec->width/1000;
    uint32_t freqstart = freq - fft_sr/2/1000;
    write_csv (fd, spec->width, step, freqstart, spec->pow, center, str, min );
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
    display_array_graph( g, spec->freq, p, 50, spec->width,0);
    coordinate_axes(g->bm, 200, 255,0);
    write_pam (fd, g->bm);
}

void print_spectrum_options()
{
    fprintf(stderr,
	    "\n SPECTRUM OPTIONS:\n"
	    " -k           : use Kaiser window before FFT\n"
	    " -l alpha     : parameter of the Kaiser window\n"
	    " -n number    : number of FFTs averaging (default 1000)\n"
	    " -q           : faster FFT\n"
	);
}

int parse_args_spectrum(int argc, char **argv, specdata *spec)
{
    int use_window = 0;
    double alpha = 2.0;
    int nfft = 1000; //number of FFTs for average
    int full = 0;
    int width = FFT_LENGTH;
    int height = 9*FFT_LENGTH/16;
    char *nexts= NULL;
    uint32_t lnb = 0;
    opterr = 0;
    optind = 1;
    char **myargv;

    myargv = malloc(argc*sizeof(char*));
    memcpy(myargv, argv, argc*sizeof(char*));
    
    while (1) {
	int option_index = 0;
	int c;
	static struct option long_options[] = {
	    {"Kaiserwindow", no_argument, 0, 'k'},
	    {"alpha", required_argument, 0, 'l'},
	    {"nfft", required_argument, 0, 'n'},	    
	    {"quick", no_argument, 0, 'q'},
	    {0, 0, 0, 0}
	};
	
	c = getopt_long(argc, myargv, 
			"kl:n:q",
			long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {

	case 'k':
	    use_window = 1;
	    break;
	    
	case 'l':
	    alpha = strtod(optarg, NULL);	    
	    break;
	    
	case 'n':
	    nfft = strtoul(optarg, NULL, 10);
	    break;

	case 'q':
	    width = FFT_LENGTH/2;
	    break;


	default:
	    break;
	    
	}
    }

    height = 9*width/16;
    if (init_specdata(spec, width, height, alpha,
		      nfft, use_window) < 0) {
	exit(3);
    }

    return 0;
}
