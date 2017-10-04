/* $Id: board.h,v 1.61 2008/02/08 14:25:50 bergo Exp $ */

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



#ifndef BOARD_H
#define BOARD_H

#include "stl.h"
#include "eboard.h"
#include "widgetproxy.h"
#include "pieces.h"
#include "position.h"
#include "chess.h"
#include "clock.h"
#include "notebook.h"

// foreign classes referenced here
class ChessGame;

class AnimatedPiece {
 public:
  virtual void create(GdkWindow *parent,PieceSet *set,int sqside)=0;
  virtual void lemming()=0;
  virtual void destroy()=0;
  virtual void step()=0;

  int over();

  int getX();
  int getY();
  piece getPiece();
  bool getSoundHold();

 protected:
  piece Piece;
  int X0,Y0,X,Y,XF,YF,DX,DY;
  int steps,ower;
  int square;
  bool SoundHold;
};

class FlatAnimatedPiece : public AnimatedPiece {
 public:
  FlatAnimatedPiece(piece p,int x0,int y0,int xf,int yf,int s,bool sndhold);
  virtual void create(GdkWindow *parent,PieceSet *set,int sqside);
  virtual void lemming();
  virtual void destroy();
  virtual void step();
};

class Rect {
 public:
  Rect() { x=0; y=0; w=0; h=0; }
  Rect(int a,int b,int c,int d) { x=a; y=b; w=c; h=d; }
  void translate(int dx,int dy) { x+=dx; y+=dy; }
  int x,y,w,h;
};

class DropSource {
 public:
  DropSource(piece p, int x1,int y1,int w,int h, bool d=true);
  bool hit(int x,int y);

  int X,Y,X2,Y2;
  piece P;
  bool dragged;
};

class TargetManager {
 public:
  void cleanTargets();
  void addTarget(DropSource *ds);
  DropSource *hitTarget(int x,int y);

 private:
  vector<DropSource *> targets;
};

class RootBoard {
 protected:
  void clockString(int val,char *z,int maxlen);
};

// I just can't work out the correct syntax to put the
// bodies of methods in the .cc file, if you know, email me
template<class T>
class mblock {
 public:

  mblock(int _sz=0) { data=0; sz=0; if (_sz) ensure(_sz); }

  ~mblock() { if (sz) g_free((gpointer) data); sz=0; data=0; }

  bool ensure(int _sz) {
    if (_sz <= sz) return true; // never shrink
    if (sz==0) data=(T *) g_malloc(_sz * sizeof(T));
    else data=(T *) g_realloc((gpointer) data, _sz * sizeof(T));
    sz=_sz;
    if (!data) cerr << "eboard ** mblock error, expect oddities\n";
    return(data!=0);
  }

  T * data;
  int sz;
};

