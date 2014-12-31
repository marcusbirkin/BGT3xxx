#ifndef UI_H
#define UI_H

#include "vt.h"
#include "xio.h"
#include "vbi.h"
#include "search.h"

#define N_HISTORY (1 << 6) // number of history entries

struct vtwin
{
    struct vtwin *parent, *child;
    struct xio_win *xw;
    struct vbi *vbi;
    struct {
    int pgno;
    int subno;
    } hist[N_HISTORY];
    int hist_top;
    int searching;
    int revealed;
    int hold;
    int pgno, subno;
    struct vt_page *vtp;
    struct search *search;
    int searchdir;
    int status;
    u8 statusline[W+1];
    struct export *export;
};

extern struct vtwin *vtwin_new(struct xio *xio, struct vbi *vbi, char *geom,
    struct vtwin *parent, int pgno, int subno);
#endif
