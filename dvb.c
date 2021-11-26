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

#include <stdlib.h>
#include <getopt.h>
#include "dvb.h"

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

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void dvb_init_dev(dvb_devices *dev)
{
    dev->adapter = 0;    
    dev->num = 0;    
    dev->fd_fe = -1;
    dev->fd_dvr = -1;
    dev->fd_dmx = -1;
    dev->fd_dvr = -1;
    dev->fd_mod = -1;
}

void dvb_init_fe(dvb_fe *fe)
{
    fe->id = DVB_UNDEF;
    fe->freq = DVB_UNDEF;
    fe->sr = DVB_UNDEF;
    fe->pol = DVB_UNDEF;
    fe->hi = 0;
    fe->input = 0;
}

void dvb_init_lnb(dvb_lnb *lnb)
{
    lnb->delay = 0;
    lnb->type = UNIVERSAL;
    lnb->num = 0;
    lnb->lofs = 11700000;
    lnb->lof1 =  9750000;
    lnb->lof2 = 10600000;
    lnb->scif_slot = 0;
    lnb->scif_freq = 1210;
}

void dvb_init(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb)
{
    dvb_init_dev(dev);
    dvb_init_fe(fe);
    dvb_init_lnb(lnb);
}

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


static int get_property(int fd, uint32_t cmd, uint32_t *data)
{
    struct dtv_property p;
    struct dtv_properties c;
    int ret;
    
    p.cmd = cmd;
    c.num = 1;
    c.props = &p;
    ret = ioctl(fd, FE_GET_PROPERTY, &c);
    if (ret < 0) {
	fprintf(stderr, "FE_GET_PROPERTY returned %d %s\n", ret
		,strerror(errno));
	return -1;
    }
    *data = p.u.data;
    return 0;
}


int set_fe_input(int fd, uint32_t fr,
		 uint32_t sr, fe_delivery_system_t ds,
		 uint32_t input, uint32_t id)
{
    struct dtv_property p[] = {
	{ .cmd = DTV_CLEAR },
	{ .cmd = DTV_DELIVERY_SYSTEM, .u.data = ds },
	{ .cmd = DTV_FREQUENCY, .u.data = fr },
	{ .cmd = DTV_INVERSION, .u.data = INVERSION_AUTO },
	{ .cmd = DTV_SYMBOL_RATE, .u.data = sr },
	{ .cmd = DTV_INNER_FEC, .u.data = FEC_AUTO },
    };		
    struct dtv_properties c;
    int ret;
    
    c.num = ARRAY_SIZE(p);
    c.props = p;
    ret = ioctl(fd, FE_SET_PROPERTY, &c);
    if (ret < 0) {
	fprintf(stderr, "FE_SET_PROPERTY fe returned %d\n", ret);
	return -1;
    }
    ret = set_property(fd, DTV_STREAM_ID, id);
    if (ret < 0) {
	fprintf(stderr, "FE_SET_PROPERTY id returned %d\n", ret);
	return -1;
    }
    ret = set_property(fd, DTV_INPUT, input);
    if (ret < 0) {
	fprintf(stderr, "FE_SET_PROPERTY input returned %d\n", ret);
	return -1;
    }
    
    ret = set_property(fd, DTV_TUNE, 0);
    if (ret < 0) {
	fprintf(stderr, "FE_SET_PROPERTY tune returned %d\n", ret);
	return -1;
    }
    
    return 0;
}


int open_dmx(int adapter, int num)
{
    char fname[80];
    struct dmx_pes_filter_params pesFilterParams;
    int fd = -1;
	
    sprintf(fname, "/dev/dvb/adapter%u/demux%u", adapter, num); 
    
    fd = open(fname, O_RDWR);
    if (fd < 0) return -1;
    
    pesFilterParams.input = DMX_IN_FRONTEND; 
    pesFilterParams.output = DMX_OUT_TS_TAP; 
    pesFilterParams.pes_type = DMX_PES_OTHER; 
    pesFilterParams.flags = DMX_IMMEDIATE_START;
    pesFilterParams.pid = 0x2000;
    
    if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
	return -1;
    return fd;
}

int open_dmx_section_filter(int adapter, int num, uint16_t pid, uint8_t tid,
			    uint32_t ext, uint32_t ext_mask,
			    uint32_t ext_nmask)    
{
    char fname[80];
    int fd;
    struct dmx_sct_filter_params sctfilter;

    sprintf(fname, "/dev/dvb/adapter%u/demux%u", adapter, num); 
    
    fd = open(fname, O_RDWR);
    if (fd < 0) return -1;

    memset(&sctfilter, 0, sizeof(struct dmx_sct_filter_params));
    
    sctfilter.pid = pid;
    sctfilter.filter.filter[0] = tid;
    sctfilter.filter.mask[0]   = 0xff;
    sctfilter.filter.mode[0]   = 0x00;
    // automatically set the mask for ext if none is given
    if (ext && !ext_mask && !ext_nmask) ext_mask = ext;
    for (int i=1; i < 5; i++) {
	sctfilter.filter.filter[i] = (uint8_t)(ext >> (8*(4-i))) & 0xff;
	sctfilter.filter.mask[i] = (uint8_t)(ext_mask >> (8*(4-i))) & 0xff;
	sctfilter.filter.mode[i] = (uint8_t)(ext_nmask >> (8*(4-i))) & 0xff;
    }
    
    sctfilter.flags = DMX_IMMEDIATE_START |DMX_CHECK_CRC;
    
    if (ioctl(fd, DMX_SET_FILTER, &sctfilter) < 0) {
	perror ("ioctl DMX_SET_FILTER failed");
	return -1;
    }
    return fd;
}

