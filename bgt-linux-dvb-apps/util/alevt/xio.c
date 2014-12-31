#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <sys/time.h>
#include "vt.h"
#include "misc.h"
#include "dllist.h"
#include "xio.h"
#include "fdset.h"
#include "lang.h"
#include "icon.xbm"
#include "font.h"

#define WW (W*CW) /* pixel width of window */
#define WH (H*CH) /* pixel hegiht of window */
#define NO_SEL 999 /* sel_y1 value if no selection */
#define SEL_MIN_TIME 500 /* anything shorter is a click */

static struct dl_head dpys[1]; /* list of all displays */
static void xio_timer(void *data, int fd);
static void handle_event(struct xio *xio, int fd);


static int timer_init(int argc, char **argv)
{
    int p[2], timer_pid, i;

    if (pipe(p) == -1)
    return -1;

    signal(SIGPIPE, SIG_DFL);
    timer_pid = fork();
    if (timer_pid == -1)
    return -1;
    if (timer_pid > 0)
    {
    fdset_add_fd(fds, p[0], xio_timer, 0);
    close(p[1]);
    return 0;
    }

    close(p[0]);
    for (i = 0; i < 32; ++i)
    if (p[1] != i)
    close(i);
    memcpy(argv[0], "Timer", 6);

    for (;;)
    {
    usleep(300000);
    write(p[1], "*", 1);
    }
}


static int local_init(int argc, char **argv)
{
    static int inited = 0;

    if (inited)
	return 0;

    if (timer_init(argc, argv) == -1)
	return -1;

    dl_init(dpys);

    inited = 1;
    return 0;
}


static int get_colors(struct xio *xio)
{
    int i;
    XColor c;

    static short rgb[][3] =
    {
	{ 0x0000,0x0000,0x0000 },
	{ 0xffff,0x0000,0x0000 },
	{ 0x0000,0xffff,0x0000 },
	{ 0xffff,0xffff,0x0000 },
	{ 0x0000,0x0000,0xffff },
	{ 0xffff,0x0000,0xffff },
	{ 0x0000,0xffff,0xffff },
	{ 0xffff,0xffff,0xffff },
	{ 0x7fff,0x7fff,0x7fff },
	{ 0x7fff,0x0000,0x0000 },
	{ 0x0000,0x7fff,0x0000 },
	{ 0x7fff,0x7fff,0x0000 },
	{ 0x0000,0x0000,0x7fff },
	{ 0x7fff,0x0000,0x7fff },
	{ 0x0000,0x7fff,0x7fff },
	{ 0x3fff,0x3fff,0x3fff },
    };

    for (i = 0; i < 16; ++i)
    {
	c.red = rgb[i][0];
	c.green = rgb[i][1];
	c.blue = rgb[i][2];
	if (XAllocColor(xio->dpy, xio->cmap, &c) == 0)
	    return -1;
	xio->color[i] = c.pixel;
    }
    return 0;
}


static int get_fonts(struct xio *xio)
{
    GC gc;
    int i;
    unsigned char *font_bits;
    switch(latin1) {
        case LATIN1: font_bits=font1_bits; break;
        case LATIN2: font_bits=font2_bits; break;
        case KOI8: font_bits=font3_bits; break;
        case GREEK: font_bits=font4_bits; break;
        default: font_bits=font1_bits; break;
    }

    xio->font[0] = XCreateBitmapFromData(xio->dpy, xio->root,
					 font_bits, font_width, font_height);
    xio->font[1] = XCreatePixmap(xio->dpy, xio->root,
    					font_width, font_height*2, 1);
    gc = XCreateGC(xio->dpy, xio->font[0], 0, 0);
    for (i = 0; i < font_height; ++i)
    {
	XCopyArea(xio->dpy, xio->font[0], xio->font[1], gc, 0, i,
						    font_width, 1, 0, i*2);
	XCopyArea(xio->dpy, xio->font[0], xio->font[1], gc, 0, i,
						    font_width, 1, 0, i*2+1);
    }
    XFreeGC(xio->dpy, gc);
    return 0;
}


static void xlib_conn_watch(Display *dpy, void *fds, int fd, int open_flag, void *data)
{
    if (open_flag)
	fdset_add_fd(fds, fd, XProcessInternalConnection, dpy);
    else
	fdset_del_fd(fds, fd);
}


struct xio * xio_open_dpy(char *dpy, int argc, char **argv)
{
    XClassHint classhint[1];
    struct xio *xio;

    if (local_init(argc, argv) == -1)
	goto fail1;
    
