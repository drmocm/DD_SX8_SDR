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
#include <pthread.h>
#include "dvb.h"
#include "dvb_service.h"
#include "dvb_print.h"

#define BUFFSIZE (1024*188)

void print_help()
{
    dvb_print_tuning_options();
    err(
	    "\n OTHERS:\n"
	    " -N           : get NIT\n"
	    " -S           : get SDT\n"
	    " -P           : get PAT and PMTs\n"
	    " -F           : do full NIT scan\n"
	    " -C           : keep device open and check strength\n"
	    " -o filename  : output filename\n"
	    " -h           : this help message\n\n"
	);
}

int parse_args(int argc, char **argv, dvb_devices *dev,
	       dvb_fe *fe, dvb_lnb *lnb, char *fname)
{
    int out = 0;
    int max =0;
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
	    {"max", no_argument , 0, 'M'},
	    {"help", no_argument , 0, 'h'},
	    {0, 0, 0, 0}
	};
	
	c = getopt_long(argc, argv, "ho:OMNSPFC", long_options, &option_index);
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
	    
	case 'M':
	    max = 1;
	    break;
	    
	case 'C':
	    out = 7;
	    break;
	    
	case 'h':
	    print_help();
	    return -1;
	    break;
	default:
	    break;
	}
    }

    if (max && out ==5) out = 6;
    return out;
}


satellite *full_nit_search(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb, int max)
{
    NIT **nits = NULL;
    int n = 0;
    uint16_t tsid = 0;
    satellite *sat;
    
    err("Full NIT search\n");

    nits = get_full_nit(dev, fe, lnb);
    if (!nits){
	err("NIT not found\n");
	return NULL;
    }
    n = nits[0]->nit->last_section_number+1;

    if(!(sat  = (satellite *) malloc(sizeof(satellite)))){
	err("Not enough memory for satellite\n");
	return NULL;
    }

    sat->nit = nits;
    sat->nnit = nits[0]->nit->last_section_number+1;
    dvb_copy_dev(&sat->dev, dev);
    dvb_copy_lnb(&sat->lnb, lnb);
    sat->ntrans = 0;
    for  (int i =0; i < sat->nnit; i++){
	sat->ntrans += sat->nit[i]->trans_num;
    }

    sat->trans = (transport *) malloc(sat->ntrans*sizeof(transport));
    memset(sat->trans, 0, sat->ntrans*sizeof(transport));
    int k= 0;
    for  (int i =0; i < sat->nnit; i++){
	for (int j=0; j < sat->nit[i]->trans_num; j++){
	    transport *trans = &sat->trans[k];
	    trans->nit_transport = sat->nit[i]->transports[j];
	    if (set_frontend_with_transport(fe,trans->nit_transport)){
		err("Could not set frontend\n");
		exit(1);
	    }
	    dvb_copy_fe(&trans->fe, fe);
	    if (lnb->lofs)
		trans->fe.hi = (trans->fe.freq > lnb->lofs) ? 1 : 0;
	    k++;
	}
    }
    sat->delsys = fe->delsys;
    
    dvb_sort_sat(sat);
    for (k = 0; k < sat->ntrans; k++){
	transport *trans = sat->trans_freq[k];
	trans->sat = sat;
    }

    if (!max){
	for (k = 0; k < sat->ntrans; k++){
	    transport *trans = sat->trans_freq[k];
	    scan_transport(dev, lnb, trans);
	}
    } else {
	switch (sat->delsys){

	case SYS_DVBS:
	case SYS_DVBS2:
	    switch (lnb->type){
	    case UNIVERSAL:
		break;
	    case INVERTO32:
		close(dev->fd_fe);
		close(dev->fd_dvr);
		close(dev->fd_dmx);
		pthread_mutex_t lock;
		if (pthread_mutex_init(&lock, NULL) != 0)
		{
		    printf("\n mutex init failed\n");
		    return NULL;
		}
		for (k = 0; k < sat->ntrans; k+= max){
		    for (int i=0; i < max && i+k < sat->ntrans; i++){
			transport *trans = sat->trans_freq[k+i];
			err("start thread for frontend %d %d/%d \n",i,k+i,sat->ntrans);
			if (thread_scan_transport(dev->adapter, lnb, trans, i, &lock) < 0) exit(1);
		    }
		    for (int i = 0; i< max && i+k < sat->ntrans; i++){
			transport *trans = sat->trans_freq[k+i];
			pthread_join(trans->tMux, NULL);
			err("scan done %d %d\n",i,k);
		    }
		    
		}
		break;
	    default:
		err("Can't use max option with this lnb type (%d)\n"
		    "Using standard method\n", lnb->type);
		for (k = 0; k < sat->ntrans; k++){
		    transport *trans = sat->trans_freq[k];
		    trans->sat = sat;
		    scan_transport(dev, lnb, trans);
		}
		break;
	    }
	    break;

	case SYS_DVBC_ANNEX_A:
	    close(dev->fd_fe);
	    close(dev->fd_dvr);
	    close(dev->fd_dmx);
	    pthread_mutex_t lock;
	    for (k = 0; k < sat->ntrans; k+= max){
		for (int i=0; i < max && i+k < sat->ntrans; i++){
		    transport *trans = sat->trans_freq[k+i];
		    err("start thread for frontend %d %d/%d \n",i,k,sat->ntrans);
		    if (thread_scan_transport(dev->adapter, lnb, trans, i, NULL) < 0) exit(1);
		}
		for (int i = 0; i< max && i+k < sat->ntrans; i++){
		    transport *trans = sat->trans_freq[k+i];
		    pthread_join(trans->tMux, NULL);
		    err("scan done %d %d\n",i,k);
		}

	    }
	    break;
	    
	default:
	    err("Can't use max option with this delivery system (%d)\n",
		    "Using standard method\n", sat->delsys);
	    for (k = 0; k < sat->ntrans; k++){
		transport *trans = sat->trans_freq[k];
		trans->sat = sat;
		scan_transport(dev, lnb, trans);
	    }
	    break;
	}

    }
    return sat;
}

