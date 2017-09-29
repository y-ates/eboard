/* $Id: pieces.cc,v 1.37 2007/01/20 23:46:02 bergo Exp $ */

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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pieces.h"
#include "global.h"
#include "tstring.h"
#include "eboard.h"

#include "fallback.xpm"

PReq::PReq(piece a, rgbptr b, int c, int d, int e, bool f) {
  p=a; dest=b; x=c; y=d; dwidth=e; onlytop=f;
}

int PReq::operator< (PReq * a) { return(y < a->y); }
int PReq::operator> (PReq * a) { return(y > a->y); }

PieceSet::PieceSet(string &name, string &sqname) {
  EboardFileFinder eff;
  string wherever;
  char cpath[512];
  int i;

  // global.debug("PieceSet","PieceSet",name);

  Name=name;
  SqName=sqname;

  if (!eff.find(Name,wherever)) {
    cerr << _("<PieceSet::PieceSet> ** file not found: ") << name << endl;
    exit(2);
  }

  memset(cpath,0,512);
  wherever.copy(cpath,512);

  isrc = new CImg(cpath);  
  if (isrc==0 || !(isrc->ok)) {
    cerr << "<PieceSet::PieceSet>";
    cerr << _(" ** PNG LOAD FAILED: using internal low-res pieceset. It'll look ugly.\n");
    isrc=FallbackLoad();
  }

  // use squares from other file ?
  if (Name.compare(SqName)) {
    if (!eff.find(SqName, wherever))
      cerr << _("<PieceSet::PieceSet> ** file not found: ") << SqName << endl;
    else
      loadSquares(wherever);
  }

  side = height = isrc->W / 7;
  bottom_only=false;
  top_only=false;
  queuef=false;

  i = (isrc->W  * 10) / isrc->H;

  if ((i<34)||(i>36)) {
    extruded=true;
    height=isrc->H / 2;
  } else {
    extruded=false;
  }

  initCache();

  bgcolor= isrc->get(0,0);
  alphamask=0;
  calcAlphaMask();
}

PieceSet::PieceSet(PieceSet *src) {
  Name=src->Name;
  SqName=src->SqName;
  initCache();
  isrc=new CImg(src->isrc);
  side=src->side;
  height=src->height;
  extruded=src->extruded;
  queuef=src->queuef;
  bgcolor=src->bgcolor;
  alphamask=(rgbptr)g_malloc(side*height*14);
  memcpy(alphamask,src->alphamask,side*height*14);
  bottom_only=false;
  top_only=false;
}

PieceSet::~PieceSet() {
  list<PReq *>::iterator qi;
  if (alphamask)
    g_free(alphamask);
  delete(isrc);
  clearCache();

  if (!myq.empty()) {
    for(qi=myq.begin();qi!=myq.end();qi++)
      delete(*qi);
    myq.clear();
  }
}

string & PieceSet::getName() {
  return(Name);
}

string & PieceSet::getSquareName() {
  return(SqName);
}

void PieceSet::loadSquares(string &path) {
  CImg *tmp,*aux;
  char cpath[512];

  int w1,w2,h1,h2;
  int x,w;

  memset(cpath,0,512);
  path.copy(cpath,512);

  global.debug("PieceSet","loadSquares",cpath);

  tmp = new CImg(cpath);

  if (tmp==0 || !(tmp->ok)) {
    cerr << "<PieceSet::loadSquares>";
    cerr << _(" ** PNG LOAD FAILED: using internal low-res pieceset. It'll look ugly.\n");
    tmp=FallbackLoad();
  }

  w1=isrc->W;
  w2=tmp->W;

  h1=isrc->H;
  h2=tmp->H;

  w=w2/7;

  bitblt(tmp->data, tmp->data,
	 6*w, 0, w2,
	 0, 0, w2,
	 w, w);

  bitblt(tmp->data, tmp->data,
	 6*w, h2/2, w2,
	 0, w, w2,
	 w, w);

  tmp->crop(0,0,w,2*w);

  if ( (w1!=w2) ) {
    w=w1/7;
    aux = tmp->scale(w,2*w);
    delete(tmp);
    tmp = aux;
  }

  x=(w1/7)*6;
  w=(w1/7);

  // white square
  bitblt(tmp->data,isrc->data,
	 0,0,w,
	 x,0,w1, 
	 w,w);

  // black square
  bitblt(tmp->data,isrc->data,
	 0, w, w,
	 x, isrc->H / 2, w1, 
	 w,w);

  delete(tmp);
}

