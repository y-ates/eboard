/* $Id: movelist.cc,v 1.18 2007/01/20 15:58:43 bergo Exp $ */

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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "movelist.h"
#include "tstring.h"
#include "stl.h"
#include "eboard.h"

#include "icon-moves.xpm"

MoveListWindow::MoveListWindow(char *p1,char *p2,
			       int gid,list<Position> &moves,
			       int over,GameResult result,char *reason) {
  
  char z[128];
  GtkWidget *v,*sw;

  listener=0;

  if (gid<7000)  
    snprintf(z,128,_("Game #%d - %s vs. %s"),gid,p1,p2);
  else
    snprintf(z,128,"%s vs. %s",p1,p2);
  
  widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(widget),300,400);
  gtk_window_set_title(GTK_WINDOW(widget),z);
  gtk_widget_realize(widget);

  v=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),v);

  sw=gtk_scrolled_window_new(NULL,NULL);
  
  clist=gtk_clist_new(3);
  gtk_clist_set_shadow_type(GTK_CLIST(clist),GTK_SHADOW_IN);
  gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
  gtk_clist_set_column_title(GTK_CLIST(clist),0,"#");
  gtk_clist_set_column_title(GTK_CLIST(clist),1,_("White"));
  gtk_clist_set_column_title(GTK_CLIST(clist),2,_("Black"));
  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_column_titles_show(GTK_CLIST(clist));

  gtk_box_pack_start(GTK_BOX(v),sw,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw),clist);

  textbar=new Status();
  textbar->WaitUpdate=0;
  textbar->show();
  gtk_box_pack_start(GTK_BOX(v),textbar->widget,FALSE,TRUE,0);

  setIcon(icon_moves_xpm,_("Moves"));

  populate_clist(moves,over,result,reason);

  Gtk::show(clist,sw,v,NULL);

  gtk_signal_connect (GTK_OBJECT (widget), "destroy",
		      GTK_SIGNAL_FUNC (movelist_destroy), this);

}

MoveListWindow::~MoveListWindow() {
  delete textbar;
}

void MoveListWindow::updateList(list<Position> &moves,
				int over,GameResult result,
				char *reason) {
  populate_clist(moves,over,result,reason);
}

void MoveListWindow::populate_clist(list<Position> &moves, int over,
				    GameResult result,char *reason) {
  tstring t;
  string *p;
  list<Position>::iterator li;
  char z[64];
  char *zz[3];
  int i,cm,tm;

  gtk_clist_freeze(GTK_CLIST(clist));
  gtk_clist_clear(GTK_CLIST(clist));

  cm=0;
  t.setChomp(true);

  for(li=moves.begin(),i=-1;li!=moves.end();li++) {
    if ( ! (*li).getLastMove().empty() ) {
      t.set( (*li).getLastMove() );
      tm = t.tokenvalue(".");
    } else
      tm=0;
    if (!tm) continue;
    if (tm!=cm) {
      snprintf(z,64,"%d",tm);
      zz[0]=z;
      zz[1]=_("none");
      zz[2]=_("none");
      gtk_clist_append(GTK_CLIST(clist),zz);
      ++i;      
      cm=tm;
    }

    p=t.token(" \n");
    if (p)
      if (!p->compare("..."))
	gtk_clist_set_text(GTK_CLIST(clist),i,2,t.token("\n")->c_str());
      else {
	t.reset();
	t.token(".");
	p=t.token("\n");
	gtk_clist_set_text(GTK_CLIST(clist),i,1,p->c_str());
      }
  }

  if (over) {
    switch(result) {
    case WHITE_WIN: strcpy(z,"1-0 "); break;
    case BLACK_WIN: strcpy(z,"0-1 "); break;
    case DRAW:      strcpy(z,"1/2-1/2 "); break;
    case UNDEF:     strcpy(z,"(*) "); break;
    default: z[0]=0;
    }
    g_strlcat(z,reason,64);
    textbar->setText(z);
  } else {
    textbar->setText(_("Game in progress."));
  }

  gtk_clist_set_column_width(GTK_CLIST(clist),0,32);
  gtk_clist_set_column_width(GTK_CLIST(clist),1,96);
  gtk_clist_set_column_width(GTK_CLIST(clist),2,96);
  gtk_clist_thaw(GTK_CLIST(clist));
}

void MoveListWindow::setListener(MoveListListener *mll) {
  listener=mll;
}

void MoveListWindow::close() {
  gtk_widget_destroy(widget);
}

void movelist_destroy (GtkWidget * w, gpointer data) {
  MoveListWindow *me;
  me=(MoveListWindow *)data;
  if (me->listener)
    me->listener->moveListClosed();
}
