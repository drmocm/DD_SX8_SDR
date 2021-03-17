#ifndef _NUMERIC_H_
#define _NUMERIC_H_
#include <math.h>
#include <complex.h>
#include <fftw3.h>

int find_range(double *f, int length, double *min, double *max);
int fft_power_log(fftw_complex *fft, double *pow, int numiq);
double* KaiserWindow(int n, double alpha);
void FFT(short dir, int m, double *x, double *y);
void df(double *f, double *df, int length);
void smooth(double *f, int l);
void do_fft(fftw_complex *in, double *window, int num);

#endif /* _NUMERIC_H_*/
 
