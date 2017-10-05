/* $Id: chess.h,v 1.47 2008/02/08 14:25:50 bergo Exp $ */

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



#ifndef EBOARD_CHESS_H
#define EBOARD_CHESS_H 1

#include "eboard.h"
#include "widgetproxy.h"
#include "position.h"
#include "board.h"
#include "movelist.h"
#include "clock.h"
#include <stdio.h>
#include "stl.h"

// foreign classes
class Board;

class PGNpair {
 public:
    PGNpair();
    PGNpair(const char *n, char *v);
    PGNpair(const char *n, string &v);

    string name;
    string value;
};

class PGNheader {
 public:
    ~PGNheader();
    void set(const char *n,char *v);
    void set(const char *n,string &v);
    void setIfAbsent(const char *n,char *v);
    void remove(const char *n);
    int  empty();
    int  size();
    PGNpair *get(int index);
    const char *get(const char *n);

 private:
    vector<PGNpair *> header;

    friend ostream &operator<<(ostream &s, PGNheader h);
};

ostream &operator<<(ostream &s, PGNheader h);

class ChessGame : public MoveListListener {
 public:
    ChessGame();
    ChessGame(int _number,int _tyme,int _inc, int _rated,variant _variant,
              char *p1,char *p2);
    ChessGame(ChessGame *src);
    virtual ~ChessGame();

    int operator==(int gnum);

    void updatePosition2(Position &p,int movenum,int blacktomove,
                         int wclockms,int bclockms,char *infoline,
                         bool sndflag=false);
    void updateStock();
    void updateGame(list<Position> &gamedata);
    void flipHint(int flip);
    void setBoard(Board *b);
    Board * getBoard();
    void sendMove(int x1,int y1,int x2,int y2);
    void sendDrop(piece p, int x, int y);
    void acknowledgeInfo();
    void endGame(char *reason,GameResult _result);
    void fireWhiteClock(int wval,int bval);
    void enableMoving(bool flag);
    int  isOver();

    void incrementActiveClock(int secs);

    char *getPlayerString(int index);
    char *getEndReason();
    GameResult getResult();

    void dump();

    Position & getLastPosition();
    Position & getCurrentPosition();
    Position & getPreviousPosition();

    void goBack1();
    void goBackAll();
    void goForward1();
    void goForwardAll();
    void openMoveList();
    void closeMoveList();

    void setFree();

    virtual void moveListClosed();

    void retreat(int nmoves);
    static void LoadPGN(char *filename);
    static bool ParsePgnGame(zifstream &f,
                             char * filename,
                             bool indexonly,
                             int gameid,
                             variant v = REGULAR,
                             ChessGame *updatee = NULL);
    bool savePGN(char *filename, bool append=false);

    bool loadMoves();

    bool isFresh();

    void guessInfoFromPGN();
    void guessPGNFromInfo();

    void editEmpty();
    void editStartPos();

    bool getSideHint(); // true=white to move, used in scratch boards
    void setSideHint(bool white);

    static const char *variantName(variant v);
    static variant variantFromName(const char *p);

    int         GameNumber;
    int         Rated;
    variant     Variant;
    char        PlayerName[2][64];
    char        Rating[2][32];
    piece       MyColor;
    int         StopClock; // for examined games
    TimeControl timecontrol;

    bool    LocalEdit;

    bool    Loaded;
    char    PGNSource[256];
    long    SourceOffset;

    PGNheader pgn;

    GameSource source;
    string     source_data;
    bool       AmPlaying;

    char info0[64];
    int  clock_regressive;

    int  protodata[8];

 private:
    void showResult();
    void fixExamineZigZag(Position &suspect);
    void updateClockAndInfo2(int wclockms, int bclockms,int blacktomove,
                             char *infoline,
                             bool sndflag);

    list<Position> moves;
    list<Position>::iterator cursor;

    char PrivateString[96];

    Board *myboard;
    MoveListWindow *mymovelist;

    Position startpos;

    int last_half_move;
    int over;
    GameResult result;
    char ereason[128];

    static bool             GlyphsInited;
    static vector<string *> Glyphs;
    static void initGlyphs();
    static void failGlyphs();

};

class PGNEditInfoDialog : public ModalDialog {
 public:
    PGNEditInfoDialog(ChessGame *src);
 private:
    ChessGame *obj;
    GtkWidget *clist,*del;
    GtkWidget *en[2];
    int Selection;

    void populate();

    friend void pgnedit_set(GtkWidget *w, gpointer data);
    friend void pgnedit_del(GtkWidget *w, gpointer data);
    friend void pgnedit_rowsel(GtkCList *w, gint row, gint col,
                               GdkEventButton *eb,gpointer data);
    friend void pgnedit_rowunsel(GtkCList *w, gint row, gint col,
                                 GdkEventButton *eb,gpointer data);
};

void pgnedit_set(GtkWidget *w, gpointer data);
void pgnedit_del(GtkWidget *w, gpointer data);
void pgnedit_rowsel(GtkCList *w, gint row, gint col,
                    GdkEventButton *eb,gpointer data);
void pgnedit_rowunsel(GtkCList *w, gint row, gint col,
                      GdkEventButton *eb,gpointer data);


#endif
