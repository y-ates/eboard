/* $Id: script.h,v 1.5 2007/01/01 18:29:03 bergo Exp $ */

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

#ifndef EBOARD_SCRIPT_H
#define EBOARD_SCRIPT_H

#include "eboard.h"
#include "network.h"
#include "widgetproxy.h"

class ScriptList : public ModalDialog {
 public:
	ScriptList();
 private:
	void refresh();

	GtkWidget *clist,*b[3];
	int SelectedRow;

	friend void script_refresh (GtkWidget * w, gpointer data);
	friend void script_run (GtkWidget * w, gpointer data);
	friend void script_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
								gpointer data);
	friend void script_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
								gpointer data);
};

void script_refresh (GtkWidget * w, gpointer data);
void script_run (GtkWidget * w, gpointer data);
void script_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
					 gpointer data);
void script_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
					 gpointer data);


class ScriptInstance : public WidgetProxy {
 public:
	ScriptInstance(char *name);

 private:
	PipeConnection *child;
	int toid, anid, frame;
	GtkWidget *runner;

	friend void scripti_kill(GtkWidget *w,gpointer data);
	friend gboolean scripti_check(gpointer data);
	friend gboolean scripti_anim(gpointer data);

	static void initPixmaps();

	static bool       pixmapsok;
	static GdkPixmap *spir[4];
	static GdkBitmap *smask[4];
};

void scripti_kill(GtkWidget *w,gpointer data);
gboolean scripti_check(gpointer data);
gboolean scripti_anim(gpointer data);


#endif