int dvb_open_dmx_section_filter(dvb_devices *dev, uint16_t pid, uint8_t tid,
			    uint32_t ext, uint32_t ext_mask,
			    uint32_t ext_nmask)
{

    return  open_dmx_section_filter(dev->adapter, dev->num, pid, tid, ext,
				    ext_mask, ext_nmask);

}

void stop_dmx( int fd )
{
    ioctl (fd, DMX_STOP, 0);
}

int open_fe(int adapter, int num)
{
	char fname[80];
	int fd = -1;
	
	sprintf(fname, "/dev/dvb/adapter%d/frontend%d", adapter, num); 
	fd = open(fname, O_RDWR);
	if (fd < 0) {
	    fprintf(stderr,"Could not open frontend %s\n", fname);
	    return -1;
	}
	return fd;
}

int open_dvr(int adapter, int num)
{
	char fname[80];
	int fd = -1;
	
	sprintf(fname, "/dev/dvb/adapter%d/dvr%d", adapter, num); 
	fd = open(fname, O_RDONLY);
	if (fd < 0) {
	    fprintf(stderr,"Could not open dvr %s\n", fname);
	    return -1;
	}
	return fd;
}


static void diseqc_send_msg(int fd, fe_sec_voltage_t v, 
                            struct dvb_diseqc_master_cmd *cmd,
                            fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b, 
                            int wait)
{
        if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) == -1)
                perror("FE_SET_TONE failed");
        if (ioctl(fd, FE_SET_VOLTAGE, v) == -1)
                perror("FE_SET_VOLTAGE failed");
        usleep(15 * 1000);
        if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, cmd) == -1)
                perror("FE_DISEQC_SEND_MASTER_CMD failed");
        usleep(wait * 1000);
        usleep(15 * 1000);
        if (ioctl(fd, FE_DISEQC_SEND_BURST, b) == -1)
                perror("FE_DISEQC_SEND_BURST failed");
        usleep(15 * 1000);
        if (ioctl(fd, FE_SET_TONE, t) == -1)
                perror("FE_SET_TONE failed");
}

int diseqc(int fd, int lnb, int hor, int band)
{
        struct dvb_diseqc_master_cmd cmd = {
                .msg = {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00},
                .msg_len = 4
        };

        hor &= 1;
        cmd.msg[3] = 0xf0 | ( ((lnb << 2) & 0x0c) | (band ? 1 : 0) | (hor ? 2 : 0));
        
        diseqc_send_msg(fd, hor ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
                        &cmd, band ? SEC_TONE_ON : SEC_TONE_OFF,
                        (lnb & 1) ? SEC_MINI_B : SEC_MINI_A, 0);
	//fprintf(stderr, "MS %02x %02x %02x %02x\n", 
	//	cmd.msg[0], cmd.msg[1], cmd.msg[2], cmd.msg[3]);
        return 0;
}

static int set_en50494(int fd, uint32_t freq, uint32_t sr, 
                       int lnb, int hor, int band, 
                       uint32_t slot, uint32_t ubfreq,
                       fe_delivery_system_t ds, uint32_t id,
		       uint32_t input)
{
        struct dvb_diseqc_master_cmd cmd = {
                .msg = {0xe0, 0x11, 0x5a, 0x00, 0x00},
                .msg_len = 5
        };
        uint16_t t;

        t = (freq + ubfreq + 2) / 4 - 350;
        hor &= 1;

        cmd.msg[3] = ((t & 0x0300) >> 8) | 
                (slot << 5) | ((lnb & 0x3f) ? 0x10 : 0) | (band ? 4 : 0) | (hor ? 8 : 0);
        cmd.msg[4] = t & 0xff;

        set_property(fd, DTV_INPUT, input);
        if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) == -1)
                perror("FE_SET_TONE failed");
        if (ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18) == -1)
                perror("FE_SET_VOLTAGE failed");
        usleep(15000);
        if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1)
                perror("FE_DISEQC_SEND_MASTER_CMD failed");
        usleep(15000);
        if (ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1)
                perror("FE_SET_VOLTAGE failed");

        //fprintf(stderr, "EN50494 %02x %02x %02x %02x %02x\n", 
	//	cmd.msg[0], cmd.msg[1], cmd.msg[2], cmd.msg[3], cmd.msg[4]);
	return set_fe_input(fd, ubfreq * 1000, sr, ds, input, id);

}

static int set_en50607(int fd, uint32_t freq, uint32_t sr, 
                       int lnb, int hor, int band, 
                       uint32_t slot, uint32_t ubfreq,
                       fe_delivery_system_t ds, uint32_t id,
		       uint32_t input)
{
        struct dvb_diseqc_master_cmd cmd = {
                .msg = {0x70, 0x00, 0x00, 0x00, 0x00},
                .msg_len = 4
        };
        uint32_t t = freq - 100;
        
        hor &= 1;
        cmd.msg[1] = slot << 3;
        cmd.msg[1] |= ((t >> 8) & 0x07);
        cmd.msg[2] = (t & 0xff);
        cmd.msg[3] = ((lnb & 0x3f) << 2) | (hor ? 2 : 0) | (band ? 1 : 0);

        set_property(fd, DTV_INPUT, input);
        if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) == -1)
                perror("FE_SET_TONE failed");
        if (ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18) == -1)
                perror("FE_SET_VOLTAGE failed");
        usleep(15000);
        if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) == -1)
                perror("FE_DISEQC_SEND_MASTER_CMD failed");
        usleep(15000);
        if (ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1)
                perror("FE_SET_VOLTAGE failed");
