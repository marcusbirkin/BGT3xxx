#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include "os.h"
#include "vt.h"
#include "misc.h"
#include "vbi.h"
#include "fdset.h"
#include "hamm.h"
#include "lang.h"
#include <libzvbi.h>


static vbi_capture      * pZvbiCapt;
static vbi_raw_decoder  * pZvbiRawDec;
static vbi_sliced       * pZvbiData;
static vbi_proxy_client * pProxy;

#define ZVBI_BUFFER_COUNT  10
#define ZVBI_TRACE          0


static int vbi_dvb_open(struct vbi *vbi, const char *vbi_name,
	const char *channel, char *outfile, u_int16_t sid, int ttpid);
static void dvb_handler(struct vbi *vbi, int fd);

#define FAC (1<<16) // factor for fix-point arithmetic

static u8 *rawbuf; // one common buffer for raw vbi data
static int rawbuf_size; // its current size
u_int16_t sid;
static char *vbi_names[]
	= { "/dev/vbi", "/dev/vbi0", "/dev/video0", "/dev/dvb/adapter0/demux0",
	NULL }; // default device names if none was given at the command line


static void out_of_sync(struct vbi *vbi)
{
    int i; // discard all in progress pages
    for (i = 0; i < 8; ++i)
    vbi->rpage[i].page->flags &= ~PG_ACTIVE;
}


// send an event to all clients
static void vbi_send(struct vbi *vbi, int type, int i1, int i2, int i3, void *p1)
{
    struct vt_event ev[1];
    struct vbi_client *cl, *cln;
    ev->resource = vbi;
    ev->type = type;
    ev->i1 = i1;
    ev->i2 = i2;
    ev->i3 = i3;
    ev->p1 = p1;
    for (cl = PTR vbi->clients->first; cln = PTR cl->node->next; cl = cln)
    cl->handler(cl->data, ev);
}


static void vbi_send_page(struct vbi *vbi, struct raw_page *rvtp, int page)
{
    struct vt_page *cvtp = 0;

    if (rvtp->page->flags & PG_ACTIVE)
    {
    if (rvtp->page->pgno % 256 != page)
    {
    rvtp->page->flags &= ~PG_ACTIVE;
    enhance(rvtp->enh, rvtp->page);
    if (vbi->cache)
    cvtp = vbi->cache->op->put(vbi->cache, rvtp->page);
    vbi_send(vbi, EV_PAGE, 0, 0, 0, cvtp ?: rvtp->page);
    }
    }
}


static void pll_add(struct vbi *vbi, int n, int err)
{
}


