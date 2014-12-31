#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "os.h"
#include "vt.h"
#include "fdset.h"
#include "vbi.h"
#include "lang.h"
#include "misc.h"

char *fmt = "%a %b %d %H:%M:%S %Z %Y";
int max_diff = 2*60*60; // default: 2 hours
int set_time = 0;
char *outfile = "";
static char *channel;
u_int16_t sid;


static void chk_time(int t)
{
    struct tm *tm;
    time_t sys_t;
    int dt;
    char buf[256];

    if (t < 0 || t > 235959 || t%100 > 59 || t/100%100 > 59)
    return;

    sys_t = time(0);
    tm = localtime(&sys_t);

    // Now convert to UTC seconds
    t = t/100/100 * 60*60 + t/100%100 * 60 + t%100;
#ifdef BSD
    t -= tm->tm_gmtoff; // dst already included...
#else
    t += timezone;
    if (tm->tm_isdst)
	t -= 60*60;
#endif

    dt = t - sys_t % (24*60*60);
    if (dt <= -12*60*60)
	dt += 24*60*60;

    if (dt <= -max_diff || dt >= max_diff)
	fatal("time diff too big (%2d:%02d:%02d)", dt/60/60, abs(dt)/60%60, abs(dt)%60);

    sys_t += dt;

    if (set_time)
    {
	struct timeval tv[1];

	tv->tv_sec = sys_t;
	tv->tv_usec = 500000;
	if (settimeofday(tv, 0) == -1)
	    ioerror("settimeofday");
    }
    if (*fmt)
    {
	tm = localtime(&sys_t);
	if (strftime(buf, sizeof(buf), fmt, tm))
	    puts(buf);
    }
    exit(0);
}


static void event(void *_, struct vt_event *ev)
{
    switch (ev->type)
    {
	/* vbi may generate EV_PAGE, EV_HEADER, EV_XPACKET */
	/* for event arguments see vt.h */

	case EV_HEADER: // a new title line (for running headers)
	{
	    static int last_t = -1;
	    u8 *s = ev->p1;
	    int i, t = 1;

	    if (ev->i2 & PG_OUTOFSEQ)
		break;

	    for (i = 32; i < 40; ++i)
		if (s[i] >= '0' && s[i] <= '9')
		    t = t * 10+ s[i] - '0';
	    if (t >= 1000000 && t <= 1235959)
		if (t == last_t || t - last_t == 1)
		    chk_time(t - 1000000);
	    last_t = t;
	    break;
	}
    }
}


static void usage(FILE *fp, int exit_val)
{
    fprintf(fp, "usage: %s [options]\n", prgname);
    fprintf(fp,
	    "\n"
	    "  Valid options:\t\tDefault:\n"
	    "    -d -delta <max_secs>\t7200 (2 hours)\n"
	    "    -f -format <fmtstr>\t\t%%c\n"
	    "    -h -help\n"
	    "    -s -set\t\t\toff\n"
	    "    -to -timeout <seconds>\t(none)\n"
	    "    -v -vbi <vbidev>\t\t/dev/vbi\n"
		"                 \t\t/dev/vbi0\n"
		"                 \t\t/dev/video0\n"
		"                 \t\t/dev/dvb/adapter0/demux0\n"
	    );
    exit(exit_val);
}


static int option(int argc, char **argv, int *ind, char **arg)
{
    static struct { char *nam, *altnam; int arg; } opts[] = {
	{ "-delta", "-d", 1 },
	{ "-format", "-f", 1 },
	{ "-help", "-h", 0 },
	{ "-set", "-s", 0 },
	{ "-timeout", "-to", 1 },
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
    char *vbi_name = NULL;
    int timeout = 0;
    struct vbi *vbi;
    int opt, ind;
    char *arg;
    int ttpid = -1;

    setprgname(argv[0]);
    ind = 1;
    while (opt = option(argc, argv, &ind, &arg))
	switch (opt)
	{
	    case 1: // -delta
		max_diff = atoi(arg);
		if (max_diff < 1)
		    fatal("-delta: illegal value '%s'", arg);
		if (max_diff > 12*60*60)
		{
		    max_diff = 12*60*60;
		    error("-delta: %d too big. Assuming %d", arg, max_diff);
		}
		break;
	    case 2: // -format
		fmt = arg;
		break;
	    case 3: // help
		usage(stdout, 0);
		break;
	    case 4: // -set
		set_time = 1;
		break;
	    case 5: // -timeout
		timeout = atoi(arg);
		if (timeout < 1 || timeout > 60*60)
		    fatal("-timeout: illegal value '%s'", arg);
		break;
	    case 6: // -vbi
		vbi_name = arg;
		break;
	    case -1:
		usage(stderr, 1);
		break;
	}

    fdset_init(fds);

    if (timeout)
    {
	signal(SIGALRM, SIG_DFL); // kill me
	alarm(timeout);
    }
    vbi = vbi_open(vbi_name, 0, channel, outfile, sid, ttpid); // open device
    if (not vbi)
	fatal_ioerror(vbi_name);
    vbi_add_handler(vbi, event, 0); // register event handler

    for (;;)
	fdset_select(fds, -1); // call scheduler

    /* never reached */
    vbi_del_handler(vbi, event, 0);
    vbi_close(vbi);
    exit(0);
}
