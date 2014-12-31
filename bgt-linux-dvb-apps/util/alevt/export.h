#ifndef EXPORT_H
#define EXPORT_H

#include "vt.h"
#include "misc.h"


struct fmt_char
{
    u8 ch, fg, bg, attr;
};

#define EA_DOUBLE	1	// double height char
#define EA_HDOUBLE	2	// single height char in double height line
#define EA_BLINK	4	// blink
#define EA_CONCEALED	8	// concealed
#define EA_GRAPHIC	16	// graphic symbol
#define EA_SEPARATED	32	// use separated graphic symbol

#define E_DEF_FG	7
#define E_DEF_BG	0
#define E_DEF_ATTR	0


struct fmt_page
{
    struct vt_page *vtp;
    u32 dbl, hid;
    struct fmt_char data[H][W];
};


struct export
{
    struct export_module *mod;	// module type
    char *fmt_str;		// saved option string (splitted)
    // global options
    int reveal;			// reveal hidden chars
    // local data for module's use.  initialized to 0.
    struct { int dummy; } data[0];
};


struct export_module
{
    char *fmt_name;		// the format type name (ASCII/HTML/PNG/...)
    char *extension;		// the default file name extension
    char **options;		// module options
    int local_size;
    int (*open)(struct export *fmt);
    void (*close)(struct export *fmt);
    int (*option)(struct export *fmt, int opt, char *arg);
    int (*output)(struct export *fmt, char *name, struct fmt_page *pg);
};


extern struct export_module *modules[];	// list of modules (for help msgs)
void export_error(char *str, ...);	// set error
char *export_errstr(void);		// return last error
char *export_mkname(struct export *e, char *fmt, struct vt_page *vtp, char *usr);


struct export *export_open(char *fmt);
void export_close(struct export *e);
int export(struct export *e, struct vt_page *vtp, char *user_str);
#endif
