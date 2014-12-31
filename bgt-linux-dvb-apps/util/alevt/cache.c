#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "dllist.h"
#include "cache.h"
#include "help.h"


static inline int hash(int pgno)
{
    // very simple...
    return pgno % HASH_SIZE;
}


static void do_erc(struct vt_page *ovtp, struct vt_page *nvtp)
{
    int l, c;

    if (nvtp->errors == 0 && ovtp->lines == nvtp->lines)
	return;

    for (l = 0; l < H; ++l)
    {
	if (~nvtp->lines & (1 << l))
	    memcpy(nvtp->data[l], ovtp->data[l], W);
	else if (ovtp->lines & (1 << l))
	    for (c = 0; c < W; ++c)
		if (nvtp->data[l][c] == BAD_CHAR)
		    nvtp->data[l][c] = ovtp->data[l][c];
    }
    nvtp->lines |= ovtp->lines;
}


static void cache_close(struct cache *ca)
{
    struct cache_page *cp;
    int i;

    for (i = 0; i < HASH_SIZE; ++i)
	while (not dl_empty(ca->hash + i))
	{
	    cp = PTR ca->hash[i].first;
	    dl_remove(cp->node);
	    free(cp);
	}
    free(ca);
}


static void cache_reset(struct cache *ca)
{
    struct cache_page *cp, *cpn;
    int i;

    for (i = 0; i < HASH_SIZE; ++i)
	for (cp = PTR ca->hash[i].first; cpn = PTR cp->node->next; cp = cpn)
	    if (cp->page->pgno / 256 != 9) // don't remove help pages
	    {
		dl_remove(cp->node);
		free(cp);
		ca->npages--;
	    }
    memset(ca->hi_subno, 0, sizeof(ca->hi_subno[0]) * 0x900);
}

/*  Get a page from the cache.
    If subno is SUB_ANY, the newest subpage of that page is returned */


static struct vt_page * cache_get(struct cache *ca, int pgno, int subno)
{
    struct cache_page *cp;
    int h = hash(pgno);

    for (cp = PTR ca->hash[h].first; cp->node->next; cp = PTR cp->node->next)
	if (cp->page->pgno == pgno)
	    if (subno == ANY_SUB || cp->page->subno == subno)
	    {
		// found, move to front (make it 'new')
		dl_insert_first(ca->hash + h, dl_remove(cp->node));
		return cp->page;
	    }
    return 0;
}

/*  Put a page in the cache.
    If it's already there, it is updated. */


static struct vt_page * cache_put(struct cache *ca, struct vt_page *vtp)
{
    struct cache_page *cp;
    int h = hash(vtp->pgno);
    
    for (cp = PTR ca->hash[h].first; cp->node->next; cp = PTR cp->node->next)
	if (cp->page->pgno == vtp->pgno && cp->page->subno == vtp->subno)
	    break;

    if (cp->node->next)
    {
	// move to front.
	dl_insert_first(ca->hash + h, dl_remove(cp->node));
	if (ca->erc)
	    do_erc(cp->page, vtp);
    }
    else
    {
	cp = malloc(sizeof(*cp));
	if (cp == 0)
	    return 0;
	if (vtp->subno >= ca->hi_subno[vtp->pgno])
	    ca->hi_subno[vtp->pgno] = vtp->subno + 1;
	ca->npages++;
	dl_insert_first(ca->hash + h, cp->node);
    }

    *cp->page = *vtp;
    return cp->page;
}

/* Same as cache_get but doesn't make the found entry new */


static struct vt_page * cache_lookup(struct cache *ca, int pgno, int subno)
{
    struct cache_page *cp;
    int h = hash(pgno);

    for (cp = PTR ca->hash[h].first; cp->node->next; cp = PTR cp->node->next)
	if (cp->page->pgno == pgno)
	    if (subno == ANY_SUB || cp->page->subno == subno)
		return cp->page;
    return 0;
}


static struct vt_page * cache_foreach_pg(struct cache *ca, int pgno, int subno,
    int dir, int (*func)(), void *data)
{
    struct vt_page *vtp, *s_vtp = 0;

    if (ca->npages == 0)
	return 0;

    if (vtp = cache_lookup(ca, pgno, subno))
	subno = vtp->subno;
    else if (subno == ANY_SUB)
	subno = dir < 0 ? 0 : 0xffff;

    for (;;)
    {
	subno += dir;
	while (subno < 0 || subno >= ca->hi_subno[pgno])
	{
	    pgno += dir;
	    if (pgno < 0x100)
		pgno = 0x9ff;
	    if (pgno > 0x9ff)
		pgno = 0x100;
	    subno = dir < 0 ? ca->hi_subno[pgno] - 1 : 0;
	}
	if (vtp = cache_lookup(ca, pgno, subno))
	{
	    if (s_vtp == vtp)
		return 0;
	    if (s_vtp == 0)
		s_vtp = vtp;
	    if (func(data, vtp))
		return vtp;
	}
    }
}


static int cache_mode(struct cache *ca, int mode, int arg)
{
    int res = -1;

    switch (mode)
    {
	case CACHE_MODE_ERC:
	    res = ca->erc;
	    ca->erc = arg ? 1 : 0;
	    break;
    }
    return res;
}


static struct cache_ops cops =
{
    cache_close,
    cache_get,
    cache_put,
    cache_reset,
    cache_foreach_pg,
    cache_mode,
};


struct cache * cache_open(void)
{
    struct cache *ca;
    struct vt_page *vtp;
    int i;

    if (not(ca = malloc(sizeof(*ca))))
	goto fail1;

    for (i = 0; i < HASH_SIZE; ++i)
	dl_init(ca->hash + i);

    memset(ca->hi_subno, 0, sizeof(ca->hi_subno));
    ca->erc = 1;
    ca->npages = 0;
    ca->op = &cops;

    for (vtp = help_pages; vtp < help_pages + nr_help_pages; vtp++)
	cache_put(ca, vtp);

    return ca;

fail2:
    free(ca);
fail1:
    return 0;
}
