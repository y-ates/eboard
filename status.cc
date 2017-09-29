/* $Id: status.cc,v 1.14 2006/10/26 09:24:20 bergo Exp $ */

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


#include <iostream>
#include "global.h"
#include "status.h"
#include "eboard.h"

Status::Status() {
  GtkWidget *hb;

  WaitUpdate=1;
  toid = -1;

  widget=gtk_frame_new(0);
  gtk_frame_set_shadow_type(GTK_FRAME(widget),GTK_SHADOW_ETCHED_OUT);
  hb=gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),hb);

  label=gtk_label_new(_("Welcome to eboard."));
  gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hb),label,FALSE,FALSE,2);
  Gtk::show(label,hb,NULL);
}

void Status::setText(char *msg) {
  killExp();
  gtk_label_set_text(GTK_LABEL(label),msg);
  gtk_widget_queue_resize(label);
  waitUpdate();
}

gboolean st_expire(gpointer data) {
  Status *me = (Status *) data;
  me->toid = -1;
  gtk_label_set_text(GTK_LABEL(me->label)," ");
  gtk_widget_queue_resize(me->label);
  return FALSE;
}

void Status::setText(char *msg, int secs) {
  killExp();
  setText(msg);
  toid = gtk_timeout_add(secs * 1000, st_expire, (void *) this);
}

void Status::waitUpdate() {
  int i;
  if (WaitUpdate)
    for(i=3;i;i--)
      if(gtk_events_pending())
	global.WrappedMainIteration();
      else
	break;
}

void Status::killExp() {
  if (toid>0) {
    gtk_timeout_remove(toid);
    toid = -1;
  }
}