json_object *search_nit(dvb_devices *dev, uint8_t table_id)
{
    int fdmx;
    int re = 0;
    NIT **nits = NULL;
    int nnit = 0;
    
    err("Searching NIT\n");
    if (!(nits = get_all_nits(dev, table_id))){
	err("NIT not found\n");
	return NULL;
    }

    return dvb_all_nit_json(nits);
}

void search_sdt(dvb_devices *dev)
{
    int fdmx;
    int re = 0;
    SDT **sdts = NULL;
    int n = 0;
    
    err("Searching SDT\n");
    if(!(sdts = get_all_sdts(dev))){
	err("SDT not found\n");
	exit(1);
    }
    json_object *jobj = dvb_all_sdt_json(sdts);
    fprintf (stdout,"%s\n",
	     json_object_to_json_string_ext(jobj,
					    JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_SPACED));
}

void search_pat(dvb_devices *dev)
{
    int fdmx;
    uint8_t sec_buf[4096];
    int re = 0;
    PAT **pats = NULL;
    int npat = 0;

    err("Searching PAT\n");
    pats = get_all_pats(dev);
    if (pats){
	json_object *jobj = dvb_all_pat_json(pats);
	npat = pats[0]->pat->last_section_number+1;

	json_object *jpmts = json_object_new_array();
	for (int n=0; n < npat; n++){
	    for (int i=0; i < pats[n]->nprog; i++){
		int npmt = 0;
		uint16_t pid = pats[n]->pid[i];
		if (!pats[n]->program_number[i]) continue;
		err("Searching PMT 0x%02x for program 0x%04x\n",
			pid,pats[n]->program_number[i]);
		PMT  **pmt = get_all_pmts(dev, pid);
		if (pmt){
			json_object_array_add(jpmts, dvb_all_pmt_json(pmt));
		}
	    }
	}
	json_object_object_add(jobj, "PMTs", jpmts);	
    fprintf (stdout,"%s\n",
	     json_object_to_json_string_ext(jobj,
					    JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_SPACED));
    }
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
    int re=0;
    int max = 0;
    
    dvb_init(&dev, &fe, &lnb);
    if ((out=parse_args(argc, argv, &dev, &fe, &lnb, filename)) < 0)
	exit(2);

    if (out == 6){
	int k = 0;
	int done = 0;
	do {
	    int fd = 0;
	    if ( (fd = open_fe(dev.adapter, k)) < 0){
		done =1;
	    } else {
		k++;
		close(fd);
	    }
	} while(!done);
	max = k;
	err ("Found %d frontends\n",max);
    }
    
    dvb_open(&dev, &fe, &lnb);
    if ((lock = dvb_tune(&dev, &fe, &lnb)) != 1) exit(lock);

    if (out && lock){
	switch (out) {
	    
	case 1:
	{
	    uint8_t *buf=(uint8_t *)malloc(BUFFSIZE);
	
	    if (filename){
		err("writing to %s\n", filename);
		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
			  00644);
	    } else {
		err("writing to stdout\n");
		fd = fileno(stdout); 
	    }
	    while(lock){
		int re = read(dev.fd_dvr,buf,BUFFSIZE);
		re = write(fd,buf,re);
	    }
	    break;
	}
	case 2:
	{
	    json_object *jobj = json_object_new_object();
	    json_object *jnit = search_nit(&dev,0x40);
	    if (jnit) json_object_object_add(jobj,"NIT actual",jnit);
	    jnit = search_nit(&dev,0x41);
	    if (jnit) json_object_object_add(jobj,"NIT other",jnit);
	    fprintf (stdout,"%s\n",
		     json_object_to_json_string_ext(jobj,
						    JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_SPACED));
	    break;
	}

	case 3:
	    search_sdt(&dev);
	    break;

	case 4:
	    search_pat(&dev);
	    break;

	case 5:
	case 6:
	{
	    json_object *jobj =
		dvb_satellite_json(full_nit_search(&dev, &fe, &lnb, max));
	    fprintf (stdout,"%s\n",
		     json_object_to_json_string_ext(jobj,
						    JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_SPACED));
	    break;
	}

	case 7:
	    close(dev.fd_dvr);
//	    close(dev.fd_dmx);
	    
	    while (1) {
		fe_status_t stat;
		int64_t str, cnr;
                
		stat = dvb_get_stat(dev.fd_fe);
		str = dvb_get_strength(dev.fd_fe);
		cnr = dvb_get_cnr(dev.fd_fe);
                
		printf("stat=%02x, str=%" PRId64 ".%03udBm, "
		       "snr=%" PRId64 ".%03uddB \n",
		       stat, str/1000, abs(str%1000),
		       cnr/1000, abs(cnr%1000));
                sleep(1);
	    }

	
	default:
	    break;
	}
    }
}