// process one videotext packet
static int vt_line(struct vbi *vbi, u8 *p)
{
    struct vt_page *cvtp;
    struct raw_page *rvtp;
    int hdr, mag, mag8, pkt, i;
    int err = 0;

    hdr = hamm16(p, &err);
    if (err & 0xf000)
    return -4;
    mag = hdr & 7;
    mag8 = mag?: 8;
    pkt = (hdr >> 3) & 0x1f;
    p += 2;
    rvtp = vbi->rpage + mag;
    cvtp = rvtp->page;
    switch (pkt)
    {
	case 0:
	{
	    int b1, b2, b3, b4;
	    b1 = hamm16(p, &err); // page number
	    b2 = hamm16(p+2, &err); // subpage number + flags
	    b3 = hamm16(p+4, &err); // subpage number + flags
	    b4 = hamm16(p+6, &err); // language code + more flags
	    if (vbi->ppage->page->flags & PG_MAGSERIAL)
		vbi_send_page(vbi, vbi->ppage, b1);
	    vbi_send_page(vbi, rvtp, b1);

	    if (err & 0xf000)
		return 4;

	    cvtp->errors = (err >> 8) + chk_parity(p + 8, 32);;
	    cvtp->pgno = mag8 * 256 + b1;
	    cvtp->subno = (b2 + b3 * 256) & 0x3f7f;
	    cvtp->lang = "\0\4\2\6\1\5\3\7"[b4 >> 5] + (latin1==LATIN1 ? 0 : 8);
	    cvtp->flags = b4 & 0x1f;
	    cvtp->flags |= b3 & 0xc0;
	    cvtp->flags |= (b2 & 0x80) >> 2;
	    cvtp->lines = 1;
	    cvtp->flof = 0;
	    vbi->ppage = rvtp;
	    pll_add(vbi, 1, cvtp->errors);
	    conv2latin(p + 8, 32, cvtp->lang);
	    vbi_send(vbi, EV_HEADER, cvtp->pgno, cvtp->subno, cvtp->flags, p);

	    if (b1 == 0xff)
	    return 0;
	    cvtp->flags |= PG_ACTIVE;
	    init_enhance(rvtp->enh);
	    memcpy(cvtp->data[0]+0, p, 40);
	    memset(cvtp->data[0]+40, ' ', sizeof(cvtp->data)-40);
	    return 0;
	}

	case 1 ... 24:
	{
	    pll_add(vbi, 1, err = chk_parity(p, 40));

	    if (~cvtp->flags & PG_ACTIVE)
		return 0;

	    cvtp->errors += err;
	    cvtp->lines |= 1 << pkt;
	    conv2latin(p, 40, cvtp->lang);
	    memcpy(cvtp->data[pkt], p, 40);
	    return 0;
	}
	case 26:
	{
	    int d, t[13];

	    if (~cvtp->flags & PG_ACTIVE)
		return 0;

	    d = hamm8(p, &err);
	    if (err & 0xf000)
		return 4;

	    for (i = 0; i < 13; ++i)
		t[i] = hamm24(p + 1 + 3*i, &err);
	    if (err & 0xf000)
		return 4;

	    add_enhance(rvtp->enh, d, t);
	    return 0;
	}
	case 27:
	{
	    int b1,b2,b3,x;
	    if (~cvtp->flags & PG_ACTIVE)
		return 0; // -1 flushes all pages. We may never resync again

	    b1 = hamm8(p, &err);
	    b2 = hamm8(p + 37, &err);
	    if (err & 0xf000)
		return 4;
	    if (b1 != 0 || not(b2 & 8))
		return 0;

	    for (i = 0; i < 6; ++i)
	    {
		err = 0;
		b1 = hamm16(p+1+6*i, &err);
		b2 = hamm16(p+3+6*i, &err);
		b3 = hamm16(p+5+6*i, &err);
		if (err & 0xf000)
		    return 1;
		x = (b2 >> 7) | ((b3 >> 5) & 0x06);
		cvtp->link[i].pgno = ((mag ^ x) ?: 8) * 256 + b1;
		cvtp->link[i].subno = (b2 + b3 * 256) & 0x3f7f;
	    }
	    cvtp->flof = 1;
	    return 0;
	}
	case 30:
	{
	    if (mag8 != 8)
	    return 0;
	    p[0] = hamm8(p, &err); // designation code
	    p[1] = hamm16(p+1, &err); // initial page
	    p[3] = hamm16(p+3, &err); // initial subpage + mag
	    p[5] = hamm16(p+5, &err); // initial subpage + mag
	    if (err & 0xf000)
	    return 4;
	    err += chk_parity(p+20, 20);
	    conv2latin(p+20, 20, 0);
	    vbi_send(vbi, EV_XPACKET, mag8, pkt, err, p);
	    return 0;
	}
	default:
	    return 0;
    }
    return 0;
}


// called when new vbi data is waiting
static void vbi_handler(struct vbi *vbi, int fd)
{
    double timestamp;
    struct timeval timeout;
    int lineCount;
    int line;
    int res;

    timeout.tv_sec  = 0;
    timeout.tv_usec = 25000;
    res = vbi_capture_read_sliced(pZvbiCapt, pZvbiData, &lineCount, &timestamp,
    &timeout);
    if (res > 0)
    {
	for (line=0; line < lineCount; line++)
	{
            if ((pZvbiData[line].id & VBI_SLICED_TELETEXT_B) != 0)
	    {
		vt_line(vbi, pZvbiData[line].data);
	    }
	}
    }
    else if (res < 0)
    {
    }
}


int vbi_add_handler(struct vbi *vbi, void *handler, void *data)
{
    struct vbi_client *cl;

    if (not(cl = malloc(sizeof(*cl))))
    return -1;
    cl->handler = handler;
    cl->data = data;
    dl_insert_last(vbi->clients, cl->node);
    return 0;
}


void vbi_del_handler(struct vbi *vbi, void *handler, void *data)
{
    struct vbi_client *cl;

    for (cl = PTR vbi->clients->first; cl->node->next; cl = PTR cl->node->next)
	if (cl->handler == handler && cl->data == data)
	{
	    dl_remove(cl->node);
	    break;
	}
    return;
}


