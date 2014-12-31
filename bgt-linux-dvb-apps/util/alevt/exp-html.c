/* Copyright 1999 by Paul Ortyl <ortylp@from.pl> */

#include <stdio.h>
#include <string.h>
#include "lang.h"
#include "export.h"

static int html_open(struct export *e);
static int html_option(struct export *e, int opt, char *arg);
static int html_output(struct export *e, char *name, struct fmt_page *pg);
static char *html_opts[] = // module options
{
  "gfx-chr=<char>",             // substitute <char> for gfx-symbols
  "bare",                     // no headers
   0
};

struct html_data // private data in struct export
{
  u8 gfx_chr;
  u8 bare;
};


struct export_module export_html =	// exported module definition
{
    "html",			// id
    "html",			// extension
    html_opts,			// options
    sizeof(struct html_data),	// size
    html_open,			// open
    0,				// close
    html_option,		// option
    html_output			// output
};

#define D  ((struct html_data *)e->data)


static int html_open(struct export *e)
{
    D->gfx_chr = '#';
    D->bare = 0;
    //e->reveal=1;	// the default should be the same for all formats.
    return 0;
}


static int html_option(struct export *e, int opt, char *arg)
{
  switch (opt)
    {
    case 1: // gfx-chr=
      D->gfx_chr = *arg ?: ' ';
      break;
    case 2: // bare (no headers)
      D->bare=1;
      break;
    }
  return 0;
}

#define HTML_BLACK   "#000000"
#define HTML_RED     "#FF0000"
#define HTML_GREEN   "#00FF00"
#define HTML_YELLOW  "#FFFF00"
#define HTML_BLUE    "#0000FF"
#define HTML_MAGENTA "#FF00FF"
#define HTML_CYAN    "#00FFFF"
#define HTML_WHITE   "#FFFFFF"

#undef UNREADABLE_HTML //no '\n'
#define STRIPPED_HTML   //only necessary fields in header

