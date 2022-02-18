#include "dvb_print.h"
#include <stdarg.h>

static char table64[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'};

static int mod_table[] = {0, 2, 1};

static char *dtable() {

    char *dt = malloc(256);

    for (int i = 0; i < 64; i++)
        dt[(unsigned char) table64[i]] = i;
    return dt;
}

char *base64_encode(const uint8_t *data, int len, int *olen) {

    *olen = 4 * ((len + 2) / 3);

    char *edata = malloc(*olen);
    if (edata == NULL) return NULL;

    for (int i = 0, j = 0; i < len;) {

        uint32_t octet_a = i < len ? data[i++] : 0;
        uint32_t octet_b = i < len ? data[i++] : 0;
        uint32_t octet_c = i < len ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        edata[j++] = table64[(triple >> 3 * 6) & 0x3F];
        edata[j++] = table64[(triple >> 2 * 6) & 0x3F];
        edata[j++] = table64[(triple >> 1 * 6) & 0x3F];
        edata[j++] = table64[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[len % 3]; i++)
        edata[*olen - 1 - i] = '=';

    return edata;
}


unsigned char *base64_decode(uint8_t *data, int len, int *olen)
{
    
    char *dt = dtable();

    if (len % 4 != 0) return NULL;

    *olen = len / 4 * 3;
    if (data[len - 1] == '=') (*olen)--;
    if (data[len - 2] == '=') (*olen)--;

    unsigned char *ddata = malloc(*olen);
    if (ddata == NULL) return NULL;

    for (int i = 0, j = 0; i < len;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : dt[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : dt[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : dt[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : dt[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *olen) ddata[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *olen) ddata[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *olen) ddata[j++] = (triple >> 0 * 8) & 0xFF;
    }
    
    free(dt);
    return ddata;
}

static json_object* json_object_new_double_fmt(double d, const char *fmt)
{
    char tmp[128];
    snprintf(tmp, 128, fmt, d);
    return(json_object_new_double_s(d, tmp));
}

void pr(int fd, const char  *format,  ...)
{
    va_list args;
    int print=0;
    char finalstr[4096];
    int w=0;
    char nfor[256];
    snprintf(nfor, sizeof(nfor), "%s", format);
    va_start(args, format);
    vsnprintf(finalstr,sizeof(finalstr),format,args);
    w=write(fd, finalstr, strlen(finalstr));
    va_end(args);
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
	err("Error allocating memory\n");
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

void dvb_print_section(int fd, section *sec)
{
    int c=9;
    
    pr(fd,"section: table_id 0x%02x  syntax %d\n",
	    sec->table_id, sec->section_syntax_indicator);
    if (sec->section_syntax_indicator){
	pr(fd,"          id 0x%04x version_number 0x%02x\n" 
		"          section number: 0x%02x\n"
		"          last_section_number: 0x%02x\n",
		sec->id, sec->version_number, sec->section_number,
		sec->last_section_number);
	c+=5;
    }
    pr(fd,"        data (%d bytes):\n", sec->section_length+3);
    dvb_print_data(fd, sec->data,sec->section_length, 8,"          ", "");
    pr(fd,"\n");
}

void dvb_print_pat(int fd, PAT *pat)
{
    pr(fd,"PAT (0x%02x): transport_stream_id 0x%04x\n",
	    pat->pat->table_id, pat->pat->id);
    pr(fd,"  programs: \n");
    for(int n=0; n < pat->nprog; n++){
	pr(fd,"    program_number 0x%04x %s 0x%04x\n",
		pat->program_number[n],
		pat->program_number[n] ? "program_map_PID" : "network_PID",
		pat->pid[n]);
    }
    pr(fd,"\n");
}

void dvb_print_stream(int fd, pmt_stream *stream)
{
    pr(fd,"  stream: elementary_PID 0x%04x stream_type %s \n",
	    stream->elementary_PID, stream_type(stream->stream_type));
    if (stream->desc_num){
	uint32_t priv_id = 0;
	pr(fd,"  descriptors:\n");
	for (int n=0 ; n < stream->desc_num; n++){
	    priv_id = dvb_print_descriptor(fd, stream->descriptors[n],
					   "    ", priv_id);
	}
    }
}

void dvb_print_pmt(int fd, PMT *pmt)
{
    pr(fd,"PMT (0x%02x):  program_number 0x%04x  PCR_PID 0x%04x \n",
	    pmt->pmt->table_id, pmt->pmt->id, pmt->PCR_PID);
    if (pmt->desc_num) {
	pr(fd,"  program_info:\n");
	for (int n=0; n < pmt->desc_num; n++){
	    uint32_t priv_id = 0;
	    priv_id = dvb_print_descriptor(fd, pmt->descriptors[n],
					   "    ", priv_id);
	}
    }
    if (pmt->stream_num) {
	pr(fd,"  streams:\n");
	for (int n=0; n < pmt->stream_num; n++){
	    dvb_print_stream(fd, pmt->stream[n]);
	}
    }
    pr(fd,"\n");
}

void dvb_print_nit_transport(int fd, nit_transport *trans)
{
    pr(fd,"  transport:\n"
	    "    transport_stream_id 0x%04x original_network_id 0x%04x\n",
	    trans->transport_stream_id, trans->original_network_id);
    if (trans->desc_num){
	uint32_t priv_id = 0;
	pr(fd,"    descriptors:\n");
	for (int n=0 ; n < trans->desc_num; n++){
	    priv_id = dvb_print_descriptor(fd, trans->descriptors[n],
					   "      ", priv_id);
	}
    }
}

void dvb_print_nit(int fd, NIT *nit)
{
    pr(fd,"NIT (0x%02x): %s network (%d/%d) \n  network_id 0x%04x\n",
	    nit->nit->table_id, 
	    (nit->nit->table_id == 0x41) ? "other":"actual", 
	    nit->nit->section_number+1,
	    nit->nit->last_section_number+1, nit->nit->id);
    if (nit->ndesc_num){
	uint32_t priv_id = 0;
	pr(fd,"  network descriptors:\n");
	for (int n=0 ; n < nit->ndesc_num; n++){
	    priv_id = dvb_print_descriptor(fd, nit->network_descriptors[n],
					   "    ", priv_id);
	}
    }
    if (nit->trans_num){
	pr(fd,"  transports:\n");
	for (int n=0 ; n < nit->trans_num; n++){
	    dvb_print_nit_transport(fd, nit->transports[n]);
	}
    }
    pr(fd,"\n");
}

void dvb_print_sdt_service(int fd, sdt_service *serv)
{
    const char *R[] = {	"undefined","not running",
	"starts in a few seconds","pausing","running","service off-air",
	"unknown","unknown"};
    
    pr(fd,"  service:\n"
	    "    service_id 0x%04x EIT_schedule_flag 0x%02x\n"
	    "    EIT_present_following_flag 0x%02x\n"
	    "    running_status %s free_CA_mode 0x%02x\n",
	    serv->service_id, serv->EIT_schedule_flag,
	    serv->EIT_present_following_flag, R[serv->running_status],
	    serv->free_CA_mode);
    if (serv->desc_num){
	uint32_t priv_id = 0;
	pr(fd,"    descriptors:\n");
	for (int n=0 ; n < serv->desc_num; n++){
	    priv_id = dvb_print_descriptor(fd, serv->descriptors[n], "      ",
					   priv_id);
	}
    }
}

void dvb_print_sdt(int fd, SDT *sdt)
{
    pr(fd,"SDT (0x%02x): (%d/%d)\n  original_network_id 0x%04x ",
	    sdt->sdt->table_id, sdt->sdt->id, sdt->sdt->section_number,
	    sdt->sdt->last_section_number);
    if (sdt->service_num){
	pr(fd,"  services:\n");
	for (int n=0 ; n < sdt->service_num; n++){
	    dvb_print_sdt_service(fd, sdt->services[n]);
	}
    }
}

void dvb_print_delsys_descriptor(int fd, descriptor *desc, char *s)
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

    switch(desc->tag){
    case 0x43: // satellite
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
	pr(fd,
		"%s  frequency %d orbital_position %d west_east_flag %s\n"
		"%s  polarization %s  modulation_system %s",s,
		freq, orbit, east ? "E":"W", s, DVB_POL[pol],
		delsys ? "DVB-S2":"DVB-S");
	if (delsys) pr(fd," roll_off %.2f\n", DVB_roff[roll]);
	pr(fd,
		"%s  modulation_type %s symbol_rate %d FEC_inner %s\n",s, 
		DVB_MOD[mod], srate, DVB_FEC[fec]);
	break;

    case 0x44: // cable
	freq =  getbcd(buf, 8)/10;
	delsys = buf[5] & 0x0f;
	mod = buf[6];
	if (mod > 6) mod = 6;
	srate = getbcd(buf + 7, 7) / 10;
	fec = buf[10] & 0x0f;
	if (fec > 10 && fec < 15) fec = 10;
	if (fec == 15) fec = 11;
	pr(fd,
		"%s  frequency %d FEC_outer %s modulation %s\n"
		"%s  symbol_rate %d FEC_inner %s\n",
		s, freq, DVB_FECO[delsys],DVB_MODC[mod],s, srate, DVB_FEC[fec]);
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

char *get_network_name(NIT **nits)
{
    char *name = NULL;
    int n = nits[0]->nit->last_section_number+1;
    for (int i = 0; i < n; i++){
	for (int j = 0; j<nits[i]->ndesc_num; j++){
	    descriptor *desc = nits[i]->network_descriptors[j];
	    if (desc->tag == 0x40){
		uint8_t *buf = desc->data;
		name = dvb_get_name(buf,desc->len);
		break;
	    }
	}
	if (name) break;
    }
    return name;
}

void dvb_print_data(int fd, uint8_t *b, int length, int step,
		    char *s, char *s2)
{
    int i, j, n;
    if(!step) step = 16;
    if (!length) return;
    n = 0;
    for (j = 0; j < length; j += step, b += step) {
	pr(fd,"%s%s  %03d: ",s,s2,n);
	for (i = 0; i < step; i++)
	    if (i + j < length)
		pr(fd,"0x%02x ", b[i]);
	    else
		pr(fd,"     ");
	pr(fd," | ");
	for (i = 0; i < step; i++)
	    if (i + j < length)
		pr(fd,"%c",(b[i] > 31 && b[i] < 127) ? b[i] : '.');
	pr(fd,"\n");
	n++;
    }
    
}

void dvb_print_linkage_descriptor(int fd, descriptor *desc, char *s)
{
    uint16_t nid = 0;
    uint16_t onid = 0;
    uint16_t sid = 0;
    uint16_t tsid = 0;
    uint8_t link =0;
    uint8_t *buf = desc->data;
    int length = desc->len;
    int c = 0;

    tsid = (buf[0] << 8) | buf[1];
    onid = (buf[2] << 8) | buf[3];
    sid = (buf[4] << 8) | buf[5];
    link = buf[6];
    
    const char *lk = NULL;
    if (link < 0x0D) lk = DVB_L[link];
    else if(0x80 < link && link < 0xff) lk="user defined"; 
    else lk="reserved";
    
    pr(fd,
	    "%s    transport_stream_id 0x%04x original_network_id 0x%04x\n"
	    "%s    service_id 0x%04x linkage_type \"%s\"\n",s,tsid,onid,s,
	    sid, lk);
		

    c = 7;
    if (link == 0x08){
	uint8_t hand = (buf[c]&0xf0)>>4;
	uint8_t org = buf[c]&0x01;
	pr(fd,
		"%s    handover_type %s origin_type %s\n",s,
		DVB_H[hand], org ? "SDT":"NIT");
	if (hand ==0x01 || hand ==0x02 || hand ==0x03){
	    nid = (buf[c+1] << 8) | buf[c+2];
	    pr(fd,
		    "%s    network_id 0x%04x\n",s,nid);
	    c++;
	}
	if (!org){
	    sid = (buf[c+1] << 8) | buf[c+2];
	    pr(fd,
		    "%s    initial_service_id 0x%04x\n",s,sid);
	    c++;
	}
    }
    buf += c;
    length -= c;
    if (length){
	pr(fd,"%s    private_data_bytes: %d %s\n",s, length,
		length>1 ? "bytes":"byte");
	dvb_print_data(fd, buf, length, 8,  s, "    ");
    }
}

uint32_t dvb_print_descriptor(int fd, descriptor *desc, char *s,
			      uint32_t priv_id)
{
    uint8_t *buf = desc->data;
    int c = 0;
    char *name=NULL;
    uint16_t id;

    
    pr(fd,"%sDescriptor tag: 0x%02x \n",s,desc->tag);
    pr(fd,"%s  %s: \n",s,descriptor_type(desc->tag, priv_id));
    switch(desc->tag){
    case 0x00:
    case 0x01:
	pr(fd,"%s    length: %d %s\n",s, desc->len,
		desc->len>1 ? "bytes":"byte");
	dvb_print_data(fd, desc->data, desc->len, 8, s, "  ");
	break;
    case 0x02:
	break;	
    case 0x03:
	break;	
    case 0x04:
	break;	
    case 0x05:
	break;	
    case 0x06:
	break;	
    case 0x07:
	break;	
    case 0x08:
	break;	
    case 0x09:
	break;	
    case 0x0a:
	break;	
    case 0x0b:
	break;	
    case 0x0c:
	break;	
    case 0x0d:
	break;	
    case 0x0e:
	break;	
    case 0x0f:
	break;	
    case 0x10:
	break;	
    case 0x11:
	break;	
    case 0x12:
	break;	
    case 0x13 ... 0x3F:
	break;	
    case 0x40:// network_name_descriptor
	if ((name = dvb_get_name(buf,desc->len))){
	    pr(fd,"%s  name %s\n",s, name);
	    free(name);
	}
	break;
    case 0x41: //service list
	for (int n = 0; n < desc->len; n+=3){
	    id = (buf[n] << 8) | buf[n+1];
	    pr(fd,"%s    service_id 0x%04x service_type %s\n",s, id,
		    service_type(buf[n+2]));
	}
	break;
    case 0x43: // satellite
	dvb_print_delsys_descriptor(fd, desc, s);
	break;
    
    case 0x44: // cable
	dvb_print_delsys_descriptor(fd, desc, s);
	break;

    case 0x48: //service descriptor
	pr(fd,"%s    service_type %s\n",s,service_type(buf[0])); 
	c++;
	int l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    pr(fd,"%s    provider %s",s, name);
	    free(name);
	}
	c += l;
	l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    pr(fd," name %s ", name);
	    free(name);
	}
	pr(fd,"\n");
	break;

    case 0x4a:
	dvb_print_linkage_descriptor(fd, desc, s);
	break;
	
    case 0x52:
	pr(fd,"%s    component_tag 0x%02x\n",s,buf[0]);
	break;

    case 0x56:
	break;

    case 0x59:
	break;

    case 0x5a: // terrestrial
	dvb_print_delsys_descriptor(fd, desc, s);
	break;

    case 0x5f:
	priv_id = (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
	pr(fd,"%s    private_data_specifier 0x%08x\n",s,priv_id);
	break;

    case 0x66:
	break;

    case 0x6a:
	break;

    case 0x6c:
	break;

    case 0x7f:
	pr(fd,"%s    length: %d %s\n",s, desc->len,
		desc->len>1 ? "bytes":"byte");
	dvb_print_data(fd, desc->data, desc->len, 8, s, "  ");
	break;
	    
    case 0xfa: // isdbt
	dvb_print_delsys_descriptor(fd, desc, s);
	break;

    case 0xfb ... 0xfe:
    case 0x80 ... 0xf9: // user defined
	switch (priv_id){
	case NORDIG:
	    switch (desc->tag){
	    case 0x83:
	    case 0x87:
		for (int n = 0; n < desc->len; n+=4){
		    id = (buf[n] << 8) | buf[n+1];
		    uint16_t lcn = ((buf[n+2]&0x3f) << 8) | buf[n+3];
		    pr(fd,
			    "%s    service_id 0x%04x logical channel number %d\n",
			    s, id, lcn);
		}
		break;
	    default:
		pr(fd,"%s    length: %d %s\n",s, desc->len,
			desc->len>1 ? "bytes":"byte");
		dvb_print_data(fd, desc->data, desc->len, 8, s, "  ");
		break;
	    }
	    break;

	default:
	    pr(fd,"%s    length: %d %s\n",s, desc->len,
		    desc->len>1 ? "bytes":"byte");
	    dvb_print_data(fd, desc->data, desc->len, 8, s, "  ");
	    break;
	}
	break;
    default:
	pr(fd,"%s    length: %d %s\n",s, desc->len,
		desc->len>1 ? "bytes":"byte");
	dvb_print_data(fd, desc->data, desc->len, 8, s, "  ");
	break;

    }
    return priv_id;
}


static json_object *dvb_data_json(uint8_t *data, int len)
{
    int olen = 0;
    char *sdata = base64_encode(data, len,&olen);

    return json_object_new_string_len(sdata,len);
}

json_object *dvb_section_json(section *sec, int d)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    int c=9;
    
    json_object_object_add(jobj, "table_id",
			   json_object_new_int(sec->table_id));
    json_object_object_add(jobj, "syntax_indicator",
			   json_object_new_int(sec->section_syntax_indicator));
    json_object_object_add(jobj,"length",
			   json_object_new_int(sec->section_length));
    if(d) {
	json_object_object_add(jobj,"data",
			       dvb_data_json(sec->data, sec->section_length));
    }
    if (sec->section_syntax_indicator){
	json_object_object_add(jobj, "section_number",
			   json_object_new_int(sec->section_number));
	json_object_object_add(jobj, "last_section_number",
			       json_object_new_int(sec->last_section_number));
	json_object_object_add(jobj, "version_number",
			       json_object_new_int(sec->version_number));
	if(d) {
	    json_object_object_add(jobj, "id",
				   json_object_new_int(sec->id));
	}
    }
    return jobj;
}
 
json_object *dvb_delsys_descriptor_json(descriptor *desc)
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

    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj,"tag",
			   json_object_new_int(desc->tag));
    json_object_object_add(jobj,"type",
			   json_object_new_string(descriptor_type(desc->tag,0)));

    switch(desc->tag){
    case 0x7f: //T2_delivery_system_descriptor
    {
	uint8_t etag = buf[0];
	json_object_object_add(jobj,"extended tag",
			       json_object_new_int(etag));
	json_object_object_add(jobj,"extended type",
			       json_object_new_string(
				   extended_descriptor_type(etag)));
	switch (etag){
	case 0x04: //T2_delivery_system_descriptor
	{
	    uint16_t t2id = (buf[2] << 8) | buf[3];
		
	    json_object_object_add(jobj,"plp_id",
				   json_object_new_int(buf[1]));
	    json_object_object_add(jobj,"T2_system_id",
				   json_object_new_int(t2id));
	    if (desc->len > 4){
		int sisomiso = (buf[4]>>6)&0x03;
		int bandw = 8-((buf[4]>>2)&0x0F);
		int guard = (buf[5]>>5)&0x07;
		int trans = (buf[5]>>2)&0x07;
		int ofreq = (buf[5]>>1)&0x01;
		int tfs = buf[5]&0x01;
		int c = 0;
		
		if (bandw == 4) bandw=10;
		if (bandw == 3) bandw=1712;
	    
		json_object_object_add(jobj,"SISO/MISO",
				       json_object_new_string((sisomiso ? "MISO":"SISO")));
		json_object_object_add(jobj,"bandwidth",
				       json_object_new_int(bandw));
		json_object_object_add(jobj,"guard_interval",
				       json_object_new_string(
					   DVB_GUARD[guard]));
		json_object_object_add(jobj,"transmission_mode",
				       json_object_new_string(
					   DVB_TRANS[trans]));
		json_object_object_add(jobj,"other_frequency_flag",
				       json_object_new_int(ofreq));
		json_object_object_add(jobj,"tfs_flag",
				       json_object_new_int(tfs));

		json_object *jarray = json_object_new_array();
		json_object *ja = json_object_new_object();
		c =6;
		while (c < desc->len){
		    uint16_t cid = (buf[c] << 8) | buf[c+1];
		    
		    json_object_object_add(ja,"cell_id",
					   json_object_new_int(cid));
		    c += 2;
		    if (tfs){
			int llen = buf[c];
			int cs = c+1;
			json_object *jarrayf = json_object_new_array();
			while (cs < llen+c){
			    freq = (buf[cs+3]|(buf[cs+2] << 8)|(buf[cs+1] << 16)
				    |(buf[cs] << 24))*10;
			    json_object_array_add(jarrayf,
						  json_object_new_int(freq));
			    cs+=4;
			}
			json_object_object_add(ja, "center_frequencies", jarray);
			c += llen+2;
		    } else {
			freq = (buf[c+3]|(buf[c+2] << 8)|(buf[c+1] << 16)
				|(buf[c] << 24))*10;
			json_object_object_add(ja,"center_frequency",
				       json_object_new_int(freq));
			c+=4;
		    }
		    int sublen = buf[c];
		    c+=sublen+1;
		    json_object_array_add(jarray, ja);
		}
		json_object_object_add(jobj, "Cells", jarray);
		
	    }
	    break;
	}
	case 0x05: //SH_delivery_system_descriptor 
	case 0x0D: //C2_delivery_system_descriptor 
	case 0x16: //C2_bundle_delivery_system_descriptor 
	case 0x17: //S2X_delivery_system_descriptor 
	    
	    break;
	}
	break;
    }
    case 0x43: // satellite
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
	json_object_object_add(jobj,"frequency",
			       json_object_new_int(freq));
	json_object_object_add(jobj,"orbital_position",
			       json_object_new_int(orbit));
	json_object_object_add(jobj,"west_east_flag nr",
			       json_object_new_int(east));
	json_object_object_add(jobj,"west_east_flag",
			       json_object_new_string((east ? "E":"W")));
	json_object_object_add(jobj,"polarisation nr",
			       json_object_new_int(pol));
	json_object_object_add(jobj,"polarisation",
			       json_object_new_string(DVB_POL[pol]));
	json_object_object_add(jobj,"modulation_system nr",
			       json_object_new_int(delsys));
	json_object_object_add(jobj,"modulation_system",
			       json_object_new_string(
				   (delsys ? "DVB-S2":"DVB-S")));
	if (delsys) {
	    json_object_object_add(jobj,"roll_off nr",
				   json_object_new_int(roll));
	    json_object_object_add(jobj,"roll_off",
				   json_object_new_double_fmt(DVB_roff[roll],
							      "%0.2f"));
	}
	json_object_object_add(jobj,"modulation_type nr",
			       json_object_new_int(mod));
	json_object_object_add(jobj,"modulation_type",
			       json_object_new_string(DVB_MOD[mod]));
	json_object_object_add(jobj,"symbol_rate", json_object_new_int(srate));
	json_object_object_add(jobj,"FEC_Inner nr",
			       json_object_new_int(fec));
	json_object_object_add(jobj,"FEC_Inner",
			       json_object_new_string(DVB_FEC[fec]));
	break;

    case 0x44: // cable
	freq =  getbcd(buf, 8)/10;
	delsys = buf[5] & 0x0f;
	mod = buf[6];
	if (mod > 6) mod = 6;
	srate = getbcd(buf + 7, 7) / 10;
	fec = buf[10] & 0x0f;
	if (fec > 10 && fec < 15) fec = 10;
	if (fec == 15) fec = 11;

	json_object_object_add(jobj,"frequency",
			       json_object_new_int(freq));
	json_object_object_add(jobj,"FEC_outer nr",
			       json_object_new_int(delsys));
	json_object_object_add(jobj,"FEC_outer",
			       json_object_new_string(DVB_FECO[delsys]));
	json_object_object_add(jobj,"modulation nr",
			       json_object_new_int(mod));
	json_object_object_add(jobj,"modulation",
			       json_object_new_string(DVB_MODC[mod]));
	json_object_object_add(jobj,"symbol_rate",
			       json_object_new_int(srate));
	json_object_object_add(jobj,"FEC_inner nr",
			       json_object_new_int(delsys));
	json_object_object_add(jobj,"FEC_inner",
			       json_object_new_string(DVB_FEC[delsys]));
	break;

    case 0x5a: // terrestrial
	freq = (buf[3]|(buf[2] << 8)|(buf[1] << 16)|(buf[0] << 24))*10;
	delsys = SYS_DVBT;
	json_object_object_add(jobj,"frequency",
			       json_object_new_int(freq));
	break;

    case 0xfa: // isdbt
	freq = (buf[5]|(buf[4] << 8))*7000000;
	json_object_object_add(jobj,"frequency",
			       json_object_new_int(freq));
	delsys = SYS_ISDBT;
    }
    return jobj;
}

json_object *dvb_linkage_descriptor_json(descriptor *desc)
{
    uint16_t nid = 0;
    uint16_t onid = 0;
    uint16_t sid = 0;
    uint16_t tsid = 0;
    uint8_t link =0;
    uint8_t *buf = desc->data;
    int length = desc->len;
    int c = 0;

    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj,"tag",
			   json_object_new_int(desc->tag));
    json_object_object_add(jobj,"type", json_object_new_string(
			       "Linkage descriptor"));
    tsid = (buf[0] << 8) | buf[1];
    onid = (buf[2] << 8) | buf[3];
    sid = (buf[4] << 8) | buf[5];
    link = buf[6];
    
    const char *lk = NULL;
    if (link < 0x0D) lk = DVB_L[link];
    else if(0x80 < link && link < 0xff) lk="user defined"; 
    else lk="reserved";
    
    json_object_object_add(jobj,"transport_stream_id",
			   json_object_new_int(tsid));
    json_object_object_add(jobj,"original_network_id",
			   json_object_new_int(onid));
    json_object_object_add(jobj,"service id",
			   json_object_new_int(sid));
    json_object_object_add(jobj,"linkage_type nr",
			   json_object_new_int(link));
    json_object_object_add(jobj,"linkage_type",
			   json_object_new_string(lk));
    
    c = 7;
    if (link == 0x08){
	uint8_t hand = (buf[c]&0xf0)>>4;
	uint8_t org = buf[c]&0x01;
	json_object_object_add(jobj,"handover_type",
			       json_object_new_string(DVB_H[hand]));
	json_object_object_add(jobj,"handover_type nr",
			       json_object_new_int(hand));
	json_object_object_add(jobj,"handover_type",
			       json_object_new_string((org ? "SDT":"NIT")));
	json_object_object_add(jobj,"origin_type nr",
			       json_object_new_int(org));
	if (hand ==0x01 || hand ==0x02 || hand ==0x03){
	    nid = (buf[c+1] << 8) | buf[c+2];
	    json_object_object_add(jobj,"network_id",
				   json_object_new_int(nid));
	    c++;
	}
	if (!org){
	    sid = (buf[c+1] << 8) | buf[c+2];
	    json_object_object_add(jobj,"initia_ service_id",
				   json_object_new_int(sid));
	    c++;
	}
    }
    buf += c;
    length -= c;
    if (length){
	json_object_object_add(jobj,"length",
			       json_object_new_int(length));
	json_object_object_add(jobj,"data",
			       dvb_data_json(buf, length));
    }
    return jobj;
}

json_object *dvb_descriptor_json(descriptor *desc, uint32_t *priv_id)
{
    uint8_t *buf = desc->data;
    int c = 0;
    char *name=NULL;
    uint16_t id;
    json_object *jobj = json_object_new_object();
    json_object *jarray;
    json_object *jarray2;

    json_object_object_add(jobj,"tag",
			   json_object_new_int(desc->tag));
    json_object_object_add(jobj,"type",
			   json_object_new_string(descriptor_type(desc->tag,
								  *priv_id)));
    switch(desc->tag){
    case 0x00:
    case 0x01:
	json_object_object_add(jobj,"length",
			       json_object_new_int(desc->len));
	json_object_object_add(jobj,"data",
			       dvb_data_json(desc->data, desc->len));
	break;
    case 0x02:
	break;	
    case 0x03:
	break;	
    case 0x04:
	break;	
    case 0x05:
	break;	
    case 0x06:
	break;	
    case 0x07:
	break;	
    case 0x08:
	break;	
    case 0x09:
	break;	
    case 0x0a:
	break;	
    case 0x0b:
	break;	
    case 0x0c:
	break;	
    case 0x0d:
	break;	
    case 0x0e:
	break;	
    case 0x0f:
	break;	
    case 0x10:
	break;	
    case 0x11:
	break;	
    case 0x12:
	break;	
    case 0x13 ... 0x3F:
	json_object_object_add(jobj,"length",
			       json_object_new_int(desc->len));
	json_object_object_add(jobj,"data",
			       dvb_data_json(desc->data, desc->len));
	break;	
    case 0x40:// network_name_descriptor
	if ((name = dvb_get_name(buf,desc->len))){
	    json_object_object_add(jobj,"name",
				   json_object_new_string(name));
	    free(name);
	}
	break;
    case 0x41: //service list
	jarray = json_object_new_array();

	for (int n = 0; n < desc->len; n+=3){
	    json_object *ja = json_object_new_object();
	    id = (buf[n] << 8) | buf[n+1];
	    json_object_object_add(ja, "service id",
				   json_object_new_int( id));
	    json_object_object_add(ja, "service type nr",
				   json_object_new_int( buf[n+2]));
	    json_object_object_add(ja, "service type",
				   json_object_new_string(
				       service_type(buf[n+2])));
	    json_object_array_add(jarray, ja);
	}
	json_object_object_add(jobj, "Services", jarray);
	break;
    case 0x43: // satellite
    case 0x44: // cable
    case 0x5a: // terrestrial
    case 0xfa: // isdbt
	json_object_put(jobj);
	jobj = dvb_delsys_descriptor_json(desc);
	break;

    case 0x48: //service descriptor
	json_object_object_add(jobj,"service type",
			       json_object_new_string(
				   service_type(buf[0]))); 
	json_object_object_add(jobj,"service type nr",
			       json_object_new_int(buf[0])); 
	c++;
	int l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    json_object_object_add(jobj,"provider",
				   json_object_new_string(name));
	    free(name);
	}
	c += l;
	l = buf[c];
	c++;
	if ((name = dvb_get_name(buf+c,l))){
	    json_object_object_add(jobj,"name",
				   json_object_new_string(name));
	    free(name);
	}
	break;

    case 0x4a:
	json_object_put(jobj);
	jobj = dvb_linkage_descriptor_json(desc);
	break;

    case 0x52:
	json_object_object_add(jobj,"component_tag",
			       json_object_new_int(buf[0]));
	break;
	
    case 0x56:
	break;

    case 0x59:
	break;
	
    case 0x5f:
	*priv_id = (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
	json_object_object_add(jobj,"private_data_specifier",
			       json_object_new_int(*priv_id));
	break;

    case 0x62:
	jarray = json_object_new_array();
	int cs = 1;
	uint32_t freq=0;
	int ctype = buf[0]&0x03;
	const char *ct[]={"not defined","satellite","cable","terrestrial"};
	
	json_object_object_add(jobj,"coding type",
			       json_object_new_string(ct[ctype]));
	
	while (cs < desc->len){
	    switch (ctype){
	    case 1:
		freq = getbcd(buf+cs, 8) *10;
		break;
	    case 2:
	    	freq =  getbcd(buf+cs, 8)/10;
		break;
	    case 3:
		freq = (buf[cs+3]|(buf[cs+2] << 8)|(buf[cs+1] << 16)
			|(buf[cs] << 24))*10;
		break;
	    }
	    json_object_array_add(jarray,
				  json_object_new_int(freq));
	    cs+=4;
	}
	json_object_object_add(jobj, "center_frequencies", jarray);

	break;

    case 0x66:
	break;

    case 0x6a:
	break;
	
    case 0x6c:
    {
	int c = 0;
	int len = desc->len;
	jarray = json_object_new_array();
	int s = 0;
	while (c < len){
	    json_object *ja = json_object_new_object();
	    uint16_t cid = (buf[c] << 8) | buf[c+1];
	    int16_t lat = (buf[c+2] << 8) | buf[c+3];
	    int16_t lon = (buf[c+4] << 8) | buf[c+5];
	    int16_t elat = (buf[c+6] << 4) | ((buf[c+7] >> 4) & 0x0f);
	    int16_t elon = ((buf[c+7]&0x0f) << 8) | buf[c+8];
	    json_object_object_add(ja,"cell_id",
				   json_object_new_int(cid));
	    json_object_object_add(ja,"latitude",
				   json_object_new_double(lat*90/32768));
	    json_object_object_add(ja,"longitude",
				   json_object_new_double(lon*180/32768));
	    json_object_object_add(ja,"latitude_extend",
				   json_object_new_double(elat*90/32768));
	    json_object_object_add(ja,"longitude_extend",
				   json_object_new_double(elon*180/32768));
	    s = buf[c+9]; 
	    c += 10;
	    if (s >0){
		int cs = 0;
	    
		jarray2 = json_object_new_array();
		while (cs < s){
		    json_object *ja2 = json_object_new_object();
		    cid =  buf[c+cs];
		    lat = (buf[c+cs+1] << 8) | buf[c+cs+2];
		    lon = (buf[c+cs+3] << 8) | buf[c+cs+4];
		    elat = (buf[c+cs+5] << 4) | ((buf[c+cs+6]>> 4) &0x0f);
		    elon = ((buf[c+cs+6]&0x0f) << 8) | buf[c+cs+7];
		    json_object_object_add(ja2,"cell_id_extension",
					   json_object_new_int(cid));
		    json_object_object_add(ja2,"subcell_latitude",
					   json_object_new_double(lat*90/32768));
		    json_object_object_add(ja2,"subcell_longitude",
					   json_object_new_double(lon*180/32768));
		    json_object_object_add(ja2,"subcell_latitude_extend",
					   json_object_new_double(elat*90/32768));
		    json_object_object_add(ja2,"subcell_longitude_extend",
					   json_object_new_double(elon*180/32768));
		    cs += 8;
		    json_object_array_add(jarray2, ja2);
		}
		json_object_object_add(ja, "Sub_Cells", jarray2);
		c+= s;
	    }
	    json_object_array_add(jarray, ja);
	}
	json_object_object_add(jobj, "Cells", jarray);
	break;
    }	

    case 0x7f:
    {
	int len = desc->len;
	uint8_t etag = buf[0];
	switch (etag){
	case 0x04: //T2_delivery_system_descriptor 
	case 0x05: //SH_delivery_system_descriptor 
	case 0x0D: //C2_delivery_system_descriptor 
	case 0x16: //C2_bundle_delivery_system_descriptor 
	case 0x17: //S2X_delivery_system_descriptor 
		jobj = dvb_delsys_descriptor_json(desc);
	    break;
	default:
	    json_object_object_add(jobj,"descriptor_tag_extension",
				   json_object_new_int(etag));
	    json_object_object_add(jobj,"type",
				   json_object_new_string(
				       extended_descriptor_type(etag)));
	    json_object_object_add(jobj,"length",
				   json_object_new_int(desc->len));
	    json_object_object_add(jobj,"data",
			       dvb_data_json(desc->data+1, desc->len-1));
	    break;

	}
	break;
    }   

    case 0xfb ... 0xfe:
    case 0x80 ... 0xf9: // user defined
	switch (*priv_id){
	case NORDIG:
	    switch (desc->tag){
	    case 0x83:
	    case 0x87:
		jarray = json_object_new_array();
		
		for (int n = 0; n < desc->len; n+=4){
		    id = (buf[n] << 8) | buf[n+1];
		    uint16_t lcn = ((buf[n+2]&0x3f) << 8) | buf[n+3];
		    json_object *ja = json_object_new_object();
		    json_object_object_add(ja, "service_id",
					   json_object_new_int(id));
		    json_object_object_add(ja, "logical channel number",
					   json_object_new_int( lcn));
		    json_object_array_add(jarray,ja);
		}
		json_object_object_add(jobj, "channel list",jarray);
		break;
	    default:
		json_object_object_add(jobj,"length",
				       json_object_new_int(desc->len));
		json_object_object_add(jobj,"data",
				       dvb_data_json(desc->data, desc->len));
		break;
	    }
	    break;

	default:
	    json_object_object_add(jobj,"length",
				   json_object_new_int(desc->len));
	    json_object_object_add(jobj,"data",
				   dvb_data_json(desc->data, desc->len));
	    break;
}
	break;
    default:
	json_object_object_add(jobj,"length",
			       json_object_new_int(desc->len));
	json_object_object_add(jobj,"data",
			       dvb_data_json(desc->data, desc->len));
	break;
	
    }
    return jobj;
}

void dvb_descriptor_json_array_add(json_object *jobj, const char *key,
				   descriptor **desc, int desc_num)
{
    uint32_t priv_id = 0;
    json_object *jarray = json_object_new_array();
    for (int n=0 ; n < desc_num; n++){
	json_object_array_add(jarray, dvb_descriptor_json(desc[n], &priv_id));
    }
    json_object_object_add(jobj, key, jarray);
}
    
json_object *dvb_pat_json(PAT *pat)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    json_object_object_add(jobj, "section data", dvb_section_json(pat->pat,0));
    json_object_object_add(jobj, "transport_stream_id",
			   json_object_new_int(pat->pat->id));
    jarray = json_object_new_array();
    for(int n=0; n < pat->nprog; n++){
	if (pat->program_number[n]){
	    json_object *ja = json_object_new_object();
	    json_object_object_add(ja, "program_number",
				   json_object_new_int(pat->program_number[n]));
	    json_object_object_add(ja, "program_map_PID",
				   json_object_new_int( pat->pid[n]));
	    
	    json_object_array_add(jarray,ja);
	} else {
	    json_object_object_add(jobj, "network_id",
				   json_object_new_int( pat->pid[n]));
	}
    }
    json_object_object_add(jobj, "programs", jarray);
    return jobj;
}

json_object *dvb_stream_json(pmt_stream *stream)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    json_object_object_add(jobj, "elementary_PID",
			   json_object_new_int(stream->elementary_PID));
    json_object_object_add(jobj, "stream_type_nr",
			   json_object_new_int(stream->stream_type));
    json_object_object_add(jobj, "stream_type",
			   json_object_new_string(
			       stream_type(stream->stream_type)));
    if (stream->desc_num){
	dvb_descriptor_json_array_add(jobj, "descriptors",
				      stream->descriptors, stream->desc_num);
    }
    return jobj;
}