void PieceSet::initCache() {
  int i;
  xxcache[0]=0;
  for(i=0;i<256;i++)
    cacheindex[i]=0;
  cacheindex[WHITE|PAWN]=0;
  cacheindex[WHITE|ROOK]=1;
  cacheindex[WHITE|KNIGHT]=2;
  cacheindex[WHITE|BISHOP]=3;
  cacheindex[WHITE|QUEEN]=4;
  cacheindex[WHITE|KING]=5;
  cacheindex[BLACK|PAWN]=6;
  cacheindex[BLACK|ROOK]=7;
  cacheindex[BLACK|KNIGHT]=8;
  cacheindex[BLACK|BISHOP]=9;
  cacheindex[BLACK|QUEEN]=10;
  cacheindex[BLACK|KING]=11;
}

void PieceSet::buildCache() {
  int i;
  int itemlen;
  rgbptr bp;

  itemlen=side*side*3;

  bp=(rgbptr)g_malloc(itemlen*24);

  clearCache();
  for(i=0;i<24;i++)
    xxcache[i]=bp+i*itemlen;

  // empty sqs
  drawSquare(0,xxcache[0],0,0,side);
  drawSquare(1,xxcache[12],0,0,side);
  
  for(i=1;i<12;i++) {
    memcpy(xxcache[i],xxcache[0],itemlen);
    memcpy(xxcache[i+12],xxcache[12],itemlen);
  }

  bottom_only=true;
  for(i=0;i<24;i+=12) {
    drawPiece(WHITE|PAWN,  xxcache[i],0,0,side);
    drawPiece(WHITE|ROOK,  xxcache[i+1],0,0,side);
    drawPiece(WHITE|KNIGHT,xxcache[i+2],0,0,side);
    drawPiece(WHITE|BISHOP,xxcache[i+3],0,0,side);
    drawPiece(WHITE|QUEEN, xxcache[i+4],0,0,side);
    drawPiece(WHITE|KING,  xxcache[i+5],0,0,side);    
    drawPiece(BLACK|PAWN,  xxcache[i+6],0,0,side);
    drawPiece(BLACK|ROOK,  xxcache[i+7],0,0,side);
    drawPiece(BLACK|KNIGHT,xxcache[i+8],0,0,side);
    drawPiece(BLACK|BISHOP,xxcache[i+9],0,0,side);
    drawPiece(BLACK|QUEEN, xxcache[i+10],0,0,side);
    drawPiece(BLACK|KING,  xxcache[i+11],0,0,side);
  }
  bottom_only=false;
}

void PieceSet::clearCache() {
  if (xxcache[0]!=0) {
    g_free(xxcache[0]);
    xxcache[0]=0;
  }
}

void PieceSet::scale(int newside) {
  CImg *ni;
  float factor;
  int newheight;

  //  cerr << "scaling from " << side << " to " << newside << endl;

  factor = ((float)newside) / ((float)side);

  if (extruded)
    newheight = (int) (factor * height);
  else
    newheight = newside;

  ni = isrc->alphaScale(alphamask,newside*7,newheight*2,extruded);
  bgcolor = 0x808080;

  side=newside;
  height=newheight;

  delete(isrc);

  isrc=ni;
  calcAlphaMask();

  buildCache();
}

void PieceSet::calcAlphaMask() {
  int c;
  unsigned char r,g,b;
  rgbptr t,am;
  if (alphamask!=0)
    g_free(alphamask);

  c=side*height*14;

  alphamask=(rgbptr)g_malloc(c);
  memset(alphamask,0,c);
  
  t=isrc->data;
  am=alphamask;

  r=bgcolor>>16;
  g=(bgcolor>>8)&0xff;
  b=bgcolor&0xff;

  while(c) {
    if ( (t[0] == r) &&
	 (t[1] == g) &&
	 (t[2] == b) )
      *am=1;
    ++am;
    t+=3;
    --c;
  }
}

