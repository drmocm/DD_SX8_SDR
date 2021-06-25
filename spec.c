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
    spec->data_points = NULL;
    spec->pow = NULL;
    spec->alpha = 3.0;
    spec-> maxrun = 2000;
    spec->width = 0;
    spec->height = 0;
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
    spec->height = height;
    
    if (!(spec->pow = (double *) malloc(spec->width*sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    return -1;
	}
    }

    if (spec->height){
	if (!( spec->data_points=(unsigned char *)
	       malloc(sizeof(unsigned char) *
		      width*height*3)))
	{
	    fprintf(stderr,"not enough memory\n");
	    return -1;
	}
	memset(spec->data_points,0,width*height*3*sizeof(char));
    }
    return 0;
}


static long getutime(){
        struct timespec t0;
        clock_gettime(CLOCK_MONOTONIC_RAW,&t0);
        return t0.tv_sec * (int)1e9 + t0.tv_nsec;
}


void plot(uint8_t *p, int x, int y, int width,
	  unsigned char R,
	  unsigned char G,
	  unsigned char B)
{
    int k = 3*x+3*width*y;

    p[k] = R;
    p[k+1] = G;
    p[k+2] = B;
}

void coordinate_axes(specdata *spec, unsigned char r,
			 unsigned char g, unsigned char b){
    int i;
    for (i = 0; i < spec->width; i++){
	// coordinate axes
	//plot(spec->data_points, i, spec->height/2, spec->width, r,g,b);
    }
    for (i = 0; i < spec->height; i++){
	plot(spec->data_points, spec->width/2, i, spec->width, r,g,b);
    }
}


void get_rgb(int val, uint8_t *R, uint8_t *G, uint8_t *B)
{
    *R = 0;
    *G = 0;
    *B = 0;

    if  (val < 64){
	*B = 4*val;
	*G = 4*val;
    } else {
	if (val >128) {
	    *G = val;
	} else {
	    *G = 2*val;
	    *R = 2*val;
	}
    }
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


double check_range(double *pow, int width,
		   double *ma, double *mi){
    
    double max = 0.0;
    double min = 40.0;
    double range = 0;
    for (int i=0; i < width; i++){
	if ( isfinite(pow[i]) && pow[i] > max ) max = pow[i];
	if ( isfinite(pow[i]) && pow[i] < min ) min = pow[i];
    }
    range = max-min;
    *mi = min;
    *ma = max;
    //  fprintf(stderr,"range %f min %f\n",range,min);
    return range;
}

#define BSIZE 100*(TS_SIZE-4)

void spec_display(specdata *spec, double *pow)
{
    uint8_t R = 0;
    uint8_t G = 0;
    uint8_t B = 0;
    double min = 0, max = 0, range = 0;
    int i;
    int width = spec->width;
    int height = spec->height;

    range = check_range(pow, width, &max, &min);
    
    for (i = 0; i < width; i++){
	if (!isfinite(pow[i])) continue;
	int q = (int)((double)height*(pow[i]-min)/range);

	get_rgb(q*255/height, &R, &G, &B);
	
	//FFT data
	int y = height-q;
//	fprintf(stderr,"range %f min %f\n",range,min);
//	fprintf (stderr,"y: %d q: %d pow: %f height %d\n",y,q,pow[i],height);
	plot(spec->data_points, i, y, spec->width, R,G,B);
    }
    coordinate_axes(spec, 200, 255,0);
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

#define DTIME 40 // msec
void spec_read_data (int fdin, specdata *spec)
{
    uint64_t maxd = 0;
    double *pow= spec->pow;

    clear_buffer(fdin);

    memset(pow,0,spec->width*sizeof(double));

    spec_fft(fdin, spec, pow, spec->width);
    spec_display(spec, pow);
}    


void spec_write_pam (int fd, specdata *spec){
    int size = spec->width*spec->height*3;
    
    write_pam (fd, spec->width, spec->height, spec->data_points);
    memset(spec->data_points,0,size*sizeof(char));
}

void spec_write_csv (int fd, specdata *spec, uint32_t freq, uint32_t fft_sr, int center){
    uint32_t step = fft_sr/spec->width/1000;
    uint32_t freqstart = freq - spec->width/2*step;
    write_csv (fd, spec->width, step, freqstart, spec->pow, center);
}

void write_pam (int fd, int width, int height, unsigned char *data_points)
{
    char HEAD[255];
    sprintf(HEAD,"P7\nWIDTH %d\nHEIGHT %d\nDEPTH 3\nMAXVAL 255\nTUPLTYPE RGB\nENDHDR\n",width,height);
    int headlen = strlen(HEAD);
    int size = width*height*3;    
    int we=0;
    we=write(fd,HEAD,headlen);
    we=write(fd,data_points,size);
}

void write_csv (int fd, int width, uint32_t step, uint32_t start_freq,
		double *pow, int center)
{
    FILE* fp = fdopen(fd, "w");
    int start = 0;
    int end = width;
    if (center){
	start = width/4;
	end = width/4*3;
    }
    for (int i = start; i < end; i++){
	fprintf(fp,"%.3f, %.2f\n",(double)(start_freq+step*i)/1000.0, pow[i]);
    }
    fflush(fp);
}