/*
        fprintf(stderr, "EN50607 %02x %02x %02x %02x\n", 
                  cmd.msg[0], cmd.msg[1], cmd.msg[2], cmd.msg[3]);
        fprintf(stderr, "EN50607 freq %u sr %u hor %u band: %d "
		"ds: %d ufreq %d\n", 
		freq, sr, hor, band,ds , ubfreq*1000);
*/
	return set_fe_input(fd, ubfreq * 1000, sr, ds, input, id);
}

void power_on_delay(int fd, int delay)
{
	if (ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1)
	    perror("FE_SET_VOLTAGE failed");
	usleep(delay);
}

int tune_sat(int fd, int type, uint32_t freq, 
	     uint32_t sr, fe_delivery_system_t ds, 
	     uint32_t input, uint32_t id, uint32_t pol, uint32_t hi,
	     uint32_t lnb, uint32_t lofs, uint32_t lof1, uint32_t lof2,
	     uint32_t scif_slot, uint32_t scif_freq)
{
        set_property(fd, DTV_INPUT, input);
	
	if (freq > 3000000) {
	    if (lofs)
		hi = (freq > lofs) ? 1 : 0;
	    if (hi) 
		freq -= lof2;
	    else
		freq -= lof1;
        }
//	fprintf(stderr, "tune_sat IF=%u scif_type=%d\n", freq, type);

	int re=-1;
        if (type == 1) { 
	       re = set_en50494(fd, freq / 1000, sr, lnb, pol, hi,
				scif_slot, scif_freq, ds, id, input);
        } else if (type == 2) {
	        re = set_en50607(fd, freq / 1000, sr, lnb, pol, hi,
				 scif_slot, scif_freq, ds, id, input);
        } else {
	        diseqc(fd, lnb, pol, hi);
                re = set_fe_input(fd, freq, sr, ds, input, id );
        }
	return re;
}

int dvb_tune_sat(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb)
{
    return tune_sat(dev->fd_fe, lnb->type, fe->freq, fe->sr, fe->delsys,
		    fe->input, fe->id, fe->pol, fe->hi, lnb->num, lnb->lofs,
		    lnb->lof1, lnb->lof2, lnb->scif_slot, lnb->scif_freq);
}

void dvb_print_tuning_options()
{
    fprintf(stderr,
	    "\n TUNING OPTIONS:\n"
	    " -d delsys    : the delivery system\n"
	    " -a adapter   : the number n of the DVB adapter, i.e. \n"
	    "                /dev/dvb/adapter[n] (default=0)\n"
	    " -e frontend  : the frontend/dmx/dvr to be used (default=0)\n"
	    " -f frequency : center frequency in kHz\n"
	    " -i input     : the physical input of the SX8 (default=0)\n"
	    " -I id        : set id (do not use if you don't know)\n"
	    " -l ls l1 l2  : set lofs lof1 lof2 \n"
	    "              : (default 11700000 9750000 10600000)\n"
 	    " -L n         : diseqc switch to LNB/SAT number n (default 0)\n"
	    " -p pol       : polarisation 0=vertical 1=horizontal\n"
	    "              : (must be set for any diseqc command to be send)\n"
	    " -s rate      : the symbol rate in Symbols/s\n"
	    " -u           : use hi band of LNB\n"
	    " -D           : use 1s delay to wait for LNB power up\n"
	    " -U type      : lnb is unicable type (1: EN 50494, 2: TS 50607\n"
	    " -j slot freq : slot s freqency f ( default slot 1 freq 1210 in MHz)\n"
	);
}

