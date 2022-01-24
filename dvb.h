#ifndef _DVB_H_
#define _DVB_H_

#include <stdlib.h>
#include <getopt.h>
#include <sys/poll.h>
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

enum lnbtype {
    UNIVERSAL = 0,
    UNICABLE1 = 1,
    UNICABLE2 = 2,
    INVERTO32 = 3
};

static const uint32_t inverto32_slot[32] =
{
    1210, 1420, 1680, 2040,  984, 1020, 1056, 1092, 1128, 1164, 1256, 1292,
    1328, 1364, 1458, 1494, 1530, 1566, 1602, 1638, 1716, 1752, 1788, 1824,
    1860, 1896, 1932, 1968, 2004, 2076, 2112, 2148
};
	
#ifndef MAXTRY
#define MAXTRY 10
#endif

typedef struct dvb_devices_t{
    int adapter;
    int num;
    int fd_fe;
    int fd_dmx;
    int fd_dvr;
    int fd_mod;
    pthread_mutex_t *lock;
} dvb_devices;

typedef struct dvb_fe_t{
    enum fe_delivery_system delsys;
    uint32_t freq;
    uint32_t pol;
    uint32_t hi;
    uint32_t bandw;
    uint32_t mod;
    enum fe_code_rate fec;
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

void err(const char  *format,  ...);
const char *delsys_name(enum fe_delivery_system delsys);
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
int tune_c(int fd, uint32_t freq, uint32_t bandw, uint32_t sr,
	   enum fe_code_rate fec, uint32_t mod);
int tune_terr(int fd, uint32_t freq, uint32_t bandw);
int tune_terr2(int fd, uint32_t freq, uint32_t bandw);
int tune_isdbt(int fd, uint32_t freq, uint32_t bandw);
void stop_dmx( int fd );
int read_section_from_dmx(int fd, uint8_t *buf, int max,
			  uint16_t pid, uint8_t table_id, uint32_t secnum);
int set_dmx_section_filter(int fd, uint16_t pid, uint8_t tid, uint32_t ext,
			   uint32_t ext_mask, uint32_t ext_nmask);
int open_dmx_section_filter(int adapter, int num, uint16_t pid, uint8_t tid,
			    uint32_t ext, uint32_t ext_mask,
			    uint32_t ext_nmask);

fe_status_t dvb_get_stat(int fd);
int64_t dvb_get_strength(int fd);
int64_t dvb_get_cnr(int fd);

int dvb_tune_sat(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb);
int dvb_tune_c(dvb_devices *dev, dvb_fe *fe);
int dvb_tune_terr(dvb_devices *dev, dvb_fe *fe);
int dvb_tune_terr2(dvb_devices *dev, dvb_fe *fe);
int dvb_tune_isdbt(dvb_devices *dev, dvb_fe *fe);
void dvb_init_dev(dvb_devices *dev);
void dvb_init_fe(dvb_fe *fe);
void dvb_init_lnb(dvb_lnb *lnb);
void dvb_init(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb);
void dvb_copy_fe(dvb_fe *outfe, dvb_fe *fe);
void dvb_copy_lnb(dvb_lnb *outlnb, dvb_lnb *lnb);
void dvb_copy_dev(dvb_devices *outdev, dvb_devices *dev);
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
int dvb_set_dmx_section_filter(dvb_devices *dev, uint16_t pid, uint8_t tid,
			       uint32_t ext, uint32_t ext_mask,
			       uint32_t ext_nmask);

int dvb_tune(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb);
    
#endif

