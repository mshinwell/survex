/* prbitmap.c */
/* Bitmap routines for Survex Dot-matrix and Inkjet printer drivers */
/* Copyright (C) 1993-2002 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "useful.h"
#include "filename.h"
#include "message.h"
#include "prbitmap.h"
#include "prcore.h"

void (*PlotDot)(long X, long Y);

static unsigned int max_def_char;

int fontsize = 8, fontsize_labels;

#define CHAR_SPACING 8 /* no. of char_pixels to each char */

static long xLast, yLast;

static int dppX, dppY; /* dots (device pixels) per pixel (char defn pixels) */
static int dppX_labels, dppY_labels; /* ditto for station labels */

/* Uses Bresenham Line generator algorithm */
extern void
DrawLineDots(long x, long y, long x2, long y2)
{
   long d, dx, dy;
   int xs, ys;

   dx = x2 - x;
   if (dx > 0) {
      xs = 1;
   } else {
      dx = -dx;
      xs = -1;
   }

   dy = y2 - y;
   if (dy > 0) {
      ys = 1;
   } else {
      dy = -dy;
      ys = -1;
   }

   (PlotDot)(x, y);
   if (dx > dy) {
      d = (dy << 1) - dx;
      while (x != x2) {
	 x += xs;
	 if (d >= 0) {
	    y += ys;
	    d += (dy - dx) << 1;
	 } else {
	    d += dy << 1;
	 }
	 (PlotDot)(x, y);
      }
   } else {
      d = (dx << 1) - dy;
      while (y != y2) {
	 y += ys;
	 if (d >= 0) {
	    x += xs;
	    d += (dx - dy) << 1;
	 } else {
	    d += dx << 1;
	 }
	 (PlotDot)(x, y);
      }
   }
}

extern void
MoveTo(long x, long y)
{
   xLast = x;
   yLast = y;
}

extern void
DrawTo(long x, long y)
{
   DrawLineDots(xLast, yLast, x, y);
   xLast = x;
   yLast = y;
}

extern void
DrawCross(long x, long y)
{
   DrawLineDots(x - dppX_labels, y - dppY_labels,
		x + dppX_labels, y + dppY_labels);
   DrawLineDots(x + dppX_labels, y - dppY_labels,
		x - dppX_labels, y + dppY_labels);
   xLast = x;
   yLast = y;
}

/* Font Driver Routines */

static char *font;

/* Calculate device_dots/char_pixel given point size and dpi */
#define DPP(POINTS, DPI) max(1, ((int)(POINTS) * (int)(DPI) + 288) / 576)

extern void
read_font(const char *pth, const char *leaf, int dpiX, int dpiY)
{
   FILE *fh;
   unsigned char header[20];
   int i;
   unsigned int len;
   char *fnm;

   dppX_labels = DPP(fontsize_labels, dpiX);
   dppY_labels = DPP(fontsize_labels, dpiY);
   dppX = DPP(fontsize, dpiX);
   dppY = DPP(fontsize, dpiY);
/* printf("Debug info: dpp x=%d, y=%d\n\n",dppX,dppY); */

   fh = fopenWithPthAndExt(pth, leaf, NULL, "rb", &fnm);

   if (!fh) fatalerror(/*Couldn't open file `%s'*/93, leaf);

   if (fread(header, 1, 20, fh) < 20 ||
       memcmp(header, "Svx\nFnt\r\n\xfe\xff", 12) != 0) {
      fatalerror(/*Error in format of font file `%s'*/88, fnm);
      /* TRANSLATE - not a survex font file... */
   }

   if (header[12] != 0) {
      fatalerror(/*Error in format of font file `%s'*/88, fnm);
      /* TRANSLATE - "I don't understand this font file version" */
   }

   /* this entry gives the number of chars defined (first is 32 so add 31) */
   max_def_char = (header[14] << 8) | header[15];
   max_def_char += 31;

   len = 0;
   for (i = 16; i < 20; i++) len = (len << 8) | header[i];

   if ((len / 8) + 31 != max_def_char) {
      /* len and #chars are really the same info, so check they match-up
       * (also avoids a potential buffer overrun) */
      fatalerror(/*Error in format of font file `%s'*/88, fnm);
      /* TRANSLATE Font file length and max_def_char mismatch */
   }

   font = osmalloc(len);
   if (fread(font, 1, len, fh) < len) {
      fatalerror(/*Error in format of font file `%s'*/88, fnm);
      /* TRANSLATE Font file truncated?/read error */
   }

   osfree(fnm);
   (void)fclose(fh);
}

static int dppx, dppy;

extern void
SetFont(int fontcode)
{
   switch (fontcode) {
      case PR_FONT_DEFAULT:
	 dppx = dppX;
	 dppy = dppY;
	 break;
      case PR_FONT_LABELS:
	 dppx = dppX_labels;
	 dppy = dppY_labels;
	 break;
      default:
	 BUG("unknown font code");
   }
}

static void
WriteLetter(int ch, long X, long Y)
{
   int x, y, x2, y2, t;
/*   printf("*** writeletter( %c, %ld, %ld )\n",ch,X,Y);*/
   for (y = 7; y >= 0; y--) {
      t = font[(ch - 32) * 8 + 7 - y];
      for (x = 0; x < 8; x++) {
	 if (t & 1) {
	    /* plot mega-pixel */
	    for (x2 = 0; x2 < dppx; x2++)
	       for (y2 = 0 ; y2 < dppy; y2++)
		  (PlotDot)(X + (long)x * dppx + x2, Y + (long)y * dppy + y2);
	 }
	 t = t >> 1;
      }
   }
}

extern void
WriteString(const char *s)
{
   unsigned char ch;
   unsigned const char *p = (unsigned const char *)s;
   while ((ch = *p++) >= 32) {
      if (ch <= max_def_char) WriteLetter(ch, xLast, yLast);
      xLast += (long)CHAR_SPACING * dppx;
   }
}