struct vbi * vbi_open(char *vbi_name, struct cache *ca,
	const char *channel, char *outfile, u_int16_t sid, int ttpid)
{
    static int inited = 0;
    struct vbi *vbi;
    char * pErrStr;
    int services;

    if (vbi_name == NULL)
    {
        int i;
        char *tried_devices = NULL;
        char *old_tried_devices = NULL;
        for (i = 0; vbi_names[i] != NULL; i++)
        {
        vbi_name = vbi_names[i];
        // collect device names for the error message below
        if (old_tried_devices)
        {
        if (asprintf(&tried_devices, "%s, %s", old_tried_devices, vbi_name) < 0)
        tried_devices = NULL;
        free(old_tried_devices);
        }
        else if (asprintf(&tried_devices, "%s", vbi_name) < 0)
        tried_devices = NULL;
        if (tried_devices == NULL)
        out_of_mem(-1);
        old_tried_devices = tried_devices;
        if (access(vbi_name, R_OK) != 0)
        continue;
        vbi = vbi_open(vbi_name, ca, channel, outfile, sid, ttpid);
        if (vbi != NULL)
        {
        if (tried_devices != NULL)
        free(tried_devices);
        return vbi;
        }
        }

	error("could not open any of the standard devices (%s)", tried_devices);
        free(tried_devices);
	return NULL;
    }

    if (not inited)
    lang_init();
    inited = 1;

    if (not(vbi = malloc(sizeof(*vbi))))
    {
	error("out of memory");
	goto fail1;
    }
    if (!vbi_dvb_open(vbi, vbi_name, channel, outfile, sid, ttpid)) {
	    vbi->cache = ca;
	    dl_init(vbi->clients);
	    out_of_sync(vbi);
	    vbi->ppage = vbi->rpage;
	    fdset_add_fd(fds, vbi->fd, dvb_handler, vbi);
	    return vbi;
    }

    services = VBI_SLICED_TELETEXT_B;
    pErrStr = NULL;
    vbi->fd = -1;

    pProxy = vbi_proxy_client_create(vbi_name, "alevt",
    VBI_PROXY_CLIENT_NO_STATUS_IND, &pErrStr, ZVBI_TRACE);
    if (pProxy != NULL)
    {
       pZvbiCapt = vbi_capture_proxy_new(pProxy, ZVBI_BUFFER_COUNT, 0,
       &services, 0, &pErrStr);
       if (pZvbiCapt == NULL)
       {
          vbi_proxy_client_destroy(pProxy);
          pProxy = NULL;
       }
    }
    if (pZvbiCapt == NULL)
        pZvbiCapt = vbi_capture_v4l2_new(vbi_name, ZVBI_BUFFER_COUNT,
        &services, 0, &pErrStr, ZVBI_TRACE);
    if (pZvbiCapt == NULL)
        pZvbiCapt = vbi_capture_v4l_new(vbi_name, 0, &services, 0, &pErrStr,
        ZVBI_TRACE);

    if (pZvbiCapt != NULL)
    {
        pZvbiRawDec = vbi_capture_parameters(pZvbiCapt);
        if ((pZvbiRawDec != NULL) && ((services & VBI_SLICED_TELETEXT_B) != 0))
        {
            pZvbiData = malloc((pZvbiRawDec->count[0] + pZvbiRawDec->count[1]) \
            * sizeof(*pZvbiData));

            vbi->fd = vbi_capture_fd(pZvbiCapt);
        }
        else
            vbi_capture_delete(pZvbiCapt);
    }

    if (pErrStr != NULL)
    {
        fprintf(stderr, "libzvbi: %s\n", pErrStr);
        free(pErrStr);
    }

    if (vbi->fd == -1)
        goto fail2;
    vbi->cache = ca;
    dl_init(vbi->clients);
    out_of_sync(vbi);
    vbi->ppage = vbi->rpage;
    fdset_add_fd(fds, vbi->fd, vbi_handler, vbi);
    return vbi;

fail3:
    close(vbi->fd);
fail2:
    free(vbi);
fail1:
    return 0;
}


void vbi_close(struct vbi *vbi)
{
    fdset_del_fd(fds, vbi->fd);
    if (vbi->cache)
    vbi->cache->op->close(vbi->cache);

    if (pZvbiData != NULL)
	free(pZvbiData);
    pZvbiData = NULL;

    if (pZvbiCapt != NULL)
    {
	vbi_capture_delete(pZvbiCapt);
       pZvbiCapt = NULL;
    }
    if (pProxy != NULL)
    {
       vbi_proxy_client_destroy(pProxy);
       pProxy = NULL;
    }
    free(vbi);
}


