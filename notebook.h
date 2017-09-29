/* $Id: notebook.h,v 1.25 2007/02/01 21:30:27 pierrebou Exp $ */

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

#ifndef NOTEBOOK_H
#define NOTEBOOK_H 1

#include "stl.h"
#include "eboard.h"
#include "widgetproxy.h"

class Page {
 public:
  Page();
  Page(int _number,GtkWidget *_content, char *_title, bool _removable=false);
  void setTitle(char *_title);
  char *getTitle();
  int operator==(int n);
  int under(Importance imp);
  int above(Importance imp);
  void setLevel(Importance imp);

  int number;
  bool naughty;
  bool removable;
  GtkWidget *content,*label,*labelframe;

  void dump();
  void renumber(int newid);

 private:
  Importance level;
  char title[256];
};

class Notebook : public WidgetProxy {
 public:
  Notebook();
  virtual ~Notebook() {}
  void addPage(GtkWidget *what,char *title,int id,bool removable=false);
  void removePage(int id);
  unsigned int getCurrentPage();
  int getCurrentPageId();
  void setTabColor(int pageid,int color,Importance imp=IM_TOP);
  void setTabPosition(GtkPositionType pos);
  void goToPageId(int id);

  void goToLastVisited();
  void goToNext();
  void goToPrevious();

  void setNaughty(int pageid, bool val);
  void getNaughty(vector<int> &evil_ids);
  bool isNaughty(int pageid);
  bool hasNaughty();

  void dump();  

  void renumberPage(int oldid,int newid);
  void setListener(PaneChangeListener *listener);
  void pretendPaneChanged();

  static void ensurePixmaps();
  static GdkPixmap *trashface;
  static GdkBitmap *trashmask;
  static GtkWidget *swidget;

 private:
  vector<Page *> pages;
  PaneChangeListener *pcl;

  void setTabColor(int page_num,int color,int poly,Importance imp=IM_TOP);
  void addBombToTab(Page *pg);

  friend void notebook_switch_page(GtkNotebook *notebook,
				   GtkNotebookPage *page,
				   gint page_num,
				   gpointer data);
  
  friend gboolean gtkDgtnixEvent(GIOChannel* channel, GIOCondition cond, gpointer data);


};

void notebook_switch_page(GtkNotebook *notebook,
			  GtkNotebookPage *page,
			  gint page_num,
			  gpointer data);

class NotebookInsider {
 public:  
  NotebookInsider();
  void setNotebook(Notebook *nb,int pageid);
  void contentUpdated();
  void pop();
  int  getPageId();
 protected:
  void pushLevel(Importance imp);
 private:
  Notebook *mynotebook;
  int pgid;
  stack<Importance> impstack;
};

#endif
