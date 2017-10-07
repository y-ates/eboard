/* $Id: dlg_connect.cc,v 1.24 2007/06/09 11:35:06 bergo Exp $ */

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
#include <stdlib.h>
#include <string.h>
#include "stl.h"
#include "dlg_connect.h"
#include "protocol.h"
#include "global.h"
#include "chess.h"
#include "eboard.h"
#include "mainwindow.h"

#ifdef MAEMO
#include <hildon-widgets/hildon-program.h>
#endif

extern MainWindow *mainw;

ConnectDialog::ConnectDialog() {
	GtkWidget *v,*hs,*hb,*ok,*cancel,*t;
	GtkWidget *zh,*zv,*ysw,*ysl;
	BoxedLabel *bl[3];
	GList *combo;
	int i;

#if !MAEMO
	widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
#else
	widget=(GtkWidget *) HILDON_WINDOW(hildon_window_new());
#endif

	gtk_window_set_transient_for(GTK_WINDOW(widget),GTK_WINDOW(mainw->widget));
	gtk_window_set_modal(GTK_WINDOW(widget),TRUE);
	if (!global.HostHistory.empty())
		gtk_window_set_default_size(GTK_WINDOW(widget),400,300);
	gtk_window_set_title(GTK_WINDOW(widget),_("Connect to ICS Server"));
	gtk_window_set_position(GTK_WINDOW(widget),GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(widget),4);
	gtk_widget_realize(widget);

	v=gtk_vbox_new(FALSE,4);
	gtk_container_add(GTK_CONTAINER(widget),v);

	zh=gtk_hbox_new(FALSE,4);
	zv=gtk_vbox_new(FALSE,4);

	gtk_box_pack_start(GTK_BOX(v),zh,TRUE,TRUE,0);

	if (!global.HostHistory.empty()) {
		ysw=gtk_scrolled_window_new(0,0);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ysw),
									   GTK_POLICY_AUTOMATIC,
									   GTK_POLICY_AUTOMATIC);

		ysl=gtk_clist_new(1);
		gtk_clist_set_column_title(GTK_CLIST(ysl),0,_("Recent Hosts"));
		gtk_clist_set_selection_mode(GTK_CLIST(ysl),GTK_SELECTION_SINGLE);
		gtk_clist_column_titles_show(GTK_CLIST(ysl));

		char z[256];
		char *zp[1];
		for(list<HostBookmark *>::iterator bi=global.HostHistory.begin();
			bi!=global.HostHistory.end();bi++) {
			snprintf(z,256,"%s, %d, %s",(*bi)->host,(*bi)->port,(*bi)->protocol);
			zp[0]=z;
			gtk_clist_append(GTK_CLIST(ysl),zp);
		}

		gtk_container_add(GTK_CONTAINER(ysw),ysl);
		gtk_box_pack_start(GTK_BOX(zh),ysw,TRUE,TRUE,4);

		gtk_signal_connect(GTK_OBJECT(ysl),"select_row",
						   GTK_SIGNAL_FUNC(dlgconn_rowsel),this);

		Gtk::show(ysl,ysw,NULL);
	}

	gtk_box_pack_start(GTK_BOX(zh),zv,FALSE,TRUE,0);

	bl[0]=new BoxedLabel(_("Hostname"));
	bl[1]=new BoxedLabel(_("TCP Port"));
	bl[2]=new BoxedLabel(_("Protocol"));

	server=gtk_entry_new();
	port=gtk_entry_new();
	protocol=gtk_combo_new();

	combo=0;
	combo=g_list_append(combo,(gpointer)"FICS");
	combo=g_list_append(combo,(gpointer)"FICS without timeseal");
	gtk_combo_set_popdown_strings(GTK_COMBO(protocol),combo);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(protocol)->entry),"FICS");
	gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(protocol)->entry),FALSE);

	gtk_box_pack_start(GTK_BOX(zv),bl[0]->widget,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(zv),server,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(zv),bl[1]->widget,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(zv),port,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(zv),bl[2]->widget,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(zv),protocol,FALSE,FALSE,0);

	gtk_entry_set_text(GTK_ENTRY(port),"5000");

	// MAEMO TODO
	t=gtk_label_new(_("Once you connect to a host from this dialog, it will be added to\n"\
					  "the Peer/ICS Bookmarks menu. Edit the ~/.eboard/eboard.conf file\n"\
					  "to modify or remove entries."));
	gtk_box_pack_start(GTK_BOX(v),t,FALSE,FALSE,6);

	hs=gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,6);

	hb=gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);

	gtk_box_pack_start(GTK_BOX(v),hb,FALSE,FALSE,2);

	ok=gtk_button_new_with_label(_("Connect"));
	GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
	cancel=gtk_button_new_with_label(_("Cancel"));
	GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);

	gtk_box_pack_start(GTK_BOX(hb),ok,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(hb),cancel,TRUE,TRUE,0);

	gtk_widget_grab_default(ok);

	for(i=0;i<3;i++)
		gshow(bl[i]->widget);

	Gtk::show(server,port,protocol,t,hs,hb,cancel,ok,
			  zv,zh,v,NULL);

	gtk_signal_connect(GTK_OBJECT(ok),"clicked",
					   GTK_SIGNAL_FUNC(dlg_connect_ok),this);
	gtk_signal_connect(GTK_OBJECT(cancel),"clicked",
					   GTK_SIGNAL_FUNC(dlg_connect_cancel),this);
}