int dvb_parse_args(int argc, char **argv,
		   dvb_devices *dev, dvb_fe *fe, dvb_lnb *ln)
{
    enum fe_delivery_system delsys = SYS_DVBS2;
    int adapter = 0;
    int input = 0;
    int fe_num = 0;
    uint32_t freq = 0;
    uint32_t sr = DVB_UNDEF;
    uint32_t id = DVB_UNDEF;
    int delay = 0;
    uint32_t pol = DVB_UNDEF;
    uint32_t hi = 0;
    uint32_t lnb = 0;
    int lnb_type = UNIVERSAL;
    uint32_t lofs = 0;
    uint32_t lof1 = 0;
    uint32_t lof2 = 0;
    uint32_t scif_slot = 0;
    uint32_t scif_freq = 1210;
    char *nexts= NULL;
    opterr = 0;
    optind = 1;
    char **myargv;

    myargv = malloc(argc*sizeof(char*));
    memcpy(myargv, argv, argc*sizeof(char*));
    
    while (1) {
	int option_index = 0;
	int c;
	static struct option long_options[] = {
	    {"adapter", required_argument, 0, 'a'},
	    {"delay", no_argument, 0, 'D'},
	    {"delsys", required_argument, 0, 'd'},
	    {"frequency", required_argument, 0, 'f'},
	    {"lofs", required_argument, 0, 'l'},
	    {"unicable", required_argument, 0, 'U'},
	    {"slot", required_argument, 0, 'j'},
	    {"input", required_argument, 0, 'i'},
	    {"id", required_argument, 0, 'I'},
	    {"frontend", required_argument, 0, 'e'},
	    {"lnb", required_argument, 0, 'L'},
	    {"polarisation", required_argument, 0, 'p'},
	    {"symbol_rate", required_argument, 0, 's'},
	    {"band", no_argument, 0, 'u'},
	    {0, 0, 0, 0}
	};

	c = getopt_long(argc, myargv, 
			"a:d:Df:I:i:j:e:L:p:s:ul:U:S:",
			long_options, &option_index);
	if (c==-1)
	    break;
	switch (c) {
	case 'a':
	    adapter = strtoul(optarg, NULL, 0);
	    break;

	case 'd':
	    if (!strcmp(optarg, "C"))
		delsys = SYS_DVBC_ANNEX_A;
	    if (!strcmp(optarg, "DVBC"))
		delsys = SYS_DVBC_ANNEX_A;
	    if (!strcmp(optarg, "S"))
		delsys = SYS_DVBS;
	    if (!strcmp(optarg, "DVBS"))
		delsys = SYS_DVBS;
	    if (!strcmp(optarg, "S2"))
		delsys = SYS_DVBS2;
	    if (!strcmp(optarg, "DVBS2"))
		delsys = SYS_DVBS2;
	    if (!strcmp(optarg, "T"))
		delsys = SYS_DVBT;
	    if (!strcmp(optarg, "DVBT"))
		delsys = SYS_DVBT;
	    if (!strcmp(optarg, "T2"))
		delsys = SYS_DVBT2;
	    if (!strcmp(optarg, "DVBT2"))
		delsys = SYS_DVBT2;
	    if (!strcmp(optarg, "J83B"))
		delsys = SYS_DVBC_ANNEX_B;
	    if (!strcmp(optarg, "ISDBC"))
		delsys = SYS_ISDBC;
	    if (!strcmp(optarg, "ISDBT"))
		delsys = SYS_ISDBT;
	    if (!strcmp(optarg, "ISDBS"))
		delsys = SYS_ISDBS;
	    break;

	case 'D':
	    delay = 1000000;
	    break;
	    
	case 'e':
	    fe_num = strtoul(optarg, NULL, 0);
	    break;
	    
	case 'f':
	    freq = strtoul(optarg, NULL, 0);
	    break;

	case 'l':
	    lofs = strtoul(optarg, &nexts, 0);
	    if (nexts){
		nexts++;
		lof1 = strtoul(nexts, &nexts, 0);
	    } if (nexts) {
		nexts++;
		lof2 = strtoul(nexts, NULL, 0);
	    } else {
		fprintf(stderr, "Error Missing data in -l (--lofs)");
		return -1;
	    } 
	    break;
	    
	case 'U':
	    lnb_type = strtoul(optarg, NULL, 0);
	    break;

	case 'j':
	    nexts = NULL;
	    scif_slot = strtoul(optarg, &nexts, 0);
	    scif_slot = scif_slot-1;
	    nexts++;
	    scif_freq = strtoul(nexts, NULL, 0);
	    scif_freq = scif_freq;
	    break;
	    
	case 'i':
	    input = strtoul(optarg, NULL, 0);
	    break;

	case 'I':
	    id = strtoul(optarg, NULL, 0);
	    break;

	case 'L':
	    lnb = strtoul(optarg, NULL,0);	    
	    break;
	    
	case'p':
	    if (!strcmp(optarg, "h") || !strcmp(optarg, "H"))
	    {
		pol = 1;
	    } else if (!strcmp(optarg, "v") || !strcmp(optarg, "V"))
	    {
		pol = 0;
	    } else {
		pol =  strtoul(optarg, NULL, 0);
	    }
	    break;
	    
	case 's':
	    sr = strtoul(optarg, NULL, 0);
	    break;

	case 'u':
	    if (pol == 2) pol = 0;
	    hi  = 1;
	    break;

	default:
	    break;
	    
	}
    }

    fe->delsys = delsys;
    fe->input = input;
    fe->freq = freq;
    fe->sr = sr;
    fe->pol = pol;
    fe->hi = hi;
    fe->id = id;
    ln->num = lnb;
    ln->delay = delay;
    ln->type = lnb_type;
    ln->lofs = lofs;
    ln->lof1 = lof1;
    ln->lof2 = lof2;
    ln->scif_slot = scif_slot;
    ln->scif_freq = scif_freq;
    dev->adapter = adapter;
    dev->num = fe_num;
    
    return 0;
}


int read_status(int fd)
{
    fe_status_t stat;
    int st=0;
    
    ioctl(fd, FE_READ_STATUS, &stat);
    switch((int)stat){
    case 0x1f:
	return 1;
    case FE_TIMEDOUT:
	return 2;
    default:
	return 0;
    }
}

int get_stat(int fd, uint32_t cmd, struct dtv_fe_stats *stats)
{
  struct dtv_property p;
  struct dtv_properties c;
  int ret;

  p.cmd = cmd;
  c.num = 1;
  c.props = &p;
  ret = ioctl(fd, FE_GET_PROPERTY, &c);
  if (ret < 0) {
    fprintf(stderr, "FE_GET_PROPERTY returned %d\n", ret);
    return -1;
  }
  memcpy(stats, &p.u.st, sizeof(struct dtv_fe_stats));
  return 0;
}

