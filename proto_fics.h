/* $Id: proto_fics.h,v 1.50 2008/02/08 14:25:51 bergo Exp $ */

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

// don't #include this, #include protocol.h instead

#ifndef PROTO_FICS_H
#define PROTO_FICS_H 1

#include <gtk/gtk.h>
#include "stl.h"
#include "position.h"

class FicsProtocol : public Protocol {
 public:
    FicsProtocol();
    virtual ~FicsProtocol();

    void receiveString(char *netstring);
    void sendMove(int x1,int y1,int x2,int y2,int prom);
    void sendDrop(piece p,int x,int y);
    void finalize();
    int  hasAuthenticationPrompts();

    void resign();
    void draw();
    void adjourn();
    void abort();

    void discardGame(int gameid);
    void queryGameList(GameListConsumer *glc);
    void queryAdList(GameListConsumer *glc);
    void observe(int gameid);
    void answerAd(int adid);

    void exaForward(int n);
    void exaBackward(int n);

    void refreshSeeks(bool create);

    vector<string *> * getPlayerActions();
    vector<string *> * getGameActions();

    void callPlayerAction(char *player, string *action);
    void callGameAction(int gameid, string *action);

 private:
    void createGame(ExtPatternMatcher &pm);
    void createExaminedGame(int gameid,char *whitep,char *blackp);
    void createScratchPos(int gameid,char *whitep,char *blackp);
    void addGame(ExtPatternMatcher &pm);
    void removeGame(ExtPatternMatcher &pm);
    void innerRemoveGame(int gameid);
    void parseStyle12(char *data);
    void gameOver(ExtPatternMatcher &pm, GameResult result);
    void updateGame(ExtPatternMatcher &pm);
    void attachHouseStock(ExtPatternMatcher &pm, bool OnlyForBughouse);

    void clearMoveBuffer();
    void retrieve1(ExtPatternMatcher &pm);
    void retrieve2(ExtPatternMatcher &pm);
    void retrievef();
    void retrieveMoves(ExtPatternMatcher &pm);

    void sendGameListEntry();
    void sendAdListEntry(char *pat);
    void sendInitialization();

    char *knowRating(char *player);
    void clearRatingBuffer();

    void ensureSeekGraph();
    void seekAdd(ExtPatternMatcher &pm);
    void seekRemove(ExtPatternMatcher &pm);

    void doPlayerAction(char *player, int id);
    void doGameAction(int gameid, int id);

    void prepareBugPane();

    void updateVar(ProtocolVar pv);

    bool doAllOb1();
    void doAllOb2();

    friend gboolean fics_allob(gpointer data);

    /* private parsing state and parsing methods */

    bool AuthGone;
    bool InitSent;
    bool LecBotSpecial;
    bool LastWasStyle12;
    bool LastWasPrompt;
    bool UseMsecs;

    bool RetrievingGameList;
    bool RetrievingAdList;
    int  RetrievingMoves;
    int  PendingStatus;

    int  LastChannel;
    int  LastColor;

    /* all parser* methods return true if all parsing has been done,
       false if something must still be done.
       (and I'm trying to avoid any false returns, making it all
       functional-programming-happy)
    */

    bool parser1(char *T); /* prompt removal */
    bool parser2(char *T); /* early login */
    bool parser3(char *T); /* game events */
    bool parser4(char *T); /* list retrieval */
    bool parser5(char *T); /* server/game events */
    bool parser6(char *T); /* game termination */
    bool parser7(char *T); /* bughouse and other oddities */
    bool parser8(char *T); /* chat */

    bool doOutput(char *msg, int channel, bool personal, int msgcolor);

    PatternBinder Binder;

    PatternMatcher WelcomeMessage;
    PatternMatcher UserName;

    ExtPatternMatcher Login;
    ExtPatternMatcher Password;
    ExtPatternMatcher Prompt;

    ExtPatternMatcher CreateGame;
    ExtPatternMatcher CreateGame0;
    ExtPatternMatcher ContinueGame;
    ExtPatternMatcher BeginObserve;
    ExtPatternMatcher EndObserve;
    ExtPatternMatcher NotObserve;
    ExtPatternMatcher EndExamine;

    ExtPatternMatcher GameStatus;
    ExtPatternMatcher ExaGameStatus;
    ExtPatternMatcher GameListEntry;
    ExtPatternMatcher EndOfGameList;

    ExtPatternMatcher AdListEntry;
    ExtPatternMatcher EndOfAdList;

    ExtPatternMatcher Style12;
    ExtPatternMatcher Style12House;
    ExtPatternMatcher Style12BadHouse;

    ExtPatternMatcher WhiteWon;
    ExtPatternMatcher BlackWon;
    ExtPatternMatcher Drawn;
    ExtPatternMatcher Interrupted;
    ExtPatternMatcher DrawOffer;

    ExtPatternMatcher PrivateTell;
    ExtPatternMatcher News;

    ExtPatternMatcher MovesHeader;
    ExtPatternMatcher MovesHeader2;
    ExtPatternMatcher MovesHeader3;
    ExtPatternMatcher MovesHeader4;
    ExtPatternMatcher MovesBody;
    ExtPatternMatcher MovesBody2;
    ExtPatternMatcher MovesEnd;

    ExtPatternMatcher Kibitz;
    ExtPatternMatcher Whisper;
    ExtPatternMatcher Say;
    ExtPatternMatcher Say2;
    ExtPatternMatcher Shout;
    ExtPatternMatcher cShout;
    ExtPatternMatcher It;
    ExtPatternMatcher ChannelTell;
    ExtPatternMatcher Seek;
    ExtPatternMatcher Notify;
    ExtPatternMatcher Challenge;

    ExtPatternMatcher LectureBotXTell;
    ExtPatternMatcher LectureBotXTellContinued;

    ExtPatternMatcher PTell;
    ExtPatternMatcher GotPartnerGame;

    ExtPatternMatcher ObsInfoLine;
    ExtPatternMatcher MsSet;

    // seek graph
    ExtPatternMatcher SgClear;
    ExtPatternMatcher SgAdd;
    ExtPatternMatcher SgAdd2;
    ExtPatternMatcher SgRemove;
    ExtPatternMatcher SgRemove2;

    ExtPatternMatcher AllObNone;
    ExtPatternMatcher AllObFirstLine;
    ExtPatternMatcher AllObMidLine;
    int               AllObState;
    char              AllObAcc[1024];

    char nick[64];
    piece style12table[256];

    list<Position> MoveBuf;
    deque<int> MoveRetrievals;
    deque<int> AllObReqs;
    vector<string *> PActions, GActions;
    Position WildStartPos;

    GameListConsumer *lastGLC, *lastALC;

    // 2-position circular buffer to keep rating parsing state
    char xxplayer[2][64];
    char xxrating[2][64];
    int xxnext;

    int  PartnerGame;
    bool UserPlayingWithWhite; /* for won/lost sound event tracking */
};

gboolean fics_allob(gpointer data);

#endif
