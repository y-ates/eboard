/* $Id: pieces.h,v 1.25 2004/12/27 15:16:17 bergo Exp $ */

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



#ifndef PIECES_H
#define PIECES_H 1

#include "stl.h"
#include "eboard.h"
#include "util.h"
#include "cimg.h"

class PReq {
 public:
	PReq(piece a, rgbptr b, int c, int d, int e, bool f);
	int operator< (PReq * a);
	int operator> (PReq * a);

	piece  p;
	rgbptr dest;
	int    x, y, dwidth;
	bool   onlytop;
};

class PieceSet : public ColorSpace, public Blitter {
 public:
	PieceSet(string &name, string &sqname);
	PieceSet(PieceSet *src);
	~PieceSet();

	string &getName();
	string &getSquareName();

	int  side;
	bool extruded; // pieces can be taller than one square
	int  height;   // in extruded sets only

	void scale(int newside);

	void drawPiece(piece p,rgbptr dest,int x,int y,int dwidth);
	void drawSquare(int dark,rgbptr dest,int x,int y,int dwidth);

	// much faster than calling the previous two, since this class will
	// keep a cache of square+piece
	void drawPieceAndSquare(piece p,rgbptr dest,int x,int y,int dwidth,
							int sqdark);

	// fast fully masked draw
	void drawPiece(piece p,GdkPixmap *dest,GdkGC *gc,int x,int y);

	void drawOutlinedPiece(piece p,GdkPixmap *dest,GdkGC *gc,
						   int x,int y,bool white);

	// for extruded sets
	void beginQueueing();
	void endQueueing();

 private:
	string         Name;
	string         SqName;
	int            bgcolor;
	CImg          *isrc;
	rgbptr         alphamask;
	bool           bottom_only, top_only, queuef;
	list<PReq *>   myq; // looks like a list but it's a queue ;-)

	rgbptr xxcache[24];
	piece  cacheindex[256];

	void initCache();
	void buildCache();
	void clearCache();

	CImg *FallbackLoad();

	void loadSquares(string &path);

	void calcAlphaMask();

	void rect_outline(rgbptr img,int x,int y,int w,int h,int iw);

	int  getPixel(int x,int y);
	void setPixel(int x,int y,int v);
	void xSetPixel(rgbptr img,int x,int y,int v);
	int  alpha(int x,int y);

	void queueRequest(PReq *r);
};

class VecDetail {
 public:
	void add(int a,int b,int c,int d);
	void draw(GdkPixmap *dest, GdkGC *gc,int x,int y,int sz1,int sz2);
 private:
	vector<int> x1, y1, x2, y2;
};

class VectorPieces {
 public:
	VectorPieces();

	void drawSquares(GdkPixmap *dest,GdkGC *gc,int sqside, int dx=0, int dy=0);
	void drawPiece(GdkPixmap *dest,GdkGC *gc,int sqside,int x,int y,piece p);

 private:
	static GdkPoint srcpiece[376];
	GdkPoint vec[376];

	int orig_sqside;
	int cur_sqside;

	void rescale(int newsqside);
	int np[6];
	int offset[6];
	VecDetail details[6];
};

#endif