void dvb_open(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb)
{
    
    if ( (dev->fd_fe = open_fe(dev->adapter, dev->num)) < 0){
	exit(1);
    }

    if (fe->pol != DVB_UNDEF) power_on_delay(dev->fd_fe, lnb->delay);

    if ( (dev->fd_dmx = open_dmx(dev->adapter, dev->num)) < 0){
	exit(1);
    }
    if ( (dev->fd_dvr=open_dvr(dev->adapter, dev->num)) < 0){
	exit(1);
    }
}


uint32_t getbcd(uint8_t *p, int l)
{
        int i;
        uint32_t val = 0, t;

        for (i = 0; i < l / 2; i++) {
                t = (p[i] >> 4) * 10 + (p[i] & 0x0f);
                val = val * 100 + t;
        }
        if (l & 1)
                val = val * 10 + (p[i] >> 4);
        return val;
}


static void en300468_parse_string_to_utf8(char *dest, uint8_t *src,
				   const unsigned int len)
{
    int utf8 = (src[0] == 0x15) ? 1 : 0;
    int skip = (src[0] < 0x20) ? 1 : 0;
    if( src[0] == 0x10 ) skip += 2;
    uint16_t utf8_cc;
    int dest_pos = 0;
    int emphasis = 0;
    int i;
    
    for (i = skip; i < len; i++) {
	switch(*(src + i)) {
	case SB_CC_RESERVED_80 ... SB_CC_RESERVED_85:
	case SB_CC_RESERVED_88 ... SB_CC_RESERVED_89:
	case SB_CC_USER_8B ... SB_CC_USER_9F:
	case CHARACTER_CR_LF:
	    dest[dest_pos++] = '\n';
	    continue;
	case CHARACTER_EMPHASIS_ON:
	    emphasis = 1;
	    continue;
	case CHARACTER_EMPHASIS_OFF:
	    emphasis = 0;
	    continue;
	case UTF8_CC_START:
	    if (utf8 == 1) {
		utf8_cc = *(src + i) << 8;
		utf8_cc += *(src + i + 1);
		
		switch(utf8_cc) {
		case ((UTF8_CC_START << 8) | CHARACTER_EMPHASIS_ON):
		    emphasis = 1;
		    i++;
		    continue;
		case ((UTF8_CC_START << 8) | CHARACTER_EMPHASIS_OFF):
		    emphasis = 0;
		    i++;
		    continue;
		default:
		    break;
		}
	    }
	default: {
	    if (*(src + i) < 128)
		dest[dest_pos++] = *(src + i);
	    else {
		dest[dest_pos++] = 0xc2 + (*(src + i) > 0xbf);
		dest[dest_pos++] = (*(src + i) & 0x3f) | 0x80;
	    }
	    break;
	}
	}
    }
    dest[dest_pos] = '\0';
}

void dvb2txt(char *in)
{
    uint8_t len;
    char *out, *buf;
	
    len = strlen(in);
//    my_err(0,"%s len = %d",in, len);
    if (!len) return;
    buf = (char *) malloc(sizeof(char)*(len+1));

    if (!buf) {
	fprintf(stderr,"Error allocating memory\n");
	    exit(1);
    }
    memset(buf,0,len+1);
    out = buf;
    en300468_parse_string_to_utf8(out, (uint8_t *)in, len);
    
    int outlen = strlen(buf);
    memcpy(in,buf,sizeof(char)*outlen);
    in[outlen] = 0;
    free(buf);
}


static void dvb_delete_descriptor(descriptor *desc)
{
    free(desc);
}

descriptor *dvb_get_descriptor(uint8_t *buf)
{
    descriptor *desc = NULL;
    if (!(desc = malloc(sizeof(descriptor)))) {
    	fprintf(stderr,"Error allocating memory in dvb_get_section\n");
	return NULL;	
    }
    desc->tag = buf[0];
    desc->len = buf[1];
    desc->data= buf+2;
//    dvb_print_descriptor(fileno(stderr), desc);
    return desc;
}

static void dvb_delete_descriptor_loop(descriptor **desc, int length)
{
    for(int n=0; n<length; n++)
	free(desc[n]);
}

static int dvb_get_descriptor_loop(uint8_t *buf, descriptor **descriptors,
				 int length)
{
    int nc = 0;
    int dc = 0;
    while ( nc < length){
	descriptor *desc = dvb_get_descriptor(buf+nc);
	descriptors[dc] = desc;
	nc += desc->len+2;
	dc++;
	if (dc >= MAXDESC){
	    fprintf(stderr,"WARNING: maximal descriptor coun reached\n");
	}
    }
    return dc;
}

static void dvb_delete_section(section *sec)
{
    free(sec);
}

section *dvb_get_section(uint8_t *buf) 
{
    section *sec = NULL;
    if (!(sec = malloc(sizeof(section)))){
	fprintf(stderr,"Error allocating memory in dvb_get_section\n");
	return NULL;	
    }
    memset(sec,0,sizeof(section));
    
    sec->section_length = (((buf[1]&0x0F) << 8) | buf[2]);
    memcpy(sec->data, buf, sec->section_length+3); 

    sec->table_id = buf[0];
    sec->section_syntax_indicator = buf[1] & 0x01;
    if (sec->section_syntax_indicator){
	sec->id = (buf[3] << 8) | buf[4];
	sec->current_next_indicator = buf[5]&0x01;
	sec->version_number = (buf[5]&0x3e) >> 1;
	sec->section_number = buf[6];
	sec->last_section_number = buf[7];
    }
    return sec;
}

