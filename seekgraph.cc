/* $Id: seekgraph.cc,v 1.31 2008/02/12 18:39:20 bergo Exp $ */

/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2008 Felipe Paulo Guazzi Bergo
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
#include <math.h>
#include <string.h>
#include "seekgraph.h"
#include "global.h"
#include "eboard.h"

gboolean skg2_expose(GtkWidget *w, GdkEventExpose *ee,gpointer data);
gboolean skg2_click(GtkWidget *w, GdkEventButton *be,gpointer data);
gboolean skg2_cfg(GtkWidget *w, GdkEventConfigure *ee,gpointer data);
gboolean skg2_hover(GtkWidget *w, GdkEventMotion *ee,gpointer data);
bool seekcmp_id(const SeekAd *a, const SeekAd *b);
bool seekcmp_pos(const SeekAd *a, const SeekAd *b);

bool seekcmp_id(const SeekAd *a, const SeekAd *b) {
  return (a->id < b->id);
}

bool seekcmp_pos(const SeekAd *a, const SeekAd *b) {
  if (a->y > b->y) return true;
  if (a->y < b->y) return false;
  return (a->x < b->x);
}

static void skg_refresh(GtkWidget *w, gpointer data) {
  if (global.protocol!=NULL)
    global.protocol->refreshSeeks(false);
}

static gint skg_sort_num(GtkCList *clist,
			 gconstpointer ptr1, gconstpointer ptr2)
{
  char *t1, *t2;
  int i1, i2;

  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;

  t1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
  t2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;

  i1 = atoi(t1);
  i2 = atoi(t2);

  if (i1<i2) return -1;
  if (i1>i2) return 1;
  return 0;
}

static gint skg_sort_time(GtkCList *clist,
			 gconstpointer ptr1, gconstpointer ptr2)
{
  char t1[20], t2[20];
  char *p;
  int i1, i2, j1, j2;

  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;

  g_strlcpy(t1, GTK_CELL_TEXT (row1->cell[clist->sort_column])->text, 20);
  g_strlcpy(t2, GTK_CELL_TEXT (row2->cell[clist->sort_column])->text, 20);
  
  p=strtok(t1, " \t");
  if (p) {
    i1=atoi(p);
    p=strtok(0," \t");
    if (p) j1=atoi(p); else j1=0;
  } else i1=j1=0;

  p=strtok(t2, " \t");
  if (p) {
    i2=atoi(p);
    p=strtok(0," \t");
    if (p) j2=atoi(p); else j2=0;
  } else i2=j2=0;

  if (i1<i2) return -1;
  if (i1>i2) return 1;
  if (j1<j2) return -1;
  if (j1>j2) return 1;
  return 0;
}


SeekAd::SeekAd() {
  id=32767;
  rated=false;
  automatic=false;
  formula=false;
  clock=incr=0;
  x=y=sw=lx=ly=lw=lh=-1;
}

SeekAd & SeekAd::operator=(const SeekAd &b) {
  id = b.id;
  clock = b.clock;
  incr = b.incr;
  rated = b.rated;
  automatic = b.automatic;
  formula = b.formula;

  color = b.color;
  rating = b.rating;
  player = b.player;
  range = b.range;
  kind = b.kind;
  flags = b.flags;
  x = b.x;
  y = b.y;
  return(*this);
}

int SeekAd::operator==(int v) {
  return(id==v);
}

int SeekAd::getRating() {
  return(atoi(rating.c_str()));
}

float SeekAd::getEtime() {
  return(clock + (2.0f*incr)/3.0f);
}

bool SeekAd::isComputer() {
  return(flags.find("(C)",0)!=string::npos);
}

int SeekAd::distance(int px,int py) {
  return((int)(sqrt( (px-x)*(px-x) + (py-y)*(py-y) )));
}

