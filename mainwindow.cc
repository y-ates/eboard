/* $Id: mainwindow.cc,v 1.138 2008/02/22 14:35:43 bergo Exp $ */

/*

  eboard - chess client
  http://eboard.sourceforge.net
  Copyright (C) 2000-2008 Felipe Paulo Guazzi Bergo
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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <gdk/gdkkeysyms.h>
#include "mainwindow.h"
#include "chess.h"
#include "bugpane.h"
#include "global.h"
#include "p2p.h"

#include "config.h"

#ifdef HAVE_LINUX_JOYSTICK_H
#include <linux/joystick.h>
#endif

#include "icon-eboard.xpm"

#include "back1.xpm"
#include "backn.xpm"
#include "forward1.xpm"
#include "forwardn.xpm"
#include "movelist.xpm"
#include "flip.xpm"
#include "trash.xpm"
#include "toscratch.xpm"

#include "sealoff.xpm"
#include "sealon.xpm"

GdkWindow * MainWindow::RefWindow=0;

static GtkItemFactoryEntry mainwindow_mainmenu[] = {
	{ N_("/_Peer"),                NULL,         NULL, 0, "<Branch>" },
	{ N_("/Peer/Connect to _FICS"),NULL,GTK_SIGNAL_FUNC(peer_connect_fics),0,NULL },
	{ N_("/Peer/ICS _Bookmarks"),  NULL, NULL, FA_ICSBOOKMARKS, "<Branch>" },
	{ N_("/Peer/_Connect to Other Server..."),NULL,GTK_SIGNAL_FUNC(peer_connect_ask),0,NULL },
	{ N_("/Peer/Direct connect with _Remote eboard..."),NULL,GTK_SIGNAL_FUNC(peer_connect_p2p),0,NULL },
	{ N_("/Peer/sep4"),            NULL,           NULL, 0, "<Separator>" },
	{ N_("/Peer/Play against _Engine"),NULL, NULL, 0,"<Branch>" },
	{ N_("/Peer/Play against Engine/GNU Chess _4..."),NULL,GTK_SIGNAL_FUNC(peer_connect_gnuchess4),0, NULL },
	{ N_("/Peer/Play against Engine/_Sjeng..."),NULL,GTK_SIGNAL_FUNC(peer_connect_sjeng), 0, NULL },
	{ N_("/Peer/Play against Engine/Cr_afty..."),NULL,GTK_SIGNAL_FUNC(peer_connect_crafty), 0, NULL },
	{ N_("/Peer/Play against Engine/sep1"),NULL,NULL,0,"<Separator>" },
	{ N_("/Peer/Play against Engine/_Generic Engine..."),NULL,GTK_SIGNAL_FUNC(peer_connect_xboard), 0,NULL },
	{ N_("/Peer/Engine B_ookmarks"), NULL, NULL, FA_ENGBOOKMARKS,"<Branch>" },
	{ N_("/Peer/sep3"),            NULL,           NULL, 0, "<Separator>" },
	{ N_("/Peer/_Empty Scratch Board"), NULL,  GTK_SIGNAL_FUNC(peer_scratch_empty), 0, NULL },
	{ N_("/Peer/_Scratch Board with Initial Position"),NULL,  GTK_SIGNAL_FUNC(peer_scratch_initial), 0, NULL },
	{ N_("/Peer/sep3"),            NULL,           NULL, 0, "<Separator>" },
	{ N_("/Peer/_Disconnect"),     NULL,  GTK_SIGNAL_FUNC(peer_disconnect), 0, NULL },
	{ N_("/Peer/sep2"),            NULL,           NULL, 0, "<Separator>" },
	{ N_("/Peer/_Quit"),           NULL,  GTK_SIGNAL_FUNC(mainwindow_destroy), 0, NULL },
	{ N_("/_Game"),                NULL,         NULL, 0, "<Branch>" },
	{ N_("/Game/_Resign"),         NULL,  GTK_SIGNAL_FUNC(game_resign), 0, NULL },
	{ N_("/Game/_Offer Draw"),     NULL,  GTK_SIGNAL_FUNC(game_draw), 0, NULL },
	{ N_("/Game/_Abort"),          NULL,  GTK_SIGNAL_FUNC(game_abort), 0, NULL },
	{ N_("/Game/Ad_journ"),        NULL,  GTK_SIGNAL_FUNC(game_adjourn), 0, NULL },
	{ N_("/Game/Retract _Move"),   NULL,  GTK_SIGNAL_FUNC(game_retract), 0, NULL },
	{ N_("/_Settings"),            NULL,         NULL, 0, "<Branch>" },
	{ N_("/Settings/_Highlight Last Move"),NULL, NULL, FA_HIGHLIGHT, "<CheckItem>" },
	{ N_("/Settings/_Animate Moves"),NULL,       NULL, FA_ANIMATE,"<CheckItem>" },
	{ N_("/Settings/Pre_move"),      NULL,       NULL, FA_PREMOVE,"<CheckItem>" },
	{ N_("/Settings/Sho_w Coordinates"), NULL,   NULL, FA_COORDS,"<CheckItem>" },
	{ N_("/Settings/sep3"),          NULL,       NULL, 0, "<Separator>" },
	{ N_("/Settings/_Beep on Opponent Moves"),NULL,NULL, FA_MOVEBEEP,"<CheckItem>" },
	{ N_("/Settings/_Enable Other Sounds"),NULL,NULL,    FA_SOUND,"<CheckItem>" },
	{ N_("/Settings/sep3"),          NULL,       NULL, 0, "<Separator>" },
	{ N_("/Settings/_ICS Behavior"),   NULL,       NULL, 0, "<Branch>" },
	{ N_("/Settings/ICS Behavior/Popup Board Panes _Upon Creation"),NULL,NULL,FA_POPUP,"<CheckItem>" },
	{ N_("/Settings/ICS Behavior/Smart _Discard Observed Boards After Game Ends"),NULL,NULL,FA_SMART,"<CheckItem>" },
	{ N_("/Settings/sep3"),          NULL,       NULL, 0, "<Separator>" },
	{ N_("/Settings/Enable Legality _Checking"),NULL,NULL,FA_LEGALITY,"<CheckItem>"},
	{ N_("/Settings/sep3"),          NULL,       NULL, 0, "<Separator>" },
	{ N_("/Settings/_Vectorized Pieces (Faster Rendering)"),NULL,NULL,FA_VECTOR,"<CheckItem>" },
	{ N_("/Settings/Bitmapped Piece _Sets"),   NULL,       NULL, 0, "<Branch>" },
	{ N_("/Settings/Bitmapped Piece Sets/Load _Theme"), NULL,NULL, FA_LOADTHEME, "<Branch>" },
	{ N_("/Settings/Bitmapped Piece Sets/Load _Pieces Only"),NULL,NULL,FA_LOADPIECES,"<Branch>"},
	{ N_("/Settings/Bitmapped Piece Sets/Load _Squares Only"), NULL, NULL, FA_LOADSQUARES, "<Branch>" },
	{ N_("/Settings/sep3"),          NULL,       NULL, 0, "<Separator>" },
	{ N_("/Settings/_Preferences..."), NULL,  GTK_SIGNAL_FUNC(sett_prefs), 0, NULL },
	{ N_("/_Windows"),              NULL,         NULL,0, "<Branch>" },
	{ N_("/Windows/_Games on Server..."),NULL,  GTK_SIGNAL_FUNC(windows_games), 0, NULL },
	{ N_("/Windows/_Ads on Server..."),NULL,  GTK_SIGNAL_FUNC(windows_sough), 0, NULL },
	{ N_("/Windows/sep1"), NULL, NULL, 0, "<Separator>" },
	{ N_("/Windows/Games on _Client..."),NULL,  GTK_SIGNAL_FUNC(windows_stock), 0, NULL },
	{ N_("/Windows/sep2"), NULL, NULL, 0, "<Separator>" },
	{ N_("/Windows/_Run Script..."),  NULL,  GTK_SIGNAL_FUNC(windows_script), 0, NULL },
	{ N_("/Windows/sep3"), NULL, NULL, 0, "<Separator>" },
	{ N_("/Windows/_Detached Console"),  NULL,  GTK_SIGNAL_FUNC(windows_detached), 0, NULL },
	{ N_("/Windows/sep3"), NULL, NULL, 0, "<Separator>" },
	{ N_("/Windows/_Find Text (upwards)..."), NULL, GTK_SIGNAL_FUNC(windows_find), 0, NULL },
	{ N_("/Windows/Find _Previous"), NULL, GTK_SIGNAL_FUNC(windows_findp), 0, NULL },
	{ N_("/Windows/Save _Text Buffer..."), NULL, GTK_SIGNAL_FUNC(windows_savebuffer),0,NULL},
	{ N_("/Windows/sep3"), NULL, NULL, 0, "<Separator>" },
	{ N_("/Windows/Save Desktop _Geometry"), NULL, GTK_SIGNAL_FUNC(windows_savedesk), 0, NULL },
	{ N_("/_Help"),              NULL,         NULL,0, "<Branch>" },
	{ N_("/Help/_Getting Started"),NULL, GTK_SIGNAL_FUNC(help_starting), 0, NULL },
	{ N_("/Help/_Keys"),NULL, GTK_SIGNAL_FUNC(help_keys), 0, NULL },
	{ N_("/Help/_Debug Info"),NULL, GTK_SIGNAL_FUNC(help_debug), 0, NULL },
	{ N_("/Help/sep4"),            NULL,           NULL, 0, "<Separator>" },
	{ N_("/Help/_About eboard..."),NULL,  GTK_SIGNAL_FUNC(help_about), 0, NULL },

};

MainWindow *mainw;

gchar * gtkgettext(const char *text, gpointer data) {
	return((gchar *)eboard_gettext(text));
}

MainWindow::MainWindow() {
	GtkAccelGroup *mag;
	GtkWidget *tw[10];
	GtkWidget *bhb,*whb,*dm;

	GtkWidget *P,*SN, *tl1, *tl2;

	Board *tmp;
	int i;
	int nitems=sizeof(mainwindow_mainmenu)/sizeof(mainwindow_mainmenu[0]);

	gamelist=0;
	stocklist=0;
	adlist=0;
	consolecopy=0;
	scriptlist=0;
	ims=0;
	jpd = 0;

	io_tag = -1;

	asetprefix.set("%%prefix *");
	arunscript.set("%%do *");

	for(i=0;i<8;i++)
		nav_enable[i]=true;

	widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_events(widget,GDK_KEY_PRESS_MASK);
	setTitle("eboard");
	gtk_widget_realize(widget);
	gtk_window_resize(GTK_WINDOW(widget),800,600);
	RefWindow=widget->window;

	tooltips=GTK_TOOLTIPS(gtk_tooltips_new());

	/* menu bar */
	mag=gtk_accel_group_new();
	gif=gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", mag);

