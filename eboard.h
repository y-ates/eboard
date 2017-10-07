/* $Id: eboard.h,v 1.50 2008/02/08 14:25:50 bergo Exp $ */

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

#ifndef EBOARD_H
#define EBOARD_H 1

#include <iostream>
#include <gtk/gtk.h>
#include "config.h"

#define EMPTY 0x00
#define BLACK 0x10
#define WHITE 0x20

#define ROOK   0x01
#define KNIGHT 0x02
#define BISHOP 0x03
#define QUEEN  0x04
#define KING   0x05
#define PAWN   0x06

// to keep track of promoted pieces in zhouse
#define WASPAWN 0x40

// Not A Piece -- I mean it :-)
#define NotAPiece    0x0e

#define PIECE_MASK 0x0f
#define COLOR_MASK 0x30
#define CUTFLAGS   0x3f

typedef short int piece;
typedef unsigned char * rgbptr;
typedef unsigned char hati;

class Protocol;
class EngineProtocol;
class EngineBookmark;
class NetConnection;

class PieceProvider {
 public:
	virtual piece getPiece()=0;
};

class InputHandler {
 public:
	virtual void setPasswordMode(int pm)=0;
	virtual void userInput(const char *text)=0;
	virtual void peekKeys(GtkWidget *who)=0;
	virtual int  keyPressed(int keyval, int state)=0;
	virtual void updatePrefix()=0;
};

class ConnectionHandler {
 public:
	virtual void openServer(char *host,int port,
							Protocol *protocol,char *helper)=0;
	virtual void openServer(NetConnection *conn, Protocol *protocol)=0;
	virtual void openEngine(EngineProtocol *protocol, EngineBookmark *bm)=0;
};

class UpdateInterface {
 public:
	virtual void update()=0;
};

class IONotificationInterface {
 public:
	virtual void readAvailable(int handle)=0;
	virtual void writeAvailable(int handle)=0;
};

class MoveListListener {
 public:
	virtual void moveListClosed()=0;
};

class GameListListener {
 public:
	virtual void gameListClosed()=0;
};

class StockListListener {
 public:
	virtual void stockListClosed()=0;
};

class AdListListener {
 public:
	virtual void adListClosed()=0;
};

class ConsoleListener {
 public:
	virtual void consoleClosed()=0;
};

class GameListConsumer {
 public:
	virtual void appendGame(int gamenum, char *desc)=0;
	virtual void endOfList()=0;
};

class PaneChangeListener {
 public:
	virtual void paneChanged(int pgseq,int pgid)=0;
};

class SigChildHandler {
 public:
	virtual void ZombieNotification(int pid)=0;
};

class BookmarkListener {
 public:
	virtual void updateBookmarks()=0;
};

class PieceChangeListener {
 public:
	virtual void pieceSetChanged()=0;
};

typedef enum {
	REGULAR,
	CRAZYHOUSE,
	SUICIDE,
	BUGHOUSE,
	WILD,
	LOSERS,
	GIVEAWAY,
	ATOMIC,
	WILDFR,
	WILDCASTLE,
	WILDNOCASTLE,
} variant;

#define IS_WILD(v)    (((v)==WILD)||((v)==WILDFR)||((v)==WILDCASTLE)||((v)==WILDNOCASTLE))
#define IS_NOT_WILD(v) (((v)!=WILD)&&((v)!=WILDFR)&&((v)!=WILDCASTLE)&&((v)!=WILDNOCASTLE))

typedef enum {
	JOY_AXIS, JOY_BUTTON
} JoystickEventType;

class JoystickListener {
 public:
	virtual void joystickEvent(JoystickEventType jet, int number, int value)=0;
};

typedef enum {
	WHITE_WIN,
	BLACK_WIN,
	DRAW,
	UNDEF
} GameResult;

typedef enum {
	IM_ZERO=0,
	IM_IGNORE=1,
	IM_NORMAL=2,
	IM_PERSONAL=3,
	IM_TOP=4,
	IM_RESET=5,
} Importance;

#define CLOCK_UNCHANGED (-100000000)

// number of configurable fonts
#define NFONTS 5

#define DEFAULT_FONT_CLOK "Sans Bold 26"
#define DEFAULT_FONT_PLYR "Sans 14"
#define DEFAULT_FONT_INFO "Sans 10"
#define DEFAULT_FONT_CONS "Bitstream Vera Sans Mono 10"
#define DEFAULT_FONT_SEEK "Sans 10"


typedef enum {
	EF_PlayerFont,
	EF_ClockFont,
	EF_InfoFont
} EboardFont;

typedef enum {
	GS_PGN_File,
	GS_ICS,
	GS_Engine,
	GS_Other=99
} GameSource;

typedef enum {
	TC_SPM     = 0,  /* seconds per move */
	TC_ICS     = 1,  /* time+increment   */
	TC_XMOVES  = 2,  /* X moves in Y minutes */
	TC_NONE    = 99, /* don't set, use engine's default */
} TimeControlMode;

#define N_SOUND_EVENTS 10

/* i18n placeholders in case gettext isn't available*/

#ifdef ENABLE_NLS
#include "langs.h"
#define _(s) langs_translate(s)
#define N_(s) (s)
#define eboard_gettext(s) langs_translate(s)
#else
#define _(s) (s)
#define N_(s) (s)
#define eboard_gettext(x) (x)
#endif

// gtk simplifications (code looks cleaner this way, especially dlg_prefs.cc)

#define gshow  gtk_widget_show
#define gtset  gtk_toggle_button_set_active
#define gtget  gtk_toggle_button_get_active
#define grnew  gtk_radio_button_new_with_label
#define grg    gtk_radio_button_group
#define gmiset gtk_check_menu_item_set_active

#endif