void ConnectDialog::show() {
	gshow(widget);
	gtk_grab_add(widget);
	gtk_widget_grab_focus(server);
}

void dlg_connect_ok(GtkWidget *w,gpointer data) {
	int useseal=1;
	ConnectDialog *cd;
	Protocol *pp;
	HostBookmark *hbm;
	tstring t;

	cd=(ConnectDialog *)data;
	g_strlcpy(cd->Host,gtk_entry_get_text(GTK_ENTRY(cd->server)),256);
	cd->Port=atoi(gtk_entry_get_text(GTK_ENTRY(cd->port)));
	g_strlcpy(cd->Proto,
			  gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(cd->protocol)->entry)),64);

	t.set(cd->Host);
	if (! t.token(" \n\t\r"))
		return;

	hbm=new HostBookmark();
	strcpy(hbm->host,cd->Host);
	hbm->port=cd->Port;
	strcpy(hbm->protocol,cd->Proto);
	global.addHostBookmark(hbm);

	pp=0;
	if (!strcmp(cd->Proto,"FICS without timeseal")) {
		pp=new FicsProtocol();
		useseal=0;
		goto kissagirl;
	}

	if (!strcmp(cd->Proto,"FICS")) {
		pp=new FicsProtocol();
		useseal=1;
		goto kissagirl;
	}
	if (!pp)
		pp=new NullProtocol();

 kissagirl:

	gtk_grab_remove(cd->widget);
	gtk_widget_destroy(cd->widget);

	if (useseal)
		global.chandler->openServer(cd->Host,cd->Port,pp,"timeseal");
	else
		global.chandler->openServer(cd->Host,cd->Port,pp,0);
}

void dlg_connect_cancel(GtkWidget *w,gpointer data) {
	ConnectDialog *cd;
	cd=(ConnectDialog *)data;
	gtk_grab_remove(cd->widget);
	gtk_widget_destroy(cd->widget);
}

void dlgconn_rowsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
					gpointer data) {
	ConnectDialog *me;
	list<HostBookmark *>::iterator bi;
	char z[6];
	me=(ConnectDialog *)data;

	for(bi=global.HostHistory.begin();row;row--)
		bi++;

	gtk_entry_set_text(GTK_ENTRY(me->server),(*bi)->host);
	snprintf(z,6,"%d",(*bi)->port);
	gtk_entry_set_text(GTK_ENTRY(me->port),z);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(me->protocol)->entry),(*bi)->protocol);
}

// ==============================

EditEngineBookmarksDialog::~EditEngineBookmarksDialog() {
	delete uf[0];
	delete uf[1];

	if (beholder != 0)
		beholder->updateBookmarks();
}