variant SeekAd::getKind() {
  if (!kind.compare("blitz") ||
      !kind.compare("lightning") ||
      !kind.compare("standard")) return REGULAR;
  if (!kind.compare("crazyhouse")) return CRAZYHOUSE;
  if (!kind.compare("suicide")) return SUICIDE;
  if (!kind.compare("giveaway")) return GIVEAWAY;
  if (!kind.compare("losers")) return LOSERS;
  if (!kind.compare("atomic")) return ATOMIC;
  return WILD;
}

SeekGraph2::SeekGraph2() {

  boxid = -1;
  widget = gtk_drawing_area_new();
  gc = NULL;
  pix = NULL;
  lw=lh=mx=my=-1;
  gtk_widget_set_events(widget,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_POINTER_MOTION_MASK);

  gtk_signal_connect (GTK_OBJECT (widget), "expose_event",
                      (GtkSignalFunc) skg2_expose, (gpointer) this);
  gtk_signal_connect (GTK_OBJECT (widget), "configure_event",
                      (GtkSignalFunc) skg2_cfg, (gpointer) this);
  gtk_signal_connect (GTK_OBJECT (widget), "button_press_event",
                      (GtkSignalFunc) skg2_click, (gpointer) this);
  gtk_signal_connect (GTK_OBJECT (widget), "motion_notify_event",
                      (GtkSignalFunc) skg2_hover, (gpointer) this);

}

SeekGraph2::~SeekGraph2() {
  clear();
  if (pix!=NULL)
    gdk_pixmap_unref(pix);
  if (gc!=NULL)
    gdk_gc_destroy(gc);
}

void SeekGraph2::rehover() {
  GdkEventMotion em;
  em.x = mx;
  em.y = my;
  boxid = -2;
  placeAds();
  skg2_hover(widget,&em,(gpointer)this);
}

void SeekGraph2::remove(int id) {
  unsigned int i,n;
  n = ads.size();
  for(i=0;i<n;i++)
    if (ads[i]->id == id) {
      delete(ads[i]);
      ads.erase(ads.begin() + i);
      break;
    }
  //  contentUpdated();
  rehover();
  repaint();
}

void SeekGraph2::add(SeekAd *ad) {
  ads.push_back(ad);
  //  contentUpdated();  
  rehover();
  repaint();

}

void SeekGraph2::clear() {
  unsigned int i,n;
  n = ads.size();
  for(i=0;i<n;i++)
    delete(ads[i]);
  ads.clear();
  boxid = -1;
  //  contentUpdated();
  rehover();
  repaint();
}

void SeekGraph2::updateFont() {
  unsigned int i,n;
  lw=-1;
  n = ads.size();
  for(i=0;i<n;i++) ads[i]->sw = -1;
  repaint();
}

gboolean skg2_click(GtkWidget *w, GdkEventButton *be,gpointer data) {

  SeekGraph2 *me = (SeekGraph2 *) data;
  char z[64];

  if (me->boxid >= 0 && global.protocol && be->button==1) {
    snprintf(z,64,"play %d",me->boxid);
    global.protocol->sendUserInput(z);
    snprintf(z,64,_("Replied to seek #%d"),me->boxid);
    global.status->setText(z,10);
  } else if (global.protocol && be->button==3) {
    global.protocol->refreshSeeks(false);
  }

  return TRUE;
}

gboolean skg2_cfg(GtkWidget *w, GdkEventConfigure *ee,gpointer data) {
  int ww,wh;
  SeekGraph2 *me = (SeekGraph2 *) data;
  LayoutBox *L;

  gdk_window_get_size(w->window, &ww, &wh);

  L = &(me->L);

  if (me->lw != ww || me->lh != wh) {
    if (me->pix != NULL) {
      gdk_pixmap_unref(me->pix);
      me->pix = NULL;
    }
    if (me->gc != NULL) {
      gdk_gc_destroy(me->gc);
      me->gc = NULL;
    }
  }

  if (me->pix == NULL) {
    me->pix = gdk_pixmap_new(w->window,ww,wh,-1);
    me->lw = ww;
    me->lh = wh;
    me->gc = gdk_gc_new(me->pix);
    L->prepare(w,me->pix,me->gc,0,0,ww,wh);
    L->setFont(0,global.SeekFont);
  }

  me->draw();
  return TRUE;
}

