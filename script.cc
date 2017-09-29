/* $Id: script.cc,v 1.16 2007/01/20 15:58:43 bergo Exp $ */

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
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "script.h"
#include "global.h"
#include "stl.h"
#include "eboard.h"

#include "spiral0.xpm"
#include "spiral1.xpm"
#include "spiral2.xpm"
#include "spiral3.xpm"

ScriptList::ScriptList() : ModalDialog(N_("Script List")) {
  GtkWidget *v,*sw,*bh;
  int i;

  SelectedRow=-1;
  
  gtk_window_set_default_size(GTK_WINDOW(widget),350,300);

  v=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),v);

  sw=gtk_scrolled_window_new(0,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  clist=gtk_clist_new(2);
  gtk_clist_set_shadow_type(GTK_CLIST(clist),GTK_SHADOW_IN);
  gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
  gtk_clist_set_column_title(GTK_CLIST(clist),0,_("Script"));
  gtk_clist_set_column_title(GTK_CLIST(clist),1,_("Description"));
  gtk_clist_column_titles_passive(GTK_CLIST(clist));
  gtk_clist_column_titles_show(GTK_CLIST(clist));

  gtk_box_pack_start(GTK_BOX(v),sw,TRUE,TRUE,0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sw),clist);

  for(i=0;i<2;i++)
    gtk_clist_set_column_min_width(GTK_CLIST(clist),i,96);

  bh=gtk_hbox_new(TRUE,0);
  gtk_box_pack_start(GTK_BOX(v),bh,FALSE,FALSE,0);

  b[0]=gtk_button_new_with_label(_("Refresh List"));
  b[1]=gtk_button_new_with_label(_("Run"));
  b[2]=gtk_button_new_with_label(_("Dismiss"));

  for(i=0;i<3;i++) {
    gtk_box_pack_start(GTK_BOX(bh),b[i],FALSE,TRUE,0);
    gshow(b[i]);
  }

  gtk_widget_set_sensitive(b[1],FALSE);

  Gtk::show(bh,clist,sw,v,NULL);

  gtk_signal_connect(GTK_OBJECT(b[0]),"clicked",
		     GTK_SIGNAL_FUNC(script_refresh),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(b[1]),"clicked",
		     GTK_SIGNAL_FUNC(script_run),(gpointer)this);

  setDismiss(GTK_OBJECT(b[2]),"clicked");

  gtk_signal_connect(GTK_OBJECT(clist),"select_row",
		     GTK_SIGNAL_FUNC(script_select),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(clist),"unselect_row",
		     GTK_SIGNAL_FUNC(script_unselect),(gpointer)this);

  refresh();
}

void ScriptList::refresh() {
  DIR *sd;
  char z[256],w[256],desc[256],tmp[512],*dp[2];
  struct dirent *de;
  struct stat statd;
  mode_t mode;

  gtk_clist_freeze(GTK_CLIST(clist));
  gtk_clist_clear(GTK_CLIST(clist));

  if (!global.env.Home.empty()) {
    snprintf(z,256,"%s/.eboard/scripts",global.env.Home.c_str());
    sd=opendir(z);
    if (sd) {
      while((de=readdir(sd))) {
	snprintf(tmp,512,"%s/%s",z,de->d_name);
	if (!stat(tmp,&statd)) {	  
	  mode=statd.st_mode;
	  if (S_ISREG(mode)&&(mode&(S_IRUSR|S_IXUSR)))
	    if (geteuid()==statd.st_uid) {
	      g_strlcpy(w,de->d_name,256);
	      // grab description
	      ifstream f(tmp);
	      if (!f) continue;
	      if (f.get()!='#') {
		f.close();
		desc[0]=0;
	      } else {
		f.getline(desc,255,'\n');
		do {
		  memset(desc,0,256);
		  if (!f.getline(desc,255,'\n'))
		    break;
		} while(desc[0]!='#');
		f.close();
		if (desc[0]=='#') {
		  strcpy(tmp,desc);
		  strcpy(desc,tmp+1);
		} else {
		  desc[0]=0;
		}
	      }
	      dp[0]=w;
	      dp[1]=desc;
	      gtk_clist_append(GTK_CLIST(clist),dp);
	    }
	}
      }
      closedir(sd);
    }
  }
  gtk_clist_thaw(GTK_CLIST(clist));
}

void script_refresh (GtkWidget * w, gpointer data) {
  ScriptList *me;
  me=(ScriptList *)data;
  me->refresh();
}

void script_run (GtkWidget * w, gpointer data) {
  ScriptList *me;
  char *z;
  me=(ScriptList *)data;
  gtk_clist_get_text(GTK_CLIST(me->clist),me->SelectedRow,0,&z);
  new ScriptInstance(z);
  me->release();
}

