/* $Id: bugpane.cc,v 1.18 2008/02/08 14:25:50 bergo Exp $ */

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
#include "bugpane.h"
#include "global.h"
#include "eboard.h"

string BugPane::BugTell;

// ---------------- BareBoard

BareBoard::BareBoard() : WidgetProxy() {
  global.addPieceClient(this);
  Names[0]=_("None");
  Names[1]=_("None");

  widget=gtk_drawing_area_new();
  gtk_widget_set_events(widget,GDK_EXPOSURE_MASK);
  gtk_signal_connect(GTK_OBJECT(widget),"expose_event",
             GTK_SIGNAL_FUNC(bareboard_expose),(gpointer)this);

  pixbuf=0;

  pset=0;
  if (global.pieceset)
    pset=new PieceSet(global.pieceset);

  clock.setHost(this);

  upending=false;
  frozen=0;
}

BareBoard::~BareBoard() {
  global.removePieceClient(this);
  if (pixbuf)
    gdk_pixmap_unref(pixbuf);
}

void BareBoard::setPosition(Position &pos) {
  position = pos;
  repaint();
}

void BareBoard::addPTell(char *text) {
  PTells.push_back(string(text));
  if (PTells.size() > 15) PTells.pop_front();
}

void BareBoard::setWhite(char *name) {
  Names[0]=name;
  repaint();
}

void BareBoard::setBlack(char *name) {
  Names[1]=name;
  repaint();
}

void BareBoard::update() {
  if (frozen>0)
    upending=true;
  else {
    repaint();
    upending=false;
  }
}

void BareBoard::updateClock() {
  repaint();
}

void BareBoard::setClock2(int wmsecs,int bmsecs,int actv,int cdown) {
  clock.setClock2(wmsecs,bmsecs,actv,cdown);
  update();
}

void BareBoard::pieceSetChanged() {
  if (pset)
    delete pset;
  pset = new PieceSet(global.pieceset);
  update();
}

void BareBoard::reloadPieceSet() {
  if (pset)
    delete pset;
  pset = new PieceSet(global.pieceset);
}

void BareBoard::freeze() {
  ++frozen;
}

void BareBoard::thaw() {
  --frozen;
  if ( (frozen<1) && upending )
    update();
}

