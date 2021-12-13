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


satellite *full_nit_search(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb)
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
	scan_transport(dev, lnb, trans);
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
    nnit = nits[0]->nit->last_section_number+1;
    json_object *jobj = json_object_new_object();
    json_object *jarray = json_object_new_array();
    for (int n=0; n < nnit; n++){
	json_object_array_add (jarray,dvb_nit_json(nits[n]));
    }
    json_object_object_add(jobj, "NIT", jarray);

    return jobj;
//	dvb_print_nit(fileno(stdout), nits[n]);
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
    n = sdts[0]->sdt->last_section_number+1;
    json_object *jobj = json_object_new_object();
    json_object *jarray = json_object_new_array();
    for (int i=0; i < n; i++){
	json_object_array_add (jarray,dvb_sdt_json(sdts[i]));
    }
    json_object_object_add(jobj, "SDT", jarray);
    //dvb_print_sdt(fileno(stdout), sdts[i]);

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
    json_object *jobj = json_object_new_object();

    err("Searching PAT\n");
    pats = get_all_pats(dev);
    if (pats){
	npat = pats[0]->pat->last_section_number+1;
	json_object *jpat = json_object_new_array();
	for (int i=0; i < npat; i++){
//	    dvb_print_pat(fileno(stdout), pats[i]);
	    json_object_array_add(jpat, dvb_pat_json(pats[i]));
	}
	json_object_object_add(jobj, "PATs", jpat);
	
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
		    npmt = pmt[0]->pmt->last_section_number+1;
		    for (int i=0; i < npmt; i++){
			//dvb_print_pmt(fileno(stdout), pmt[i]);
			json_object *jpmt = dvb_pmt_json(pmt[i]);
			json_object_array_add(jpmts, jpmt);
		    }
		}
	    }
	}
	json_object_object_add(jobj, "PMTs", jpmts);	
    }
    fprintf (stdout,"%s\n",
	     json_object_to_json_string_ext(jobj,
					    JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_SPACED));
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
    
    dvb_init(&dev, &fe, &lnb);
    if ((out=parse_args(argc, argv, &dev, &fe, &lnb, filename)) < 0)
	exit(2);
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
	    full_nit_search(&dev, &fe, &lnb);
	default:
	    break;
	}
    }
}