    if (not(xio = malloc(sizeof(*xio))))
	goto fail1;

    if (not(xio->dpy = XOpenDisplay(dpy)))
	goto fail2;

    xio->fd = ConnectionNumber(xio->dpy);
    xio->argc = argc;
    xio->argv = argv;
    dl_init(xio->windows);
    xio->screen = DefaultScreen(xio->dpy);
    xio->depth = DefaultDepth(xio->dpy, xio->screen);
    xio->width = DisplayWidth(xio->dpy, xio->screen);
    xio->height = DisplayHeight(xio->dpy, xio->screen);
    xio->root = DefaultRootWindow(xio->dpy);
    xio->cmap = DefaultColormap(xio->dpy, xio->screen);
    xio->xa_del_win = XInternAtom(xio->dpy, "WM_DELETE_WINDOW", False);
    xio->xa_targets = XInternAtom(xio->dpy, "TARGETS", False);
    xio->xa_timestamp = XInternAtom(xio->dpy, "TIMESTAMP", False);
    xio->xa_multiple = XInternAtom(xio->dpy, "MULTIPLE", False);
    xio->xa_text = XInternAtom(xio->dpy, "TEXT", False);

    if (get_colors(xio) == -1)
	goto fail3;

    if (get_fonts(xio) == -1)
	goto fail3;

    if (fdset_add_fd(fds, xio->fd, handle_event, xio) == -1)
	goto fail3;
    
    XAddConnectionWatch(xio->dpy, PTR xlib_conn_watch, PTR fds);
	
    xio->icon = XCreateBitmapFromData(xio->dpy, xio->root,
					icon_bits, icon_width, icon_height);

    xio->group_leader = XCreateSimpleWindow(xio->dpy, xio->root,
    							0, 0, 1, 1, 0, 0, 0);
    XSetCommand(xio->dpy, xio->group_leader, xio->argv, xio->argc);
    classhint->res_name = "VTLeader";
    classhint->res_class = "AleVT";
    XSetClassHint(xio->dpy, xio->group_leader, classhint);

    dl_insert_first(dpys, xio->node);
    return xio;

fail4:
    fdset_del_fd(fds, xio->fd);
fail3:
    XCloseDisplay(xio->dpy);
fail2:
    free(xio);
fail1:
    return 0;
}


static void set_user_geometry(struct xio_win *xw, char *geom, XSizeHints *sh, int bwidth)
{
    static int gravs[] = { NorthWestGravity, NorthEastGravity,
			   SouthWestGravity, SouthEastGravity };
    int f, g = 0;

    f = XParseGeometry(geom, &sh->x, &sh->y, &sh->width, &sh->height);

    if (f & WidthValue)
	sh->width = sh->base_width + sh->width * sh->width_inc;
    if (f & HeightValue)
	sh->height = sh->base_height + sh->height * sh->height_inc;
    if (f & XNegative)
	g+=1, sh->x = xw->xio->width + sh->x - sh->width - bwidth;
    if (f & YNegative)
	g+=2, sh->y = xw->xio->height + sh->y - sh->height - bwidth;

    sh->width = bound(sh->min_width, sh->width, sh->max_width);
    sh->height = bound(sh->min_height, sh->height, sh->max_height);

    if (f & (WidthValue | HeightValue))
	sh->flags |= USSize;
    if (f & (XValue | YValue))
	sh->flags |= USPosition | PWinGravity;

    sh->win_gravity = gravs[g];
}


struct xio_win * xio_open_win(struct xio *xio, char *geom)
{
    struct xio_win *xw;
    XSetWindowAttributes attr;
    XGCValues gcval;
    XSizeHints sizehint[1];
    XClassHint classhint[1];
    XWMHints wmhint[1];

    if (not(xw = malloc(sizeof(*xw))))
	goto fail1;

    xw->xio = xio;

    sizehint->flags = PSize | PBaseSize | PMinSize | PMaxSize | PResizeInc;
    sizehint->x = sizehint->y = 0;
    sizehint->width_inc = CW;
    sizehint->height_inc = CH;
    sizehint->base_width = 0;
    sizehint->base_height = 0;
    sizehint->min_width = 11*CW;
    sizehint->min_height = 1*CH;
    sizehint->max_width = sizehint->width = WW + CW;
    sizehint->max_height = sizehint->height = WH;
    set_user_geometry(xw, geom, sizehint, 1);