#ifdef ENABLE_NLS
	gtk_item_factory_set_translate_func(gif,(GtkTranslateFunc) gtkgettext, 0, 0);
#endif

	gtk_item_factory_create_items (gif, nitems, mainwindow_mainmenu, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(widget), mag);

	v=gtk_vbox_new(FALSE,0);
	gshow(v);
	gtk_container_add(GTK_CONTAINER(widget),v);
	menubar = gtk_item_factory_get_widget (gif, "<main>");

	/* find out themes */
	searchThemes();
	global.readRC();

	/* build ics bookmark submenu under Peer */
	updateBookmarks();

	/* promotion menu */
	promote=new PromotionPicker(widget->window);

	/* restore main window geometry */
	if (!global.Desk.wMain.isNull()) {
		gtk_window_move(GTK_WINDOW(widget),global.Desk.wMain.X,global.Desk.wMain.Y);
		gtk_window_resize(GTK_WINDOW(widget),global.Desk.wMain.W,global.Desk.wMain.H);
	}

	whb=gtk_hbox_new(FALSE,0);
	gtk_box_pack_start (GTK_BOX (v), whb, FALSE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (whb), menubar, TRUE, TRUE, 0);
	gshow (menubar);

	gtk_box_pack_end (GTK_BOX (whb), promote->widget, FALSE, FALSE, 0);
	promote->show();

	gshow(whb);

	/* paned widget */
	P=gtk_vpaned_new();
	global.mainpaned=P;
	gtk_box_pack_start(GTK_BOX(v), P, TRUE, TRUE, 0);

	/* main notebook */
	notebook=new Notebook();
	{
		GtkPositionType rqda[4]={GTK_POS_RIGHT,GTK_POS_LEFT,GTK_POS_TOP,GTK_POS_BOTTOM};
		notebook->setTabPosition(rqda[global.TabPos%4]);
	}
	notebook->show();

	SN=gtk_notebook_new();
	gtk_paned_pack1 (GTK_PANED (P), notebook->widget, TRUE, TRUE);
	gtk_paned_pack2 (GTK_PANED (P), SN, TRUE, TRUE);

	/* restore pane divider position */
	if (global.Desk.PanePosition)
		gtk_paned_set_position(GTK_PANED(P),global.Desk.PanePosition);
	else
		gtk_paned_set_position(GTK_PANED(P),7000);

	global.lowernotebook = SN;

	/* the console set */

	icsout=new TextSet();

	/* console snippet */

	tl1=gtk_label_new(_("Console"));
	xconsole=new Text();

	gtk_notebook_append_page(GTK_NOTEBOOK(SN), xconsole->widget, tl1);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(SN), GTK_POS_RIGHT);

	xconsole->show();
	Gtk::show(tl1,SN,P,NULL);

	icsout->addTarget(xconsole);

	/* bugpane */

	global.bugpane = new BugPane();
	global.bugpane->show();
	tl2=gtk_label_new(_("Bughouse"));
	gtk_notebook_append_page(GTK_NOTEBOOK(SN), global.bugpane->widget, tl2);
	gshow(tl2);

	gtk_notebook_set_page(GTK_NOTEBOOK(SN),0);

	/* main board */

	tmp=new Board();
	notebook->addPage(tmp->widget,_("Main Board"),-1);
	tmp->setNotebook(notebook,-1);

	/* quick bar */
	quickbar=new QuickBar(widget);
	gtk_box_pack_start(GTK_BOX(v), quickbar->widget, FALSE,TRUE, 0);
	quickbar->show();
	QuickbarVisible=true;
	if (!global.ShowQuickbar)
		hideQuickbar();

	inconsole=new Text();
	icsout->addTarget(inconsole);
	inconsole->show();
	notebook->addPage(inconsole->widget,_("Console"),-2);
	inconsole->setNotebook(notebook,-2);

	greet();

	/* bottom entry box */
	tw[0]=gtk_frame_new(0);
	gtk_frame_set_shadow_type(GTK_FRAME(tw[0]),GTK_SHADOW_ETCHED_OUT);
	tw[1]=gtk_hbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(tw[0]),tw[1]);
	gtk_container_set_border_width(GTK_CONTAINER(tw[1]),4);
	inputbox=gtk_entry_new();
	gtk_widget_set_events(inputbox,(GdkEventMask)(gtk_widget_get_events(inputbox)|GDK_FOCUS_CHANGE_MASK));

	gtk_signal_connect(GTK_OBJECT(inputbox),"focus_out_event",
					   GTK_SIGNAL_FUNC(mainwindow_input_focus_out),NULL);

	ims=new InputModeSelector();

	gtk_box_pack_start(GTK_BOX(tw[1]),ims->widget,FALSE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(tw[1]),inputbox,TRUE,TRUE,4);


	gshow(inputbox);
	ims->show();
	Gtk::show(tw[0],tw[1],NULL);
	gtk_box_pack_start(GTK_BOX(v),tw[0],FALSE,FALSE,0);

	InputHistory=new History(256);

	/* status bar */
	status=new Status();
	status->show();

	bhb=gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,TRUE,0);
	gshow(bhb);

	// timeseal icon
	createSealPix(bhb);

	gtk_box_pack_start(GTK_BOX(bhb),status->widget,TRUE,TRUE,0);

	/* game browsing buttons */
	createNavbar(bhb);

	gtk_signal_connect (GTK_OBJECT (widget), "delete_event",
						GTK_SIGNAL_FUNC (mainwindow_delete), NULL);
	gtk_signal_connect (GTK_OBJECT (widget), "destroy",
						GTK_SIGNAL_FUNC (mainwindow_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (inputbox), "key_press_event",
						GTK_SIGNAL_FUNC (input_key_press), (gpointer)this);
	gtk_signal_connect (GTK_OBJECT (widget), "key_press_event",
						GTK_SIGNAL_FUNC (main_key_press), (gpointer)this);

	/* FIXME
	   gtk_signal_connect (GTK_OBJECT (inconsole->getTextArea()), "changed",
	   GTK_SIGNAL_FUNC (mainwindow_icsout_changed),
	   (gpointer)this);
	*/

	dm=gtk_item_factory_get_widget_by_action(gif,FA_HIGHLIGHT);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.HilightLastMove);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_hilite),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_ANIMATE);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.AnimateMoves);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_animate),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_PREMOVE);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.Premove);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_premove),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_MOVEBEEP);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.BeepWhenOppMoves);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_beepopp),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_SOUND);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.EnableSounds);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_osound),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_VECTOR);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.UseVectorPieces);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_vector),0);
	vector_checkbox=dm;

	dm=gtk_item_factory_get_widget_by_action(gif,FA_LEGALITY);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.CheckLegality);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_legal),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_COORDS);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.ShowCoordinates);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_coord),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_POPUP);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.PopupSecondaryGames);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_popup),0);

	dm=gtk_item_factory_get_widget_by_action(gif,FA_SMART);
	gmiset(GTK_CHECK_MENU_ITEM(dm),global.SmartDiscard);
	gtk_signal_connect(GTK_OBJECT(dm),"toggled",GTK_SIGNAL_FUNC(sett_smarttrash),0);

	setIcon(icon_eboard_xpm,"eboard");
	HideMode=0;

	global.input=this;
	global.output=icsout;
	global.status=status;
	global.chandler=(ConnectionHandler *)this;
	global.promotion=(PieceProvider *)promote;
	global.ebook=notebook;
	global.inputhistory=InputHistory;
	global.bmlistener=(BookmarkListener *)this;
	global.qbcontainer=(UpdateInterface *)this;
	global.iowatcher=(IONotificationInterface *)this;
	global.quickbar=quickbar;
	global.killbox=tw[1];
	global.toplevelwidget=widget;
	mainw=this;

	notebook->setListener(this);
	paneChanged(0,-1);

	gtk_timeout_add(150,forced_focus,(gpointer)inputbox);
}

