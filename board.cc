/* $Id: board.cc,v 1.121 2008/02/08 14:25:50 bergo Exp $ */

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
#include "board.h"
#include "chess.h"
#include "global.h"
#include "text.h"
#include "protocol.h"
#include "dgtboard.h"

#include "mainwindow.h"

Board * Board::PopupOwner = 0;

static GdkEventMotion LastMotionEvent;

// --- Board --------------------------

#if MAEMO
void tap_and_hold_cb(GtkWidget *widget, gpointer user_data) {
  ((Board*)user_data)->popupProtocolMenu(100,100,0);
}
#endif

Board::Board() : WidgetProxy() {
  int i;

  global.debug("Board","Board");

  global.addPieceClient(this);

  cur=0;
  clock.setHost(this);

  premoveq=0;
  mygame=0;
  clock_ch=1;
  hilite_ch=1;
  allowMove=true;
  gotAnimationLoop=0;
  update_queue=false;
  LockAnimate=0;
  FreeMove=false;
  dr_active=false;
  FlipInvert=false;
  EditMode=false;
  UpdateClockAfterConfigure = false;
  jx=jy=-1;
  jpxv = 0; jpyv = 0;

  jsx=jsy=-1;
  jsvx=jsvy=0;
  jstid=-1;

  AnimationTimeout = -1;

  for(i=0;i<6;i++)
    info[i].erase();

  currently_rendered.setPiece(0,0,EMPTY);

  repaint_due=false;
  frozen_lvl=0;

  if (global.ShowCoordinates) { borx=20; bory=20; } else { borx=bory=0; }
  morey=0;

  global.BoardList.push_back(this);

  canselect=true;
  sel_color=0xffff00;
  sp=0;
  flipped=false;

  widget=yidget=gtk_drawing_area_new();
  gtk_widget_set_events(yidget,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
			GDK_BUTTON_RELEASE_MASK|GDK_BUTTON1_MOTION_MASK|
			GDK_POINTER_MOTION_HINT_MASK);

  sqside=59;
  sqw = 2*borx + 8*sqside + 1;
  sqh = 2*bory + morey + 8*sqside + 1;

  buffer=gdk_pixmap_new(MainWindow::RefWindow,sqw,sqh,-1);
  clkbar=gdk_pixmap_new(MainWindow::RefWindow,250,sqh,-1);

  clkgc=gdk_gc_new(clkbar);
  wgc=0;
  
  scrap=(rgbptr)g_malloc(sqw*sqh*3);
  memset(scrap,0,sqw*sqh*3);

  gtk_signal_connect (GTK_OBJECT (yidget), "expose_event",
                      (GtkSignalFunc) board_expose_event, (gpointer) this);
  gtk_signal_connect (GTK_OBJECT (yidget), "configure_event",
                      (GtkSignalFunc) board_configure_event, (gpointer) this);
  gtk_signal_connect (GTK_OBJECT (yidget), "button_press_event",
                      (GtkSignalFunc) board_button_press_event,
		      (gpointer) this);
  gtk_signal_connect (GTK_OBJECT (yidget), "button_release_event",
                      (GtkSignalFunc) board_button_release_event,
		      (gpointer) this);
  gtk_signal_connect (GTK_OBJECT (yidget), "motion_notify_event",
                      (GtkSignalFunc) board_motion_event,
		      (gpointer) this);

#if MAEMO
  // Note: the following code does not work
  // I get no events :-/
  gtk_widget_tap_and_hold_setup (yidget, NULL, NULL, GtkWidgetTapAndHoldFlags(0));
#if 0
  gtk_signal_connect (GTK_OBJECT (yidget), "tap-and-hold",
		      //(GtkSignalFunc) board_button_press_event,
		      (GtkSignalFunc) tap_and_hold_cb ,
		      (gpointer) this);
#else
  g_signal_connect (G_OBJECT (yidget), "tap-and-hold", G_CALLBACK (tap_and_hold_cb), (gpointer) this);
#endif
#endif

  gshow(widget);
}

bool Board::hasGame() {
  return(mygame!=0);
}

Board::~Board() {
  unsigned int i;
  global.debug("Board","~Board");

  if (mygame!=NULL)
    if (mygame->getBoard() == this)
      mygame->setBoard(NULL);

  global.removeBoard(this);
  global.removePieceClient(this);
  for(i=0;i<LastMove.size();i++)
    delete(LastMove[i]);
  LastMove.clear();
  if (cur) {
    delete cur;
    cur = NULL;
  }
  if (wgc)
    gdk_gc_destroy(wgc);
  if (scrap!=NULL) {
    g_free(scrap);
    scrap = NULL;
  }    
  gdk_gc_destroy(clkgc);
  gdk_pixmap_unref(clkbar);
  gdk_pixmap_unref(buffer);
  if (AnimationTimeout >= 0)
    gtk_timeout_remove(AnimationTimeout);
}

void Board::reloadFonts() {
  clock_ch=1;
  queueRepaint();
}

/*
  Resets the board state regarding any previous game played
  here.

  This method is called by the protocol classes when a new
  game is being attached to the board (e.g. when a engine is
  run, when a new game starts on FICS, etc.
*/

void Board::reset() {
  unsigned int i;
  global.debug("Board","reset");
  while(gotAnimationLoop)
    global.WrappedMainIteration();

  if (mygame) {
    if ( (mygame->getBoard() == this) && (mygame->isOver()) )
      mygame->setBoard(0);
  }

  update_queue=false;

  while(!UpdateStack.empty())
    UpdateStack.pop();

  sp=0;
  mygame=0;
  premoveq=0;
  flipped=false;
  clock_ch=1;
  hilite_ch=1;
  allowMove=true;
  dr_active=false;
  LockAnimate=0;
  FlipInvert=false;
  for(i=0;i<6;i++)
    info[i].erase();
  clock.setMirror(0);
  clock.setClock2(0,0,0,1);
  currently_rendered.invalidate();
  position.setStartPos();
  for(i=0;i<LastMove.size();i++)
    delete(LastMove[i]);
  LastMove.clear();
  cleanTargets();
  queueRepaint();
}

void Board::setFlipInversion(bool v) {
  FlipInvert=v;
  currently_rendered.invalidate();
  clock_ch=1;
  hilite_ch=1;
  queueRepaint();
}

bool Board::getFlipInversion() {
  return(FlipInvert);
}

void Board::pieceSetChanged() {
 if (cur)
    delete cur;
  cur=new PieceSet(global.pieceset);
  currently_rendered.invalidate();
  if (global.ShowCoordinates) { borx=20; bory=20; } else { borx=bory=0; }
  morey=0;
  UpdateClockAfterConfigure = true;
  queueRepaint();
}

ChessGame * Board::getGame() {
  return(mygame);
}

void Board::setGame(ChessGame *game) {
  global.debug("Board","setGame");
  mygame=game;
  clock_ch=1;
  queueRepaint();

  // update button enabling/disabling
  global.ebook->pretendPaneChanged();
}

void Board::show() {
  // already called on creation
}

void Board::setSensitive(bool state) {
  canselect=state;
  if ((!canselect)&&(sp)) {
    sp=0;
    hilite_ch=1;
    queueRepaint();
  }
}

void Board::clearSel() {
  if (sp) {
    sp=0;
    hilite_ch=1;
    queueRepaint();
  }
}

void Board::setSelColor(int color) {
  sel_color=color;
  if (sp) {
    hilite_ch=1;
    queueRepaint();
  }
}

void Board::setFlipped(bool flip) {
  bool ov;
  ov=flipped;
  flipped=flip;
  if (flipped!=ov) {
    currently_rendered.invalidate();
    clock_ch=1;
    queueRepaint();
  }
}

void Board::supressNextAnimation() {
  ++LockAnimate;
}

void Board::updateClock() {
  if (yidget->window) {
    clock_ch=1;  
    renderClock();
    gdk_draw_pixmap(yidget->window,
		    yidget->style->fg_gc[GTK_WIDGET_STATE (yidget)],
		    clkbar,
		    0, 0,
		    Width(), 0,250, Height());
    if (dr_active)
      board_motion_event(widget,&LastMotionEvent,(gpointer) this);
  }
}

void RootBoard::clockString(int val,char *z,int maxlen) {
  static char *minus = "-";
  int cv,mr,neg,H,M,S;
  cv=val;
  if (cv<0) {
    cv=-cv;
    neg=1;
  } else
    neg=0;
  
  mr = cv%1000;
  cv /= 1000;

  H=cv/3600;
  M=(cv-3600*H)/60;
  S=(cv-3600*H)%60;

  if (H) {
    snprintf(z,maxlen,"%s%d:%.2d:%.2d",
	     neg?minus:&minus[1],H,M,S);
  } else {
    if (mr && M==0)
      snprintf(z,maxlen,"%s%.2d:%.2d.%.3d",
	       neg?minus:&minus[1],M,S,mr);
    else
      snprintf(z,maxlen,"%s%.2d:%.2d",
	       neg?minus:&minus[1],M,S);
  }
}

void Board::repaint() {
  global.debug("Board","repaint");
  WidgetProxy::repaint();
}

