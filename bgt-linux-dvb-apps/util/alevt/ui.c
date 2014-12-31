#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include "vt.h"
#include "misc.h"
#include "xio.h"
#include "vbi.h"
#include "fdset.h"
#include "search.h"
#include "export.h"
#include "ui.h"

static void vtwin_event(struct vtwin *w, struct vt_event *ev);
static void msg(struct vtwin *w, u8 *str, ...);
static void err(struct vtwin *w, u8 *str, ...);

#define hist(w,o) ((w)->hist + (((w)->hist_top + (o)) & (N_HISTORY-1)))


static int inc_hex(int i, int bcd_mode)
{
    i++;
    if (bcd_mode)
    {
	if ((i & 0x000f) > 0x0009)
	    i = (i + 0x0010) & 0x0ff0;
	if ((i & 0x00f0) > 0x090)
	    i = (i + 0x0100) & 0x0f00;
	if ((i & 0x0f00) > 0x0900)
	    i = (i + 0x1000) & 0xf000;
    }
    return i;
}


static int dec_hex(int i, int bcd_mode)
{
    i--;
    if (bcd_mode)
    {
	if ((i & 0x000f) > 0x0009)
	    i = (i & 0xfff0) + 0x0009;
	if ((i & 0x00f0) > 0x0090)
	    i = (i & 0xff00) + 0x0099;
	if ((i & 0x0f00) > 0x0900)
	    i = (i & 0xf000) + 0x0999;
    }
    return i;
}


static void set_title(struct vtwin *w)
{
    char buf[32], buf2[32];

    if (w->subno == ANY_SUB)
	sprintf(buf, "%x", w->pgno);
    else
	sprintf(buf, "%x/%x", w->pgno, w->subno);
    if (w->searching)
	sprintf(buf2, "(%s)", buf);
    else
	sprintf(buf2, "%s", buf);
    xio_title(w->xw, buf2);
}


static void query_page(struct vtwin *w, int pgno, int subno)
{
    w->pgno = pgno;
    w->subno = subno;
    w->searching = 1;
    w->hold = 0; //subno != ANY_SUB;
    xio_set_concealed(w->xw, w->revealed = 0);

    if (hist(w, 0)->pgno != pgno ||
	(hist(w,0)->subno == ANY_SUB && subno != ANY_SUB))
	w->hist_top++;
    hist(w, 0)->pgno = pgno;
    hist(w, 0)->subno = subno;
    hist(w, 1)->pgno = 0;	// end marker

    xio_cancel_selection(w->xw);
    if (vbi_query_page(w->vbi, pgno, subno) == 0)
    {
	w->vtp = 0;
    }
    set_title(w);
}


static void new_or_query(struct vtwin *w, int pgno, int subno, int new_win)
{
    if (new_win)
    {
	if (w->child)
	    query_page(w->child, pgno, subno);
	else
	    vtwin_new(w->xw->xio, w->vbi, 0, w, pgno, subno);
    }
    else
	query_page(w, pgno, subno);
}

static int _next_pgno(int *arg, struct vt_page *vtp)
{
    int pgno = vtp->pgno;

    if (arg[0] == pgno) // want different page
	return 0;
    if (arg[1])		// and not a hex page
	for (; pgno; pgno >>=4)
	    if ((pgno & 15) > 9)
		return 0;
    return 1;
}


static int _next_subno(int *arg, struct vt_page *vtp)
{
    return vtp->pgno == arg[0];	// only subpages of this page
}


static void do_next_pgno(struct vtwin *w, int dir, int bcd_mode, int subs,
    int new_win)
{
    struct vt_page *vtp;
    struct vtwin *cw = (new_win && w->child) ? w->child : w;
    int pgno = cw->pgno;
    int subno = cw->subno;

    if (w->vbi->cache)
    {
	int arg[2];
	arg[0] = pgno;
	arg[1] = bcd_mode;
        if (vtp = w->vbi->cache->op->foreach_pg(w->vbi->cache,
		    pgno, subno, dir, subs ? _next_subno:_next_pgno, &arg))
	{
	    new_or_query(w, vtp->pgno, subs ? vtp->subno : ANY_SUB, new_win);
	    return;
	}
    }
    err(w, "No page.");
}

