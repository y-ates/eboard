/* $Id: dlg_gamelist.cc,v 1.37 2007/01/20 15:58:42 bergo Exp $ */

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
#include "eboard.h"
#include "global.h"
#include "chess.h"
#include "dlg_gamelist.h"
#include "tstring.h"

#include "icon-games.xpm"
#include "icon-ads.xpm"
#include "icon-local.xpm"

#include "goc_open.xpm"
#include "goc_save.xpm"
#include "goc_refresh.xpm"
#include "goc_discardall.xpm"
#include "goc_display.xpm"
#include "goc_discard.xpm"
#include "goc_edit.xpm"

#include "treeics.xpm"
#include "treepgn.xpm"
#include "treeeng.xpm"
#include "treeoth.xpm"
#include "treepgnf.xpm"
#include "treegam.xpm"

GameListDialog::GameListDialog(GameListListener *someone) {
  GtkWidget *sw,*v,*bh;
  GtkStyle *style;
  GdkFont *fixed;
  int i;

  owner=someone;
  SelectedRow=-1;
  canclose=1;

  widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(widget),600,400);
  gtk_window_set_title(GTK_WINDOW(widget),_("Game List"));
  gtk_window_set_position(GTK_WINDOW(widget),GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(widget),4);
  gtk_widget_realize(widget);

  v=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),v);

  sw=gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  style=gtk_style_copy( gtk_widget_get_default_style() );

  /* FIXME
  fixed=gdk_font_load("fixed");
  if (fixed)
    style->font=fixed;
  else
    cerr << _("<GameListDialog::GameListDialog> Warning: couldn't load fixed font, this dialog's content will look awful.\n\n");
  */

  clist=gtk_clist_new(2);
  gtk_clist_set_shadow_type(GTK_CLIST(clist),GTK_SHADOW_IN);
  gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
  gtk_clist_set_column_title(GTK_CLIST(clist),0,"#");
  gtk_clist_set_column_title(GTK_CLIST(clist),1,_("Game Description"));
  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_column_titles_show(GTK_CLIST(clist));

  gtk_box_pack_start(GTK_BOX(v),sw,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_widget_set_style(clist,style);
  gtk_container_add(GTK_CONTAINER(sw),clist);

  for(i=0;i<2;i++)
    gtk_clist_set_column_min_width(GTK_CLIST(clist),i,32);

  bh=gtk_hbox_new(TRUE,0);
  gtk_box_pack_start(GTK_BOX(v),bh,FALSE,FALSE,0);

  b[0]=gtk_button_new_with_label(_("Refresh List"));
  b[1]=gtk_button_new_with_label(_("Observe"));

  for(i=0;i<2;i++) {
    gtk_box_pack_start(GTK_BOX(bh),b[i],FALSE,TRUE,0);
    gshow(b[i]);
  }

  gtk_widget_set_sensitive(b[1],FALSE);

  setIcon(icon_games_xpm,_("Games"));

  Gtk::show(bh,clist,sw,v,NULL);

  gtk_signal_connect(GTK_OBJECT(widget),"delete_event",
		     GTK_SIGNAL_FUNC(gamelist_delete),(gpointer)(&canclose));
  gtk_signal_connect(GTK_OBJECT(widget),"destroy",
		     GTK_SIGNAL_FUNC(gamelist_destroy),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[0]),"clicked",
		     GTK_SIGNAL_FUNC(gamelist_refresh),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[1]),"clicked",
		     GTK_SIGNAL_FUNC(gamelist_observe),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(clist),"select_row",
		     GTK_SIGNAL_FUNC(gamelist_select),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(clist),"unselect_row",
		     GTK_SIGNAL_FUNC(gamelist_unselect),(gpointer)this);
  refresh();
}

void GameListDialog::appendGame(int gamenum, char *desc) {
  char z[10];
  char *zp[2];
  snprintf(z,10,"%d",gamenum);
  zp[0]=z;
  zp[1]=desc;
  gtk_clist_append(GTK_CLIST(clist),zp);
}

void GameListDialog::endOfList() {
  gtk_widget_set_sensitive(b[0],TRUE);
  gtk_window_set_title(GTK_WINDOW(widget),_("Game List"));
  gtk_clist_columns_autosize(GTK_CLIST(clist));
  canclose=1;
}