    attr.background_pixel = xio->color[0];
    attr.event_mask = KeyPressMask |
    			 ButtonPressMask|ButtonReleaseMask|Button1MotionMask |
			 ExposureMask;
    xw->win = XCreateWindow(xio->dpy, xio->root,
		sizehint->x, sizehint->y, sizehint->width, sizehint->height, 1,
			CopyFromParent, CopyFromParent, CopyFromParent,
			CWBackPixel|CWEventMask, &attr);

    classhint->res_name = "VTPage";
    classhint->res_class = "AleVT";

    wmhint->flags = InputHint | StateHint | WindowGroupHint | IconPixmapHint;
    wmhint->input = True;
    wmhint->initial_state = NormalState; //IconicState;
    wmhint->window_group = xio->group_leader;
    wmhint->icon_pixmap = xio->icon;

    XSetWMProperties(xio->dpy, xw->win, 0,0, 0,0, sizehint, wmhint, classhint);
    XSetWMProtocols(xio->dpy, xw->win, &xio->xa_del_win, 1);

    xw->title[0] = 0;
    xio_title(xw, "AleVT"); // will be reset pretty soon

    gcval.graphics_exposures = False;
    xw->gc = XCreateGC(xio->dpy, xw->win, GCGraphicsExposures, &gcval);

    xw->tstamp = CurrentTime;
    xw->fg = xw->bg = -1;	/* unknown colors */

    xw->curs_x = xw->curs_y = 999;	// no cursor

    xw->sel_y1 = NO_SEL;	/* no selection area */
    xw->sel_start_t = 0;	/* no selection-drag active */
    xw->sel_set_t = 0;		/* not selection owner */
    xw->sel_pixmap = 0;		/* no selection pixmap yet */

    xio_clear_win(xw);
    xw->blink_on = xw->reveal = 0;

    xw->handler = 0;

    XMapWindow(xio->dpy, xw->win);
    dl_insert_first(xio->windows, xw->node);
    return xw;

fail2:
    free(xw);
fail1:
    return 0;
}


void xio_close_win(struct xio_win *xw, int dpy_too)
{
    struct xio *xio = xw->xio;

    XDestroyWindow(xio->dpy, xw->win);
    dl_remove(xw->node);
    free(xw);

    if (dpy_too && dl_empty(xio->windows))
	xio_close_dpy(xio);
}


void xio_close_dpy(struct xio *xio)
{
    while (not dl_empty(xio->windows))
	xio_close_win((struct xio_win *)xio->windows->first, 0);

    XDestroyWindow(xio->dpy, xio->group_leader);
    XRemoveConnectionWatch(xio->dpy, PTR xlib_conn_watch, PTR fds);
    fdset_del_fd(fds, xio->fd);
    dl_remove(xio->node);
    free(xio);
}


void xio_set_handler(struct xio_win *xw, void *handler, void *data)
{
    xw->handler = handler;
    xw->data = data;
}


void xio_title(struct xio_win *xw, char *title)
{
    char buf[sizeof(xw->title) + 32];

    if (strlen(title) >= sizeof(xw->title))
	return; //TODO: trimm...
    if (strcmp(xw->title, title) == 0)
	return;

    strcpy(xw->title, title);
    sprintf(buf, "AleVT " VERSION "    %s", xw->title);
    XStoreName(xw->xio->dpy, xw->win, buf);
    XSetIconName(xw->xio->dpy, xw->win, xw->title);
}


void xio_clear_win(struct xio_win *xw)
{
    memset(xw->ch, ' ', sizeof(xw->ch));
    xw->dheight = xw->blink = xw->concealed = 0;
    xw->hidden = xw->lhidden = 0;
    xw->modified = ALL_LINES;
}


void xio_put_line(struct xio_win *xw, int y, u8 *data)
{
    u8 *p = xw->ch + y*W;
    u8 *ep = p + W;
    lbits yb = 1 << y;
    lbits x = xw->dheight;

    if (y < 0 || y >= H)
	return;

    if (memcmp(data, p, ep - p) == 0)
	return;

    xw->modified |= yb;
    xw->blink &= ~yb;
    xw->dheight &= ~yb;
    xw->concealed &= ~yb;

    while (p < ep)
	switch (*p++ = *data++)
	{
	    case 0x08:
		xw->blink |= yb;
		break;
	    case 0x0d:
		if (y < H-1)
		    xw->dheight |= yb;
		break;
	    case 0x18:
	    	xw->concealed |= yb;
		break;
	}

    if ((xw->dheight ^ x) & yb) // dheight has changed, recalc hidden
    {
	xw->hidden &= yb*2 - 1;
	for (; yb & ALL_LINES/2; yb *= 2)
	    if (~xw->hidden & xw->dheight & yb)
		xw->hidden |= yb*2;
    }
}