void PieceSet::drawPiece(piece p, GdkPixmap *dest, GdkGC *gc,
			 int x, int y) {
  int sx=0,sy=0,S;
  int i,j,k=0,ai;

  unsigned char buf[3072]; // enough for 1024 x N pieces
  int bufcount=0;
  rgbptr r,q=0,a;

  p&=CUTFLAGS;
  if (p==EMPTY) return;

  if (p&BLACK) sy=height;
  sx=side*((p&PIECE_MASK)-1);

  if (extruded)
    y-=(height-side);

  S=0;
  r=buf;

  a=alphamask+7*side*sy+sx;
  ai=6*side;

  for(i=0;i<height;i++) {

    r=isrc->data+3*(7*side*(i+sy)+sx); // piece
    S=0;

    for(j=0;j<side;j++) {
      // 2 states: 0: skipping transparent
      //           1: accumulating pixel strip

      switch(S) {
      case 0:
	if (*a) break;

	S=1;

	k=j;
	q=buf;
	*(q++) = *(r);
	*(q++) = *(r+1);
	*(q++) = *(r+2);
	bufcount=1;

	break;
      case 1:
	if (*a) {
	  S=0;
	  // commit pixel strip
	  gdk_draw_rgb_image(dest,gc,x+k,y+i,bufcount,1,
			     GDK_RGB_DITHER_NORMAL,
			     buf,
			     side * 3);
	} else {

	  *(q++) = *(r);
	  *(q++) = *(r+1);
	  *(q++) = *(r+2);
	  ++bufcount;

	}

	break;
      }

      r+=3;
      ++a;

    } // for j

    if (S==1)
      gdk_draw_rgb_image(dest,gc,x+k,y+i,bufcount,1,
			 GDK_RGB_DITHER_NORMAL,
			 buf,
			 side * 3);

    a+=ai;
    
  } // for i

}

void PieceSet::drawOutlinedPiece(piece p,GdkPixmap *dest,GdkGC *gc,
				 int x,int y,bool white)
{
  int sx=0,sy=0,S;
  int i,j,k=0,ai;

  unsigned char buf[3072]; // enough for 1024 x N pieces
  unsigned char outline[3072];
  int bufcount=0;
  rgbptr r,q=0,a;

  p&=CUTFLAGS;
  if (p==EMPTY) return;

  if (p&BLACK) sy=height;
  sx=side*((p&PIECE_MASK)-1);

  memset(outline,white?0xff:0,3072);

  if (extruded)
    y-=(height-side);

  S=0;
  r=buf;

  a=alphamask+7*side*sy+sx;
  ai=6*side;

  for(i=0;i<height;i++) {

    r=isrc->data+3*(7*side*(i+sy)+sx); // piece
    S=0;

    for(j=0;j<side;j++) {
      // 2 states: 0: skipping transparent
      //           1: accumulating pixel strip

      switch(S) {
      case 0:
	if (*a) break;

	S=1;

	k=j;
	q=buf;
	*(q++) = *(r);
	*(q++) = *(r+1);
	*(q++) = *(r+2);
	bufcount=1;

	break;
      case 1:
	if (*a) {
	  S=0;
	  // border
	  gdk_draw_rgb_image(dest,gc,x+k-1,y+i,bufcount+2,1,
			     GDK_RGB_DITHER_NORMAL,
			     outline,
			     side * 3);	  

	  // commit pixel strip
	  gdk_draw_rgb_image(dest,gc,x+k,y+i,bufcount,1,
			     GDK_RGB_DITHER_NORMAL,
			     buf,
			     side * 3);
	} else {

	  *(q++) = *(r);
	  *(q++) = *(r+1);
	  *(q++) = *(r+2);
	  ++bufcount;

	}

	break;
      }

      r+=3;
      ++a;

    } // for j

    if (S==1) {
      gdk_draw_rgb_image(dest,gc,x+k-1,y+i,bufcount+2,1,
			 GDK_RGB_DITHER_NORMAL,
			 outline,
			 side * 3);	  
      gdk_draw_rgb_image(dest,gc,x+k,y+i,bufcount,1,
			 GDK_RGB_DITHER_NORMAL,
			 buf,
			 side * 3);
    }

    a+=ai;
    
  } // for i

}

