#ifndef _DVB_SERVICE_H_
#define _DVB_SERVICE_H_

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <linux/dvb/frontend.h>
#include "dvb.h"


#define NORDIG 0x00000029

#define MAXSTREAM 500
#define MAXSERV 500
#define MAXDESC 500
#define MTRANS  500
#define MAXSECT 4096
#define MAXSDT  1024
#define MAXNIT  1024
#define MAXEIT  4096
#define MAXBAT  1024
#define MAXPAT  1024
#define MAXPMT  1024
#define MAXCAT  1024
#define MAXTDT  8
#define MAXTOT  32

static const char *DVB_POL[] = {"linear-horizontal", "linear-vertical",
    "circular-left", "circulra-right"};
static const char *DVB_MOD[] = {"Auto", "QPSK", "8PSK", "16QAM"};
static const char *DVB_MODC[] ={"not defined","16-QAM","32-QAM","64-QAM",
    "128-QAM","256-QAM","reserved"};
static const double DVB_roff[] ={0.25, 0.35, 0.20, 0};
static const char *DVB_FECO[] ={"not defined","no outer FEC coding",
    "RS(204/188)","reserved"};
static const char *DVB_FEC[] ={"not defined", "1/2" ,"2/3", "3/4","5/6","7/8","8/9",
    "3/5","4/5","9/10","reserved","no conv. coding"};

static const char *DVB_H[] = {
    "reserved",
    "DVB hand-over to an identical service in a neighbouring country",
    "DVB hand-over to a local variation of the same service",
    "DVB hand-over to an associated service",
    "reserved"
};
	
static const char *DVB_L[] = {
    "reserved","information service","EPG service",
    "CA replacement service",
    "TS containing complete Network/Bouquet SI",
    "service replacement service",
    "data broadcast service","RCS Map","mobile hand-over",
    "System Software Update Service (TS 102 006 [11])",
    "TS containing SSU BAT or NIT (TS 102 006 [11])",
    "IP/MAC Notification Service (EN 301 192 [4])",
    "TS containing INT BAT or NIT (EN 301 192 [4])"};

typedef struct descriptor_t {
    uint8_t tag;
    uint8_t len;
    uint8_t *data;
} descriptor;

typedef struct section_t {
    uint8_t    table_id;
    uint8_t    section_syntax_indicator;                
    uint16_t   section_length;
    uint16_t   id;
    uint8_t    version_number;
    uint8_t    current_next_indicator;
    uint8_t    section_number;
    uint8_t    last_section_number;
    uint8_t    data[MAXSECT];
} section;

typedef struct PAT_t {
    section *pat;
    int nprog;
    uint16_t program_number[MAXSERV];
    uint16_t pid[MAXSERV];
} PAT;

typedef struct pmt_stream_t {
    uint8_t  stream_type;
    uint16_t elementary_PID;
    uint16_t ES_info_length;
    int        desc_num;
    descriptor *descriptors[MAXDESC];
} pmt_stream;

typedef struct PMT_t {
    section    *pmt;
    uint16_t   PCR_PID;
    uint16_t   program_info_length;
    int        desc_num;
    descriptor *descriptors[MAXDESC];
    int        stream_num;
    pmt_stream *stream[MAXSTREAM];
} PMT;

typedef struct nit_transport_t {
    uint16_t   transport_stream_id;
    uint16_t   original_network_id;
    uint16_t   transport_descriptors_length;
    int        desc_num;
    descriptor *descriptors[MAXDESC];
} nit_transport;

typedef struct NIT_t {
    section       *nit;
    uint16_t      network_descriptor_length;
    int           ndesc_num;
    descriptor    *network_descriptors[MAXDESC];
    uint16_t      transport_stream_loop_length;
    int           trans_num;
    nit_transport *transports[MTRANS];
} NIT;

typedef struct sdt_service_t {
    uint16_t service_id;
    uint8_t  EIT_schedule_flag;
    uint8_t  EIT_present_following_flag;
    uint8_t  running_status;
    uint8_t  free_CA_mode;
    uint16_t descriptors_loop_length;
    int        desc_num;
    descriptor *descriptors[MAXDESC];
} sdt_service;

typedef struct SDT_t {
    section    *sdt;
    uint16_t   original_network_id;
    int        service_num;
    sdt_service *services[MAXSERV];
} SDT;

struct satellite_t;
struct transport_t;

typedef struct service_t{
    uint16_t id;
    struct satellite_t *sat;
    struct tranport_t *trans;
    sdt_service *sdt_service;
    PMT **pmt;
} service;

typedef struct tranport_t{
    struct satellite_t *sat;
    int lock;
    int nsdt;
    SDT **sdt;
    dvb_fe fe;
    nit_transport *nit_transport;
    int npat;
    PAT **pat;
    int nserv;
    service *serv;
    pthread_t tMux;
} transport;

typedef struct satellite_t{
    enum fe_delivery_system delsys;
    dvb_devices dev;
    dvb_lnb lnb;
    int nnit;
    NIT **nit;
    int ntrans;
    transport *trans;

    transport **trans_freq;

    int n_lh_trans;
    transport **l_h_trans;
    int n_lv_trans;
    transport **l_v_trans;
    int n_uh_trans;
    transport **u_h_trans;
    int n_uv_trans;
    transport **u_v_trans;
} satellite;
    
uint32_t getbcd(uint8_t *p, int l);
const char *service_type(uint8_t type);
const char *stream_type(uint8_t type);

void dvb_delete_pat(PAT *pat);
void dvb_delete_pmt(PMT *pmt);
void dvb_delete_sdt(SDT *sdt);
void dvb_delete_nit(NIT *nit);

section *dvb_get_section(uint8_t *buf);
PAT *dvb_get_pat(uint8_t *buf,section * sec);
PMT *dvb_get_pmt(uint8_t *buf, section *sec);
NIT  *dvb_get_nit(uint8_t *buf, section *sec);
SDT *dvb_get_sdt(uint8_t *buf, section *sec);

section **get_all_sections(dvb_devices *dev, uint16_t pid, uint8_t table_id);
PAT  **get_all_pats(dvb_devices *dev);
PMT  **get_all_pmts(dvb_devices *dev, uint16_t pid);
NIT  **get_all_nits(dvb_devices *dev, uint8_t table_id);
SDT  **get_all_sdts(dvb_devices *dev);

nit_transport *dvb_get_nit_transport(uint8_t *buf);
sdt_service *dvb_get_sdt_service(uint8_t *buf);
descriptor *dvb_get_descriptor(uint8_t *buf);

descriptor  *dvb_find_descriptor(descriptor **descs, int ndesc, uint8_t tag);
int set_frontend_with_transport(dvb_fe *fe, nit_transport *trans);
int get_all_services(transport *trans, dvb_devices *dev);
NIT **get_full_nit(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb);
void dvb_sort_sat(satellite *sat);
void scan_transport(dvb_devices *dev, dvb_lnb *lnb, transport *trans);
int thread_scan_transport(int adapter, dvb_lnb *lnb, transport *trans,
			  int m, pthread_mutex_t *lock);

#endif

