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
#include "iod.h"
#include "dvb.h"

#define MAXTRY 5
#define BUFFSIZE (1024*188)


int parse_args(int argc, char **argv, io_data *iod)
{
    int out = 0;
    if (parse_args_io_tune(argc, argv, iod)< 0) return -1;

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
	    iod->filename = strdup(optarg);
	    out = 1;
	    break;

	case 'O':
	    out = 1;
	    break;
	    
	case 'h':
	    print_tuning_options();
	    return -1;
	    break;
	default:
	    break;
	}
    }
    return out;
}	


int main(int argc, char **argv){
    io_data iod;
    int lock = 0;
    int t=0;
    int out = 0;
    int fd = 0;
    
    init_io(&iod);
    if ((out=parse_args(argc, argv, &iod)) < 0)
	exit(2);
    open_io(&iod);
    iod.id = DVB_UNDEF;
    fprintf(stderr,"out %d\n", out);

    fprintf(stderr,
	    "Trying to tune freq: %d pol: %s sr: %d delsys: %s \n"
	    "               lnb_type: %d input: %d\n",
	    iod.freq, iod.pol ? "h":"v", iod.fft_sr,
	    iod.delsys == SYS_DVBS ? "DVB-S" : "DVB-S2", iod.lnb_type, iod.input);
    fprintf(stderr,"Tuning ");
    if (tune(&iod, 0) < 0) exit(1);

    while (!lock && t < MAXTRY){
	t++;
	fprintf(stderr,".");
	lock = read_status(iod.fe_fd);
	sleep(1);
    }
    fprintf(stderr,"%slock\n\n",lock ? " ": " no ");
    if (out){
	uint8_t *buf=(uint8_t *)malloc(BUFFSIZE);
	
	if (iod.filename){
	    fprintf(stderr,"writing to %s\n",iod.filename);
	    fd = open(iod.filename, O_WRONLY | O_CREAT | O_TRUNC,
		      00644);
	} else {
	    fprintf(stderr,"writing to stdout\n");
	    fd = fileno(stdout); 
	}
	while(1){
	    int re = read(iod.fdin,buf,BUFFSIZE);
	    re = write(fd,buf,re);
	}

    }
}