void xio_put_str(struct xio_win *xw, int y, u8 *str)
{
    u8 buf[W];
    int l;
    l = strlen(str);
    if (l < W)
    {
	memcpy(buf, str, l);
	memset(buf + l, ' ', W - l);
    }
    else
	memcpy(buf, str, W);
    xio_put_line(xw, y, buf);
}


static void dirty(struct xio_win *xw, int y1, int y2) // mark [y1,y2[ dirty
{
    if (y1 >= 0 && y1 < H && y1 < y2)
    {
	if (y2 > H)
	    y2 = H;
	if (xw->hidden & (1 << y1))
	    y1--;
	while (y1 < y2)
	    xw->modified |= 1 << y1++;
    }
}


int xio_get_line(struct xio_win *xw, int y, u8 *data)
{
    if (y < 0 || y >= H)
	return -1;
    if (xw->hidden & (1 << y))
	y--;
    memcpy(data, xw->ch + y*W, 40);
    return 0;
}


void xio_set_cursor(struct xio_win *xw, int x, int y)
{
    if (xw->curs_y >= 0 && xw->curs_y < H)
	dirty(xw, xw->curs_y, xw->curs_y + 1);
    if (x >= 0 && x < W && y >= 0 && y < H)
	dirty(xw, y, y + 1);
    else
	x = y = 999;
    xw->curs_x = x;
    xw->curs_y = y;
}


static inline void draw_char(struct xio_win *xw, Window win, int fg, int bg,
    int c, int dbl, int x, int y, int ry)
{
    struct xio *xio = xw->xio;

    if (fg != xw->fg)
	XSetForeground(xio->dpy, xw->gc, xio->color[xw->fg = fg]);
    if (bg != xw->bg)
	XSetBackground(xio->dpy, xw->gc, xio->color[xw->bg = bg]);

    if (dbl)
    {
	XCopyPlane(xio->dpy, xio->font[1], win, xw->gc,
				c%32*CW, c/32*CH*2, CW, CH*2, x*CW, y*CH, 1);
    }
    else
    {
	XCopyPlane(xio->dpy, xio->font[0], win, xw->gc,
				    c%32*CW, c/32*CH, CW, CH, x*CW, y*CH, 1);
	if (xw->dheight & (1<<ry))
	    XCopyPlane(xio->dpy, xio->font[0], win, xw->gc,
			    ' '%32*CW, ' '/32*CH, CW, CH, x*CW, y*CH+CH, 1);
    }
}

static void draw_cursor(struct xio_win *xw, int x, int y, int dbl)
{
    struct xio *xio = xw->xio;

    if (xw->blink_on)
	XSetForeground(xio->dpy, xw->gc, xio->color[xw->fg = xw->bg ^ 8]);
    XDrawRectangle(xio->dpy, xw->win, xw->gc, x * CW, y * CH, CW-1,
    		(dbl ? 2*CH : CH)-1);
}