void gamelist_observe (GtkWidget * w, gpointer data) {
  GameListDialog *me;
  char *z;
  me=(GameListDialog *)data;
  int gn;
  gtk_clist_get_text(GTK_CLIST(me->clist),me->SelectedRow,0,&z);
  gn=atoi(z);
  if (global.getGame(gn))
    return;
  if (global.protocol)
    global.protocol->observe(gn);  
}

void GameListDialog::refresh() {
  if (global.protocol) {
    canclose=0;
    global.protocol->queryGameList(this);
    SelectedRow=-1;
    gtk_clist_clear(GTK_CLIST(clist));
    gtk_widget_set_sensitive(b[0],FALSE);
    gtk_widget_set_sensitive(b[1],FALSE);
    gtk_window_set_title(GTK_WINDOW(widget),_("Game List (refreshing...)"));
  }
}

void gamelist_refresh (GtkWidget * w, gpointer data) {
  GameListDialog *me;
  me=(GameListDialog *)data;
  me->refresh();
}

gint
gamelist_delete  (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  int flag;
  flag=*((int *)data);
  return(flag?FALSE:TRUE);
}

void
gamelist_destroy (GtkWidget * widget, gpointer data)
{
  GameListDialog *me;
  me=(GameListDialog *)data;
  if (me->owner)
    me->owner->gameListClosed();
}

void
gamelist_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
		  gpointer data)
{
  GameListDialog *me;
  me=(GameListDialog *)data;
  me->SelectedRow=row;
  gtk_widget_set_sensitive(me->b[1],TRUE);
}

void
gamelist_unselect  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
		    gpointer data)
{
  GameListDialog *me;
  me=(GameListDialog *)data;
  me->SelectedRow=-1;
  gtk_widget_set_sensitive(me->b[1],FALSE);
}

// ------------- client-side game list