void Board::renderClock() {
  GdkGC *gc=clkgc;
  int i,j,k,sq,bh,ch,th;
  char z[64];
  bool ef, lowtime=false;

  if (!clock_ch)
    return;

  if (mygame)
    if (mygame->AmPlaying)
      lowtime=clock.lowTimeWarning(mygame->MyColor);

  sq=sqside;

  C.prepare(widget,clkbar,gc,0,0,250,sqh);
  C.setColor(0);
  C.drawRect(0,0,C.W,C.H,true);
  C.H = Height();

  C.setFont(0,global.PlayerFont);
  C.setFont(1,global.ClockFont);
  C.setFont(2,global.InfoFont);

  ch = C.fontHeight(1);
  th = C.fontHeight(0);
  bh = ch + th;

  if (sq*3 < bh) {
    C.setFont(0,global.InfoFont);
    C.setFont(1,global.InfoFont);
    C.setFont(2,global.InfoFont);
    ch = C.fontHeight(1);
    th = C.fontHeight(0);
    bh = ch + th;
  }

  // draw the clock time and White/Black labels

  ef=effectiveFlip();
  if (ef) i= C.H - bh; else i = 0;
  j=16;

  C.setColor(0xffffff);
  if (clock.getActive()>0) {
    if (lowtime) C.setColor(0xff8080);
    C.drawRect(15, i, 200-30, bh+4, true);
    C.setColor(0);
  }

  C.drawString(j,i,0,_("Black"));
  clockString(clock.getValue2(1),z,64);
  C.drawString(j,i+th,1,z);

  if (ef) i=0; else i= C.H - bh;

  C.setColor(0xffffff);
  if (clock.getActive()<0) {
    if (lowtime) C.setColor(0xff8080);
    C.drawRect(15, i, 200-30, bh+4, true);
    C.setColor(0);
  }
  C.drawString(j,i,0,_("White"));
  clockString(clock.getValue2(0),z,64);
  C.drawString(j,i+th,1,z);

  C.consumeTop(bh+10);
  C.consumeBottom(bh+10);

  // if we have enough space, render player names too
  if ( (mygame) && (C.H > 2*th) ) {
    C.setColor(0xffffff);
    C.drawString(j,C.Y,0, mygame->getPlayerString(ef?0:1) );
    C.drawString(j,C.Y + C.H - th,0, mygame->getPlayerString(ef?1:0) );
    C.consumeTop(th+8);
    C.consumeBottom(th+8);
  }

  cleanTargets();

  // now render the information lines

  if (mygame) {
    C.setColor(0xffffff);

    for(k=0;k<5;k++) {
      if (C.H < 16) break;
      C.drawString(j, C.Y, 2, info[k] );
      C.consumeTop(16);
    }
    
    // house string is info[5]

    if (!info[5].empty()) {      
      if ((!global.DrawHouseStock)||(C.H < 96)||(EditMode)) {
	if (C.H >= 16) {
	  C.drawString(j,C.Y,2,info[5]);
	  C.consumeTop(16);
	}
      } else {
	// draw pieces in stock
	piece pk[5]={PAWN,ROOK,KNIGHT,BISHOP,QUEEN};
	int st[5][2];
	int si,sj,we,be;

	// build stock info from string
	for(si=0;si<5;si++) { st[si][0]=st[si][1]=0; }
	sj=0;
	k=info[5].length();
	for(i=0;i<k;i++) {
	  switch(info[5][i]) {
	  case 'P': st[0][sj]++; break;
	  case 'R': st[1][sj]++; break;
	  case 'N': st[2][sj]++; break;
	  case 'B': st[3][sj]++; break;
	  case 'Q': st[4][sj]++; break;
	  case ']': sj=1; break;
	  }
	}

	// draw stock pieces
	int eph, my, fh2, phs, pcsw;

	if (global.UseVectorPieces)
	  eph=40+8;
	else
	  eph=cur->side + 8;

	if (ef) { we=0; be=eph; } else { we=eph; be=0; }
	fh2 = C.fontHeight(2);
	if (!global.UseVectorPieces)
	  phs = cur->side/2;

	my=2*eph;
	C.setColor(0x505050);
	for(si=2;si<my+2;si+=2)
	  C.drawLine(5,C.Y+si,235,C.Y+si);

	for(si=3;si<my+2;si+=2) {
	  int lv;
	  lv=my/2 - si;
	  if (lv<0) lv=-lv;
	  lv*=2;
	  if (lv>0xc0) lv=0xc0;
	  lv=0xc0-lv;
	  lv=(lv<<16)|(lv<<8)|lv;
	  C.setColor(lv);

	  C.drawLine(5,C.Y+si,235,C.Y+si);
	}

	C.setColor(0x555555);
	for(si=0;si<5;si++)
	  C.drawLine(40*(si+1),C.Y+2,40*(si+1),C.Y+my+1);

	for(si=0;si<5;si++) {
	  if (st[si][0]) {

	    if (global.UseVectorPieces)
	      global.vpieces.drawPiece(clkbar,gc,40,40*si,C.Y+we,pk[si]|WHITE);
	    else
	      cur->drawPiece(pk[si]|WHITE,clkbar,gc,40*si+20-phs, C.Y+we);

	    snprintf(z,64,"%d",st[si][0]);

	    pcsw = C.stringWidth(2,z);
	    shadyText(40*si+20-pcsw/2, C.Y+eph+we-fh2, 2, z);

	    if (mygame->MyColor==WHITE)
	      addTarget(new DropSource(pk[si]|WHITE,Width()+40*si,C.Y+we,40,eph));

	    /*
	    C.setColor(0xffff00);
	    C.drawRect(40*si,C.Y+we,40,eph,false);
	    */
	  }
	  if (st[si][1]) {

	    if (global.UseVectorPieces)
	      global.vpieces.drawPiece(clkbar,gc,40,40*si,C.Y+be,pk[si]|BLACK);
	    else
	      cur->drawPiece(pk[si]|BLACK,clkbar,gc,40*si+20-phs, C.Y+be);

	    snprintf(z,64,"%d",st[si][1]);

	    pcsw = C.stringWidth(2,z);
	    shadyText(40*si+20-pcsw/2, C.Y+eph+be-fh2, 2, z);

	    if (mygame->MyColor==BLACK)
	      addTarget(new DropSource(pk[si]|BLACK,Width()+40*si,C.Y+be,40,eph));

	    /*
	    C.setColor(0xffff00);
	    C.drawRect(40*si,C.Y+be,40,eph,false);
	    */
	  }
	} // for

	C.consumeTop(2*eph+8);
      } // text | graphic stock
    } // if !info[5].empty

    // draw mouse if protocol menu available
    if (global.protocol)
      if (global.protocol->getPlayerActions()) {
	C.setColor(0xffffff);
	C.drawRect(170,C.Y+8,16,18,true);
	C.drawLine(177,C.Y+8,177,C.Y+4);
	C.setColor(0xffcc00);
	C.drawRect(180,C.Y+8,6,8,true);
	C.setColor(0);
	C.drawLine(170,C.Y+16,186,C.Y+16);
	C.drawLine(175,C.Y+8,175,C.Y+16);
	C.drawLine(180,C.Y+8,180,C.Y+16);
	C.consumeTop(20);
      }	
  }

  if (C.H > 32 && !EditMode) {
    if (position.getAnnotation()) {
      C.consumeTop(16);
      j=16;

      C.setColor(0x252535);
      C.drawRect(j-5,C.Y,170,C.H,true);
      C.setColor(0x4545ff);
      C.drawRect(j-5,C.Y,170,C.H,false);

      C.setColor(0xffffff);
      C.drawBoxedString(j,C.Y,160,C.H,2, position.getAnnotation(), " ");
    }
  }

  clock_ch=0;
}

ChessClock * Board::getClock() {
  return(&clock);
}

int Board::getClockValue2(bool white) {
  return(clock.getValue2(white?0:1));
}

void Board::setClock2(int wmsecs,int bmsecs,int actv,int cdown) {
  global.debug("Board","setClock");
  clock_ch=1;
  clock.setClock2(wmsecs,bmsecs,actv,cdown);
  queueRepaint();
}

void Board::shadyText(int x,int y, int f, char *z) {
  int i,j;
  
  C.setColor(0);
  for(i=-2;i<=3;i++)
    for(j=-2;j<=2;j++)
      C.drawString(x+i,y+j,f,z);

  C.setColor(0x808080);
  C.drawString(x,y,f,z);
  C.setColor(0xffffff);
  C.drawString(x+1,y,f,z);
}

void Board::freeze() {
  frozen_lvl++;
}

void Board::thaw() {
  frozen_lvl--;
  if (frozen_lvl < 0) frozen_lvl=0;
  if ((!frozen_lvl)&&(repaint_due)) {
    repaint();
    repaint_due=false;
  }
}

void Board::invalidate() {
  currently_rendered.invalidate();
  queueRepaint();
}

void Board::queueRepaint() {
  global.debug("Board","queueRepaint");
  if (frozen_lvl)
    repaint_due=true;
  else
    repaint();
}

void Board::setInfo(int idx,char *s) {
  global.debug("Board","setInfo",s);
  info[idx%6]=s;
  clock_ch=1;
  queueRepaint();
}

void Board::setInfo(int idx,string &s) {
  global.debug("Board","setInfo/2");
  info[idx%6] = s;
  clock_ch=1;
  queueRepaint();
}

void Board::update(bool sndflag) {
  unsigned int i,rate;
  AnimatedPiece *ap;  
  Position rpv;
  bool ef;
  bool sok=false;

  global.debug("Board","update");

  if (!mygame)
    return;

  if ((global.AnimateMoves)&&(gotAnimationLoop)) {
    update_queue=true;
    UpdateStack.push(sndflag);
    return;
  }  

  position = mygame->getCurrentPosition();
  rpv = mygame->getPreviousPosition();
  ef=effectiveFlip();

  if ((global.HilightLastMove)||(global.AnimateMoves)) {
    rpv.diff(position,LastMove);
    if (LockAnimate)
      --LockAnimate;
    else
      if (currently_rendered != position)
	if ((global.AnimateMoves)&&(gdk_window_is_viewable(yidget->window))) {
	  sok=true; // sound ok
	  fake=position;
	  fake.intersection(rpv);
	  rate = global.SmootherAnimation ? 5 : 3;
	  for(i=0;i<LastMove.size();i++) {
	    if (ef)
	      ap=new FlatAnimatedPiece(LastMove[i]->Piece,
				       borx+sqside*(7-LastMove[i]->SX),
				       morey+bory+sqside*(LastMove[i]->SY),
				       borx+sqside*(7-LastMove[i]->DX),
				       morey+bory+sqside*(LastMove[i]->DY),
				       rate*LastMove[i]->distance(),sndflag);
	    else
	      ap=new FlatAnimatedPiece(LastMove[i]->Piece,
				       borx+sqside*(LastMove[i]->SX),
				       morey+bory+sqside*(7-LastMove[i]->SY),
				       borx+sqside*(LastMove[i]->DX),
				       morey+bory+sqside*(7-LastMove[i]->DY),
				       rate*LastMove[i]->distance(),sndflag);
	    ap->create(yidget->window,cur,sqside);
	    animes.push_back(ap);
	  }
	  if ((!LastMove.size())&&(sndflag))
	    global.flushSound();
	  if ((!animes.empty())&&(!gotAnimationLoop)) {
	    int tag;
	    gotAnimationLoop=1;
	    currently_rendered.invalidate();
	    board_configure_event(yidget,0,(gpointer)this);
	    if (rate==3)
	      tag=gtk_timeout_add(global.UseVectorPieces?40:70,
				  board_animate,this);
	    else
	      tag=gtk_timeout_add(global.UseVectorPieces?30:40,
				  board_animate,this);
	    AnimationTimeout = tag;
	  }
	}
  }
  if (!global.AnimateMoves)
    queueRepaint();

  /* force redraw of background of dragging */
  if (dr_active)
    dr_step=0;

  update_queue=false;

  if ((!sok)&&(global.BeepWhenOppMoves)&&(sndflag))
    global.flushSound();
}

