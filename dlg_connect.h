/* $Id: dlg_connect.h,v 1.5 2007/01/01 18:29:03 bergo Exp $ */

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

#ifndef DLG_CONNECT_H
#define DLG_CONNECT_H 1

#include "eboard.h"
#include "clock.h"
#include "widgetproxy.h"

class ConnectDialog : public WidgetProxy {
 public:
	ConnectDialog();

	virtual void show();

 private:
	GtkWidget *server,*port,*protocol;
	char Host[256];
	int  Port;
	char Proto[64];

	friend void dlg_connect_ok(GtkWidget *w,gpointer data);
	friend void dlg_connect_cancel(GtkWidget *w,gpointer data);

	friend void dlgconn_rowsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
							   gpointer data);
};

void dlg_connect_ok(GtkWidget *w,gpointer data);
void dlg_connect_cancel(GtkWidget *w,gpointer data);

void dlgconn_rowsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
					gpointer data);


class EditEngineBookmarksDialog : public ModalDialog,
	public UpdateInterface
{
 public:
	EditEngineBookmarksDialog(BookmarkListener *updatee);
	virtual ~EditEngineBookmarksDialog();
 private:
	BookmarkListener *beholder;
	GtkWidget *bl;
	GtkWidget *fe[7], *rm, *rmall, *apply, *edittc;
	BoxedLabel *uf[2];
	int selindex;
	TimeControl localtc;

	void refresh();
	void updateRightPane();

	virtual void update();

	friend void eebmd_rowsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
							 gpointer data);
	friend void eebmd_rowunsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
							   gpointer data);

	friend void eebmd_rm1(GtkWidget *w, gpointer data);
	friend void eebmd_rmall(GtkWidget *w, gpointer data);
	friend void eebmd_apply(GtkWidget *w, gpointer data);
	friend void eebmd_edittc(GtkWidget *w, gpointer data);

};

void eebmd_rowsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
				  gpointer data);
void eebmd_rowunsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
					gpointer data);

void eebmd_rm1(GtkWidget *w, gpointer data);
void eebmd_rmall(GtkWidget *w, gpointer data);
void eebmd_apply(GtkWidget *w, gpointer data);
void eebmd_edittc(GtkWidget *w, gpointer data);

typedef EditEngineBookmarksDialog * EEBMDp;

#endif