void SeekGraph2::placeAds() {
  unsigned int i,n;
  int j,lx,ly,qw,qh;
  int fh=0;

  n = ads.size();
  sort(ads.begin(),ads.end(),&seekcmp_id);
  for(i=0;i<n;i++)
    ads[i]->lx = ads[i]->lw = ads[i]->ly = ads[i]->lh = -1;

  if (gc!=NULL)
    fh = L.fontHeight(0);

  for(i=0;i<n;i++) {
    int rat;
    float etime;
    etime = ads[i]->getEtime();
    rat = ads[i]->getRating();
    if (etime > 60.0f) etime = 60.0f;
    if (rat > 3000) rat = 3000;
    if (rat < 200) rat = 200;
    ads[i]->y = lh - ((lh * rat) / 3000);
    ads[i]->x = (int)((sqrt(etime)*lw) / 8.0);

    // avoid dot overlaps
    for(j=0;j<(int)i;j++)
      if (ads[i]->distance(ads[j]->x,ads[j]->y) < 12) {
	ads[i]->y -= 12;
	j = -1;
      }
  }

  sort(ads.begin(),ads.end(),&seekcmp_pos);

  for(i=0;i<n;i++) {
    if (gc!=NULL) {
      if (ads[i]->sw < 0)
	qw = ads[i]->sw = L.stringWidth(0,ads[i]->player.c_str());
      else
	qw = ads[i]->sw;
      qh = fh;
      lx = ads[i]->x + 8;
      ly = ads[i]->y - fh/2;

      for(j=0;j<=200;j+=10)
	if (rectFree(lx,ly-j,qw,qh)) {
	  ads[i]->lx = lx;
	  ads[i]->ly = ly-j;
	  ads[i]->lw = qw;
	  ads[i]->lh = qh;
	  break;
	} else if (rectFree(lx,ly+j,qw,qh)) {
	  ads[i]->lx = lx;
	  ads[i]->ly = ly+j;
	  ads[i]->lw = qw;
	  ads[i]->lh = qh;
	  break;
	}
      if (ads[i]->lx < 0) {
	    ads[i]->lx = lx;
	    ads[i]->ly = ly;
	    ads[i]->lw = qw;
	    ads[i]->lh = qh;
      }

      
    }   
  }

  sort(ads.begin(),ads.end(),&seekcmp_id);
}

bool SeekGraph2::rectFree(int x,int y,int w,int h) {
  unsigned int i,n;
  n = ads.size();
  for(i=0;i<n;i++) {
    if (rectOverlap(x,y,w,h,ads[i]->x-4,ads[i]->y-4,9,9))
      return false;
    if (ads[i]->lx >= 0)
      if (rectOverlap(x,y,w,h,ads[i]->lx,ads[i]->ly,ads[i]->lw,ads[i]->lh))
	return false;
  }
  return true;
}

bool SeekGraph2::rectOverlap(int rx,int ry,int rw, int rh,
		 int sx,int sy,int sw, int sh) {

  return (intervalOverlap(rx,rx+rw,sx,sx+sw)&&
	  intervalOverlap(ry,ry+rh,sy,sy+sh));
}

bool SeekGraph2::intervalOverlap(int a1, int a2, int b1, int b2) {
  return ((a1 >= b1 && a1 <= b2) ||
	  (a2 >= b1 && a2 <= b2) ||
	  (b1 >= a1 && b1 <= a2) ||
	  (b2 >= a1 && b2 <= a2));
}