void PieceSet::drawPiece(piece p,rgbptr dest,int x,int y,int dwidth) {
  int sx=0, sy=0;
  int top, sc=0, hlt=0;
  p&=CUTFLAGS;
  if (p==EMPTY) return;

  if (queuef) {
    queueRequest(new PReq(p,dest,x,y,dwidth,false));
    return;
  }

  if (p&BLACK)  sy=height;
  sx=side*((p&PIECE_MASK)-1);

  if (extruded) {
    if (bottom_only) {
      sc=height-side; top=y;
    } else {
      top=y-(height-side);
      if (top < 0) { sc = -top; top=0; }
    }

    if (top_only) hlt=height-side;
    if (sc<height)
      bitblt_a(isrc->data,dest,
	       sx,sy+sc,7*side,
	       x,top,dwidth,
	       side,height-sc-hlt,alphamask);
  } else
    bitblt_a(isrc->data,dest,sx,sy,7*side,x,y,dwidth,side,side,alphamask);    
}


void PieceSet::drawSquare(int dark,rgbptr dest,int x,int y,int dwidth) {
  int sx,sy=0;

  if (global.PlainSquares) {
    rgbptr sq;
    unsigned char r,g,b;
    int i,j,lo;
    r=(unsigned char)((dark?global.DarkSqColor:global.LightSqColor)>>16);
    g=(unsigned char)(0xff&((dark?global.DarkSqColor:global.LightSqColor)>>8));
    b=(unsigned char)((dark?global.DarkSqColor:global.LightSqColor)&0xff);
    sq=dest+3*(dwidth*y+x);
    lo=3*(dwidth-side);
    for(i=side;i;i--) {
      for(j=side;j;j--) {
	*(sq++)=r; *(sq++)=g; *(sq++)=b;
      }
      sq+=lo;
    }    
  } else {
    if (dark) sy=height;
    sx=6*side;  
    bitblt(isrc->data,dest,sx,sy,7*side,x,y,dwidth,side,side);
  }
  rect_outline(dest,x,y,side+1,side+1,dwidth);
}

void PieceSet::drawPieceAndSquare(piece p,rgbptr dest,
				  int x,int y,int dwidth,
				  int sqdark) {
  int i;

  if (global.PlainSquares) {
    drawSquare(sqdark,dest,x,y,dwidth);
    drawPiece(p,dest,x,y,dwidth);
    return;
  }

  p&=CUTFLAGS;

  if ((p==EMPTY)||(p==NotAPiece)) {
    drawSquare(sqdark,dest,x,y,dwidth);
    return;
  }
  if (!xxcache[0])
    buildCache();
  i=cacheindex[p]+(sqdark?12:0);
  bitblt(xxcache[i],dest,0,0,side,x,y,dwidth,side,side);

  if (queuef)
    queueRequest(new PReq(p,dest,x,y,dwidth,true));
}

void PieceSet::rect_outline(rgbptr img,int x,int y,int w,int h,int iw) {
  int i;
  memset(img+3*(iw*y+x),0,3*w);
  memset(img+3*(iw*(y+h-1)+x),0,3*w);
  for(i=0;i<h;i++) {
    memset(img+3*(iw*(y+i)+x),0,3);
    memset(img+3*(iw*(y+i)+x+w-1),0,3);
  }  
}

int  PieceSet::getPixel(int x,int y) {
  rgbptr src;
  src=isrc->data+(y*(isrc->W)+x)*3;
  return(pixel(src[0],src[1],src[2]));
}

void PieceSet::setPixel(int x,int y,int v) {
  rgbptr src;
  src=isrc->data+(y*(isrc->W)+x)*3;
  src[0]=(unsigned char)pixr(v);
  src[1]=(unsigned char)pixg(v);
  src[2]=(unsigned char)pixb(v);
}

void PieceSet::xSetPixel(rgbptr img,int x,int y,int v) {
  rgbptr src;
  src=img+(y*(isrc->W)+x)*3;
  src[0]=(unsigned char)pixr(v);
  src[1]=(unsigned char)pixg(v);
  src[2]=(unsigned char)pixb(v);
}

