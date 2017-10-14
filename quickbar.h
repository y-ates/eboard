/* $Id: quickbar.h,v 1.5 2004/12/27 15:16:17 bergo Exp $ */

/*

  eboard - chess client
  http://eboard.sourceforge.net
  Copyright (C) 2000-2002 Felipe Paulo Guazzi Bergo
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

#ifndef EBOARD_QUICKBAR_H
#define EBOARD_QUICKBAR_H 1

#include "widgetproxy.h"
#include "stl.h"

class QButton {
 public:
	QButton();
	QButton(const char *a, const char *b, int c);
	string caption;
	string command;
	int    icon;
};

ostream &operator<<(ostream &s, QButton q);

class QuickBarIcons {
 public:
	static GdkPixmap *arrow[6], *qbl, *qbr;
	static GdkBitmap *mask[6], *mqbl, *mqbr;
	static int numbers[10];

 protected:
	static void loadIcons(GtkWidget *parent);
	GtkWidget * CommandButton(GdkPixmap *pix, GdkBitmap *mask, char *caption);

 private:
	static bool IconsLoaded;
};

class QuickBar : public WidgetProxy,
	public QuickBarIcons
{
 public:
	QuickBar(GtkWidget *parent);
	virtual ~QuickBar() {}

	void update();

 private:
	vector<GtkWidget *> buttons;

	void build();
	void clear();
};

class QuickBarSetupDialog : public ModalDialog,
	public QuickBarIcons
{
 public:
	QuickBarSetupDialog();

 private:
	GtkWidget *bcap[10], *bcom[10], *bpix[10], *bl[10], *br[10];
	int iconValue[10];

	int whichButton(GtkWidget *w);

	friend void qbsetup_ok(GtkWidget *w, gpointer data);
	friend void qbsetup_left(GtkWidget *w, gpointer data);
	friend void qbsetup_right(GtkWidget *w, gpointer data);
};


#endif