void xio_update_win(struct xio_win *xw)
{
    u8 *p = xw->ch;
    lbits yb, redraw;
    int x, y, c;

    if (xw->modified == 0)
	return;

    redraw = xw->modified; // all modified lines
    redraw |= xw->lhidden; // all previously hidden lines
    redraw &= ~xw->hidden;

    xw->lhidden = xw->hidden;
    xw->modified = 0;

    if (redraw == 0)
	return;

    for (yb = 1, y = 0; y < H; ++y, yb *= 2)
	if (redraw & yb)
	{
	    int fg = 7, bg = 0, _fg, _bg;
	    int dbl = 0, blk = 0, con = 0, gfx = 0, sep = 0, hld = 0;
	    int last_ch = ' ';

	    for (x = 0; x < W; ++x)
	    {
		switch (c = *p++)
		{
		    case 0x00 ... 0x07: /* alpha + foreground color */
			fg = c & 7;
			gfx = 0;
			con = 0;
			goto ctrl;
		    case 0x08: /* flash */
			blk = not xw->blink_on;
			goto ctrl;
		    case 0x09: /* steady */
			blk = 0;
			goto ctrl;
		    case 0x0a: /* end box */
		    case 0x0b: /* start box */
			goto ctrl;
		    case 0x0c: /* normal height */
			dbl = 0;
			goto ctrl;
		    case 0x0d: /* double height */
			dbl = y < H-1;
			goto ctrl;
		    case 0x10 ... 0x17: /* graphics + foreground color */
			fg = c & 7;
			gfx = 1;
			con = 0;
			goto ctrl;
		    case 0x18: /* conceal display */
			con = not xw->reveal;
			goto ctrl;
		    case 0x19: /* contiguous graphics */
			sep = 0;
			goto ctrl;
		    case 0x1a: /* separate graphics */
			sep = 1;
			goto ctrl;
		    case 0x1c: /* black background */
			bg = 0;
			goto ctrl;
		    case 0x1d: /* new background */
			bg = fg;
			goto ctrl;
		    case 0x1e: /* hold graphics */
			hld = 1;
			goto ctrl;
		    case 0x1f: /* release graphics */
			hld = 0;
			goto ctrl;

		    case 0x0e: /* SO (reserved, double width) */
		    case 0x0f: /* SI (reserved, double size) */
		        c= ' '; break;
		    case 0x1b: /* ESC (reserved) */
			c = ' ';
			break;

		    ctrl:
			c = ' ';
			if (hld && gfx)
			    c = last_ch;
			break;

		    case 0x80 ... 0x9f: /* these aren't used */
			c = BAD_CHAR;
			break;

		    default: /* mapped to selected font */
			break;
		}

		if (gfx && (c & 0xa0) == 0x20)
		{
		    last_ch = c;
		    c += (c & 0x40) ? 32 : -32;
		}

		_fg = fg;
		_bg = bg;
		if (blk)
		    _fg |= 8;
		if (y >= xw->sel_y1 && y < xw->sel_y2 &&
		    x >= xw->sel_x1 && x < xw->sel_x2)
		    _bg |= 8;
		if (con)
		    _fg = _bg;
		
		draw_char(xw, xw->win, _fg, _bg, c, dbl, x, y, y);

		if (y == xw->curs_y && x == xw->curs_x)
		    draw_cursor(xw, xw->curs_x, xw->curs_y, dbl);

		if (xw->sel_pixmap && (_bg & 8))
		    draw_char(xw, xw->sel_pixmap, con ? bg : fg, bg, c, dbl,
					    x - xw->sel_x1, y - xw->sel_y1, y);
	    }
	}
	else
	    p += 40;
}


static void for_all_windows(void (*func)(struct xio_win *xw), struct xio_win *except)
{
    struct xio *xio, *vtn;
    struct xio_win *xw, *vwn;

    for (xio = PTR dpys->first; vtn = PTR xio->node->next; xio = vtn)
	for (xw = PTR xio->windows->first; vwn = PTR xw->node->next; xw = vwn)
	    if (xw != except)
		func(xw);
}


int xio_set_concealed(struct xio_win *xw, int on)
{
    on = !!on;
    if (xw->reveal == on)
	return on;

    xw->reveal = on;
    xw->modified |= xw->concealed;
    return !on;
}


static void sel_set(struct xio_win *xw, int x1, int y1, int x2, int y2)
{
    int t;

    x1 = bound(0, x1, W-1);
    y1 = bound(0, y1, H-1);
    x2 = bound(0, x2, W-1);
    y2 = bound(0, y2, H-1);

    if (x1 > x2)
	t = x1, x1 = x2, x2 = t;
    if (y1 > y2)
	t = y1, y1 = y2, y2 = t;

    dirty(xw, xw->sel_y1, xw->sel_y2);

    if (xw->hidden & (1 << y1))
	y1--;
    if (xw->hidden & (2 << y2))
	y2++;

    xw->sel_x1 = x1;
    xw->sel_y1 = y1;
    xw->sel_x2 = x2 + 1;
    xw->sel_y2 = y2 + 1;
    dirty(xw, xw->sel_y1, xw->sel_y2);
}


static void sel_abort(struct xio_win *xw)
{
    if (xw->sel_set_t)
	XSetSelectionOwner(xw->xio->dpy, XA_PRIMARY, None, xw->sel_set_t);
    if (xw->sel_y1 != NO_SEL)
	dirty(xw, xw->sel_y1, xw->sel_y2);
    xw->sel_y1 = NO_SEL;
    xw->sel_set_t = 0;
    xw->sel_start_t = 0;
}


static void sel_start(struct xio_win *xw, int x, int y, Time t)
{
    sel_abort(xw);
    xw->sel_start_x = x;
    xw->sel_start_y = y;
    xw->sel_start_t = t;
}


