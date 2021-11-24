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

uint8_t parse_nit(uint8_t *buf)
{
    int slen, tsp, c, nlen,nc;
    int ntune = 0;
    uint16_t pid, pnr, nid;
    uint16_t ndl, tsll, tdl=0, dl=0;
    uint16_t tsid, onid;
    char *network_name =NULL;
    uint32_t freq;
    uint32_t srate;
    uint8_t pol;
    uint8_t delsys;
    uint8_t fec;
    
    slen = (((buf[1]&0x0F) << 8) | buf[2]) +3;
    if (buf[1] & 0x80)
	slen -= 4;

    nid = (buf[3] << 8) | buf[4];
    ndl = (((buf[8]&0x0F) << 8) | buf[9]);
    nc = 10;
    while ( nc < 10+ndl){
	nlen = (int) buf[nc+1];
	fprintf(stderr,"tag 0x%01x\n",buf[nc]);
	switch (buf[nc]){
	case 0x40:// network_name_descriptor
	    fprintf(stderr,"name %s\n",&buf[nc+2]);
	    break;
	}
	nc += 2+nlen;
    }
    tsp = ndl + 10;
    tsll =  (((buf[tsp]&0x0F) << 8) | buf[tsp+1]);
    fprintf(stderr, "NIT(%02x): len %u nid %u snr %02x lsnr %02x "
	    "ndl %02x  tsll %02x\n",
	    buf[0], slen, nid, buf[6], buf[7], ndl, tsll);

    for (c = tsp + 2; c < slen; c += tdl) {
	uint16_t tsid, onid;
	tsid = (buf[c] << 8) | buf[c+1];
	onid = (buf[c+2] << 8) | buf[c+3];
	tdl =  ((buf[c+4]&0x0f) << 8) | buf[c+5];
	fprintf(stderr, "tsid %02x onid %02x tdl %02x\n", tsid, onid, tdl);
	c += 6;
	ntune++;
	switch (buf[c]){
	    
	case 0x43: // satellite
	    uint32_t freq = getbcd(buf + c + 2, 8) *10;
	    uint32_t srate = getbcd(buf + c + 9, 7) / 10;
	    uint8_t pol = 1 ^ ((buf[c + 8] & 0x60) >> 5); // H V L R
	    uint8_t delsys = ((buf[c + 8] & 0x04) >> 2) ? SYS_DVBS2 : SYS_DVBS;
	    uint8_t fec = buf[c + 12] & 0x0f;
	    fprintf(stderr," freq = %u  pol = %u  sr = %u  fec = %u delsys=%d\n"
		    , freq, pol, srate, fec, delsys);
	    break;
	    
	case 0x44: // cable
	    freq =  getbcd(buf + c + 2, 8) / 10000;
	    srate = getbcd(buf + c + 9, 7) / 10;
	    delsys = SYS_DVBC_ANNEX_A;
	    fprintf(stderr," freq = %u  sr = %u  delsys=%d\n",
		   freq, srate, delsys);
	    break;
	    
	case 0x5a: // terrestrial
	    freq = (buf[c+5]|(buf[c+4] << 8)|(buf[c+3] << 16)|(buf[c+2] << 24))*10;
	    delsys = SYS_DVBT;
	    break;
	    
	case 0xfa: // isdbt
	    freq = (buf[c+5]|(buf[c+4] << 8))*7000000;
	    delsys = SYS_ISDBT;
	    break;
	}
    }
    return buf[7];
}

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
	    {"nit", required_argument , 0, 'N'},
	    {"stdout", no_argument , 0, 'O'},
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	
	c = getopt_long(argc, argv, "ho:ON", long_options, &option_index);
	if (c==-1)
	    break;
	
	switch (c) {
	case 'o':
	    fname = strdup(optarg);
	    out = 1;
	    break;

	case 'N':
	    out = 2;
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

    if (out && lock){
	switch (out) {
	    
	case 1:
	    uint8_t *buf=(uint8_t *)malloc(BUFFSIZE);
	
	    if (filename){
		fprintf(stderr,"writing to %s\n", filename);
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
			  00644);
	    } else {
		fprintf(stderr,"writing to stdout\n");
		fd = fileno(stdout); 
	    }
	    while(lock){
		int re = read(dev.fd_dvr,buf,BUFFSIZE);
		re = write(fd,buf,re);
	    }
	    break;

	case 2:
	    close(dev.fd_dmx);
	    int fdmx;
	    uint8_t sec_buf[4096];
	    if ((fdmx = dvb_open_dmx_section_filter(&dev,0x10 , 0x40,
						    0,0x000000FF,0)) < 0)
		exit(1); 

	    re = 0;
	    while ( (re = read(fdmx, sec_buf, 4096)) <= 0) sleep(1);
	    int nnit = parse_nit(sec_buf);
	    close(fdmx);
	    if (nnit){
		for (int i=1; i < nnit+1; i++){
		if ((fdmx = dvb_open_dmx_section_filter(&dev,0x10 , 0x40,
							(uint32_t)i,
							0x000000ff,0)) < 0)
		    exit(1); 
		while ( (re = read(fdmx, sec_buf, 4096)) <= 0) sleep(1);
		nnit = parse_nit(sec_buf);
		close(fdmx);
		}
	    }
	    break;
	    
	default:
	    break;
	}
    }
}



