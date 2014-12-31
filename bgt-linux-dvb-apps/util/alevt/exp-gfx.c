/* Copyright 1999 by Paul Ortyl <ortylp@from.pl> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lang.h"
#include "export.h"
#include "font.h"
#define WW (W*CW) /* pixel width of window */
#define WH (H*CH) /* pixel hegiht of window */


static inline void draw_char(unsigned char * colour_matrix, int fg, int bg,
    int c, int dbl, int _x, int _y, int sep)
{
  int x,y;
  unsigned char* src= (latin1==LATIN1 ? font1_bits : font2_bits);
  int dest_x=_x*CW;
  int dest_y=_y*CH;
      
  for(y=0;y<(CH<<dbl); y++)
    {
      for(x=0;x<CW; x++)
	{
	  int bitnr, bit, maskbitnr, maskbit;
	  bitnr=(c/32*CH + (y>>dbl))*CW*32+ c%32*CW +x;
	  bit=(*(src+bitnr/8))&(1<<bitnr%8);
	  if (sep)
	    {
	      maskbitnr=(0xa0/32*CH + (y>>dbl))*CW*32+ 0xa0%32*CW +x;
	      maskbit=(*(src+maskbitnr/8))&(1<<maskbitnr%8);
	      *(colour_matrix+WW*(dest_y+y)+dest_x+x)=
		(char)((bit && (!maskbit)) ? fg : bg);
	    }
	  else 
	    *(colour_matrix+WW*(dest_y+y)+dest_x+x)=
	      (char)(bit ? fg : bg);
	}
    }
  return;
}


static void prepare_colour_matrix(/*struct export *e,*/
		      struct fmt_page *pg, 
		      unsigned char *colour_matrix)
{
   int x, y;
   for (y = 0; y < H; ++y)
	{
	  for (x = 0; x < W; ++x)
	    { 
	      if (pg->dbl & (1<<(y-1)))
		{
		  if (pg->data[y-1][x].attr & EA_HDOUBLE)
		     draw_char(colour_matrix, pg->data[y][x].fg, 
			    pg->data[y][x].bg, pg->data[y][x].ch, 
			    (0), 
			    x, y, 
			    ((pg->data[y][x].attr & EA_SEPARATED) ? 1 : 0)
			    );
		}
	      else
		{
		  draw_char(colour_matrix, pg->data[y][x].fg, 
			    pg->data[y][x].bg, pg->data[y][x].ch, 
			    ((pg->data[y][x].attr & EA_DOUBLE) ? 1 : 0), 
			    x, y, 
			    ((pg->data[y][x].attr & EA_SEPARATED) ? 1 : 0)
			    );
		}
	    }
	}
    return;
}


static int ppm_output(struct export *e, char *name, struct fmt_page *pg);

struct export_module export_ppm = // exported module definition
{
    "ppm",			// id
    "ppm",			// extension
    0,				// options
    0,				// size
    0,				// open
    0,				// close
    0,				// option
    ppm_output			// output
};


static int ppm_output(struct export *e, char *name, struct fmt_page *pg)
{
  FILE *fp;
  long n;
  static u8 rgb1[][3]={{0,0,0},
		      {1,0,0},
		      {0,1,0},
		      {1,1,0},
		      {0,0,1},
		      {1,0,1},
		      {0,1,1},
		      {1,1,1}};
  unsigned char *colour_matrix;

  if (!(colour_matrix=malloc(WH*WW))) 
    {
      export_error("cannot allocate memory");
      return 0;
    }

  prepare_colour_matrix(/*e,*/ pg, (unsigned char *)colour_matrix); 
  if (not(fp = fopen(name, "w")))
    {
      free(colour_matrix);
      export_error("cannot create file");
      return -1;
    }
  fprintf(fp,"P6 %d %d 1\n", WW, WH);

  for(n=0;n<WH*WW;n++)
    {
      if (!fwrite(rgb1[(int) *(colour_matrix+n)], 3, 1, fp))
	{
	  export_error("error while writting to file");
	  free(colour_matrix);
	  fclose(fp);
	  return -1;
	}
    }
  free(colour_matrix);
  fclose(fp);
  return 0;
}


#ifdef WITH_PNG

#include <png.h>
static int png_open(struct export *e);
static int png_option(struct export *e, int opt, char *arg);
static int png_output(struct export *e, char *name, struct fmt_page *pg);
static char *png_opts[] =	// module options
{
    "compression=<0-9>",	// set compression level
    0
};

struct png_data			// private data in struct export
{
    int compression;
};

struct export_module export_png =	// exported module definition
{
    "png",			// id
    "png",			// extension
    png_opts,			// options
    sizeof(struct png_data),	// size
    png_open,			// open
    0,				// close
    png_option,			// option
    png_output			// output
};

#define D  ((struct png_data *)e->data)


static int png_open(struct export *e)
{
    D->compression = Z_DEFAULT_COMPRESSION;
    return 0;
}


static int png_option(struct export *e, int opt, char *arg)
{
    switch (opt)
    {
	case 1: // compression=
	    if (*arg >= '0' && *arg <= '9')
		D->compression = *arg - '0';
	    break;
    }
    return 0;
}


static int png_output(struct export *e, char *name, struct fmt_page *pg)
{
  FILE *fp;
  int x;
  png_structp png_ptr;
  png_infop info_ptr;
  png_byte *row_pointers[WH];
  static u8 rgb8[][3]={{  0,  0,  0},
		      {255,  0,  0},
		      {  0,255,  0},
		      {255,255,  0},
		      {  0,  0,255},
		      {255,  0,255},
		      {  0,255,255},
		      {255,255,255}};
  unsigned char *colour_matrix;

  if (!(colour_matrix=malloc(WH*WW))) 
    {
      export_error("cannot allocate memory");
      return -1;
    }
  prepare_colour_matrix(/*e,*/ pg, (unsigned char *)colour_matrix); 
  if (not(fp = fopen(name, "w")))
    {
      free(colour_matrix);
      export_error("cannot create file");
      return -1;
      }
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 
				    NULL, NULL, NULL);
  if (!png_ptr)
    {
      free(colour_matrix);
      fclose(fp);
      export_error("libpng init error");
      return -1;
    }
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
      png_destroy_write_struct(&png_ptr,
			       (png_infopp)NULL);
      free(colour_matrix);
      fclose(fp);
      export_error("libpng init error");
      return -1;
    }
  png_init_io(png_ptr, fp);
  png_set_compression_level(png_ptr, D->compression);
  png_set_compression_mem_level(png_ptr, 9);
  png_set_compression_window_bits(png_ptr, 15);
  png_set_IHDR(png_ptr, info_ptr, WW, WH,
	       8, PNG_COLOR_TYPE_PALETTE , PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_set_PLTE(png_ptr, info_ptr,(png_color*) rgb8 , 8);
  png_write_info(png_ptr, info_ptr);
  for(x=0; x<WH; x++)
    { row_pointers[x]=colour_matrix+x*WW; }
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  free(colour_matrix);
  fclose(fp);
  return 0;
}

#endif

