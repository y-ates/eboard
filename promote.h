/* $Id: promote.h,v 1.3 2007/01/01 18:29:03 bergo Exp $ */


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

#ifndef PROMOTE_H
#define PROMOTE_H 1

#include "eboard.h"
#include "widgetproxy.h"

class PromotionPicker : public WidgetProxy,
                        public PieceProvider 
{
 public:
  PromotionPicker(GdkWindow *window);
  void   setPromotion(int index);
  int    getPromotion();
  piece  getPromotionPiece();

  virtual piece getPiece();

 private:
  int promotion;
  GtkWidget *button[5];
  friend void promote_toggle(GtkWidget *widget,gpointer data);
};

void promote_toggle(GtkWidget *widget,gpointer data);

// yeah. this is the only one in the whole code. <laugh>
class UglyHack {
 public:
  UglyHack(PromotionPicker *a,int b);
  PromotionPicker *picker;
  int index;
};

#endif
