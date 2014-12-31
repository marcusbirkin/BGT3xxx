#ifndef VBI_H
#define VBI_H

#include "vt.h"
#include "dllist.h"
#include "cache.h"
#include "lang.h"

#define PLL_ADJUST 4

struct raw_page
{
    struct vt_page page[1];
    struct enhance enh[1];
};

struct vbi
{
    int fd;
    struct cache *cache;
    struct dl_head clients[1];
    // page assembly
    struct raw_page rpage[8]; // one for each magazin
    struct raw_page *ppage; // points to page of previous pkt0
    // DVB stuff
    unsigned int ttpid;
    u_int16_t sid;
};

struct vbi_client
{
    struct dl_node node[1];
    void (*handler)(void *data, struct vt_event *ev);
    void *data;
};

struct vbi *vbi_open(char *vbi_dev_name, struct cache *ca,
	const char *channel, char *outfile, u_int16_t sid, int ttpid);
void vbi_close(struct vbi *vbi);
void vbi_reset(struct vbi *vbi);
int vbi_add_handler(struct vbi *vbi, void *handler, void *data);
void vbi_del_handler(struct vbi *vbi, void *handler, void *data);
struct vt_page *vbi_query_page(struct vbi *vbi, int pgno, int subno);

struct vbi *open_null_vbi(struct cache *ca);
void send_errmsg(struct vbi *vbi, char *errmsg, ...);
#endif
