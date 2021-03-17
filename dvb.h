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
#define AGC_ON  0x20000000
#define AGC_OFF 0x20020092

int set_fe_input(int fd, uint32_t fr,
		 uint32_t sr, fe_delivery_system_t ds,
		 uint32_t input, uint32_t id);

int open_dmx(int adapter, int num);
int open_fe(int adapter, int num);
int open_dvr(int adapter, int num);

#endif