void MainWindow::update() {
	bool dState;
	dState = global.ShowQuickbar ? true : false;
	if (dState != QuickbarVisible) {
		if (QuickbarVisible) hideQuickbar(); else showQuickbar();
	}
}

void MainWindow::showQuickbar() {
	if (!QuickbarVisible) {
		quickbar->update();
		quickbar->show();
		QuickbarVisible = true;
	}
}

void MainWindow::hideQuickbar() {
	if (QuickbarVisible) {
		quickbar->hide();
		QuickbarVisible = false;
	}
}

void MainWindow::setSealPix(int flag) {
	gtk_pixmap_set(GTK_PIXMAP(picseal),sealmap[flag?1:0],
				   sealmask[flag?1:0]);
	gtk_widget_queue_resize(picseal);
}

void MainWindow::createSealPix(GtkWidget *box) {
	GtkWidget *fr;
	GtkStyle *style;

	fr=gtk_frame_new(0);
	gtk_frame_set_shadow_type(GTK_FRAME(fr),GTK_SHADOW_ETCHED_OUT);

	style=gtk_widget_get_style(widget);
	sealmap[0] = gdk_pixmap_create_from_xpm_d (widget->window, &sealmask[0],
											   &style->bg[GTK_STATE_NORMAL],
											   (gchar **) sealoff_xpm);
	sealmap[1] = gdk_pixmap_create_from_xpm_d (widget->window, &sealmask[1],
											   &style->bg[GTK_STATE_NORMAL],
											   (gchar **) sealon_xpm);

	picseal=gtk_pixmap_new(sealmap[0],sealmask[0]);
	gtk_container_add(GTK_CONTAINER(fr),picseal);

	gtk_box_pack_start(GTK_BOX(box),fr,FALSE,FALSE,0);
	Gtk::show(picseal,fr,NULL);
}

void MainWindow::createNavbar(GtkWidget *box) {
	GdkPixmap *d[8];
	GdkBitmap *mask[8];
	GtkWidget *p[8],*b[8],*fr,*mb,*lb;
	GtkStyle *style;
	int i;

	style=gtk_widget_get_style(widget);
	d[0] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[0],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) backn_xpm);
	d[1] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[1],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) back1_xpm);
	d[2] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[2],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) forward1_xpm);
	d[3] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[3],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) forwardn_xpm);
	d[4] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[4],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) movelist_xpm);
	d[5] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[5],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) flip_xpm);
	d[6] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[6],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) trash_xpm);
	d[7] = gdk_pixmap_create_from_xpm_d (widget->window, &mask[7],
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) toscratch_xpm);
	mb=gtk_hbox_new(FALSE,0);
	fr=gtk_frame_new(0);
	gtk_frame_set_shadow_type(GTK_FRAME(fr),GTK_SHADOW_ETCHED_OUT);
	gtk_container_add(GTK_CONTAINER(fr),mb);

	lb=gtk_label_new(_("Game/Board: "));
	gtk_box_pack_start(GTK_BOX(mb),lb,FALSE,FALSE,2);

	for(i=0;i<8;i++) {
		b[i]=gtk_button_new();
		p[i]=gtk_pixmap_new(d[i],mask[i]);
		gtk_container_add(GTK_CONTAINER(b[i]),p[i]);
		gshow(p[i]);
		gtk_box_pack_start(GTK_BOX(mb),b[i],FALSE,TRUE,0);
		gshow(b[i]);
		navbar[i]=b[i];
	}


	gtk_box_pack_end(GTK_BOX(box),fr,FALSE,FALSE,0);
	Gtk::show(mb,lb,fr,NULL);

	gtk_tooltips_set_tip(tooltips,b[0],_("goes back to start of game"),0);
	gtk_tooltips_set_tip(tooltips,b[1],_("goes back 1 halfmove"),0);
	gtk_tooltips_set_tip(tooltips,b[2],_("goes forward 1 halfmove"),0);
	gtk_tooltips_set_tip(tooltips,b[3],_("goes forward to end of game"),0);
	gtk_tooltips_set_tip(tooltips,b[4],_("pops up the move list"),0);
	gtk_tooltips_set_tip(tooltips,b[5],_("flips board"),0);
	gtk_tooltips_set_tip(tooltips,b[6],_("discards board"),0);
	gtk_tooltips_set_tip(tooltips,b[7],_("opens new scratch board with position"),0);

	gtk_signal_connect(GTK_OBJECT(b[0]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_back_all),this);
	gtk_signal_connect(GTK_OBJECT(b[1]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_back_1),this);
	gtk_signal_connect(GTK_OBJECT(b[2]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_forward_1),this);
	gtk_signal_connect(GTK_OBJECT(b[3]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_forward_all),this);
	gtk_signal_connect(GTK_OBJECT(b[4]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_movelist),this);
	gtk_signal_connect(GTK_OBJECT(b[5]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_flip),this);
	gtk_signal_connect(GTK_OBJECT(b[6]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_trash),this);
	gtk_signal_connect(GTK_OBJECT(b[7]),"clicked",
					   GTK_SIGNAL_FUNC(navbar_toscratch),this);
}

void MainWindow::setTitle(char *msg) {
	gtk_window_set_title(GTK_WINDOW(widget),msg);
}

void MainWindow::restoreDesk() {
	// main window is restored in the constructor

	if (! global.Desk.wGames.isNull() ) {
		openGameList();
		gamelist->restorePosition(& global.Desk.wGames);
	}

	if (! global.Desk.wLocal.isNull() ) {
		openStockList();
		stocklist->restorePosition(& global.Desk.wLocal);
	}

	if (! global.Desk.wAds.isNull() ) {
		openAdList();
		adlist->restorePosition(& global.Desk.wAds);
	}

	global.Desk.spawnConsoles(icsout);
}

