/*
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

#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include "dvb.h"

#define MAXTRY 5
#define BUFFSIZE (1024*188)

void print_help()
{
    dvb_print_tuning_options();
    fprintf(stderr,
	    "\n OTHERS:\n"
	    " -o filename  : output filename\n"
	    " -h           : this help message\n\n"
	);
}

int parse_args(int argc, char **argv, dvb_devices *dev,
	       dvb_fe *fe, dvb_lnb *lnb, char *fname)
{
    int out = 0;
    if (dvb_parse_args(argc, argv, dev, fe, lnb)< 0) return -1;

    optind = 1;

    while (1) {
	int option_index = 0;
	int c;
	static struct option long_options[] = {
	    {"fileout", required_argument , 0, 'o'},
	    {"stdout", no_argument , 0, 'O'},
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	
	c = getopt_long(argc, argv, "ho:O", long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {
	case 'o':
	    fname = strdup(optarg);
	    out = 1;
	    break;

	case 'O':
	    out = 1;
	    break;
	    
	case 'h':
	    print_help();
	    return -1;
	    break;
	default:
	    break;
	}
    }

    return out;
}	


int main(int argc, char **argv){
    dvb_devices dev;
    dvb_lnb lnb;
    dvb_fe fe;
    int lock = 0;
    int t=0;
    int out = 0;
    int fd = 0;
    char *filename = NULL;
    
    dvb_init(&dev, &fe, &lnb);
    if ((out=parse_args(argc, argv, &dev, &fe, &lnb, filename)) < 0)
	exit(2);
    dvb_open(&dev, &fe, &lnb);
    fprintf(stderr,
	    "Trying to tune freq: %d pol: %s sr: %d delsys: %s \n"
	    "               lnb_type: %d input: %d\n",
	    fe.freq, fe.pol ? "h":"v", fe.sr,
	    fe.delsys == SYS_DVBS ? "DVB-S" : "DVB-S2", lnb.type, fe.input);
    fprintf(stderr,"Tuning ");
    int re;
    if ((re=dvb_tune_sat( &dev, &fe, &lnb)) < 0) exit(1);
    while (!lock && t < MAXTRY){
	t++;
	fprintf(stderr,".");
	lock = read_status(dev.fd_fe);
	sleep(1);
    }
    fprintf(stderr,"%slock\n\n",lock ? " ": " no ");
    if (out){
	uint8_t *buf=(uint8_t *)malloc(BUFFSIZE);
	
	if (filename){
	    fprintf(stderr,"writing to %s\n", filename);
	    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		      00644);
	} else {
	    fprintf(stderr,"writing to stdout\n");
	    fd = fileno(stdout); 
	}
	while(1){
	    int re = read(dev.fd_dvr,buf,BUFFSIZE);
	    re = write(fd,buf,re);
	}

    }
}




