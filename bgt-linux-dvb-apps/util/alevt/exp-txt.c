#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "os.h"
#include "export.h"

static int txt_open(struct export *e);
static int txt_option(struct export *e, int opt, char *arg);
static int txt_output(struct export *e, char *name, struct fmt_page *pg);
static char *txt_opts[] =	// module options
{
    "color",			// generate ansi color codes (and attributes)
    "gfx-chr=<char>",		// substitute <char> for gfx-symbols
    "fg=<0-7|none>",		// assume term has <x> as foreground color
    "bg=<0-7|none>",		// assume term has <x> as background color
    "lines=<1-25>",		// output 24 or 25 lines
    0
};


struct txt_data			// private data in struct export
{
    u8 color;
    u8 gfx_chr;
    u8 def_fg;
    u8 def_bg;
    int endline;
    struct fmt_char curr[1];
    FILE *fp;
};


struct export_module export_txt =	// exported module definition
{
    "ascii",			// id
    "txt",			// extension
    txt_opts,			// options
    sizeof(struct txt_data),	// data size
    txt_open,			// open
    0,				// close
    txt_option,			// option
    txt_output,			// output
};


struct export_module export_ansi =	// exported module definition
{
    "ansi",			// id
    "txt",			// extension
    txt_opts,			// options
    sizeof(struct txt_data),	// data size
    txt_open,			// open
    0,				// close
    txt_option,			// option
    txt_output,			// output
};

#define D  ((struct txt_data *)e->data)


char * my_stpcpy(char *dst, const char *src)
{
    while (*dst = *src++)
	dst++;
    return dst;
}


static int txt_open(struct export *e)
{
    D->gfx_chr = '#';
    D->def_fg = -1;
    D->def_bg = -1;
    D->endline = H;
    if (e->mod == &export_ansi)
	D->color = 1;
    return 0;
}


static int txt_option(struct export *e, int opt, char *arg)
{
    switch (opt)
    {
	case 1: // color
	    D->color = 1;
	    break;
	case 2: // gfx-chr=
	    D->gfx_chr = *arg ?: ' ';
	    break;
	case 3: // fg=
	    D->def_fg = *arg - '0';
	    break;
	case 4: // bg=
	    D->def_bg = *arg - '0';
	    break;
	case 5: // lines=
	    D->endline = atoi(arg);
	    if (D->endline < 1 || D->endline > H)
	    {
		export_error("lines: invalid number");
		return 1;
	    }
    }
    return 0;
}


static void put_attr(struct export *e, struct fmt_char *new)
{
    char buf[512];
    char *p = buf;
    int fg, bg, attr;
    int reset = 0;

    if (D->color)
    {
	fg = D->curr->fg ^ new->fg;
	bg = D->curr->bg ^ new->bg;
	attr = (D->curr->attr ^ new->attr) & (EA_BLINK | EA_DOUBLE);

	if (fg | bg | attr)
	{
	    if (~new->attr & attr)	// reset some attributes ->  reset all.
		reset = 1;
	    if (fg && new->fg == D->def_fg)	// switch to def fg -> reset all
		reset = 1;
	    if (bg && new->bg == D->def_bg)	// switch to def bg -> reset all
		reset = 1;

	    p = my_stpcpy(buf, "\e[");
	    if (reset)
	    {
		p = my_stpcpy(p, ";");		// "0;" but 0 isn't neccesary
		attr = -1;			// set all attributes
		fg = new->fg ^ D->def_fg;	// set fg if != default fg
		bg = new->bg ^ D->def_bg;	// set bg if != default bg
	    }
	    if (attr & new->attr & EA_BLINK)
		p = my_stpcpy(p, "5;");			// blink
	    if (attr & new->attr & EA_DOUBLE)
		p = my_stpcpy(p, "1;");			// bold
	    if (fg)
		p += sprintf(p, "%d;", new->fg + 30);	// fg-color
	    if (bg)
		p += sprintf(p, "%d;", new->bg + 40);	// bg-color
	    p[-1] = 'm';	// replace last ;
	    *D->curr = *new;
	}
    }
    *p++ = new->ch;
    *p = 0;
    fputs(buf, D->fp);
}


static int txt_output(struct export *e, char *name, struct fmt_page *pg)
{
    struct fmt_char def_c[1];
    struct fmt_char l[W+2];
    #define L (l+1)
    int x, y;

    D->fp = fopen(name, "w");
    if (not D->fp)
    {
	export_error("cannot create file");
	return -1;
    }

    /* initialize default colors. These have to be restored at EOL. */
    def_c->ch = '\n';
    def_c->fg = D->def_fg;
    def_c->bg = D->def_bg;
    def_c->attr = E_DEF_ATTR;
    *D->curr = *def_c;
    L[-1] = L[W] = *def_c;

    for (y = 0; y < D->endline; y++)
	if (~pg->hid & (1 << y))	// not hidden
	{
	    // character conversion
	    for (x = 0; x < W; ++x)
	    {
		struct fmt_char c = pg->data[y][x];

		switch (c.ch)
		{
		    case 0x00: case 0xa0:		c.ch = ' '; break;
		    case 0x7f:				c.ch = '*'; break;
		    case BAD_CHAR:			c.ch = '?'; break;
		    default:
			if (c.attr & EA_GRAPHIC)
			    c.ch = D->gfx_chr;
			break;
		}
		L[x] = c;
	    }

	    if (D->color)
	    {
		// optimize color and attribute changes
		// delay fg and attr changes as far as possible
		for (x = 0; x < W; ++x)
		    if (L[x].ch == ' ')
		    {
			L[x].fg = L[x-1].fg;
			l[x].attr = L[x-1].attr;
		    }

		// move fg and attr changes to prev bg change point
		for (x = W-1; x >= 0; x--)
		    if (L[x].ch == ' ' && L[x].bg == L[x+1].bg)
		    {
			L[x].fg = L[x+1].fg;
			L[x].attr = L[x+1].attr;
		    }
	    }

	    // now emit the whole line (incl EOL)
	    for (x = 0; x < W+1; ++x)
		put_attr(e, L + x);
	}
    fclose(D->fp);
    return 0;
}