static void sel_move(struct xio_win *xw, int x, int y, Time t)
{
    if (xw->sel_start_t == 0)
	return;
    if (xw->sel_y1 == NO_SEL)
	if (t - xw->sel_start_t < SEL_MIN_TIME)
	    if (x == xw->sel_start_x)
		if (y == xw->sel_start_y)
		    return;
    sel_set(xw, xw->sel_start_x, xw->sel_start_y, x, y);
}


static int sel_end(struct xio_win *xw, int x, int y, Time t)
{
    sel_move(xw, x, y, t);
    xw->sel_start_t = 0;

    if (xw->sel_y1 == NO_SEL)
	return 0;

    for_all_windows(sel_abort, xw);
    XSetSelectionOwner(xw->xio->dpy, XA_PRIMARY, xw->win, t);
    if (XGetSelectionOwner(xw->xio->dpy, XA_PRIMARY) == xw->win)
	xw->sel_set_t = t;
    else
	sel_abort(xw);
    return 1;
}


static int sel_convert2ascii(struct xio_win *xw, u8 *buf)
{
    u8 *d = buf;
    int x, y, nl = 0;

    for (y = xw->sel_y1; y < xw->sel_y2; y++)
    {
	u8 *s = xw->ch + y * W;
	int gfx = 0, con = 0;

	if (~xw->hidden & (1 << y))
	{
	    for (x = 0; x < xw->sel_x2; ++x)
	    {
		int ch, c = ' ';
		switch (ch = *s++)
		{
		    case 0x00 ... 0x07:
			gfx = con = 0;
			break;
		    case 0x10 ... 0x17:
			gfx = 1, con = 0;
			break;
		    case 0x18:
			con = not xw->reveal;
			break;
		    case 0xa0 ... 0xff:
		    case 0x20 ... 0x7f:
			if (not con)
			    if (gfx && ch != ' ' && (ch & 0xa0) == 0x20)
				c = '#';
			    else if (ch == 0x7f)
				c = '*';
			    else
				c = ch;
			break;
		}
		if (x >= xw->sel_x1)
		{
		    if (nl)
			*d++ = '\n', nl = 0;
		    *d++ = c;
		}
	    }
	    nl = 1;
	}
    }
    *d = 0; // not necessary
    return d - buf;
}


static Pixmap sel_convert2pixmap(struct xio_win *xw)
{
    struct xio *xio = xw->xio;
    Pixmap pm;

    if (xw->sel_y1 == NO_SEL)
    return None;

    pm = XCreatePixmap(xio->dpy, xio->root, (xw->sel_x2 - xw->sel_x1) * CW,
					    (xw->sel_y2 - xw->sel_y1) * CH,
								 xio->depth);
    xw->sel_pixmap = pm;
    dirty(xw, xw->sel_y1, xw->sel_y2);
    xio_update_win(xw);
    xw->sel_pixmap = 0;

    return pm;
}


static int sel_do_conv(struct xio_win *xw, Window w, Atom type, Atom prop)
{
    struct xio *xio = xw->xio;

    if (type == xio->xa_targets)
    {
	u32 atoms[6];

	atoms[0] = XA_STRING;
	atoms[1] = xio->xa_text;
	atoms[2] = XA_PIXMAP;
	atoms[3] = XA_COLORMAP;
	atoms[4] = xio->xa_multiple;
	atoms[5] = xio->xa_timestamp;
	XChangeProperty(xio->dpy, w, prop, type,
				32, PropModeReplace, PTR atoms, NELEM(atoms));
    }
    else if (type == xio->xa_timestamp)
    {
	u32 t = xw->sel_set_t;

	XChangeProperty(xio->dpy, w, prop, type, 32, PropModeReplace, PTR &t, 1);
    }
    else if (type == XA_COLORMAP)
    {
	u32 t = xio->cmap;

	XChangeProperty(xio->dpy, w, prop, type, 32, PropModeReplace, PTR &t, 1);
    }
    else if (type == XA_STRING || type == xio->xa_text)
    {
	u8 buf[H * (W+1)];
	int len;

	len = sel_convert2ascii(xw, buf);

	XChangeProperty(xio->dpy, w, prop, type, 8, PropModeReplace, buf, len);
    }
    else if (type == XA_PIXMAP || type == XA_DRAWABLE)
    {
	Pixmap pm;

	pm = sel_convert2pixmap(xw);

	XChangeProperty(xio->dpy, w, prop, type, 32, PropModeReplace, PTR &pm, 1);
    }
    else if (type == xio->xa_multiple)
    {
	u32 *atoms, ty, fo, i;
	unsigned long n, b;

	if (prop != None)
	{
	    if (Success == XGetWindowProperty(xio->dpy, w, prop, 0, 1024, 0,
			AnyPropertyType, PTR &ty, PTR &fo, &n, &b, PTR &atoms))
	    {
		if (fo == 32 && n%2 == 0)
		{
		    for (i = 0; i < n; i += 2)
			if (sel_do_conv(xw, w, atoms[i], atoms[i+1]) == None)
			    atoms[i] = None;
		}
		XChangeProperty(xio->dpy, w, prop, type, 32, PropModeReplace,
								 PTR atoms, n);
		XFree(atoms);
	    }
	}
    }
    else
	return None;
    return prop;
}