EditEngineBookmarksDialog::EditEngineBookmarksDialog(BookmarkListener *updatee) :
	ModalDialog(N_("Edit Engine Bookmarks"))
{
	GtkWidget *v, *v2, *h, *tch, *bs, *hs, *hb, *klose, *h2, *etc;
	BoxedLabel *fl[5];
	int i;

	beholder=updatee;
	gtk_window_set_default_size(GTK_WINDOW(widget), 550, 300);

	v=gtk_vbox_new(FALSE,4);
	gtk_container_add(GTK_CONTAINER(widget),v);

	h=gtk_hbox_new(TRUE,4);
	gtk_box_pack_start(GTK_BOX(v),h,TRUE,TRUE,0);


	bs=gtk_scrolled_window_new(0,0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(bs),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);

	bl=gtk_clist_new(1);
	gtk_clist_set_column_title(GTK_CLIST(bl),0,_("Engine Bookmarks"));
	gtk_clist_set_selection_mode(GTK_CLIST(bl),GTK_SELECTION_SINGLE);
	gtk_clist_column_titles_show(GTK_CLIST(bl));

	gtk_container_add(GTK_CONTAINER(bs),bl);
	gtk_box_pack_start(GTK_BOX(h),bs,TRUE,TRUE,4);

	// ---

	v2=gtk_vbox_new(FALSE,2);
	gtk_box_pack_start(GTK_BOX(h), v2, FALSE, TRUE, 4);

	fl[0]=new BoxedLabel(_("Bookmark Caption:"));
	fl[1]=new BoxedLabel(_("Directory:"));
	fl[2]=new BoxedLabel(_("Command Line:"));
	fl[3]=new BoxedLabel(_("Time Control:"));
	fl[4]=new BoxedLabel(_("Max Ply:"));

	fe[0]=gtk_entry_new();
	fe[1]=gtk_entry_new();
	fe[2]=gtk_entry_new();
	fe[3]=gtk_label_new(" ");
	fe[4]=gtk_entry_new();

	tch = gtk_hbox_new(FALSE,4);

	edittc = etc = gtk_button_new_with_label(_("Change..."));

	fe[5]=gtk_check_button_new_with_label(_("Human plays white"));
	fe[6]=gtk_check_button_new_with_label(_("Think Always"));

	uf[0]=new BoxedLabel(_("Variant:"));
	uf[1]=new BoxedLabel(_("Engine Type:"));

	for(i=0;i<5;i++) {
		gtk_box_pack_start(GTK_BOX(v2), fl[i]->widget, FALSE,TRUE, 2);
		if (i!=3)
			gtk_box_pack_start(GTK_BOX(v2), fe[i], FALSE,TRUE, 0);
		else {
			gtk_box_pack_start(GTK_BOX(v2), tch, FALSE,TRUE, 0);
			gtk_box_pack_start(GTK_BOX(tch), fe[3], FALSE, TRUE, 0);
			gtk_box_pack_end(GTK_BOX(tch), etc, FALSE, TRUE, 0);
		}
	}

	for(i=5;i<7;i++)
		gtk_box_pack_start(GTK_BOX(v2), fe[i], FALSE,TRUE, 2);

	for(i=0;i<2;i++)
		gtk_box_pack_start(GTK_BOX(v2), uf[i]->widget, FALSE,TRUE, 2);

	h2=gtk_hbox_new(FALSE,4);

	apply=gtk_button_new_with_label(_("Apply Changes"));
	rm=gtk_button_new_with_label(_("Delete This Entry"));
	rmall=gtk_button_new_with_label(_("Delete All Entries"));

	gtk_box_pack_start(GTK_BOX(v2),h2,FALSE,TRUE,2);
	gtk_box_pack_end(GTK_BOX(h2),apply,FALSE,TRUE,4);
	gtk_box_pack_end(GTK_BOX(h2),rmall,FALSE,TRUE,4);
	gtk_box_pack_end(GTK_BOX(h2),rm,FALSE,TRUE,4);

	// ---

	hs=gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,2);

	hb=gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);

	gtk_box_pack_start(GTK_BOX(v),hb,FALSE,FALSE,2);

	klose=gtk_button_new_with_label(_("Close"));
	GTK_WIDGET_SET_FLAGS(klose,GTK_CAN_DEFAULT);

	gtk_box_pack_start(GTK_BOX(hb),klose,TRUE,TRUE,0);

	gtk_widget_grab_default(klose);

	Gtk::show(etc,tch,klose,hb,hs,h2,apply,rm,rmall,v2,NULL);

	for(i=0;i<7;i++)
		gshow(fe[i]);
	for(i=0;i<5;i++)
		gshow(fl[i]->widget);
	for(i=0;i<2;i++)
		uf[i]->show();

	Gtk::show(bl,bs,h,v,NULL);
	setDismiss(GTK_OBJECT(klose),"clicked");

	refresh();

	selindex=-1;

	gtk_signal_connect(GTK_OBJECT(bl),"select_row",
					   GTK_SIGNAL_FUNC(eebmd_rowsel), (gpointer) this);

	gtk_signal_connect(GTK_OBJECT(bl),"unselect_row",
					   GTK_SIGNAL_FUNC(eebmd_rowunsel), (gpointer) this);

	gtk_widget_set_sensitive(rm,FALSE);
	gtk_widget_set_sensitive(apply,FALSE);
	gtk_widget_set_sensitive(edittc,FALSE);

	gtk_signal_connect(GTK_OBJECT(rm),"clicked",
					   GTK_SIGNAL_FUNC(eebmd_rm1), (gpointer) this);

	gtk_signal_connect(GTK_OBJECT(rmall),"clicked",
					   GTK_SIGNAL_FUNC(eebmd_rmall), (gpointer) this);

	gtk_signal_connect(GTK_OBJECT(apply),"clicked",
					   GTK_SIGNAL_FUNC(eebmd_apply), (gpointer) this);

	gtk_signal_connect(GTK_OBJECT(etc),"clicked",
					   GTK_SIGNAL_FUNC(eebmd_edittc), (gpointer) this);
}

