#ifndef _DVB_SERVICE_H_
#define _DVB_SERVICE_H_

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <linux/dvb/frontend.h>

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
    uint16_t       network_descriptor_length;
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

uint32_t getbcd(uint8_t *p, int l);
void dvb2txt(char *in);
NIT  *dvb_get_nit(uint8_t *buf);
SDT *dvb_get_sdt(uint8_t *buf);
section *dvb_get_section(uint8_t *buf);
nit_transport *dvb_get_nit_transport(uint8_t *buf);
sdt_service *dvb_get_sdt_service(uint8_t *buf);
descriptor *dvb_get_descriptor(uint8_t *buf);
void dvb_print_descriptor(FILE *fp, descriptor *desc, char *s);
void dvb_print_nit(int fd, NIT *nit);
void dvb_print_sdt(int fd, SDT *sdt);
void dvb_delete_sdt(SDT *sdt);
void dvb_delete_nit(NIT *nit);
void dvb_print_data(FILE *fp, uint8_t *b, int length, int step,
		    char *s, char *s2);

#endif

