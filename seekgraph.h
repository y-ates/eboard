/* $Id: seekgraph.h,v 1.17 2008/02/12 18:39:20 bergo Exp $ */

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

#ifndef SEEKGRAPH_H
#define SEEKGRAPH_H

#include "stl.h"
#include "eboard.h"
#include "widgetproxy.h"
#include "notebook.h"

class SeekAd {
 public:

	SeekAd();
	int operator==(int v);
	SeekAd &operator=(const SeekAd &b);

	int getRating();
	float getEtime();
	variant getKind();
	int distance(int px,int py);
	bool isComputer();

	const char **getListLine();

	int id;

	int  clock;
	int  incr;
	bool rated;
	bool automatic;
	bool formula;

	string color;
	string rating;
	string player;
	string range;
	string kind;
	string flags;

	int x,y,lx,ly,lw,lh,sw;

};

class SeekGraph2 : public WidgetProxy,
	public NotebookInsider {

 public:
	SeekGraph2();
	virtual ~SeekGraph2();

	void remove(int id);
	void add(SeekAd *ad);
	void clear();
	void updateFont();

	void draw();

 private:
	vector<SeekAd *> ads;
	LayoutBox L;
	GdkGC *gc;
	GdkPixmap *pix;
	int lw,lh,boxid,mx,my;

	SeekAd *getAd(int id);
	void rehover();
	void placeAds();

	bool rectFree(int x,int y,int w,int h);
	bool rectOverlap(int rx,int ry,int rw, int rh,
					 int sx,int sy,int sw, int sh);
	bool intervalOverlap(int a1, int a2, int b1, int b2);

	friend gboolean skg2_hover(GtkWidget *w, GdkEventMotion *ee,gpointer data);
	friend gboolean skg2_click(GtkWidget *w, GdkEventButton *be,gpointer data);
	friend gboolean skg2_expose(GtkWidget *w, GdkEventExpose *ee,gpointer data);
	friend gboolean skg2_cfg(GtkWidget *w, GdkEventConfigure *ee,gpointer data);

	};


#endif