void Board::sendMove() {
  global.debug("Board","sendMove");
  if ((sp<2)||(!mygame)) return;
  if ((allowMove)||(FreeMove)) {
    if (global.effectiveLegalityChecking() && !FreeMove)
      if (!position.isMoveLegalCartesian(ax[0],ay[0],ax[1],ay[1],
					 mygame->MyColor,mygame->Variant)) {
	char z[128];
	snprintf(z,128,_("Illegal Move %c%d%c%d (Legality Checking On)"),
		 'a'+ax[0],ay[0]+1,'a'+ax[1],ay[1]+1);
	global.status->setText(z,15);
	goto sendmove_cleanup;	
      }      
    position.moveCartesian(ax[0],ay[0],ax[1],ay[1],REGULAR,true);
    mygame->sendMove(ax[0],ay[0],ax[1],ay[1]);
    allowMove=false;
  } else if (global.Premove) {
    premoveq=1;
    pmx[0]=ax[0];
    pmy[0]=ay[0];
    pmx[1]=ax[1];
    pmy[1]=ay[1];
    pmp=EMPTY;
  }
 sendmove_cleanup:
  sp=0;
  hilite_ch=1;
  repaint();
}

void Board::setCanMove(bool value) {
  bool oldv;
  global.debug("Board","setCanMove");

  oldv=allowMove;
  allowMove=value;

  // commit premove if any
  if ((mygame)&&(global.Premove)&&(!oldv)&&(value)&&(premoveq)) {
    repaint();

    if (pmp==EMPTY) {
      if (position.isMoveLegalCartesian(pmx[0],pmy[0],pmx[1],pmy[1],
					mygame->MyColor,mygame->Variant)) {
	position.moveCartesian(pmx[0],pmy[0],pmx[1],pmy[1],REGULAR,true);
	mygame->sendMove(pmx[0],pmy[0],pmx[1],pmy[1]);
      }
    } else {
      if (position.isDropLegal(pmp,pmx[1],pmy[1],
			       mygame->MyColor,mygame->Variant)) {
	if (pmp&COLOR_MASK == 0) pmp|=mygame->MyColor;
	position.moveDrop(pmp, pmx[1], pmy[1]);
	mygame->sendDrop(pmp, pmx[1], pmy[1]);
      }
    }
    premoveq=0;
    sp=0;
    hilite_ch=1;
    repaint();
  }
}

void Board::stopClock() {
  clock_ch=1;
  clock.setClock2(clock.getValue2(0),clock.getValue2(1),0,1);
  queueRepaint();
}

void Board::openMovelist() {
  global.debug("Board","openMovelist");
  if (!mygame)
    return;
  mygame->openMoveList();
}

void Board::walkBack1() {
  if (!mygame)
    return;
  freeze();
  mygame->goBack1();
  position= mygame->getCurrentPosition();
  setInfo(2,position.getLastMove());
  setInfo(4,position.getMaterialString(mygame->Variant));
  setInfo(5,position.getHouseString());
  queueRepaint();
  thaw();
}

void Board::walkBackAll() {
  if (!mygame)
    return;
  freeze();
  mygame->goBackAll();
  position= mygame->getCurrentPosition();
  setInfo(2,position.getLastMove());
  setInfo(4,position.getMaterialString(mygame->Variant));
  setInfo(5,position.getHouseString());
  queueRepaint();
  thaw();
}

void Board::walkForward1() {
  if (!mygame)
    return;
  freeze();
  mygame->goForward1();
  position= mygame->getCurrentPosition();
  setInfo(2,position.getLastMove());
  setInfo(4,position.getMaterialString(mygame->Variant));
  setInfo(5,position.getHouseString());
  queueRepaint();
  thaw();
}

void Board::walkForwardAll() {
  if (!mygame)
    return;
  freeze();
  mygame->goForwardAll();
  position=mygame->getCurrentPosition();
  setInfo(2,position.getLastMove());
  setInfo(4,position.getMaterialString(mygame->Variant));
  setInfo(5,position.getHouseString());
  queueRepaint();
  thaw();
}

void Board::drawJoystickCursor(GdkGC *gc) {
  double arrow[7][2] =
    { { 0, 0}, {0,90}, {18,68}, {40,108}, 
      { 52,100 }, {30, 63}, {63, 63} };
  GdkPoint myarrow[7];
  int i;

  if (jsx >= 0) {

    for(i=0;i<7;i++) {
      myarrow[i].x = (gint) (4 + jsx + arrow[i][0] * sqside / 128.0);
      myarrow[i].y = (gint) (4 + jsy + arrow[i][1] * sqside / 128.0);
    }

    gdk_rgb_gc_set_foreground(gc,0x000000);
    gdk_draw_polygon(yidget->window,gc,TRUE,myarrow,7);
    for(i=0;i<7;i++) { myarrow[i].x -= 4; myarrow[i].y -= 4; }
    gdk_rgb_gc_set_foreground(gc,0x00ff00);
    gdk_draw_polygon(yidget->window,gc,TRUE,myarrow,7);
    gdk_rgb_gc_set_foreground(gc,0x000000);
    gdk_draw_polygon(yidget->window,gc,FALSE,myarrow,7);

  }
}

void Board::outlineRectangle(GdkGC *gc, int x,int y,int color,int pen) {
  int i,j,k,X,Y,S;
  gdk_rgb_gc_set_foreground(gc,color);
  j=x; k=y; if (effectiveFlip()) j=7-j; else k=7-k;
  if ((global.UseVectorPieces)||(!cur->extruded))
    for(i=0;i<pen;i++)
      gdk_draw_rectangle(yidget->window,gc,FALSE,borx+j*sqside+i,
			 morey+bory+k*sqside+i,
			 sqside-2*i,sqside-2*i);
  else {
    X=borx+j*sqside; Y=morey+bory+k*sqside; S=sqside;
    gdk_draw_rectangle(yidget->window,gc,TRUE,X,Y,pen,sqside);
    gdk_draw_rectangle(yidget->window,gc,TRUE,X+S-pen,Y,pen,sqside);
    gdk_draw_rectangle(yidget->window,gc,TRUE,X,Y,3*pen,pen);
    gdk_draw_rectangle(yidget->window,gc,TRUE,X+S-3*pen,Y,3*pen,pen);
    gdk_draw_rectangle(yidget->window,gc,TRUE,X,Y+S-pen,3*pen,pen);
    gdk_draw_rectangle(yidget->window,gc,TRUE,X+S-3*pen,Y+S-pen,3*pen,pen);
  }
      
}

void Board::drawBall(GdkGC *gc, int x,int y,int color,int radius) {
  int j,k;
  j=x; k=y; if (effectiveFlip()) j=7-j; else k=7-k;
  gdk_rgb_gc_set_foreground(gc,color);
  gdk_draw_arc(yidget->window,gc,FALSE,
	       borx+j*sqside+sqside/2-radius,
	       morey+bory+k*sqside+sqside/2-radius,
	       radius*2, radius*2, 0, 360*64);
}

void Board::drop(piece p) {
  global.debug("Board","drop");
  if (!mygame) return;
  if (p&COLOR_MASK == 0) p|=mygame->MyColor;
  if ((!allowMove)&&(!FreeMove)) {
    if (global.Premove) {
      premoveq=1;
      pmx[0]=-1;
      pmx[1]=dropsq[0];
      pmy[1]=dropsq[1];
      pmp=p;
    } else
      return;
  } else {
    if ((global.CheckLegality)&&(!FreeMove))
      if (!position.isDropLegal(p&PIECE_MASK, dropsq[0], dropsq[1],
				mygame->MyColor,mygame->Variant))
	{
	  char z[128];
	  snprintf(z,128,_("Illegal Drop on %c%d (Legality Checking On)"),
		   'a'+dropsq[0],dropsq[1]+1);
	  global.status->setText(z,15);
	  goto drop_cleanup;
	}
    position.moveDrop(p, dropsq[0], dropsq[1]);
    mygame->sendDrop(p, dropsq[0], dropsq[1]);
  }

 drop_cleanup:
  sp=0;
  hilite_ch=1;
  repaint();
}