static void sel_send(struct xio_win *xw, XSelectionRequestEvent *req)
{
    XSelectionEvent ev[1];

    if (req->property == None)
	req->property = req->target;

    ev->type = SelectionNotify;
    ev->requestor = req->requestor;
    ev->selection = req->selection;
    ev->target = req->target;
    ev->property = sel_do_conv(xw, req->requestor, req->target, req->property);
    ev->time = req->time;
    XSendEvent(xw->xio->dpy, req->requestor, False, 0, PTR ev);
}


static void sel_retrieve(struct xio_win *xw, Window w, Atom prop, int del)
{
    u8 *data;
    u32 ty, fo;
    unsigned long n, b;
    struct xio *xio = xw->xio;

    if (prop == None)
	return;

    if (Success == XGetWindowProperty(xio->dpy, w, prop, 0, 1024, del,
			AnyPropertyType, PTR &ty, PTR &fo, &n, &b, PTR &data))
    {
	if (fo == 8 && n != 0)
	{
	    struct vt_event vtev[1];

	    vtev->resource = xw;
	    vtev->type = EV_SELECTION;
	    vtev->i1 = n;
	    vtev->p1 = data;
	    xw->handler(xw->data, vtev);
	}
	XFree(data);
    }
}


void xio_cancel_selection(struct xio_win *xw)
{
    sel_abort(xw);
}


void xio_query_selection(struct xio_win *xw)
{
    struct xio *xio = xw->xio;

    if (XGetSelectionOwner(xio->dpy, XA_PRIMARY) == None)
	sel_retrieve(xw, xio->root, XA_CUT_BUFFER0, False);
    else
    {
	XDeleteProperty(xio->dpy, xw->win, XA_STRING);
	XConvertSelection(xio->dpy, XA_PRIMARY, XA_STRING,
					    XA_STRING, xw->win, xw->tstamp);
    }
}


void xio_set_selection(struct xio_win *xw, int x1, int y1, int x2, int y2)
{
    sel_start(xw, x1, y1, xw->tstamp - SEL_MIN_TIME);
    sel_end(xw, x2, y2, xw->tstamp);
}