gboolean bareboard_expose(GtkWidget *widget,GdkEventExpose *ee,
              gpointer data) {

  BareBoard *me;
  int i,j,k,ww,wh,zh,x,y=0;
  int sqside;
  GdkGC *gc;
  char z[256],t[64];
  list<string>::iterator pti;
  rgbptr tmp;
  int rowsz;
  bool flip;

  static piece stock[5]={PAWN,ROOK,KNIGHT,BISHOP,QUEEN};

  flip=global.BoardList.front()->effectiveFlip();
  flip=!flip;

  me=(BareBoard *)data;

  gdk_window_get_size(widget->window,&ww,&wh);
  zh=wh;
  sqside=ww/8;

  if ( (!global.UseVectorPieces) && (me->pset->extruded) )
    wh-=wh/16;

  if (wh<ww) sqside=wh/8;

  if (me->pixbuf) {
    if (ww != me->pixw || wh != me->pixh) {
      gdk_pixmap_unref(me->pixbuf);
      me->pixbuf=0;
      me->pixw=ww;
      me->pixh=wh;
    }
  }

  if (! me->pixbuf)
    me->pixbuf=gdk_pixmap_new(widget->window,ww,wh,-1);

  gc=gdk_gc_new(me->pixbuf);
  gdk_rgb_gc_set_foreground(gc,0);
  gdk_draw_rectangle(me->pixbuf,gc,TRUE,0,0,ww,wh);

  me->C.prepare(widget,me->pixbuf,gc,0,0,ww,wh);
  me->C.setFont(0,global.InfoFont);

  if (sqside > 8) {

    if (global.UseVectorPieces) {
      global.vpieces.drawSquares(me->pixbuf,gc,sqside);

      for(i=0;i<8;i++)
    for(j=0;j<8;j++)
      global.vpieces.drawPiece(me->pixbuf,gc,sqside,
                   i*sqside, j*sqside,
                   me->position.getPiece(flip?7-i:i,
                             flip?j:7-j));
    } else {
      if (me->pset->extruded) y=sqside/2;
      if (me->pset->side != sqside) {
    me->reloadPieceSet();
    me->pset->scale(sqside);
      }

      rowsz=sqside*8;
      tmp=(rgbptr)g_malloc(rowsz * (rowsz+sqside/2) * 4);


      me->pset->beginQueueing();
      for(i=0;i<8;i++)
    for(j=0;j<8;j++)
      me->pset->drawPieceAndSquare(me->position.getPiece(flip?7-i:i,
                                 flip?j:7-j),
                       tmp, i*sqside , y+j*sqside, rowsz,
                       (i+j)%2);
      me->pset->endQueueing();

      gdk_draw_rgb_image(me->pixbuf, gc, 0, 0, rowsz, rowsz+y,
             GDK_RGB_DITHER_NORMAL, tmp, rowsz*3);

      g_free(tmp);

    }


    x=sqside*8+10;

    int fh = me->C.fontHeight(0);

    me->C.setColor(0xffffff);
    me->C.drawString(x,10,0,_("Bughouse: Partner Game"));
    me->C.drawString(x+1,10,0,_("Bughouse: Partner Game"));

    // white's clock

    if (me->clock.getActive() == -1) {
      gdk_draw_rectangle(me->pixbuf,gc,TRUE,x-5,(flip?40:zh-5-fh), 150, 20);
      me->C.setColor(0);
    }

    me->clockString(me->clock.getValue2(0),t,64);
    snprintf(z,256,_("White: %s - %s"),me->Names[0].c_str(),t);

    me->C.drawString(x,flip?40:zh-5-fh,0,z);

    // black's clock

    me->C.setColor(0xffffff);
    if (me->clock.getActive() == 1) {
      gdk_draw_rectangle(me->pixbuf,gc,TRUE,x-5,(flip?zh-5-fh:40), 150, 20);
      me->C.setColor(0);
    }

    me->clockString(me->clock.getValue2(1),t,64);
    snprintf(z,256,_("Black: %s - %s"),me->Names[1].c_str(),t);
    me->C.drawString(x,flip?zh-5-fh:40,0,z);

    // paint stock

    // white's stock

    x=sqside*8+10;
    for(i=0;i<5;i++) {
      k=me->position.getStockCount(stock[i]|WHITE);
      for(j=0;j<k;j++) {
    global.vpieces.drawPiece(me->pixbuf,gc,sqside,x,flip?45:zh-5-sqside-20,
                 stock[i]|WHITE);
    x+=sqside+2;
      }
    }

    // black's stock

    x=sqside*8+10;
    for(i=0;i<5;i++) {
      k=me->position.getStockCount(stock[i]|BLACK);
      for(j=0;j<k;j++) {
    global.vpieces.drawPiece(me->pixbuf,gc,sqside,x,flip?zh-5-sqside-20:45,
                 stock[i]|BLACK);
    x+=sqside+2;
      }
    }

    // paint ptells

    x=sqside*8+10;
    me->C.setColor(global.Colors.TextBright);
    me->C.drawString(x,40+sqside,0,_("Partner Tells:"));

    if (!me->PTells.empty()) {
      me->C.setColor(global.Colors.PrivateTell);
      i=zh-20-5-sqside;

      pti=me->PTells.end();
      for(pti--;i>(59+sqside);i-=20, pti--) {
    me->C.drawString(x,i,0,(*pti).c_str() );
    if (pti == me->PTells.begin())
      break;
      }
    }
  }
  gdk_gc_destroy(gc);
  gc=gdk_gc_new(widget->window);
  gdk_draw_pixmap(widget->window,gc,me->pixbuf,0,0,0,0,ww,zh);
  gdk_gc_destroy(gc);
  return 1;
}