#define notdigit(x) (not isdigit((x)))


static int chk_screen_fromto(u8 *p, int x, int *n1, int *n2)
{
    p += x;

    if (x >= 0 && x+5 < 42)
	if (notdigit(p[1]) || notdigit(p[0]))
	    if (isdigit(p[2]))
		if (p[3] == '/' || p[3] == ':')
		    if (isdigit(p[4]))
			if (notdigit(p[5]) || notdigit(p[6]))	/* p[6] is save here */
			{
			    *n1 = p[2] % 16;
			    if (isdigit(p[1]))
				*n1 += p[1] % 16 * 16;
			    *n2 = p[4] % 16;
			    if (isdigit(p[5]))
				*n2 = *n2 * 16 + p[5] % 16;
			    return 1;
			}
    return 0;
}


static int chk_screen_pgno(u8 *p, int x, int *pgno, int *subno)
{
    p += x;

    if (x >= 0 && x+4 < 42)
	if (notdigit(p[0]) && notdigit(p[4]))
	    if (isdigit(p[1]) && isdigit(p[2]) && isdigit(p[3]))
	    {
		*pgno = p[1] % 16 * 256 + p[2] % 16 * 16 + p[3] % 16;
		if (*pgno >= 0x100 && *pgno <= 0x999)
		{
		    *subno = ANY_SUB;
		    if (x+6 < 42)
			if (p[4] == '.' || p[4] == '/')
			    if (isdigit(p[5]))
				if (notdigit(p[6]) || notdigit(p[7]))	/* p[7] is save here */
				{
				    *subno = p[5] % 16;
				    if (isdigit(p[6]))
					*subno = *subno * 16 + p[6] % 16;
				}
		    // hackhackhack:
		    // pgno followed by start box gets subno 1
		    if (x+4 < 42 && p[4] == 11)
			*subno = 1;
		    return 1;
		}
	    }
    return 0;
}


static void do_screen_pgno(struct vtwin *w, int x, int y, int new_win)
{
    u8 buf[42];
    int n1, n2, i;

    if (x >= 0 && x < 40)
    {
	if (xio_get_line(w->xw, y, buf+1) == 0)
	{
	    buf[0] = buf[41] = ' ';
	    x++;

	    for (i = -6; i < 35; i++)
	    {
		if (w->vtp == 0 || w->vtp->subno != 0)
		    if (chk_screen_fromto(buf, x+i, &n1, &n2))
		    {
			// subno cycling works wrong with children.
			// so middle button cycles backwards...
			if (w->subno != ANY_SUB)
			    n1 = w->subno;
			n1 = new_win ? dec_hex(n1, 1) : inc_hex(n1, 1);
			if (n1 < 1)
			    n1 = n2;
			if (n1 > n2)
			    n1 = 1;
			new_or_query(w, w->pgno, n1, 0);
			return;
		    }
		if (i >= -4)
		    if (chk_screen_pgno(buf, x+i, &n1, &n2))
		    {
			new_or_query(w, n1, n2, new_win);
			return;
		    }
	    }
	}
    }
    err(w, "No page.");
}


static void do_flof_pgno(struct vtwin *w, int button, int x, int new_win)
{
    struct vt_page *vtp = w->vtp;
    int lk = 99, i, c;

    if (vtp && vtp->flof)
    {
	switch (button)
	{
	    case 1 ... 3:
		for (i = 0; i <= x && i < 40; ++i)
		    if ((c = vtp->data[24][i]) < 8)	// fg-color code
			lk = c;
		lk = "x\0\1\2\3x\3x"[lk];		// color -> link#
		break;
	    case KEY_F(1): lk = 0; break;
	    case KEY_F(2): lk = 1; break;
	    case KEY_F(3): lk = 2; break;
	    case KEY_F(4): lk = 3; break;
	    case KEY_F(5): lk = 5; break;
	}
	if (lk < 6 && (vtp->link[lk].pgno & 0xff) != 0xff)
	{
	    new_or_query(w, vtp->link[lk].pgno, vtp->link[lk].subno, new_win);
	    return;
	}
    }
    else
    {
	switch (button)
	{
	    case 1 ... 3: lk = x / 8;	break;
	    case KEY_F(1): lk = 0;	break;
	    case KEY_F(2): lk = 1;	break;
	    case KEY_F(3): lk = 2;	break;
	    case KEY_F(4): lk = 3;	break;
	    case KEY_F(5): lk = 4;	break;
	}
	switch (lk)
	{
	    case 0: new_or_query(w, 0x100, ANY_SUB, new_win);	return;
	    case 1: do_next_pgno(w, -1, 1, 0, new_win);		return;
	    case 2: new_or_query(w, 0x900, ANY_SUB, new_win);	return;
	    case 3: do_next_pgno(w, 1, 1, 0, new_win);		return;
	    case 4: new_or_query(w, 0x999, ANY_SUB, new_win);	return;
	}
    }
    err(w, "No page.");
}


