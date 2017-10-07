/* $Id: dlg_gamelist.h,v 1.13 2007/01/01 18:29:03 bergo Exp $ */

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


#ifndef DLG_GAMELIST_H
#define DLG_GAMELIST_H

#include "eboard.h"
#include "widgetproxy.h"

class GameListDialog : public WidgetProxy,
	public GameListConsumer {
 public:
	GameListDialog(GameListListener *someone);

	void appendGame(int gamenum, char *desc);
	void endOfList();

	void refresh();

 private:
	GameListListener *owner;
	GtkWidget *clist,*b[2];
	int SelectedRow;
	int canclose;

	friend void gamelist_refresh (GtkWidget * w, gpointer data);
	friend void gamelist_observe (GtkWidget * w, gpointer data);
	friend gint gamelist_delete  (GtkWidget * widget, GdkEvent * event, gpointer data);
	friend void gamelist_destroy (GtkWidget * widget, gpointer data);
	friend void gamelist_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
								  gpointer data);
	friend void gamelist_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
								  gpointer data);
	};

void gamelist_refresh (GtkWidget * w, gpointer data);
void gamelist_observe (GtkWidget * w, gpointer data);
gint gamelist_delete  (GtkWidget * widget, GdkEvent * event, gpointer data);
void gamelist_destroy (GtkWidget * widget, gpointer data);
void gamelist_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
					   gpointer data);
void gamelist_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
					   gpointer data);

class StockListDialog : public WidgetProxy {
 public:
	StockListDialog(StockListListener *someone);
	void refresh();

 private:
	void calcEnable();
	void open();
	void trash();
	void trashAll();

	StockListListener *owner;
	GtkWidget *clist;
	GtkWidget *b[7],*fdlg;
	int SelectedRow;
	int canclose;

	GtkCTreeNode * toplevel[4];
	GdkPixmap *icons[6];
	GdkBitmap *masks[6];

	friend void stocklist_refresh (GtkWidget * w, gpointer data);
	friend void stocklist_open (GtkWidget * w, gpointer data);
	friend void stocklist_loadpgn (GtkWidget * w, gpointer data);
	friend void stocklist_savepgn (GtkWidget * w, gpointer data);
	friend void stocklist_editpgn (GtkWidget * w, gpointer data);
	friend void stocklist_dump (GtkWidget * w, gpointer data);
	friend void stocklist_dumpall (GtkWidget * w, gpointer data);
	friend void stocklist_destroy (GtkWidget * widget, gpointer data);
	friend void stocklist_select  (GtkCTree *cl, GtkCTreeNode *node, gint column,
								   gpointer data);
	friend void stocklist_unselect(GtkCTree *cl, GtkCTreeNode *node, gint column,
								   gpointer data);
};

void stocklist_refresh (GtkWidget * w, gpointer data);
void stocklist_open (GtkWidget * w, gpointer data);
void stocklist_loadpgn (GtkWidget * w, gpointer data);
void stocklist_savepgn (GtkWidget * w, gpointer data);
void stocklist_editpgn (GtkWidget * w, gpointer data);
void stocklist_dump (GtkWidget * w, gpointer data);
void stocklist_dumpall (GtkWidget * w, gpointer data);
void stocklist_destroy (GtkWidget * widget, gpointer data);
void stocklist_select  (GtkCTree *cl, GtkCTreeNode *node, gint column,
						gpointer data);
void stocklist_unselect(GtkCTree *cl, GtkCTreeNode *node, gint column,
						gpointer data);


class AdListDialog : public WidgetProxy,
	public GameListConsumer {
 public:
	AdListDialog(AdListListener *someone);

	void appendGame(int gamenum, char *desc);
	void endOfList();

	void refresh();

 private:
	AdListListener *owner;
	GtkWidget *clist,*b[2];
	int SelectedRow;
	int canclose;

	friend void adlist_refresh (GtkWidget * w, gpointer data);
	friend void adlist_answer  (GtkWidget * w, gpointer data);
	friend void adlist_destroy (GtkWidget * widget, gpointer data);
	friend void adlist_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
								gpointer data);
	friend void adlist_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
								gpointer data);
	};

void adlist_refresh (GtkWidget * w, gpointer data);
void adlist_answer  (GtkWidget * w, gpointer data);
void adlist_destroy (GtkWidget * widget, gpointer data);
void adlist_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
					 gpointer data);
void adlist_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
					 gpointer data);

#endif