void MainWindow::parseThemeFile(char *name) {
	static char * comma = ",";
	char s[256], aux[16];
	ThemeEntry *te;
	list<ThemeEntry *>::iterator it;
	tstring t;
	string *p;
	int ndc=0;

	if (name==0) return;

	ifstream f(name);
	if (!f) return;

	global.debug("MainWindow","parseThemeFile",name);

	while( memset(s,0,256), f.getline(s,255,'\n') ) {
		if (s[0]=='#') continue;

		// sound file
		if (s[0]=='+') {
			t.set(&s[1]);
			p=t.token(comma);
			if ( p && !global.hasSoundFile(*p) )
				global.SoundFiles.push_back( * (new string(*p)) );
			continue;
		}

		t.set(s);
		p=t.token(comma);
		if (!p) continue;
		te=new ThemeEntry();
		te->Filename=*p;
		p=t.token(comma);
		if (p) te->Text=*p;

		// avoid dupes
		for(it=Themes.begin();it!=Themes.end();it++) {
			if ( (*it)->isDupe(te) ) {
				delete te;
				te=0;
				break;
			}
			if ( (*it)->isNameDupe(te) ) {
				++ndc;
				snprintf(aux,16," (%d)",ndc);
				te->Text+=aux;
			}
		}

		if (te)
			Themes.push_back(te);
	}
	f.close();
}

void MainWindow::updateBookmarks() {
	GtkWidget *pmenu, *item;
	GList *r,*s;
	list<HostBookmark *>::iterator hi;
	list<EngineBookmark *>::iterator ei;
	char z[256];
	int i;
	string x;

	// ics bookmarks

	pmenu=gtk_item_factory_get_widget_by_action(gif,FA_ICSBOOKMARKS);

	r=gtk_container_children(GTK_CONTAINER(pmenu));
	for(s=r;s!=0;s=g_list_next(s))
		gtk_container_remove( GTK_CONTAINER(pmenu), GTK_WIDGET(s->data) );
	g_list_free(r);

	i=1;
	for(hi=global.HostHistory.begin();hi!=global.HostHistory.end();hi++,i++) {
		snprintf(z,256,_("%d. Connect to %s:%d (%s)"),i,(*hi)->host,(*hi)->port,(*hi)->protocol);
		item=gtk_menu_item_new_with_label( z );

		gtk_signal_connect(GTK_OBJECT(item),"activate",
						   GtkSignalFunc(mainwindow_connect_bookmark),
						   (gpointer)(*hi));

		gtk_menu_shell_append(GTK_MENU_SHELL(pmenu),item);
		gshow(item);
	}

	if (global.HostHistory.empty()) {
		item=gtk_menu_item_new_with_label(_("(no bookmarks)"));
		gtk_widget_set_sensitive(item,FALSE);
		gtk_menu_shell_append(GTK_MENU_SHELL(pmenu),item);
		gshow(item);
	}

	// engine bookmarks

	pmenu=gtk_item_factory_get_widget_by_action(gif,FA_ENGBOOKMARKS);

	r=gtk_container_children(GTK_CONTAINER(pmenu));
	for(s=r;s!=0;s=g_list_next(s))
		gtk_container_remove( GTK_CONTAINER(pmenu), GTK_WIDGET(s->data) );
	g_list_free(r);

	if (!global.EnginePresets.empty()) {
		item=gtk_menu_item_new_with_label(_("Edit Bookmarks..."));
		gtk_menu_shell_append(GTK_MENU_SHELL(pmenu),item);
		gshow(item);

		gtk_signal_connect(GTK_OBJECT(item),"activate",
						   GtkSignalFunc(mainwindow_edit_engbm),
						   (gpointer) this);

		item=gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(pmenu),item);
		gshow(item);
	}

	i=1;
	for(ei=global.EnginePresets.begin();ei!=global.EnginePresets.end();ei++, i++) {
		snprintf(z,256,"%d. ",i);
		x=z;
		x+=(*ei)->caption;
		item=gtk_menu_item_new_with_label( x.c_str() );

		gtk_signal_connect(GTK_OBJECT(item),"activate",
						   GtkSignalFunc(mainwindow_connect_bookmark2),
						   (gpointer)(*ei));

		gtk_menu_shell_append(GTK_MENU_SHELL(pmenu),item);
		gshow(item);
	}

	if (global.EnginePresets.empty()) {
		item=gtk_menu_item_new_with_label(_("(no bookmarks)"));
		gtk_widget_set_sensitive(item,FALSE);
		gtk_menu_shell_append(GTK_MENU_SHELL(pmenu),item);
		gshow(item);
	}
}

void MainWindow::searchThemes() {
	EboardFileFinder eff;
	list<ThemeEntry *>::iterator it;
	GtkWidget *tmenu[3];
	GtkWidget *menuitem;
	int i,j;
	char tmp[512];
	DIR *dh;
	struct dirent *ds;
	ExtPatternMatcher ExtraConf;
	string s;

	tmenu[0]=gtk_item_factory_get_widget_by_action(gif,FA_LOADTHEME);
	tmenu[1]=gtk_item_factory_get_widget_by_action(gif,FA_LOADPIECES);
	tmenu[2]=gtk_item_factory_get_widget_by_action(gif,FA_LOADSQUARES);

	j=eff.getPathCount();

	for(i=0;i<j;i++) {
		g_strlcpy(tmp,eff.getPath(i).c_str(),512);
		g_strlcat(tmp,"/",512);
		g_strlcat(tmp,"eboard_themes.conf",512);
		parseThemeFile(tmp);
	}

	// now load all DATADIR/eboard/themeconf.*
	dh=opendir(DATADIR "/eboard");
	if (dh) {
		ExtraConf.set("themeconf.*");

		while( (ds=readdir(dh)) != 0 ) {
			if (ExtraConf.match(ds->d_name)) {
				snprintf(tmp,512,DATADIR "/eboard/%s",ds->d_name);
				parseThemeFile(tmp);
			}
		}
		closedir(dh);
	}

	if (!Themes.empty()) {
		for(i=0,it=Themes.begin();it!=Themes.end();it++,i++) {

			// load all

			menuitem=gtk_menu_item_new_with_label( (*it)->Text.c_str() );
			gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
							   GtkSignalFunc(mainwindow_themeitem),
							   (gpointer)(*it));
			gtk_menu_shell_append(GTK_MENU_SHELL(tmenu[0]),menuitem);
			gshow(menuitem);

			// load pieces

			menuitem=gtk_menu_item_new_with_label( (*it)->Text.c_str() );
			gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
							   GtkSignalFunc(mainwindow_themeitem2),
							   (gpointer)(*it));
			gtk_menu_shell_append(GTK_MENU_SHELL(tmenu[1]),menuitem);
			gshow(menuitem);

			// load squares

			menuitem=gtk_menu_item_new_with_label( (*it)->Text.c_str() );
			gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
							   GtkSignalFunc(mainwindow_themeitem3),
							   (gpointer)(*it));
			gtk_menu_shell_append(GTK_MENU_SHELL(tmenu[2]),menuitem);
			gshow(menuitem);

		}
		global.setPieceSet(Themes.front()->Filename,true,true);
	} else {
		s="classic.png";
		global.setPieceSet(s,true,true);
	}
}

void MainWindow::greet() {
	char z[128];
	snprintf (z,128,_("eboard version %s (%s)"),global.Version,global.SystemType);
	icsout->append(z,0xc0ff00,IM_IGNORE);
	snprintf (z,128,_("(c) 2000-%d Felipe Bergo <fbergo@gmail.com> (FICS handle: Pulga)"), 2007);
	icsout->append(z,0xc0ff00,IM_IGNORE);
	icsout->append(_("Distributed under the terms of the GNU General Public License, version 2 or later"),0xffc000,IM_IGNORE);
	icsout->append("http://www.gnu.org/copyleft/gpl.html",0xffc000,IM_IGNORE);
	icsout->append(_("Source code available at http://eboard.sourceforge.net"),0xc0ff00,IM_IGNORE);
	icsout->append("---",0xc0ff00,IM_IGNORE);

}

void MainWindow::updatePrefix() {
	int id;
	id=notebook->getCurrentPageId();
	if (id==-2) {
		imscache.set(-2,global.ConsoleReply);
		ims->setPrefix( * (imscache.get(-2)) );
	}
}

void MainWindow::setPasswordMode(int pm) {
	HideMode=pm;
	global.PauseLog=pm;
	gtk_entry_set_visibility(GTK_ENTRY(inputbox),(pm?FALSE:TRUE));
	global.setPasswordMode(pm);
}

void MainWindow::openGameList() {
	if (gamelist)
		return;
	gamelist=new GameListDialog(this);
	gamelist->show();
}

void MainWindow::openAdList() {
	if (adlist)
		return;
	adlist=new AdListDialog(this);
	adlist->show();
}

