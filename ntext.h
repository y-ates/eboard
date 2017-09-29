/* $Id: ntext.h,v 1.14 2008/02/22 00:57:18 bergo Exp $ */

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

#ifndef NTEXT_H
#define NTEXT_H 1

#include "widgetproxy.h"
#include "stl.h"

// line index
class NLine {
 public:
  NLine();
  NLine(const char *text, int color, int maxlen=-1);
  virtual ~NLine();

  char  *Text;
  int    NBytes;
  int    Color;
  int    Width;
  time_t Timestamp;
};

// secondary line index
class FLine : public NLine {
 public:
  FLine();
  FLine(const char *text, int color, int maxlen=-1, time_t stamp=0);
  FLine(NLine *src);
  void setSource(int src,int off);

  int   Src,Off;
  int   X,Y,H; // position of last rendering
  bool  valid; // visible ?
  char  stamp[12];
 private:
  void makeStamp();
};

// a point in the text (a selection has two of this)
class TPoint {
 public:
  TPoint();
  TPoint &operator=(TPoint &src);
  int     operator<(TPoint &src);
  int     operator>(TPoint &src);
  int     operator==(TPoint &src);

  int     operator<(int i);
  int     operator<=(int i);
  int     operator>(int i);
  int     operator>=(int i);
  int     operator==(int i);

  int     LineNum, ByteOffset;
};

// a decent text buffer widget
class NText : public WidgetProxy {
 public:
  NText();
  virtual ~NText();

  void setFont(const char *font);
  void append(const char *text, int len, int color);
  void pageUp(float pages);
  void pageDown(float pages);

  void lineUp(int n);
  void lineDown(int n);

  void gotoLine(int n);

  void setScrollBack(int n);
  void setBG(int c);

  bool findTextUpward(int top, int bottom, const char *needle, 
		      bool select=true);
  int  getLastFoundLine();
  
  bool saveTextBuffer(const char *path);


  virtual void repaint();
  void scheduleRepaint(int latency=100);

 private:

  void createGui();
  void formatLine(unsigned int i);
  void formatBuffer();
  void setScroll(int topline, int linecount, int nlines);
  void discardLines(int n);
  void discardExcess(bool _repaint=false);
  bool calcTP(TPoint &t, int x,int y);

  bool matchTextInLine(int i, const char *needle, bool select=true);
  void selectRegion(int startline,int startoff,int endline,int endoff);
  void freeOldSelections();

  deque<NLine *> lines;
  deque<FLine *> xlines;
  stack<char *>  prevsel;
  int            bgcolor;
  GtkWidget     *body, *sb;
  GtkAdjustment *vsa;
  GdkPixmap     *canvas;
  GdkGC         *cgc;
  int            cw, ch, lw, lh, fmtw;
  int            fh;
  int            leftb, rightb, tsskip;
  LayoutBox      L;

  int            IgnoreChg;
  unsigned int   MaxLines;
  bool           WasAtBottom;

  /* selection handling */
  bool           havesel;
  TPoint         A,B;      // selection end points
  int            dropmup;

  int            toid;
  int            lfl;

  /* the usual callback friends */
  friend gboolean ntext_expose(GtkWidget *widget, GdkEventExpose *ee,
			    gpointer data);
  friend gboolean ntext_configure(GtkWidget *widget, GdkEventConfigure *ee,
			    gpointer data);
  friend void     ntext_sbchange(GtkAdjustment *adj, gpointer data);

  friend gboolean ntext_mdown(GtkWidget *widget, GdkEventButton *eb,
			    gpointer data);
  friend gboolean ntext_mup(GtkWidget *widget, GdkEventButton *eb,
			    gpointer data);
  friend gboolean ntext_mdrag(GtkWidget *widget, GdkEventMotion *em,
			    gpointer data);
  friend gboolean ntext_scroll(GtkWidget *widget, GdkEventScroll *es, 
			    gpointer data);

  friend gboolean ntext_ksel(GtkWidget * widget,
			     GdkEventSelection * event, gpointer data);
  friend void     ntext_getsel(GtkWidget * widget,
			       GtkSelectionData * seldata,
			       guint info, guint time, gpointer data);

  friend gboolean ntext_redraw(gpointer data);
};

gboolean ntext_expose(GtkWidget *widget, GdkEventExpose *ee,
		      gpointer data);
gboolean ntext_configure(GtkWidget *widget, GdkEventConfigure *ee,
			 gpointer data);
void     ntext_sbchange(GtkAdjustment *adj, gpointer data);

gboolean ntext_mdown(GtkWidget *widget, GdkEventButton *eb,
		     gpointer data);
gboolean ntext_mup(GtkWidget *widget, GdkEventButton *eb,
		   gpointer data);
gboolean ntext_mdrag(GtkWidget *widget, GdkEventMotion *em,
		     gpointer data);
gboolean ntext_scroll(GtkWidget *widget, GdkEventScroll *es, 
			    gpointer data);

gboolean ntext_ksel(GtkWidget * widget,
		    GdkEventSelection * event, gpointer data);
void     ntext_getsel(GtkWidget * widget,
		      GtkSelectionData * seldata,
		      guint info, guint time, gpointer data);

gboolean ntext_redraw(gpointer data);


#endif
