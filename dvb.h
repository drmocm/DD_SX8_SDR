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

int set_fe_input(int fd, uint32_t fr,
		 uint32_t sr, fe_delivery_system_t ds,
		 uint32_t input, uint32_t id);
int diseqc(int fd, int sat, int hor, int band);
int open_dmx(int adapter, int num);
int open_fe(int adapter, int num);
int open_dvr(int adapter, int num);
int tune_sat(int fd, int type, uint32_t freq, 
	     uint32_t sr, fe_delivery_system_t ds, 
	     uint32_t input, uint32_t id, 
	     uint32_t sat, uint32_t pol, uint32_t hi,
	     uint32_t lnb, uint32_t lofs, uint32_t lof1, uint32_t lof2,
	     uint32_t scif_slot, uint32_t scif_freq);
void power_on_delay(int fd, int delay);
int read_status(int fd);
int get_stat(int fd, uint32_t cmd, struct dtv_fe_stats *stats);
#endif