static void do_hist_pgno(struct vtwin *w)
{
    if (hist(w, -1)->pgno)
    {
	w->hist_top--;
	query_page(w, hist(w, 0)->pgno, hist(w, 0)->subno);
    }
    else
	err(w, "Empty history.");
}


static void put_head_line(struct vtwin *w, u8 *p)
{
    char buf[40];

    if (p == 0)
	xio_get_line(w->xw, 0, buf);
    else
	memcpy(buf + 8, p + 8, 32);

    if (w->subno == ANY_SUB)
	sprintf(buf, "\2%3x \5\xb7", w->pgno);
    else
	sprintf(buf, "\2S%02x \5\xb7", w->subno & 0xff);

    if (w->searching)
	buf[0] = 1;
    if (w->hold)
	buf[4] = 'H';
    if (w->xw->concealed)
	buf[6] = '*';
    buf[7] = 7;

    xio_put_line(w->xw, 0, buf);
}


static void put_menu_line(struct vtwin *w)
{
    if (w->status > 0)
	xio_put_line(w->xw, 24, w->statusline);
    else if (w->vtp && w->vtp->flof)
	xio_put_line(w->xw, 24, w->vtp->data[24]);
    else
	xio_put_line(w->xw, 24, "\0   100     \4<<  \6Help  \4>>\0     999    ");
}


static void _msg(struct vtwin *w, u8 *str, va_list args)
{
    u8 buf[128];
    int i;

    i = vsprintf(buf, str, args);
    if (i > W)
    i = W;
    memset(w->statusline, ' ', W);
    memcpy(w->statusline + (W-i+1)/2, buf, i);
    w->status = 6;
    put_menu_line(w);
}


static void msg(struct vtwin *w, u8 *str, ...)
{
    va_list args;

    va_start(args, str);
    _msg(w, str, args);
    va_end(args);
}


static void err(struct vtwin *w, u8 *str, ...)
{
    va_list args;

    va_start(args, str);
    _msg(w, str, args);
    va_end(args);
}


static void next_search(struct vtwin *w, int rev)
{
    if (w->search)
    {
	int pgno = w->pgno;
	int subno = w->subno;
	int dir = rev ? -w->searchdir : w->searchdir;

	if (search_next(w->search, &pgno, &subno, dir) == 0)
	{
	    query_page(w, pgno, subno);
	    if (not w->searching && w->search->len)
		xio_set_selection(w->xw, w->search->x, w->search->y,
			    w->search->x + w->search->len - 1, w->search->y);
	    return;
	}
	else
	    err(w, "Pattern not found.");
    }
    else
	err(w, "No search pattern.");
}


static void start_search(struct vtwin *w, u8 *string)
{
    if (not string)
	return;

    if (*string)
    {
	if (w->search)
	    search_end(w->search);
	w->search = search_start(w->vbi->cache, string);
	if (w->search == 0)
	{
	    err(w, "Bad search pattern.");
	    return;
	}
    }
    next_search(w, 0);
}


static void start_save2(struct vtwin *w, u8 *name)
{
    if (name && *name)
	if (export(w->export, w->vtp, name))
	    err(w, export_errstr());

    export_close(w->export);
    w->export = 0;
    put_menu_line(w);
}


struct vtwin * vtwin_new(struct xio *xio, struct vbi *vbi, char *geom,
    struct vtwin *parent, int pgno, int subno)
{
    struct vtwin *w;