StockListDialog::StockListDialog(StockListListener *someone) {
  GtkWidget *v,*sw,*h,*left;
  GdkFont *fixed;
  GtkStyle *style;
  int i;

  owner=someone;
  SelectedRow=-1;
  canclose=1;

  widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(widget),650,400);
  gtk_window_set_title(GTK_WINDOW(widget),_("Local Game List"));
  gtk_window_set_position(GTK_WINDOW(widget),GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(widget),4);
  gtk_widget_realize(widget);

  style=gtk_widget_get_style(widget);
  icons[0] = gdk_pixmap_create_from_xpm_d (widget->window, &masks[0],
					   &style->bg[GTK_STATE_NORMAL],
					   (gchar **) treeics_xpm);

  icons[1] = gdk_pixmap_create_from_xpm_d (widget->window, &masks[1],
					   &style->bg[GTK_STATE_NORMAL],
					   (gchar **) treepgn_xpm);

  icons[2] = gdk_pixmap_create_from_xpm_d (widget->window, &masks[2],
					   &style->bg[GTK_STATE_NORMAL],
					   (gchar **) treeeng_xpm);

  icons[3] = gdk_pixmap_create_from_xpm_d (widget->window, &masks[3],
					   &style->bg[GTK_STATE_NORMAL],
					   (gchar **) treeoth_xpm);

  icons[4] = gdk_pixmap_create_from_xpm_d (widget->window, &masks[4],
					   &style->bg[GTK_STATE_NORMAL],
					   (gchar **) treepgnf_xpm);

  icons[5] = gdk_pixmap_create_from_xpm_d (widget->window, &masks[5],
					   &style->bg[GTK_STATE_NORMAL],
					   (gchar **) treegam_xpm);


  h=gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),h);

  left=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(h),left,FALSE,TRUE,0);

  b[3]=PixButton(goc_open_xpm,_("Load PGN..."));
  gtk_box_pack_start(GTK_BOX(left),b[3],FALSE,TRUE,0);

  b[0]=PixButton(goc_refresh_xpm,_("Refresh List"));
  gtk_box_pack_start(GTK_BOX(left),b[0],FALSE,TRUE,0);

  b[6]=PixButton(goc_discardall_xpm,_("Discard All"));
  gtk_box_pack_start(GTK_BOX(left),b[6],FALSE,TRUE,0);

  b[1]=PixButton(goc_display_xpm,_("Display Game"));
  gtk_box_pack_start(GTK_BOX(left),b[1],FALSE,TRUE,0);

  b[4]=PixButton(goc_save_xpm,_("Save Game..."));
  gtk_box_pack_start(GTK_BOX(left),b[4],FALSE,TRUE,0);

  b[2]=PixButton(goc_discard_xpm,_("Discard Game"));
  gtk_box_pack_start(GTK_BOX(left),b[2],FALSE,TRUE,0);

  b[5]=PixButton(goc_edit_xpm,_("Edit Game Info"));
  gtk_box_pack_start(GTK_BOX(left),b[5],FALSE,TRUE,0);

  // -------

  v=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(h),v,TRUE,TRUE,0);

  sw=gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  style=gtk_style_copy( gtk_widget_get_default_style() );
  /* FIXME
  fixed=gdk_font_load("fixed");
  if (fixed)
    style->font=fixed;
  else
    cerr << _("<StockListDialog::StockListDialog> Warning: couldn't load fixed font, this dialog's content will look awful.\n\n");
  */

  // number(tree) displayed white black variant result

  clist=gtk_ctree_new(7,0);
  gtk_clist_set_shadow_type(GTK_CLIST(clist),GTK_SHADOW_IN);
  gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
  gtk_clist_set_column_title(GTK_CLIST(clist),0,_("Game #"));
  gtk_clist_set_column_title(GTK_CLIST(clist),1,_("Displayed"));
  gtk_clist_set_column_title(GTK_CLIST(clist),2,_("White"));
  gtk_clist_set_column_title(GTK_CLIST(clist),3,_("Black"));
  gtk_clist_set_column_title(GTK_CLIST(clist),4,_("Variant"));
  gtk_clist_set_column_title(GTK_CLIST(clist),5,_("Result"));

  gtk_clist_set_column_title(GTK_CLIST(clist),6,"ctree is buggy it seems");
  gtk_clist_set_column_visibility(GTK_CLIST(clist),6,FALSE);

  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_column_titles_show(GTK_CLIST(clist));

  gtk_box_pack_start(GTK_BOX(v),sw,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_widget_set_style(clist,style);
  gtk_container_add(GTK_CONTAINER(sw),clist);

  for(i=0;i<6;i++)
    gtk_clist_set_column_min_width(GTK_CLIST(clist),i,64);
  gtk_clist_columns_autosize(GTK_CLIST(clist));

  // ----------------------------------

  for(i=0;i<7;i++)
    gshow(b[i]);

  gtk_widget_set_sensitive(b[0],TRUE);
  gtk_widget_set_sensitive(b[1],FALSE);
  gtk_widget_set_sensitive(b[2],FALSE);
  gtk_widget_set_sensitive(b[3],TRUE);
  gtk_widget_set_sensitive(b[4],FALSE);
  gtk_widget_set_sensitive(b[5],FALSE);
  gtk_widget_set_sensitive(b[6],TRUE);

  setIcon(icon_local_xpm,_("Local"));

  Gtk::show(clist,sw,v,left,h,NULL);

  gtk_signal_connect(GTK_OBJECT(widget),"delete_event",
		     GTK_SIGNAL_FUNC(gamelist_delete),(gpointer)(&canclose));
  gtk_signal_connect(GTK_OBJECT(widget),"destroy",
		     GTK_SIGNAL_FUNC(stocklist_destroy),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[0]),"clicked",
		     GTK_SIGNAL_FUNC(stocklist_refresh),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[1]),"clicked",
		     GTK_SIGNAL_FUNC(stocklist_open),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[2]),"clicked",
		     GTK_SIGNAL_FUNC(stocklist_dump),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[3]),"clicked",
		     GTK_SIGNAL_FUNC(stocklist_loadpgn),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[6]),"clicked",
		     GTK_SIGNAL_FUNC(stocklist_dumpall),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[5]),"clicked",
		     GTK_SIGNAL_FUNC(stocklist_editpgn),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[4]),"clicked",
		     GTK_SIGNAL_FUNC(stocklist_savepgn),(gpointer)this);

  gtk_signal_connect(GTK_OBJECT(clist),"tree_select_row",
		     GTK_SIGNAL_FUNC(stocklist_select),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(clist),"tree_unselect_row",
		     GTK_SIGNAL_FUNC(stocklist_unselect),(gpointer)this);

  refresh();
}

