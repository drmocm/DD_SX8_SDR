#include <pthread.h>
#include "dvb_service.h"

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

const char *service_type(uint8_t type)
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
    case 0x13:
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
    case 0x90:
	t = "Blu-ray Presentation Graphic Stream (subtitling)";
	break;
    case 0x91:
	t = "ATSC DSM CC Network Resources table";
	break;
    case 0xC0:
	t ="DigiCipher II text";
	break;
    case 0xC1:
	t ="Dolby Digital (AC-3) up to six channel audio";
	break;
    case 0xC2:
	t ="ATSC DSM CC synchronous data";
	break;
    case 0xEA:
	t = "video VC1";
	break;
    case 0xD1:
	t = "video DIRAC";
	break;

    case 0x88 ... 0x8F:
    case 0x92 ... 0xBF:
    case 0xC3 ... 0xCE:
	t = "privately defined";
	break;
    }
    return t;
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
	    err("WARNING: maximal descriptor count reached\n");
	    dc = MAXDESC-1;
	    return dc;
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
    int fdmx;
    int nsec = 0;
    section *sec1;
    section **sec;
    uint16_t len = 0;
    
    if(!read_section_from_dmx(dev->fd_dmx, sec_buf, 4096, pid,  table_id, 0))
	return NULL;

    
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
	    if(!read_section_from_dmx(dev->fd_dmx, sec_buf, 4096, pid,					   table_id, (uint32_t)i))
		return NULL;
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

    int nserv = i;
    int j=0;
    for (int n=0; n < trans->npat; n++){
	for (int i=0; i < trans->pat[n]->nprog; i++){
	    int npmt = 0;
	    uint16_t pid = trans->pat[n]->pid[i];
	    if (!trans->pat[n]->program_number[i]) continue;
	    PMT  **pmt = get_all_pmts(dev, pid);
	    if (pmt){
		int found =0;
		for (int k = 0; k < nserv; k++){
		    if (trans->pat[n]->program_number[i] ==
			trans->serv[k].id){
			trans->serv[k].pmt = pmt;
			found = 1;
			break;
		    }
		}
		if (!found){ // in case there is no SDT entry
		    trans->serv[nserv+j].id = trans->pat[n]->program_number[i];
		    j++;
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


void dvb_sort_sat(satellite *sat)
{
    sat->l_h_trans = NULL;
    sat->l_v_trans = NULL;
    sat->u_h_trans = NULL;
    sat->u_v_trans = NULL;
    sat->n_lh_trans = 0;
    sat->n_lv_trans = 0;
    sat->n_uh_trans = 0;
    sat->n_uv_trans = 0;

    sat->trans_freq = (transport **) malloc(sat->ntrans*sizeof(transport *));
    memset(sat->trans_freq, 0, sat->ntrans*sizeof(transport *));
    for (int i=0; i < sat->ntrans; i++){
	int j = 0;
	transport *trans = &sat->trans[i];
	while (j < i && sat->trans_freq[j]){
	    if (trans->fe.freq < sat->trans_freq[j]->fe.freq){
		for (int k = i; k > j; k--) {
		    sat->trans_freq[k] = sat->trans_freq[k-1];
		}
		sat->trans_freq[j] = NULL;
	    } else {
		j++;
	    }
	}
	sat->trans_freq[j] = trans;
	if (sat->delsys == SYS_DVBS  || sat->delsys == SYS_DVBS2){
	    if (trans->fe.pol){
		if (trans->fe.hi) sat->n_uh_trans++;
		else sat->n_lh_trans++;
	    } else {
		if (trans->fe.hi) sat->n_uv_trans++;
		else sat->n_lv_trans++;
	    }
	}
    }

    if (sat->delsys != SYS_DVBS && sat->delsys != SYS_DVBS2) return;
    sat->l_h_trans = (transport **) malloc(sat->n_lh_trans*sizeof(transport*));
    sat->l_v_trans = (transport **) malloc(sat->n_lv_trans*sizeof(transport*));
    sat->u_h_trans = (transport **) malloc(sat->n_uh_trans*sizeof(transport*));
    sat->u_v_trans = (transport **) malloc(sat->n_uv_trans*sizeof(transport*));
    memset(sat->l_h_trans, 0, sat->n_lh_trans*sizeof(transport *));
    memset(sat->l_v_trans, 0, sat->n_lv_trans*sizeof(transport *));
    memset(sat->u_h_trans, 0, sat->n_uh_trans*sizeof(transport *));
    memset(sat->u_v_trans, 0, sat->n_uv_trans*sizeof(transport *));

    int uh=0;
    int uv=0;
    int lh=0;
    int lv=0;
    for (int i=0; i < sat->ntrans; i++){
	transport *trans= sat->trans_freq[i];
	if (trans->fe.pol){
	    if (trans->fe.hi){
		sat->u_h_trans[uh] = trans;
		uh++;
	    } else {
		sat->l_h_trans[lh] = trans;
		lh++;
	    }
	} else {
	    if (trans->fe.hi) { 
		sat->u_v_trans[uv] = trans;
		uv++;
	    }else {
		sat->l_v_trans[lv] = trans;
		lv++;
	    }
	}
    }
}

void scan_transport(dvb_devices *dev, dvb_lnb *lnb, transport *trans)
{
    int lock = 0;
    lock = dvb_tune(dev, &trans->fe, lnb);
    trans->lock = lock;
    if (lock == 1){ 
//	err("  getting SDT\n");
	if (!(trans->sdt = get_all_sdts(dev))){
	    err("SDT not found\n");
	    trans->nsdt = 0;
	} else {
	    trans->nsdt = trans->sdt[0]->sdt->last_section_number+1;
	}
//	err("  getting PAT\n");
	if (!(trans->pat = get_all_pats(dev))){
	    err("PAT not found\n");
	    trans->npat = 0;
	} else {
	    trans->npat = trans->pat[0]->pat->last_section_number+1;
	}
//	err("  getting PMTs\n");
	trans->nserv = get_all_services(trans, dev);
    }
}

struct scan_thread_t
{
    dvb_devices *dev;
    dvb_lnb *lnb;
    transport *trans;
};

static void *start_scan_thread(void *ptr)
{
    struct scan_thread_t *scant = (struct scan_thread_t *) ptr;
    dvb_devices *dev = scant->dev;
    dvb_lnb *lnb = scant->lnb;
    transport *trans = scant->trans;
    
    scan_transport(dev, lnb, trans);

    if (dev->fd_fe  >=0) close(dev->fd_fe);
    if (dev->fd_dvr >=0) close(dev->fd_dvr);
    if (dev->fd_dmx >=0) close(dev->fd_dmx);
    if (dev) free(dev);
    if (lnb) free(lnb);
    if (scant) free(scant);

    pthread_exit(NULL);  
}

int thread_scan_transport(int adapter, dvb_lnb *lnb, transport *trans,
			  int m, pthread_mutex_t *lock)
{
    struct scan_thread_t *scant = (struct scan_thread_t *)
	malloc(sizeof(struct scan_thread_t));
    dvb_devices *dev = (dvb_devices *) malloc(sizeof(dvb_devices));
    dvb_lnb *lnb2 = (dvb_lnb *) malloc(sizeof(dvb_lnb));

    if (!lnb || !trans){
	err("NULL pointer in thread_scan_transport lnb=0x%x trans=0x%x\n",
	    lnb,trans,lock);
	exit(1);
    }
    if (!dev || !lnb2 || !scant) {
	err("Not enough memory in thread_scan_transport\n");
	exit(1);
    }
    memcpy(lnb2,lnb,sizeof(dvb_lnb));
    lnb2->scif_slot = m;
    lnb2->delay = 0;
    dvb_init_dev(dev);
    dev->num = m;
    dev->adapter = adapter;
    dev->lock = lock;
    if ( (dev->fd_fe = open_fe(dev->adapter, dev->num)) < 0){
	return -1;
    }
    if ( (dev->fd_dmx = open_dmx(dev->adapter, dev->num)) < 0){
	return -1;
    }
    scant->lnb = lnb2;
    scant->dev = dev;
    scant->trans = trans;
    if(pthread_create(&trans->tMux, NULL, start_scan_thread, scant))
    {
       return -1;
    }
    return 0;
}


