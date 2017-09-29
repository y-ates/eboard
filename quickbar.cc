/* $Id: quickbar.cc,v 1.16 2007/01/20 15:58:43 bergo Exp $ */

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
#include <string.h>
#include "quickbar.h"
#include "global.h"
#include "stl.h"
#include "tstring.h"
#include "script.h"
#include "eboard.h"

#include "qb1.xpm"
#include "qb2.xpm"
#include "qb3.xpm"
#include "qb4.xpm"
#include "qb5.xpm"
#include "qb6.xpm"
#include "qbr.xpm"
#include "qbl.xpm"
#include "hammer.xpm"

void qbsetup(GtkWidget *w, gpointer data) {
  (new QuickBarSetupDialog())->show();
}

void qbhide(GtkWidget *w, gpointer data) {
  global.ShowQuickbar=0;
  global.writeRC();  
  global.qbcontainer->update();
}

QuickBar::QuickBar(GtkWidget *parent) {
  GtkStyle *style;
  GdkPixmap *d0;
  GdkBitmap *m0;
  GtkWidget *config, *hide;

  widget = gtk_hbox_new(FALSE,0);

  style=gtk_widget_get_style(parent);
  d0 = gdk_pixmap_create_from_xpm_d (parent->window, &m0,
				    &style->bg[GTK_STATE_NORMAL],
				    (gchar **) hammer_xpm);
  config = CommandButton(d0,m0,_("Setup Buttons"));
  hide = gtk_button_new_with_label(_("Hide!"));

  gtk_box_pack_start(GTK_BOX(widget), config, FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(widget), hide, FALSE,FALSE,0);

  Gtk::show(config,hide,NULL);
  QuickBarIcons::loadIcons(parent);

  gtk_signal_connect(GTK_OBJECT(config), "clicked",
		     GTK_SIGNAL_FUNC(qbsetup), 0);

  gtk_signal_connect(GTK_OBJECT(hide), "clicked",
		     GTK_SIGNAL_FUNC(qbhide), 0);

  build();
}

void QuickBar::update() {
  clear();
  build();
}

void QuickBar::clear() {
  unsigned int i;
  for(i=0;i<buttons.size();i++)
    gtk_container_remove(GTK_CONTAINER(widget), buttons[i]);
  // I tried a gtk_widget_destroy here but it seems gtk_container_remove
  // already destroys it

  buttons.clear();
}

void qb_shortcut(GtkWidget *w, gpointer data) {
  int *v;
  char z[2048],y[2048],a[2048];
  tstring t;
  string *p;

  v=(int *) data;

  if (global.QuickbarButtons[*v] != 0) {
    g_strlcpy(z, global.QuickbarButtons[*v]->command.c_str(), 256);

    if (global.protocol == 0) return;
    if (global.network == 0) return;
    if (!global.network->isConnected()) return;

    if (strstr(z,"script.") == z) {
      strcpy(y,z+7);
      snprintf(a,2048,_("> [script run from shortcut] %s"),y);
      global.output->append(a, global.SelfInputColor);   
      new ScriptInstance(y);
      return;
    }

    t.set(z);
    while((p=t.token(";"))!=0) {
      g_strlcpy(y,p->c_str(),2048);
      global.protocol->sendUserInput(y);
      g_strlcpy(y,_("> [issued from shortcut] "),2048);
      g_strlcat(y,p->c_str(),2048);
      global.output->append(y, global.SelfInputColor);   
    }    
   
  }
}

void QuickBar::build() {
  unsigned int i;
  int j;
  GtkWidget *b;
  char caption[64];
  for(i=0;i<global.QuickbarButtons.size();i++) {
    j=global.QuickbarButtons[i]->icon;
    g_strlcpy(caption, global.QuickbarButtons[i]->caption.c_str(), 64);
    b=CommandButton(arrow[j],mask[j],caption);
    gtk_box_pack_start(GTK_BOX(widget), b, FALSE, FALSE, 0);
    buttons.push_back(b);
    gshow(b);
    gtk_signal_connect(GTK_OBJECT(b), "clicked",
		       GTK_SIGNAL_FUNC(qb_shortcut), (gpointer) (& numbers[i%10]));
  }
}

// ---------------------------------------------

