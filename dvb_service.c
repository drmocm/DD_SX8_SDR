#include <sys/poll.h>
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


static void dvb_delete_descriptor(descriptor *desc)
{
    free(desc);
}

descriptor *dvb_get_descriptor(uint8_t *buf)
{
    descriptor *desc = NULL;
    if (!(desc = malloc(sizeof(descriptor)))) {
    	err("Error allocating memory in dvb_get_section\n");
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
	    err("WARNING: maximal descriptor coun reached\n");
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
	err("Error allocating memory in dvb_get_section\n");
	return NULL;	
    }
    memset(sec,0,sizeof(section));
    sec->section_length = (((buf[1]&0x0F) << 8) | buf[2]);
    memcpy(sec->data, buf, sec->section_length+3); 
    sec->table_id = buf[0];
    sec->section_syntax_indicator = (buf[1]>>4) & 0x01;
    if (sec->section_syntax_indicator){
	sec->id = (buf[3] << 8) | buf[4];
	sec->current_next_indicator = buf[5]&0x01;
	sec->version_number = (buf[5]&0x3e) >> 1;
	sec->section_number = buf[6];
	sec->last_section_number = buf[7];
    }
    return sec;
}

section **get_all_sections(dvb_devices *dev, uint16_t pid, uint8_t table_id)
{
    uint8_t sec_buf[4096];
    struct pollfd ufd;
    int fdmx;
    int nsec = 0;
    int re = 0;
    section *sec1;
    section **sec;
    uint16_t len = 0;
    
    close(dev->fd_dmx);
    if ((fdmx = dvb_open_dmx_section_filter(dev,pid , table_id,
					    0,0x000000FF,0)) < 0){
	err("Error opening section filter\n");
	exit(1);
    }
    ufd.fd=fdmx;
    ufd.events=POLLPRI;
    if (poll(&ufd,1,2000) <= 0 ) {
	err("TIMEOUT on read from demux\n");
	close(fdmx);
	return NULL;
    }
    if (!(re = read(fdmx, sec_buf, 3))){
	err("Failed to read from demux\n");
	return NULL;
    }	
    len = ((sec_buf[1] & 0x0f) << 8) | (sec_buf[2] & 0xff);
    if (!(re = read(fdmx, sec_buf+3, len))){
	err("Failed to read from demux\n");
	return NULL;
    }

    sec1 = dvb_get_section(sec_buf);
    if (!sec1->section_syntax_indicator) nsec = 0;
    else nsec = sec1->last_section_number;
    
    if (! (sec = (section **)malloc((nsec+1)*sizeof(section *)))){
	err("Error not enough memory in get_all_sections");
	return NULL;
    }
    memset(sec,0,(nsec+1)*sizeof(section *));
    sec[0] = sec1;
    if (nsec){
	for (int i=1; i < nsec+1; i++){
	    if ((fdmx = dvb_open_dmx_section_filter(dev,pid , table_id,
						    (uint32_t)i,
						    0x000000ff,0)) < 0)
		exit(1); 
	    while ( (re = read(fdmx, sec_buf, 4096)) <= 0) sleep(1);
	    sec[i] = dvb_get_section(sec_buf);
	}
    }
    return sec;
}

void dvb_delete_pat(PAT *pat)
{
    dvb_delete_section(pat->pat);
    free(pat);
}

PAT *dvb_get_pat(uint8_t *buf, section *sec)
{
    PAT *pat = NULL;

    if (buf && !sec){
	sec = dvb_get_section(buf);
	buf = sec->data;
    } else if (sec && buf) {
	err(
		"ERROR dvb_get_pat, one function arguments must be NULL\n");
	return NULL;
    }
    buf = sec->data;

    if (!(pat = malloc(sizeof(PAT)))){
	err("Error allocating memory in dvb_get_pat\n");
	return NULL;
    }
    pat->pat = sec;
    pat->nprog = (sec->section_length -8)/4;
    buf += 8;
    
    for(int n=0; n < pat->nprog; n++){
	pat->program_number[n] = (buf[n*4] << 8) | buf[n*4+1];
	pat->pid[n] = ((buf[n*4+2]&0x1f) << 8) | buf[n*4+3];
    }
    return pat;
}