struct vt_page * vbi_query_page(struct vbi *vbi, int pgno, int subno)
{
    struct vt_page *vtp = 0;
    if (vbi->cache)
    vtp = vbi->cache->op->get(vbi->cache, pgno, subno);
    if (vtp == 0)
    {
    return 0;
    }
    vbi_send(vbi, EV_PAGE, 1, 0, 0, vtp);
    return vtp;
}


void vbi_reset(struct vbi *vbi)
{
    if (vbi->cache)
    vbi->cache->op->reset(vbi->cache);
    vbi_send(vbi, EV_RESET, 0, 0, 0, 0);
}


/* Starting from here: DVB API */
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/video.h>

static int dvb_get_table(int fd, u_int16_t pid, u_int8_t tblid, u_int8_t *buf,
	size_t bufsz)
{
    struct dmx_sct_filter_params sctFilterParams;
    struct pollfd pfd;
    int r;
    memset(&sctFilterParams, 0, sizeof(sctFilterParams));
    sctFilterParams.pid = pid;
    sctFilterParams.timeout = 10000;
    sctFilterParams.flags = DMX_ONESHOT | DMX_IMMEDIATE_START | DMX_CHECK_CRC;
    sctFilterParams.filter.filter[0] = tblid;
    sctFilterParams.filter.mask[0] = 0xff;
    if (ioctl(fd, DMX_SET_FILTER, &sctFilterParams)) {
		perror("DMX_SET_FILTER");
		return -1;
	}
	pfd.fd = fd;
	pfd.events = POLLIN;
	r = poll(&pfd, 1, 10000);
	if (r < 0) {
		perror("poll");
		goto out;
	}
	if (r > 0) {
		r = read(fd, buf, bufsz);
		if (r < 0) {
			perror("read");
			goto out;
		}
	}
 out:
	ioctl(fd, DMX_STOP, 0);
	return r;
}

static const u_int8_t byterev8[256] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static void dvb_handle_pes_payload(struct vbi *vbi, const u_int8_t *buf,
	unsigned int len)
{
	unsigned int p, i;
	u_int8_t data[42];

	if (buf[0] < 0x10 || buf[0] > 0x1f)
		return;  /* no EBU teletext data */
	for (p = 1; p < len; p += /*6 + 40*/ 2 + buf[p + 1]) {
#if 0
	printf("Txt Line:\n"
	       "  data_unit_id		   0x%02x\n"
	       "  data_unit_length	   0x%02x\n"
	       "  reserved_for_future_use  0x%01x\n"
	       "  field_parity		   0x%01x\n"
	       "  line_offset		   0x%02x\n"
	       "  framing_code		   0x%02x\n"
	       "  magazine_and_packet_addr 0x%04x\n"
	       "  data_block		   0x%02x 0x%02x 0x%02x 0x%02x\n",
	       buf[p], buf[p+1],
	       buf[p+2] >> 6,
	       (buf[p+2] >> 5) & 1,
	       buf[p+2] & 0x1f,
	       buf[p+3],
	       (buf[p+4] << 8) | buf[p+5],
	       buf[p+6], buf[p+7], buf[p+8], buf[p+9]);
#endif
		for (i = 0; i < sizeof(data); i++)
			data[i] = byterev8[buf[p+4+i]];
		/* note: we should probably check for missing lines and then
		 * call out_of_sync(vbi); and/or vbi_reset(vbi); */
		vt_line(vbi, data);
	}
}

static unsigned int rawptr;