int PieceSet::alpha(int x,int y) {
  rgbptr a;
  if (!alphamask) return 0;
  a=alphamask+(y*(isrc->W)+x);
  return( (int)(a[0]) );
}

void PieceSet::queueRequest(PReq *r) {
  list<PReq *>::iterator qi, qj;

  if (myq.empty()) { myq.push_back(r); return; }

  for(qi=myq.begin();qi!=myq.end();qi=qj) {
    qj=qi; ++qj;
    if (qj==myq.end())  { myq.push_back(r); return; }
    if ( (*r) < (*qj) ) { myq.insert(qj,r); return; }
  }
}

void PieceSet::beginQueueing() {
  queuef=extruded;
}

void PieceSet::endQueueing() {
  list<PReq *>::iterator qi;
  if (!queuef) return;
  queuef=false;
  for(qi=myq.begin();qi!=myq.end();qi++) {
    top_only=(*qi)->onlytop;
    drawPiece((*qi)->p, (*qi)->dest, (*qi)->x, (*qi)->y, (*qi)->dwidth);
    delete(*qi);
  }
  myq.clear();
  top_only=false;
}


CImg * PieceSet::FallbackLoad() {
  tstring t;
  int w,h,c,i,j;
  rgbptr data, p;
  CImg *img;
  string *s;

  int cmap[256];
  int rgb;
  
  t.set(fallback_xpm[0]);

  w=t.tokenvalue(" \t");
  h=t.tokenvalue(" \t");
  c=t.tokenvalue(" \t");

  for(i=0;i<c;i++) {
    t.set(fallback_xpm[i+1]);
    t.token("\t");
    t.token(" ");
    s=t.token("# \t");
    if (s)
      rgb=strtol(s->c_str(),0,16);
    else
      rgb=0;
    cmap[fallback_xpm[i+1][0]]=rgb;
  }

  data=(rgbptr)g_malloc(w*h*3);

  p=data;
  for(j=0;j<h;j++)
    for(i=0;i<w;i++) {
      *(p++)=pixr(cmap[fallback_xpm[1+c+j][i]]);
      *(p++)=pixg(cmap[fallback_xpm[1+c+j][i]]);
      *(p++)=pixb(cmap[fallback_xpm[1+c+j][i]]);
    }

  img = new CImg(w,h);
  memcpy(img->data, data, h*(img->rowlen));
  g_free(data);
  return img;
}

// ------------------------------ vectorized pieces

// offsets 32 67 90 76 65 46 
// sum = 376
// these data were generated automatically. Changing it by
// hand is *stupid*

