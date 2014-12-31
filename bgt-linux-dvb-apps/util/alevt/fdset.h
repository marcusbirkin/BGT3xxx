#ifndef FDSET_H
#define FDSET_H

#include "dllist.h"


struct fdset
{
    struct dl_head list[1];
    int del_count;
};


struct fdset_node /*internal*/
{
    struct dl_node node[1];
    int fd;
    void (*handler)(void *data, int fd);
    void *data;
};

extern struct fdset fds[1]; /* global fd list */

int fdset_init(struct fdset *fds);
int fdset_add_fd(struct fdset *fds, int fd, void *handler, void *data);
int fdset_del_fd(struct fdset *fds, int fd);
int fdset_select(struct fdset *fds, int timeout /*millisec*/);
#endif
