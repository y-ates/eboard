/* $Id: text.h,v 1.25 2008/02/22 14:34:30 bergo Exp $ */

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



#ifndef EBOARD_TEXT_H
#define EBOARD_TEXT_H 1

#include "eboard.h"
#include "widgetproxy.h"
#include "ntext.h"
#include "notebook.h"
#include "history.h"
#include "util.h"
#include "stl.h"

class OutputPane {
 public:
  virtual ~OutputPane();
  virtual void append(char *msg,int color,Importance imp=IM_NORMAL)=0;
  virtual void append(char *msg,char *msg2,int color,Importance imp=IM_NORMAL)=0;
  virtual void updateScrollBack()=0;
  virtual void updateFont()=0;
  virtual void setBackground(int color)=0;
};

class Searchable {
 public:
  virtual ~Searchable() {}
  virtual void execSearch()=0;
  string SearchString;
};

class TextFilter {
 public:
  TextFilter();
  ~TextFilter();
  
  void set(const char *t);
  const char *getString();

  bool accept(char *textline);

 private:
  string FilterString;
  bool AcceptedLast;
  void cleanUp();
  vector<ExtPatternMatcher *> thefilter;
};

class Text : public NText,
             public NotebookInsider,
             public OutputPane,
             public Searchable {
 public:
 Text();
 virtual ~Text();

  void append(char *msg,int color,Importance imp=IM_NORMAL);
  void append(char *msg,char *msg2,int color,Importance imp=IM_NORMAL);

  void pageUp();
  void pageDown();

  void setBackground(int color);
  void updateScrollBack();
  void updateFont();

  void saveBuffer();

  void findText();
  void findTextNext();

  void execSearch();

  TextFilter Filter;

 private:
  int linecount;
  int LastMatch;
};

class TextSet : public OutputPane {
 public:
  TextSet();
  ~TextSet();

  void addTarget(Text *target);
  void removeTarget(Text *target);

  void append(char *msg,int color,Importance imp=IM_NORMAL);
  void append(char *msg,char *msg2,int color,Importance imp=IM_NORMAL);

  void pageUp();
  void pageDown();

  void updateScrollBack();
  void updateFont();
  void setBackground(int color);

 private:
  list<Text *> targets;
};

class DetachedConsole : public WidgetProxy {
 public:
  DetachedConsole(TextSet *yourset, ConsoleListener *cl);
  virtual ~DetachedConsole();

  const char *getFilter();

  void show();
  void setFilter(char *s);
  void setPasswordMode(int pm);

 private:
  Text *inner;
  TextSet *myset;
  GtkWidget *inputbox;
  ConsoleListener *listener;
  History::iterator hcursor;
  int focus_sig_id;
  string basetitle;
  GtkWidget *flabel;

  static int ConsoleCount;

  void injectInput();
  void historyUp();
  void historyDown();

  void clone();
  void updateFilterLabel();

  friend gint detached_delete  (GtkWidget * widget, GdkEvent * event, gpointer data);
  friend void detached_destroy (GtkWidget * widget, gpointer data);
  friend int  dc_input_key_press (GtkWidget * wid, GdkEventKey * evt,
				  gpointer data);
  friend void dc_set_filter(GtkWidget *w,gpointer data);
  friend void dc_new_console(GtkWidget *w,gpointer data);
};

gint detached_delete  (GtkWidget * widget, GdkEvent * event, gpointer data);
void detached_destroy (GtkWidget * widget, gpointer data);
int  dc_input_key_press (GtkWidget * wid, GdkEventKey * evt,
				gpointer data);
void dc_set_filter(GtkWidget *w,gpointer data);
void dc_new_console(GtkWidget *w,gpointer data);

class TextFilterDialog : public ModalDialog {
 public:
  TextFilterDialog(Text *target, GtkWidget *label2update);
 private:
  GtkWidget *pattern;
  Text *obj;
  GtkWidget *ulabel;
  friend void tfd_ok(GtkWidget *w, gpointer data);
};

void tfd_ok(GtkWidget *w, gpointer data);

#endif