void StockListDialog::refresh() {
  list<ChessGame *>::iterator gli;
  char x[16],y[8],z[64],a[64],b[32],c[16];
  char *zp[7];
  unsigned int i;
  int j;

  char ka[256], kb[256];
  string *kp;
  int kids[4];

  vector<GtkCTreeNode *> filenodes;
  vector<string> filenames;
  vector<int>    count;

  GtkCTreeNode *myfile;
  tstring t;

  SelectedRow=-1;
  gtk_clist_freeze(GTK_CLIST(clist));
  gtk_clist_clear(GTK_CLIST(clist));

  // add toplevel nodes
  for(i=0;i<4;i++)
    toplevel[i]=gtk_ctree_insert_node(GTK_CTREE(clist),0,0,
				      0,2, icons[i],masks[i], icons[i],masks[i], 
				      FALSE, TRUE);

  kids[0]=kids[1]=kids[2]=kids[3]=0;

  zp[0]=x;
  zp[1]=y;
  zp[2]=z;
  zp[3]=a;
  zp[4]=b;
  zp[5]=c;
  zp[6]=x;

  for(gli=global.GameList.begin();gli!=global.GameList.end();gli++) {
    snprintf(x,16,"%d",(*gli)->GameNumber);

    g_strlcpy(y,(*gli)->getBoard()?_("Yes"):
	                           _("No"),8);

    strcpy(z,(*gli)->PlayerName[0]);
    strcpy(a,(*gli)->PlayerName[1]);

    strcpy(b,ChessGame::variantName((*gli)->Variant));

    if (!(*gli)->isOver())
      g_strlcpy(c,_("in progress"),16);
    else {
      GameResult gr=(*gli)->getResult();
      switch(gr) {
      case WHITE_WIN: strcpy(c,"1-0"); break;
      case BLACK_WIN: strcpy(c,"0-1"); break;
      case DRAW:      strcpy(c,"1/2-1/2"); break;
      case UNDEF:     strcpy(c,"*"); break;
      }
    }

    if ( (*gli)->source == GS_PGN_File ) {
      j=-1;
      for(i=0;i<filenames.size();i++) {
	if ( filenames[i] == (*gli)->source_data ) {
	  count[i]++;
	  j=i; break;
	}
      }

      if (j<0) {
	myfile=gtk_ctree_insert_node(GTK_CTREE(clist), toplevel[1], 0,
				     0, 2, icons[4],masks[4], 
				     icons[4],masks[4], FALSE, TRUE);
	gtk_ctree_node_set_text(GTK_CTREE(clist), myfile, 6, "gtk_is_buggy");
	filenames.push_back( (*gli)->source_data );
	filenodes.push_back( myfile );
	count.push_back(1);
	j = filenames.size() - 1;
      }

      gtk_ctree_insert_node(GTK_CTREE(clist), filenodes[j], 0,
			    zp, 2, icons[5],masks[5], icons[5],masks[5], TRUE, TRUE);
      kids[1]++;
    } else {
      switch( (*gli)->source ) {
      case GS_ICS: j=0; break;
      case GS_Engine: j=2; break;
      case GS_Other: j=3; break;
      default: j=3;
      }

      gtk_ctree_insert_node(GTK_CTREE(clist), toplevel[j], 0,
			    zp, 2, icons[5],masks[5], icons[5],masks[5], TRUE, TRUE);
      kids[j]++;
    }    

  }

  for(i=0;i<count.size();i++) {
    memset(ka,0,256);
    filenames[i].copy(ka,255);
    t.set( filenames[i] );

    do {
      kp=t.token("/\n\r");
      if (kp) { 
	memset(ka,0,256);
	kp->copy(ka,255);
      }
    } while(kp);

    snprintf(kb,256,"%s (%d %s)", ka, count[i], 
	     count[i]==1?_("game"):
                         _("games"));
      
    gtk_ctree_node_set_pixtext(GTK_CTREE(clist) , filenodes[i], 0, kb,
			       4, icons[4], masks[4]);
  }

  for(i=0;i<4;i++)
    gtk_ctree_node_set_text(GTK_CTREE(clist), toplevel[i], 6, "gtk-is-buggy");

  snprintf(kb,256,_("From ICS (%d %s)"), kids[0], 
	   kids[0]==1?_("game"):
	   _("games"));
  gtk_ctree_node_set_pixtext(GTK_CTREE(clist), toplevel[0], 0, kb, 
			     4, icons[0],masks[0]);

  snprintf(kb,256,_("From PGN Files (%d %s)"), 
	   kids[1], kids[1]==1?_("game"):
	   _("games"));
  gtk_ctree_node_set_pixtext(GTK_CTREE(clist), toplevel[1], 0, kb,
			     4, icons[1], masks[1]);

  snprintf(kb,256,_("From Engines (%d %s)"), 
	   kids[2], kids[2]==1?_("game"):
	   _("games"));
  gtk_ctree_node_set_pixtext(GTK_CTREE(clist), toplevel[2], 0, kb,
			     4, icons[2], masks[2]);

  snprintf(kb,256,_("From Elsewhere (%d %s)"), 
	   kids[3], kids[3]==1?_("game"):
	   _("games"));
  gtk_ctree_node_set_pixtext(GTK_CTREE(clist), toplevel[3], 0, kb,
			     4, icons[3], masks[3]);

  gtk_clist_columns_autosize(GTK_CLIST(clist));
  gtk_clist_thaw(GTK_CLIST(clist));
  calcEnable();
}