static void handle_event(struct xio *xio, int fd)
{
    struct xio_win *xw;
    struct vt_event vtev[1];
    XEvent ev[1];

    XNextEvent(xio->dpy, ev);

    for (xw = PTR xio->windows->first; xw->node->next; xw = PTR xw->node->next)
	if (xw->win == ev->xany.window)
	    break;
    if (xw->node->next == 0)
	return;

    vtev->resource = xw;

    switch(ev->type)
    {
	case Expose:
	{
	    int y1 = ev->xexpose.y / CH;
	    int y2 = (ev->xexpose.y + ev->xexpose.height + CH-1) / CH;

	    dirty(xw, y1, y2);
	    break;
	}
	case ClientMessage:
	{
	    vtev->type = EV_CLOSE;
	    if (ev->xclient.format == 32)
		if ((Atom)ev->xclient.data.l[0] == xio->xa_del_win)
		    xw->handler(xw->data, vtev);
	    break;
	}
	case KeyPress:
	{
	    unsigned char ch;
	    KeySym k;

	    xw->tstamp = ev->xkey.time;
	    vtev->type = EV_KEY;
	    vtev->i1 = 0;
	    vtev->i2 = (ev->xkey.state & ShiftMask) != 0;
	    if (XLookupString(&ev->xkey, &ch, 1, &k, 0))
		vtev->i1 = ch;
	    else
		switch (k)
		{
		    case XK_Left:	vtev->i1 = KEY_LEFT;	break;
		    case XK_Right:	vtev->i1 = KEY_RIGHT;	break;
		    case XK_Up:		vtev->i1 = KEY_UP;	break;
		    case XK_Down:	vtev->i1 = KEY_DOWN;	break;
		    case XK_Prior:	vtev->i1 = KEY_PUP;	break;
		    case XK_Next:	vtev->i1 = KEY_PDOWN;	break;
		    case XK_Delete:	vtev->i1 = KEY_DEL;	break;
		    case XK_Insert:	vtev->i1 = KEY_INS;	break;
		    case XK_F1:		vtev->i1 = KEY_F(1);	break;
		    case XK_F2:		vtev->i1 = KEY_F(2);	break;
		    case XK_F3:		vtev->i1 = KEY_F(3);	break;
		    case XK_F4:		vtev->i1 = KEY_F(4);	break;
		    case XK_F5:		vtev->i1 = KEY_F(5);	break;
		    case XK_F6:		vtev->i1 = KEY_F(6);	break;
		    case XK_F7:		vtev->i1 = KEY_F(7);	break;
		    case XK_F8:		vtev->i1 = KEY_F(8);	break;
		    case XK_F9:		vtev->i1 = KEY_F(9);	break;
		    case XK_F10:	vtev->i1 = KEY_F(10);	break;
		    case XK_F11:	vtev->i1 = KEY_F(11);	break;
		    case XK_F12:	vtev->i1 = KEY_F(12);	break;
		}
	    if (vtev->i1)
		xw->handler(xw->data, vtev);
	    break;
	}
	case ButtonPress:
	{
	    xw->tstamp = ev->xkey.time;
	    ev->xbutton.x /= CW;
	    ev->xbutton.y /= CH;
	    if (ev->xbutton.button == Button1)
		sel_start(xw, ev->xbutton.x, ev->xbutton.y, ev->xbutton.time);
	    break;
	}
	case MotionNotify:
	{
	    xw->tstamp = ev->xkey.time;
	    ev->xmotion.x /= CW;
	    ev->xmotion.y /= CH;
	    if (ev->xmotion.state & Button1Mask)
		sel_move(xw, ev->xmotion.x, ev->xmotion.y, ev->xmotion.time);
	    break;
	}
	case ButtonRelease:
	{
	    xw->tstamp = ev->xkey.time;
	    ev->xbutton.x /= CW;
	    ev->xbutton.y /= CH;
	    if (ev->xbutton.button == Button1)
		if (sel_end(xw, ev->xbutton.x, ev->xbutton.y, ev->xbutton.time))
		    break;

	    vtev->type = EV_MOUSE;
	    vtev->i1 = ev->xbutton.button;
	    vtev->i2 = (ev->xbutton.state & ShiftMask) != 0;
	    vtev->i3 = ev->xbutton.x;
	    vtev->i4 = ev->xbutton.y;
	    if (vtev->i3 >= 0 && vtev->i3 < W && vtev->i4 >= 0 && vtev->i4 < H)
		xw->handler(xw->data, vtev);
	    break;
	}
	case SelectionClear:
	{
	// may be our own Owner=None due to sel_start
	    if (xw->sel_set_t && ev->xselectionclear.time >= xw->sel_set_t)
	    {
		xw->sel_set_t = 0; // no need to reset owner
		sel_abort(xw);
	    }
	    break;
	}
	case SelectionRequest:
	{
	    sel_send(xw, &ev->xselectionrequest);
	    break;
	}
	case SelectionNotify:
	{
	    sel_retrieve(xw, ev->xselection.requestor, ev->xselection.property, True);
	    break;
	}
	default:
		break;
    }
}


static void switch_blink_state(struct xio_win *xw)
{
    xw->blink_on = !xw->blink_on;
    xw->modified |= xw->blink;
    dirty(xw, xw->curs_y, xw->curs_y + 1);
}


static void send_timer_event(struct xio_win *xw)
{
    struct vt_event vtev[1];
    vtev->type = EV_TIMER;
    xw->handler(xw->data, vtev);
}


static void xio_timer(void *data, int fd)
{
    char buf[64];
    read(fd, buf, sizeof(buf));
    for_all_windows(switch_blink_state, 0);
    for_all_windows(send_timer_event, 0);
}


void xio_event_loop(void)
{
    struct xio *xio, *vtn;
    int f;

    while (not dl_empty(dpys))
    {
	do
	{
	    for_all_windows(xio_update_win, 0);
	    f = 0;
	    for (xio = PTR dpys->first; vtn = PTR xio->node->next; xio = vtn)
		while (XPending(xio->dpy))
		{
		    handle_event(xio, xio->fd);
		    f++;
		}
	} while (f);
	fdset_select(fds, -1);
    }
}
