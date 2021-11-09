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

#define SMOOTH 6
static double *prepare_df(double *spec, int speclen, int smooth)
{
    double pmin = -1000;
    double pmax = 1000;
    double prange = 0;
    double *dspec = NULL;
    double avg = 0;
    double avg2 = 0;
    double var = 0;
    int i;

    dspec = df(spec, speclen); //differentiate spectrum
    smoothen(dspec, speclen, smooth); // smoothen differential 

    // determine statistics of the differential function
    if (find_range(dspec, speclen, &pmin, &pmax) < 0) return NULL;
    prange = pmax - pmin;
    for (i=0; i< speclen; i++){
	avg += dspec[i];
	avg2 += dspec[i]*dspec[i];
    }
    avg /= (double)speclen;
    avg2 /= (double)speclen;
    var = avg2 - avg*avg;
    
//    fprintf(stderr,"pmin: %f pmax: %f range: %f  avg: %f avg2: %f var: %f\n",
//	    pmin, pmax, prange, avg, avg2, var);
    // flatten differential function for easier peek finding
    for (i=0; i< speclen; i++){
	if (dspec[i]*dspec[i] < var) dspec[i]=0;
    }
    return dspec;
}

// smoothing over this number of points
int do_blindscan(blindscan *b)
{
    int speclen = b->speclen;
    double *spec = b->spec;
    double *dspec = NULL;
    int i,j;

    if (!(dspec = prepare_df(spec, speclen, SMOOTH))) return -1;
        
    int length = speclen;
    int pos = 0;
    for (i=0; i < MAXPEAK; i++){
	int start = 0;
	int stop = 0;
	int startend = 0;
	int stopstart = 0;
	double h = 0;
	
	if (!find_peak(dspec, length, pos , &b->peaks[i])) break;
	b->numpeaks++;
	start = b->peaks[i].start;
	stop = b->peaks[i].stop;
	startend = b->peaks[i].startend;
	stopstart = b->peaks[i].stopstart;
	pos = stop;

	// caclculate peak data
	b->peaks[i].width = (stopstart-startend)*b->freq_step;
	b->peaks[i].freq = b->freq_start
	    +b->freq_step*(startend+(stopstart-startend)/2);
	for (j=startend; j <= stopstart; j++){
	    h += spec[j];
	}
	b->peaks[i].height = h /(stopstart-startend); 
	b->peaks[i].slopestart = (spec[startend]-spec[start])/
	    (startend-start)*b->freq_step;
	b->peaks[i].slopestop = (spec[stopstart]-spec[stop])/
	    (stopstart-stop)*b->freq_step;
	
    }

    fprintf(stderr,"found %d peaks\n", i);
    for (i=0; i < b->numpeaks; i++){
	fprintf(stderr,
		"%3d. start: %d  stop: %d freq: %.2f MHz width: %.2f MHZ\n"
		"     height= %f upslope: %f downslope: %f \n\n", i+1,
		b->peaks[i].start,
		b->peaks[i].stop,
		b->peaks[i].freq,
		b->peaks[i].width,
		b->peaks[i].height,
		b->peaks[i].slopestart,
		b->peaks[i].slopestop
	    );
    
    }
    b->spec = dspec;
    return 0;
}

// search upwards slope
static int find_up(double *d, int l, int p)
{
    int i=p;

    while (d[i] <= 0 && i < l) i++;
    if (i >= l) return -1;    
    return i;
}

// search upwards slope backwards
static int find_up_bw(double *d, int l, int p)
{
    int i=p;

    while (d[i] <= 0 && i >= 0) i--;
    if (i < 0) return -1;    
    return i;
}

// search downwards slope
static int find_down(double *d, int l, int p)
{
    int i=p;

    while (d[i] >= 0 && i < l) i++;
    if (i >= l) return -1;    
    return i;
}

// search next plateau
static int find_flat(double *d, int l, int p)
{
    int i=p;

    while (d[i] != 0 && i < l) i++;
    if (i >= l) return -1;    
    return i;
}

// search next plateau backwards
static int find_flat_bw(double *d, int l, int p)
{
    int i=p;

    while (d[i] != 0 && i >= 0) i--;
    if (i < 0) return -1;    
    return i;
}

// find next peak from differential
int find_peak(double *dspec, int length, int pos, peak *p)
{
    int i = 0;
    int start = 0;
    int stop = 0;
    int startend = 0;
    int stopstart = 0;
    

    if ((i = find_up(dspec,length,pos)) < 0) return 0;
// found positive slope
    start = i;

    if ((i = find_flat(dspec,length,start)) < 0) return 0;
// slope ends
    startend = i;                            
	
    if ((i = find_down(dspec,length,i)) < 0) return 0;
// found negative slope
    stopstart = i;                        

    if ((i = find_flat(dspec,length,stopstart)) < 0) return 0;
// slope ends
    stop = i;                             

    if ((i = find_up_bw(dspec,length,stopstart)) < 0) return 0;
    if ( i != startend -1){
// found positive slope closer to negative slope
	startend = i+1;
	if ((i = find_flat_bw(dspec,length,startend)) < 0) return 0;
// slope starts here
	start = i;                            
	
    }

    if (startend && stop){
	p->start = start;
	p->stop = stop;
	p->startend = startend;
	p->stopstart = stopstart;
	return 1;
    }
    return 0;
}