void SeekGraph2::draw() {
  int W,H,RW,x,y,lb,bs,lx;
  int a,fh,r;
  char z[32];
  unsigned int i,n;
  SeekAd *j;

  int bg[6] = { 0xffd0d0, 0xeec0c0,
		0xffe0d0, 0xeed0c0,
		0xffffe0, 0xeeeed0 };

  int bg2[6] = { 0xddb0b0, 0xeec0c0,
		 0xddc0b0, 0xeed0c0,
		 0xddddc0, 0xeeeed0 };


  W = lw; H = lh;

  RW = W;

  fh = L.fontHeight(0);

  // main panes
  L.setColor(0xffffff);
  L.drawRect(0,0,W,H,true);

  lb = (int)((sqrt(2.0) * RW)/8.0);
  bs = (int)((sqrt(15.0) * RW)/8.0);

  // lightning
  L.setColor(bg[0]);
  L.drawRect(0,0,lb,H,true);

  // blitz
  L.setColor(bg[2]);
  L.drawRect(lb,0,bs-lb,H,true);

  // standard
  L.setColor(bg[4]);
  L.drawRect(bs,0,RW-bs,H,true);


  for(a=500;a<3000;a+=1000) {
    y = H - ((a*H) / 3000);
    x = (H*500)/3000;
    L.setColor(bg[1]);
    L.drawRect(0,y,lb,x,true);
    L.setColor(bg[3]);
    L.drawRect(lb,y,bs-lb,x,true);
    L.setColor(bg[5]);
    L.drawRect(bs,y,RW-bs,x,true);
  }

  for(a=100;a<3000;a+=100) {
    y = H - ((a*H) / 3000);
    L.setColor(bg2[ (a/500)%2 != 0 ? 1 : 0 ]);
    L.drawLine(0,y,lb,y);
    L.setColor(bg2[ (a/500)%2 != 0 ? 3 : 2 ]);
    L.drawLine(lb,y,bs,y);
    L.setColor(bg2[ (a/500)%2 != 0 ? 5 : 4 ]);
    L.drawLine(bs,y,RW,y);
  }

  // box it

  // vertical scale
  L.setColor(0x444444);
  lx = 0;

  int hval[12] = { 1, 2, 3, 5, 10, 15, 20, 25, 30, 40, 50, 60 };

  for(a=0;a<12;a++) {
    x = (int)((sqrt(hval[a]) * RW)/8.0);
    if (x > lx) {
      snprintf(z,32,"%dm",hval[a]);
      L.drawString(x+2,H-fh,0,z);
      L.drawLine(x,H,x,H-fh);
      lx = x + L.stringWidth(0,z) + 10;
    }
  }

  // horizontal scale
  L.setColor(0x444444);
  for(a=500;a<3000;a+=500) {
    y = H - ((a*H) / 3000);
    snprintf(z,32,"%d",a);
    x = L.stringWidth(0,z);
    L.drawString(W-x-10,y-fh-2,0,z);
    L.drawLine(W-20,y,W,y);
  }

  L.setColor(0x884444);
  L.drawString(10,1,0,_("Left click to play, right click to refresh."));

  L.setColor(0);
  L.drawRect(0,0,RW,H,false);  

  placeAds();
  n = ads.size();
  for(i=0;i<n;i++) {
    L.setColor(0xaa9999);
    L.drawLine(ads[i]->x,ads[i]->y,ads[i]->lx,ads[i]->ly + fh/2);
    L.drawString(ads[i]->lx+2,ads[i]->ly,0,ads[i]->player.c_str());
  }

  for(i=0;i<n;i++) {
    variant v;

    v = ads[i]->getKind();
    switch(v) {
    case REGULAR: L.setColor(0x2222ee); break;
    case CRAZYHOUSE: L.setColor(0x00eedd); break;
    case SUICIDE: L.setColor(0xff2222); break;
    case LOSERS: 
    case GIVEAWAY: L.setColor(0x882222); break;
    case ATOMIC: L.setColor(0x22ff22); break;
    case WILD: L.setColor(0xffff00); break;
    default: L.setColor(0x888888);
    }

    r = (ads[i]->id == boxid) ? 6 : 4;

    bool isac = ads[i]->isComputer();
    
    if (isac)
      L.drawRect(ads[i]->x-r,ads[i]->y-r,2*r+1,2*r+1,true);
    else
      L.drawEllipse(ads[i]->x-r,ads[i]->y-r,2*r+1,2*r+1,true);
    L.setColor(ads[i]->rated ? 0 : 0xffffff);
    if (isac)
      L.drawRect(ads[i]->x-r,ads[i]->y-r,2*r+1,2*r+1,false);
    else
      L.drawEllipse(ads[i]->x-r,ads[i]->y-r,2*r+1,2*r+1,false);
  }

  if (boxid < 0) return;
  j = getAd(boxid);
  if (j==NULL) return;

  int bx,by,bw,bh;

  char lines[3][100];
  snprintf(lines[0],100,"%s%s %s",
	   j->player.c_str(),
	   j->flags.c_str(),
	   j->rating.c_str());
  snprintf(lines[1],100,"%d %d %s %s (%s)",
	   j->clock,j->incr,
	   j->rated ? _("rated") : 
	   _("unrated"),
	   j->kind.c_str(),
	   j->range.c_str());
  snprintf(lines[2],100,"%c/%c %s",
	   j->automatic ? '-' : 'm',
	   j->formula ? 'f' : '-',
	   j->color.c_str());
  x = L.stringWidth(0,lines[0]);
  y = L.stringWidth(0,lines[1]);
  if (x < y) x = y;
  y = L.stringWidth(0,lines[2]);
  if (x < y) x = y;

  bw = x+20;
  bh = 3*(fh+2)+2;
  if (j->x < RW/2) bx = j->x + 20; else bx = j->x - 20 - bw;
  by = j->y - bh/2;
  if (by < 0) by = 0;
  if (by + bh > H) by = H - bh;

  L.setColor(0);
  L.drawLine(j->x,j->y,bx+bw/2,by+bh/2);

  L.setColor(0x888877);
  L.drawRect(bx+5,by+5,bw,bh,true);
  L.setColor(0xffffff);
  L.drawRect(bx,by,bw,bh,true);
  L.setColor(0);
  L.drawRect(bx,by,bw,bh,false);

  L.drawString(bx+10,by+2,0,lines[0]);
  L.drawString(bx+10,by+2+(fh+2),0,lines[1]);
  L.drawString(bx+10,by+2+2*(fh+2),0,lines[2]);

}