void MainWindow::gameListClosed() {
	if (gamelist)
		delete gamelist;
	gamelist=0;
}

void MainWindow::openStockList() {
	if (stocklist)
		return;
	stocklist=new StockListDialog(this);
	stocklist->show();
}

void MainWindow::stockListClosed() {
	if (stocklist)
		delete stocklist;
	stocklist=0;
}

void MainWindow::adListClosed() {
	if (adlist)
		delete adlist;
	adlist=0;
}

void MainWindow::openDetachedConsole() {
	if (consolecopy)
		return;
	consolecopy=new DetachedConsole(icsout,this);
	consolecopy->show();
}

void MainWindow::peekKeys(GtkWidget *who) {
	gtk_signal_connect(GTK_OBJECT(who),"key_press_event",
					   GTK_SIGNAL_FUNC(main_key_press),(gpointer)this);
}

void MainWindow::consoleClosed() {
	if (consolecopy)
		delete consolecopy;
	consolecopy=0;
}

void MainWindow::userInput(const char *text) {
	char *nv;
	int i,j;
	if (text==0) return;

	j=strlen(text);

	nv=(char *) Global::safeMalloc(j+1);
	strcpy(nv,text);

	// join multi-line pastes in one single line
	for(i=0;i<j;i++)
		if ((nv[i]=='\n')||(nv[i]=='\r'))
			nv[i]=' ';

	if (HideMode)
		icsout->append(_("> (password sent)"),global.SelfInputColor);
	else
		icsout->append("> ",nv,global.SelfInputColor);

	if (global.protocol)
		global.protocol->sendUserInput(nv);

	if (!HideMode)
		InputHistory->appendString(nv);
}

void MainWindow::injectInput() {
	char z[4096],y[256];
	int id;

	g_strlcpy(z,gtk_entry_get_text(GTK_ENTRY(inputbox)),4096);

	if (ims->getSearchMode()) {
		inconsole->SearchString = z;
		inconsole->execSearch();
		ims->setSearchMode(false);
		goto ii_nothing_else;
	}

	if (asetprefix.match(z)) {
		id=notebook->getCurrentPageId();
		imscache.set(id, asetprefix.getStarToken(0) );
		ims->setPrefix( * (imscache.get(id)) );
		goto ii_nothing_else;
	}

	if (arunscript.match(z)) {
		g_strlcpy(y,arunscript.getStarToken(0),256);
		new ScriptInstance(y);
		goto ii_nothing_else;
		return;
	}

	if (ims->getChatMode()) {
		g_strlcpy(z,ims->getPrefix().c_str(),4096);
		if (z[0])
			g_strlcat(z," ",4096);
	} else
		z[0]=0;
	g_strlcat(z,gtk_entry_get_text(GTK_ENTRY(inputbox)),4096);

	userInput(z);

 ii_nothing_else:
	gtk_entry_set_text(GTK_ENTRY(inputbox),"\0");
}

void MainWindow::historyUp() {
	gtk_entry_set_text(GTK_ENTRY(inputbox),InputHistory->moveUp());
	gtk_editable_set_position(GTK_EDITABLE(inputbox),-1);
}

void MainWindow::historyDown() {
	gtk_entry_set_text(GTK_ENTRY(inputbox),InputHistory->moveDown());
	gtk_editable_set_position(GTK_EDITABLE(inputbox),-1);
}

void MainWindow::saveBuffer() {
	inconsole->saveBuffer();
}

void MainWindow::saveDesk() {
	global.Desk.clear();

	global.Desk.wMain.retrieve(widget);
	if (gamelist)   global.Desk.wGames.retrieve(gamelist->widget);
	if (adlist)     global.Desk.wAds.retrieve(adlist->widget);
	if (stocklist)  global.Desk.wLocal.retrieve(stocklist->widget);
	if (scriptlist) global.Desk.wLocal.retrieve(scriptlist->widget);

	global.Desk.PanePosition = gtk_paned_get_position(GTK_PANED(global.mainpaned));
	global.gatherConsoleState();
	global.writeRC();
}

void MainWindow::openServer(char *host,int port,Protocol *protocol,
							char *helper) {
	global.debug("MainWindow","openServer",host);
	inconsole->pop();
	tryConnect(host,port,protocol,helper);
}

void MainWindow::openServer(NetConnection *conn, Protocol *protocol) {
	if (!conn->isConnected())
		return;

	inconsole->pop();

	if ((global.protocol)||(global.network))
		disconnect();

	global.protocol=protocol;
	global.network=conn;
	global.network->notifyReadReady(this);

	setSealPix(conn->hasTimeGuard());
	//  incrementHook();
}

void MainWindow::openXBoardEngine() {
	XBoardProtocol *xpp;
	global.debug("MainWindow","openXBoardEngine");
	xpp=new XBoardProtocol();
	openEngine(xpp);
}

void MainWindow::openGnuChess4() {
	GnuChess4Protocol *gpp;
	global.debug("MainWindow","openGnuChess4");
	gpp=new GnuChess4Protocol();
	openEngine(gpp);
}

void MainWindow::openCrafty() {
	CraftyProtocol *cpp;
	global.debug("MainWindow","openCrafty");
	cpp=new CraftyProtocol();
	openEngine(cpp);
}

void MainWindow::openSjeng() {
	SjengProtocol *spp;
	global.debug("MainWindow","openSjeng");
	spp=new SjengProtocol();
	openEngine(spp);
}

void MainWindow::openEngineBookmark(EngineBookmark *bm) {
	EngineProtocol *xpp;

	switch(bm->proto) {
	case 0: xpp=new XBoardProtocol(); break;
	case 1: xpp=new CraftyProtocol(); break;
	case 2: xpp=new SjengProtocol(); break;
	case 3: xpp=new GnuChess4Protocol(); break;
	default:
		cerr << _("** [eboard] bad engine protocol # in bookmark: ") << bm->proto << endl;
		return;
	}

	disconnect();
	if (xpp->run(bm)) {
		// success
		global.protocol=xpp;
		global.network->notifyReadReady(this);
		// incrementHook();
	}
}

void MainWindow::openEngine(EngineProtocol *xpp, EngineBookmark *ebm) {
	int i;
	global.debug("MainWindow","openEngine");
	disconnect();

	if (ebm)
		i=xpp->run(ebm);
	else
		i=xpp->run();

	if (i) {
		// success
		global.protocol=xpp;
		global.network->notifyReadReady(this);
		// incrementHook();
	}
}

void MainWindow::tryConnect(char *host,int port,Protocol *protocol,
							char *helper)
{
	DirectConnection *bytcp;
	PipeConnection *bypipe;
	FallBackConnection *glue;
	NetConnection *net;

	global.debug("MainWindow","tryConnect",host);

	if ((global.protocol)||(global.network))
		disconnect();

	if (helper) {
		glue=new FallBackConnection();
		bypipe=new PipeConnection(host,port,helper,global.SystemType);
		bypipe->TimeGuard=1;
		bytcp=new DirectConnection(host,port);
		glue->append(bypipe);
		glue->append(bytcp);
		net=glue;
	} else {
		net=new DirectConnection(host,port);
	}

	global.protocol=protocol;
	global.network=net;

	if (net->open()) {
		// failure
		global.protocol=NULL;
		global.network=NULL;
		status->setText(net->getError(),10);
		delete net;
		delete protocol;
	} else {
		// success
		setSealPix(net->hasTimeGuard());
		net->notifyReadReady(this);
		// incrementHook();
	}
}

void MainWindow::disconnect() {
	global.debug("MainWindow","disconnect");
	cleanUpConnection();
}

void MainWindow::cleanUpConnection() {
	list<Board *>::iterator bli;
	global.debug("MainWindow","cleanUpConnection");
	setSealPix(0);

	if (global.protocol!=NULL) {
		global.protocol->finalize();
		delete global.protocol;
		global.protocol=NULL;
	}
	if (global.network!=NULL) {
		global.network->close();
		delete global.network;
		global.network=NULL;
		icsout->append(_("--- Disconnected"),0xc0ff00);
	}

	// stop all clocks
	for(bli=global.BoardList.begin();bli!=global.BoardList.end();bli++)
		(*bli)->stopClock();

	status->setText(_("No peer."),15);
}