// for crazyhouse/bughouse
void Board::popupDropMenu(int col,int row,int sx,int sy,guint32 etime) {
  GtkWidget *popmenu;
  GtkWidget *mi[7];
  Position pos;
  int i;
  char z[64];

  // sanity checks
  if (!allowMove)       return;
  if (!mygame)          return;
  if (mygame->isOver()) return;
  if ( (mygame->Variant != CRAZYHOUSE)&&(mygame->Variant != BUGHOUSE) )
    return;

  dropsq[0]=col;
  dropsq[1]=row;

  popmenu=gtk_menu_new();

  pos=mygame->getLastPosition();
  
  mi[0]=gtk_menu_item_new_with_label(_("Drop Piece:"));
  gtk_widget_set_sensitive(mi[0],FALSE);
  mi[1]=gtk_separator_menu_item_new();

  snprintf(z,64,_("Pawn    %d"),i=pos.getStockCount(PAWN|mygame->MyColor));
  mi[2]=gtk_menu_item_new_with_label(z);
  if (!i) gtk_widget_set_sensitive(mi[2],FALSE);

  snprintf(z,64,_("Rook    %d"),i=pos.getStockCount(ROOK|mygame->MyColor));
  mi[3]=gtk_menu_item_new_with_label(z);
  if (!i) gtk_widget_set_sensitive(mi[3],FALSE);

  snprintf(z,64,_("Knight  %d"),i=pos.getStockCount(KNIGHT|mygame->MyColor));
  mi[4]=gtk_menu_item_new_with_label(z);
  if (!i) gtk_widget_set_sensitive(mi[4],FALSE);

  snprintf(z,64,_("Bishop  %d"),i=pos.getStockCount(BISHOP|mygame->MyColor));
  mi[5]=gtk_menu_item_new_with_label(z);
  if (!i) gtk_widget_set_sensitive(mi[5],FALSE);

  snprintf(z,64,_("Queen   %d"),i=pos.getStockCount(QUEEN|mygame->MyColor));
  mi[6]=gtk_menu_item_new_with_label(z);
  if (!i) gtk_widget_set_sensitive(mi[6],FALSE);

  for(i=0;i<7;i++) {
    gshow(mi[i]);
    gtk_menu_shell_append(GTK_MENU_SHELL(popmenu),mi[i]);
  }

  gtk_signal_connect(GTK_OBJECT(mi[2]),"activate",
		     GTK_SIGNAL_FUNC(drop_callbackP),
		     (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(mi[3]),"activate",
		     GTK_SIGNAL_FUNC(drop_callbackR),
		     (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(mi[4]),"activate",
		     GTK_SIGNAL_FUNC(drop_callbackN),
		     (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(mi[5]),"activate",
		     GTK_SIGNAL_FUNC(drop_callbackB),
		     (gpointer)this);
  gtk_signal_connect(GTK_OBJECT(mi[6]),"activate",
		     GTK_SIGNAL_FUNC(drop_callbackQ),
		     (gpointer)this);

  gtk_menu_popup(GTK_MENU(popmenu),0,0,0,0,3,etime);
}

void Board::popupProtocolMenu(int x,int y, guint32 etime) {
  GtkWidget *mi, *popmenu;
  vector<string *> *v;
  char z[64];
  unsigned int i;
  
  if (!global.protocol) return;
  if (!mygame) return;
  if (global.protocol->getPlayerActions() == NULL) return;

  popmenu=gtk_menu_new();
  PopupOwner = this;

  // players
  v=global.protocol->getPlayerActions();

  for(i=0;i<v->size();i++) {
    snprintf(z,64,"%s: %s",mygame->PlayerName[0], ((*v)[i])->c_str() );
    mi=gtk_menu_item_new_with_label(z);
    gshow(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(popmenu),mi);

    gtk_signal_connect(GTK_OBJECT(mi),"activate",
		       GTK_SIGNAL_FUNC(menu_whitep),
		       (gpointer) ( (*v)[i] ) );    
  }

  mi=gtk_separator_menu_item_new();
  gshow(mi);
  gtk_menu_shell_append(GTK_MENU_SHELL(popmenu),mi);

  for(i=0;i<v->size();i++) {
    snprintf(z,64,"%s: %s",mygame->PlayerName[1], ((*v)[i])->c_str() );
    mi=gtk_menu_item_new_with_label(z);
    gshow(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(popmenu),mi);

    gtk_signal_connect(GTK_OBJECT(mi),"activate",
		       GTK_SIGNAL_FUNC(menu_blackp),
		       (gpointer) ( (*v)[i] ) );
  }

  if (global.protocol->getGameActions() != 0) {
    mi=gtk_separator_menu_item_new();
    gshow(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(popmenu),mi);

    v=global.protocol->getGameActions();

    for(i=0;i<v->size();i++) {
      snprintf(z,64,_("Game #%d: %s"),mygame->GameNumber, ((*v)[i])->c_str() );
      mi=gtk_menu_item_new_with_label(z);
      gshow(mi);
      gtk_menu_shell_append(GTK_MENU_SHELL(popmenu),mi);
      
      gtk_signal_connect(GTK_OBJECT(mi),"activate",
			 GTK_SIGNAL_FUNC(menu_gamep),
			 (gpointer) ( (*v)[i] ) );
    }
  }

#if MAEMO
  gtk_menu_popup(GTK_MENU(popmenu),0,0,0,0,1,etime);
#else   
  gtk_menu_popup(GTK_MENU(popmenu),0,0,0,0,3,etime);
#endif
}

gboolean board_joycursor(gpointer data) {
  Board *me;
  static double accel[5] = { 0.2, 0.4, 0.8, 1.2, 1.5 };
  static int acc = 0;
  static Timestamp prev;
  Timestamp cur;
  double elapsed;

  me = (Board *) data;
  cur = Timestamp::now();

  if (me->jsx < 0) {
    me->jsx = me->sqw / 2;
    me->jsy = me->sqh / 2;
    acc = 0;
  } else {
    elapsed = cur-prev;
    if (elapsed < 0.50 && me->jsnt==0) {
      me->jsx += (int) (accel[acc] * global.JSSpeed * 5.0 * (me->jsvx / 32768.0) * (elapsed/0.060));
      me->jsy += (int) (accel[acc] * global.JSSpeed * 5.0 * (me->jsvy / 32768.0) * (elapsed/0.060));
      if (me->jsx < 0)       me->jsx = 0;
      if (me->jsx > me->sqw) me->jsx = me->sqw;
      if (me->jsy < 0)       me->jsy = 0;
      if (me->jsy > me->sqh) me->jsy = me->sqh;
      ++acc;
      if (acc>4) acc=4;
      me->queueRepaint();
    }
  }

  if (me->jsnt!=0) acc=0; 
  me->jsnt = 0;
  prev = cur;
  if (global.JSMode!=1) {
    me->jstid = -1;
    return FALSE;
  } else return TRUE;
}

void Board::joystickCursor(int axis, int value) {

  // continuous cursor
  if (global.JSMode == 1) {
    if (axis==0) jsvx = value;
    if (axis==1) jsvy = value;
    if (jstid<0 && (jsvx!=0 || jsvy!=0)) {
      jsnt = 1;
      jstid = gtk_timeout_add(60,board_joycursor,(gpointer) this);
    } else if (jstid>=0 && (jsvx==0 && jsvy==0)) {
      gtk_timeout_remove(jstid);
      jstid = -1;
    }
  }

  if (effectiveFlip()) value = -value;   
  if (value < 0) value = -1;
  if (value > 0) value = 1;
  

  if (jx < 0) {
    jx = 3; jy = 3; queueRepaint();
  } else {

    switch(axis) {
    case 0: // X
      if (jpxv != value) {
	if (value < 0) jx--; if (jx<0) jx=0;
	if (value > 0) jx++; if (jx>7) jx=7;
	jpxv = value;
      }
      break;
    case 1: // Y
      if (jpyv != value) {
	if (value < 0) jy++; if (jy>7) jy=7;
	if (value > 0) jy--; if (jy<0) jy=0;
	jpyv = value;
      }
      break;
    }
    queueRepaint();
  }

}

void Board::joystickSelect() {

  int x,y;

  if (global.JSMode == 0) {
    x = jx;
    y = jy;
    
    if (sp > 1) sp=0;
    if ( (sp == 1) && (x==ax[0]) && (y==ay[0]) )
      sp=0;
    else {
      
      if ((sp == 1) &&
	  ( (position.getPiece(ax[0],ay[0])&COLOR_MASK) ==
	    (position.getPiece(x,y)&COLOR_MASK)) &&
	  (position.getPiece(x,y)!=EMPTY) && (allowMove) )
	sp=0;
      
      ax[sp]=x; ay[sp]=y;
      ++sp;
    }
    
    if ((sp==2)&&(mygame)) {
      sendMove();
    } else {
      premoveq=0;
      hilite_ch=1;
      repaint();
    }
  } else {
    GdkEventButton be;
    be.x = jsx;
    be.y = jsy;
    be.button = 1;
    be.state = GDK_MOD5_MASK;
    board_button_press_event(widget,&be,this);
    board_button_release_event(widget,&be,this);
  }
}

void Board::dump() {
  cerr.setf(ios::hex,ios::basefield);
  cerr.setf(ios::showbase);

  cerr << "[board " << ((uint64_t) this) << "] ";
  cerr << "game=[" << ((uint64_t) mygame) << "] ";

  cerr.setf(ios::dec,ios::basefield);
  cerr << "paneid=" << getPageId() << endl;
}

/* callbacks */

void drop_callbackP(GtkMenuItem *item,gpointer data) {
  Board *b; b=(Board *)data; b->drop(PAWN);
}

void drop_callbackR(GtkMenuItem *item,gpointer data) {
  Board *b; b=(Board *)data; b->drop(ROOK);
}

void drop_callbackN(GtkMenuItem *item,gpointer data) {
  Board *b; b=(Board *)data; b->drop(KNIGHT);
}

void drop_callbackB(GtkMenuItem *item,gpointer data) {
  Board *b; b=(Board *)data; b->drop(BISHOP);
}

void drop_callbackQ(GtkMenuItem *item,gpointer data) {
  Board *b; b=(Board *)data; b->drop(QUEEN);
}

void menu_whitep(GtkMenuItem *item, gpointer data) {
  Board *me;
  me=Board::PopupOwner;
  Board::PopupOwner = 0;
  if (!me) return;
  if (!me->mygame) return;
  if (!global.protocol) return;  
  global.protocol->callPlayerAction(me->mygame->PlayerName[0],(string *) data);
}

void menu_blackp(GtkMenuItem *item, gpointer data) {
  Board *me;
  me=Board::PopupOwner;
  Board::PopupOwner = 0;
  if (!me) return;
  if (!me->mygame) return;
  if (!global.protocol) return;  
  global.protocol->callPlayerAction(me->mygame->PlayerName[1],(string *) data);
}

void menu_gamep(GtkMenuItem *item, gpointer data) {
  Board *me;
  me=Board::PopupOwner;
  Board::PopupOwner = 0;
  if (!me) return;
  if (!me->mygame) return;
  if (!global.protocol) return;  
  global.protocol->callGameAction(me->mygame->GameNumber,(string *) data);
}

void Board::drawCoordinates(GdkPixmap *dest,GdkGC *gc,int side) {
  bool ef;
  int i;
  char z[2];
  PangoLayout *pl;
  PangoFontDescription *pfd;

  z[1]=0;
  ef=effectiveFlip();

  if ((borx)||(bory)) {
    gdk_rgb_gc_set_foreground(gc,0);
    gdk_draw_rectangle(dest,gc,TRUE,0,0,Width(),bory);
    gdk_draw_rectangle(dest,gc,TRUE,0,Height()-bory,Width(),bory);
    gdk_draw_rectangle(dest,gc,TRUE,0,0,borx,Height());
    gdk_draw_rectangle(dest,gc,TRUE,Width()-borx,0,borx,Height());
  }

  gdk_rgb_gc_set_foreground(gc,0x404040);
  gdk_draw_rectangle(dest,gc,TRUE,borx,2,side*8,bory-4);
  gdk_draw_rectangle(dest,gc,TRUE,borx,2+side*8+bory+morey,side*8,bory-4);
  gdk_draw_rectangle(dest,gc,TRUE,2,bory+1,borx-4,morey+side*8-1);
  gdk_draw_rectangle(dest,gc,TRUE,2+side*8+borx,bory+1,borx-4,morey+side*8-1);

  gdk_rgb_gc_set_foreground(gc,0xffffff);

  pl = gtk_widget_create_pango_layout(widget, NULL);
  pfd = pango_font_description_from_string(global.InfoFont);
  pango_layout_set_font_description(pl, pfd);

  for(i=0;i<8;i++) {
    z[0]=ef?'h'-i:'a'+i;

    pango_layout_set_text(pl,z,-1);
    gdk_draw_layout(dest,gc,borx+side*i+side/2, 2,pl);
    gdk_draw_layout(dest,gc,borx+side*i+side/2, morey+8*side+bory,pl);
    z[0]=ef?'1'+i:'8'-i;
    pango_layout_set_text(pl,z,-1);

    gdk_draw_layout(dest,gc, 6, bory+morey+side*i+side/2, pl);
    gdk_draw_layout(dest,gc, side*8+borx+6, bory+morey+side*i+side/2, pl);
  }

  pango_font_description_free(pfd);
  g_object_unref(G_OBJECT(pl));
}

void Board::composeVecBoard(GdkGC *gc) {
  Position *joker;
  int i,j;
  global.vpieces.drawSquares(buffer,gc,sqside, borx, bory);

  if (gotAnimationLoop) joker=&fake; else joker=&position;
  if (effectiveFlip())
    for(i=0;i<8;i++)
      for(j=0;j<8;j++)
	global.vpieces.drawPiece(buffer,gc,sqside,borx+i*sqside,bory+j*sqside,
				 joker->getPiece(7-i,j));
  else
    for(i=0;i<8;i++)
      for(j=0;j<8;j++)
	global.vpieces.drawPiece(buffer,gc,sqside,borx+i*sqside,bory+j*sqside,
				 joker->getPiece(i,7-j));

  if (global.ShowCoordinates)
    drawCoordinates(buffer,gc,sqside);

  i=Width();
  gdk_rgb_gc_set_foreground(gc,0);
  gdk_draw_rectangle(buffer,gc,TRUE,0,i,i,sqh-Height());

  currently_rendered=*(joker);
}

void Board::pasteVecBoard() {
  gdk_draw_pixmap(yidget->window,
		  yidget->style->fg_gc[GTK_WIDGET_STATE (yidget)],
		  buffer,
		  0, 0,
		  0, 0,Width(),Height());
}

/* ******************************************************************
   board_expose_event

   called on: repaints (both generated by the program and by the
                        windowing system).

   description: 
   paints straight on window. paints external borders
   black if window size requires it.

   Board:
   If using vectorized pieces, calls [composeVecBoard] to update
   the [buffer] variable.
   Paints [buffer] (board) on window.

   Clock:
   If [clock_ch] is set, call renderClock.
   Draws [clkbar] on window.

   Highlights:
   Paints highlights, selection, premove indicators straight
   to window.

   Dragging:
   If dragging, call [board_motion_event] to update the
   dragging buffers.

   ********************************************************** */
gboolean board_expose_event(GtkWidget *widget,GdkEventExpose *ee,
                            gpointer data) {
  Board *me;
  GdkGC *gc;
  unsigned int i;
  int w,h,sq,pw,fw,fh;

  global.debug("Board","board_expose_event");

  me=(Board *)data;
  w=ee->area.width;
  h=ee->area.height;
  fw=ee->area.x+ee->area.width;
  fh=ee->area.y+ee->area.height;

  sq=me->sqside;

  if (!me->wgc)
    me->wgc=gdk_gc_new(widget->window);
  gc=me->wgc;

  if (fw> ( me->Width() + 250) )
    gdk_draw_rectangle(widget->window,widget->style->black_gc,TRUE,
		       me->Width()+250,0,fw-me->Width()-250,fh);

  if (fh > me->sqh) {
    gdk_draw_rectangle(widget->window,widget->style->black_gc,TRUE,
		       0,me->sqh,fw,fh-me->sqh);
    fh=me->sqh;
  }

  if (fw > me->sqw)
    fw=me->sqw;
  
  pw=fw;
  if (fw> me->Width() ) pw=me->Width();

  // draw board+pieces

  if (global.UseVectorPieces)
    me->composeVecBoard(gc);

  gdk_draw_pixmap(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  me->buffer,
		  0, 0,
		  0, 0,pw,fh);

  // draw clock

  if (me->clock_ch)
    me->renderClock();
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  me->clkbar,
                  0, 0,
                  me->Width(), 0,250,fh);

  // draw highlighted squares

  if (!me->gotAnimationLoop)
    if ((me->sp)||(!me->LastMove.empty())||(me->premoveq)||(me->jx>=0)) {
    
      if (!me->EditMode)
	if ((global.HilightLastMove)&&(!me->LastMove.empty()))
	  for(i=0;i<me->LastMove.size();i++) {
	    me->outlineRectangle(gc,me->LastMove[i]->SX,me->LastMove[i]->SY,
				 0x0000ff,3);
	    me->outlineRectangle(gc,me->LastMove[i]->DX,me->LastMove[i]->DY,
				 0x0000ff,3);
	}
      
      if (me->sp)
	for(i=0;i < (unsigned int) (me->sp);i++)
	  me->outlineRectangle(gc,me->ax[i],me->ay[i],me->sel_color,4);
      
      if (me->premoveq) {
	me->outlineRectangle(gc,me->pmx[1],me->pmy[1],0xff0000,3);
	me->drawBall(gc,me->pmx[1],me->pmy[1],0xff0000,5);
	if (me->pmx[0]>=0) {
	  me->outlineRectangle(gc,me->pmx[0],me->pmy[0],0xff0000,3);
	  me->drawBall(gc,me->pmx[0],me->pmy[0],0xff0000,5);
	}
      }

      if (global.JSMode==0 && me->jx >= 0)
	me->outlineRectangle(gc,me->jx,me->jy,0x00ff00,8);

      if (global.JSMode==1)
	me->drawJoystickCursor(gc);
    }
  
  /* if dragging... */
  if (me->dr_active) {
    me->dr_step=0;
    board_motion_event(widget,&LastMotionEvent,data);
  }

  return 1;
}

/* ******************************************************************
   board_configure_event

   called on: resizes, piece changes, program-generated repaints.

   Updates the [buffer] pixmap.
   
   Description:

   Square size/Pieces:
   Recalculates square size. Resizes piece set [Board::cur] if needed.

   Return immediately if dealing with vectorized pieces.

   Compose [buffer]:
   Render position to [scrap]. If animating a move, renders
   [Board::fake], else renders [Board::position].

   Copy [scrap] to [buffer]. (X programming gizmo, scrap is an image,
   buffer is a pixmap, and bitmap pieces can't be rendered on
   pixmaps directly)

   Coordinates:
   Draw coordinates to [buffer] if global.ShowCoordinates set.

   updates [currently_rendered] to reflect the current situation.
   
   ************************************************************* */
gboolean board_configure_event(GtkWidget *widget,GdkEventConfigure *ce,
			       gpointer data) {
  Board *me;
  int optimal_square,sq;
  int ww,wh,i,j;
  int oldsqw, oldsqh;
  Position *joker;

  me=(Board *)data;

  global.debug("Board","board_configure_event");

  gdk_window_get_size(widget->window,&ww,&wh);
  ww-=200;

  if ( (ww-2*me->borx) > (wh-2*me->bory) ) {
    if ( (global.UseVectorPieces) || (! global.pieceset->extruded) ) {
      optimal_square=(wh - 2 * me->bory)/8;
      me->morey=0;
    } else {
      optimal_square=(int) ((wh - 2 * me->bory)/8.50);
      me->morey = optimal_square / 2;
    }
  } else {
    optimal_square=(ww - 2 * me->borx)/8;
    if ( (global.UseVectorPieces) || (! global.pieceset->extruded) )
      me->morey = 0;
    else
      me->morey = optimal_square / 2;
  }

  if (optimal_square < 10) return FALSE;

  if (me->cur == 0)
    me->cur=new PieceSet(global.pieceset);
  if (me->cur->side != optimal_square) {
    if (me->cur->side != global.pieceset->side) {
      delete me->cur;
      me->cur=new PieceSet(global.pieceset);
    }      
    me->cur->scale(optimal_square);
    me->currently_rendered.invalidate();
  }
  sq=optimal_square;
  if (sq!=me->sqside) {
    me->currently_rendered.invalidate();
    me->clock_ch=1;
  }

  oldsqw=me->sqw;
  oldsqh=me->sqh;
  me->sqside=sq;
  me->sqw = 2*me->borx + 8 * me->sqside + 1;
  me->sqh = 2*me->bory + me->morey + 8 * me->sqside + 1;

  if ((me->currently_rendered == me->position)&&(!me->update_queue)) {
    return TRUE;
  }

  if (oldsqw!= me->sqw || oldsqh != me->sqh) {
    gdk_pixmap_unref(me->clkbar);
    gdk_pixmap_unref(me->buffer);
    if (me->clkgc) { gdk_gc_destroy(me->clkgc); me->clkgc=0; }
    if (me->scrap) { g_free(me->scrap); me->scrap=0; }

    me->buffer=gdk_pixmap_new(MainWindow::RefWindow,
			      me->sqw,me->sqh,-1);
    me->clkbar=gdk_pixmap_new(MainWindow::RefWindow,
			      250,me->sqh,-1);
    me->clkgc=gdk_gc_new(me->clkbar);
    me->scrap=(rgbptr)g_malloc(me->sqw * me->sqh * 3);
  }

  memset(me->scrap,0,me->sqw * me->sqh * 3);

  if (me->UpdateClockAfterConfigure) {
    me->updateClock();
    me->UpdateClockAfterConfigure = false;
  }

  if (global.UseVectorPieces)
    return TRUE;

  if (me->gotAnimationLoop)
    joker=&(me->fake);
  else
    joker=&(me->position);

  me->cur->beginQueueing();

  if (me->effectiveFlip())
    for(i=0;i<8;i++)
      for(j=0;j<8;j++)
	me->cur->drawPieceAndSquare(joker->getPiece(7-i,j),
				    me->scrap,me->borx+i*sq,
				    me->morey+me->bory+j*sq,me->sqw,
				    (i+j)%2);
  else
    for(i=0;i<8;i++)
      for(j=0;j<8;j++)
	me->cur->drawPieceAndSquare(joker->getPiece(i,7-j),
				    me->scrap,me->borx+i*sq,
				    me->morey+me->bory+j*sq,me->sqw,
				    (i+j)%2);

  me->cur->endQueueing();
  // and doing without curly braces just adds to the general perplexity

  // copy scrap to pixmap buffer
  gdk_draw_rgb_image(me->buffer,widget->style->black_gc,0,0,
		     me->sqw, me->sqh,
		     GDK_RGB_DITHER_NORMAL,me->scrap,me->sqw*3);

  if (global.ShowCoordinates) {
    if (!me->wgc)
      me->wgc=gdk_gc_new(widget->window);
    me->drawCoordinates(me->buffer,me->wgc,sq);
  }

  me->currently_rendered=*(joker);

  return TRUE;
}

/*
static bool not_disjoint_intervals(int a, int b, int c, int d) {
  return( (c<=b) && (d>=a) );
}

static bool rects_overlap(int x1,int y1,int w1,int h1,
			  int x2,int y2,int w2,int h2) {
  return( not_disjoint_intervals(x1,x1+w1, x2, x2+w2) &&
	  not_disjoint_intervals(y1,y1+h1, y2, y2+h2) );
}
*/

static GdkPixmap *GCPbuffer=0;
static int gcpw=0,gcph=0;

/* ***************************************************************
   board_motion_event

   called on: mouse moved during drag, also called by 
              [board_expose_event]

   depends on all Board::dr_* fields.
   uses [GCPbuffer] (static to this module) as drawing buffer.

   Startup Sanity Checking:
   Return immediately if the after-click timeout hasn't
   expired [dr_fto], not dragging a piece [dr_active],
   no event [em], or dragging not with left mouse button.

   GCPbuffer allocation:
   If current [GCPbuffer] is smaller than needed, dump the
   current one and allocate another.

   Step 0: Prepare background
   If first call on this drag ([dr_step] = 0), copy [buffer] to
   [GCPbuffer]

   Step 1: Erase previous dragging frame
   Only if not first call on this drag ([dr_step] != 0).
   copy square from [buffer] to [GCPbuffer],
   copy clock from [clkbar] to [GCPbuffer] if needed.
   Extruded sets: be sure to erase enough to cover [PieceSet::height]

   Step 2: Erase source square
   Only if dragging from the board (instead of zhouse stock,
   [dr_fromstock]).
   Vectorized pieces: draw square and coordinates directly to
    [GCPbuffer].
   Bitmapped pieces: allocate new image (M[0]), draw empty square on
    it, copy [M[0]] to [GCPbuffer].
   Extruded Bitmapped pieces: redraw piece on square right above the
    source square.

   Step 3: Draw dragged piece
   Vectorized pieces: just draw it on [GCPbuffer]
   Bitmap pieces: paint alpha'ed piece (more comments to come, experimental
    code)

   Step 3 1/2: Draw coordinates (bitmap-mode only)
   Redraw the coordinates (if global.ShowCoordinates set) on
   [GCPbuffer]

   Step 4: Commit
   Draw [GCPbuffer] on window, update Board::dr_* about what has
   been done, deallocate all local storage as needed.

   *************************************************************** */
gboolean board_motion_event(GtkWidget *widget,
			    GdkEventMotion *em,
			    gpointer data) {
  Board *me;
  GdkGC *gcpgc;
  int rw,rh;
  int ww,wh,sw,sh;
  int sq,he,xc,xr;
  int bw,bh;

  if (em!=NULL) {
    if (!GDK_IS_WINDOW(em->window))
      return TRUE;
    memcpy(&LastMotionEvent,em,sizeof(GdkEventMotion));
  }

  me=(Board *)data;

  if ((!me->dr_fto)||(!me->dr_active)||(em==NULL)||(!(em->state & GDK_BUTTON1_MASK)))
    return TRUE;

  sq=he=me->sqside;
  if ((me->cur->extruded)&&(!global.UseVectorPieces)) he=me->cur->height;

  // STEP 0: prepare double buffer for drawing

  gdk_window_get_size(widget->window,&ww,&wh);

  if (!me->wgc)
    me->wgc=gdk_gc_new(widget->window);
  gcpgc=me->wgc;
  gdk_rgb_gc_set_foreground(gcpgc,0);

  sw=ww; sh=wh;
  if (sw> me->sqw) sw=me->sqw;
  if (sh> me->sqh) sh=me->sqh;

  if ( (gcpw < ww)|| (gcph < wh) ) {
    gcpw=ww; gcph=wh;
    if (GCPbuffer) gdk_pixmap_unref(GCPbuffer);
    GCPbuffer=gdk_pixmap_new(widget->window, gcpw, gcph, -1);
    gdk_draw_rectangle(GCPbuffer,gcpgc,TRUE,0,0,gcpw,gcph);
    gdk_draw_pixmap(GCPbuffer,gcpgc,me->buffer,0,0,0,0,sw,sh);
  } else
    if (! me->dr_step) {
      gdk_draw_rectangle(GCPbuffer,gcpgc,TRUE,0,0,gcpw,gcph);
      gdk_draw_pixmap(GCPbuffer,gcpgc,me->buffer,0,0,0,0,sw,sh);
    }

  bw=me->Width();
  bh=me->Height();

  // STEP 1: erase dirty animation square

  if (me->dr_step) {
    rw= bw - me->dr_dirty[0];
    rh= bh - me->dr_dirty[1];
    if (rw > sq) rw = sq; 
    if (rh > he) rh = he;
 
    if ((rw>0)&&(rh>0))
      gdk_draw_pixmap(GCPbuffer,gcpgc,
		      me->buffer,
		      me->dr_dirty[0],me->dr_dirty[1],
		      me->dr_dirty[0],me->dr_dirty[1],
		      rw,rh);

    //    gdk_rgb_gc_set_foreground(gcpgc,0xffff00);
    gdk_draw_rectangle(GCPbuffer,gcpgc,TRUE,bw+250,0,ww-(bw+250),wh);
    gdk_draw_rectangle(GCPbuffer,gcpgc,TRUE,0,bh,bw+250,wh-bh);
    //    gdk_rgb_gc_set_foreground(gcpgc,0);

  }

  gdk_draw_pixmap(GCPbuffer,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  me->clkbar,
		  0, 0,
		  bw, 0,250, bh );
    
  // STEP 2: erase source square (if not dragging from zhouse stock)

  if (! me->dr_fromstock) {
    if (global.UseVectorPieces) {

      gdk_rgb_gc_set_foreground(gcpgc,((me->dr_c+me->dr_r)&0x01)?global.DarkSqColor:global.LightSqColor);
      gdk_draw_rectangle(GCPbuffer,gcpgc,TRUE,
			 me->borx + me->dr_c*sq, 
			 me->morey + me->bory + me->dr_r*sq,
			 sq,sq);
      gdk_rgb_gc_set_foreground(gcpgc,0);
      gdk_draw_rectangle(GCPbuffer,gcpgc,FALSE,
			 me->borx + me->dr_c*sq, 
			 me->morey + me->bory + me->dr_r*sq,
			 sq,sq);
    } else {
      
      // I only need *3, but glibc has some weird glitches when it
      // comes to allocating not padded to 32-bit words

      me->M[0].ensure(sq*he*4);
      me->cur->drawSquare( (me->dr_c+me->dr_r)&0x01, me->M[0].data , 0, 0, sq );
      gdk_draw_rgb_image(GCPbuffer,widget->style->black_gc,
			 me->borx + me->dr_c * sq, 
			 me->morey + me->bory + me->dr_r * sq,
			 sq, sq,
			 GDK_RGB_DITHER_NORMAL, me->M[0].data ,3 * sq);

      // erase upper portion of extruded piece
      if (me->cur->extruded) {
	if (me->dr_r > 0) { // there's a square above it
	  me->M[1].ensure(sq*sq*4);
	  xc = me->effectiveFlip() ? 7 - me->dr_c : me->dr_c;
	  xr = me->effectiveFlip() ? me->dr_r - 1 : 8 - me->dr_r ;
	  me->cur->drawPieceAndSquare( 
				      me->position.getPiece(xc, xr) ,
				      me->M[0].data, 0, 0, sq, ((xc+xr)%2 == 0) );
	  memcpy(me->M[1].data , me->M[0].data, sq*sq*3);

	  gdk_draw_rgb_image(GCPbuffer,widget->style->black_gc,
			     me->borx + me->dr_c * sq, 
			     me->morey + me->bory + (me->dr_r - 1) * sq,
			     sq, sq,
			     GDK_RGB_DITHER_NORMAL,me->M[0].data,3 * sq);
	} else { // there is nothing above it, just paint it black
	  gdk_draw_rectangle(GCPbuffer,widget->style->black_gc,TRUE,
			     me->borx + me->dr_c * sq,
			     me->bory, sq, me->morey);
	}
      }

    }
  }

  // STEP 3: draw dragged piece

  int dx,dy;
  int x,y;
  GdkModifierType state;

  if (em->is_hint){

    x=(int)(em->x);
    y=(int)(em->y);

    if (em->window != NULL) {
      if (GDK_IS_WINDOW(em->window))
	gdk_window_get_pointer(em->window, &x, &y, &state);
      else
	cout << "[ case 1: em->window is NOT a GDK_WINDOW ]\n";
    } else {
      if (me->widget->window==NULL) {
	cout << "[ case 3: both window pointers are NULL ]\n";
      } else if (GDK_IS_WINDOW(me->widget->window))
	gdk_window_get_pointer(me->widget->window, &x, &y, &state);
      else
	cout << "[ case 2: me->widget->window is NOT a GDK_WINDOW ]\n";
      cout << "[ And this is where it used to print a warning ]\n"; // FIXME
    }

  } else {
    x=(int)(em->x);
    y=(int)(em->y);
  }

  // prevent segfaults (bug sf#459164)
  if (x < 0) x=0;
  if (y < 0) y=0;
  if (x > ww) x=ww;
  if (y > wh) y=wh;

  dx = x - me->dr_ox;
  dy = y - me->dr_oy;

  // yet more segfault prevention
  if (dx < 0) { x-=dx; dx=0; }
  if (dy < 0) { y-=dy; dy=0; }

  if (global.UseVectorPieces) {
    global.vpieces.drawPiece((GdkPixmap *)(GCPbuffer),
			     gcpgc,sq,dx,dy,me->dr_p);    
    xr=dy;
    //  } else if (dx+sq > me->Width() ) {
  } else {
    // segment-by-segment masked render, piece overlaps clock area

    if (me->cur->extruded) {
      xr=dy-he+sq; xc=he-sq;
      if (xr < 0) { xc+=xr; xr=0; }
      if (xc > me->morey) { --xc; ++xr; }
    } else { xc=0; xr=dy; }

    me->cur->drawPiece(me->dr_p,GCPbuffer,gcpgc,dx,dy);
  }

  // STEP 3 1/2: draw coordinates if not in vector mode
  if ((!global.UseVectorPieces)&&(global.ShowCoordinates))
    me->drawCoordinates(GCPbuffer,gcpgc,sq);

  // STEP 4: commit double buffer to window

  gdk_draw_pixmap(widget->window,gcpgc,GCPbuffer,0,0,0,0,ww,wh);

  me->dr_dirty[0]=dx;
  me->dr_dirty[1]=xr;
  me->dr_step++;
  
  return TRUE;
}

gboolean board_button_release_event(GtkWidget *widget,
				    GdkEventButton *be,
				    gpointer data) {
  Board *me;
  int x,y;
  bool must_repaint=false;
  bool drop_from_drag=false;

  if (be==0) return 0;
  me=(Board *)data;

  if ((be->button==1)&&(me->dr_active)) {
    if (me->dr_step)
      must_repaint=true;
    me->dr_active=false;

    if (me->dr_fromstock)
      drop_from_drag=true;
  }

  if ( (be->x < me->borx) || (be->y < me->morey + me->bory ) )
    goto b_rele_nothing;

  x=((int)(be->x)-me->borx)/me->sqside;
  y=((int)(be->y)-me->bory-me->morey)/me->sqside;  

  if ((x>7)||(y>7)) goto b_rele_nothing;
  if (me->effectiveFlip()) x=7-x; else y=7-y;
  //
  if (be->button==1) {

    // piece dragged from stock
    if (drop_from_drag) {
      me->dropsq[0]=x;
      me->dropsq[1]=y;
      if (me->mygame) {
	me->drop(me->dr_p);
	return 1;
      }
    }

    if ( (me->sp==1) && ( (x!=me->ax[0]) || (y!=me->ay[0]) ) ) {
      me->ax[me->sp]=x; me->ay[me->sp]=y;
      me->sp=2;      
      if (me->mygame)
	me->sendMove();
      else {
	me->sp=0;
	me->hilite_ch=1;
	me->repaint();
      }
      return 1;
    }
  }

 b_rele_nothing:
  if (must_repaint) {
    me->clock_ch=1;
    me->hilite_ch=1;
    me->repaint();
  }
  return 1;
}

gboolean board_button_press_event(GtkWidget *widget,GdkEventButton *be,
			    gpointer data) {
  Board *me;
  int x,y;
  me=(Board *)data;
  if (be==NULL) return 0;
  if ((!me->canselect)&&(be->button!=3)) return 1;

  if ( (be->x < me->borx) || (be->y < me->bory + me->morey ) ) return 1;

  x=((int)(be->x)- me->borx)/me->sqside;
  y=((int)(be->y)- me->bory - me->morey )/me->sqside;

  if ((x<8)&&(y<8)) {
    me->dr_ox= ((int)(be->x) - me->borx ) % me->sqside;
    me->dr_oy= ((int)(be->y) - me->bory - me->morey ) % me->sqside;
    me->dr_c=x;
    me->dr_r=y;
  }

  if ((x>7)||(y>7)) {
    DropSource *ds;
    if (be->button == 1) {
      ds=me->hitTarget((int)(be->x),(int)(be->y));      
      if (ds!=NULL) {
	if (ds->dragged) {
	  me->dr_active=true;
	  me->dr_fto=true;
	  me->dr_fromstock=true;
	  me->dr_step=0;
	  me->dr_p=ds->P;
	  me->dr_ox = me->sqside / 2;
	  me->dr_oy = me->sqside / 2;
	  me->dr_c=0; // non-sense
	  me->dr_r=0;	  
	} else {
	  EditBoard *eme;
	  eme=(EditBoard *)data;

	  // empty / start pos buttons
	  if ((me->EditMode)&&(me->mygame)) {
	    switch(ds->P) {
	    case EMPTY: me->mygame->editEmpty(); break;
	    case WHITE|BLACK: me->mygame->editStartPos(); break;
	    case WHITE: eme->popRunEngine(ds->X,ds->Y2); break;
	    case BLACK: eme->flipSide(); break;
	    case WASPAWN: eme->getFEN(); break;
	    }
	  }
	}
      }
    }
    if (be->button == 3) {
      me->popupProtocolMenu( (int) be->x, (int) be->y, be->time );
    }
    return 1;
  }
  
  if (me->effectiveFlip()) x=7-x; else y=7-y;  

  if (be->button == 1) {

    if (be->state != GDK_MOD5_MASK) {
      me->jx = -1;
      me->jy = -1;
      me->jpxv = 0;
      me->jpyv = 0;
      me->jsx = -1;
      me->jsy = -1;
    }

    me->dr_active=true;
    me->dr_fto=true;
    me->dr_fromstock=false;
    me->dr_step=0;
    me->dr_p=me->position.getPiece(x,y);
    if (me->dr_p == EMPTY)
      me->dr_active=false;

    if (me->sp > 1) me->sp=0;
    if ( (me->sp == 1) && (x==me->ax[0]) && (y==me->ay[0]) )
      me->sp=0;
    else {

      if ((me->sp == 1) &&
	  ( (me->position.getPiece(me->ax[0],me->ay[0])&COLOR_MASK) ==
	    (me->position.getPiece(x,y)&COLOR_MASK)) &&
	  (me->position.getPiece(x,y)!=EMPTY) && (me->allowMove) )
	me->sp=0;
      
      me->ax[me->sp]=x; me->ay[me->sp]=y;
      ++(me->sp);
    }

    if ((me->sp==2)&&(me->mygame)) {
      me->sendMove();
    } else {
      me->premoveq=0;
      me->hilite_ch=1;
      me->repaint();
    }
  }
  if ((be->button == 3)&&(me->canselect)) {
    me->popupDropMenu(x,y,0,0,be->time);
  }

  return 1;
}

gboolean vec_board_animate(gpointer data) {
  list<AnimatedPiece *>::iterator li;
  int not_over=0, nonover=0;
  Board *me;
  GdkGC *gc;
  GdkEventExpose ee;
  int w,h;
  int sh0=0,sh1=0;

  me=(Board *)data;

  for(li=me->animes.begin();li!=me->animes.end();li++) {
    if (! (*li)->over() )
      nonover++; // to paint overs when nonovers are in the queue
    if ( (*li)->getSoundHold() )
      sh0++;
  }

  gc=gdk_gc_new(me->yidget->window);
  gdk_window_get_size(me->yidget->window,&w,&h);
  ee.area.width=w;
  ee.area.height=h;
  ee.area.x=0;
  ee.area.y=0;
  me->composeVecBoard(gc);
  for(li=me->animes.begin();li!=me->animes.end();li++)
    if (nonover) {
      (*li)->step();
      global.vpieces.drawPiece(me->buffer,gc,me->sqside,
			       (*li)->getX(),(*li)->getY(),(*li)->getPiece());
    }
  me->pasteVecBoard();
  gdk_gc_destroy(gc);

  // count remaining
  for(li=me->animes.begin();li!=me->animes.end();li++) {
    if (! (*li)->over())
      not_over++;
    if ( (*li)->getSoundHold() )
      sh1++;
  }

  // if none remain, finish them all
  if (!not_over) {
    for(li=me->animes.begin();li!=me->animes.end();li++)
      (*li)->lemming();
    me->animes.clear();
  }

  // if there was at least one animation waiting to release the sound
  // and now there is none, flush the sound stack
  if ((sh0)&&(!sh1))
    global.flushSound();

  if (not_over)
    return TRUE;
  else {
    me->gotAnimationLoop=0;
    me->AnimationTimeout=-1;
    if (me->update_queue) {
      me->update(me->UpdateStack.top());
      me->UpdateStack.pop();
    }
    me->queueRepaint();
    return FALSE;
  }
}

gboolean board_animate(gpointer data) {
  list<AnimatedPiece *>::iterator li;
  Board *me;
  Rect *r;
  int dy,nonover=0;
  int sh0=0,sh1=0;

  global.debug("Board","board_animate");
  if (global.UseVectorPieces)
    return(vec_board_animate(data));

  int not_over=0;
  
  me=(Board *)data;
  if (me->cur == NULL)
    return FALSE;

  dy=me->cur->side - me->cur->height;

  for(li=me->animes.begin();li!=me->animes.end();li++) {
    if (! (*li)->over() )
      nonover++; // to paint overs when nonovers are in the queue
    if ( (*li)->getSoundHold() )
      sh0++;
  }

  // clear previous frames

  while(! me->adirty.empty() ) {
    r=me->adirty.front();

    gdk_draw_pixmap(me->yidget->window,
		    me->yidget->style->black_gc,
		    me->buffer,
		    r->x, r->y,
		    r->x, r->y,
		    r->w, r->h);

    delete r;
    me->adirty.pop_front();
  }

  // draw current frames

  for(li=me->animes.begin();li!=me->animes.end();li++)
    if (nonover) {

      (*li)->step();

      r=new Rect((*li)->getX(), (*li)->getY(),
		 me->cur->side, me->cur->height);

      me->cur->drawPiece((*li)->getPiece(),
			 me->yidget->window,
			 me->yidget->style->black_gc,
			 r->x, r->y);
      r->translate(0,dy);
      me->adirty.push_back(r);
    } 

  // count remaining
  for(li=me->animes.begin();li!=me->animes.end();li++) {
    if (! (*li)->over())
      not_over++;
    if ( (*li)->getSoundHold() )
      sh1++;
  }

  // if none remain, finish them all
  if (!not_over) {
    for(li=me->animes.begin();li!=me->animes.end();li++)
      (*li)->lemming();
    me->animes.clear();
  }

  // if there was at least one animation waiting to release the sound
  // and now there is none, flush the sound stack
  if ((sh0)&&(!sh1))
    global.flushSound();
  
  if (not_over)
    return TRUE;
  else {
    me->gotAnimationLoop=0;
    me->AnimationTimeout=-1;
    if (me->update_queue) {
      me->update(me->UpdateStack.top());
      me->UpdateStack.pop();
    }
    me->queueRepaint();
    return FALSE;
  }
}

// ------------- targets

DropSource::DropSource(piece p, int x1,int y1,int w,int h,bool d) {
  X=x1; Y=y1;
  X2=X+w; Y2=Y+h;
  P=p;
  dragged = d;
}

bool DropSource::hit(int x,int y) {
  return((x>=X)&&(x<=X2)&&(y>Y)&&(y<Y2));
}

void TargetManager::cleanTargets() {
  int i,j;
  j=targets.size();
  for(i=0;i<j;i++)
    delete(targets[i]);
  targets.clear();
}

void TargetManager::addTarget(DropSource *ds) {
  targets.push_back(ds);
}

DropSource * TargetManager::hitTarget(int x,int y) {
  int i,j;
  j=targets.size();
  for(i=0;i<j;i++)
    if (targets[i]->hit(x,y))
      return(targets[i]);
  return NULL;
}

EditBoard::EditBoard(ChessGame *cg) : Board() {
  EditMode = true;
  FreeMove = true;
  setGame(cg);
}

static EditBoard *eb_last=0;

void eb_runengine_nobm(GtkMenuItem *item,gpointer data) {
  int *i;
  EngineProtocol *ep;
  Position p;

  if (!eb_last) return;
  if (!eb_last->mygame) return;
  
  i=(int *)data;
  
  switch(*i) {
  case 0: ep=new GnuChess4Protocol(); break;
  case 1: ep=new CraftyProtocol();    break;
  case 2: ep=new SjengProtocol();     break;
  case 3: ep=new XBoardProtocol();    break;
  default: return;
  }

  p=eb_last->mygame->getCurrentPosition();
  ep->setInitialPosition(&p);
  global.chandler->openEngine(ep,0);
}

void eb_runengine_bm(GtkMenuItem *item,gpointer data) {
  EngineBookmark *ebm;
  EngineProtocol *ep;
  Position p;

  if (!eb_last) return;
  if (!eb_last->mygame) return;

  ebm=(EngineBookmark *)data;

  if (!ebm) return;

  switch(ebm->proto) {
  case 0: ep=new XBoardProtocol();    break;
  case 1: ep=new CraftyProtocol();    break;
  case 2: ep=new SjengProtocol();     break;
  case 3: ep=new GnuChess4Protocol(); break;
  default: return;
  }

  p=eb_last->mygame->getCurrentPosition();
  ep->setInitialPosition(&p);
  global.chandler->openEngine(ep,ebm);
}

void EditBoard::popRunEngine(int x,int y) {
  GtkWidget *popmenu, *note=0, *e;
  vector<GtkWidget *> mi;  
  list<EngineBookmark *>::iterator ei;
  unsigned int i;
  static int sindex[4]={0,1,2,3};

  eb_last=this;

  popmenu=gtk_menu_new();

  mi.push_back(gtk_menu_item_new_with_label("GNU Chess 4..."));
  mi.push_back(gtk_menu_item_new_with_label("Crafty..."));
  mi.push_back(gtk_menu_item_new_with_label("Sjeng..."));
  mi.push_back(gtk_menu_item_new_with_label(_("Generic XBoard Engine...")));
  mi.push_back(gtk_separator_menu_item_new());

  for(i=0;i<4;i++)
    gtk_signal_connect(GTK_OBJECT(mi[i]),"activate",
		       GTK_SIGNAL_FUNC(eb_runengine_nobm),
		       (gpointer) (&sindex[i]) );
  
  if (!global.EnginePresets.empty()) {
    note=gtk_menu_item_new_with_label(_("If you pick a bookmark, the engine\n"\
				      "will play the next move, ignoring\n"\
				      "the side setting in the bookmark."));
    gtk_widget_set_sensitive(note,FALSE);
  }

  for(ei=global.EnginePresets.begin();ei!=global.EnginePresets.end();ei++) {
    e=gtk_menu_item_new_with_label((*ei)->caption.c_str());
    mi.push_back(e);

    gtk_signal_connect(GTK_OBJECT(e),"activate",
		       GTK_SIGNAL_FUNC(eb_runengine_bm),
		       (gpointer)(*ei));
  }

  if (note)
    mi.push_back(gtk_separator_menu_item_new());

  for(i=0;i<mi.size();i++) {
    gshow(mi[i]);
    gtk_menu_shell_append(GTK_MENU_SHELL(popmenu),mi[i]);
  }
  if (note) {
    gshow(note);
    gtk_menu_shell_append(GTK_MENU_SHELL(popmenu), note);
  }

  gtk_menu_popup(GTK_MENU(popmenu),NULL,NULL,NULL,NULL,1,gtk_get_current_event_time());
}

void EditBoard::renderClock() {
  GdkGC *gc=clkgc;
  int lch=clock_ch;
  int i,j,k;  

  Board::renderClock();
  if (!lch) return;
  if (!mygame) return;

  // scratch-board pieces and buttons

  C.consumeTop(10);

  if (C.H > 80+48) {
    i = 32;
    for(k=0;k<6;k++) {
      C.drawButton(10+i*k, C.Y, i, i, NULL, 0, 0xffffff, 0x505070);
      C.drawButton(10+i*k, C.Y+i+8, i, i, NULL, 0, 0xffffff, 0x505070);
      
      global.vpieces.drawPiece(clkbar,gc, i, 10+i*k, C.Y,WHITE|(k+1));
      global.vpieces.drawPiece(clkbar,gc, i, 10+i*k, C.Y+i+8,BLACK|(k+1));
      addTarget(new DropSource(WHITE|(k+1),Width()+10+i*k,C.Y,i,i));
      addTarget(new DropSource(BLACK|(k+1),Width()+10+i*k,C.Y+i+8,i,i));
    }    
    C.consumeTop(i*2+16);
  } else if (C.H > 24) {
    i = 16;
    for(k=0;k<6;k++) {
      C.drawButton(10+i*k, C.Y, i, i, NULL, 0, 0xffffff, 0x505070);
      C.drawButton(10+i*(6+k), C.Y, i, i, NULL, 0, 0xffffff, 0x505070);
      
      global.vpieces.drawPiece(clkbar,gc, i, 10+i*k, C.Y,WHITE|(k+1));
      global.vpieces.drawPiece(clkbar,gc, i, 10+i*(6+k), C.Y,BLACK|(k+1));
      addTarget(new DropSource(WHITE|(k+1),Width()+10+i*k,C.Y,i,i));
      addTarget(new DropSource(BLACK|(k+1),Width()+10+i*(6+k),C.Y,i,i));
    }    
    C.consumeTop(24);
  }

  if (C.H > 28) {
    j=C.drawButton(10,C.Y,-1,20,_("Empty"),2,0xffffff, 0x505070);
    addTarget(new DropSource(EMPTY,Width()+10,C.Y,j,20,false));
    
    k=10+j+10;
    j=C.drawButton(k,C.Y,-1,20,_("Initial Position"),2,0xffffff,0x505070);
    
    addTarget(new DropSource(WHITE|BLACK,Width()+k,C.Y,j,20,false));
    
    k+=10+j;
    j=C.drawButton(k,C.Y,-1,20,_("From FEN"),2,0xffffff,0x505070);
    
    addTarget(new DropSource(WASPAWN,Width()+k,C.Y,j,20,false));
  
    C.consumeTop(28);
  }
  if (C.H > 20) {
    j=C.drawButton(10,C.Y,-1,20,_("Run Engine..."),
		   2,0xffffff,0xa07050);
    addTarget(new DropSource(WHITE,Width()+10,C.Y,j,20,false));
    
    k=j+20;
    j+=20+C.stringWidth(2,_("Side to move: "));
    
    C.drawButton( k,C.Y,j-k+10+24,20,NULL,2,0xffffff,0x505070);
    addTarget(new DropSource(BLACK,Width()+k,C.Y,j-k+10+24,20,false));
    
    C.drawString( k+10, C.Y+2, 2, _("Side to move: "));
    
    C.setColor( mygame->getSideHint() ? 0xffffff : 0);
    C.drawEllipse( 10+j+3, C.Y+3, 14, 14, true);
    C.setColor(0xffffff);
    C.drawEllipse( 10+j+3, C.Y+3, 14, 14, false);  
    
    C.consumeTop(20);
  }

}

void EditBoard::flipSide() {
  if (mygame) {
    mygame->setSideHint( ! mygame->getSideHint() );
    clock_ch=1;
    queueRepaint();
  }
}

void EditBoard::getFEN() {
  (new GetFENDialog(this))->show();
}

void EditBoard::applyFEN(string &s) {
  Position p;

  p.setFEN(s.c_str());

  mygame->editEmpty();
  mygame->updatePosition2(p,1,p.sidehint ? 0 : 1,
			  0,0,"<FEN>",false);  
}

GetFENDialog::GetFENDialog(EditBoard *_owner) 
  : ModalDialog(N_("Enter FEN Position"))
{
  GtkWidget *v,*hs,*hb,*ok,*cancel;

  owner=_owner;
  
  gtk_window_set_default_size(GTK_WINDOW(widget), 450, 100);
  
  v=gtk_vbox_new(FALSE,4);

  gtk_container_add(GTK_CONTAINER(widget),v);

  e=gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(v),e,FALSE,TRUE,4);

  hs=gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v),hs,FALSE,TRUE,4);

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

  Gtk::show(ok,cancel,hb,hs,e,v,NULL);
  setDismiss(GTK_OBJECT(cancel),"clicked");

  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(getfen_ok),(gpointer)this);

  focused_widget=e;
}

