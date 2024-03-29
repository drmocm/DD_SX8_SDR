#ifndef _DVB_PRINT_H_
#define _DVB_PRINT_H_
#include <string.h>
#include <json-c/json.h>
#include "dvb_service.h"

#define UTF8_CC_START 0xc2
#define SB_CC_RESERVED_80 0x80
#define SB_CC_RESERVED_81 0x81
#define SB_CC_RESERVED_82 0x82
#define SB_CC_RESERVED_83 0x83
#define SB_CC_RESERVED_84 0x84
#define SB_CC_RESERVED_85 0x85
#define CHARACTER_EMPHASIS_ON 0x86
#define CHARACTER_EMPHASIS_OFF 0x87
#define SB_CC_RESERVED_88 0x88
#define SB_CC_RESERVED_89 0x89
#define CHARACTER_CR_LF 0x8a
#define SB_CC_USER_8B 0x8b
#define SB_CC_USER_9F 0x9f

char *base64_encode(const uint8_t *data, int len, int *olen);
unsigned char *base64_decode(uint8_t *data, int len, int *olen);
void dvb2txt(char *in);
void dvb_print_data(int fd, uint8_t *b, int length, int step,
		    char *s, char *s2);
uint32_t dvb_print_descriptor(int fd, descriptor *desc, char *s,
			      uint32_t priv_id);
char *get_network_name(NIT **nits);

void dvb_print_section(int fd, section *sec);
void dvb_print_pat(int fd, PAT *pat);
void dvb_print_pmt(int fd, PMT *pmt);
void dvb_print_nit(int fd, NIT *nit);
void dvb_print_sdt(int fd, SDT *sdt);

json_object *dvb_pat_json(PAT *pat);
json_object *dvb_pmt_json(PMT *pmt);
json_object *dvb_nit_json(NIT *nit);
json_object *dvb_sdt_json(SDT *sdt);
json_object *dvb_all_pat_json(PAT **pats);
json_object *dvb_all_pmt_json(PMT **pmts);
json_object *dvb_all_nit_json(NIT **nits);
json_object *dvb_all_sdt_json(SDT **sdts);

json_object *dvb_satellite_json(satellite *sat);
json_object *dvb_lnb_json(dvb_lnb *lnb);
json_object *dvb_devices_json(dvb_devices *dev);
json_object *dvb_fe_json(dvb_fe *fe);


#endif
