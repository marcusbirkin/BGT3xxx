#include <stdio.h>
#include <stdarg.h>
#include "misc.h"

char *prgname = 0;

extern char *strrchr(const char *, int);
NORETURN(exit(int));


void setprgname(char *str)
{
    char *x = strrchr(str, '/');
    prgname = x ? x+1 : str;
}


static void print_prgname(void)
{
    if (prgname && *prgname)
	fprintf(stderr, "%s: ", prgname);
}


void error(const char *str, ...)
{
    va_list args;
    va_start(args, str);
    print_prgname();
    vfprintf(stderr, str, args);
    fputc('\n', stderr);
}


void ioerror(const char *str)
{
    print_prgname();
    perror(str);
}


void fatal(const char *str, ...)
{
    va_list args;
    va_start(args, str);
    print_prgname();
    vfprintf(stderr, str, args);
    fputc('\n', stderr);
    exit(2);
}


void fatal_ioerror(const char *str)
{
    print_prgname();
    perror(str);
    exit(2);
}


void out_of_mem(int size)
{
    if (size > 0)
	fatal("out of memory allocating %d bytes.", size);
    fatal("out of memory.");
}
