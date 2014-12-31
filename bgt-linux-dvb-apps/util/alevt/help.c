#include "vt.h"
#include "misc.h"

#define VFILL "     "
#define HELP_HEADER							\
"........\6AleVT Online Help System       ",				\
"  \22`p0`0    p `0pppp                    ",				\
"\4\35\22\177 \177j5`p  \177 j5 j5        \7   Version \34",		\
"\4\35\22\177,\177j5\177.! +t>! j5        \7"VFILL VERSION" \34",	\
"  \22# #\42!\42#   \42   \42!                     ",
#define FLOF_DATA							\
	1, { {0x100,ANY_SUB}, {0x200,ANY_SUB}, {0x300,ANY_SUB},		\
	     {0x400,ANY_SUB}, {0x0ff,ANY_SUB}, {0x100,ANY_SUB} }


struct vt_page help_pages[] =
{
    { 0x900, 0, -1, 0, 0, (1<<26)-1, {
#include "vt900.out"
    }, FLOF_DATA },

    { 0x901, 1, -1, 0, 0, (1<<26)-1, {
#include "vt901.out"
    }, FLOF_DATA },

    { 0x902, 1, -1, 0, 0, (1<<26)-1, {
#include "vt902.out"
    }, FLOF_DATA },

    { 0x903, 1, -1, 0, 0, (1<<26)-1, {
#include "vt903.out"
    }, FLOF_DATA },

    { 0x904, 1, -1, 0, 0, (1<<26)-1, {
#include "vt904.out"
    }, FLOF_DATA },

    { 0x905, 2, -1, 0, 0, (1<<26)-1, {
#include "vt905.out"
    }, FLOF_DATA },

    { 0x906, 1, -1, 0, 0, (1<<26)-1, {
#include "vt906.out"
    }, FLOF_DATA },

    { 0x907, 2, -1, 0, 0, (1<<26)-1, {
#include "vt907.out"
    }, FLOF_DATA },

    { 0x908, 1, -1, 0, 0, (1<<26)-1, {
#include "vt908.out"
    }, FLOF_DATA },

    { 0x909, 0, -1, 0, 0, (1<<26)-1, {
#include "vt909.out"
    }, FLOF_DATA },

    { 0x910, 2, -1, 0, 0, (1<<26)-1, {
#include "vt910.out"
    }, FLOF_DATA },

    { 0x911, 1, -1, 0, 0, (1<<26)-1, {
#include "vt911.out"
    }, FLOF_DATA },

    { 0x912, 2, -1, 0, 0, (1<<26)-1, {
#include "vt912.out"
    }, FLOF_DATA },

    { 0x913, 1, -1, 0, 0, (1<<26)-1, {
#include "vt913.out"
    }, FLOF_DATA },

    { 0x914, 0, -1, 0, 0, (1<<26)-1, {
#include "vt914.out"
    }, FLOF_DATA },

    { 0x915, 0, -1, 0, 0, (1<<26)-1, {
#include "vt915.out"
    }, FLOF_DATA },
};

const int nr_help_pages = NELEM(help_pages);
