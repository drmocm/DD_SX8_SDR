#include <stdlib.h>
#include <string.h>
#include "numeric.h"

int find_range(double *f, int length, double *min, double *max)
{
    double  range = 0;
    double  mmax = *max;
    double  mmin = *min;
    *max = 0;
    *min = 1000000.0;
    
    for (int i=0; i < length; i++){
	if ( f[i] > mmax || f[i] <= mmin ) {
//	    printf("Warning: unusual value f[i]=%f\n",f[i]);
	    return -1;
	}
	if ( f[i] > *max ) *max = f[i];
	if ( f[i] < *min ) *min = f[i];
    }
    return 0;
    
}


int fft_power_log(fftw_complex *fft, double *pow, int numiq)
{

    double *logsum = (double *) malloc(sizeof(double)*numiq);
    if (!logsum) return 0;
    memset(logsum,0,sizeof(double)*numiq);
    
    for (int i = 0; i < numiq; i += 1)
    {
	int j;
	
	if (i < numiq/2) j =  i + numiq/2;
	else j = i-numiq/2;
	
	double real = creal(fft[j]);
	double imag = cimag(fft[j]);
	double sum = real*real+imag*imag;

	if (!isfinite(sum)){
//	    printf("not a number in at %d  %f\n",i,sum);
	    free(logsum);
	    return 0;
	}
	sum = log(sum)/log(10.0) * 10.0;
	
	logsum[i] = sum;
    }
    for (int i = 0; i < numiq; i += 1){
	pow[i] += logsum[i];
    }
    free(logsum);
    return 1;
}

void do_fft(fftw_complex *in, double *window, int num)
{
    fftw_complex *out = in;
    fftw_plan p;

    if (window){
	for (int i = 0; i < num; i += 1)
	{
	    in[i] = window[i] * in[i];
	}
    }
    
    p = fftw_plan_dft_1d(num, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    fftw_execute(p); /* repeat as needed */
    fftw_destroy_plan(p);

}

void smooth(double *f, int l)
{
    double av=0;
    double a = f[0];
    for (int i=1; i < l-1; i++){
	double b = f[i-1];
	double c = f[i];
	double m = fmax(fmin(a,b), fmin(fmax(a,b),c));
	a = f[i];
	f[i] = m;
	av += f[i];;
    }
    a = f[0];
    for (int i=1; i < l-1; i++){
	double b = f[i];
	f[i] = (a + f[i+1])/2.0;
	a = b;
    }
}


void df(double *f, double *df, int length)
{
    int i;

    for (i=2; i< length-2; i++){
	df[i] = (-f[i+2]+8.0*f[i+1]-8.0*f[i-1]+f[i-2])/12.0;
    }
    df[0] = 0;//(f[1]-f[0]);
    df[1] = 0;//(f[2]-f[0])/2.0;
    df[length-1] = 0;//(f[length-1]-f[length-2]);
    df[length-2] = 0;//(f[length-1]-f[length-3])/2.0;
}


        //   This function computes the modified Bessel Function of the first  
        //   kind and zero order. 


static double I0(double x)
{
    double ax = fabs(x);
    double Bessel = 0;
    double y = 0;

    if (ax < 3.75)
    {
	y = x / 3.75;
	y *= y;
	Bessel = 1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492
							      + y * (0.2659732 + y * (0.360768e-1 + y * 0.45813e-2)))));
    } else {
	y = 3.75 / ax;
	Bessel = (exp(ax) / sqrt(ax)) * (0.39894228 + y * (0.1328592e-1 + y *
								     (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2 + y * (-0.2057706e-1
																+ y * (0.2635537e-1 + y * (-0.1647633e-1 + y * 0.392377e-2))))))));
    }
    return Bessel;
}

double* KaiserWindow(int n, double alpha)
{
    double *w;
    double i0alpha = I0(alpha);

    w = (double *) malloc(sizeof(double)*n);
    
    for (int i = 0; i < n; i += 1 ){
	double tmp = (2.0 * i) / (n - 1) - 1;
	w[i] = I0(alpha * sqrt( 1 - tmp * tmp )) / i0alpha;
    }
    
    return w;
}