json_object *dvb_pmt_json(PMT *pmt)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    json_object_object_add(jobj, "section data", dvb_section_json(pmt->pmt,0));
    json_object_object_add(jobj, "program_number",
			   json_object_new_int(pmt->pmt->id));
    json_object_object_add(jobj, "PCR_PID",
			   json_object_new_int(pmt->PCR_PID));

    if (pmt->desc_num) {
	dvb_descriptor_json_array_add(jobj, "program info descriptors",
				      pmt->descriptors, pmt->desc_num);
    }
    if (pmt->stream_num) {
	jarray = json_object_new_array();
	for (int n=0; n < pmt->stream_num; n++){
	    json_object_array_add(jarray, dvb_stream_json(pmt->stream[n]));
	}
	json_object_object_add(jobj, "streams", jarray);
    }
    return jobj;
}

json_object *dvb_nit_transport_json(nit_transport *trans)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    json_object_object_add(jobj, "transport_stream_id",
			   json_object_new_int(trans->transport_stream_id));
    json_object_object_add(jobj, "original_network_id",
			   json_object_new_int(trans->original_network_id));

    if (trans->desc_num){
	dvb_descriptor_json_array_add(jobj, "descriptors",
				      trans->descriptors, trans->desc_num);
    }
    return jobj;
}

