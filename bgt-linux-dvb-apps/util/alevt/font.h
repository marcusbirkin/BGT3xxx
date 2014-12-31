#ifndef FONT_H
#define FONT_H

#include "fontsize.h" /* the #defines from font?.xbm */

#if font1_width != font2_width || font1_height != font2_height
#error different font sizes.
#endif

extern unsigned char font1_bits[];
extern unsigned char font2_bits[];
extern unsigned char font3_bits[];
extern unsigned char font4_bits[];

#define font_width font1_width
#define font_height font1_height
#define CW (font_width/32) /* pixel width of a character */
#define CH (font_height/8) /* pixel height of a character */
#endif
