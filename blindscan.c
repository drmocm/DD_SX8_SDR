#include "blindscan.h"

void init_blindscan (blindscan *b, double *spec, double *freq, int speclen)
{
    b->spec = spec;
    b->freq_start  = freq[0];
    b->freq_end  = freq[speclen];
    b->freq_step  = (freq[speclen] - freq[0])/speclen;
    b->speclen = speclen;
}

    
