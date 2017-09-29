/* $Id: cimg.cc,v 1.4 2007/06/09 11:35:06 bergo Exp $ */

/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2007 Felipe Paulo Guazzi Bergo
    bergo@seul.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "cimg.h"

CImg::CImg(int w,int h) {
  data = 0;
  ok = 0;
  alloc(w,h);
}

CImg::CImg(CImg *src) {
  data = 0;
  ok = 0;
  alloc(src->W,src->H);
  if (!ok) return;
  memcpy(data,src->data,H*rowlen);
  ok = 1;
}

CImg::CImg(const char *filename) {
  FILE *f;
  png_byte header[8];
  png_structp pngp=NULL;
  png_infop infp=NULL, endp=NULL;
  int i;

  png_uint_32 width, height;
  int bpp, ct, it, zt, ft;

  data = NULL;
  ok = 0;
  f = fopen(filename, "r");
  if (f==NULL) return;

  fread(header, 1, 8, f);
  if (png_sig_cmp(header,0,8)!=0) {
    fprintf(stderr, "** not a PNG image: %s\n\n",filename);
    fclose(f);
    return;
  }

  pngp = png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
  if (pngp!=NULL) {
    infp = png_create_info_struct(pngp);
    endp = png_create_info_struct(pngp);
  }

  if (infp==NULL || endp==NULL) {
    fclose(f);
    return;
  }

  png_init_io(pngp, f);
  png_set_sig_bytes(pngp, 8);
  png_read_info(pngp,infp);
  png_get_IHDR(pngp, infp, &width, &height, &bpp, &ct, &it, &zt, &ft);

  if (ct == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(pngp);
    png_set_strip_alpha(pngp);
  }
  if (bpp == 16)
    png_set_strip_16(pngp);
  if (ct & PNG_COLOR_MASK_ALPHA)
    png_set_strip_alpha(pngp);
  if (ct == PNG_COLOR_TYPE_GRAY ||
      ct == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(pngp);

  alloc(pngp->width,pngp->height);
  if (!ok) { fclose(f); return; }
  ok = 0;

  for(i=0;i<pngp->height;i++) {
    png_read_row(pngp, (png_bytep) (&data[i*rowlen]), NULL);
  }

  png_read_end(pngp, endp);
  png_destroy_read_struct(&pngp, &infp, &endp);
  ok = 1;
}

void CImg::crop(int x,int y,int w,int h) {
  CImg *tmp;

  tmp = new CImg(w,h);
  if (!tmp) return;
  if (!tmp->ok) return;
  bitblt(data, tmp->data, x,y, W, 0,0, w, w, h);

  if (w>W || h>H) return;
  if (x!=0 || y!=0)

  data = (hati *) realloc(data, w*h*3);
  W = w;
  H = h;
  rowlen = 3 * W;
  memcpy(data, tmp->data, w*h*3);
  delete tmp;
}

void CImg::writeP6(const char *path) {
  FILE *f;
  int i,j;
  hati *p;
  
  f = fopen(path,"w");
  if (!f) return;

  fprintf(f,"P6\n%d %d\n255\n",W,H);

  j = 3 * W * H;
  p = data;
  for(i=0;i<j;i++)
    fputc(*(p++),f);

  fclose(f);
  return;
}

void CImg::alloc(int w,int h) {
  W = w;
  H = h;
  rowlen = 3 * W;
  data = (hati *) malloc(rowlen * h * sizeof(hati));
  if (data==NULL) {
    ok = 0;
    return;
  }
  memset(data,0,h * rowlen);
  ok = 1;
}

CImg::~CImg() {
  if (data) {
    free(data);
    data = 0;
  }
}

void CImg::fill(int color) {
  int i;
  for(i=0;i<W;i++)
    set(i,0,color);
  for(i=0;i<H;i++)
    memcpy(data+i*rowlen, data, rowlen);
}

void CImg::set(int x,int y, int color) {
  hati r,g,b, *p;

  r=t0(color);
  g=t1(color);
  b=t2(color);
  
  p = data + (rowlen * y + 3 * x);
  *(p++) = r;
  *(p++) = g;
  *p = b;  
}

int  CImg::get(int x,int y) {
  hati r,g,b, *p;
  p = data + (rowlen * y + 3 * x);
  r = *(p++);
  g = *(p++);
  b = *p;
  return(triplet(r,g,b));
}

#define EPSILON 0.0001
CImg * CImg::alphaScale(rgbptr alpha, int nw,int nh, 
			bool extruded) 
{

  CImg *img, *a, *b;
  rgbptr s,d;
  hati   p, R,G,B;
  int i, q, z, bgcolor;
  int ew,eh,missing;

  a = new CImg(W,H);
  
  // make a fake rgb alpha mask
  s=alpha;
  d=a->data;  

  i=W * H;
  for(;i;i--) {
    p= *(s++) << 7;
    *(d++) = p;
    *(d++) = p;
    *(d++) = p;
  }

  R=G=B=0x80;
  bgcolor=triplet(R,G,B); 

  // substitute bgcolor
  i=W * H;
  d=data;
  z=triplet(*d,*(d+1),*(d+2));
  for(;i;i--) {
      q=triplet(*d,*(d+1),*(d+2));
      if (q==z) *d=*(d+1)=*(d+2)=R; 
      d+=3;
  }

  img=scale(nw,nh);
  b=a->scale(nw, nh);
  delete(a);

  // fix the last column before the squares
  d=img->data + 3* ((nw/7)*6 - 1);
  for(i=0;i<nh;i++) {
    *d     = R;
    *(d+1) = G;
    *(d+2) = B;
    d+=3*nw;
  }
  
  s=b->data;
  d=img->data;
  i=nw * nh * 3;

  for(;i;i-=3) {
    if ( (*s) > 0x60 ) {
      *(d) = R;
      *(d+1) = G;
      *(d+2) = B;
    }
    s+=3;
    d+=3;
  }

  // fix annoying border (d)effect on extruded sets
  if (extruded) {
    ew=nw/7;
    eh=nh/2;

    i=ew-1;
    missing=0;

    d=img->data+ 3* (nw*i + 6*ew + 2);
    while(i>0) {
      q=triplet(*d,*(d+1),*(d+2));

      if (q!=bgcolor)
	break;

      ++missing;
      --i;
      d-=3*nw;
    }

    if (missing < ew/2) {
      bitblt( img->data, img->data, // !!!
	      6*ew, ew-(2*missing), nw,       // src
	      6*ew, ew-missing    , nw,       // dest
	      ew, missing );
      
      bitblt( img->data, img->data, // !!!
	      6*ew, eh+ew-(2*missing), nw,     //src
	      6*ew, eh+ew-missing, nw,       //dest
	      ew, missing );
    }
  }
  delete(b);
  return(img);
}

// taken from gimp 1.0.4, scale_region(...) from app/paint_funcs.c
CImg * CImg::scale(int nw, int nh) {
  unsigned char * src_m1, * src, * src_p1, * src_p2;
  unsigned char * s_m1, * s, * s_p1, * s_p2;
  unsigned char * dest, * d;
  double * row, * r;
  int src_row, src_col;
  int bytes, b;
  int width, height;
  int orig_width, orig_height;
  double x_rat, y_rat;
  double x_cum, y_cum;
  double x_last, y_last;
  double * x_frac, y_frac, tot_frac;
  float dx, dy;
  int i, j;
  int frac;
  int advance_dest_x, advance_dest_y;
  int minus_x, plus_x, plus2_x;
  int scale_type;

  CImg *img;
  int stride;

  orig_width = W;
  orig_height = H;

  width = nw;
  height = nh;

  img = new CImg(nw, nh);
  if (!img) return 0;
  if (!img->ok) return 0;

  /*  Some calculations...  */
  bytes = 3;
  stride= orig_width*bytes;

  /*  the data pointers...  */
  src_m1 = (unsigned char *) g_malloc (orig_width * bytes);
  src    = (unsigned char *) g_malloc (orig_width * bytes);
  src_p1 = (unsigned char *) g_malloc (orig_width * bytes);
  src_p2 = (unsigned char *) g_malloc (orig_width * bytes);
  dest   = (unsigned char *) g_malloc (width * bytes);

  /*  find the ratios of old x to new x and old y to new y  */
  x_rat = (double) orig_width / (double) width;
  y_rat = (double) orig_height / (double) height;

  /*  determine the scale type  */
  if (x_rat < 1.0 && y_rat < 1.0)
    scale_type = 0; // MagnifyX_MagnifyY
  else if (x_rat < 1.0 && y_rat >= 1.0)
    scale_type = 1; // MagnifyX_MinifyY
  else if (x_rat >= 1.0 && y_rat < 1.0)
    scale_type = 2; // MinifyX_MagnifyY
  else
    scale_type = 3; // MinifyX_MinifyY

  /*  allocate an array to help with the calculations  */
  row    = (double *) g_malloc (sizeof (double) * width * bytes);
  x_frac = (double *) g_malloc (sizeof (double) * (width + orig_width));

  /*  initialize the pre-calculated pixel fraction array  */
  src_col = 0;
  x_cum = (double) src_col;
  x_last = x_cum;

  for (i = 0; i < width + orig_width; i++)
    {
      if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	{
	  x_cum += x_rat;
	  x_frac[i] = x_cum - x_last;
	}
      else
	{
	  src_col ++;
	  x_frac[i] = src_col - x_last;
	}
      x_last += x_frac[i];
    }
  
  /*  clear the "row" array  */
  memset (row, 0, sizeof (double) * width * bytes);
  
  /*  counters...  */
  src_row = 0;
  y_cum = (double) src_row;
  y_last = y_cum;

  /*  Get the first src row  */
  memcpy(src, data + src_row * stride, stride);

  /*  Get the next two if possible  */
  if (src_row < (orig_height - 1))
    memcpy(src_p1, data + (src_row + 1) * stride, stride);

  if ((src_row + 1) < (orig_height - 1))
    memcpy(src_p2, data + (src_row + 2) * stride, stride);

  /*  Scale the selected region  */
  i = height;
  while (i)
    {
      src_col = 0;
      x_cum = (double) src_col;

      /* determine the fraction of the src pixel we are using for y */
      if (y_cum + y_rat <= (src_row + 1 + EPSILON))
	{
	  y_cum += y_rat;
	  dy = y_cum - src_row;
	  y_frac = y_cum - y_last;
	  advance_dest_y = TRUE;
	}
      else
	{
	  y_frac = (src_row + 1) - y_last;
	  dy = 1.0;
	  advance_dest_y = FALSE;
	}

      y_last += y_frac;

      s = src;
      s_m1 = (src_row > 0) ? src_m1 : src;
      s_p1 = (src_row < (orig_height - 1)) ? src_p1 : src;
      s_p2 = ((src_row + 1) < (orig_height - 1)) ? src_p2 : s_p1;

      r = row;

      frac = 0;
      j = width;

      while (j)
	{
	  if (x_cum + x_rat <= (src_col + 1 + EPSILON))
	    {
	      x_cum += x_rat;
	      dx = x_cum - src_col;
	      advance_dest_x = TRUE;
	    }
	  else
	    {
	      dx = 1.0;
	      advance_dest_x = FALSE;
	    }
	  
	  tot_frac = x_frac[frac++] * y_frac;

	  minus_x = (src_col > 0) ? -bytes : 0;
	  plus_x = (src_col < (orig_width - 1)) ? bytes : 0;
	  plus2_x = ((src_col + 1) < (orig_width - 1)) ? bytes * 2 : plus_x;

	  switch (scale_type)
	    {
	    case 0: // MagnifyX_MagnifyY
	      for (b = 0; b < bytes; b++)
		r[b] += ((1 - dy) * ((1 - dx) * s[b] + dx * s[b+plus_x]) +
			 dy  * ((1 - dx) * s_p1[b] + dx * s_p1[b+plus_x])) * tot_frac;
	      break;
	    case 1: // MagnifyX_MinifyY
	      for (b = 0; b < bytes; b++)
		r[b] += (s[b] * (1 - dx) + s[b+plus_x] * dx) * tot_frac;
	      break;
	    case 2: // MinifyX_MagnifyY
	      for (b = 0; b < bytes; b++)
		r[b] += (s[b] * (1 - dy) + s_p1[b] * dy) * tot_frac;
	      break;
	    case 3: //MinifyX_MinifyY
	      for (b = 0; b < bytes; b++)
		r[b] += s[b] * tot_frac;
	      break;
	    }

	  if (advance_dest_x)
	    {
	      r += bytes;
	      j--;
	    }
	  else
	    {
	      s_m1 += bytes;
	      s    += bytes;
	      s_p1 += bytes;
	      s_p2 += bytes;
	      src_col++;
	    }
	}

      if (advance_dest_y)
	{
	  tot_frac = 1.0 / (x_rat * y_rat);

	  /*  copy "row" to "dest"  */
	  d = dest;
	  r = row;

	  j = width;
	  while (j--)
	    {
	      b = bytes;
	      while (b--)
		*d++ = (unsigned char) (*r++ * tot_frac);
	    }

	  /*  set the pixel region span  */
	  memcpy(img->data + (width * 3) * (height-i), dest, width * 3);

	  /*  clear the "row" array  */
	  memset (row, 0, sizeof (double) * width * bytes);

	  i--;
	}
      else
	{
	  /*  Shuffle pointers  */
	  s = src_m1;
	  src_m1 = src;
	  src = src_p1;
	  src_p1 = src_p2;
	  src_p2 = s;

	  src_row++;
	  if ((src_row + 1) < (orig_height - 1))
	    memcpy(src_p2, data + (src_row + 2) * stride, stride);
	}
    }

  /*  free up temporary arrays  */
  g_free (row);
  g_free (x_frac);
  g_free (src_m1);
  g_free (src);
  g_free (src_p1);
  g_free (src_p2);
  g_free (dest);

  return img;
}

// ----------- RGB <-> YCbCr transform

int ColorSpace::lighter(int triplet) {
  int a,b;
  a=RGB2YCbCr(triplet);
  b=pixr(a);
  b=(256+b)/2;
  b=YCbCr2RGB(pixel(b,pixg(a),pixb(a)));
  return b;
}

int ColorSpace::darker(int triplet) {
  int a,b;
  a=RGB2YCbCr(triplet);
  b=pixr(a);
  b/=2;
  b=YCbCr2RGB(pixel(b,pixg(a),pixb(a)));
  return b;
}

int ColorSpace::RGB2YCbCr(int triplet) {
  float y,cb,cr;

  y=0.257*(float)pixr(triplet)+
    0.504*(float)pixg(triplet)+
    0.098*(float)pixb(triplet)+16.0;

  cb=-0.148*(float)pixr(triplet)-
      0.291*(float)pixg(triplet)+
      0.439*(float)pixb(triplet)+
      128.0;

  cr=0.439*(float)pixr(triplet)-
     0.368*(float)pixg(triplet)-
     0.071*(float)pixb(triplet)+
     128.0;

  return(pixel((int)y,(int)cb,(int)cr));
}

int ColorSpace::YCbCr2RGB(int triplet) {
  float r,g,b;

  r=1.164*((float)pixr(triplet)-16.0)+
    1.596*((float)pixb(triplet)-128.0);

  g=1.164*((float)pixr(triplet)-16.0)-
    0.813*((float)pixb(triplet)-128.0)-
    0.392*((float)pixg(triplet)-128.0);
  
  b=1.164*((float)pixr(triplet)-16.0)+
    2.017*((float)pixg(triplet)-128.0);

  return(pixel((int)r,(int)g,(int)b));
}

int ColorSpace::pixr(int triplet) {
  return((triplet>>16)&0xff);
}

int ColorSpace::pixg(int triplet) {
  return((triplet>>8)&0xff);
}

int ColorSpace::pixb(int triplet) {
  return(triplet&0xff);
}

int ColorSpace::pixel(int r,int g,int b) {
  int v=((r&0xff)<<16)|((g&0xff)<<8)|(b&0xff);
  return(v);
}

void Blitter::bitblt(rgbptr src,rgbptr dest,int sx,int sy,int sw,
		     int dx,int dy,int dw,int w,int h) 
{
  register rgbptr d, s;
  register int wcd;
  static int slg,dlg, wcd0;
  d=dest+3*(dw*dy+dx);
  s=src+3*(sw*sy+sx);
  dlg=3*(dw-w);
  slg=3*(sw-w);
  wcd0=3*w;
  while(h) {
    wcd=wcd0;
    while(wcd--)
      *(d++)=*(s++);
    d+=dlg;
    s+=slg;
    --h;
  }
}

void Blitter::bitblt_a(rgbptr src,rgbptr dest,int sx,int sy,int sw,
		       int dx,int dy,int dw,int w,int h, rgbptr alpha) 
{
  register rgbptr d, s, a;
  static int wcd,slg,dlg,alg;
  s=src+3*(sw*sy+sx);
  d=dest+3*(dw*dy+dx);
  a=alpha+(sw*sy+sx);
  slg=3*(sw-w);
  dlg=3*(dw-w);
  alg=sw-w;
  while(h) {
    wcd=w;
    while(wcd--) {
      if (*a) {
	d+=3;
	s+=3;
      } else {
	*(d++)=*(s++);
	*(d++)=*(s++);
	*(d++)=*(s++);
      }
      ++a;
    }
    d+=dlg;
    s+=slg;
    a+=alg;
    --h;
  }
}
