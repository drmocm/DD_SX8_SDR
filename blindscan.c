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
static int find_peak(double *dspec, int length, int pos , peak *p);

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
    if (!(b->peaks = (peak *) malloc(MAXPEAK*
				       sizeof(peak)))){
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
    smoothen(dspec, speclen,18);
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
    int length = speclen;
    double *ds=dspec;
    int pos = 0;
    for (i=0; i < MAXPEAK; i++){
	int start = 0;
	int stop = 0;
	
	if (!find_peak(ds, length, pos , &b->peaks[i])) break;
	b->numpeaks++;
	start = b->peaks[i].start;
	stop = b->peaks[i].stop;
	pos = stop;
	b->peaks[i].width = stop*b->freq_step-start*b->freq_step;
	b->peaks[i].freq = b->freq_start+b->freq_step*start+b->peaks[i].width/2;
    }
    fprintf(stderr,"found %d peaks\n", i);
    for (i=0; i < b->numpeaks; i++){
	fprintf(stderr,"%d start: %d  stop: %d freq: %f width: %f\n", i+1,
		b->peaks[i].start,
		b->peaks[i].stop,
		b->peaks[i].freq,
		b->peaks[i].width
	    );
    
    }
    return 0;
}

int find_peak(double *dspec, int length, int pos, peak *p)
{
    int i = pos;
    int start = 0;
    int stop = 0;
    while (dspec[i] <= 0 && i < length) i++;
    if (i < length){
	start = i;
	while (dspec[i] >= 0 && i < length) i++;
	if (i < length){
	    while (dspec[i] < 0 && i < length) i++;
	    stop = i;
	} else return 0;
    } else return 0;
    if (start && stop){
	p->start = start;
	p->stop = stop;
	return 1;
    }
    return 0;
}