GdkPoint VectorPieces::srcpiece[376]={
{35,16},{21,16},{21,31},{29,39},{29,71},{27,75},{23,79},{22,83},{22,90},{17,90},{15,93},{15,97},{17,99},{89,99},{91,97},{91,93},{89,90},{84,90},{84,83},{83,79},{79,75},{77,71},{77,39},{85,31},{85,16},{71,16},{71,22},{60,22},{60,16},{46,16},{46,22},{35,22},
{29,10},{28,14},{29,17},{29,21},{28,24},{25,28},{23,32},{20,44},{17,48},{14,52},{12,56},{10,60},{9,64},{9,68},{10,71},{12,73},{15,74},{18,75},{22,75},{23,78},{26,78},{28,76},{30,72},{38,64},{40,64},{43,61},{45,61},{48,58},{50,58},{53,56},{55,54},{56,57},{55,61},{54,65},{51,69},{48,73},{44,77},{41,81},{37,85},{35,89},{34,93},{33,97},{35,99},{95,99},{98,97},{98,89},{97,85},{97,77},{96,73},{96,69},{94,61},{92,53},{90,49},{89,45},{87,41},{84,37},{81,33},{74,25},{70,23},{66,21},{62,20},{54,18},{49,10},{46,13},{44,17},{41,20},{37,16},
{53,7},{48,10},{47,14},{48,17},{50,19},{47,22},{43,24},{39,27},{35,30},{31,34},{29,38},{27,42},{27,46},{27,50},{28,53},{29,56},{31,58},{32,61},{34,63},{36,65},{35,69},{33,74},{32,77},{32,80},{35,81},{38,82},{42,82},{45,83},{48,83},{47,86},{40,88},{20,88},{12,90},{8,92},{8,94},{10,96},{11,99},{13,100},{15,100},{18,99},{22,98},{30,98},{33,99},{41,99},{44,98},{47,96},{49,96},{52,94},{54,94},{57,95},{59,97},{65,99},{73,99},{76,98},{88,98},{91,99},{93,101},{95,99},{97,95},{96,91},{92,89},{88,88},{68,88},{60,86},{59,83},{62,83},{66,82},{69,82},{73,81},{75,79},{73,74},{71,69},{69,65},{71,63},{73,61},{75,59},{77,55},{78,51},{79,47},{79,43},{77,39},{75,35},{71,31},{67,27},{63,24},{59,22},{56,19},{58,17},{59,13},{57,9},
{51,7},{48,10},{47,14},{49,16},{51,18},{43,54},{31,22},{34,18},{34,14},{30,11},{26,12},{23,15},{24,18},{25,21},{28,22},{27,57},{12,33},{13,29},{14,25},{11,21},{7,20},{3,22},{2,26},{3,29},{5,31},{8,32},{10,34},{15,65},{21,71},{25,83},{22,88},{19,95},{28,98},{32,98},{35,99},{39,99},{42,100},{62,100},{65,99},{69,99},{73,98},{79,98},{83,97},{87,95},{84,88},{81,83},{83,71},{89,65},{96,33},{98,32},{101,31},{103,29},{104,25},{101,21},{97,20},{93,22},{92,26},{93,29},{94,32},{78,56},{78,23},{80,22},{82,18},{82,14},{78,11},{74,12},{71,15},{72,18},{73,21},{75,23},{62,54},{54,18},{57,17},{59,13},{57,9},{53,7},
{50,8},{51,13},{46,12},{46,17},{51,16},{51,23},{49,26},{46,29},{44,37},{43,41},{37,37},{31,35},{27,35},{23,35},{19,36},{15,38},{11,41},{9,45},{7,49},{7,53},{8,56},{9,59},{11,61},{12,64},{18,70},{21,71},{22,74},{22,82},{23,85},{23,93},{38,98},{41,98},{45,99},{57,99},{61,98},{64,98},{68,97},{83,92},{83,80},{84,76},{84,72},{87,70},{95,62},{97,60},{98,56},{99,52},{98,48},{96,44},{93,40},{89,37},{85,36},{81,35},{77,35},{73,36},{67,39},{62,41},{61,37},{59,29},{56,26},{54,23},{54,16},{59,17},{59,12},{54,13},{55,8},
{50,15},{46,17},{43,21},{42,25},{42,29},{45,33},{40,35},{36,39},{35,43},{34,47},{34,51},{35,54},{36,57},{38,59},{40,61},{34,65},{29,70},{27,74},{23,82},{22,86},{21,90},{21,94},{22,97},{25,98},{81,98},{85,97},{85,93},{84,85},{83,81},{80,74},{77,69},{72,64},{67,61},{69,59},{71,55},{72,51},{72,47},{71,43},{69,39},{65,35},{61,33},{64,29},{64,25},{62,21},{59,17},{55,15},
};

VectorPieces::VectorPieces() {
  int i,j;
  orig_sqside=cur_sqside=108;

  np[0]=32; np[1]=67; np[2]=90; np[3]=76; np[4]=65; np[5]=46;

  for(i=0;i<6;i++) {
    offset[i]=0;
    for(j=0;j<i;j++)
      offset[i]+=np[j];
  }

  // rook
  details[0].add(21,31,85,31);
  details[0].add(23,79,83,79);
  details[0].add(27,75,79,75);
  details[0].add(29,39,77,39);
  details[0].add(22,90,84,90);

  // knight
  details[1].add(29,42,34,35);
  details[1].add(34,35,38,34);
  details[1].add(38,34,35,39);
  details[1].add(34,39,29,42);
  details[1].add(41,20,34,26);
  details[1].add(22,75,25,68);

  // bishop
  details[2].add(53,35,53,54);
  details[2].add(44,44,62,44);
  details[2].add(48,83,59,83);
  details[2].add(36,65,53,63);
  details[2].add(53,63,69,65);
  details[2].add(33,74,53,72);
  details[2].add(53,72,73,74);

  // queen
  details[3].add(19,95,53,88);
  details[3].add(53,88,87,95);
  details[3].add(25,83,53,76);
  details[3].add(53,76,81,83);

  // king
  details[4].add(44,40,53,58);
  details[4].add(53,58,53,68);
  details[4].add(62,40,53,58);
  details[4].add(23,73,53,68);
  details[4].add(53,68,83,73);
  details[4].add(24,92,53,86);
  details[4].add(53,86,82,92);

  rescale(108);
}