static void dvb_delete_sdt_service(sdt_service *serv)
{
    dvb_delete_descriptor_loop(serv->descriptors,serv->desc_num);
    free(serv);
}

sdt_service *dvb_get_sdt_service(uint8_t *buf)
{
    sdt_service *serv = NULL;
    if (!(serv = malloc(sizeof(sdt_service)))) {
    	fprintf(stderr,"Error allocating memory in dvb_get_sdt_service\n");
	return NULL;	
    }
    serv->service_id = (buf[0] << 8) | buf[1];
    serv->EIT_schedule_flag = buf[2]&0x02 ;
    serv->EIT_present_following_flag = buf[2]&0x01 ;
    serv->running_status = (buf[3]&0xe0)>>5;
    serv->free_CA_mode = (buf[3]&0x10)>>8;
    serv->descriptors_loop_length =  ((buf[3]&0x0f) << 8) | buf[4];
    serv->desc_num = 0;
    buf += 5;

    serv->desc_num = dvb_get_descriptor_loop(buf, serv->descriptors,
					     serv->descriptors_loop_length);
    return serv;
}

void dvb_delete_sdt(SDT *sdt)
{
    for (int n=0; n < sdt->service_num; n++)
	dvb_delete_sdt_service(sdt->services[n]);
    dvb_delete_section(sdt->sdt);
    free(sdt);
}

SDT *dvb_get_sdt(uint8_t *buf)
{
    SDT *sdt = NULL;
    section *sec = dvb_get_section(buf);

    buf = sec->data;
    if (!(sdt = malloc(sizeof(NIT)))){
	fprintf(stderr,"Error allocating memory in dvb_get_sdt\n");
	return NULL;
    }
    sdt->sdt = sec;
    sdt->original_network_id = (buf[8] << 8) | buf[9];
    buf += 11;

    int dc = 0;
    int c = 0;
    while ( c < sdt->sdt->section_length-12 ){ // section_length +3-4-11
	sdt_service *serv = dvb_get_sdt_service(buf+c);
	sdt->services[dc] = serv;
	c += serv->descriptors_loop_length+5; 
	dc++;
    }
    sdt->service_num = dc;
    
    return sdt;
}

static void dvb_delete_nit_transport(nit_transport *trans)
{
    dvb_delete_descriptor_loop(trans->descriptors, trans->desc_num);
    free(trans);
}

nit_transport *dvb_get_nit_transport(uint8_t *buf)
{
    nit_transport *trans = NULL;
    if (!(trans = malloc(sizeof(nit_transport)))) {
    	fprintf(stderr,"Error allocating memory in dvb_get_nit_transport\n");
	return NULL;	
    }
    trans->transport_stream_id = (buf[0] << 8) | buf[1];
    trans->original_network_id = (buf[2] << 8) | buf[3];
    trans->transport_descriptors_length =  ((buf[4]&0x0f) << 8) | buf[5];

    trans->desc_num = 0;
    buf += 6;
    trans->desc_num = dvb_get_descriptor_loop(buf, trans->descriptors,
					      trans->transport_descriptors_length);

    return trans;
}

void dvb_delete_nit(NIT *nit)
{
    dvb_delete_descriptor_loop(nit->network_descriptors, nit->ndesc_num);
    for (int n=0; n < nit->trans_num; n++)
	dvb_delete_nit_transport(nit->transports[n]);
    dvb_delete_section(nit->nit);
    free(nit);
}


NIT  *dvb_get_nit(uint8_t *buf)
{
    NIT *nit = NULL;
    section *sec = dvb_get_section(buf);

    buf = sec->data;
    if (!(nit = malloc(sizeof(NIT)))){
	fprintf(stderr,"Error allocating memory in dvb_get_nit\n");
	return NULL;
    }
    nit->nit = sec;

    if (sec->table_id != 0x40 && sec->table_id != 0x41){
	free(sec);
	fprintf(stderr,"Error in dvb_get_nit, not a NIT section\n");
	return NULL;
    }
    nit->network_descriptor_length = (((buf[8]&0x0F) << 8) | buf[9]);

    buf += 10;
    nit->ndesc_num = dvb_get_descriptor_loop(buf, nit->network_descriptors,
					     nit->network_descriptor_length);
    buf += nit->network_descriptor_length;

    nit->transport_stream_loop_length = (((buf[0]&0x0F) << 8) | buf[1]);
    buf += 2;

    int nc = 0;
    int dc = 0;
    while ( nc < nit->transport_stream_loop_length){
	nit_transport *trans = dvb_get_nit_transport(buf+nc);
	nit->transports[dc] = trans;
	nc += trans->transport_descriptors_length+6; 
	dc++;
    }
    nit->trans_num = dc;
    
    return nit;

}

void dvb_print_transport(FILE *fp, nit_transport *trans)
{
    fprintf(fp,"  transport:\n"
	    "    transport_stream_id 0x%04x original_network_id 0x%04x\n",
	    trans->transport_stream_id, trans->original_network_id);
    if (trans->desc_num){
	fprintf(fp,"    descriptors:\n");
	for (int n=0 ; n < trans->desc_num; n++){
	    dvb_print_descriptor(fp, trans->descriptors[n], "      ");
	}
    }
}