json_object *dvb_nit_json(NIT *nit)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    json_object_object_add(jobj, "section data", dvb_section_json(nit->nit,0));
    json_object_object_add(jobj, "network_id",
			   json_object_new_int(nit->nit->id));
    json_object_object_add(jobj, "type",
			   json_object_new_string(
			       (nit->nit->table_id == 0x41) ?
			       "other":"actual"));

    if (nit->ndesc_num){
	dvb_descriptor_json_array_add(jobj, "network_descriptors",
				      nit->network_descriptors,
				      nit->ndesc_num);
    }
    if (nit->trans_num){
	jarray = json_object_new_array();
	for (int n=0 ; n < nit->trans_num; n++){
 	    json_object_array_add(jarray,
				  dvb_nit_transport_json(nit->transports[n]));
	}
	json_object_object_add(jobj, "transports", jarray);
    }
    return jobj;
}


json_object *dvb_sdt_service_json(sdt_service *serv)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    const char *R[] = {	"undefined","not running",
	"starts in a few seconds","pausing","running","service off-air",
	"unknown","unknown"};
    
    json_object_object_add(jobj, "service_id",
			   json_object_new_int(serv->service_id));
    json_object_object_add(jobj, "EIT_schedule_flag",
			   json_object_new_int(serv->EIT_schedule_flag));
    json_object_object_add(jobj,"EIT_present_following_flag",
			   json_object_new_int(
			       serv->EIT_present_following_flag));
    json_object_object_add(jobj, "running_status",
			   json_object_new_string(R[serv->running_status]));
    json_object_object_add(jobj, "free_CA_mode",
			   json_object_new_int(serv->free_CA_mode));


    if (serv->desc_num){
	dvb_descriptor_json_array_add(jobj, "descriptors",
				      serv->descriptors,serv->desc_num);
    }
    return jobj;
}