void MainWindow::cloneOnScratch(ChessGame *cg0) {
	Board *b;
	ChessGame *cg;
	char z[64];
	int id;

	id = global.nextFreeGameId(30000);
	cg=new ChessGame(cg0);
	cg->GameNumber = id;
	cg->LocalEdit = true;
	cg->source = GS_Other;

	global.appendGame(cg,false);
	b=new EditBoard(cg);
	cg->setBoard(b);
	cg->StopClock = 1;

	snprintf(z,64,_("Scratch %d"),++global.LastScratch);
	notebook->addPage(b->widget,z,id);
	b->setNotebook(notebook,id);
	notebook->goToPageId(id);
	b->update();
}

void MainWindow::newScratchBoard(bool clearboard) {
	Board *b;
	ChessGame *cg;
	Position *p;
	int id,i,j;
	char z[64];

	id = global.nextFreeGameId(30000);

	cg=new ChessGame(id, 0, 0, 0, REGULAR,
					 "Nobody","Nessuno");
	cg->LocalEdit=true;

	global.appendGame(cg,false);
	b=new EditBoard(cg);
	cg->setBoard(b);
	cg->StopClock = 1;

	snprintf(z,64,_("Scratch %d"),++global.LastScratch);
	notebook->addPage(b->widget,z,id);
	b->setNotebook(notebook,id);

	p=new Position();
	if (clearboard)
		for(i=0;i<8;i++)
			for(j=0;j<8;j++)
				p->setPiece(i,j,EMPTY);

	cg->updatePosition2(*p,1,0,0,0,_("<editing>"),false);
	notebook->goToPageId(id);
}

void MainWindow::gameWalk(int op) {
	Board *b=0;
	ChessGame *cg=0;
	int id;

	global.debug("MainWindow","gameWalk");

	id=notebook->getCurrentPageId();

	if (id<=-2) // text panes
		return;

	if (id==-1)
		b=global.BoardList.front();
	else {
		cg=global.getGame(id);
		if (cg)
			b=cg->getBoard();
	}

	if (!b)
		return;

	// examining
	if (cg)
		if (cg->protodata[0]==258) {
			switch(op) {
			case 0: global.protocol->exaBackward(999); return;
			case 1: global.protocol->exaBackward(1); return;
			case 2: global.protocol->exaForward(1); return;
			case 3: global.protocol->exaForward(999); return;
			}
		}

	switch(op) {
	case 0: b->walkBackAll(); break;
	case 1: b->walkBack1(); break;
	case 2: b->walkForward1(); break;
	case 3: b->walkForwardAll(); break;
	case 4: b->openMovelist(); break;
	case 5:
		if ((id<=0)||(!cg)) break;
		if ( (cg->protodata[1]) || ( ! global.protocol ) || (cg->LocalEdit) ) {
			cg->protodata[1]=0;
			global.ebook->removePage(cg->GameNumber);
			global.removeBoard(b);
			delete(b);
			cg->setBoard(NULL);
			if (cg->LocalEdit)
				global.deleteGame(cg);
		} else {
			if ((id>0)&&(global.protocol!=NULL))
				global.protocol->discardGame(cg->GameNumber);
		}
		break;
	case 6:
		b->setFlipInversion(!b->getFlipInversion());
		break;
	case 7: // clone on scratch
		cloneOnScratch(b->getGame());
		break;
	}
}

static int smart_remove_pending=0;

// take care of pending 'smart removals'
gboolean do_smart_remove(gpointer data) {
	vector<int> evil_ids;
	int i,j,pgid;

	smart_remove_pending=0;
	pgid=global.ebook->getCurrentPageId();

	global.ebook->getNaughty(evil_ids);
	j=evil_ids.size();
	if ( (!j) || (! global.protocol) ) return FALSE;
	for(i=0;i<j;i++) {
		if (evil_ids[i]==pgid) continue; /* almost impossible, would require a heck
											of race condition */
		if (evil_ids[i]>=0)
			global.protocol->discardGame(evil_ids[i]);
	}

	return FALSE;
}

void MainWindow::joystickEvent(JoystickEventType jet, int number, int value) {
	int id, anum;
	ChessGame *cg = NULL;
	Board *b = NULL;

	if (global.joycapture != NULL) {
		global.joycapture->joystickEvent(jet,number,value);
		return;
	}

	id=notebook->getCurrentPageId();

	if (id==-1)
		b=global.BoardList.front();
	else {
		cg=global.getGame(id);
		if (cg!=NULL)
			b=cg->getBoard();
	}

	anum = number & 0xfffe;

	if (jet == JOY_AXIS) {
		if (anum == global.JSCursorAxis && b!=NULL &&
			(global.JSMode==1 || value<-16000 || value>16000 || value==0) )
			b->joystickCursor(number%2, value);
		if (number == global.JSBrowseAxis && b!=NULL && (value<-16000 || value>16000 || value==0) ) {
			if (value < 0) value = -1;
			if (value > 0) value =  1;

			if (value < 0 && jpd!=value) b->walkBack1();
			if (value > 0 && jpd!=value) b->walkForward1();
			jpd = value;
		}
	}

	if (jet == JOY_BUTTON && value==1) {
		if (number == global.JSMoveButton && b!=NULL)
			b->joystickSelect();
		if (number == global.JSPrevTabButton)
			global.ebook->goToPrevious();
		if (number == global.JSNextTabButton)
			global.ebook->goToNext();
	}

}

// changes enabling of buttons upon pane change
// also schedules 'smart removes' for finished boards
void MainWindow::paneChanged(int pgseq,int pgid) {
	bool nv[8];
	int i;

	// #     button(s)  constraint for enabling
	// ------------------------------------------------------
	// 0..3  game move   requires game
	// 4     move list   requires game
	// 5     flip        requires board
	// 6     trash       requires board other than main board
	// 7     new scratch requires game

	// main board: -1  console: -2  seek graph: -3
	// channel tabs: -200 .. -455 (for FICS, formula is -(200+channelnum) )
	// other boards: >= 0
	// most probably computer thinking pane will be -4

	// change talking prefix as appropriate:

	if ( (pgid != -2) && imscache.get(pgid)) {
		ims->setPrefix(* (imscache.get(pgid)));
	} else if (pgid <= -200) {
		int ch;
		char z[32];
		ch=-200-pgid;
		snprintf(z,32,"t %d",ch);
		ims->setPrefix(z);
		imscache.set(pgid, ims->getPrefix());
	} else if (pgid >= 0) {
		ims->setPrefix(",");
		imscache.set(pgid, ims->getPrefix());
	} else if (pgid == -2) {
		ims->setPrefix(global.ConsoleReply);
		imscache.set(pgid, ims->getPrefix());
	} else if (pgid == -1) {
		ims->setPrefix("say");
		imscache.set(pgid, ims->getPrefix());
	}

	for(i=0;i<8;i++) nv[i]=true;

	// 0..4
	if (pgid < -1) nv[0]=false;
	if ( (pgid == -1) && (! global.BoardList.front()->hasGame()) ) nv[0]=false;

	nv[1]=nv[2]=nv[3]=nv[4]=nv[7]=nv[0];

	// 5
	if (pgid < -1) nv[5]=false;

	// 6
	if (pgid < 0) nv[6]=false;

	for(i=0;i<8;i++) {
		if (nv[i]!=nav_enable[i]) {
			gtk_widget_set_sensitive(navbar[i],nv[i]?TRUE:FALSE);
			nav_enable[i]=nv[i];
		}
	}

	if ((global.ebook->hasNaughty())&&(!smart_remove_pending)) {
		smart_remove_pending=1;
		gtk_timeout_add(500,do_smart_remove,NULL);
	}
}

// some key was pressed in some other window, but it's (possibly) a
// global keybind
int
MainWindow::keyPressed(int keyval, int state) {
	GdkEventKey e;
	e.keyval = keyval;
	e.state  = state;
	return(main_key_press(0, &e, this));
}

/* callbacks */

gint
mainwindow_delete (GtkWidget * widget, GdkEvent * event, gpointer data) {
	global.Quitting=1;
	return FALSE;
}

void
mainwindow_destroy (GtkWidget * widget, gpointer data) {
	global.Quitting=1;
	global.WrappedMainQuit();
}

