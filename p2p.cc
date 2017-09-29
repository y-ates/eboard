/* $Id: p2p.cc,v 1.7 2007/01/20 15:58:43 bergo Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "p2p.h"
#include "protocol.h"

P2PDialog::P2PDialog() : NonModalDialog(_("eboard Direct Connection Manager")) {
  GtkWidget *v, *bhb, *close;
  GtkWidget *vconn, *vlisten, *vops;
  GtkWidget *ct, *cl[2];
  GtkWidget *wl[4];
  GtkWidget *ot;
  int i;
  char z[64];

  wconn = 0;
  toid  = -1;

  v = gtk_vbox_new(FALSE,4);
  gtk_container_add(GTK_CONTAINER(widget),v);

  /* notebook */

  nb = new Notebook();

  gtk_box_pack_start(GTK_BOX(v),nb->widget, TRUE,TRUE, 0);
  
  vconn   = gtk_vbox_new(FALSE,4);
  vlisten = gtk_vbox_new(FALSE,4);
  vops    = gtk_vbox_new(FALSE,4);

  gtk_container_set_border_width(GTK_CONTAINER(vconn), 4);
  gtk_container_set_border_width(GTK_CONTAINER(vlisten), 4);
  gtk_container_set_border_width(GTK_CONTAINER(vops), 4);

  /* start a connection pane */
  
  ct = gtk_table_new(2,2,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(ct), 4);
  gtk_table_set_col_spacings(GTK_TABLE(ct), 4);
  chost = gtk_entry_new();
  cport = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(cport), "15451");
  
  bl[0] = new BoxedLabel(_("Hostname or IP address:"));
  bl[1] = new BoxedLabel(_("TCP Port:"));

  gtk_table_attach_defaults(GTK_TABLE(ct), bl[0]->widget, 0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(ct), chost, 1, 2, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(ct), bl[1]->widget, 0, 1, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(ct), cport, 1, 2, 1, 2);

  gtk_box_pack_start(GTK_BOX(vconn), ct, FALSE, TRUE, 2);

  cl[0]=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(cl[0]), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(cl[0]), 5);

  cl[1] = gtk_button_new_with_label(_("Connect"));
  gtk_box_pack_start(GTK_BOX(vconn), cl[0], FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(cl[0]), cl[1], TRUE, TRUE, 0);
  
  Gtk::show(cl[0], cl[1], chost, cport, ct, NULL);
  
  gtk_signal_connect(GTK_OBJECT(cl[1]), "clicked",
		     GTK_SIGNAL_FUNC(p2p_connect), (gpointer) this);

  /* wait for a connection pane */

  bl[2]  = new BoxedLabel(_("TCP Port:"));
  wl[0]  = gtk_hbox_new(FALSE,2);
  wport  = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(wport), "15451");
  
  gtk_box_pack_start(GTK_BOX(vlisten), wl[0], TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(wl[0]), bl[2]->widget, FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(wl[0]), wport, FALSE, TRUE, 6);

  wl[1]=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(wl[1]), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(wl[1]), 5);

  wl[2] = gtk_button_new_with_label(_("Wait"));
  wl[3] = gtk_button_new_with_label(_("Cancel"));

  wbw = wl[2]; wbc = wl[3];

  gtk_widget_set_sensitive(wl[3], FALSE);

  gtk_box_pack_start(GTK_BOX(vlisten), wl[1], FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(wl[1]), wl[2], TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(wl[1]), wl[3], TRUE, TRUE, 0);
  
  Gtk::show(wl[0],wl[1],wl[2],wl[3],wport,NULL);

  gtk_signal_connect(GTK_OBJECT(wl[2]), "clicked",
		     GTK_SIGNAL_FUNC(p2p_wait), (gpointer) this);
  gtk_signal_connect(GTK_OBJECT(wl[3]), "clicked",
		     GTK_SIGNAL_FUNC(p2p_cancelwait), (gpointer) this);

  /* options pane */

  ot = gtk_table_new(1,2,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(ot), 4);
  gtk_table_set_col_spacings(GTK_TABLE(ot), 4);
  oname = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(oname), global.P2PName);

  bl[3] = new BoxedLabel(_("Your name:"));

  gtk_table_attach_defaults(GTK_TABLE(ot), bl[3]->widget, 0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(ot), oname, 1, 2, 0, 1);

  gtk_box_pack_start(GTK_BOX(vops), ot, FALSE, TRUE, 2);

  Gtk::show(oname,ot,NULL);

  /* bind them */

  for(i=0;i<4;i++) bl[i]->show();  
  nb->addPage(vconn,  _("Start a connection"),0);
  nb->addPage(vlisten,_("Wait for a connection"),1);
  nb->addPage(vops,   _("Options"),2);

  /* bottom */

  bhb=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
  
  gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,FALSE,4);
  
  close = gtk_button_new_with_label(_("Close"));

  GTK_WIDGET_SET_FLAGS(close,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bhb),close,TRUE,TRUE,0);
  
  gtk_widget_grab_default(close);

  nb->show();
  nb->setTabPosition(GTK_POS_TOP);

  focused_widget = chost;
  Gtk::show(close,bhb,vconn,vlisten,vops,v,NULL);
  setDismiss(GTK_OBJECT(close),"clicked");
}

