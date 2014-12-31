#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vt.h"
#include "misc.h"
#include "export.h"

extern struct export_module export_txt;
extern struct export_module export_ansi;
extern struct export_module export_html;
extern struct export_module export_png;
extern struct export_module export_ppm;
struct export_module *modules[] =
{
    &export_txt,
    &export_ansi,
    &export_html,
    &export_ppm,
#ifdef WITH_PNG
    &export_png,
#endif
    0
};


static char *glbl_opts[] =
{
    "reveal",		// show hidden text
    "hide",		// don't show hidden text (default)
    0
};

static char errbuf[64];


void export_error(char *str, ...)
{
    va_list args;

    va_start(args, str);
    vsnprintf(errbuf, sizeof(errbuf)-1, str, args);
}


char * export_errstr(void)
{
    return errbuf;
}


static int find_opt(char **opts, char *opt, char *arg)
{
    int err = 0;
    char buf[256];
    char **oo, *o, *a;

    if (oo = opts)
	while (o = *oo++)
	{
	    if (a = strchr(o, '='))
	    {
		a = buf + (a - o);
		o = strcpy(buf, o);
		*a++ = 0;
	    }
	    if (strcasecmp(o, opt) == 0)
	    {
		if ((a != 0) == (arg != 0))
		    return oo - opts;
		err = -1;
	    }
	}
    return err;
}


struct export * export_open(char *fmt)
{
    struct export_module **eem, *em;
    struct export *e;
    char *opt, *optend, *optarg;
    int opti;

    if (fmt = strdup(fmt))
    {
	if (opt = strchr(fmt, ','))
	    *opt++ = 0;
	for (eem = modules; em = *eem; eem++)
	    if (strcasecmp(em->fmt_name, fmt) == 0)
		break;
	if (em)
	{
	    if (e = malloc(sizeof(*e) + em->local_size))
	    {
		e->mod = em;
		e->fmt_str = fmt;
		e->reveal = 0;
		memset(e + 1, 0, em->local_size);
		if (not em->open || em->open(e) == 0)
		{
		    for (; opt; opt = optend)
		    {
			if (optend = strchr(opt, ','))
			    *optend++ = 0;
			if (not *opt)
			    continue;
			if (optarg = strchr(opt, '='))
			    *optarg++ = 0;
			if ((opti = find_opt(glbl_opts, opt, optarg)) > 0)
			{
			    if (opti == 1) // reveal
				e->reveal = 1;
			    else if (opti == 2) // hide
				e->reveal = 0;
			}
			else if (opti == 0 &&
				(opti = find_opt(em->options, opt, optarg)) > 0)
			{
			    if (em->option(e, opti, optarg))
				break;
			}
			else
			{
			    if (opti == 0)
				export_error("%s: unknown option", opt);
			    else if (optarg)
				export_error("%s: takes no arg", opt);
			    else
				export_error("%s: missing arg", opt);
			    break;
			}
		    }
		    if (opt == 0)
			return e;

		    if (em->close)
			em->close(e);
		}
		free(e);
	    }
	    else
		export_error("out of memory");
	}
	else
	    export_error("unknown format: %s", fmt);
	free(fmt);
    }
    else
	export_error("out of memory");
    return 0;
}


void export_close(struct export *e)
{
    if (e->mod->close)
	e->mod->close(e);
    free(e->fmt_str);
    free(e);
}


static char * hexnum(char *buf, unsigned int num)
{
    char *p = buf + 5;

    num &= 0xffff;
    *--p = 0;
    do
    {
	*--p = "0123456789abcdef"[num % 16];
	num /= 16;
    } while (num);
    return p;
}


static char * adjust(char *p, char *str, char fill, int width)
{
    int l = width - strlen(str);

    while (l-- > 0)
	*p++ = fill;
    while (*p = *str++)
	p++;
    return p;
}


char * export_mkname(struct export *e, char *fmt, struct vt_page *vtp, char *usr)
{
    char bbuf[1024];
    char *p = bbuf;

    while (*p = *fmt++)
	if (*p++ == '%')
	{
	    char buf[32], buf2[32];
	    int width = 0;

	    p--;
	    while (*fmt >= '0' && *fmt <= '9')
		width = width*10 + *fmt++ - '0';

	    switch (*fmt++)
	    {
		case '%':
		    p = adjust(p, "%", '%', width);
		    break;
		case 'e':	// extension
		    p = adjust(p, e->mod->extension, '.', width);
		    break;
		case 'p':	// pageno[.subno]
		    if (vtp->subno)
			p = adjust(p,strcat(strcat(hexnum(buf, vtp->pgno),
				"."), hexnum(buf2, vtp->subno)), ' ', width);
		    else
			p = adjust(p, hexnum(buf, vtp->pgno), ' ', width);
		    break;
		case 'S':	// subno
		    p = adjust(p, hexnum(buf, vtp->subno), '0', width);
		    break;
		case 'P':	// pgno
		    p = adjust(p, hexnum(buf, vtp->pgno), '0', width);
		    break;
		case 's':	// user strin
		    p = adjust(p, usr, ' ', width);
		    break;
		//TODO: add date, channel name, ...
	    }
	}
    p = strdup(bbuf);
    if (not p)
	export_error("out of memory");
    return p;
}


