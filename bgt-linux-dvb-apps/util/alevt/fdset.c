#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "dllist.h"
#include "misc.h"
#include "fdset.h"

struct fdset fds[1];		/* global fd list */


int fdset_init(struct fdset *fds)
{
    dl_init(fds->list);
    fds->del_count = 0;
    return 0;
}


int fdset_add_fd(struct fdset *fds, int fd, void *handler, void *data)
{
    struct fdset_node *fn;

    if (fd < 0)
	return -1;
    if (handler == 0)
	return -1;

    for (fn = PTR fds->list->first; fn->node->next; fn = PTR fn->node->next)
	if (fn->fd == fd)
	    return -1;

    if (not(fn = malloc(sizeof(*fn))))
	return -1;
    fn->fd = fd;
    fn->handler = handler;
    fn->data = data;
    dl_insert_last(fds->list, fn->node);
    return 0;
}


int fdset_del_fd(struct fdset *fds, int fd)
{
    struct fdset_node *fn;

    for (fn = PTR fds->list->first; fn->node->next; fn = PTR fn->node->next)
	if (fn->fd == fd)
	{
	    dl_remove(fn->node);
	    free(fn);
	    fds->del_count++;
	    return 0;
	}
    return -1;
}


int fdset_select(struct fdset *fds, int timeout)
{
    struct fdset_node *fn;
    fd_set rfds[1];
    struct timeval tv[1], *tvp = 0;
    int max_fd, x, del_count;

    FD_ZERO(rfds);
    max_fd = 0;
    for (fn = PTR fds->list->first; fn->node->next; fn = PTR fn->node->next)
    {
	FD_SET(fn->fd, rfds);
	if (fn->fd >= max_fd)
	    max_fd = fn->fd + 1;
    }

    if (timeout >= 0)
    {
	tv->tv_sec = timeout/1000;
	tv->tv_usec = timeout%1000*1000;
	tvp = tv;
    }

    x = select(max_fd, rfds, 0, 0, tvp);
    if (x <= 0)
	return x;

    /* A little bit complicated. A called handler may modify the fdset... */
restart:
    del_count = fds->del_count;
    for (fn = PTR fds->list->first; fn->node->next; fn = PTR fn->node->next)
	if (FD_ISSET(fn->fd, rfds))
	{
	    FD_CLR(fn->fd, rfds);
	    fn->handler(fn->data, fn->fd);
	    if (fds->del_count != del_count)
		goto restart;
	}
    return 1;
}
