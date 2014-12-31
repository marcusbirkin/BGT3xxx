#ifndef VTXIO_H
#define VTXIO_H

#include <X11/Xlib.h>
#include "vt.h"
#include "dllist.h"

typedef u32 lbits;
#define ALL_LINES ((1ul << H) - 1)

/* one xio per display */
struct xio
{
    struct dl_node node[1];
    int argc;
    char **argv;
    Display *dpy;			/* display connection */
    int fd;				/* the displays file descriptor */
    Atom xa_del_win;			/* WM_DELETE_WINDOW atom */
    Atom xa_targets;			/* TARGETS atom (selection) */
    Atom xa_timestamp;			/* TIMESTAMP atom (selection) */
    Atom xa_text;			/* TEXT atom (selection) */
    Atom xa_multiple;			/* MULTIPLE atom (selection) */
    Window group_leader;		/* unmapped window */
    int screen;				/* DefaultScreen */
    int width, height;			/* DisplayWidth/Height */
    int depth;				/* DefaultDepth */
    Window root;			/* DefaultRoot */
    Colormap cmap;
    int color[16];			/* 8 normal, 8 dim intensity */
    Pixmap font[2];			/* normal, dbl-height */
    Pixmap icon;			/* icon pixmap */
    struct dl_head windows[1];		/* all windows on this display */
};

/* one vt_win per window */
struct xio_win
{
    struct dl_node node[1];
    struct xio *xio;			/* display */
    Window win;				/* the drawing window */
    Time tstamp;			/* timestamp of last user event */
    GC gc;				/* it's graphics context */
    u8 ch[H*W];				/* the page contents */
    lbits modified, hidden, lhidden;	/* states for each line */
    lbits dheight, blink, concealed;	/* attributes for each line */
    int fg, bg;				/* current foreground/background */
    int blink_on;			/* blinking on */
    int reveal;				/* reveal concealed text */
    void (*handler)(void *data, struct vt_event *ev); /* event-handler */
    void *data;				/* data for the event-handler */
    int curs_x, curs_y;			/* cursor position */
    u8 title[32];			/* the user title */
    // selection support
    int sel_start_x, sel_start_y;
    Time sel_start_t;
    Time sel_set_t;			/* time we got selection owner */
    int sel_x1, sel_y1, sel_x2, sel_y2;	/* selected area */
    Pixmap sel_pixmap;			/* for pixmap-selection requests */
};

struct xio *xio_open_dpy(char *dpy, int argc, char **argv);
struct xio_win *xio_open_win(struct xio *xio, char *geom);
void xio_close_win(struct xio_win *xw, int dpy_too);
void xio_close_dpy(struct xio *xio);
void xio_set_handler(struct xio_win *xw, void *handler, void *data);
void xio_clear_win(struct xio_win *xw);
void xio_put_line(struct xio_win *xw, int line, u8 *data);
void xio_put_str(struct xio_win *xw, int line, u8 *c_str);
int xio_get_line(struct xio_win *xw, int line, u8 *data);
int xio_set_concealed(struct xio_win *xw, int on);
void xio_update_win(struct xio_win *xw);
void xio_fd_handler(int fd, void *handler, void *data);
void xio_cancel_selection(struct xio_win *xw);
void xio_query_selection(struct xio_win *xw);
void xio_set_selection(struct xio_win *xw, int x1, int y1, int x2, int y2);
void xio_set_cursor(struct xio_win *xw, int x, int y);
void xio_event_loop(void);
void xio_title(struct xio_win *xw, char *title);
#endif
