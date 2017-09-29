/* $Id: p2p.h,v 1.3 2007/01/01 18:29:03 bergo Exp $ */

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

#ifndef EBOARD_P2P_H
#define EBOARD_P2P_H 1

#include "eboard.h"
#include "widgetproxy.h"
#include "notebook.h"
#include "clock.h"
#include "network.h"

class P2PDialog : public NonModalDialog
{
 public:
  P2PDialog();
  virtual ~P2PDialog();

 private:
  Notebook *nb;
  GtkWidget *chost, *cport, *wport, *wbw, *wbc, *oname;
  BoxedLabel *bl[4];

  IncomingConnection *wconn;
  int                 toid;

  void waitConnection();
  void cancelWait();
  int  checkForConnection();


  friend void p2p_connect(GtkWidget *w, gpointer data);
  friend void p2p_wait(GtkWidget *w, gpointer data);
  friend void p2p_cancelwait(GtkWidget *w, gpointer data);

  friend gboolean p2p_check_incoming(gpointer data);

};

void p2p_connect(GtkWidget *w, gpointer data);
void p2p_wait(GtkWidget *w, gpointer data);
void p2p_cancelwait(GtkWidget *w, gpointer data);
gboolean p2p_check_incoming(gpointer data);

#endif