json_object *dvb_sdt_json(SDT *sdt)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    json_object_object_add(jobj, "section data", dvb_section_json(sdt->sdt,0));
    json_object_object_add(jobj, "original_network_id",
			   json_object_new_int(sdt->sdt->id));
    if (sdt->service_num){
	jarray = json_object_new_array();
	for (int n=0 ; n < sdt->service_num; n++){
 	    json_object_array_add(jarray,
				  dvb_sdt_service_json(sdt->services[n]));
	}
	json_object_object_add(jobj, "services", jarray);
    }
    return jobj;
}

json_object *dvb_all_pat_json(PAT **pats)
{
    json_object *jobj = json_object_new_object();
    int npat = pats[0]->pat->last_section_number+1;
    json_object *jpat = json_object_new_array();
    for (int i=0; i < npat; i++){
	json_object_array_add(jpat, dvb_pat_json(pats[i]));
    }
    json_object_object_add(jobj, "PAT", jpat);
    return jobj;
}

json_object *dvb_all_pmt_json(PMT **pmts)
{
    json_object *jobj = json_object_new_object();
    int npmt = pmts[0]->pmt->last_section_number+1;
    json_object *jpmt = json_object_new_array();
    for (int i=0; i < npmt; i++){
	json_object_array_add(jpmt, dvb_pmt_json(pmts[i]));
    }
    json_object_object_add(jobj, "PMT", jpmt);
    return jobj;
}

