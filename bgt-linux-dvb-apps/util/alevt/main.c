#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "vt.h"
#include "misc.h"
#include "fdset.h"
#include "xio.h"
#include "vbi.h"
#include "lang.h"
#include "cache.h"
#include "ui.h"

static char *geometry;
static char *dpy_name;
static char *vbi_name = NULL;
static struct xio *xio;
static struct vbi *vbi;
static int erc = 1;
char *outfile = "";
static char *channel;
static int ttpid = -1;
u_int16_t sid = 0;


static void usage(FILE *fp, int exitval)
{
    fprintf(fp, "\nUsage: %s [options]\n", prgname);
    fprintf(fp,
	    "\n"
	    "  Valid options:\t\tDefault:\n"
	    "    -c <channel name>\t\t(none;dvb only)\n"
	    "    -ch -child <ppp.ss>\t\t(none)\n"
	    "    -cs -charset\t\tlatin-1\n"
	    "    <latin-1/2/koi8-r/iso8859-7>\n"
	    "    -h -help\n"
	    "    -o <outfile>\t\t(none;dvb only)\n"
	    "    -p -parent <ppp.ss>\t\t900\n"
	    "    -s -sid <sid>\t\t(none;dvb only)\n"
	    "    -t -ttpid <ttpid>\t\t(none;dvb only)\n"
	    "    -v -vbi <vbidev>\t\t/dev/vbi\n"
		"                 \t\t/dev/vbi0\n"
		"                 \t\t/dev/video0\n"
		"                 \t\t/dev/dvb/adapter0/demux0\n"
	    "\n"
	    "  ppp.ss stands for a page number and an\n"
	    "  optional subpage number (Example: 123.4).\n"
	    "\n"
	    "  The -child option requires a parent\n"
	    "  window. So it must be preceded by\n"
	    "  a parent or another child window.\n"
	);
    exit(exitval);
}


static int arg_pgno(char *p, int *subno)
{
    char *end;
    int pgno;

    *subno = ANY_SUB;
    if (*p)
    {
	pgno = strtol(p, &end, 16);
	if ((*end == ':' || *end == '/' || *end == '.') && end[1])
	    *subno = strtol(end + 1, &end, 16);
	if (*end == 0)
	    if (pgno >= 0x100 && pgno <= 0x999)
		if (*subno == ANY_SUB || (*subno >= 0x00 && *subno <= 0x3f7f))
		    return pgno;
    }
    fatal("%s: invalid page number", p);
}


static struct vtwin * start(int argc, char **argv, struct vtwin *parent,
	int pgno, int subno)
{
    static int valid_vbi_name = 1;

    if (!valid_vbi_name)
	return parent;

    if (vbi == 0)
	vbi = vbi_open(vbi_name, cache_open(), channel, outfile, sid, ttpid);
    if (vbi == 0)
    {
	if (vbi_name)
	    error("cannot open device: %s", vbi_name);
    	valid_vbi_name = 0;
    	vbi = open_null_vbi(cache_open());
    }
    if (vbi->cache)
	vbi->cache->op->mode(vbi->cache, CACHE_MODE_ERC, erc);

    if (xio == 0)
	xio = xio_open_dpy(dpy_name, argc, argv);
    if (xio == 0)
	fatal("cannot open display");

    parent = vtwin_new(xio, vbi, geometry, parent, pgno, subno);
    if (parent == 0)
	fatal("cannot create window");

    if (!valid_vbi_name)
    {
	if (vbi_name)
	    send_errmsg(vbi, "cannot open device: %s", vbi_name);
	else
	    send_errmsg(vbi, "cannot open any device", vbi_name);
    }

    return parent;
}


static int option(int argc, char **argv, int *ind, char **arg)
{
    static struct { char *nam, *altnam; int arg; } opts[] = {
	{ "-channel", "-c", 1 },
	{ "-child", "-ch", 1 },
	{ "-charset", "-cs", 1 },
	{ "-help", "-h", 0 },
	{ "-outfile", "-o", 1 },
	{ "-parent", "-p", 1 },
	{ "-sid", "-s", 1 },
	{ "-ttpid", "-t", 1 },
	{ "-vbi", "-v", 1 },
    };
    int i;
    if (*ind >= argc)
	return 0;
    *arg = argv[(*ind)++];
    for (i = 0; i < NELEM(opts); ++i)
	if (streq(*arg, opts[i].nam) || streq(*arg, opts[i].altnam))
	{
	    if (opts[i].arg)
		if (*ind < argc)
		    *arg = argv[(*ind)++];
		else
		    fatal("option %s requires an argument", *arg);
	    return i+1;
	}
    if (**arg == '-')
    {
	fatal("%s: invalid option", *arg);
	usage(stderr, 1);
    }
    return -1;
}


int main(int argc, char **argv)
{
    struct vtwin *parent = 0;
    int pgno, subno;
    int opt, ind;
    char *arg;
    sid = 0;

    setprgname(argv[0]);
    fdset_init(fds);

    ind = 1;
    while (opt = option(argc, argv, &ind, &arg))
	switch (opt)
	{

	    case 1: // channel
		channel = arg;
		break;
	    case 2: // child
		if (parent == 0)
		    fatal("-child requires a parent window");
		pgno = arg_pgno(arg, &subno);
		parent = start(argc, argv, parent, pgno, subno);
		geometry = 0;
		break;
	    case 3: // charset
		if (streq(arg, "latin-1") || streq(arg, "1"))
		    latin1 = LATIN1;
		else if (streq(arg, "latin-2") || streq(arg, "2"))
		    latin1 = LATIN2;
		else if (streq(arg, "koi8-r") || streq(arg, "koi"))
		    latin1 = KOI8;
		else if (streq(arg, "iso8859-7") || streq(arg, "el"))
		    latin1 = GREEK;
		else
		    fatal("bad charset (not latin-1/2/koi8-r/iso8859-7)");
		break;
	    case 4: // help
		usage(stdout, 0);
		break;
	    case 5: // outfile
		outfile = arg;
		break;
	    case 6: // parent
	    case -1: // non-option arg
		pgno = arg_pgno(arg, &subno);
		parent = start(argc, argv, 0, pgno, subno);
		geometry = 0;
		break;
	    case 7: // sid
		sid = strtoul(arg, NULL, 0);
		break;
	    case 8: // ttpid
		ttpid = strtoul(arg, NULL, 0);
		break;
	    case 9: // vbi
		vbi_name = arg;
		vbi = 0;
		parent = 0;
		break;
	}

    if (parent == 0)
    start(argc, argv, 0, 0x900, ANY_SUB);
    xio_event_loop();
    exit(0);
}