static void dvb_handler(struct vbi *vbi, int fd)
{
	/* PES packet start code prefix and stream_id == private_stream_1 */
	static const u_int8_t peshdr[4] = { 0x00, 0x00, 0x01, 0xbd };
	u_int8_t *bp;
	int n;
	unsigned int p, i, len;
        u_int16_t rpid;
        u_int32_t crc, crccomp;

	if (rawptr >= (unsigned int)rawbuf_size)
		rawptr = 0;
	n = read(vbi->fd, rawbuf + rawptr, rawbuf_size - rawptr);
	if (n <= 0)
		return;
	rawptr += n;
	if (rawptr < 6)
		return;
	if (memcmp(rawbuf, peshdr, sizeof(peshdr))) {
		bp = memmem(rawbuf, rawptr, peshdr, sizeof(peshdr));
		if (!bp)
			return;
		rawptr -= (bp - rawbuf);
		memmove(rawbuf, bp, rawptr);
		if (rawptr < 6)
			return;
	}
	len = (rawbuf[4] << 8) | rawbuf[5];
	if (len < 9) {
		rawptr = 0;
		return;
	}
	if (rawptr < len + 6)
		return;
	p = 9 + rawbuf[8];
#if 0
	for (i = 0; i < len - p; i++) {
		if (!(i & 15))
			printf("\n%04x:", i);
		printf(" %02x", rawbuf[p + i]);
	}
	printf("\n");
#endif
	if (!dl_empty(vbi->clients))
		dvb_handle_pes_payload(vbi, rawbuf + p, len - p);
	rawptr -= len;
	if (rawptr)
		memmove(rawbuf, rawbuf + len, rawptr);
}


