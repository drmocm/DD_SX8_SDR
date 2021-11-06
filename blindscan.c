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
    double pmin = 0;
    double pmax = 1000;
    double prange = 0;
    double *spec = b->spec;
    double *dspec = NULL;
    double *ddspec = NULL;
    double avg = 0;
    double fac;
    int *bin;
    double binstep = 100.5/NBIN;
    int k,i;

    if (!(bin = (int *) malloc(MAXPEAK*sizeof(int)))){
	{
	    fprintf(stderr,"not enough memory\n");
	    exit(1);
	}
    }
    memset(bin,0,NBIN*sizeof(int));
    smooth(spec, speclen);
    
    smooth(spec, speclen);
    
    smooth(spec, speclen);
    
    if (find_range(spec, speclen, &pmin, &pmax) < 0) return -1;
    prange = pmax - pmin;
    fac = 100.0/prange;
    fprintf(stderr,"pmin: %f pmax: %f range: %f\n",pmin,pmax,fac);
    for (i=0; i< speclen; i++){
	k=0;
	spec[i] -= pmin;                // min is zero
	spec[i] = spec[i]*fac; // percentage of max
	avg += spec[i];
	k = (int)(spec[i]/binstep);
	bin[k]++;
	spec[i] = k*binstep;
    }
    avg = avg/(double)speclen;
    double mid=0;
    k=0;
    while (mid*100.0/speclen < 50.0 && k < NBIN){
	mid += bin[k];
	k++;
    }
    fprintf(stderr,"average:%f med:%f %f\n", avg,(k-1)*binstep,binstep );
    return 0;
}

int find_peak(double *spec, int length, peak *p)
{
    double avg = 0;
    int start = 0;
    int stop = 0;
    
    for (int i = 0; i < length; i++){
	avg += spec[i];
    }
    avg = avg/(double)length;

    int i=0;
    while ( spec[i] < avg && i < length ) i++;
    if (i < length) start = i;
    else return -1;
    while ( spec[i] > avg && i < length ) i++;
    if (i < length) stop = i;
    else return -1;

    p->width = stop - start;
    p->mid = start+ p->width/2;
    return stop;
}
