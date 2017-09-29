/* $Id: cimg.h,v 1.1 2004/12/27 15:16:17 bergo Exp $ */

/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2005 Felipe Paulo Guazzi Bergo
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

#ifndef CIMG_H
#define CIMG_H 1

#include "eboard.h" // hati type

#define t0(a) (((a)>>16)&0xff)
#define t1(a) (((a)>>8)&0xff)
#define t2(a) ((a)&0xff)
#define triplet(a,b,c) ((((a)&0xff)<<16)|(((b)&0xff)<<8)|((c)&0xff))

class Blitter {
 public:
  void bitblt(rgbptr src,rgbptr dest,int sx,int sy,int sw,
	      int dx,int dy,int dw,int w,int h);
  void bitblt_a(rgbptr src,rgbptr dest,int sx,int sy,int sw,
		int dx,int dy,int dw,int w,int h, rgbptr alpha);
};

class ColorSpace {
 public:

  int RGB2YCbCr(int triplet);
  int YCbCr2RGB(int triplet);
  int lighter(int triplet);
  int darker(int triplet);

  int pixr(int triplet);
  int pixg(int triplet);
  int pixb(int triplet);
  int pixel(int r,int g,int b);

};

class CImg : public Blitter {
 public:
  CImg(int w,int h);
  CImg(const char *filename);
  CImg(CImg *src);
  virtual ~CImg();

  void fill(int color);
  void set(int x,int y, int color);
  int  get(int x,int y);
  void crop(int x,int y,int w,int h);

  CImg *alphaScale(rgbptr alpha, int nw,int nh, bool extruded);
  CImg *scale(int nw, int nh);

  void  writeP6(const char *path);

  int W,H;
  int rowlen;
  hati *data;
  int ok;

 private:
  void alloc(int w,int h);
};

#endif