void qbsetup_ok(GtkWidget *w, gpointer data) {
  QuickBarSetupDialog *me;
  int i;
  QButton *b;
  const char *x, *y;

  me=(QuickBarSetupDialog *) data;
  global.dropQuickbarButtons();
  for(i=0;i<10;i++) {
    x=	gtk_entry_get_text(GTK_ENTRY(me->bcap[i]));
    y=	gtk_entry_get_text(GTK_ENTRY(me->bcom[i]));    
    if (strlen(x) && strlen(y) ) {
      b=new QButton(x,y, me->iconValue[i]);
      global.QuickbarButtons.push_back(b);
    }
  }
  me->release();
  global.writeRC();
  global.quickbar->update();
}

void qbsetup_left(GtkWidget *w, gpointer data) {
  QuickBarSetupDialog *me;
  int i,j;

  me=(QuickBarSetupDialog *) data;
  i=me->whichButton(w);
  if (i<0) return;

  me->iconValue[i]--;
  if (me->iconValue[i] < 0) me->iconValue[i]=5;
  j=me->iconValue[i];

  gtk_pixmap_set(GTK_PIXMAP(me->bpix[i]), me->arrow[j], me->mask[j]);
  gtk_widget_queue_resize(me->bpix[i]);
}

void qbsetup_right(GtkWidget *w, gpointer data) {
  QuickBarSetupDialog *me;
  int i,j;

  me=(QuickBarSetupDialog *) data;
  i=me->whichButton(w);
  if (i<0) return;
  
  me->iconValue[i]++;
  if (me->iconValue[i] > 5) me->iconValue[i]=0;
  j=me->iconValue[i];

  gtk_pixmap_set(GTK_PIXMAP(me->bpix[i]), me->arrow[j], me->mask[j]);
  gtk_widget_queue_resize(me->bpix[i]);
}

QuickBarSetupDialog::QuickBarSetupDialog() : ModalDialog(N_("Shortcut Button Setup")) {
  GtkWidget *v, *t, *hs, *hb, *ok, *cancel, *l1, *l2, *l3, *mc;
  GtkWidget *bh[10];
  unsigned int i;

  QuickBarIcons::loadIcons(widget);

  v=gtk_vbox_new(FALSE,4);
  gtk_container_add(GTK_CONTAINER(widget),v);

  t=gtk_table_new(11,14,TRUE);

  l3=gtk_label_new(_("Button Icon"));
  l1=gtk_label_new(_("Button Caption"));
  l2=gtk_label_new(_("Command"));
  gtk_table_attach_defaults(GTK_TABLE(t), l3, 0, 2, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(t), l1, 2, 6, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(t), l2, 6, 14, 0, 1);

  for(i=0;i<10;i++) {
    bh[i]=gtk_hbox_new(FALSE,0);
    bl[i]=CommandButton(qbl, mqbl, 0);
    bpix[i]=gtk_pixmap_new(arrow[0],mask[0]);
    br[i]=CommandButton(qbr, mqbr, 0);

    gtk_box_pack_start(GTK_BOX(bh[i]),bl[i],FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(bh[i]),br[i],FALSE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(bh[i]),bpix[i],TRUE,TRUE,4);

    gtk_table_attach_defaults(GTK_TABLE(t), bh[i], 0, 2, i+1, i+2);

    bcap[i]=gtk_entry_new();
    bcom[i]=gtk_entry_new();

    gtk_table_attach_defaults(GTK_TABLE(t), bcap[i], 2, 6, i+1, i+2);
    gtk_table_attach_defaults(GTK_TABLE(t), bcom[i], 6, 14, i+1, i+2);

    Gtk::show(bcap[i],bcom[i],br[i],bl[i],bpix[i],bh[i],NULL);

    gtk_signal_connect(GTK_OBJECT(bl[i]), "clicked",
		       GTK_SIGNAL_FUNC(qbsetup_left), (gpointer) this);
    gtk_signal_connect(GTK_OBJECT(br[i]), "clicked",
		       GTK_SIGNAL_FUNC(qbsetup_right), (gpointer) this);
  }
  gtk_box_pack_start(GTK_BOX(v),t,TRUE,TRUE,2);  

  mc=gtk_label_new(_("To run multiple commands with one shortcut, separate "\
		   "the commands with ; (semicolon).\n"\
		   "To run a script from a shortcut, set command to script.ScriptName, e.g.: script.myscript.pl ."));
  gtk_box_pack_start(GTK_BOX(v),mc,TRUE,TRUE,2);

  hs=gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v),hs,TRUE,TRUE,2);
  
  hb=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hb), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hb), 5);
  gtk_box_pack_start(GTK_BOX(v), hb, FALSE, TRUE, 2); 

  ok=gtk_button_new_with_label(_("Ok"));
  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  cancel=gtk_button_new_with_label(_("Cancel"));
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(hb),ok,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(hb),cancel,TRUE,TRUE,0);
  gtk_widget_grab_default(ok);

  Gtk::show(ok,cancel,hb,hs,mc,l1,l2,l3,t,v,NULL);
  setDismiss(GTK_OBJECT(cancel),"clicked");

  for(i=0;i<10;i++)
    iconValue[i]=0;

  for(i=0;i<global.QuickbarButtons.size();i++) {
    gtk_entry_set_text(GTK_ENTRY(bcap[i]), global.QuickbarButtons[i]->caption.c_str());
    gtk_entry_set_text(GTK_ENTRY(bcom[i]), global.QuickbarButtons[i]->command.c_str());
    iconValue[i]=global.QuickbarButtons[i]->icon % 6;
    gtk_pixmap_set(GTK_PIXMAP(bpix[i]),arrow[iconValue[i]],mask[iconValue[i]]);
  }

  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(qbsetup_ok),(gpointer)this);
  focused_widget = bcap[0];
}

