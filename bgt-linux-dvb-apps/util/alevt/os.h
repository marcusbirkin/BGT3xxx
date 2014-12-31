#ifndef OS_H
#define OS_H
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBsd__) \
	|| defined(__bsdi__)
#define BSD
#endif
#endif