class Board : public WidgetProxy,
          public ClockHost,
          public RootBoard,
          public NotebookInsider,
              public TargetManager,
          public PieceChangeListener
{
 public:
  Board();
  virtual ~Board();

  void repaint();

  void        setSensitive(bool state);
  void        setPartnerGame();
  void        setSelColor(int color);
  void        clearSel();
  void        setFlipped(bool flip);
  void        setFlipInversion(bool v);
  bool        getFlipInversion();
  ChessClock *getClock();
  int         getClockValue2(bool white);
  void        setClock2(int wmsecs,int bmsecs,int actv,int cdown);
  void        setInfo(int idx,char *s);
  void        setInfo(int idx,string &s);
  void        setCanMove(bool value);
  void        stopClock();
  void        reset();

  void  setGame(ChessGame *game);
  ChessGame *getGame();
  void  reloadFonts();

  void freeze();
  void thaw();
  void queueRepaint();
  void invalidate();

  virtual void show();

  void update(bool sndflag=false);
  virtual void updateClock();

  virtual void pieceSetChanged();

  void walkBack1();
  void walkBackAll();
  void walkForward1();
  void walkForwardAll();
  void openMovelist();
  bool hasGame();

  bool effectiveFlip() { return(FlipInvert?(!flipped):flipped); }

  void dump();

  void supressNextAnimation();

  bool FreeMove; // editing positions, examining games
  bool EditMode; // explicit edit mode (with pieces to drop and additional commands)

  static Board * PopupOwner;

  virtual void renderClock();

  void joystickCursor(int axis, int value);
  void joystickSelect();

 protected:
  GdkPixmap *clkbar;
  GdkGC *clkgc;
  LayoutBox C;
  int clock_ch;

  ChessGame *mygame;

  // inline calculations
  int Width()  { return( (sqside<<3) + (borx<<1) ); }
  int Height() { return( (sqside<<3) + (bory<<1) + morey); }

 private:
  GtkWidget *yidget;

  PieceSet *cur;
  GdkPixmap *buffer;
  GdkGC *wgc;
  rgbptr scrap;

  mblock<unsigned char> M[2];

  bool canselect;
  bool allowMove;
  bool FlipInvert;
  bool flipped;
  bool repaint_due;
  bool update_queue;
  bool UpdateClockAfterConfigure;

  int sel_color;
  int frozen_lvl;
  int LockAnimate;

  vector<SMove *> LastMove;

  list<AnimatedPiece *> animes;
  int gotAnimationLoop;
  int AnimationTimeout;

  Position position;
  Position currently_rendered;
  Position fake;

  ChessClock clock;

  int hilite_ch;

  // selection/highlight
  int ax[2],ay[2],sp;
  int jx,jy,jpxv,jpyv; // discrete joystick cursor
  int jsx,jsy,jsvx,jsvy,jstid,jsnt; // smooth joystick cursor

  // premove
  int premoveq;
  int pmx[2],pmy[2];
  piece pmp;

  // dragged piece info
  bool  dr_active;
  bool  dr_fromstock;
  int   dr_ox, dr_oy, dr_c, dr_r;
  piece dr_p;
  int   dr_step;
  int   dr_dirty[2];
  bool  dr_fto;

  // animation
  list<Rect *> adirty;

  // info lines
  string info[6];

  int dropsq[2];

  int sqside;
  int sqw, sqh; /* canvas size */
  int borx, bory; // borders (when coordinates are shown)
  int morey; // extra y space for extruded sets

  stack<bool> UpdateStack;

  void popupDropMenu(int col,int row,int sx,int sy,guint32 etime);
  void popupProtocolMenu(int x,int y, guint32 etime);
  void sendMove();
  void outlineRectangle(GdkGC *gc, int x,int y,int color,int pen);
  void drawJoystickCursor(GdkGC *gc);
  void drawBall(GdkGC *gc, int x,int y,int color,int radius);
  void drop(piece p);
  void drawCoordinates(GdkPixmap *dest,GdkGC *gc,int side);

  void composeVecBoard(GdkGC *gc);
  void pasteVecBoard();

  void shadyText(int x,int y, int f, char *z);

  friend gboolean board_animate(gpointer data);
  friend gboolean board_joycursor(gpointer data);
  friend gboolean vec_board_animate(gpointer data);

  friend gboolean board_expose_event(GtkWidget *widget,GdkEventExpose *ee,
                     gpointer data);
  friend gboolean board_configure_event(GtkWidget *widget,
                    GdkEventConfigure *ce,
                    gpointer data);
  friend gboolean board_button_press_event(GtkWidget *widget,
                       GdkEventButton *be,
                       gpointer data);
  friend gboolean board_button_release_event(GtkWidget *widget,
                         GdkEventButton *be,
                         gpointer data);
  friend gboolean board_motion_event(GtkWidget *widget,
                     GdkEventMotion *em,
                     gpointer data);

  friend gboolean gtkDgtnixEvent(GIOChannel* channel, GIOCondition cond, gpointer data);


  friend void drop_callbackP(GtkMenuItem *item,gpointer data);
  friend void drop_callbackR(GtkMenuItem *item,gpointer data);
  friend void drop_callbackN(GtkMenuItem *item,gpointer data);
  friend void drop_callbackB(GtkMenuItem *item,gpointer data);
  friend void drop_callbackQ(GtkMenuItem *item,gpointer data);

  friend void menu_whitep(GtkMenuItem *item, gpointer data);
  friend void menu_blackp(GtkMenuItem *item, gpointer data);
  friend void menu_gamep(GtkMenuItem *item, gpointer data);

#if MAEMO
  friend void tap_and_hold_cb(GtkWidget *widget, gpointer user_data);
#endif

};

class EditBoard : public Board {
 public:
  EditBoard(ChessGame *cg);
  virtual ~EditBoard() {}
  void popRunEngine(int x,int y);
  void flipSide();
  void getFEN();
  void applyFEN(string &s);

  virtual void renderClock();

 private:
  friend void eb_runengine_bm(GtkMenuItem *item,gpointer data);
  friend void eb_runengine_nobm(GtkMenuItem *item,gpointer data);
};

class GetFENDialog : public ModalDialog {
 public:
  GetFENDialog(EditBoard *_owner);

 private:
  EditBoard *owner;
  GtkWidget *e;

  friend void getfen_ok(GtkWidget *w, gpointer data);
};


/* friend declarations for GCC 4.x */
gboolean board_animate(gpointer data);
gboolean vec_board_animate(gpointer data);
gboolean board_expose_event(GtkWidget *widget,GdkEventExpose *ee,gpointer data);
gboolean board_configure_event(GtkWidget *widget,GdkEventConfigure *ce,
                               gpointer data);
gboolean board_button_press_event(GtkWidget *widget,GdkEventButton *be,
                                  gpointer data);
gboolean board_button_release_event(GtkWidget *widget,GdkEventButton *be,
                                    gpointer data);
gboolean board_motion_event(GtkWidget *widget,GdkEventMotion *em,gpointer data);
void drop_callbackP(GtkMenuItem *item,gpointer data);
void drop_callbackR(GtkMenuItem *item,gpointer data);
void drop_callbackN(GtkMenuItem *item,gpointer data);
void drop_callbackB(GtkMenuItem *item,gpointer data);
void drop_callbackQ(GtkMenuItem *item,gpointer data);
void menu_whitep(GtkMenuItem *item, gpointer data);
void menu_blackp(GtkMenuItem *item, gpointer data);
void menu_gamep(GtkMenuItem *item, gpointer data);
void eb_runengine_bm(GtkMenuItem *item,gpointer data);
void eb_runengine_nobm(GtkMenuItem *item,gpointer data);
void getfen_ok(GtkWidget *w, gpointer data);
gboolean board_joycursor(gpointer data);

#endif