    if (not(w = malloc(sizeof(*w))))
	goto fail1;

    if (not (w->xw = xio_open_win(xio, geom)))
	goto fail2;
    w->vbi = vbi;
    w->vtp = 0;
    w->search = 0;
    w->export = 0;
    w->parent = parent;
    w->child = 0;
    if (parent && parent->child)
	fatal("internal error: parent already has a child != 0");
    if (parent)
	parent->child = w;

    w->hist_top = 1;
    hist(w,0)->pgno = 0;
    hist(w,1)->pgno = 0;
    w->status = 0;
    xio_set_handler(w->xw, vtwin_event, w);
    vbi_add_handler(w->vbi, vtwin_event, w);
    query_page(w, pgno, subno);
    return w;

fail2:
    free(w);
fail1:
    return 0;
}


static void vtwin_close(struct vtwin *w)
{
    if (w->parent)
	w->parent->child = w->child;
    if (w->child)
	w->child->parent = w->parent;

    if (w->search)
	search_end(w->search);
    if (w->export)
	export_close(w->export);

    vbi_del_handler(w->vbi, vtwin_event, w);
    xio_close_win(w->xw, 1);
    free(w);
}


static void vtwin_event(struct vtwin *w, struct vt_event *ev)
{
    struct xio_win *xw = w->xw;
    int i;

    switch (ev->type)
    {
	case EV_CLOSE:
	    vtwin_close(w);
	    break;

	case EV_TIMER:
	    if (w->status > 0 && --w->status == 0)
		put_menu_line(w);
	    break;

	case EV_KEY:
	{
	    switch (ev->i1)
	    {
		case '0' ... '9':
		    i = ev->i1 - '0';
		    if (w->pgno >= 0x100)
		    {
			if (i == 0)
			    break;
			w->pgno = i;
		    }
		    else
		    {
			w->pgno = w->pgno * 16 + i;
			if (w->pgno >= 0x100)
			    query_page(w, w->pgno, ANY_SUB);
		    }
		    break;
		case 'q':
		case '\e':
		    vtwin_close(w);
		    break;
		case 'h':
		    query_page(w, 0x900, ANY_SUB);
		    break;
		case 'e':
		    if (w->vbi->cache)
		    {
			i = w->vbi->cache->op->mode(w->vbi->cache,
							    CACHE_MODE_ERC, 0);
			w->vbi->cache->op->mode(w->vbi->cache,
							    CACHE_MODE_ERC, !i);
			msg(w, "Error reduction %sabled.", i ? "dis" : "en");
		    }
		    break;
		case 'o':
		    if (vtwin_new(xw->xio, w->vbi, 0, 0, w->pgno, w->subno) == 0)
			err(w, "Unable to open new window.");
		    break;
		case KEY_RIGHT:
		    do_next_pgno(w, 1, not ev->i2, 0, 0);
		    break;
		case KEY_LEFT:
		    do_next_pgno(w, -1, not ev->i2, 0, 0);
		    break;
		case KEY_UP:
		    do_next_pgno(w, -1, not ev->i2, 1, 0);
		    break;
		case KEY_DOWN:
		    do_next_pgno(w, 1, not ev->i2, 1, 0);
		    break;
		case '\b':
		    do_hist_pgno(w);
		    break;
		case ' ':
		    w->hold = !w->hold;
		    break;
		case 'c':
		    vbi_reset(w->vbi);
		    break;
		case 'i':
		    if (w->vtp && w->vtp->flof &&
					 (w->vtp->link[5].pgno & 0xff) != 0xff)
			query_page(w, w->vtp->link[5].pgno,
						 w->vtp->link[5].subno);
		    else
			query_page(w, 0x100, ANY_SUB);
		    break;
		case 'r':
		    xio_set_concealed(xw, w->revealed = !w->revealed);
		    break;
		case KEY_F(1) ... KEY_F(5):
		    do_flof_pgno(w, ev->i1, 0, ev->i2);
		    break;
		case 'n':
		    next_search(w, 0);
		    break;
		case 'N':
		    next_search(w, 1);
		    break;
		default:
		    err(w, "Unused key.");
		    break;
	    }
	    break;
	}
	case EV_RESET:
	{
	    if (w->search)
		search_end(w->search);
	    w->search = 0;

	    query_page(w, w->pgno, w->subno);
	    msg(w, "Cache cleared!");
	    break;
	}
	case EV_MOUSE:
	{
	    if (ev->i1 == 3)
		do_hist_pgno(w);
	    else if (ev->i1 == 5) // wheel mouse
		do_next_pgno(w, 1, not ev->i2, 0, 0);
	    else if (ev->i1 == 4) // wheel mouse
		do_next_pgno(w, -1, not ev->i2, 0, 0);
	    else if (ev->i1 == 7) // dual wheel mouse
		do_next_pgno(w, 1, not ev->i2, 1, 0);
	    else if (ev->i1 == 6) // dual wheel mouse
		do_next_pgno(w, -1, not ev->i2, 1, 0);
	    else if (ev->i4 == 24)
		do_flof_pgno(w, ev->i1, ev->i3, ev->i1 == 2);
	    else if (ev->i4 == 0 && ev->i3 < 5)
	    {
		if (ev->i1 == 1)
		    w->hold = !w->hold;
		else
		    vtwin_new(xw->xio, w->vbi, 0, 0, w->pgno, w->subno);
	    }
	    else if (ev->i4 == 0 && ev->i3 < 8)
	    {
		if (ev->i1 == 2 && w->child)
		    w = w->child;
		xio_set_concealed(w->xw, w->revealed = !w->revealed);
	    }
	    else
		do_screen_pgno(w, ev->i3, ev->i4, ev->i1 == 2);
	    break;
	}
	case EV_PAGE:
	{
	    struct vt_page *vtp = ev->p1;

	    if (0)
		if (vtp->errors)
		    printf("errors=%4d\n",vtp->errors);
	    if (w->searching || not(w->hold || ev->i1))
		if (vtp->pgno == w->pgno)
		    if (w->subno == ANY_SUB || vtp->subno == w->subno)
		{
			w->searching = 0;
			w->vtp = vtp;
			put_head_line(w, vtp->data[0]);
			for (i = 1; i < 24; ++i)
			    xio_put_line(w->xw, i, vtp->data[i]);
			put_menu_line(w);
			set_title(w);
		}
	    break;
	}
	case EV_HEADER:
	{
	    u8 *p = ev->p1;
	    int hdr_mag = ev->i1 / 256;
	    int flags = ev->i3;
	    int mag = w->pgno;
	    if (mag >= 0x10)
		mag = mag >> 4;
	    if (mag >= 0x10)
		mag = mag >> 4;
	    if (flags & PG_OUTOFSEQ)
		p = 0;
	    else
		if (~flags & PG_MAGSERIAL)
		    if (mag != hdr_mag)
			p = 0;

	    put_head_line(w, p);
	    break;
	}
	case EV_XPACKET:
	{
#if 0 /* VPS data (seems to be unused in .de */
	    u8 *p = ev->p1;

	    if (ev->i1 == 8 && ev->i2 == 30 && p[0]/2 == 1)
	    {
		int i;
		int pil, cni, pty, misc;

		for (i = 7; i < 20; ++i)
		    p[i] = hamm8(p+i, &ev->i3);
		if (ev->i3 & 0xf000)	/* uncorrectable errors */
		    break;
		cni = p[9] + p[15]/4*16 + p[16]%4*64 + p[10]%4*256
					+ p[16]/4*1024 + p[17]*4096;
		pty = p[18] + p[19]*16;
		pil = p[10]/4 + p[11]*4 + p[12]*64 + p[13]*1024
					+ p[14]*16384 + p[15]%4*262144;
		misc = p[7] + p[8]*16;
		err(w, "%02x %04x %05x %02x: %.20s", misc, cni, pil, pty, p+20);
	    }
#endif
	    break;
	}
	case EV_ERR:
	{
	    char *errmsg = ev->p1;
	    if (errmsg != NULL && *errmsg != '\0')
	    {
	    	err(w, errmsg);
	    	w->status = 30;
	    	ev->p1 = NULL;
	    	free(errmsg);
	    }
	    break;
	}
    }
}