// ---------------- BugPane

BugPane::BugPane() {
  GtkWidget *v, *tbl, *qb[18];
  GdkColor gray[5];
  GtkStyle *style;
  int i,c,r;
  static char *stuff[18]={"---","--","-","+","++","+++",
              "P","N","B","R","Q","Diag",
              "Sit","Go","Fast","Hard","Dead","Safe"};


  widget = gtk_hbox_new(FALSE,0);

  board = new BareBoard();
  board->show();

  gtk_box_pack_start(GTK_BOX(widget), board->widget, TRUE,TRUE, 0);

  // vbox
  v=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(widget), v, FALSE, TRUE, 0);

  // table
  tbl=gtk_table_new(6,3,FALSE);
  gray[0].red=gray[0].green=gray[0].blue=0xaaaa;
  gray[1].red=gray[1].green=gray[1].blue=0x8888;
  gray[2].red=gray[2].green=gray[2].blue=0xcccc;
  gray[3].red=gray[3].green=gray[3].blue=0x8888;
  gray[4].red=gray[4].green=gray[4].blue=0xaaaa;
  style=gtk_style_copy( gtk_widget_get_default_style() );
  for(i=0;i<5;i++)
    style->bg[i]=gray[i];

  // buttons
  for(i=0;i<18;i++) {

    qb[i]=gtk_button_new_with_label(stuff[i]);
    if (i<6)
      gtk_widget_set_style(qb[i],style);

    c=i/6;
    r=i%6;
    gtk_table_attach_defaults(GTK_TABLE(tbl),qb[i],c,c+1,r,r+1);
    gshow(qb[i]);
    gtk_signal_connect(GTK_OBJECT(qb[i]),"clicked",
               GTK_SIGNAL_FUNC(bug_ptell),(gpointer)(stuff[i]));
  }

  gtk_box_pack_start(GTK_BOX(v), tbl, FALSE, FALSE, 0);

  Gtk::show(tbl,v,NULL);
}

void BugPane::stopClock() {
  board->setClock2(0,0,0,1);
}

void BugPane::reset() {
  Position p;

  freeze();
  board->setClock2(0,0,0,1);
  board->setPosition(p);
  board->setWhite(_("None"));
  board->setBlack(_("None"));
  thaw();

}

void BugPane::addBugText(char *text) {
  board->addPTell(text);
  board->update();
}

void BugPane::setPlayerNames(char *white, char *black) {
  board->setWhite(white);
  board->setBlack(black);
  board->update();
}

void BugPane::setPosition(Position &pos) {
  board->setPosition(pos);
  board->update();
}

void BugPane::setClock2(int wmsecs,int bmsecs,int actv,int cdown) {
  board->setClock2(wmsecs,bmsecs,actv,cdown);
}

void BugPane::freeze() { board->freeze(); }
void BugPane::thaw()   { board->thaw(); }

void bug_ptell(GtkWidget *b,gpointer data) {
  char z[256],x[256],c;

  g_strlcpy(z,(char *)data,256);

  if ((z[0]=='-')||(z[0]=='+')) {
    BugPane::BugTell=z;
    return;
  } else {
    if (BugPane::BugTell.empty())
      BugPane::BugTell=z;
    else {
      g_strlcpy(x,BugPane::BugTell.c_str(),256);
      c=x[strlen(x)-1];
      if ((c=='+')||(c=='-'))
    BugPane::BugTell+=z;
      else
    BugPane::BugTell=z;
    }
  }

  if (global.protocol) {
    snprintf(z,256,"ptell %s",BugPane::BugTell.c_str());
    global.protocol->sendUserInput(z);
  }
}
