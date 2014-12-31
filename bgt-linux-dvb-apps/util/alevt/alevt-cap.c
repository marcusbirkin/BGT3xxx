#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include "vt.h"
#include "misc.h"
#include "fdset.h"
#include "vbi.h"
#include "lang.h"
#include "dllist.h"
#include "export.h"

static volatile int timed_out = 0;
static char *channel;
char *outfile = "";
u_int16_t sid;


struct req
{
    struct dl_node node[1];
    char *name; // file name
    char *pgno_str; // the pgno as given on the cmdline
    int pgno, subno; // decoded pgno
    struct export *export; // export data
    struct vt_page vtp[1]; // the capture page data
};


static void usage(FILE *fp, int exitval)
{
    fprintf(fp, "\nUsage: %s [options] ppp.ss...\n", prgname);
    fprintf(fp,
	    "\n"
	    "  Valid options:\t\tDefault:\n"
	    "    -cs -charset\t\tlatin-1\n"
	    "    <latin-1/2/koi8-r/iso8859-7>\n"
	    "    -f -format <fmt,options>\tascii\n"
	    "    -f help -format help\n"
	    "    -h -help\n"
	    "    -n -name <filename>\t\tttext-%%s.%%e\n"
	    "    -s -sid <sid>\t\t(none;dvb only)\n"
	    "    -to -timeout <secs>\t\t(none)\n"
	    "    -t -ttpid <ttpid>\t\t(none;dvb only)\n"
	    "    -v -vbi <vbidev>\t\t/dev/vbi\n"
		"                 \t\t/dev/vbi0\n"
		"                 \t\t/dev/video0\n"
		"                 \t\t/dev/dvb/adapter0/demux0\n"
	    "\n"
	    "  ppp.ss stands for a page number and an\n"
	    "  optional subpage number (ie 123.4).\n"
	);
    exit(exitval);
}


static void exp_help(FILE *fp)
{
    struct export_module **ep;
    char **cp, c;

    fprintf(fp,
	    "\nSyntax: -format Name[,Options]\n"
	    "\n"
	    "    Name\tExt.\tOptions\n"
	    "    --------------------------------\n"
	);
    for (ep = modules; *ep; ep++)
    {
	fprintf(fp, "    %-7s\t.%-4s", (*ep)->fmt_name, (*ep)->extension);
	for (c = '\t', cp = (*ep)->options; cp && *cp; cp++, c = ',')
	    fprintf(fp, "%c%s", c, *cp);
	fprintf(fp, "\n");
    }
    fprintf(fp,
	    "\n"
	    "Common options: reveal,hide\n"
	    "Example: -format ansi,reveal,bg=none\n"
	    "\n"
	);
    exit(0);
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
	    if (pgno >= 0x100 && pgno <= 0x899)
		if (*subno == ANY_SUB || (*subno >= 0x00 && *subno <= 0x3f7f))
		    return pgno;
    }
    fatal("%s: invalid page number", p);
}


static int option(int argc, char **argv, int *ind, char **arg)
{
    static struct { char *nam, *altnam; int arg; } opts[] = {
	{ "-charset", "-cs", 1 },
	{ "-format", "-f", 1 },
	{ "-help", "-h", 0 },
	{ "-name", "-n", 1 },
	{ "-sid", "-s", 1 },
	{ "-timeout", "-to", 1 },
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
	usage(stderr, 2);
    }

    return -1;
}


static void event(struct dl_head *reqs, struct vt_event *ev)
{
    struct req *req, *nxt;

    switch (ev->type)
    {
	case EV_PAGE: // new page
	{
	    struct vt_page *vtp = ev->p1;

	    for (req = PTR reqs->first; nxt = PTR req->node->next; req = nxt)
		if (req->pgno == vtp->pgno)
		    if (req->subno == ANY_SUB || req->subno == vtp->subno)
		    {
			*req->vtp = *vtp;
			dl_insert_last(reqs + 1, dl_remove(req->node));
	    }
	}
    }
}


int main(int argc, char **argv)
{
    char *vbi_name = NULL;
    int timeout = 0;
    char *fname = "ttext-%s.%e";
    char *out_fmt = "ascii";
    struct export *fmt = 0;
    int opt, ind;
    char *arg;
    struct vbi *vbi;
    struct req *req;
    struct dl_head reqs[2]; // simple linear lists of requests & captures
    int ttpid = -1;

    setlocale (LC_CTYPE, "");
    setprgname(argv[0]);

    fdset_init(fds);
    dl_init(reqs); // the requests
    dl_init(reqs+1); // the captured pages

    ind = 1;
    while (opt = option(argc, argv, &ind, &arg))
	switch (opt)
	{
            case 1: // charset
		if (streq(arg, "latin-1") || streq(arg, "1"))
		    latin1 = 1;
		else if (streq(arg, "latin-2") || streq(arg, "2"))
		    latin1 = 0;
		else if (streq(arg, "koi8-r") || streq(arg, "koi"))
		    latin1 = KOI8;
		else if (streq(arg, "iso8859-7") || streq(arg, "el"))
		    latin1 = GREEK;
		else
		    fatal("bad charset (not latin-1/2/koi8-r/iso8859-7)");
		break;
	    case 2: // format
		if (streq(arg, "help") || streq(arg, "?") || streq(arg, "list"))
		    exp_help(stdout);
		out_fmt = arg;
		fmt = 0;
		break;
	    case 3: // help
		usage(stdout, 0);
		break;
	    case 4: // name
		fname = arg;
		break;
	    case 5: // timeout
		timeout = strtol(arg, 0, 10);
		if (timeout < 1 || timeout > 999999)
		fatal("bad timeout value", timeout);
		break;
	    case 6: // service id
		sid = strtoul(arg, NULL, 0);
		break;
	    case 7: // teletext pid
		ttpid = strtoul(arg, NULL, 0);
		break;
	    case 8: // vbi
		vbi_name = arg;
		break;
	    case -1: // non-option arg
		if (not fmt)
		fmt = export_open(out_fmt);
		if (not fmt)
		fatal("%s", export_errstr());
		if (not(req = malloc(sizeof(*req))))
		out_of_mem(sizeof(*req));
		req->name = fname;
		req->pgno_str = arg;
		req->pgno = arg_pgno(arg, &req->subno);
		req->export = fmt;
		dl_insert_last(reqs, req->node);
		break;
	}

    if (dl_empty(reqs))
	fatal("no pages requested");

    // setup device
    if (not(vbi = vbi_open(vbi_name, 0, channel, outfile, sid, ttpid)))
	fatal("cannot open %s", vbi_name);
    vbi_add_handler(vbi, event, reqs); // register event handler

    if (timeout)
	alarm(timeout);

    // capture pages (moves requests from reqs[0] to reqs[1])
    while (not dl_empty(reqs) && not timed_out)
	if (fdset_select(fds, 30000) == 0) // 30sec select time out
	{
	    error("no signal.");
	    break;
	}

    alarm(0);
    vbi_del_handler(vbi, event, reqs);
    vbi_close(vbi);
    if (not dl_empty(reqs))
	error("capture aborted. Some pages are missing.");

    for (req = PTR reqs[1].first; req->node->next; req = PTR req->node->next)
    {
	fname = export_mkname(req->export, req->name, req->vtp, req->pgno_str);
	if (not fname || export(req->export, req->vtp, fname))
	    error("error saving page %s: %s", req->pgno_str, export_errstr());
	if (fname)
	    free(fname);
    }
    exit(dl_empty(reqs) ? 0 : 1);
}
