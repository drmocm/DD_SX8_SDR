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
	    fprintf(stderr,"Warning: unusual value f[i]=%f  %f %f\n",f[i],mmin,mmax);
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


void smoothen(double *f, int l, int range)
{
    double a1 = 0;
    double a2 = 0;
    double a3 = 0;
    int r = 3*range;

    for (int j = 0; j < r/3; j++){
	a1 += f[j];
    }
    a1 /= r/3;

    for (int i = 0; i < l ; i+= r){
	a2 = 0;
	a3 = 0;
	for (int j = i; j < i+r/3; j++){
	    a2 += f[j+r/3];
	    a3 += f[j+2*r/3];
	}
	a2 /= r/3;
	a3 /= r/3;
	double m = fmax(fmin(a2,a1), fmin(fmax(a2,a1),a3));
	a1 = a3; 
	for (int j = i; j < i+r; j++){
	    f[j] = m;
	}
    }
}

void smooth(double *f, int l)
{
    double a = f[0];
    for (int i=1; i < l-1; i++){
	double b = f[i-1];
	double c = f[i];
	double m = fmax(fmin(a,b), fmin(fmax(a,b),c));
	a = f[i];
	f[i] = m;
    }
    a = f[0];
    for (int i=1; i < l-1; i++){
	double b = f[i];
	f[i] = (a + f[i+1])/2.0;
	a = b;
    }
}


double *df(double *f, int length)
{
    int i;
    double *df;

    if (!(df = (double *) malloc(length*sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    return NULL;
	}
    }


    for (i=2; i< length-2; i++){
	df[i] = (-f[i+2]+8.0*f[i+1]-8.0*f[i-1]+f[i-2])/12.0;
    }
    df[0] = 0;//(f[1]-f[0]);
    df[1] = 0;//(f[2]-f[0])/2.0;
    df[length-1] = 0;//(f[length-1]-f[length-2]);
    df[length-2] = 0;//(f[length-1]-f[length-3])/2.0;

    return df;
}

double *ddf(double *f, int length)
{
    int i;
    double *ddf;

    if (!(ddf = (double *) malloc(length*sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    return NULL;
	}
    }


    for (i=2; i< length-2; i++){
	ddf[i] = (-f[i+2]+16.0*f[i+1]-30*f[i]-16.0*f[i-1]+f[i-2])/12.0;
    }
    ddf[0] = 0;//(f[1]-f[0]);
    ddf[1] = 0;//(f[2]-f[0])/2.0;
    ddf[length-1] = 0;//(f[length-1]-f[length-2]);
    ddf[length-2] = 0;//(f[length-1]-f[length-3])/2.0;

    return ddf;
}


double *intf(double *f, int length){
    double sum = 0;
    double *F;
    
    if (!(F = (double *) malloc(length*sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    return NULL;
	}
    }

    F[0] = f[0];
    for (int i=1; i < length; i++){
	F[i] = F[i-1]+f[i]; 	    
    }

    return F;
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