void script_select  (GtkCList *cl, gint row, gint column, GdkEventButton *eb,
		     gpointer data) {
  ScriptList *me;
  me=(ScriptList *)data;
  me->SelectedRow=row;
  gtk_widget_set_sensitive(me->b[1],TRUE);

}

void script_unselect(GtkCList *cl, gint row, gint column, GdkEventButton *eb,
		     gpointer data) {
 ScriptList *me;
  me=(ScriptList *)data;
  me->SelectedRow=-1;
  gtk_widget_set_sensitive(me->b[1],FALSE);
}

// -------------------------

bool ScriptInstance::pixmapsok=false;
GdkPixmap * ScriptInstance::spir[4];
GdkBitmap * ScriptInstance::smask[4];

void ScriptInstance::initPixmaps() {
  GtkStyle *style;
  if (pixmapsok) return;

  style=gtk_widget_get_style(global.killbox);

  spir[0] = gdk_pixmap_create_from_xpm_d (global.killbox->window, &smask[0],
					  &style->bg[GTK_STATE_NORMAL],
					  (gchar **) spiral0_xpm);
  spir[1] = gdk_pixmap_create_from_xpm_d (global.killbox->window, &smask[1],
					  &style->bg[GTK_STATE_NORMAL],
					  (gchar **) spiral1_xpm);
  spir[2] = gdk_pixmap_create_from_xpm_d (global.killbox->window, &smask[2],
					  &style->bg[GTK_STATE_NORMAL],
					  (gchar **) spiral2_xpm);
  spir[3] = gdk_pixmap_create_from_xpm_d (global.killbox->window, &smask[3],
					  &style->bg[GTK_STATE_NORMAL],
					  (gchar **) spiral3_xpm);
  pixmapsok=true;  
}

ScriptInstance::ScriptInstance(char *name) : WidgetProxy() {
  GtkWidget *h, *l, *b;
  char z[256],fp[512];

  if (global.env.Home.empty()) {
    delete this;
    return;
  }

  snprintf(fp,512,"%s/.eboard/scripts/%s",global.env.Home.c_str(),name);

  ifstream f(fp);
  if (!f) {
    delete this;
    return;
  }
  f.close();

  child=new PipeConnection("/bin/sh","-c",fp,0,0);
  child->Quiet=1;
  if (child->open()) {
    delete child;
    delete this;
    return;
  }
  global.addAgent(child);

  if (!pixmapsok) initPixmaps();

  widget=gtk_frame_new(0);
  gtk_container_set_border_width(GTK_CONTAINER(widget),2);
  gtk_frame_set_shadow_type(GTK_FRAME(widget), GTK_SHADOW_ETCHED_IN);
  h=gtk_hbox_new(FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(h),2);

  runner=gtk_pixmap_new(spir[0],smask[0]);
  snprintf(z,256,_("running %s"),name);
  l=gtk_label_new(z);
  b=gtk_button_new_with_label(_("Kill"));

  gtk_container_add(GTK_CONTAINER(widget), h);
  gtk_box_pack_start(GTK_BOX(h), runner, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(h), l, FALSE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(h), b, FALSE, TRUE, 0);

  Gtk::show(b,l,runner,h,NULL);
  gtk_box_pack_start(GTK_BOX(global.killbox), widget, FALSE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(b),"clicked",
		     GTK_SIGNAL_FUNC(scripti_kill),(gpointer)this);

  frame=0;
  toid=gtk_timeout_add(500,scripti_check,(gpointer)this);
  anid=gtk_timeout_add(150,scripti_anim,(gpointer)this);
  show();
}

gboolean scripti_anim(gpointer data) {
  ScriptInstance *me;
  me=(ScriptInstance *)data;
  me->frame++;
  me->frame%=4;
  gtk_pixmap_set(GTK_PIXMAP(me->runner), me->spir[me->frame], me->smask[me->frame]);
  return TRUE;
}

void scripti_kill(GtkWidget *w,gpointer data) {
  ScriptInstance *me;
  me=(ScriptInstance *)data;
  gtk_timeout_remove(me->anid);
  gtk_timeout_remove(me->toid);
  global.removeAgent(me->child);
  me->child->close();
  gtk_container_remove(GTK_CONTAINER(global.killbox), me->widget);
  if (global.toplevelwidget)
    gtk_widget_queue_resize(global.toplevelwidget);
  //gtk_widget_destroy(me->widget);
  delete me->child;
  delete me;
}

gboolean scripti_check(gpointer data) {
  ScriptInstance *me;
  me=(ScriptInstance *)data;
  if (! me->child->isConnected()) {
    scripti_kill(0,data);
    return FALSE;
  }
  return TRUE;
}