void EditEngineBookmarksDialog::refresh() {
	list<EngineBookmark *>::iterator ei;
	char *zp[1];
	char z[256];

	gtk_clist_freeze(GTK_CLIST(bl));
	gtk_clist_clear(GTK_CLIST(bl));


	for(ei=global.EnginePresets.begin();ei!=global.EnginePresets.end();ei++) {
		g_strlcpy(z,(*ei)->caption.c_str(),256);
		zp[0]=z;
		gtk_clist_append(GTK_CLIST(bl),zp);
	}

	gtk_clist_thaw(GTK_CLIST(bl));

	selindex=-1;
	updateRightPane();

}

void EditEngineBookmarksDialog::update() {
	updateRightPane();
}

void EditEngineBookmarksDialog::updateRightPane() {
	list<EngineBookmark *>::iterator ei;
	int i;
	char z[128],x[64];

	if (selindex>=0) {
		ei=global.EnginePresets.begin();
		for(i=selindex;i;i--)
			ei++;

		gtk_entry_set_text(GTK_ENTRY(fe[0]),(*ei)->caption.c_str());
		gtk_entry_set_text(GTK_ENTRY(fe[1]),(*ei)->directory.c_str());
		gtk_entry_set_text(GTK_ENTRY(fe[2]),(*ei)->cmdline.c_str());

		localtc.toString(z,128);
		gtk_label_set_text(GTK_LABEL(fe[3]),z);
		gtk_widget_queue_resize(fe[3]);

		snprintf(z,128,"%d",(*ei)->maxply);
		gtk_entry_set_text(GTK_ENTRY(fe[4]),z);

		gtset(GTK_TOGGLE_BUTTON(fe[5]),(*ei)->humanwhite?TRUE:FALSE);
		gtset(GTK_TOGGLE_BUTTON(fe[6]),(*ei)->think?TRUE:FALSE);

		snprintf(z,128,_("Variant: %s (uneditable)"),
				 ChessGame::variantName( (*ei)->mode ) );
		uf[0]->setText(z);

		switch( (*ei)->proto ) {
		case 0: g_strlcpy(x,_("generic xboard v2"),64); break;
		case 1: strcpy(x,"crafty"); break;
		case 2: strcpy(x,"sjeng"); break;
		case 3: strcpy(x,"GNU chess 4.x"); break;
		}

		snprintf(z,128,_("Engine Type: %s (uneditable)"),x);
		uf[1]->setText(z);

		gtk_widget_set_sensitive(rm,TRUE);
		gtk_widget_set_sensitive(apply,TRUE);
		gtk_widget_set_sensitive(edittc,TRUE);

	} else {
		gtk_entry_set_text(GTK_ENTRY(fe[0]),"\0");
		gtk_entry_set_text(GTK_ENTRY(fe[1]),"\0");
		gtk_entry_set_text(GTK_ENTRY(fe[2]),"\0");
		gtk_label_set_text(GTK_LABEL(fe[3])," ");
		gtk_entry_set_text(GTK_ENTRY(fe[4]),"\0");
		gtset(GTK_TOGGLE_BUTTON(fe[5]),0);
		gtset(GTK_TOGGLE_BUTTON(fe[6]),0);

		uf[0]->setText(_("Variant:"));
		uf[1]->setText(_("Engine Type:"));

		gtk_widget_set_sensitive(rm,FALSE);
		gtk_widget_set_sensitive(apply,FALSE);
		gtk_widget_set_sensitive(edittc,FALSE);
	}
}

