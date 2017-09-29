/* $Id: widgetproxy.h,v 1.28 2007/02/05 19:37:56 bergo Exp $ */

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

#ifndef WIDGET_PROXY_H
#define WIDGET_PROXY_H

#include <gtk/gtk.h>
#include "eboard.h"
#include "stl.h"

class WindowGeometry;

class WidgetProxy {
 public:
  WidgetProxy();

  virtual void show();
  virtual void hide();
  virtual void repaint();

  void setIcon(char **xpmdata,const char *text);
  void restorePosition(WindowGeometry *wg);

  GtkWidget * PixButton(char **xpmicon, char *caption);

  GtkWidget * scrollBox(GtkWidget *child);

  GtkWidget *widget;
};

class BoxedLabel : public WidgetProxy {
 public:
  BoxedLabel(char *text);
  void setText(char *msg);
 private:
  GtkWidget *label;
};

class ModalDialog : public WidgetProxy {
 public:
  ModalDialog(char *title);
  virtual ~ModalDialog();
  virtual void show();

  void setDismiss(GtkObject *obj,char *sig);
  void setCloseable(bool v);

  void release();
  virtual void releaseWithoutDelete();

  GtkWidget *focused_widget;

  friend void modal_release(GtkWidget *w,gpointer data);
  friend gint modal_closereq(GtkWidget * widget, 
			     GdkEvent * event, gpointer data);

 private:
  bool closeable;
};

void modal_release(GtkWidget *w,gpointer data);
gint modal_closereq(GtkWidget * widget, 
		    GdkEvent * event, gpointer data);

class NonModalDialog : public ModalDialog {
 public:
  NonModalDialog(char *title);
  ~NonModalDialog();
  virtual void show();

  virtual void releaseWithoutDelete();
};

class UnboundedProgressWindow : public WidgetProxy {
 public:
  UnboundedProgressWindow(const char *_templ);
  virtual ~UnboundedProgressWindow();
  void setProgress(int v);  
 private:
  const char *templ;
  GtkWidget *label;
};

class UpdateNotify {
 public:
  virtual void update()=0;
};

class ColorButton : public WidgetProxy {
 public:
  ColorButton(char *caption, int value);
  void hookNotify(UpdateNotify *listen);
  void setColor(int value);
  int  getColor();
 private:
  void updateButtonFace();
  friend void colorb_click(GtkWidget *b,gpointer data);
  friend void colorb_csok(GtkWidget *b,gpointer data);

  int ColorValue;
  GtkWidget *colordlg, *label;
  UpdateNotify * notified;
};

void colorb_click(GtkWidget *b,gpointer data);
void colorb_csok(GtkWidget *b,gpointer data);

class TextPreview : public WidgetProxy, public UpdateNotify {
 public:
  TextPreview(GdkWindow *wnd, ColorButton *_bg);
  virtual ~TextPreview();
  void attach(ColorButton *cb, const char *sample);
  void freeze();
  void thaw();
  void update();
 private:
  ColorButton *bg;
  GdkPixmap *pixmap;
  bool frozen, pending;
  vector<ColorButton *> colors;
  vector<string> examples;
  PangoLayout *pl;
  PangoFontDescription *pfd;

  friend gboolean preview_expose(GtkWidget *widget,GdkEventExpose *ee,
				 gpointer data);
};

gboolean preview_expose(GtkWidget *widget,GdkEventExpose *ee,
			gpointer data);

class FileDialog : public WidgetProxy {

 public:
  FileDialog(char *title);
  virtual ~FileDialog();

  bool run();

  char FileName[128];

 private:
  bool gotresult;
  bool destroyed;

  friend void filedlg_ok (GtkWidget * w, gpointer data);
  friend void filedlg_destroy (GtkWidget * w, gpointer data);

};

void filedlg_ok (GtkWidget * w, gpointer data);
void filedlg_destroy (GtkWidget * w, gpointer data);

class LayoutBox {
 public:
  LayoutBox();
  virtual ~LayoutBox();

  void prepare(GtkWidget *w, GdkPixmap *t, GdkGC *gc, int a, int b, int c,int d);
  void setFont(int i,const char *fontname);

  void setColor(int c);
  void drawRect(int x,int y,int w,int h,bool fill);
  void drawLine(int x,int y,int w,int h);
  void drawEllipse(int x,int y,int w,int h,bool fill);
  void drawString(int x,int y, int f, const char *s);
  void drawString(int x,int y, int f, string &s);
  void drawSubstring(int x,int y, int f, const char *s,int len);
  int  drawButton(int x,int y,int w,int h, char *s,int font,
		  int fg,int bg);

  int  stringWidth(int f,const char *s);
  int  substringWidth(int f,const char *s,int len);
  int  fontHeight(int f);

  bool drawBoxedString(int x,int y,int w,int h,int f,char *s,char *sep);
  bool prepared();

  void consumeTop(int v);
  void consumeBottom(int v);

  int X,Y,W,H;
  GdkPixmap * Target;
  GdkGC     * GC;

 private:
  PangoFontDescription *pfd[5];
  PangoLayout *pl;
};

class DropBox : public WidgetProxy {
 public:
  DropBox(char *option, ...);
  virtual ~DropBox();

  int  getSelection();
  void setSelection(int i);
  void setUpdateListener(UpdateInterface *ui);

 private:
  int selection;
  vector<GtkWidget *> options;
  UpdateInterface *listener;

  friend void dropbox_select(GtkWidget *w, gpointer data);
};

void dropbox_select(GtkWidget *w, gpointer data);

/* some gtk helpers */

namespace Gtk {

  // call gtk_widget_show for a bunch of widgets
  void show(GtkWidget *w, ...);

}

#endif
