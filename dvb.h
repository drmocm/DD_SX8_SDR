#ifndef _DVB_H_
#define _DVB_H_

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/video.h>
#include <linux/dvb/net.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>

#define DTV_INPUT 71
#define DVB_UNDEF  (~0U)
#define AGC_ON    0x20000000
#define AGC_OFF   0x20020080
#define AGC_OFF_C 0x20020020

#define UNIVERSAL 0
#define UNICABLE1 1
#define UNICABLE2 2

typedef struct dvb_devices_t{
    int adapter;
    int num;
    int fd_fe;
    int fd_dmx;
    int fd_dvr;
    int fd_mod;
} dvb_devices;

typedef struct dvb_fe_t{
    enum fe_delivery_system delsys;
    uint32_t freq;
    uint32_t pol;
    uint32_t hi;
    uint32_t sr;
    uint32_t id;
    int input;
} dvb_fe;

typedef struct dvb_lnb_t{
    int type;
    int delay;
    uint32_t num;
    uint32_t lofs;
    uint32_t lof1;
    uint32_t lof2;
    uint32_t scif_slot;
    uint32_t scif_freq;
} dvb_lnb;

#define MAXSTREAM 100
#define MAXSERV 100
#define MAXDESC 100
#define MTRANS  200
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
    section    *pat;
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
    uint8_t       network_descriptor_length;
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

int set_fe_input(int fd, uint32_t fr,
		 uint32_t sr, fe_delivery_system_t ds,
		 uint32_t input, uint32_t id);
int diseqc(int fd, int lnb, int hor, int band);
int open_dmx(int adapter, int num);
int open_fe(int adapter, int num);
int open_dvr(int adapter, int num);
int tune_sat(int fd, int type, uint32_t freq, 
	     uint32_t sr, fe_delivery_system_t ds, 
	     uint32_t input, uint32_t id, 
	     uint32_t pol, uint32_t hi,
	     uint32_t lnb, uint32_t lofs, uint32_t lof1, uint32_t lof2,
	     uint32_t scif_slot, uint32_t scif_freq);
void stop_dmx( int fd );
int open_dmx_section_filter(int adapter, int num, uint16_t pid, uint8_t tid,
			    uint32_t ext, uint32_t ext_mask,
			    uint32_t ext_nmask);
int dvb_tune_sat(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb);
void dvb_init_dev(dvb_devices *dev);
void dvb_init_fe(dvb_fe *fe);
void dvb_init_lnb(dvb_lnb *lnb);
void dvb_init(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb);
void dvb_print_tuning_options();
int dvb_parse_args(int argc, char **argv, dvb_devices *dev,
		   dvb_fe *fe, dvb_lnb *ln);
void power_on_delay(int fd, int delay);
int read_status(int fd);
int get_stat(int fd, uint32_t cmd, struct dtv_fe_stats *stats);
void dvb_open(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb);
int dvb_open_dmx_section_filter(dvb_devices *dev, uint16_t pid, uint8_t tid,
			    uint32_t ext, uint32_t ext_mask,
			    uint32_t ext_nmask);

uint32_t getbcd(uint8_t *p, int l);
void dvb2txt(char *in);
NIT  *dvb_get_nit(uint8_t *buf);
SDT *dvb_get_sdt(uint8_t *buf);
section *dvb_get_section(uint8_t *buf);
nit_transport *dvb_get_nit_transport(uint8_t *buf);
sdt_service *dvb_get_sdt_service(uint8_t *buf);
descriptor *dvb_get_descriptor(uint8_t *buf);
void dvb_print_descriptor(int fd, descriptor *desc, char *s);
void dvb_print_nit(int fd, NIT *nit);
void dvb_print_sdt(int fd, SDT *sdt);

#endif

