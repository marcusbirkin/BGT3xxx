#ifndef LANG_H
#define LANG_H

#include "misc.h"
#include "vt.h"

extern int latin1;

#define LATIN1 1
#define LATIN2 2
#define KOI8 3
#define GREEK 4


struct enhance
{
    int next_des; // next expected designation code
    u32 trip[13*16]; // tripplets
};

void lang_init(void);
void conv2latin(u8 *p, int n, int lang);
void conv2koi8(u8 *p);
void conv2greek(u8 *p);
void init_enhance(struct enhance *eh);
void add_enhance(struct enhance *eh, int dcode, u32 *data);
void enhance(struct enhance *eh, struct vt_page *vtp);
#endif