static void dvb_delete_pmt_stream(pmt_stream *stream)
{
    dvb_delete_descriptor_loop(stream->descriptors, stream->desc_num);
    free(stream);
}

static pmt_stream *dvb_get_pmt_stream(uint8_t *buf)
{
    pmt_stream *stream = NULL;
    if (!(stream = malloc(sizeof(pmt_stream)))) {
    	err("Error allocating memory in dvb_get_pmt_stream\n");
	return NULL;	
    }
    stream->stream_type = buf[0];
    stream->elementary_PID = ((buf[1]&0x1f) << 8) | buf[2];
    stream->ES_info_length = ((buf[3]&0x0f) << 8) | buf[4];

    stream->desc_num = 0;
    buf += 5;
    stream->desc_num = dvb_get_descriptor_loop(buf, stream->descriptors,
					       stream->ES_info_length);

    return stream;
}

void dvb_delete_pmt(PMT *pmt)
{
    for (int n=0; n < pmt->stream_num; n++){
	dvb_delete_pmt_stream(pmt->stream[n]);
    }
	
    dvb_delete_section(pmt->pmt);
    free(pmt);
}

PMT *dvb_get_pmt(uint8_t *buf, section *sec)
{
    PMT *pmt = NULL;
    int c = 0;

    if (buf && !sec){
	sec = dvb_get_section(buf);
	buf = sec->data;
    } else if (sec && buf) {
	err(
		"ERROR dvb_get_pmt, one function arguments must be NULL\n");
	return NULL;
    }
    buf = sec->data;
    if (!(pmt = malloc(sizeof(PMT)))){
	err("Error allocating memory in dvb_get_sdt\n");
	return NULL;
    }
    pmt->pmt = sec;
    pmt->PCR_PID = ((buf[8]&0x1f) << 8) | buf[9];
    pmt->program_info_length = ((buf[10]&0x0f) << 8) | buf[11];
    c = 12;
    pmt->desc_num = dvb_get_descriptor_loop(buf+c, pmt->descriptors,
					    pmt->program_info_length);
    c += pmt->program_info_length;

    int n = 0;
    int length = pmt->pmt->section_length -7;
    while (c < length){
	pmt->stream[n] =  dvb_get_pmt_stream(buf+c);
	c += pmt->stream[n]->ES_info_length+5;
	n++;
    }
    pmt->stream_num = n;
    return pmt;
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
    	err("Error allocating memory in dvb_get_sdt_service\n");
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

SDT *dvb_get_sdt(uint8_t *buf, section *sec)
{
    SDT *sdt = NULL;

    if (buf && !sec){
	sec = dvb_get_section(buf);
	buf = sec->data;
    } else if (sec && buf) {
	err(
		"ERROR dvb_get_sdt, one function arguments must be NULL\n");
	return NULL;
    }

    buf = sec->data;
    if (!(sdt = malloc(sizeof(SDT)))){
	err("Error allocating memory in dvb_get_sdt\n");
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
    	err("Error allocating memory in dvb_get_nit_transport\n");
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


NIT  *dvb_get_nit(uint8_t *buf, section *sec)
{
    NIT *nit = NULL;

    if (buf && !sec){
	sec = dvb_get_section(buf);
	buf = sec->data;
    } else if (sec && buf) {
	err(
		"ERROR dvb_get_sdt, one function arguments must be NULL\n");
	return NULL;
    }

    buf = sec->data;
    if (!(nit = malloc(sizeof(NIT)))){
	err("Error allocating memory in dvb_get_nit\n");
	return NULL;
    }
    nit->nit = sec;

    if (sec->table_id != 0x40 && sec->table_id != 0x41){
	free(sec);
	err("Error in dvb_get_nit, not a NIT section\n");
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

PMT  **get_all_pmts(dvb_devices *dev, uint16_t pid)
{
    int n = 0;
    int re = 0;
    section **sec=NULL;
    PMT **pmts;
    
    if (!(sec =  get_all_sections(dev, pid, 0x02))) return NULL;
    n = sec[0]->last_section_number+1;
    if (n){
	if (!(pmts = (PMT **)malloc(n*sizeof(PMT*)))){
	    err("Could not allocate NIT");
	    return 0;
	}
	for (int i=0; i < n; i++){
	    pmts[i] = dvb_get_pmt(NULL,sec[i]);
	}
    }
    return pmts;
}

PAT  **get_all_pats(dvb_devices *dev)
{
    int n = 0;
    int re = 0;
    section **sec=NULL;
    PAT **pats;
    
    if (!(sec =  get_all_sections(dev, 0x00, 0x00))) return NULL;
    n = sec[0]->last_section_number+1;
    if (n){
	if (!(pats = (PAT **)malloc(n*sizeof(PAT*)))){
	    err("Could not allocate NIT");
	    return 0;
	}
	for (int i=0; i < n; i++){
	    pats[i] = dvb_get_pat(NULL,sec[i]);
	}
    }
    return pats;
}

NIT  **get_all_nits(dvb_devices *dev, uint8_t table_id)
{
    int nnit = 0;
    int re = 0;
    section **sec=NULL;
    NIT **nits;
    
    if (!(sec =  get_all_sections(dev, 0x10, table_id))) return NULL;
    nnit = sec[0]->last_section_number+1;
    if (nnit){
	if (!(nits = (NIT **)malloc(nnit*sizeof(NIT*)))){
	    err("Could not allocate NIT");
	    return 0;
	}
	for (int i=0; i < nnit; i++){
	    nits[i] = dvb_get_nit(NULL,sec[i]);
	}
    }
    return nits;
}

SDT  **get_all_sdts(dvb_devices *dev)
{
    int nsdt = 0;
    int re = 0;
    section **sec=NULL;
    SDT **sdts;
    
    if (!(sec =  get_all_sections(dev, 0x11, 0x42))) return NULL;
    nsdt = sec[0]->last_section_number+1;
    if (nsdt){
	if (!(sdts = (SDT **)malloc(nsdt*sizeof(NIT*)))){
	    err("Could not allocate NIT");
	    return 0;
	}
	for (int i=0; i < nsdt; i++){
	    sdts[i] = dvb_get_sdt(NULL,sec[i]);
	}
    }
    return sdts;
}

void dvb_print_section(int fd, section *sec)
{
    FILE* fp = fdopen(fd, "w");
    int c=9;
    
    fprintf(fp,"section: table_id 0x%02x  syntax %d\n",
	    sec->table_id, sec->section_syntax_indicator);
    if (sec->section_syntax_indicator){
	fprintf(fp,"          id 0x%04x version_number 0x%02x\n" 
		"          section number: 0x%02x\n"
		"          last_section_number: 0x%02x\n",
		sec->id, sec->version_number, sec->section_number,
		sec->last_section_number);
	c+=5;
    }
    fprintf(fp,"        data (%d bytes):\n", sec->section_length+3);
    dvb_print_data(fp, sec->data,sec->section_length, 8,"          ", "");
    fprintf(fp,"\n");
    fflush(fp);
}

void dvb_print_pat(int fd, PAT *pat)
{
    FILE* fp = fdopen(fd, "w");

    fprintf(fp,"PAT (0x%02x): transport_stream_id 0x%04x\n",
	    pat->pat->table_id, pat->pat->id);
    fprintf(fp,"  programs: \n");
    for(int n=0; n < pat->nprog; n++){
	fprintf(fp,"    program_number 0x%04x %s 0x%04x\n",
		pat->program_number[n],
		pat->program_number[n] ? "program_map_PID" : "network_PID",
		pat->pid[n]);
    }
    fprintf(fp,"\n");
    fflush(fp);
}

const char *stream_type(uint8_t type)
{
    const char *t = "unknown";

    switch (type) {
    case 0x01:
	t = "video MPEG1";
	break;
    case 0x02:
	t = "video MPEG2";
	break;
    case 0x03:
	t = "audio MPEG1";
	break;
    case 0x04:
	t = "audio MPEG2";
	break;
    case 0x05:
	t = "MPEG-2 private data";
	break;
    case 0x06:
	t = "MPEG-2 packetized data (subtitles)";
	break;
    case 0x07:
	t = "MHEG";
	break;
    case 0x08:
	t = "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC";
	break;
    case 0x09:
	t = "ITU-T Rec. H.222.1";
	break;
    case 0x0A:
	t = "DSM-CC ISO/IEC 13818-6 type A (Multi-protocol Encapsulation)";
	break;
    case 0x0B:
	t = "DSM-CC ISO/IEC 13818-6 type B (U-N messages)";
	break;
    case 0x0C:
	t = "DSM-CC ISO/IEC 13818-6 type C (Stream Descriptors)";
	break;
    case 0x0D:
	t = "DSM-CC ISO/IEC 13818-6 type D (Sections – any type)";
	break;
    case 0x0E:
	t = "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 auxiliary";
	break;
    case 0x0F:
	t = "audio AAC";
	break;
    case 0x10:
	t = "video MPEG2";
	break;
    case 0x11:
	t = "audio LATM";
	break;
    case 0x12:
	t = "ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets";
	break;
    case 0:
	t = "ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC14496_sections.";
	break;
    case 0x14:
	t = "ISO/IEC 13818-6 Synchronized Download Protocol";
	break;
    case 0x15:
	t = "Metadata in PES packets";
	break;
    case 0x16:
	t = "Metadata in metadata_sections";
	break;
    case 0x17:
	t = "Metadata 13818-6 Data Carousel";
	break;
    case 0x18:
	t = "Metadata 13818-6 Object Carousel";
	break;
    case 0x19:
	t = "Metadata 13818-6 Synchronized Download Protocol";
	break;
    case 0x1A:
	t = "IPMP (13818-11, MPEG-2 IPMP)";
	break;
    case 0x1B:
	t = "video H264 ISO/IEC 14496-10";
	break;
    case 0x1C:
	t = "audio ISO/IEC 14496-3 (DST, ALS and SLS)";
	break;
    case 0x1D:
	t = "text ISO/IEC 14496-17";
	break;
    case 0x1E:
	t = "video ISO/IEC 23002-3 Aux.";
	break;
    case 0x1F:
	t = "video ISO/IEC 14496-10 sub";
	break;
    case 0x20:
	t = "video MVC sub-bitstream";
	break;
    case 0x21:
	t = "video J2K";
	break;
    case 0x22:
	t = "video H.262 for 3D services";
	break;
    case 0x23:
	t = "video H.264 for 3D services";
	break;
    case 0x24:
	t = "video H.265 or HEVC temporal sub-bitstream";
	break;
    case 0x25:
	t = "video H.265 temporal subset";
	break;
    case 0x26:
	t = "video MVCD in AVC";
	break;
    case 0x42:
	t = "video CAVS";
	break;
    case 0x7F:
	t = "IPMP";
	break;
    case 0x81:
	t = "audio AC-3 (ATSC)";
	break;
    case 0x82:
	t = "audio DTS";
	break;
    case 0x83:
	t = "audio TRUEHD";
	break;
    case 0x86:
	t = "SCTE-35";
	break;
    case 0x87:
	t = "audio E-AC-3 (ATSC)";
	break;
    case 0xEA:
	t = "video VC1";
	break;
    case 0xD1:
	t = "video DIRAC";
	break;
    }
    return t;
}


void dvb_print_stream(FILE *fp, pmt_stream *stream)
{
    fprintf(fp,"  stream: elementary_PID 0x%04x stream_type %s \n",
	    stream->elementary_PID, stream_type(stream->stream_type));
    if (stream->desc_num){
	uint32_t priv_id = 0;
	fprintf(fp,"  descriptors:\n");
	for (int n=0 ; n < stream->desc_num; n++){
	    priv_id = dvb_print_descriptor(fp, stream->descriptors[n],
					   "    ", priv_id);
	}
    }
}

void dvb_print_pmt(int fd, PMT *pmt)
{
    FILE* fp = fdopen(fd, "w");
    fprintf(fp,"PMT (0x%02x):  program_number 0x%04x  PCR_PID 0x%04x \n",
	    pmt->pmt->table_id, pmt->pmt->id, pmt->PCR_PID);
    if (pmt->desc_num) {
	fprintf(fp,"  program_info:\n");
	for (int n=0; n < pmt->desc_num; n++){
	    uint32_t priv_id = 0;
	    priv_id = dvb_print_descriptor(fp, pmt->descriptors[n],
					   "    ", priv_id);
	}
    }
    if (pmt->stream_num) {
	fprintf(fp,"    descriptors:\n");
	for (int n=0; n < pmt->stream_num; n++){
	    dvb_print_stream(fp, pmt->stream[n]);
	}
    }
    fprintf(fp,"\n");
    fflush(fp);
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

	freq =  getbcd(buf, 8)/10;
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

descriptor  *dvb_find_descriptor(descriptor **descs, int ndesc, uint8_t tag)
{
    for (int n=0; n < ndesc; n++){
	if (descs[n]->tag == tag) return descs[n];
    }
    return NULL;
}

int set_frontend_with_transport(dvb_fe *fe, nit_transport *trans)
{
    descriptor  *desc;
    uint8_t  dtags[] ={0x43,0x44,0x5a,0xfa};
    uint8_t  *bf;

    for (int i= 0; i < 4 ; i++){
	if ((desc=dvb_find_descriptor(trans->descriptors,
				      trans->desc_num, dtags[i])))
	    break;
    }
    if (!desc) return -1;
    bf = desc->data;
    switch (desc->tag){

    case 0x43: // satellite
	fe->freq = getbcd(bf, 8) *10;
	fe->sr = getbcd(bf + 7, 7)*100;
	fe->pol =  (((bf[6] & 0x60) >> 5) % 2) ? 0:1; 
	fe->delsys = ((bf[6] & 0x04) >> 2) ? SYS_DVBS2 : SYS_DVBS;
	break;

    case 0x44: // cable
	fe->freq = getbcd(bf, 8)/10;
	fe->delsys = SYS_DVBC_ANNEX_A;
	fe->sr = getbcd(bf + 7, 7) *100;
	break;

    case 0x5a: // terrestrial
	fe->freq = (bf[5]|(bf[4] << 8)|(bf[3] << 16)|(bf[0] << 24))*10;
	fe->delsys = SYS_DVBT;
	break;

    case 0xfa: // isdbt
	fe->freq = (bf[5]|(bf[4] << 8))*7000000;
	fe->delsys = SYS_ISDBT;
    }
    return 0;
}
				 
static descriptor *find_descriptor(descriptor **desc, int length, uint8_t tag)
{
    descriptor *d = NULL;;

    for (int i=0;i< length; i++){
	if (desc[i]->tag == tag){
	    d = desc[i];
	    return d;
	}
    }

    return NULL;
}

static nit_transport *find_nit_transport(NIT **nits, uint16_t tsid)
{
    int n = nits[0]->nit->last_section_number+1;
    nit_transport *trans = NULL;
    err("searching transport tsid 0x%04x\n",tsid);
    for(int i = 0; i < n; i++){
	for (int j = 0; j < nits[i]->trans_num; j++){
	    trans = nits[i]->transports[j];
	    if (trans->transport_stream_id == tsid){
		err("Found transport with complete NIT/BAT\n");
		return trans;
	    }
	}
    }
    return NULL;
}

int get_all_services(transport *trans, dvb_devices *dev)
{
    int sdt_snum=0;
    int pat_snum=0;
    int snum = 0;

    if (!trans->pat) return -1;
    if (trans->sdt){
	for (int n=0; n < trans->nsdt; n++)
	    sdt_snum += trans->sdt[n]->service_num;
    }
    for (int n=0; n < trans->npat; n++)
	    pat_snum += trans->pat[n]->nprog;

    snum = ((pat_snum >= sdt_snum) ? pat_snum : sdt_snum);
    trans->nserv = snum;
    trans->serv = (service *) malloc(snum*sizeof(service));
    for (int j=0; j < snum; j++) {
	trans->serv[j].sdt_service = NULL;
	trans->serv[j].id  = 0;
	trans->serv[j].pmt  = NULL;
	trans->serv[j].sat  = trans->sat;
	trans->serv[j].trans  = trans;
    }
    int i=0;
    if (trans->sdt){
	for (int n=0; n < trans->nsdt; n++){
	    for (int j=0; j < trans->sdt[n]->service_num; j++){
		trans->serv[i].sdt_service = trans->sdt[n]->services[j];
		trans->serv[i].id  = trans->sdt[n]->services[j]->service_id;
		i++;
	    }
	}
    } 
    
    for (int n=0; n < trans->npat; n++){
	for (int i=0; i < trans->pat[n]->nprog; i++){
	    int npmt = 0;
	    uint16_t pid = trans->pat[n]->pid[i];
	    if (!trans->pat[n]->program_number[i]) continue;
	    PMT  **pmt = get_all_pmts(dev, pid);
	    if (pmt){
		npmt = pmt[0]->pmt->last_section_number+1;
		for (int k=0; k < npmt; k++){
		    int j=0;
		    while (trans->pat[n]->program_number[i] !=
			trans->serv[j].id && j < i){
			j++;
		    }
		    trans->serv[j].pmt = pmt;
		    if (j==i){ // in case there is no SDT entry
			trans->serv[j].id = trans->pat[n]->program_number[i];
			i++;
		    }
		}
	    }
	}
    }

    return snum;
}


NIT **get_full_nit(dvb_devices *dev, dvb_fe *fe, dvb_lnb *lnb)
{
    NIT **nits = NULL;
    int n = 0;
    nit_transport *trans;
    descriptor *desc = NULL;
    uint16_t tsid = 0;

    if (!(nits = get_all_nits(dev, 0x40))) return NULL;
    n = nits[0]->nit->last_section_number+1;

    for (int i=0; i<n; i++){    // look for linkage to full NIT
	for (int j=0; j < nits[i]->ndesc_num; j++){
	    desc = nits[i]->network_descriptors[j];
	    if (desc && desc->tag == 0X4a){
		if (desc->data[6] == 0x04){
		    tsid = (desc->data[0] << 8) | desc->data[1];
		    break;
		}
	    }
	}
    }
    
    if (tsid){
	uint32_t freq = fe->freq;
	if ((trans = find_nit_transport(nits,tsid))){
	    if (set_frontend_with_transport(fe, trans)) {
		err("Could not set frontend\n");
		exit(1);
	    }
	    if (freq != fe->freq){
		int lock = dvb_tune(dev, fe, lnb);
		if (lock == 1){ 
		    for (int i=0; i < n; i++){
			dvb_delete_nit(nits[i]);
		    }
		    if (!(nits = get_all_nits(dev, 0x40))) return NULL;
		    n = nits[0]->nit->last_section_number+1;
		}
	    }
	}
    }
    return nits;
}