static void fmt_page(struct export *e, struct fmt_page *pg, struct vt_page *vtp)
{
    char buf[16];
    int x, y;
    u8 *p = vtp->data[0];

    pg->dbl = 0;

    sprintf(buf, "\2%x.%02x\7", vtp->pgno, vtp->subno & 0xff);

    for (y = 0; y < H; y++)
    {
	struct fmt_char c;
	int last_ch = ' ';
	int dbl = 0, hold = 0;

	c.fg = 7;
	c.bg = 0;
	c.attr = 0;

	for (x = 0; x < W; ++x)
	{
	    c.ch = *p++;
	    if (y == 0 && x < 8)
		c.ch = buf[x];
	    switch (c.ch)
	    {
		case 0x00 ... 0x07:	/* alpha + fg color */
		    c.fg = c.ch & 7;
		    c.attr &= ~(EA_GRAPHIC | EA_CONCEALED);
		    goto ctrl;
		case 0x08:		/* flash */
		    c.attr |= EA_BLINK;
		    goto ctrl;
		case 0x09:		/* steady */
		    c.attr &= ~EA_BLINK;
		    goto ctrl;
		case 0x0a:		/* end box */
		case 0x0b:		/* start box */
		    goto ctrl;
		case 0x0c:		/* normal height */
		    c.attr &= EA_DOUBLE;
		    goto ctrl;
		case 0x0d:		/* double height */
		    if (y < H-2)	/* ignored on last 2 lines */
		    {
			c.attr |= EA_DOUBLE;
			dbl = 1;
		    }
		    goto ctrl;
		case 0x10 ... 0x17:	/* gfx + fg color */
		    c.fg = c.ch & 7;
		    c.attr |= EA_GRAPHIC;
		    c.attr &= ~EA_CONCEALED;
		    goto ctrl;
		case 0x18:		/* conceal */
		    c.attr |= EA_CONCEALED;
		    goto ctrl;
		case 0x19:		/* contiguous gfx */
		    c.attr &= ~EA_SEPARATED;
		    goto ctrl;
		case 0x1a:		/* separate gfx */
		    c.attr |= EA_SEPARATED;
		    goto ctrl;
		case 0x1c:		/* black bg */
		    c.bg = 0;
		    goto ctrl;
		case 0x1d:		/* new bg */
		    c.bg = c.fg;
		    goto ctrl;
		case 0x1e:		/* hold gfx */
		    hold = 1;
		    goto ctrl;
		case 0x1f:		/* release gfx */
		    hold = 0;
		    goto ctrl;

		case 0x0e:		/* SO */
		case 0x0f:		/* SI */
		case 0x1b:		/* ESC */
		    c.ch = ' ';
		    break;

		ctrl:
		    c.ch = ' ';
		    if (hold && (c.attr & EA_GRAPHIC))
			c.ch = last_ch;
		    break;
	    }
	    if (c.attr & EA_GRAPHIC)
		if ((c.ch & 0xa0) == 0x20)
		{
		    last_ch = c.ch;
		    c.ch += (c.ch & 0x40) ? 32 : -32;
		}
	    if (c.attr & EA_CONCEALED)
		if (not e->reveal)
		    c.ch = ' ';
	    pg->data[y][x] = c;
	}
	if (dbl)
	{
	    pg->dbl |= 1 << y;
	    for (x = 0; x < W; ++x)
	    {
		if (~pg->data[y][x].attr & EA_DOUBLE)
		    pg->data[y][x].attr |= EA_HDOUBLE;
		pg->data[y+1][x] = pg->data[y][x];
		pg->data[y+1][x].ch = ' ';
	    }
	    y++;
	    p += W;
	}
    }
    pg->hid = pg->dbl << 1;
}


int export(struct export *e, struct vt_page *vtp, char *name)
{
    struct fmt_page pg[1];

    fmt_page(e, pg, vtp);
    return e->mod->output(e, name, pg);
}