void dvb_print_nit(int fd, NIT *nit)
{
    FILE* fp = fdopen(fd, "w");
    fprintf(fp,"NIT (0x%02x): (%d/%d) \n  network_id 0x%04x\n",
	    nit->nit->table_id, nit->nit->section_number+1,
	    nit->nit->last_section_number+1, nit->nit->id);
    if (nit->ndesc_num){
	fprintf(fp,"  network descriptors:\n");

	for (int n=0 ; n < nit->ndesc_num; n++){
	    dvb_print_descriptor(fp, nit->network_descriptors[n],"    ");
	}
    }
    if (nit->trans_num){
	fprintf(fp,"  transports:\n");
	for (int n=0 ; n < nit->trans_num; n++){
	    dvb_print_transport(fp, nit->transports[n]);
	}
    }
    fprintf(fp,"\n");
}

void dvb_print_service(FILE *fp, sdt_service *serv)
{
    const char *R[] = {	"undefined","not running",
	"starts in a few seconds","pausing","running","service off-air",
	"unknown","unknown"};
    
    fprintf(fp,"  service:\n"
	    "    service_id 0x%04x EIT_schedule_flag 0x%02x\n"
	    "    EIT_present_following_flag 0x%02x\n"
	    "    running_status %s free_CA_mode 0x%02x\n",
	    serv->service_id, serv->EIT_schedule_flag,
	    serv->EIT_present_following_flag, R[serv->running_status],
	    serv->free_CA_mode);
    if (serv->desc_num){
	fprintf(fp,"    descriptors:\n");
	for (int n=0 ; n < serv->desc_num; n++){
	    dvb_print_descriptor(fp, serv->descriptors[n], "      ");
	}
    }
}

void dvb_print_sdt(int fd, SDT *sdt)
{
    FILE* fp = fdopen(fd, "w");
    fprintf(fp,"SDT (0x%02x): (%d/%d)\n  original_network_id 0x%04x ",
	    sdt->sdt->table_id, sdt->sdt->id, sdt->sdt->section_number,
	    sdt->sdt->last_section_number);
    if (sdt->service_num){
	fprintf(fp,"  services:\n");
	for (int n=0 ; n < sdt->service_num; n++){
	    dvb_print_service(fp, sdt->services[n]);
	}
    }
}

void dvb_print_delsys_descriptor(FILE *fp, descriptor *desc, char *s)
{
    uint8_t *buf = desc->data;
    uint32_t freq;
    uint16_t orbit;
    uint32_t srate;
    uint8_t pol;
    uint8_t delsys;
    uint8_t mod;
    uint8_t fec;
    uint8_t east;
    uint8_t roll;

    const char *P[] = {"linear-horizontal", "linear-vertical",
	"circular-left", "circulra-right"};
    const char *M[] = {"Auto", "QPSK", "8PSK", "16QAM"};
    const double roff[] ={0.25, 0.35, 0.20, 0};
    const char *FO[] ={"not defined","no outer FEC coding",
	"RS(204/188)","reserved"};
    const char *F[] ={"not defined", "1/2" ,"2/3", "3/4","5/6","7/8","8/9",
	"3/5","4/5","9/10","reserved","no conv. coding"};
	

    switch(desc->tag){
    case 0x43: // satellite
	fprintf(fp,"%s  Satellite delivery system descriptor: \n",s);
	freq = getbcd(buf, 8) *10;
	orbit = getbcd(buf+4, 4) *10;
	srate = getbcd(buf + 7, 7) / 10;
	east = ((buf[6] & 0x80) >> 7);
	pol =  ((buf[6] & 0x60) >> 5); 
	roll = ((buf[6] & 0x18) >> 3);
	delsys = ((buf[6] & 0x04) >> 2) ? SYS_DVBS2 : SYS_DVBS;
	mod = buf[6] & 0x03;
	fec = buf[10] & 0x0f;
	fprintf(fp,
		"%s  frequency %d orbital_position %d west_east_flag %s\n"
		"%s  polarization %s  modulation_system %s",s,
		freq, orbit, east ? "E":"W", s, P[pol],
		delsys ? "DVB-S2":"DVB-S");
	if (delsys) fprintf(fp," roll_off %.2f\n", roff[roll]);
	fprintf(fp,
		"%s  modulation_type %s symbol_rate %d FEC_inner %s\n",s, 
		M[mod], srate, F[fec]);
	break;

    case 0x44: // cable
	fprintf(fp,"%s  Cable delivery system descriptor\n",s);

	freq =  getbcd(buf, 8) / 10000;
	delsys = buf[5] & 0x0f;
	mod = buf[6];
	srate = getbcd(buf + 7, 7) / 10;
	fec = buf[10] & 0x0f;
	fprintf(fp,
		"%s  frequency %d FEC_outer %s modulation %s\n"
		"%s  symbol_rate %d FEC_inner %s\n",
		s, freq, FO[delsys], srate, F[fec]);
	break;

    case 0x5a: // terrestrial
	freq = (buf[5]|(buf[4] << 8)|(buf[3] << 16)|(buf[0] << 24))*10;
	delsys = SYS_DVBT;
	break;

    case 0xfa: // isdbt
	freq = (buf[5]|(buf[4] << 8))*7000000;
	delsys = SYS_ISDBT;
    }
}