void getfen_ok(GtkWidget *w, gpointer data) {
  string s;
  GetFENDialog *me;
  me=(GetFENDialog *)data;
  s=gtk_entry_get_text(GTK_ENTRY(me->e));
  me->owner->applyFEN(s);
  me->release();
}

// former animate.cc

int AnimatedPiece::over() { return(ower<=0); }

int AnimatedPiece::getX() { return X; }

int AnimatedPiece::getY() { return Y; }

piece AnimatedPiece::getPiece() { return Piece; }

bool AnimatedPiece::getSoundHold() { return((SoundHold)&&(ower>0)); }

// ================================================
// flat implementation
// ================================================

FlatAnimatedPiece::FlatAnimatedPiece(piece p,int x0,int y0,
				     int xf,int yf,int s,bool sndhold) :
  AnimatedPiece()
{
  Piece=p;
  X0=x0; Y0=y0; XF=xf; YF=yf;
  steps=s; X=X0; Y=Y0;
  DX=(XF-X0)/steps;
  DY=(YF-Y0)/steps;
  ower=steps;
  SoundHold=sndhold;
}

void FlatAnimatedPiece::create(GdkWindow *parent,PieceSet *set,int sqside) {
  square=sqside;
}

void FlatAnimatedPiece::lemming() {
  delete this;
}

void FlatAnimatedPiece::destroy() {

}

void FlatAnimatedPiece::step() {
  if (ower > 1) {
    X+=DX;
    Y+=DY;
  }
  if (ower==1) { X=XF; Y=YF; }
  --ower;  
}