json_object *dvb_all_nit_json(NIT **nits)
{
    int nnit = nits[0]->nit->last_section_number+1;
    json_object *jobj = json_object_new_object();
    json_object *jarray = json_object_new_array();
    for (int n=0; n < nnit; n++){
	json_object_array_add (jarray,dvb_nit_json(nits[n]));
    }
    json_object_object_add(jobj, "NIT", jarray);
    return jobj;
}

json_object *dvb_all_sdt_json(SDT **sdts)
{
    int n = sdts[0]->sdt->last_section_number+1;
    json_object *jobj = json_object_new_object();
    json_object *jarray = json_object_new_array();
    for (int i=0; i < n; i++){
	json_object_array_add (jarray,dvb_sdt_json(sdts[i]));
    }
    json_object_object_add(jobj, "SDT", jarray);
    return jobj;
}

json_object *dvb_service_json(service *serv)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    char *name = NULL;
    uint32_t p;
    
    json_object_object_add(jobj,"id", json_object_new_int(serv->id));
    if (serv->sdt_service){
	for (int j = 0; j< serv->sdt_service->desc_num; j++){
	    descriptor *desc = serv->sdt_service->descriptors[j];
	    if (desc->tag == 0x48){
		json_object_object_add(jobj,"service descriptor",
				       dvb_descriptor_json(desc,&p));
		break;
	    }
	}
    }

    if (serv->pmt){
	jarray = json_object_new_array();	
	for (int i= 0; i < serv->pmt[0]->pmt->last_section_number+1; i++){
	    PMT *pmt = serv->pmt[i];
	    for (int j=0; j < pmt->stream_num; j++){
		pmt_stream *stream = pmt->stream[j];
		json_object *jp = json_object_new_object();
		json_object_object_add(jp,"pid",json_object_new_int(
					   stream->elementary_PID));
		json_object_object_add(jp, "stream_type_nr",
			   json_object_new_int(stream->stream_type));
		json_object_object_add(jp,"type",json_object_new_string(
					   stream_type(stream->stream_type)));
		json_object_array_add (jarray,jp);
	    }
	}
	json_object_object_add(jobj,"pids",jarray);
    }
    return jobj;
}