static char *dvb_get_name(uint8_t *buf, int len)
{
    char *name=NULL;
    if (len){
	name = malloc(sizeof(char)*len+1);
	memset(name,0,len+1);
	memcpy(name,buf,len);
	dvb2txt(name);
    }
    return name;
}

void dvb_print_data(FILE *fp, uint8_t *b, int length, int step,
		    char *s, char *s2)
{
    int i, j;
    if(!step) step = 16;
    if (!length) return;

    for (j = 0; j < length; j += step, b += step) {
	fprintf(fp,"%s%s  ",s,s2);
	for (i = 0; i < step; i++)
	    if (i + j < length)
		fprintf(fp,"0x%02x ", b[i]);
	    else
		fprintf(fp,"     ");
	fprintf(fp," | ");
	for (i = 0; i < step; i++)
	    if (i + j < length)
		putc((b[i] > 31 && b[i] < 127) ? b[i] : '.',fp);
	fprintf(fp,"\n");
    }
    
}

void dvb_print_linkage_descriptor(FILE *fp, descriptor *desc, char *s)
{
    uint16_t nid = 0;
    uint16_t onid = 0;
    uint16_t sid = 0;
    uint16_t tsid = 0;
    uint8_t link =0;
    uint8_t *buf = desc->data;
    int c = 0;
    fprintf(fp,"%s  Linkage descriptor:\n",s);
    
    tsid = (buf[0] << 8) | buf[1];
    onid = (buf[2] << 8) | buf[3];
    sid = (buf[4] << 8) | buf[5];
    link = buf[6];
    int length = desc->len;
    uint8_t hand = (buf[c]&0xf0)>>4;
    uint8_t org = buf[c]&0x01;
    const char *H[] = {
	"reserved",
	"DVB hand-over to an identical service in a neighbouring country",
	"DVB hand-over to a local variation of the same service",
	"DVB hand-over to an associated service",
	"reserved"
    };
	
    const char *L[] = {
	"reserved","information service","EPG service",
	"CA replacement service",
	"TS containing complete Network/Bouquet SI",
	"service replacement service",
	"data broadcast service","RCS Map","mobile hand-over",
	"System Software Update Service (TS 102 006 [11])",
	"TS containing SSU BAT or NIT (TS 102 006 [11])",
	"IP/MAC Notification Service (EN 301 192 [4])",
	"TS containing INT BAT or NIT (EN 301 192 [4])"};
    
    const char *lk = NULL;
    if (link < 0x0D) lk = L[link];
    else if(0x80 < link && link < 0xff) lk="user defined"; 
    else lk="reserved";
    
    fprintf(fp,
	    "%s    transport_stream_id 0x%04x original_network_id 0x%04x\n"
	    "%s    service_id 0x%04x linkage_type \"%s\"\n",s,tsid,onid,s,
	    sid, lk);
		

    c = 7;
    if (link == 0x08){
	fprintf(fp,
		"%s    handover_type %s origin_type %s\n",s,
		H[hand], org ? "SDT":"NIT");
	if (hand ==0x01 || hand ==0x02 || hand ==0x03){
	    nid = (buf[c+1] << 8) | buf[c+2];
	    fprintf(fp,
		    "%s    network_id 0x%04x\n",s,nid);
	    c++;
	}
	if (!org){
	    sid = (buf[c+1] << 8) | buf[c+2];
	    fprintf(fp,
		    "%s    initial_service_id 0x%04x\n",s,sid);
	    c++;
	}
	buf += c;
    }
    length -= c;
    if (length){
	fprintf(fp,"%s    private_data_bytes: %d %s\n",s, length,
		length>1 ? "bytes":"byte");
	dvb_print_data(fp, buf, length, 8,  s, "    ");
    }
}

void dvb_print_descriptor(FILE *fp, descriptor *desc, char *s)
{
    uint8_t *buf = desc->data;
    int c = 0;
    char *name=NULL;

    fprintf(fp,"%sDescriptor tag: 0x%02x \n",s,desc->tag);
    switch(desc->tag){
    case 0x40:// network_name_descriptor
	fprintf(fp,"%s  Network name descriptor: \n",s);
	fprintf(fp,"%s  name %s \n",s,(char *)&buf[0]);
	break;
    
    case 0x43: // satellite
	dvb_print_delsys_descriptor(fp, desc, s);
	break;
    
    case 0x44: // cable
	dvb_print_delsys_descriptor(fp, desc, s);
	break;

    case 0x48: //service descriptor
	fprintf(fp,"%s  Service descriptor:\n",s);
	fprintf(fp,"%s    service type 0x%02x ",s, desc->tag);
	c++;
	int l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    fprintf(fp,"provider %s ", name);
	    free(name);
	}
	c += l;
	l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    fprintf(fp,"name %s ", name);
	    free(name);
	}
	fprintf(fp,"\n");
	break;

    case 0x4a:
	dvb_print_linkage_descriptor(fp, desc, s);
	break;
	
    case 0x5a: // terrestrial
	dvb_print_delsys_descriptor(fp, desc, s);
	break;

    case 0x5f:
	fprintf(fp,"%s  Private data specifier descriptor: \n",s);
	fprintf(fp,"%s    private_data_specifier: 4 bytes\n",s);
	dvb_print_data(fp, buf, 4, 8,  s, "    ");
	break;
    
    case 0xfa: // isdbt
	dvb_print_delsys_descriptor(fp, desc, s);
	break;

    default:
	break;
	
    }
}