int
main_key_press (GtkWidget * wid, GdkEventKey * evt, gpointer data) {
	MainWindow *ptr;
	int cpage;
	ptr=(MainWindow *)data;

	if (evt->state&GDK_CONTROL_MASK) {
		switch(evt->keyval) {
		case GDK_Left:
			ptr->gameWalk(1);
			break;
		case GDK_Right:
			ptr->gameWalk(2);
			break;
		case GDK_F:
		case GDK_f:
			windows_find(0,0);
			break;
		case GDK_G:
		case GDK_g:
			windows_findp(0,0);
			break;
		default:
			return 0;
		}
		return 1;
	}

	switch(evt->keyval) {
	case GDK_Escape:
		if (ptr->ims)
			ptr->ims->flip();
		break;
	case GDK_F3: // previous pane
		global.ebook->goToPrevious();
		break;
	case GDK_F4: // next pane
		global.ebook->goToNext();
		break;
	case GDK_F5: // pop main board
		global.ebook->goToPageId(-1);
		break;
	case GDK_F6: // pop console
		global.ebook->goToPageId(-2);
		break;
	case GDK_F7: // pop seek graph
		global.ebook->goToPageId(-3);
		break;
	case GDK_F8: // toggle shortcut bar
		global.ShowQuickbar = ! (global.ShowQuickbar);
		global.qbcontainer->update();
		global.writeRC();
		break;
	case GDK_Page_Up:
		if (wid != 0) {
			cpage = global.ebook->getCurrentPageId();
			if (cpage==-2)
				ptr->inconsole->pageUp();
			if (cpage >= -500 && cpage <= -200)
				global.channelPageUp(-200-cpage);
			ptr->xconsole->pageUp();
		}
		break;
	case GDK_Page_Down:
		if (wid != 0) {
			cpage = global.ebook->getCurrentPageId();
			if (cpage==-2)
				ptr->inconsole->pageDown();
			if (cpage >= -500 && cpage <= -200)
				global.channelPageDown(-200-cpage);
			ptr->xconsole->pageDown();
		}
		break;
	default:
		return 0;
	}
	return 1;
}

int
input_key_press (GtkWidget * wid, GdkEventKey * evt, gpointer data) {
	MainWindow *ptr;
	ptr=(MainWindow *)data;

	switch(evt->keyval) {
	case GDK_Up:
		gtk_signal_emit_stop_by_name(GTK_OBJECT(wid), "key_press_event");
		ptr->historyUp();
		return 1;
	case GDK_Down:
		gtk_signal_emit_stop_by_name(GTK_OBJECT(wid), "key_press_event");
		ptr->historyDown();
		return 1;
	case GDK_KP_Enter:
		gtk_signal_emit_stop_by_name(GTK_OBJECT(wid), "key_press_event");
	case GDK_Return:
		ptr->injectInput();
		return 1;
	}
	return 0;
}

void
mainwindow_themeitem (GtkMenuItem *menuitem, gpointer data) {
	ThemeEntry *te;
	te=(ThemeEntry *)data;
	gmiset(GTK_CHECK_MENU_ITEM(mainw->vector_checkbox),0);
	global.UseVectorPieces=0;
	global.setPieceSet(te->Filename,true,true);
	global.writeRC();
}

void
mainwindow_themeitem2 (GtkMenuItem *menuitem, gpointer data) {
	ThemeEntry *te;
	te=(ThemeEntry *)data;
	gmiset(GTK_CHECK_MENU_ITEM(mainw->vector_checkbox),0);
	global.UseVectorPieces=0;
	global.setPieceSet(te->Filename,true,false);
	global.writeRC();
}

void
mainwindow_themeitem3 (GtkMenuItem *menuitem, gpointer data) {
	ThemeEntry *te;
	te=(ThemeEntry *)data;
	gmiset(GTK_CHECK_MENU_ITEM(mainw->vector_checkbox),0);
	global.UseVectorPieces=0;
	global.setPieceSet(te->Filename,false,true);
	global.writeRC();
}

void
peer_disconnect(gpointer data) {
	mainw->disconnect();
}

void
peer_scratch_empty(gpointer data) {
	mainw->newScratchBoard(true);
}

void
peer_scratch_initial(gpointer data) {
	mainw->newScratchBoard(false);
}

void
peer_connect_fics(gpointer data) {
	mainw->openServer("freechess.org",5000,new FicsProtocol(),"timeseal");
	if (global.FicsAutoLogin)
		if (global.network)
			if (global.network->isConnected())
				new ScriptInstance("autofics.pl");
}

void peer_connect_xboard(gpointer data) {
	mainw->openXBoardEngine();
}

void peer_connect_gnuchess4(gpointer data) {
	mainw->openGnuChess4();
}
void peer_connect_crafty(gpointer data) {
	mainw->openCrafty();
}

void peer_connect_sjeng(gpointer data) {
	mainw->openSjeng();
}

void
peer_connect_ask(gpointer data) {
	(new ConnectDialog())->show();
}

void
peer_connect_p2p(gpointer data) {
	(new P2PDialog())->show();
}

void
windows_games(GtkWidget *w, gpointer data)
{
	mainw->openGameList();
}

void
windows_sough(GtkWidget *w, gpointer data)
{
	mainw->openAdList();
}

void
windows_stock(GtkWidget *w, gpointer data)
{
	mainw->openStockList();
}

void
windows_detached(GtkWidget *w, gpointer data)
{
	mainw->openDetachedConsole();
}

void
windows_script(GtkWidget *w, gpointer data)
{
	(new ScriptList())->show();
}

void
windows_savedesk(GtkWidget *w, gpointer data) {
	mainw->saveDesk();
}

void
windows_savebuffer(GtkWidget *w, gpointer data) {
	mainw->saveBuffer();
}

void
windows_find(GtkWidget *w, gpointer data) {
	mainw->ims->setSearchMode(true);
}

void
windows_findp(GtkWidget *w, gpointer data) {
	mainw->inconsole->findTextNext();
}

void
help_about(gpointer data) {
	(new Help::AboutDialog())->show();
}

void
help_keys(gpointer data) {
	(new Help::KeysDialog())->show();
}

void
help_debug(gpointer data) {
	(new Help::DebugDialog())->show();
}

void
help_starting(gpointer data) {
	(new Help::GettingStarted())->show();
}

void MainWindow::readAvailable(int handle) {
	NetConnection *net;
	char line[2048];
	int gotinput, loopc;

	global.debug("MainWindow","readAvailable");
	net = global.network;

	if ((net==NULL)||(!net->isConnected())||(global.protocol==NULL)) {
		cleanUpConnection();
		return;
	}

	if (handle == net->getReadHandle()) { /* got input from main connection */

		loopc = 0;
		do {
			gotinput=0;
			if (net->readLine(line,2048)==0) {
				gotinput=1;
				global.protocol->receiveString(line);
				global.agentBroadcast(line);
			} else {
				if (global.protocol->hasAuthenticationPrompts())
					if ((net->bufferMatch("login:"))||
						(net->bufferMatch("password:")))
						if (net->readPartial(line,2048)==0) {
							global.protocol->receiveString(line);
							global.agentBroadcast(line);
							gotinput=1;
						}
			}
			++loopc;
			if (loopc%10 == 9)
				gtk_main_iteration();

			if ( (global.network==0) || (global.protocol==0) )
				break;

		} while(gotinput);

	} else { /* got input from an agent (script) */

		if (io_tag < 0)
			io_tag = gtk_timeout_add(200, mainwindow_read_agents, (gpointer) this);
	}

}

gboolean mainwindow_read_agents(gpointer data) {
	MainWindow *me = (MainWindow *) data;
	int loopc;
	char line[2048];

	if (global.protocol==NULL)
		return FALSE;

	loopc = 0;
	while(global.receiveAgentLine(line,2048)) {
		global.protocol->sendUserInput(line);
		++loopc;
		if (loopc > 4)
			return TRUE;
	}
	me->io_tag = -1;
	return FALSE;
}

void MainWindow::writeAvailable(int handle) {
	// not used, write is buffered, the OS takes care for us, thanks.
}

void
mainwindow_icsout_changed(GtkEditable *gtke, gpointer data) {
	MainWindow *me;

	me=(MainWindow *)data;
	me->inconsole->contentUpdated();
}

void
navbar_back_all(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(0);
}

void
navbar_back_1(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(1);
}

void
navbar_forward_1(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(2);
}

void
navbar_forward_all(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(3);
}

void
navbar_movelist(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(4);
}

void
navbar_trash(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(5);
}

void
navbar_toscratch(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(7);
}

