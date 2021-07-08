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


int do_blindscan(blindscan *b)
{
    int speclen = b->speclen;
    double pmin = 0;
    double pmax = 0;
    double prange = 0;
    double *spec = b->spec;
    double *dspec = NULL;
    double *ddspec = NULL;
    
    if (find_range(spec, speclen, &pmin, &pmax) < 0) return -1;
    prange = pmax - pmin;

    for (int i=0; i< speclen; i++){
	spec[i] -= pmin;                // min is zero
	spec[i] = spec[i]*100.0/prange; // percentage of max
    }
    dspec = df(b->spec,b->speclen);
    ddspec = ddf(b->spec,b->speclen);
    
    return 0;
}