int QuickBarSetupDialog::whichButton(GtkWidget *w) {
  int i;
  for(i=0;i<10;i++)
    if ( (w == bl[i]) || (w == br[i]) ) return i;
  return -1; 
}

// -------------

QButton::QButton() {
  caption="none";
  command="none";
  icon=0;
}

QButton::QButton(const char *a, const char *b, int c) {
  caption=a;
  command=b;
  icon=c;
}

ostream &operator<<(ostream &s, QButton q) {
  s << q.icon << ':' << q.caption << ':' << q.command;
  return(s);
}

// ------------------------------

GdkPixmap * QuickBarIcons::arrow[6]={0,0,0,0,0,0};
GdkPixmap * QuickBarIcons::qbl=0;
GdkPixmap * QuickBarIcons::qbr=0;

GdkBitmap * QuickBarIcons::mask[6]={0,0,0,0,0,0};
GdkBitmap * QuickBarIcons::mqbl=0;
GdkBitmap * QuickBarIcons::mqbr=0;

bool QuickBarIcons::IconsLoaded = false;

int QuickBarIcons::numbers[10]={0,1,2,3,4,5,6,7,8,9};

void QuickBarIcons::loadIcons(GtkWidget *parent) {
  GtkStyle *style;
  if (!IconsLoaded) {
    style=gtk_widget_get_style(parent);
    arrow[0] = gdk_pixmap_create_from_xpm_d (parent->window, &mask[0],
					     &style->bg[GTK_STATE_NORMAL],
					     (gchar **) qb1_xpm);
    arrow[1] = gdk_pixmap_create_from_xpm_d (parent->window, &mask[1],
					     &style->bg[GTK_STATE_NORMAL],
					     (gchar **) qb2_xpm);
    arrow[2] = gdk_pixmap_create_from_xpm_d (parent->window, &mask[2],
					     &style->bg[GTK_STATE_NORMAL],
					     (gchar **) qb3_xpm);
    arrow[3] = gdk_pixmap_create_from_xpm_d (parent->window, &mask[3],
					     &style->bg[GTK_STATE_NORMAL],
					     (gchar **) qb4_xpm);
    arrow[4] = gdk_pixmap_create_from_xpm_d (parent->window, &mask[4],
					     &style->bg[GTK_STATE_NORMAL],
					     (gchar **) qb5_xpm);
    arrow[5] = gdk_pixmap_create_from_xpm_d (parent->window, &mask[5],
					     &style->bg[GTK_STATE_NORMAL],
					     (gchar **) qb6_xpm);
    
    qbl = gdk_pixmap_create_from_xpm_d (parent->window, &mqbl,
					&style->bg[GTK_STATE_NORMAL],
					(gchar **) qbl_xpm);   
    qbr = gdk_pixmap_create_from_xpm_d (parent->window, &mqbr,
					&style->bg[GTK_STATE_NORMAL],
					(gchar **) qbr_xpm);
    IconsLoaded = true;
  }
}

GtkWidget * QuickBarIcons::CommandButton(GdkPixmap *pix, GdkBitmap *mask, 
					 char *caption) {
  GtkWidget *b, *h, *p, *t=0;

  b=gtk_button_new();
  h=gtk_hbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(b),h);

  p=gtk_pixmap_new(pix, mask);

  if (caption) t=gtk_label_new(caption);

  gtk_box_pack_start(GTK_BOX(h),p,FALSE,TRUE,0);

  if (caption) gtk_box_pack_start(GTK_BOX(h),t,FALSE,TRUE,2);

  gshow(p);
  if (caption) gshow(t);
  gshow(h);
  return b;
}