void
eebmd_rowsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
			 gpointer data)
{
	EEBMDp me = (EEBMDp) data;
	list<EngineBookmark *>::iterator ei;
	int i;

	ei=global.EnginePresets.begin();
	for(i=row;i;i--)
		ei++;
	me->localtc = (*ei)->timecontrol;

	me->selindex=row;
	me->updateRightPane();
}

void eebmd_rowunsel(GtkCList *clist,int row,int column,GdkEventButton *eb,
					gpointer data)
{
	EEBMDp me;
	me=(EEBMDp) data;

	me->selindex=-1;
	me->updateRightPane();
}

void
eebmd_rm1(GtkWidget *w, gpointer data)
{
	EEBMDp me;
	list<EngineBookmark *>::iterator ei;
	int i;

	me=(EEBMDp) data;

	if (me->selindex < 0) return;

	ei=global.EnginePresets.begin();
	for(i=me->selindex;i;i--)
		ei++;

	delete(*ei);
	global.EnginePresets.erase(ei);

	global.writeRC();

	me->refresh();
	me->updateRightPane();
}

void
eebmd_rmall(GtkWidget *w, gpointer data)
{
	list<EngineBookmark *>::iterator ei;
	EEBMDp me;

	me=(EEBMDp) data;

	for(ei=global.EnginePresets.begin();ei!=global.EnginePresets.end();ei++)
		delete(*ei);

	global.EnginePresets.clear();

	global.writeRC();

	me->refresh();
	me->updateRightPane();
}

void
eebmd_edittc(GtkWidget *w, gpointer data) {
	EEBMDp me = (EEBMDp) data;
	TimeControlEditDialog *tced;

	if (me->selindex < 0)
		return;

	tced = new TimeControlEditDialog(&(me->localtc));
	tced->setUpdateListener( (UpdateInterface *) me );
	tced->show();
}

void
eebmd_apply(GtkWidget *w, gpointer data)
{
	EEBMDp me;
	list<EngineBookmark *>::iterator ei;
	int i;

	me=(EEBMDp) data;

	if (me->selindex < 0) return;

	ei=global.EnginePresets.begin();
	for(i=me->selindex;i;i--)
		ei++;

	(*ei)->caption     = gtk_entry_get_text(GTK_ENTRY(me->fe[0]));
	(*ei)->directory   = gtk_entry_get_text(GTK_ENTRY(me->fe[1]));
	(*ei)->cmdline     = gtk_entry_get_text(GTK_ENTRY(me->fe[2]));
	(*ei)->timecontrol = me->localtc;
	(*ei)->maxply      = atoi(gtk_entry_get_text(GTK_ENTRY(me->fe[4])));

	(*ei)->think = (*ei)->humanwhite=0;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->fe[5])))
		(*ei)->humanwhite=1;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->fe[6])))
		(*ei)->think=1;

	global.writeRC();

	i=me->selindex;
	me->refresh(); // to update the caption when needed
	gtk_clist_select_row(GTK_CLIST(me->bl), i, 0); // reselect the same entry
	me->updateRightPane();
}