SeekAd *SeekGraph2::getAd(int id) {
  unsigned int i,n;
  n = ads.size();
  for(i=0;i<n;i++)
    if (ads[i]->id == id)
      return(ads[i]);
  return NULL;
}

gboolean skg2_expose(GtkWidget *w, GdkEventExpose *ee,gpointer data) {
  SeekGraph2 *me = (SeekGraph2 *) data;

  if (me->pix == NULL) return FALSE;
  gdk_draw_pixmap(w->window,w->style->black_gc,
                  me->pix,
                  ee->area.x, ee->area.y,
                  ee->area.x, ee->area.y,
                  ee->area.width, ee->area.height);
  return TRUE;
}

gboolean skg2_hover(GtkWidget *w, GdkEventMotion *ee,gpointer data) {
  SeekGraph2 *me = (SeekGraph2 *) data;
  int id,md,d,x,y;
  unsigned int i,n;

  
  if (me->ads.empty()) {
    id = -1;
    if (id != me->boxid) {
      me->boxid = id;
      me->repaint();
    }
    return TRUE;
  }

  x = me->mx = (int) (ee->x);
  y = me->my = (int) (ee->y);

  md = me->ads[0]->distance(x,y);
  id = me->ads[0]->id;

  n = me->ads.size();
  for(i=0;i<n;i++) {
    d = me->ads[i]->distance(x,y);
    if (d < md) { md = d; id = me->ads[i]->id; }
  }

  if (md > 100)
    id = -1;
  
  if (id != me->boxid) {
    me->boxid = id;
    me->repaint();
  }

  return TRUE;
}
