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
#include "dvb_service.h"

#define BUFFSIZE (1024*188)

void print_help()
{
    dvb_print_tuning_options();
    fprintf(stderr,
	    "\n OTHERS:\n"
	    " -N           : get NIT\n"
	    " -S           : get SDT\n"
	    " -P           : get PAT and PMTs\n"
	    " -F           : do full NIT scan\n"
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
	    {"pat", required_argument , 0, 'P'},
	    {"full_scan", required_argument , 0, 'F'},
	    {"stdout", no_argument , 0, 'O'},
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	
	c = getopt_long(argc, argv, "ho:ONSPF", long_options, &option_index);
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

	case 'P':
	    out = 4;
	    break;

	case 'F':
	    out = 5;
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


void full_nit_search(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb)
{
    NIT **nits =NULL;
    fprintf(stderr,"Full NIT search\n");

    nits = get_all_nits(dev, 0x40);

    
    
}

void search_nit(dvb_devices *dev, uint8_t table_id)
{
    int fdmx;
    int re = 0;
    NIT **nits = NULL;
    int nnit = 0;
    
    fprintf(stderr,"Searching NIT\n");
    nits = get_all_nits(dev, table_id);
    nnit = nits[0]->nit->last_section_number+1;
    for (int n=0; n < nnit; n++)
	dvb_print_nit(fileno(stdout), nits[n]);
}

void search_sdt(dvb_devices *dev)
{
    int fdmx;
    int re = 0;
    SDT **sdts = NULL;
    int n = 0;
    
    fprintf(stderr,"Searching NIT\n");
    sdts = get_all_sdts(dev);
    n = sdts[0]->sdt->last_section_number+1;
    for (int i=0; i < n; i++)
	dvb_print_sdt(fileno(stdout), sdts[i]);
}

void search_pat(dvb_devices *dev)
{
    int fdmx;
    uint8_t sec_buf[4096];
    int re = 0;
    PAT **pats = NULL;
    int npat = 0;

    fprintf(stderr,"Searching PAT\n");
    pats = get_all_pats(dev);
    if (pats){
	npat = pats[0]->pat->last_section_number+1;
	for (int i=0; i < npat; i++){
	    dvb_print_pat(fileno(stdout), pats[i]);
	}

	for (int n=0; n < npat; n++){
	    for (int i=0; i < pats[n]->nprog; i++){
		int npmt = 0;
		uint16_t pid = pats[n]->pid[i];
		if (!pats[n]->program_number[i]) continue;
		fprintf(stderr,"Searching PMT 0x%02x for program 0x%04x\n",
			pid,pats[n]->program_number[i]);
		PMT  **pmt = get_all_pmts(dev, pid);
		if (pmt){
		    npmt = pmt[0]->pmt->last_section_number+1;
		    for (int i=0; i < npmt; i++){
			dvb_print_pmt(fileno(stdout), pmt[i]);
		    }
		}
	    }
	}
    }
}

#define MAXTRY 10
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
    int re=0;
    
    dvb_init(&dev, &fe, &lnb);
    if ((out=parse_args(argc, argv, &dev, &fe, &lnb, filename)) < 0)
	exit(2);
    dvb_open(&dev, &fe, &lnb);

    switch (fe.delsys){
    case SYS_DVBC_ANNEX_A:
	fprintf(stderr,
		"Trying to tune freq: %dsr: %d delsys: DVB-C \n",
		fe.freq, fe.sr);
	fprintf(stderr,"Tuning ");
	if ((re=dvb_tune_c( &dev, &fe)) < 0) exit(1);
	break;
	
    case SYS_DVBS:
    case SYS_DVBS2:	
	fprintf(stderr,
		"Trying to tune freq: %d pol: %s sr: %d delsys: %s \n"
		"               lnb_type: %d input: %d\n",
		fe.freq, fe.pol ? "h":"v", fe.sr,
		fe.delsys == SYS_DVBS ? "DVB-S" : "DVB-S2", lnb.type, fe.input);
	fprintf(stderr,"Tuning ");
	if ((re=dvb_tune_sat( &dev, &fe, &lnb)) < 0) exit(1);
	break;


    case SYS_DVBT:
    case SYS_DVBT2:
    case SYS_DVBC_ANNEX_B:
    case SYS_ISDBC:
    case SYS_ISDBT:
    case SYS_ISDBS:
    case SYS_UNDEFINED:
    default:
	fprintf(stderr,"Delivery System not yet implemented\n");
	exit(1);
	break;
    }

    while (!lock && t < MAXTRY ){
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
	{
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
	}
	case 2:
	    search_nit(&dev,0x40);
	    search_nit(&dev,0x41);
	    break;

	case 3:
	    search_sdt(&dev);
	    break;

	case 4:
	    search_pat(&dev);
	    break;

	default:
	    break;
	}
    }
}