void StockListDialog::open() {
  Board *b;
  ChessGame *cg;
  char tab[32];

  cg=global.getGame(SelectedRow);
  if (!cg)
    return;

  if (!cg->loadMoves()) return;
  cg->protodata[1]=1;

  b=new Board();
  b->reset();
  cg->setBoard(b);
  b->setGame(cg);
  b->setCanMove(false);
  b->setSensitive(false);
  b->walkBackAll();  
  cg->acknowledgeInfo();
  
  snprintf(tab,32,_("Game #%d"),cg->GameNumber);
  global.ebook->addPage(b->widget,tab,cg->GameNumber);
  b->setNotebook(global.ebook,cg->GameNumber);
  b->pop();
  b->repaint();
  refresh();
}

void StockListDialog::trashAll() {
  ChessGame *cg;
  Board *b;
  list<ChessGame *>::iterator gli;
  int i;

  for(i=0;i<2;i++) {
    for(gli=global.GameList.begin();gli!=global.GameList.end();gli++) {
      cg=*gli;
      
      if (cg->getBoard()) {
	b=cg->getBoard();
	
	if (b==global.BoardList.front())
	  continue; // can't discard game shown in main board

	cg->closeMoveList();
	
	global.ebook->removePage(cg->GameNumber);
	global.removeBoard(b);
	delete(b);
	cg->setBoard(0);
      }
      
      delete(cg);
      global.GameList.erase(gli);
      gli=global.GameList.begin();
    }
  }

  refresh();
}

void StockListDialog::trash() {
  ChessGame *cg;
  Board *b;
  list<ChessGame *>::iterator gli;

  cg=global.getGame(SelectedRow);
  if (cg==NULL)
    return;

  if (cg->getBoard()!=NULL) {
    b=cg->getBoard();

    if (b==global.BoardList.front()) // can't remove game in main board
      return;

    global.ebook->removePage(cg->GameNumber);
    global.removeBoard(b);
    delete(b);
    cg->setBoard(NULL);
  }

  for(gli=global.GameList.begin();gli!=global.GameList.end();gli++)
    if ( (*gli) == cg ) {
      delete(*gli);
      global.GameList.erase(gli);
      break;
    }

  refresh();
}

void StockListDialog::calcEnable() {
  ChessGame *cg;

  if (SelectedRow<0) {
  oops:
    gtk_widget_set_sensitive(b[1],FALSE);
    gtk_widget_set_sensitive(b[2],FALSE);
    gtk_widget_set_sensitive(b[4],FALSE);
    gtk_widget_set_sensitive(b[5],FALSE);
    return;
  }

  cg=global.getGame(SelectedRow);
  if (!cg)
    goto oops;

  if (cg->getBoard()) {
    gtk_widget_set_sensitive(b[1],FALSE); // can't display
    gtk_widget_set_sensitive(b[2],cg->isOver()); // can discard if over
  } else {
    gtk_widget_set_sensitive(b[1],TRUE); // can display
    gtk_widget_set_sensitive(b[2],TRUE); // can discard
  }
  gtk_widget_set_sensitive(b[4],TRUE); // can save
  gtk_widget_set_sensitive(b[5],TRUE); // can edit pgn
}