static int html_output(struct export *e, char *name, struct fmt_page *pg)
{
    
  const char* html_colours[]={ HTML_BLACK,
			       HTML_RED,
			       HTML_GREEN,
			       HTML_YELLOW,
			       HTML_BLUE,
			       HTML_MAGENTA,
			       HTML_CYAN,
			       HTML_WHITE};
  FILE *fp;
  int x, y;

#ifdef UNREADABLE_HTML
#define HTML_NL
#else
#define HTML_NL fputc('\n',fp);
#endif
  
  if (not(fp = fopen(name, "w")))
    {
      export_error("cannot create file");
      return -1;
    }

if (!D->bare)
  {
#ifndef STRIPPED_HTML  
    fputs("<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">",fp);
    HTML_NL
#endif
      fputs("<html><head>",fp);
    HTML_NL
#ifndef STRIPPED_HTML
      fputs("<meta http-equiv=\"Content-Type\" content=\"text/html;",fp);
    switch(latin1) {
      case LATIN1: fprintf(fp,"charset=iso-8859-1\">"); break;
      case LATIN2: fprintf(fp,"charset=iso-8859-2\">"); break;
      case KOI8: fprintf(fp,"charset=koi8-r\">"); break;
      case GREEK: fprintf(fp,"charset=iso-8859-7\">"); break;
    }
    HTML_NL
      fputs("<meta name=\"GENERATOR\" content=\"alevt-cap\">",fp);
    HTML_NL
#else
    switch(latin1) {
      case LATIN1: fprintf(fp,"<meta charset=iso-8859-1\">"); break;
      case LATIN2: fprintf(fp,"<meta charset=iso-8859-2\">"); break;
      case KOI8: fprintf(fp,"<meta charset=koi8-r\">"); break;
      case GREEK: fprintf(fp,"<meta charset=iso-8859-7\">"); break;
    }
    HTML_NL
#endif
      fputs("</head>",fp);
    fputs("<body text=\"#FFFFFF\" bgcolor=\"#000000\">",fp);
    HTML_NL
      } //bare

      fputs("<tt><b>",fp);
    HTML_NL

  // write tables in form of HTML format
  for (y = 0; y < 25; ++y)
    { 
      int last_nonblank=0;
      int first_unprinted=0;
      int last_space=1;
      // previous char was &nbsp;
      // is used for deciding to put semicolon or not
      int nbsp=0; 

      // for output filled with ' ' up to 40 chars
      // set last_nonblank=39
      for (x = 0 ; x < 40; ++x) 
	{ 
	  if (pg->data[y][x].attr & EA_GRAPHIC)
	    {pg->data[y][x].ch= D->gfx_chr;}
	  
	  if (pg->data[y][x].ch!=' ')
	  {
	    last_nonblank=x;
	  }
	}

      for (x = 0 ; x <= last_nonblank ; ++x)
	{
	  if (pg->data[y][x].ch==' ')
	    {
	      // if single space between blinking/colour words
	      // then make the space blinking/colour too
	      if ((x)&&(x<39))
		{
		  if ((pg->data[y][x-1].ch!=' ')
		      &&(pg->data[y][x+1].ch!=' ')
		      &&(pg->data[y][x-1].attr & EA_BLINK)
		      &&(pg->data[y][x+1].attr & EA_BLINK))
		    {pg->data[y][x].attr |= EA_BLINK;}
		  else 		    
		    {pg->data[y][x].attr &= ~EA_BLINK;}
		  	    
		  if ((pg->data[y][x-1].ch!=' ')
		      &&(pg->data[y][x+1].ch!=' ')
		      &&(pg->data[y][x-1].fg==pg->data[y][x+1].fg))
		    {pg->data[y][x].fg=pg->data[y][x-1].fg;}
		  else
		    pg->data[y][x].fg=7;
		}
	      else
		{
		  pg->data[y][x].attr &= ~EA_BLINK;
		  pg->data[y][x].fg=7;
		}
	    }
	  else
	    {
	      // if foreground is black set the foreground to previous 
	      // background colour to let it be visible
	      if (!pg->data[y][x].fg)
		{pg->data[y][x].fg=pg->data[y][x].bg;}
	    }
	  //check if attributes changed,
	  //if yes then print chars and update first_unprinted
	  //if not then go to next char
	  if (x)
	    {
	      if (((
		    (pg->data[y][x].attr & EA_BLINK)
		    ==
		    (pg->data[y][x-1].attr & EA_BLINK)
		    )
		   &&
		   (
		    pg->data[y][x].fg == pg->data[y][x-1].fg
		    ))
		  &&(x!=last_nonblank))
		
		{ continue; }
	    }
	  else continue;
	    
	  {
	    int z=first_unprinted;
	    for(;(pg->data[y][z].ch==' ') && (z<x);z++)
	      {
		if (last_space)
		  {
		    fprintf(fp,"&nbsp");
		    last_space=0;
		    nbsp=1;
		  }
		else 
		  {
		    fputc(' ',fp);
		    last_space=1;
		    nbsp=0;
		  }
	      }
	   
	    first_unprinted=z;
	    
	    if (z==x) continue; 
	    
	    if (pg->data[y][first_unprinted].attr & EA_BLINK) 
	      {
		fprintf(fp,"<blink>");
		nbsp=0;
	      }
	    
	    if (pg->data[y][first_unprinted].fg!=7)
	      {
		fprintf(fp,"<font color=\"%s\">",
			html_colours[pg->data[y][first_unprinted].fg]);
		nbsp=0;
	      }
	    for(;(z<x)||(z==last_nonblank);z++)
	      {
		
		if (pg->data[y][z].ch==' ')
		  {
		    for(;(pg->data[y][z].ch==' ') && (z<x);z++)
		      {
			if (last_space)
			  {
			    fprintf(fp,"&nbsp");
			    last_space=0;
			    nbsp=1;
			  }
			else 
			  {
			    fputc(' ',fp);
			    last_space=1;
			    nbsp=0;
			  }
		      }
		    z--;
		  }
		else
		  {
		    //if previous nbsp --> put semicolon!!!
		    if (nbsp) fputc(';',fp);
		    fputc(pg->data[y][z].ch,fp);
		    last_space=0;
		    nbsp=0;
		  }
	      }
	    if (pg->data[y][first_unprinted].fg!=7)
	      {
		fprintf(fp,"</font>");
	      }
	    if (pg->data[y][first_unprinted].attr & EA_BLINK)
	      fprintf(fp,"</blink>");
	    
	    first_unprinted=z;
	  }
	}
      fputs("<br>",fp);
      HTML_NL
    }
  fputs("</b></tt>",fp);
  if (!D->bare)
    fputs("</body></html>",fp);
  fclose(fp);
  return 0;
}