static int vbi_dvb_open(struct vbi *vbi, const char *vbi_name,
	const char *channel, char *outfile, u_int16_t sid, int ttpid)
{
	struct {
		u_int16_t pmtpid;
		u_int16_t ttpid;
		u_int16_t service_id;
		u_int8_t service_type;
		char service_provider_name[64];
		char service_name[64];
		u_int8_t txtlang[3];
		u_int8_t txttype;
		u_int8_t txtmagazine;
		u_int8_t txtpage;
	} progtbl[16], *progp;
	u_int8_t tbl[4096];
	u_int8_t * ppname, * psname, pncode, sncode, pnlen, snlen;
	int r;
	FILE *ofd;
	unsigned int i, j, k, l, progcnt = 0;
	struct dmx_pes_filter_params filterpar;

	/* open DVB demux device */
	if (!vbi_name)
		vbi_name = "/dev/dvb/adapter0/demux0";
	if ((vbi->fd = open(vbi_name, O_RDWR)) == -1) {
		error("cannot open demux device %s", vbi_name);
		return -1;
	}
	memset(progtbl, 0, sizeof(progtbl));
	if (ttpid >= 0x15 && ttpid < 0x1fff) {
		vbi->ttpid = ttpid;
		printf("Using command line specified teletext PID 0x%x\n",
		vbi->ttpid);
		goto ttpidfound;
	}
	/* parse PAT to enumerate services and to find the PMT PIDs */
	r = dvb_get_table(vbi->fd, 0, 0, tbl, sizeof(tbl));
	if (r == -1)
		goto outerr;
	if (!(tbl[5] & 1)) {
		error("PAT not active (current_next_indicator == 0)");
		goto outerr;
	}
	if (tbl[6] != 0 || tbl[7] != 0) {
		error("PAT has multiple sections");
		goto outerr;
	}
	if (r < 13) {
		error("PAT too short\n");
		goto outerr;
	}
	r -= 13;
	for (i = 0; i < (unsigned)r; i += 4) {
		if (progcnt >= sizeof(progtbl)/sizeof(progtbl[0])) {
			error("Program table overflow");
			goto outerr;
		}
		progtbl[progcnt].service_id = (tbl[8 + i] << 8) | tbl[9 + i];
		if (!progtbl[progcnt].service_id) /* this is the NIT pointer */
			continue;
		progtbl[progcnt].pmtpid = ((tbl[10 + i] << 8) | tbl[11 + i])
		& 0x1fff;
		progcnt++;
	}
	/* find the SDT to get the station names */
	r = dvb_get_table(vbi->fd, 0x11, 0x42, tbl, sizeof(tbl));
	if (r == -1)
		goto outerr;
	if (!(tbl[5] & 1)) {
		error("SDT not active (current_next_indicator == 0)");
		goto outerr;
	}
	if (tbl[6] != 0 || tbl[7] != 0) {
		error("SDT has multiple sections");
		goto outerr;
	}
	if (r < 12) {
		error("SDT too short\n");
		goto outerr;
	}
	i = 11;
	while (i < (unsigned)r - 1) {
		k = (tbl[i] << 8) | tbl[i+1]; /* service ID */
		progp = NULL;
		for (j = 0; j < progcnt; j++)
			if (progtbl[j].service_id == k) {
				progp = &progtbl[j];
				break;
			}
		j = i + 5;
		i = j + (((tbl[i+3] << 8) | tbl[i+4]) & 0x0fff);
		if (!progp) {
			error("SDT: service_id 0x%x not in PAT\n", k);
			continue;
		}
         while (j < i) {
            switch (tbl[j]) {
               case 0x48: // service descriptor
                  k = j + 4 + tbl[j + 3];
                  progp->service_type = tbl[j+2];
                  ppname = tbl+j+4 ; // points to 1st byte of provider_name
                  pncode = *ppname ; // 1st byte of provider_name
                  pnlen = tbl[j+3]; // length of provider_name
                  psname = tbl+k+1 ; // points to 1st byte of service_name
                  sncode = *psname ; // 1st byte of service_name
                  snlen = tbl[k]  ; // length of service_name
                  if (pncode >= 0x20) {
                     pncode = 0 ; // default character set Latin alphabet fig.A.1
                  } else {
                     ppname++ ; pnlen-- ;
                     // character code from table A.3 1st byte = ctrl-code
                  }
                  if (sncode >= 0x20) {
                     sncode = 0 ; // default character set Latin alphabet fig.A.1
                  } else {
                     psname++ ; snlen-- ;
                     // character code from table A.3 ; 1st byte = ctrl-code
                  }
                  snprintf(progp->service_provider_name,
                  sizeof(progp->service_provider_name), "%.*s", pnlen, ppname);
                  snprintf(progp->service_name,
                  sizeof(progp->service_name), "%.*s", snlen, psname); break;
            }
            j += 2 + tbl[j + 1]; // next descriptor
         }
      }
	/* parse PMT's to find Teletext Services */
	for (l = 0; l < progcnt; l++) {
		progtbl[l].ttpid = 0x1fff;
		if (progtbl[l].service_type != 0x01 || /* not digital TV */
		    progtbl[l].pmtpid < 0x15 || /* PMT PID sanity check */
		    progtbl[l].pmtpid >= 0x1fff)
			continue;
		r = dvb_get_table(vbi->fd, progtbl[l].pmtpid, 0x02, tbl,
		sizeof(tbl));
		if (r == -1)
			goto outerr;
		if (!(tbl[5] & 1)) { error \
		("PMT pid 0x%x not active (current_next_indicator == 0)",
		progtbl[l].pmtpid);
			goto outerr;
		}
		if (tbl[6] != 0 || tbl[7] != 0) {
			error("PMT pid 0x%x has multiple sections",
			progtbl[l].pmtpid);
			goto outerr;
		}
		if (r < 13) {
			error("PMT pid 0x%x too short\n", progtbl[l].pmtpid);
			goto outerr;
		}
		i = 12 + (((tbl[10] << 8) | tbl[11]) & 0x0fff);
		/* skip program info section */
		while (i <= (unsigned)r-6) {
			j = i + 5;
			i = j + (((tbl[i + 3] << 8) | tbl[i + 4]) & 0x0fff);
			if (tbl[j - 5] != 0x06)
			/* teletext streams have type 0x06 */
				continue;
			k = ((tbl[j - 4] << 8) | tbl[j - 3]) & 0x1fff;
		/* elementary PID - save until we know if it's teletext PID */
			while (j < i) {
				switch (tbl[j]) {
				case 0x56: /* EBU teletext descriptor */
					progtbl[l].txtlang[0] = tbl[j + 2];
					progtbl[l].txtlang[1] = tbl[j + 3];
					progtbl[l].txtlang[2] = tbl[j + 4];
					progtbl[l].txttype = tbl[j + 5] >> 3;
					progtbl[l].txtmagazine = tbl[j + 5] & 7;
					progtbl[l].txtpage = tbl[j + 6];
					progtbl[l].ttpid = k;
					break;
				}
				j += 2 + tbl[j + 1];
			}
		}
	}

    printf \
    ("sid:pmtpid:ttpid:type:provider:name:language:texttype:magazine:page\n\n");
    for (i = 0; i < progcnt; i++) {
    printf("%d:%d:%d:%d:%s:%s:lang=%.3s:type=%d:magazine=%1u:page=%3u\n",
    progtbl[i].service_id, progtbl[i].pmtpid, progtbl[i].ttpid,
    progtbl[i].service_type, progtbl[i].service_provider_name,
    progtbl[i].service_name, progtbl[i].txtlang, progtbl[i].txttype,
    progtbl[i].txtmagazine, progtbl[i].txtpage);
    }

    if (*outfile) {
    ofd = fopen(outfile,"w") ;
    if (ofd == NULL) { error("cannot open outfile\n"); goto outerr ; }
    for (i = 0; i < progcnt; i++) {
    if (progtbl[i].ttpid == 0x1fff) continue ; // service without teletext
    fprintf(ofd,"%d:%d:%s:%s:lang=%.3s\n",
    progtbl[i].service_id, progtbl[i].ttpid, progtbl[i].service_provider_name,
    progtbl[i].service_name, progtbl[i].txtlang);
    }
    fclose(ofd) ;
 }

	progp = NULL;

	if (channel) {
		j = strlen(channel);
		for (i = 0; i < progcnt; i++)
			if (!strncmp(progtbl[i].service_name, channel, j)
			&& progtbl[i].ttpid != 0x1fff) { progp = &progtbl[i];
				break ;
			}
	}

	if (channel && !progp) {
		j = strlen(channel);
		for (i = 0; i < progcnt; i++)
			if (!strncasecmp(progtbl[i].service_name, channel, j)
			&& progtbl[i].ttpid != 0x1fff) { progp = &progtbl[i];
				break ;
			}
	}

    if (sid) {
    for (i = 0; i < progcnt; i++) {
    if ((progtbl[i].service_id == sid) && (progtbl[i].ttpid != 0x1fff)) {
            progp = &progtbl[i]; break ; }
            }
    }

    if (!progp) {
		for (i = 0; i < progcnt; i++)
			if (progtbl[i].ttpid != 0x1fff) {
				progp = &progtbl[i]; break ;
			}
	}

    printf("\nUsing: Service ID = %d ; PMT PID = %d ; TXT PID = %d ;\n"
    "Service type = %d ; Provider Name = %s ; Service name = %s ;\n"
    "language = %.3s ; Text type = %d ; Text Magazine = %1u ; Text page = %3u\n",
    progp->service_id, progp->pmtpid, progp->ttpid, progp->service_type,
    progp->service_provider_name, progp->service_name, progp->txtlang,
    progp->txttype, progp->txtmagazine, progp->txtpage);
    vbi->ttpid = progp->ttpid;

 ttpidfound:
	rawbuf = malloc(rawbuf_size = 8192);
	if (!rawbuf)
		goto outerr;
	rawptr = 0;
#if 0
	close(vbi->fd);
	if ((vbi->fd = open(vbi_name, O_RDWR)) == -1) {
		error("cannot open demux device %s", vbi_name);
		return -1;
	}
#endif
	memset(&filterpar, 0, sizeof(filterpar));
	filterpar.pid = vbi->ttpid;
        filterpar.input = DMX_IN_FRONTEND;
        filterpar.output = DMX_OUT_TAP;
        filterpar.pes_type = DMX_PES_OTHER;
        filterpar.flags = DMX_IMMEDIATE_START;
        if (ioctl(vbi->fd, DMX_SET_PES_FILTER, &filterpar) < 0) {
        error("ioctl: DMX_SET_PES_FILTER %s (%u)", strerror(errno), errno);
        goto outerr;
        }
	return 0;

 outerr:
	close(vbi->fd);
	vbi->fd = -1;
	return -1;
}


struct vbi *open_null_vbi(struct cache *ca)
{
    static int inited = 0;
    struct vbi *vbi;

    if (not inited)
    lang_init();
    inited = 1;

    vbi = malloc(sizeof(*vbi));
    if (!vbi)
    {
		error("out of memory");
		goto fail1;
    }

    vbi->fd = open("/dev/null", O_RDONLY);
	if (vbi->fd == -1)
	{
		error("cannot open null device");
		goto fail2;
	}

    vbi->ttpid = -1;
    out_of_sync(vbi);
    vbi->ppage = vbi->rpage;
    fdset_add_fd(fds, vbi->fd, vbi_handler, vbi);
    return vbi;

fail3:
    close(vbi->fd);
fail2:
    free(vbi);
fail1:
    return 0;
}


void send_errmsg(struct vbi *vbi, char *errmsg, ...)
{
	va_list args;
	if (errmsg == NULL || *errmsg == '\0')
		return;
	va_start(args, errmsg);
	char *buff = NULL;
	if (vasprintf(&buff, errmsg, args) < 0)
		buff = NULL;
	va_end(args);
	if(buff == NULL)
		out_of_mem(-1);
	vbi_send(vbi, EV_ERR, 0, 0, 0, buff);
}