json_object *dvb_transport_json(transport *trans)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;
    json_object_object_add(jobj,"frontend", dvb_fe_json(&trans->fe));

    jarray = json_object_new_array();
    for (int i=0; i < trans->nserv; i++){
	service *serv = &trans->serv[i];
	json_object_array_add(jarray, dvb_service_json(serv));
    }
    json_object_object_add(jobj, "services", jarray);

    return jobj;
}

json_object *dvb_devices_json(dvb_devices *dev)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;
    json_object_object_add(jobj,"adapter", json_object_new_int(dev->adapter));
    json_object_object_add(jobj,"num", json_object_new_int(dev->num));

    return jobj;
}

json_object *dvb_fe_json(dvb_fe *fe)
{
    json_object *jobj = json_object_new_object();

    json_object_object_add(jobj,"delivery system", json_object_new_string(
			       delsys_name(fe->delsys)));
    json_object_object_add(jobj,"frequency", json_object_new_int(fe->freq));
    json_object_object_add(jobj,"symbol rate", json_object_new_int(fe->sr));
    json_object_object_add(jobj,"fec", json_object_new_int(fe->fec));
    json_object_object_add(jobj,"input", json_object_new_int(fe->input));
    if (fe->delsys == SYS_DVBS || fe->delsys == SYS_DVBS2){
	json_object_object_add(jobj,"pol", json_object_new_string(
				   fe->pol ? "h":"v" ));
	json_object_object_add(jobj,"band", json_object_new_string(
				   fe->hi ? "upper":"lower"));
    }

    return jobj;
}