void
navbar_flip(GtkWidget *w,gpointer data) {
	MainWindow *me;
	me=(MainWindow *)data;
	me->gameWalk(6);
}

void
sett_prefs(gpointer data) {
	(new PreferencesDialog())->show();
}

void
sett_hilite(GtkWidget *w,gpointer data) {
	global.HilightLastMove=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
	global.repaintAllBoards();
}

void
sett_animate(GtkWidget *w,gpointer data) {
	global.AnimateMoves=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
}

void
sett_premove(GtkWidget *w,gpointer data) {
	global.Premove=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
	if (global.protocol && global.network)
		if (global.network->isConnected())
			global.protocol->updateVar(PV_premove);
}

void
sett_beepopp(GtkWidget *w,gpointer data) {
	global.BeepWhenOppMoves=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
}

void
sett_osound(GtkWidget *w,gpointer data) {
	global.EnableSounds=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
}

void
sett_vector(GtkWidget *w,gpointer data) {
	global.UseVectorPieces=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
	global.repaintAllBoards();
}

void
sett_legal(GtkWidget *w,gpointer data) {
	global.CheckLegality=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
}

void
sett_popup(GtkWidget *w,gpointer data) {
	global.PopupSecondaryGames=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
}

void
sett_smarttrash(GtkWidget *w,gpointer data) {
	global.SmartDiscard=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
}

void
sett_coord(GtkWidget *w,gpointer data) {
	global.ShowCoordinates=GTK_CHECK_MENU_ITEM(w)->active;
	global.writeRC();
	global.repaintAllBoards();
}

void
game_resign(GtkWidget *w,gpointer data) {
	if (global.protocol) global.protocol->resign();
}

void
game_draw(GtkWidget *w,gpointer data) {
	if (global.protocol) global.protocol->draw();
}

void
game_adjourn(GtkWidget *w,gpointer data) {
	if (global.protocol) global.protocol->adjourn();
}

void
game_abort(GtkWidget *w,gpointer data) {
	if (global.protocol) global.protocol->abort();
}

void game_retract(GtkWidget *w,gpointer data) {
	if (global.protocol) global.protocol->retractMove();
}

// the next 2 functions keep focus in the input box
// gcp was particularly annoyed by the focus not being
// kept there...

gboolean
forced_focus(gpointer data)
{
	gint a,b;
	gtk_editable_get_selection_bounds(GTK_EDITABLE(data),&a,&b); //FIXME
	gtk_widget_grab_focus(GTK_WIDGET(data));
	gtk_editable_select_region(GTK_EDITABLE(data),a,b);
	return FALSE;
}

gboolean
mainwindow_input_focus_out(GtkWidget *widget,GdkEventFocus *event,gpointer user_data)
{
	gtk_timeout_add(100,forced_focus,(gpointer)widget);
	return FALSE;
}

void
mainwindow_connect_bookmark(GtkWidget *w, gpointer data) {
	HostBookmark *bm;
	char p[32];
	bm=(HostBookmark *)data;

	// gcc 2.95 and up are just annoying with this const / non-const thing
	strcpy(p,"timeseal");
	global.chandler->openServer(bm->host,bm->port,new FicsProtocol(),
								strcmp(bm->protocol,"FICS") ? 0 : p);
}

void
mainwindow_connect_bookmark2(GtkWidget *w, gpointer data) {
	EngineBookmark *bm;
	bm=(EngineBookmark *)data;

	mainw->openEngineBookmark(bm);
}

void
mainwindow_edit_engbm(GtkWidget *w, gpointer data) {
	MainWindow *me;
	me=(MainWindow *) data;
	(new EditEngineBookmarksDialog( (BookmarkListener *) me ))->show();
}

// ThemeEntry

bool ThemeEntry::isDupe(ThemeEntry *te) {
	if (!te) return false;
	if (!Filename.compare(te->Filename)) return true;
	return false;
}

bool ThemeEntry::isNameDupe(ThemeEntry *te) {
	if (!te) return false;
	if (!Text.compare(te->Text)) return true;
	return false;
}

// --- input mode selector

InputModeSelector::InputModeSelector() {
	GtkWidget *h;
	int i;

	ChatMode=false;
	SearchMode=false;
	prefix.erase();

	widget = gtk_button_new();

	h=gtk_hbox_new(FALSE,0);
	l[0]=gtk_label_new(_("[cmd]"));
	l[1]=gtk_label_new("");

	gtk_container_add(GTK_CONTAINER(widget),h);
	for(i=0;i<2;i++) {
		gtk_box_pack_start(GTK_BOX(h),l[i],FALSE,TRUE,3*i);
		gshow(l[i]);
	}

	gshow(h);

	gtk_signal_connect(GTK_OBJECT(widget), "clicked",
					   GTK_SIGNAL_FUNC(ims_switch), (gpointer) this);
}

bool InputModeSelector::getChatMode() {
	return ChatMode;
}

bool InputModeSelector::getSearchMode() {
	return SearchMode;

}

void InputModeSelector::setSearchMode(bool m) {
	SearchMode = m;

	if (!SearchMode) {
		setChatMode(ChatMode);
		return;
	}

	gtk_label_set_text(GTK_LABEL(l[0]), _("[find]"));
	gtk_label_set_text(GTK_LABEL(l[1]), "");
	setColor(l[0],0,0x80,0);
	setColor(l[1],0,0,0);
}

void InputModeSelector::setChatMode(bool m) {
	string x;
	ChatMode=m;

	x=m?_("[chat]"):
		_("[cmd]");
	gtk_label_set_text(GTK_LABEL(l[0]), x.c_str());

	if (m) {
		x=prefix;
		setColor(l[0],0xff,0,0);
		setColor(l[1],0,0,0xff);
	} else {
		x.erase();
		setColor(l[0],0,0,0);
	}



	gtk_label_set_text(GTK_LABEL(l[1]), x.c_str());
	gtk_widget_queue_resize(widget);
}

string & InputModeSelector::getPrefix() {
	return prefix;
}

void InputModeSelector::setPrefix(char *s) {
	prefix=s;
	if (ChatMode) setChatMode(true);
}

void InputModeSelector::setPrefix(string &s) {
	prefix=s;
	if (ChatMode) setChatMode(true);
}

void InputModeSelector::setColor(GtkWidget *w, int R,int G, int B) {
	GdkColor nc;
	GtkStyle *style;
	int i;

	nc.red   = R << 8;
	nc.green = G << 8;
	nc.blue  = B << 8;
	style=gtk_style_copy( gtk_widget_get_style(w) );

	for(i=0;i<5;i++)
		style->fg[i]=nc;

	gtk_widget_set_style( w, style );
	gtk_widget_queue_draw( w );
}

void InputModeSelector::flip() {
	setChatMode(!ChatMode);
}

void ims_switch(GtkWidget *w, gpointer data) {
	InputModeSelector *me;
	me=(InputModeSelector *) data;
	me->flip();
}

string * PrefixCache::get(int id) {
	int i;
	// search back to front, since we are likely to get the latest
	// to be added
	for(i=ids.size()-1;i>=0;i--)
		if (ids[i] == id) return( text[i] );
	return 0;
}

void PrefixCache::set(int id,string &val) {
	int i;
	for(i=ids.size()-1;i>=0;i--)
		if (ids[i] == id) {
			if (text[i]) delete(text[i]);
			text[i]=new string(val);
			return;
		}
	ids.push_back(id);
	text.push_back( new string(val) );
}

void PrefixCache::set(int id,char *val) {
	set( id, * (new string(val)) );
}

void PrefixCache::setIfNotSet(int id, string &val) {
	int i;
	for(i=ids.size()-1;i>=0;i--)
		if (ids[i] == id)
			return;
	ids.push_back(id);
	text.push_back( new string(val) );
}

#ifdef HAVE_LINUX_JOYSTICK_H
void mainwindow_joystick(gpointer data,gint source,GdkInputCondition cond) {
	struct js_event jse;
	MainWindow *me = (MainWindow *) data;
	if (read(global.JoystickFD,&jse,sizeof(struct js_event)) == sizeof(struct js_event)) {
		switch(jse.type) {
		case JS_EVENT_BUTTON: me->joystickEvent(JOY_BUTTON, (int) (jse.number), (int) (jse.value)); break;
		case JS_EVENT_AXIS:   me->joystickEvent(JOY_AXIS, (int) (jse.number), (int) (jse.value)); break;
		}
	}
}
#endif