P2PDialog::~P2PDialog() {
  int i;
  if (nb) delete nb;
  for(i=0;i<5;i++)
    delete(bl[i]);
  if (wconn) {
    wconn->close();
    delete(wconn);
  }
  if (toid>0)
    gtk_timeout_remove(toid);
}

void p2p_connect(GtkWidget *w, gpointer data) {
  P2PDialog *me = (P2PDialog *) data;

  int port;
  char host[128];
  P2PProtocol *proto;

  g_strlcpy(host, gtk_entry_get_text(GTK_ENTRY(me->chost)),128);
  port = atoi(gtk_entry_get_text(GTK_ENTRY(me->cport)));

  if (global.network)
    if (global.network->isConnected()) {
      global.status->setText(_("Finish the current connection first."),10);
      return;
    }

  memset(global.P2PName, 0, 64);
  g_strlcpy(global.P2PName, gtk_entry_get_text(GTK_ENTRY(me->oname)), 64);
  global.writeRC();

  global.chandler->openServer(host, port, proto = new P2PProtocol(), 0);
  if (global.network)
    if (global.network->isConnected()) {
      proto->hail();
      me->release();
    }
}

void p2p_wait(GtkWidget *w, gpointer data) {
  P2PDialog *me = (P2PDialog *) data;

  memset(global.P2PName, 0, 64);
  g_strlcpy(global.P2PName, gtk_entry_get_text(GTK_ENTRY(me->oname)), 64);
  global.writeRC();

  me->waitConnection();
}

void p2p_cancelwait(GtkWidget *w, gpointer data) {
  P2PDialog *me = (P2PDialog *) data;
  me->cancelWait();
}

void P2PDialog::waitConnection() {
  int port;
  char z[128];
  port = atoi(gtk_entry_get_text(GTK_ENTRY(wport)));

  wconn = new IncomingConnection(port);
  if (wconn->open()!=0) {
    global.status->setText(wconn->getError(),15);
    delete wconn;
    wconn = 0;
  } else {
    gtk_widget_set_sensitive(wbw, FALSE);
    gtk_widget_set_sensitive(wbc, TRUE);
    toid = gtk_timeout_add(500, p2p_check_incoming, (gpointer) this);
    snprintf(z,128,_("Waiting for connection on port %d."),port);
    global.status->setText(z,500);
  }
}

void P2PDialog::cancelWait() {
  char z[128];
  if (wconn) {
    wconn->close();
    delete wconn;
    wconn = 0;
  }
  if (toid > 0) {
    gtk_timeout_remove(toid);
    toid = -1;
  }
  gtk_widget_set_sensitive(wbw, TRUE);
  gtk_widget_set_sensitive(wbc, FALSE);
  g_strlcpy(z,_("Cancelled connection wait."),128);
  global.status->setText(z,5);
}

int  P2PDialog::checkForConnection() {
  if (wconn->open()!=0)
    return 0;
  
  gtk_widget_set_sensitive(wbw, TRUE);
  gtk_widget_set_sensitive(wbc, FALSE);

  global.chandler->openServer(wconn, new P2PProtocol());
  wconn = 0;

  release();
  return 1;
}

gboolean p2p_check_incoming(gpointer data) {
  P2PDialog *me = (P2PDialog *) data;
  if (me->checkForConnection()) {
    me->toid = -1;
    return FALSE;
  }
  return TRUE;
}
