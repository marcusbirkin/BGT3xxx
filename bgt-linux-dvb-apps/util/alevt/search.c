#include <sys/types.h> // for freebsd
#include <stdlib.h>
#include "vt.h"
#include "misc.h"
#include "cache.h"
#include "search.h"


static void convert(u8 *p, u8 *buf, int *line)
{
    int x, y, c, ch, gfx, hid = 0;

    for (y = 1, p += 40; y < 25; ++y)
    {
	if (not hid)
	{
	    gfx = 0;
	    for (x = 0; x < 40; ++x)
	    {
		c = ' ';
		switch (ch = *p++)
		{
		    case 0x00 ... 0x07:
			gfx = 0;
			break;
		    case 0x10 ... 0x17:
			gfx = 1;
			break;
		    case 0x0c:
			hid = 1;
			break;
		    case 0x7f:
			c = '*';
			break;
		    case 0x20 ... 0x7e:
			if (gfx && ch != ' ' && (ch & 0xa0) == 0x20)
			    ch = '#';
		    case 0xa0 ... 0xff:
			c= ch;
		}
		*buf++ = c;
	    }
	    *buf++ = '\n';
	    *line++ = y;
	}
	else
	{
	    p += 40;
	    hid = 0;
	}
    }
    *line = y;
    *buf = 0;
}


static int search_pg(struct search *s, struct vt_page *vtp)
{
    regmatch_t m[1];
    u8 buf[H *(W+1) + 1];
    int line[H];

    convert(PTR vtp->data, buf, line);
    if (regexec(s->pattern, buf, 1, m, 0) == 0)
    {
	s->len = 0;
	if (m->rm_so >= 0)
	{
	    s->y = line[m->rm_so / (W+1)];
	    s->x = m->rm_so % (W+1);
	    s->len = m->rm_eo - m->rm_so;
	    if (s->x + s->len > 40)
		s->len = 40 - s->x;
	}
	return 1;
    }
    return 0;
}


struct search * search_start(struct cache *ca, u8 *pattern)
{
    struct search *s;
    int f = 0;

    if (not(s = malloc(sizeof(*s))))
	goto fail1;

    if (pattern[0] == '!')
	pattern++;
    else
	f = REG_ICASE;

    if (regcomp(s->pattern, pattern, f | REG_NEWLINE) != 0)
	goto fail2;

    s->cache = ca;
    return s;

fail2:
    free(s);
fail1:
    return 0;
}


void search_end(struct search *s)
{
    regfree(s->pattern);
    free(s);
}


int search_next(struct search *s, int *pgno, int *subno, int dir)
{
    struct vt_page *vtp = 0;

    if (s->cache)
	vtp = s->cache->op->foreach_pg(s->cache, *pgno, *subno, dir,
	search_pg, s);
    if (vtp == 0)
	return -1;

    *pgno = vtp->pgno;
    *subno = vtp->subno ?: ANY_SUB;
    return 0;
}
