#include "dvb.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))



/******************************************************************************/


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
		fprintf(stderr, "FE_SET_PROPERTY returned %d\n", ret);
		return -1;
	}
	set_property(fd, DTV_STREAM_ID, id);
	set_property(fd, DTV_INPUT, input);
	set_property(fd, DTV_TUNE, 0);
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

int diseqc(int fd, int sat, int hor, int band)
{
        struct dvb_diseqc_master_cmd cmd = {
                .msg = {0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00},
                .msg_len = 4
        };

        hor &= 1;
        cmd.msg[3] = 0xf0 | ( ((sat << 2) & 0x0c) | (band ? 1 : 0) | (hor ? 2 : 0));
        
        diseqc_send_msg(fd, hor ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
                        &cmd, band ? SEC_TONE_ON : SEC_TONE_OFF,
                        (sat & 1) ? SEC_MINI_B : SEC_MINI_A, 0);
	fprintf(stderr, "MS %02x %02x %02x %02x\n", 
		cmd.msg[0], cmd.msg[1], cmd.msg[2], cmd.msg[3]);
        return 0;
}

static int set_en50494(int fd, uint32_t freq, uint32_t sr, 
                       int sat, int hor, int band, 
                       uint32_t slot, uint32_t ubfreq,
                       fe_delivery_system_t ds, uint32_t id)
{
        struct dvb_diseqc_master_cmd cmd = {
                .msg = {0xe0, 0x11, 0x5a, 0x00, 0x00},
                .msg_len = 5
        };
        uint16_t t;
        uint32_t input = 3 & (sat >> 6);

        t = (freq + ubfreq + 2) / 4 - 350;
        hor &= 1;

        cmd.msg[3] = ((t & 0x0300) >> 8) | 
                (slot << 5) | ((sat & 0x3f) ? 0x10 : 0) | (band ? 4 : 0) | (hor ? 8 : 0);
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

        fprintf(stderr, "EN50494 %02x %02x %02x %02x %02x\n", 
		cmd.msg[0], cmd.msg[1], cmd.msg[2], cmd.msg[3], cmd.msg[4]);
	return set_fe_input(fd, ubfreq * 1000, sr, ds, input, id);

}

static int set_en50607(int fd, uint32_t freq, uint32_t sr, 
                       int sat, int hor, int band, 
                       uint32_t slot, uint32_t ubfreq,
                       fe_delivery_system_t ds, uint32_t id)
{
        struct dvb_diseqc_master_cmd cmd = {
                .msg = {0x70, 0x00, 0x00, 0x00, 0x00},
                .msg_len = 4
        };
        uint32_t t = freq - 100;
        uint32_t input = 3 & (sat >> 6);
        
        //printf("input = %u, sat = %u\n", input, sat&0x3f);
        hor &= 1;
        cmd.msg[1] = slot << 3;
        cmd.msg[1] |= ((t >> 8) & 0x07);
        cmd.msg[2] = (t & 0xff);
        cmd.msg[3] = ((sat & 0x3f) << 2) | (hor ? 2 : 0) | (band ? 1 : 0);

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

        fprintf(stderr, "EN50607 %02x %02x %02x %02x\n", 
                  cmd.msg[0], cmd.msg[1], cmd.msg[2], cmd.msg[3]);
        fprintf(stderr, "EN50607 freq %u sr %u hor %u\n", 
                  freq, sr, hor);
	return set_fe_input(fd, ubfreq * 1000, sr, ds, input, id);
}

void power_on_delay(int fd, int delay)
{
        fprintf(stderr, "pre voltage %d\n", delay);
	if (ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13) == -1)
	    perror("FE_SET_VOLTAGE failed");
	usleep(delay);
}

int tune_sat(int fd, int type, uint32_t freq, 
	     uint32_t sr, fe_delivery_system_t ds, 
	     uint32_t input, uint32_t id, 
	     uint32_t sat, uint32_t pol, uint32_t hi,
	     uint32_t lnb, uint32_t lofs, uint32_t lof1, uint32_t lof2,
	     uint32_t scif_slot, uint32_t scif_freq)
{
	fprintf(stderr, "tune_sat freq=%u\n", freq);
	
	if (freq > 3000000) {
	    if (lofs)
                        hi = (freq > lofs) ? 1 : 0;
                if (hi) 
                        freq -= lof2;
                else
                        freq -= lof1;
        }
        fprintf(stderr, "tune_sat IF=%u\n", freq);

        fprintf(stderr, "scif_type = %u\n", type);
	int re=-1;
        if (type == 1) { 
	       re = set_en50494(fd, freq / 1000, sr, lnb, pol, hi,
				scif_slot, scif_freq, ds, id);
        } else if (type == 2) {
	        re = set_en50607(fd, freq / 1000, sr, sat, pol, hi,
				 scif_slot, scif_freq, ds, id);
        } else {
                if (input != (~0U)) {
                        input = 3 & (input >> 6);
                        printf("input = %u\n", input);
                }
                diseqc(fd, lnb, pol, hi);
                re = set_fe_input(fd, freq, sr, ds, input, id );
        }
	return re;
}


int read_status(int fd)
{
    fe_status_t stat;
    int st=0;
    
    ioctl(fd, FE_READ_STATUS, &stat);
    if (stat==0x1f) return 1;
    else return 0;
}
