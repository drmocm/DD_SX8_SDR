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

#include "dvb.h"


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

void dvb_copy_dev(dvb_devices *outdev, dvb_devices *dev)
{
    memcpy(outdev, dev, sizeof(dvb_devices));
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

void dvb_copy_fe(dvb_fe *outfe, dvb_fe *fe)
{
    memcpy(outfe, fe, sizeof(dvb_fe));
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

void dvb_copy_lnb(dvb_lnb *outlnb, dvb_lnb *lnb)
{
    memcpy(outlnb, lnb, sizeof(dvb_lnb));
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
//	fprintf(stderr, "tune_sat IF=%u scif_type=%d pol=%d band %d lofs %d lof1 %d lof2 %d\n", freq, type,pol,hi,lofs,lof1,lof2);
	
	if (freq > 3000000) {
	    if (lofs)
		hi = (freq > lofs) ? 1 : 0;
	    if (hi) 
		freq -= lof2;
	    else
		freq -= lof1;
        }
//	fprintf(stderr, "tune_sat IF=%u scif_type=%d pol=%d band %d\n", freq, type,pol,hi);

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


int tune_c(int fd, uint32_t freq, uint32_t bandw, uint32_t sr,
	   enum fe_code_rate fec, uint32_t mod)
{
    struct dtv_property p[] = {
	{ .cmd = DTV_CLEAR },
	{ .cmd = DTV_FREQUENCY, .u.data = freq * 1000},
	{ .cmd = DTV_BANDWIDTH_HZ, .u.data = (bandw != DVB_UNDEF) ?
	  bandw : 8000000 },
	{ .cmd = DTV_SYMBOL_RATE, .u.data = sr },
	{ .cmd = DTV_INNER_FEC, .u.data = (fec != DVB_UNDEF) ?
	  fec : FEC_AUTO },
	{ .cmd = DTV_MODULATION,
	  .u.data = (mod != DVB_UNDEF) ? mod : QAM_AUTO },
	{ .cmd = DTV_TUNE },
    };              
    struct dtv_properties c;
        int ret;
//        fprintf(stderr, "tune_c()\n");
        set_property(fd, DTV_DELIVERY_SYSTEM, SYS_DVBC_ANNEX_A);

        c.num = ARRAY_SIZE(p);
        c.props = p;
        ret = ioctl(fd, FE_SET_PROPERTY, &c);
        if (ret < 0) {
                fprintf(stderr, "FE_SET_PROPERTY returned %d\n", ret);
                return -1;
        }
        return 0;
}

int dvb_tune_c(dvb_devices *dev, dvb_fe *fe)
{
    return tune_c(dev->fd_fe, fe->freq, fe->bandw, fe->sr, fe->fec, fe->mod);
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
    uint32_t bandw = DVB_UNDEF;
    uint32_t mod = DVB_UNDEF;
    enum fe_code_rate fec = DVB_UNDEF;
    int delay = 0;
    uint32_t pol = DVB_UNDEF;
    uint32_t hi = 0;
    uint32_t lnb = 0;
    int lnb_type = UNIVERSAL;
    uint32_t lofs = 11700000;
    uint32_t lof1 = 9750000;
    uint32_t lof2 = 10600000;
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
			"a:d:Df:I:i:j:e:L:p:s:ul:U:",
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
    fe->bandw = bandw;
    fe->mod = mod;
    fe->fec = fec;
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


int dvb_tune(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb)
{
    int re = 0;
    int t= 0;
    int lock = 0;
    switch (fe->delsys){
    case SYS_DVBC_ANNEX_A:
	fprintf(stderr,
		"Tuning freq: %d kHz sr: %d delsys: DVB-C  ",
		fe->freq, fe->sr);
	if ((re=dvb_tune_c( dev, fe)) < 0) return 0;
	break;
	
    case SYS_DVBS:
    case SYS_DVBS2:	
	fprintf(stderr,
		"Tuning freq: %d kHz pol: %s sr: %d delsys: %s "
		"lnb_type: %d input: %d  ",
		fe->freq, fe->pol ? "h":"v", fe->sr,
		fe->delsys == SYS_DVBS ? "DVB-S" : "DVB-S2",
		lnb->type, fe->input);
	if ((re=dvb_tune_sat( dev, fe, lnb)) < 0) return 0;
	break;


    case SYS_DVBT:
    case SYS_DVBT2:
    case SYS_DVBC_ANNEX_B:
    case SYS_ISDBC:
    case SYS_ISDBT:
    case SYS_ISDBS:
    case SYS_UNDEFINED:
    default:
	fprintf(stderr,"Delivery System not yet implemented\n");
	return 0;
	break;
    }

    while (!lock && t < MAXTRY ){
	t++;
	fprintf(stderr,".");
	lock = read_status(dev->fd_fe);
	sleep(1);
    }
    if (lock == 2) {
	fprintf(stderr," tuning timed out\n");
    } else {
	fprintf(stderr,"%slock\n",lock ? " ": " no ");
    }
    return lock;
}

