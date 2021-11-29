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
	uint32_t priv_id = 0;
	fprintf(fp,"    descriptors:\n");
	for (int n=0 ; n < trans->desc_num; n++){
	    priv_id = dvb_print_descriptor(fp, trans->descriptors[n],
					   "      ", priv_id);
	}
    }
}

void dvb_print_nit(int fd, NIT *nit)
{
    FILE* fp = fdopen(fd, "w");
    fprintf(fp,"NIT (0x%02x): %s network (%d/%d) \n  network_id 0x%04x\n",
	    nit->nit->table_id, 
	    (nit->nit->table_id == 0x41) ? "other":"actual", 
	    nit->nit->section_number+1,
	    nit->nit->last_section_number+1, nit->nit->id);
    if (nit->ndesc_num){
	uint32_t priv_id = 0;
	fprintf(fp,"  network descriptors:\n");
	for (int n=0 ; n < nit->ndesc_num; n++){
	    priv_id = dvb_print_descriptor(fp, nit->network_descriptors[n],
					   "    ", priv_id);
	}
    }
    if (nit->trans_num){
	fprintf(fp,"  transports:\n");
	for (int n=0 ; n < nit->trans_num; n++){
	    dvb_print_transport(fp, nit->transports[n]);
	}
    }
    fprintf(fp,"\n");
    fflush(fp);
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
	uint32_t priv_id = 0;
	fprintf(fp,"    descriptors:\n");
	for (int n=0 ; n < serv->desc_num; n++){
	    priv_id = dvb_print_descriptor(fp, serv->descriptors[n], "      ",
					   priv_id);
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
    fflush(fp);
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

    const char *POL[] = {"linear-horizontal", "linear-vertical",
	"circular-left", "circulra-right"};
    const char *MOD[] = {"Auto", "QPSK", "8PSK", "16QAM"};
    const char *MODC[] ={"not defined","16-QAM","32-QAM","64-QAM",
	"128-QAM","256-QAM","reserved"};
    const double roff[] ={0.25, 0.35, 0.20, 0};
    const char *FECO[] ={"not defined","no outer FEC coding",
	"RS(204/188)","reserved"};
    const char *FEC[] ={"not defined", "1/2" ,"2/3", "3/4","5/6","7/8","8/9",
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
	if (fec > 10 && fec < 15) fec = 10;
	if (fec == 15) fec = 11;
	fprintf(fp,
		"%s  frequency %d orbital_position %d west_east_flag %s\n"
		"%s  polarization %s  modulation_system %s",s,
		freq, orbit, east ? "E":"W", s, POL[pol],
		delsys ? "DVB-S2":"DVB-S");
	if (delsys) fprintf(fp," roll_off %.2f\n", roff[roll]);
	fprintf(fp,
		"%s  modulation_type %s symbol_rate %d FEC_inner %s\n",s, 
		MOD[mod], srate, FEC[fec]);
	break;

    case 0x44: // cable
	fprintf(fp,"%s  Cable delivery system descriptor\n",s);

	freq =  getbcd(buf, 8);
	delsys = buf[5] & 0x0f;
	mod = buf[6];
	if (mod > 6) mod = 6;
	srate = getbcd(buf + 7, 7) / 10;
	fec = buf[10] & 0x0f;
	if (fec > 10 && fec < 15) fec = 10;
	if (fec == 15) fec = 11;
	fprintf(fp,
		"%s  frequency %d FEC_outer %s modulation %s\n"
		"%s  symbol_rate %d FEC_inner %s\n",
		s, freq, FECO[delsys],MODC[mod],s, srate, FEC[fec]);
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
    int i, j, n;
    if(!step) step = 16;
    if (!length) return;
    n = 0;
    for (j = 0; j < length; j += step, b += step) {
	fprintf(fp,"%s%s  %03d: ",s,s2,n);
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
	n++;
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
    int length = desc->len;
    int c = 0;
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

    fprintf(fp,"%s  Linkage descriptor:\n",s);
    tsid = (buf[0] << 8) | buf[1];
    onid = (buf[2] << 8) | buf[3];
    sid = (buf[4] << 8) | buf[5];
    link = buf[6];
    
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
	uint8_t hand = (buf[c]&0xf0)>>4;
	uint8_t org = buf[c]&0x01;
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
    }
    buf += c;
    length -= c;
    if (length){
	fprintf(fp,"%s    private_data_bytes: %d %s\n",s, length,
		length>1 ? "bytes":"byte");
	dvb_print_data(fp, buf, length, 8,  s, "    ");
    }
}

static const char *service_type(uint8_t type)
{
    const char *t = "unknown";

    switch (type) {

    case 0x00:
    case 0x20 ... 0x7F:
    case 0x12 ... 0x15:
    case 0xFF:
	t = "reserved";
	break;
    case 0x01:
	t = "digital television service";
	break;
    case 0x02:
	t = "digital radio sound service";
	break;
    case 0x03:
	t = "Teletext service";
	break;
    case 0x04:
	t = "NVOD reference service";
	break;
    case 0x05:
	t = "NVOD time-shifted service";
	break;
    case 0x06:
	t = "mosaic service";
	break;
    case 0x07:
	t = "PAL coded signal";
	break;
    case 0x08:
	t = "SECAM coded signal";
	break;
    case 0x09:
	t = "D/D2-MAC";
	break;
    case 0x0A:
	t = "FM Radio";
	break;
    case 0x0B:
	t = "NTSC coded signal";
	break;
    case 0x0C:
	t = "data broadcast service";
	break;
    case 0x0D:
	t = "reserved for Common Interface usage";
	break;
    case 0x0E:
	t = "RCS Map (see EN 301 790)";
	break;
    case 0x0F:
	t = "RCS FLS (see EN 301 790)";
	break;
    case 0x10:
	t = "DVB MHP service";
	break;
    case 0x11:
	t = "MPEG-2 HD digital television service";
	break;
    case 0x16:
	    t = "H.264/AVC SD digital television service";
	break;
    case 0x17:
	    t = "H.264/AVC SD NVOD time-shifted service";
	break;
    case 0x18:
	    t = "H.264/AVC SD NVOD reference service";
	break;
    case 0x19:
	    t = "H.264/AVC HD digital television service";
	break;
    case 0x1A:
	    t = "H.264/AVC HD NVOD time-shifted service";
	break;
    case 0x1B:
	    t = "H.264/AVC HD NVOD reference service";
	break;
    case 0x1C:
	    t = "H.264/AVC frame compatible plano-stereoscopic HD digital television service ";
	break;
    case 0x1D:
	    t = "H.264/AVC frame compatible plano-stereoscopic HD NVOD time-shifted service";
	break;
    case 0x1E:
	    t = "H.264/AVC frame compatible plano-stereoscopic HD NVOD reference service";
	break;
    case 0x1F:
	    t = "HEVC digital television service";
	break;
    case 0x80 ... 0xFE:
	t = "user defined";
	break;
    }
    return t;
}


uint32_t dvb_print_descriptor(FILE *fp, descriptor *desc, char *s,
			      uint32_t priv_id)
{
    uint8_t *buf = desc->data;
    int c = 0;
    char *name=NULL;
    uint16_t id;

    
    fprintf(fp,"%sDescriptor tag: 0x%02x \n",s,desc->tag);
    switch(desc->tag){
    case 0x40:// network_name_descriptor
	fprintf(fp,"%s  Network name descriptor: \n",s);
	if ((name = dvb_get_name(buf,desc->len))){
	    fprintf(fp,"%s  name %s\n",s, name);
	    free(name);
	}
	break;
    case 0x41: //service list
	fprintf(fp,"%s  Service list descriptor:\n",s);
	for (int n = 0; n < desc->len; n+=3){
	    id = (buf[n] << 8) | buf[n+1];
	    fprintf(fp,"%s    service_id 0x%04x service_type %s\n",s, id,
		    service_type(buf[n+2]));
	}
	break;
    case 0x43: // satellite
	dvb_print_delsys_descriptor(fp, desc, s);
	break;
    
    case 0x44: // cable
	dvb_print_delsys_descriptor(fp, desc, s);
	break;

    case 0x48: //service descriptor
	fprintf(fp,"%s  Service descriptor:\n",s,desc->tag);
	fprintf(fp,"%s    service_type %s\n",s,service_type(buf[0])); 
	c++;
	int l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    fprintf(fp,"%s    provider %s",s, name);
	    free(name);
	}
	c += l;
	l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    fprintf(fp," name %s ", name);
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
	priv_id = (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
	fprintf(fp,"%s    private_data_specifier 0x%08x\n",s,priv_id);
	break;

    case 0x7f:
	fprintf(fp,"%s  Extension descriptor: \n",s);
	fprintf(fp,"%s    length: %d %s\n",s, desc->len,
		desc->len>1 ? "bytes":"byte");
	dvb_print_data(fp, desc->data, desc->len, 8, s, "  ");
	break;
	    
    case 0xfa: // isdbt
	dvb_print_delsys_descriptor(fp, desc, s);
	break;

    case 0xfb ... 0xfe:
    case 0x80 ... 0xf9: // user defined
	switch (priv_id){
	case NORDIG:
	    switch (desc->tag){
	    case 0x83:
	    case 0x87:
		fprintf(fp,"%s  NorDig Logical channel descriptor: \n",s);
		for (int n = 0; n < desc->len; n+=3){
		    id = (buf[n] << 8) | buf[n+1];
		    uint16_t lcn = ((buf[n+2]&0x3f) << 8) | buf[n+3];
		    fprintf(fp,
			    "%s    service_id 0x%04x logical channel number %d\n",
			    s, id, lcn);
		}
		break;
	    default:
		fprintf(fp,"%s  NorDig defined: \n",s);
		fprintf(fp,"%s    length: %d %s\n",s, desc->len,
			desc->len>1 ? "bytes":"byte");
		dvb_print_data(fp, desc->data, desc->len, 8, s, "  ");
		break;
	    }
	    break;

	default:
	    fprintf(fp,"%s  User defined descriptor:\n",s);
	    fprintf(fp,"%s    length: %d %s\n",s, desc->len,
		    desc->len>1 ? "bytes":"byte");
	    dvb_print_data(fp, desc->data, desc->len, 8, s, "  ");
	    break;
	}
	break;
    default:
	fprintf(fp,"%s  UNHANDLED descriptor: \n",s);
	fprintf(fp,"%s    length: %d %s\n",s, desc->len,
		desc->len>1 ? "bytes":"byte");
	dvb_print_data(fp, desc->data, desc->len, 8, s, "  ");
	break;
	
    }
    return priv_id;
}

