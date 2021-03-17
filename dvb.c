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