void
stocklist_refresh (GtkWidget * w, gpointer data)
{
  StockListDialog *me;
  me=(StockListDialog *)data;
  me->refresh();
}

void
stocklist_open (GtkWidget * w, gpointer data)
{
  StockListDialog *me;
  me=(StockListDialog *)data;
  me->open();
}

void
stocklist_savepgn (GtkWidget * w, gpointer data)
{
  StockListDialog *me;
  FileDialog *fd;
  ChessGame *cg;

  me=(StockListDialog *)data;
  fd=new FileDialog(_("Save as PGN"));
  
  if (fd->run()) {
    cg=global.getGame(me->SelectedRow);
    if (cg)
      cg->savePGN(fd->FileName, true);
  }

  delete fd;
}

void
stocklist_loadpgn (GtkWidget * w, gpointer data)
{
  StockListDialog *me;
  FileDialog *fd;

  me=(StockListDialog *)data;
  fd=new FileDialog(_("Load PGN"));

  if (fd->run()) {
    ChessGame::LoadPGN(fd->FileName);
    me->refresh();
  }
  
  delete fd;
}

void
stocklist_editpgn (GtkWidget * w, gpointer data)
{
  StockListDialog *me;
  ChessGame *cg;
  me=(StockListDialog *)data;
  cg=global.getGame(me->SelectedRow);
  if (!cg)
    return;
  (new PGNEditInfoDialog(cg))->show();
}

void
stocklist_dump (GtkWidget * w, gpointer data)
{
  StockListDialog *me;
  me=(StockListDialog *)data;
  me->trash();
}

void
stocklist_dumpall (GtkWidget * w, gpointer data)
{
  StockListDialog *me;
  me=(StockListDialog *)data;
  me->trashAll();
}

void
stocklist_destroy (GtkWidget * widget, gpointer data)
{
  StockListDialog *me;
  me=(StockListDialog *)data;
  if (me->owner)
    me->owner->stockListClosed();
}

// the gtk documentation has a wrong declaration of this,
// only figured out after looking at the code of the Balsa
// mail client

void
stocklist_select  (GtkCTree *cl, GtkCTreeNode *node, gint column,
		   gpointer data)
{
  StockListDialog *me;
  char *p;

  me=(StockListDialog *)data;

  // getting column 0 either crashes or returns garbage, so
  // I duplicated it in an invisible column
  gtk_ctree_node_get_text(cl, 
			  node,
			  6, &p);
  if (p[0] != 'g') {
    me->SelectedRow=atoi(p);
    if (me->SelectedRow == 0) me->SelectedRow=-1;
    me->calcEnable();
  }
}

void
stocklist_unselect (GtkCTree *cl, GtkCTreeNode *node, gint column,
		    gpointer data)
{
  StockListDialog *me;
  int i;

  me=(StockListDialog *)data;
  me->SelectedRow=-1;
  for(i=1;i<3;i++)
    gtk_widget_set_sensitive(me->b[i],FALSE);
  gtk_widget_set_sensitive(me->b[4],FALSE);
  gtk_widget_set_sensitive(me->b[5],FALSE);
}

// ------------------------ ad list

