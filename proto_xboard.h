/* $Id: proto_xboard.h,v 1.18 2007/01/03 16:49:46 bergo Exp $ */

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

#ifndef EBOARD_XBOARD_H
#define EBOARD_XBOARD_H

#include "stl.h"
#include <gtk/gtk.h>
#include "widgetproxy.h"
#include "clock.h"

class ChessGame;
class EngineBookmark;

#define XBOARD_GAME 7777

// a generic xboard protocol (version 2) parser
// http://www.tim-mann.org/xboard/engine-intf.html

/*
  Protocol
  |
  EngineProtocol (in protocol.cc)
  |
  XBoardProtocol (proto_xboard.cc)
  |
  +----------------+-----------------+
  |                |                 |
  CraftyProtocol   SjengProtocol  GnuChess4Protocol

*/

class XBoardProtocol : public EngineProtocol,
    public UpdateInterface
{

 public:
    XBoardProtocol();
    virtual ~XBoardProtocol() {}

    virtual void receiveString(char *netstring);
    virtual void sendMove(int x1,int y1,int x2,int y2,int prom);
    virtual void finalize();

    virtual void resign();
    virtual void draw();
    virtual void adjourn();
    virtual void abort();
    virtual void retractMove();

    virtual int run();
    virtual int run(EngineBookmark *bm);
    virtual int loadEngine();
    virtual void initEngine();
    virtual void readDialog();

    virtual void setInitialPosition(Position *p);

    virtual void update(); // for time control editing

 protected:
    // initEngine work after variant settings and "new" command,
    // common to most classes
    virtual void lateInit();

    // set status bar after all is done
    virtual void endInit();

    virtual void loadBookmark(EngineBookmark *bm);
    virtual void createDialog();
    virtual void createGame();
    virtual void parseFeatures();
    virtual char *getDialogName();
    virtual char *getComputerName();

    virtual char *xlateDialect(char *s);

    void makeBookmarkCaption();

    void backMove();
    void advanceMove();
    void gameOver(ExtPatternMatcher &pm,GameResult gr,int hasreason=1);
    ChessGame *getGame(bool allowFailure=false);

    virtual void setupPosition();

    GtkWidget *eng_dlg, *eng_book, *engctl_engcmd, *engctl_engdir;

    int         EngineWhite;
    int         MoveNumber;
    int         MoveBlack;
    TimeControl timecontrol;
    int         MaxDepth;
    bool        ThinkAlways;
    variant     Variant;
    char        EngineCommandLine[512];
    char        EngineRunDir[256];
    char        ComputerName[64];

    bool supports_setboard;
    bool requires_usermove;

    int  want_path_pane;
    bool need_handshake;
    int  timeout_id;

    ExtPatternMatcher WhiteWin[2];
    ExtPatternMatcher BlackWin[2];
    ExtPatternMatcher Drawn[2];

    ExtPatternMatcher Features;
    ExtPatternMatcher Moved[3];
    ExtPatternMatcher IllegalMove;

    ExtPatternMatcher Dialect[10];

    Position CurrentPosition;
    Position LegalityBackup;

    EngineBookmark *ebm;

    Position initpos;
    bool     got_init_pos;
    bool     just_sent_fen;

 private:
    int runDialog();
    void destroyDialog();

    void dumpGame();

    char FullCommand[1024];
    GtkWidget *engctl_lbl, *engctl_ewhite, *engctl_ply,*think,*bookmark;
    int  GotResult;
    bool Finalized;
    bool InitDone;

    friend void xboard_eng_ok(GtkWidget *w,gpointer data);
    friend void xboard_eng_cancel(GtkWidget *w,gpointer data);
    friend void xboard_edit_time(GtkWidget *w,gpointer data);
    friend gboolean xboard_eng_delete(GtkWidget *w,GdkEvent *e,gpointer data);

};

void xboard_eng_ok(GtkWidget *w,gpointer data);
void xboard_eng_cancel(GtkWidget *w,gpointer data);
void xboard_edit_time(GtkWidget *w,gpointer data);
gboolean xboard_eng_delete(GtkWidget *w,GdkEvent *e,gpointer data);


class CraftyProtocol : public XBoardProtocol {
 public:
    CraftyProtocol();
    virtual ~CraftyProtocol() {}

    virtual void initEngine();
    virtual void readDialog();

 protected:
    virtual char *getDialogName();

 private:
    char BookPath[256];
    char LogPath[256];
    void resolvePaths();

};

class SjengProtocol : public XBoardProtocol {
 public:
    SjengProtocol();
    virtual ~SjengProtocol() {}

    virtual void sendDrop(piece p,int x,int y);
    virtual void initEngine();
    virtual void readDialog();

 protected:
    virtual void createDialog();
    virtual char *getDialogName();

 private:
    GtkWidget *varbutton[5];
};

class GnuChess4Protocol : public XBoardProtocol {
 public:
    GnuChess4Protocol();
    virtual ~GnuChess4Protocol() {}

    virtual void initEngine();
    virtual void readDialog();

 protected:
    virtual char *getDialogName();
};

#endif