json_object *dvb_lnb_json(dvb_lnb *lnb)
{
    json_object *jobj = json_object_new_object();

    json_object_object_add(jobj,"type", json_object_new_int(lnb->type));
    json_object_object_add(jobj,"num", json_object_new_int(lnb->num));
    json_object_object_add(jobj,"lofs", json_object_new_int(lnb->lofs));
    json_object_object_add(jobj,"lof1", json_object_new_int(lnb->lof1));
    json_object_object_add(jobj,"lof2", json_object_new_int(lnb->lof2));

    if (lnb->type > 0){
	json_object_object_add(jobj,"scif_slot",
			       json_object_new_int(lnb->scif_slot));
	json_object_object_add(jobj,"scif_freq",
			       json_object_new_int(lnb->scif_freq));
    }
    return jobj;
}

json_object *dvb_satellite_json(satellite *sat)
{
    json_object *jobj = json_object_new_object();
    json_object *jarray;

    json_object_object_add(jobj, "delivery system",
			   json_object_new_string(delsys_name(sat->delsys)));
    json_object_object_add(jobj, "network name",
			   json_object_new_string(get_network_name(sat->nit)));
    json_object_object_add(jobj, "devices", dvb_devices_json(&sat->dev));
    if (sat->delsys == SYS_DVBS || sat->delsys == SYS_DVBS2){
	json_object_object_add(jobj, "lnb", dvb_lnb_json(&sat->lnb));
    }
    jarray = json_object_new_array();
    for (int i=0; i < sat->ntrans; i++){
	transport *trans = NULL;
	if (sat->trans_freq) trans = sat->trans_freq[i];
	else trans = &sat->trans[i];
	json_object_array_add(jarray, dvb_transport_json(trans));
    }
    json_object_object_add(jobj, "transports", jarray);
    
    return jobj;
}
