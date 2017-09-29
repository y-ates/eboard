/* $Id: promote.cc,v 1.8 2003/06/30 16:55:42 bergo Exp $ */


/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2003 Felipe Paulo Guazzi Bergo
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
#include "promote.h"
#include "eboard.h"

#include "q18.xpm"
#include "r18.xpm"
#include "b18.xpm"
#include "n18.xpm"
#include "k18.xpm"

UglyHack::UglyHack(PromotionPicker *a,int b) {
  picker=a;
  index=b;
}

PromotionPicker::PromotionPicker(GdkWindow *window) {
  GtkWidget *l,*pm;
  GdkPixmap *d;
  GdkBitmap *b;
  GtkStyle *style,*dstyle;
  GtkWidget *h;
  GdkColor red,red2;
  int i;

  widget=gtk_frame_new(0);
  gtk_frame_set_shadow_type(GTK_FRAME(widget),GTK_SHADOW_OUT);

  h=gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),h);

  l=gtk_label_new(_("Promotion Piece  "));
  gtk_box_pack_start(GTK_BOX(h),l,FALSE,TRUE,0);
  gshow(l);

  style=gtk_widget_get_style(h);

  // 0 button
  button[0]=gtk_toggle_button_new(); 
  d = gdk_pixmap_create_from_xpm_d (window, &b,
				    &style->bg[GTK_STATE_NORMAL],
				    (gchar **) q18_xpm);
  pm=gtk_pixmap_new(d,b);
  gtk_container_add(GTK_CONTAINER(button[0]),pm);
  gshow(pm);

  // 1 button
  button[1]=gtk_toggle_button_new(); 
  d = gdk_pixmap_create_from_xpm_d (window, &b,
				    &style->bg[GTK_STATE_NORMAL],
				    (gchar **) r18_xpm);
  pm=gtk_pixmap_new(d,b);
  gtk_container_add(GTK_CONTAINER(button[1]),pm);
  gshow(pm);

  // 2 button
  button[2]=gtk_toggle_button_new(); 
  d = gdk_pixmap_create_from_xpm_d (window, &b,
				    &style->bg[GTK_STATE_NORMAL],
				    (gchar **) b18_xpm);
  pm=gtk_pixmap_new(d,b);
  gtk_container_add(GTK_CONTAINER(button[2]),pm);
  gshow(pm);

  // 3 button
  button[3]=gtk_toggle_button_new(); 
  d = gdk_pixmap_create_from_xpm_d (window, &b,
				    &style->bg[GTK_STATE_NORMAL],
				    (gchar **) n18_xpm);
  pm=gtk_pixmap_new(d,b);
  gtk_container_add(GTK_CONTAINER(button[3]),pm);
  gshow(pm);

  // 4 button
  button[4]=gtk_toggle_button_new(); 
  d = gdk_pixmap_create_from_xpm_d (window, &b,
				    &style->bg[GTK_STATE_NORMAL],
				    (gchar **) k18_xpm);
  pm=gtk_pixmap_new(d,b);
  gtk_container_add(GTK_CONTAINER(button[4]),pm);
  gshow(pm);

  dstyle=gtk_style_copy( gtk_widget_get_style(button[0]) );
  red.red=0xffff;
  red.green=0x6000;
  red.blue=0x0000;

  red2.red=0xffff;
  red2.green=0xadad;
  red2.blue=0x7c7c;

  dstyle->bg[GTK_STATE_ACTIVE]=red;
  dstyle->bg[GTK_STATE_SELECTED]=red;
  dstyle->bg[GTK_STATE_PRELIGHT]=red2;

  /*
    GTK_STATE_NORMAL,
    GTK_STATE_ACTIVE,
    GTK_STATE_PRELIGHT,
    GTK_STATE_SELECTED,
    GTK_STATE_INSENSITIVE
  */

  for(i=0;i<5;i++) {
    gtk_widget_set_style( button[i], dstyle );
    gtk_box_pack_start(GTK_BOX(h),button[i],FALSE,FALSE,0);
    gshow(button[i]);
    gtset(GTK_TOGGLE_BUTTON(button[i]),!i);
    gtk_signal_connect(GTK_OBJECT(button[i]),"toggled",
		       GTK_SIGNAL_FUNC(promote_toggle),
		       new UglyHack(this,i));
  }
  gshow(h);
  promotion=0;
}

void PromotionPicker::setPromotion(int index) {
  int i;
  promotion=index;
  for(i=0;i<5;i++)
    gtset(GTK_TOGGLE_BUTTON(button[i]),(i==promotion));
}

int  PromotionPicker::getPromotion() {
  return(promotion);
}

piece PromotionPicker::getPromotionPiece() {
  switch(promotion) {
  case 1:  return ROOK;
  case 2:  return BISHOP;
  case 3:  return KNIGHT;
  case 4:  return KING;
  default: return QUEEN;
  }
}

piece PromotionPicker::getPiece() {
  return(getPromotionPiece());
}

void promote_toggle(GtkWidget *widget,gpointer data)
{
  UglyHack *uh;
  int ia,i;

  uh=(UglyHack *)data;

  ia=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  if (ia) { // activated
    uh->picker->promotion=uh->index;
    for(i=0;i<5;i++)
      if (i!=uh->index)
	gtset(GTK_TOGGLE_BUTTON(uh->picker->button[i]),0);      
  } else { // deactivated
    if (uh->picker->promotion != uh->index)
      return;
    gtset(GTK_TOGGLE_BUTTON(uh->picker->button[uh->index]),1);
  }
}

