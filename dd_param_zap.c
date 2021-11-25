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

#define BUFFSIZE (1024*188)



uint8_t parse_nit(uint8_t *buf)
{
    int slen, tsp, c, nlen,nc;
    int ntune = 0;
    uint16_t nid;
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
	switch (buf[nc]){
	case 0x40:// network_name_descriptor
	    network_name = (char *)&buf[nc+2];
	    break;

	default:
//	    fprintf(stderr,"tag 0x%01x\n",buf[nc]);
	    break;
	}
	nc += 2+nlen;
    }
    tsp = ndl + 10;
    tsll =  (((buf[tsp]&0x0F) << 8) | buf[tsp+1]);
//    fprintf(stderr, "NIT(%02x): len %u nid %u snr %02x lsnr %02x "
//	    "ndl %02x  tsll %02x\n",
//     buf[0], slen, nid, buf[6], buf[7], ndl, tsll);
    printf("NIT (0x%02x): %s nid 0x%02x (%d/%d) \n",
	   buf[0], network_name ? network_name: " ",nid, buf[6], buf[7]);

    for (c = tsp + 2; c < slen; c += tdl) {
	uint16_t tsid, onid;
	tsid = (buf[c] << 8) | buf[c+1];
	onid = (buf[c+2] << 8) | buf[c+3];
	tdl =  ((buf[c+4]&0x0f) << 8) | buf[c+5];
//	fprintf(stderr, "tsid %02x onid %02x tdl %02x\n", tsid, onid, tdl);
	c += 6;
	ntune++;
	//fprintf(stderr, "tsid %02x onid %02x tdl %02x\n", tsid, onid, tdl);
	printf("TRANSPONDER: tsid=0x%02x onid=0x%02x", tsid, onid);

	switch (buf[c]){
	    
	case 0x43: // satellite
	    freq = getbcd(buf + c + 2, 8) *10;
	    srate = getbcd(buf + c + 9, 7) / 10;
	    pol = 1 ^ ((buf[c + 8] & 0x60) >> 5); // H V L R
	    delsys = ((buf[c + 8] & 0x04) >> 2) ? SYS_DVBS2 : SYS_DVBS;
	    fec = buf[c + 12] & 0x0f;
	    printf(" freq=%u  pol=%u  sr=%u  fec=%u delsys=%d\n"
		  , freq, pol, srate, fec, delsys);
	    break;
	    
	case 0x44: // cable
	    freq =  getbcd(buf + c + 2, 8) / 10000;
	    srate = getbcd(buf + c + 9, 7) / 10;
	    delsys = SYS_DVBC_ANNEX_A;
	    printf(" freq = %u  sr = %u  delsys=%d\n",
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

	default:
//	    fprintf(stderr,"tag 0x%01x\n",buf[c]);
	    break;
	}
    }
    return buf[7];
}


uint8_t parse_sdt(uint8_t *buf)
{
    int slen=0;
    int ntune = 0;
    int c=0;
    uint16_t tsid, onid;
    
    slen = (((buf[1]&0x0F) << 8) | buf[2])-3;
    tsid = (buf[3] << 8) | buf[4];
    onid = (buf[8] << 8) | buf[9];
    printf("SDT(0x%02x): tsid 0x%02x onid 0x%02x (%d/%d)\n",
	   buf[0], tsid, onid, buf[6], buf[7]);
    c = 11;
    while ( c < slen-4 ){
	int cc = 0;
	char *name = NULL;
	uint16_t sid = (buf[c] << 8) | buf[c+1];
	uint16_t tdl =  ((buf[c+3]&0x0f) << 8) | buf[c+4];
	printf ("SERVICE: sid 0x%04x ",sid);
	cc = 5;
	while (cc < tdl){
	    uint8_t tag = buf[c+cc];
	    uint8_t dl = buf[c+1+cc];
	    switch (tag){
	    case 0x48: //service descriptor
	    {
		char *prov=NULL;
		char *name=NULL;
		printf("type 0x%02x ", buf[c+cc+2]);

		cc += 3;
		int l = buf[c+cc];
		cc++;
		if (l){
		    prov = malloc(sizeof(char)*l+1);
		    memcpy(prov,buf+c+cc,l);
		    prov[l] = 0x00;
		    dvb2txt(prov);
		    printf("provider %s ", prov);
		}
		cc += l;
		int ll = buf[c+cc];
		cc++;
		if (ll){
		    name = malloc(sizeof(char)*ll+1);
		    memcpy(name,buf+c+cc,ll);
		    name[ll] = 0x00;
		    dvb2txt(name);
		    printf("name %s ", name);
		}
		cc += ll;
		break;
	    }	
	    default:
		break;
	    }
	    printf("\n");

	}
	c+=tdl+5;
    }
    return buf[7];
}

void print_help()
{
    dvb_print_tuning_options();
    fprintf(stderr,
	    "\n OTHERS:\n"
	    " -N           : get NIT\n"
	    " -S           : get SDT\n"
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
	    {"sdt", required_argument , 0, 'S'},
	    {"stdout", no_argument , 0, 'O'},
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	
	c = getopt_long(argc, argv, "ho:ONS", long_options, &option_index);
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

	case 'S':
	    out = 3;
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
    uint8_t sec_buf[4096];
    
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

    while (!lock){
	t++;
	fprintf(stderr,".");
	lock = read_status(dev.fd_fe);
	sleep(1);
    }
    if (lock == 2) {
	fprintf(stderr," tuning timed out\n");
	exit(2);
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
//	    NIT  *nit = dvb_get_nit(sec_buf);
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

	case 3:
	    close(dev.fd_dmx);
	    if ((fdmx = dvb_open_dmx_section_filter(&dev,0x11 , 0x42,
						    0,0x000000FF,0)) < 0)
		exit(1); 

	    re = 0;
	    while ( (re = read(fdmx, sec_buf, 4096)) <= 0) sleep(1);
	    int nsdt = parse_sdt(sec_buf);
//	    SDT  *sdt = dvb_get_sdt(sec_buf);
	    close(fdmx);
	    if (nsdt){
		for (int i=1; i < nsdt+1; i++){
		if ((fdmx = dvb_open_dmx_section_filter(&dev,0x11 , 0x42,
							(uint32_t)i,
							0x000000ff,0)) < 0)
		    exit(1); 
		while ( (re = read(fdmx, sec_buf, 4096)) <= 0) sleep(1);
		nsdt = parse_sdt(sec_buf);
		close(fdmx);
		}
	    }
	    break;

	default:
	    break;
	}
    }
}



