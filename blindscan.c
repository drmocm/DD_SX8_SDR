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

#include "blindscan.h"
#include "numeric.h"

#define MAXPEAK 100

void init_blindscan (blindscan *b, double *spec, double *freq, int speclen)
{
    b->freq_start  = freq[0];
    b->freq_end  = freq[speclen];
    b->freq_step  = (freq[speclen] - freq[0])/speclen;
    b->speclen = speclen;
    b->numpeaks = 0;
    
    if (!(b->spec = (double *) malloc(speclen*
				       sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    exit(1);
	}
    }
    memcpy(b->spec, spec, speclen*sizeof(double));
    if (!(b->peaks = (double *) malloc(MAXPEAK*
				       sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    exit(1);
	}
    }
    if (!(b->widths = (double *) malloc(MAXPEAK*
				       sizeof(double)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    exit(1);
	}
    }
}

#define NBIN 100
int do_blindscan(blindscan *b)
{
    int speclen = b->speclen;
    double pmin = -1000;
    double pmax = 1000;
    double prange = 0;
    double *spec = b->spec;
    double *dspec = NULL;
    double *ddspec = NULL;
    double avg = 0;
    double avg2 = 0;
    double var = 0;
    int i,j,k;

    dspec = df(spec, speclen);
    smoothen(dspec, speclen,45);
    if (find_range(dspec, speclen, &pmin, &pmax) < 0) return -1;
    prange = pmax - pmin;
    for (i=0; i< speclen; i++){
	avg += dspec[i];
	avg2 += dspec[i]*dspec[i];
    }
    avg /= (double)speclen;
    avg2 /= (double)speclen;
    var = avg2 - avg*avg;
    fprintf(stderr,"pmin: %f pmax: %f range: %f  avg: %f avg2: %f var: %f\n",
	    pmin, pmax, prange, avg, avg2, var);

    
    for (i=0; i< speclen; i++){
	if (dspec[i]*dspec[i] < var) dspec[i]=0;
    }
    b->spec = dspec;
    return 0;
}

int find_peak(double *spec, int length, peak *p, int min_width)
{
    double avg = 0;
    int start = 0;
    int stop = 0;
    int i,j,k;
    int pcount = 0; 
    int davg = min_width/2; //average over davg data points

    for (i = davg; i < length; i++){
	for (j = i-davg; j < i; j++){
	    avg += spec[j];
	}
	avg = avg/davg;
	if (spec[i] > avg) pcount++;
	else pcount=0;
	if (pcount == 1) start = i;
	if (pcount > min_width) break;
    }
    if (i < length - 2*davg){

    } else return length;
				    
    

    return stop;
}