void VectorPieces::drawSquares(GdkPixmap *dest,GdkGC *gc,int sqside, 
                               int dx, int dy) 
{
  int i,j;
  for(i=0;i<8;i++)
    for(j=0;j<8;j++) {      
      gdk_rgb_gc_set_foreground(gc,((j+i)&0x01)?global.DarkSqColor:global.LightSqColor);
      gdk_draw_rectangle(dest,gc,TRUE,dx+i*sqside,dy+j*sqside,sqside,sqside);
    }
  gdk_rgb_gc_set_foreground(gc,0);
  for(i=0;i<8;i++) {
    gdk_draw_line(dest,gc,dx,dy+i*sqside,dx+8*sqside,dy+i*sqside);
    gdk_draw_line(dest,gc,dx+i*sqside,dy,dx+i*sqside,dy+8*sqside);
  }
}

void VectorPieces::drawPiece(GdkPixmap *dest,GdkGC *gc,int sqside,
			      int x,int y,piece p) {
  int i,v;
  piece rp,pc;
  //GdkGCValues gcval;

  if ((p&CUTFLAGS)==EMPTY)
    return;

  if (sqside!=cur_sqside)
    rescale(sqside);

  rp=p&PIECE_MASK;
  pc=p&COLOR_MASK;

  switch(rp) {
  case ROOK:   v=0; break;
  case KNIGHT: v=1; break;
  case BISHOP: v=2; break;
  case QUEEN:  v=3; break;
  case KING:   v=4; break;
  case PAWN:   v=5; break;
  default:
    return;
  }

  for(i=0;i<np[v];i++) {
    vec[offset[v]+i].x+=x;
    vec[offset[v]+i].y+=y;
  }

  /*
  gdk_gc_get_values(gc,&gcval);
  gdk_gc_set_line_attributes(gc,2,gcval.line_style,
			     GDK_CAP_ROUND,GDK_JOIN_ROUND);
  */
  gdk_rgb_gc_set_foreground(gc,(pc==WHITE)?0xffffff:0);
  gdk_draw_polygon(dest,gc,TRUE,&vec[offset[v]],np[v]);
  gdk_rgb_gc_set_foreground(gc,(pc==BLACK)?0xffffff:0);
  gdk_draw_polygon(dest,gc,FALSE,&vec[offset[v]],np[v]);
  
  details[v].draw(dest,gc,x,y,cur_sqside,orig_sqside);

  /*
  gdk_gc_set_line_attributes(gc,gcval.line_width,gcval.line_style,
			     gcval.cap_style,gcval.join_style);
  */

  for(i=0;i<np[v];i++) {
    vec[offset[v]+i].x-=x;
    vec[offset[v]+i].y-=y;
  }
}

void VectorPieces::rescale(int newsqside) {
  float factor;
  int i;
  factor=((float)newsqside)/((float)orig_sqside);
  for(i=0;i<376;i++) {
    vec[i].x = (int) ( ( (float) (srcpiece[i].x) ) * factor);
    vec[i].y = (int) ( ( (float) (srcpiece[i].y) ) * factor);
  }
  cur_sqside=newsqside;
}

// ----------------------------

void VecDetail::add(int a,int b,int c,int d) {
  x1.push_back(a);
  y1.push_back(b);
  x2.push_back(c);
  y2.push_back(d);
}

void VecDetail::draw(GdkPixmap *dest, GdkGC *gc,int x,int y,int sz1,int sz2) {
  int i,j;
  j=x1.size();
  for(i=0;i<j;i++)
    gdk_draw_line(dest,gc,
		  x+(x1[i]*sz1)/sz2,y+(y1[i]*sz1)/sz2,
		  x+(x2[i]*sz1)/sz2,y+(y2[i]*sz1)/sz2);
}

