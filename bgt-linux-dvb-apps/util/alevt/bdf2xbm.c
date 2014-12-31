/*
    Simple program to convert a bdf-font to a bitmap.
    The characters are arranged in a 32x8 matrix.
    usage: bdf2xbm [identifier] <bdf >xbm
    Copyright 1998,1999 by E. Toernig (froese@gmx.de)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define not !
#define streq(a,b) (strcmp((a),(b)) == 0)

int lineno;
char *word[64];
int nword;

char *font = "font%dx%d";
int w, h, bpl;
unsigned char *bmap;


static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fprintf(stderr, "bdf2xbm");
    if (lineno)
	fprintf(stderr, ":%d", lineno);
    fprintf(stderr, ": ");
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    exit(1);
}


static int nextline()
{
    static char buf[256];
    char *p;
    int i;

    do
    {
	nword = 0;
	if (fgets(buf, sizeof(buf), stdin) == 0)
	    return nword;
	lineno++;
	
	p = buf;
	for (;;)
	{
	    while (isspace(*p))
		p++;
	    if (*p == 0)
		break;
	    word[nword++] = p;
	    while (*p && not isspace(*p))
		*p = toupper(*p), p++;
	    if (*p == 0)
		break;
	    *p++ = 0;
	}
    } while (nword == 0);

    for (i = nword; i < 64; ++i)
	word[i] = "";
    return nword;
}


static inline void setbit(int ch, int x, int y)
{

    int yo = ch / 32 * h + y;
    int xo = ch % 32 * w + x;

    bmap[yo * bpl + xo / 8] |= 1 << (xo % 8);
}


static void dobitmap(int ch, int x, int y)
{
    int i, j;

    for (i = 0; i < y; ++i)
    {
	nextline();
	if (nword > 1 || strlen(word[0]) != (x + 7) / 8 * 2)
	    error("bad BITMAP");
	for (j = 0; j < x; ++j)
	{
	    int c = word[0][j / 4];
	    if (c >= '0' && c <= '9')
		c -= '0';
	    else if (c >= 'A' && c <= 'F')
		c -= 'A' - 10;
	    else
		error("bad hexchar in BITMAP");
	    if (c & (8 >> (j % 4)))
		setbit(ch, j, i);
	}
    }
}


static void dochar()
{
    int ch = -1, x = -1,  y = -1;

    while (nextline())
    {
	if (streq(word[0], "ENDCHAR"))
	    return;
	else if (streq(word[0], "ENCODING") && nword == 2)
	{
	    ch = atoi(word[1]);
	    if (ch < 0 || ch > 255)
		error("bad character code %d", ch);
	}
	else if (streq(word[0], "BBX") && nword == 5)
	{
	    x = atoi(word[1]), y = atoi(word[2]);
	    if (x < 1 || x > 64 || y < 1 || y > 64)
		error("bad BBX (%dx%d)", x, y);
	}
	else if (streq(word[0], "BITMAP"))
	{
	    if (x < 0)
		error("missing BBX");
	    if (ch < 0)
		error("missing ENDCODING");
	    dobitmap(ch, x, y);
	}
    }
    error("unexpected EOF (missing ENDCHAR)");
}


static void dofile()
{
    lineno = 0;
    w = h = 0;
    bmap = 0;

    nextline();
    if (nword != 2 || not streq(word[0], "STARTFONT"))
	error("not a bdf-file");

    while (nextline())
    {
	if (streq(word[0], "ENDFONT"))
	    return;
	else if (streq(word[0], "FONTBOUNDINGBOX") && nword == 5)
	{
	    if (bmap)
		error("multiple FONTBOUNDINGBOXes!?!");
	    w = atoi(word[1]), h = atoi(word[2]);
	    if (w < 1 || w > 64 || h < 1 || h > 64)
		error("bad bounding box %dx%d\n", w, h);
	    bpl = (w*32+7)/8; // rounding is unnecessary
	    bmap = calloc(1, bpl * h*8);
	    if (bmap == 0)
		error("out of memory");
	}
	else if (streq(word[0], "STARTCHAR"))
	{
	    if (not bmap)
		error("no FONTBOUNDINGBOX");
	    dochar();
	}
    }
    error("unexpected EOF (missing ENDFONT)");
}


static void writexbm()
{
    char buf[256];
    int i, j;
    unsigned char *p = bmap;

    if (not bmap)
	return;

    sprintf(buf, font, w, h);

    printf("#define %s_width %d\n", buf, 32 * w);
    printf("#define %s_height %d\n", buf, 8 * h);
    printf("static unsigned char %s_bits[] = {\n", buf);
    for (i = 0; i < 16 * h * w / 8; ++i)
    {
	for (j = 0; j < 16; ++j)
	    printf("0x%02x,", *p++);
	printf("\n");
    }
    printf("};\n");
}


int main(int argc, char **argv)
{
    if (argc > 1)
	font = argv[1];
    dofile();
    writexbm();
    exit(0);
}
