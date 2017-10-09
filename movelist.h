/* $Id: movelist.h,v 1.8 2007/01/01 18:29:03 bergo Exp $ */

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

#ifndef MOVELIST_H
#define MOVELIST_H 1

#include "stl.h"
#include "eboard.h"
#include "position.h"
#include "widgetproxy.h"
#include "status.h"

class MoveListWindow : public WidgetProxy {
 public:
	MoveListWindow(char *p1,char *p2,int gid,list<Position> &moves,
				   int over,GameResult result, char *reason);
	virtual ~MoveListWindow();
	void setListener(MoveListListener *mll);
	void updateList(list<Position> &moves,
					int over=0,GameResult result=UNDEF, char *reason=0);
	void close();

 private:
	GtkWidget *clist;
	MoveListListener *listener;
	Status *textbar;

	void populate_clist(list<Position> &moves, int over,
						GameResult result,char *reason);

	friend void movelist_destroy (GtkWidget *w, gpointer data);
};

void movelist_destroy (GtkWidget *w, gpointer data);


#endif