AdListDialog::AdListDialog(AdListListener *someone) {
  GtkWidget *sw,*v,*bh;
  GtkStyle *style;
  GdkFont *fixed;
  int i;

  owner=someone;
  SelectedRow=-1;
  canclose=1;

  widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(widget),600,400);
  gtk_window_set_title(GTK_WINDOW(widget),_("Ad List"));
  gtk_window_set_position(GTK_WINDOW(widget),GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(widget),4);
  gtk_widget_realize(widget);

  v=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),v);

  sw=gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  style=gtk_style_copy( gtk_widget_get_default_style() );

  /* FIXME
  fixed=gdk_font_load("fixed");
  if (fixed)
    style->font=fixed;
  else
    cerr << _("<AdListDialog::AdListDialog> Warning: couldn't load fixed font, this dialog's content will look awful.\n\n");
  */

  clist=gtk_clist_new(2);
  gtk_clist_set_shadow_type(GTK_CLIST(clist),GTK_SHADOW_IN);
  gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
  gtk_clist_set_column_title(GTK_CLIST(clist),0,"#");
  gtk_clist_set_column_title(GTK_CLIST(clist),1,_("Ad Description"));
  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_column_titles_show(GTK_CLIST(clist));

  gtk_box_pack_start(GTK_BOX(v),sw,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_widget_set_style(clist,style);
  gtk_container_add(GTK_CONTAINER(sw),clist);

  for(i=0;i<2;i++)
    gtk_clist_set_column_min_width(GTK_CLIST(clist),i,32);

  bh=gtk_hbox_new(TRUE,0);
  gtk_box_pack_start(GTK_BOX(v),bh,FALSE,FALSE,0);

  b[0]=gtk_button_new_with_label(_("Refresh List"));
  b[1]=gtk_button_new_with_label(_("Play"));

  for(i=0;i<2;i++) {
    gtk_box_pack_start(GTK_BOX(bh),b[i],FALSE,TRUE,0);
    gshow(b[i]);
  }

  gtk_widget_set_sensitive(b[1],FALSE);

  setIcon(icon_ads_xpm,_("Ads"));

  Gtk::show(bh,clist,sw,v,NULL);

  gtk_signal_connect(GTK_OBJECT(widget),"delete_event",
		     GTK_SIGNAL_FUNC(gamelist_delete),(gpointer)(&canclose));
  gtk_signal_connect(GTK_OBJECT(widget),"destroy",
		     GTK_SIGNAL_FUNC(adlist_destroy),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[0]),"clicked",
		     GTK_SIGNAL_FUNC(adlist_refresh),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[1]),"clicked",
		     GTK_SIGNAL_FUNC(adlist_answer),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(clist),"select_row",
		     GTK_SIGNAL_FUNC(adlist_select),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(clist),"unselect_row",
		     GTK_SIGNAL_FUNC(adlist_unselect),(gpointer)this);
  refresh();
}

void AdListDialog::appendGame(int gamenum, char *desc) {
  char z[10];
  char *zp[2];
  snprintf(z,10,"%d",gamenum);
  zp[0]=z;
  zp[1]=desc;
  gtk_clist_append(GTK_CLIST(clist),zp);
}

void AdListDialog::endOfList() {
  gtk_widget_set_sensitive(b[0],TRUE);
  gtk_window_set_title(GTK_WINDOW(widget),_("Ad List"));
  gtk_clist_columns_autosize(GTK_CLIST(clist));
  canclose=1;
}

void AdListDialog::refresh() {
  if (global.protocol) {
    canclose=0;
    global.protocol->queryAdList(this);
    SelectedRow=-1;
    gtk_clist_clear(GTK_CLIST(clist));
    gtk_widget_set_sensitive(b[0],FALSE);
    gtk_widget_set_sensitive(b[1],FALSE);
    gtk_window_set_title(GTK_WINDOW(widget),_("Ad List (refreshing...)"));
  }
}

void
adlist_refresh (GtkWidget * w, gpointer data)
{
  AdListDialog *me;
  me=(AdListDialog *)data;
  me->refresh();
}

void
adlist_answer  (GtkWidget * w, gpointer data)
{
  AdListDialog *me;
  char *z;
  me=(AdListDialog *)data;
  int gn;
  gtk_clist_get_text(GTK_CLIST(me->clist),me->SelectedRow,0,&z);
  gn=atoi(z);
  if (global.protocol)
    global.protocol->answerAd(gn);
}

void
adlist_destroy (GtkWidget * widget, gpointer data)
{
  AdListDialog *me;
  me=(AdListDialog *)data;
  if (me->owner)
    me->owner->adListClosed();
}

void
adlist_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
		gpointer data)
{
  AdListDialog *me;
  me=(AdListDialog *)data;
  me->SelectedRow=row;
  gtk_widget_set_sensitive(me->b[1],TRUE);
}

void
adlist_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
		gpointer data)
{
  AdListDialog *me;
  me=(AdListDialog *)data;
  me->SelectedRow=-1;
  gtk_widget_set_sensitive(me->b[1],FALSE);
}
