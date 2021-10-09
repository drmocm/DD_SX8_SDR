/*
dump_raw is an **example** program written in C to show how to use 
the SDR mode of the DigitalDevices MAX SX8 to get raw IQ data.

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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>

#define SDR_ID 0x20000000
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define DTV_INPUT 71

static int set_property(int fd, uint32_t cmd, uint32_t data)
{
        struct dtv_property p;
        struct dtv_properties c;
        int ret;

        p.cmd = cmd;
        c.num = 1;
        c.props = &p;
        p.u.data = data;
        ret = ioctl(fd, FE_SET_PROPERTY, &c);
        if (ret < 0) {
                fprintf(stderr, "FE_SET_PROPERTY returned %d %s\n", ret,
                        strerror(errno));
                return -1;
        }
        return 0;
}

int setup_sx8(uint32_t freq, uint32_t sr, int adapter, int input, int fe_num) {
    
    char fname[80];
    int fd;
    int ret;
    struct dmx_pes_filter_params pesFilterParams;

    struct dtv_property p[] = {
        { .cmd = DTV_CLEAR },
        { .cmd = DTV_DELIVERY_SYSTEM, .u.data = SYS_DVBS2 },
        { .cmd = DTV_FREQUENCY, .u.data = freq },
        { .cmd = DTV_INVERSION, .u.data = INVERSION_AUTO },
        { .cmd = DTV_SYMBOL_RATE, .u.data = sr},
        { .cmd = DTV_INNER_FEC, .u.data = FEC_AUTO },
    };              
    struct dtv_properties c;

    // open frontend
    sprintf(fname, "/dev/dvb/adapter%d/frontend%d", adapter, fe_num); 
    fd = open(fname, O_RDWR);
    if (fd < 0) {
        fprintf(stderr,"Could not open frontend %s\n", fname);
        exit(1);
    }

   //set frontend
    c.num = ARRAY_SIZE(p);
    c.props = p;
    ret = ioctl(fd, FE_SET_PROPERTY, &c);

    if (ret < 0) {
        fprintf(stderr, "FE_SET_PROPERTY returned %d\n", ret);
        exit(1);
    }
    set_property(fd, DTV_STREAM_ID, SDR_ID);
    set_property(fd, DTV_INPUT, input);
    set_property(fd, DTV_TUNE, 0);

    // open demux
    sprintf(fname, "/dev/dvb/adapter%u/demux%u", adapter, fe_num); 

    fd = open(fname, O_RDWR);
    if (fd < 0) {
        fprintf(stderr,"Could not open demux %s\n", fname);
        exit(1);
    }
   // set demux
    pesFilterParams.input = DMX_IN_FRONTEND; 
    pesFilterParams.output = DMX_OUT_TS_TAP; 
    pesFilterParams.pes_type = DMX_PES_OTHER; 
    pesFilterParams.flags = DMX_IMMEDIATE_START;
    pesFilterParams.pid = 0x2000;
    if (ioctl(fd , DMX_SET_PES_FILTER, &pesFilterParams) < 0){
        fprintf(stderr,"Error setting demux\n");        
        exit(1);
    }

    // open dvr
    sprintf(fname, "/dev/dvb/adapter%d/dvr%d", adapter,fe_num); 
    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"Could not open dvr %s\n", fname);
        exit(1);
    }
    return fd;
}

int my_read(int fd, u_char *buf, int size)
{
    int re = 0;
    int len = size;

    while ( (re = read(fd, buf, size))  < size){
        
    }
    return re;
}

void shelp(void) {
    printf("usage dump_raw <options>\n");
    printf(
	"--freq <khz>        Set frequency (default: 1090 Mhz).\n"
	"--sr   <rate>       Set symbol rate (default 2000000).\n"
	"--adapter <nb>      Set the DVB device adapter (default 0).\n"
	"--input <nb>        Set the physical input (default 0).\n"
	"--fe_num <nb>       Set frontend number, needed if driver uses adapter_alloc=3 (default 0).\n"
	);
}

#define TS_SIZE 188
#define RSIZE 100
#define OBUF ((TS_SIZE-4)*RSIZE)
int main (int argc, char **argv) {
    u_char ibuf[RSIZE*TS_SIZE];
    unsigned char obuf[OBUF];
    int fdin = 0;
    int adapter = 0;
    int input = 0;
    int fe_num = 0;
    uint32_t freq = 1090000;
    uint32_t sr = 2000000;

    for (int j = 1; j < argc; j++) {
	if (!strcmp(argv[j],"--freq")) {
            freq = strtoll(argv[++j],NULL,10);
        } else if (!strcmp(argv[j],"--sr")) {
            sr = strtoll(argv[++j],NULL,10);
        } else if (!strcmp(argv[j],"--adapter")) {
            adapter = atoi(argv[++j]);
        } else if (!strcmp(argv[j],"--input")) {
            input = atoi(argv[++j]);
        } else if (!strcmp(argv[j],"--fe_num")) {
            fe_num = atoi(argv[++j]);
	} else if (!strcmp(argv[j],"--help")) {
            shelp();
            exit(0);
	} else {
            fprintf(stderr,
		    "Unknown or not enough arguments for option '%s'.\n\n",
		    argv[j]);
            shelp();
            exit(1);
        }
    }
	
    fdin = setup_sx8( freq, sr, adapter, input, fe_num);
    int c=0;
    while (1){
        int re = 0;

        re = my_read(fdin, ibuf, OBUF);
        
        for (int i=0; i < re; i+=TS_SIZE){
            for (int j=4; j<TS_SIZE; j++){
                obuf[c] = ibuf[j+i]+0x80;
                c++;
                if ( c == OBUF) {
                    int wre = write(STDOUT_FILENO, obuf, OBUF);
                    c = 0;
                }
            }
        }
    }
}
