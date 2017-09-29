/* $Id: bugpane.h,v 1.11 2008/02/08 14:25:50 bergo Exp $ */

/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2006 Felipe Paulo Guazzi Bergo
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

#ifndef BUGPANE_H
#define BUGPANE_H 1

#include "eboard.h"
#include "widgetproxy.h"
#include "board.h"
#include "pieces.h"
#include "stl.h"

class BareBoard : public WidgetProxy,
	          public RootBoard,
	          public ClockHost,
	          public PieceChangeListener
{
 public:
  BareBoard();
  virtual ~BareBoard();

  void setPosition(Position &pos);
  void setWhite(char *name);
  void setBlack(char *name);
  void setClock2(int wmsecs,int bmsecs,int actv,int cdown);
  virtual void updateClock();
  void addPTell(char *text);
  void update();  

  void freeze();
  void thaw();

  ChessClock clock;

  virtual void pieceSetChanged();

 private:

  string Names[2];
  Position position;
  GdkPixmap *pixbuf;
  int pixw, pixh;
  PieceSet *pset;
  bool upending;
  int  frozen;
  LayoutBox C;

  list<string> PTells;

  void reloadPieceSet();

  friend gboolean bareboard_expose(GtkWidget *widget,GdkEventExpose *ee,
				   gpointer data);
};

class BugPane : public WidgetProxy {
 public:
  BugPane();
  void addBugText(char *text);
  void setPlayerNames(char *white, char *black);
  void setPosition(Position &pos);
  void setClock2(int wmsecs,int bmsecs,int actv,int cdown);

  void freeze();
  void thaw();

  void reset();
  void stopClock();

  static string BugTell;

 private:
  BareBoard *board;  

  friend void bug_ptell(GtkWidget *b,gpointer data);  
};

/* friend prototypes */
gboolean bareboard_expose(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
void     bug_ptell(GtkWidget *b,gpointer data);  

#endif
