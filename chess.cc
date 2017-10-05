/* $Id: chess.cc,v 1.93 2008/02/08 14:25:50 bergo Exp $ */

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
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "eboard.h"
#include "chess.h"
#include "movelist.h"
#include "tstring.h"
#include "util.h"
#include "global.h"

// --- stream operators

ostream &operator<<(ostream &s, PGNheader h) {
    unsigned int i;
    for(i=0;i<h.header.size();i++)
        s << '[' << h.header[i]->name << " \"" << h.header[i]->value << "\"]\n";
    return(s);
}

// --- classes

bool             ChessGame::GlyphsInited = false;
vector<string *> ChessGame::Glyphs;

ChessGame::ChessGame() {
    int i;

    for(i=0;i<8;i++)
        protodata[i]=0;

    GameNumber=-1;
    Rated=0;
    Variant=REGULAR;
    memset(PlayerName[0],0,64);
    memset(PlayerName[1],0,64);
    Rating[0][0]=Rating[1][0]=0;
    last_half_move=-1;
    myboard=0;
    clock_regressive=1;
    info0[0]=0;
    cursor=moves.end();
    mymovelist=0;
    over=0;
    MyColor=WHITE;
    StopClock=0;
    LocalEdit=false;
    Loaded=true;
    PGNSource[0]=0;
    SourceOffset=0;

    source=GS_Other;
    source_data="n/a";
    AmPlaying=false;
}

ChessGame::ChessGame(int _number,int _tyme,int _inc, int _rated,
                     variant _variant,
                     char *p1,char *p2) {
    int i;

    GameNumber=_number;
    timecontrol.setIcs(_tyme,_inc);
    Rated=_rated;
    Variant=_variant;
    PlayerName[0][63]=PlayerName[1][63]=0;
    g_strlcpy(PlayerName[0],p1,64);
    g_strlcpy(PlayerName[1],p2,64);
    last_half_move=-1;
    myboard=0;
    cursor=moves.end();
    mymovelist=0;
    over=0;
    MyColor=WHITE;
    StopClock=0;
    Rating[0][0]=Rating[1][0]=0;
    LocalEdit=false;
    Loaded=true;
    PGNSource[0]=0;
    SourceOffset=0;

    source=GS_Other;
    source_data="n/a";
    AmPlaying=false;

    for(i=0;i<8;i++)
        protodata[i]=0;
}

ChessGame::ChessGame(ChessGame *src) {
    list<Position>::iterator i;
    int j;
    bool cset=false;

    GameNumber = src->GameNumber;
    Rated = src->Rated;
    Variant = src->Variant;
    strcpy(PlayerName[0],src->PlayerName[0]);
    strcpy(PlayerName[1],src->PlayerName[1]);
    last_half_move = src->last_half_move;
    myboard=0;
    mymovelist=0;
    over=0;
    MyColor=WHITE;
    StopClock=0;
    Rating[0][0]=Rating[1][0]=0;
    LocalEdit=false;
    timecontrol = src->timecontrol;

    Loaded=true;
    PGNSource[0]=0;
    SourceOffset=0;

    for(j=0;j<8;j++)
        protodata[j] = src->protodata[j];

    // clone move list
    for(i=src->moves.begin();i!=src->moves.end();i++) {
        moves.push_back(Position(*i));
        if (src->cursor == i) { cursor = moves.end(); cursor--; cset = true; }
    }

    if (moves.empty())
        moves.push_back(Position());
    if (!cset) { cursor = moves.end();  cursor--; }

    source=src->source;
    source_data=src->source_data;
    AmPlaying=src->AmPlaying;
}

ChessGame::~ChessGame() {
    moves.clear();
}

int ChessGame::operator==(int gnum) {
    return(gnum==GameNumber);
}

void ChessGame::setFree() {
    if (myboard)
        myboard->FreeMove=true;
}

void ChessGame::acknowledgeInfo() {
    char s[64],tz[128];

    if (myboard==NULL)
        return;
    myboard->freeze();

    // P2P and engine games
    if (GameNumber > 7000 && GameNumber < 9000) {
        timecontrol.toString(s,64);
    } else if (GameNumber >= 9000) { // client-side games - rated/unrated unknown
        timecontrol.toShortString(tz,128);
        snprintf(s,64,_("Game #%d - %s"), // TRANSLATE
                 GameNumber,tz);
    } else { // live ICS games
        timecontrol.toShortString(tz,128);
        snprintf(s,64,_("Game #%d - %s - %s"), // TRANSLATE?
                 GameNumber,tz,
                 Rated?_("rated"):
                 _("unrated"));
    }

    myboard->setInfo(0,s);

    strcpy(s,variantName(Variant));
    myboard->setInfo(1,s);

    if (!moves.empty())
        myboard->setInfo(4,moves.back().getMaterialString(Variant));

    if (over)
        showResult();

    myboard->invalidate();
    myboard->thaw();
    myboard->updateClock();
}

void ChessGame::showResult() {
    char str[256];
    if ((!over)||(!myboard))
        return;
    switch(result) {
    case WHITE_WIN: strcpy(str,"1-0 "); break;
    case BLACK_WIN: strcpy(str,"0-1 "); break;
    case DRAW: strcpy(str,"1/2-1/2 "); break;
    default: strcpy(str,"(*) "); break;
    }
    if (strlen(ereason)>2)
        g_strlcat(str,ereason,256);
    myboard->setInfo(3,str);
}

void ChessGame::updateStock() {
    if (over)
        return;
    if (!myboard)
        return;
    if (!moves.empty())
        myboard->setInfo(5,moves.back().getHouseString());

    // bughouse partner game
    if (protodata[2])
        global.bugpane->setPosition(moves.back());
}

bool ChessGame::isFresh() {
    return((bool)(moves.empty()));
}

// usually called to remove an illegal move from the movelist
void ChessGame::retreat(int nmoves) {
    for(;nmoves;nmoves--)
        if (!moves.empty())
            moves.pop_back();
    cursor=moves.end();
    cursor--;
}

void ChessGame::fixExamineZigZag(Position &suspect) {
    list<Position>::iterator i;

    for(i=moves.begin();i!=moves.end();i++)
        if ( (*i) == suspect )
            if (! suspect.getLastMove().compare( (*i).getLastMove() ) ) {
                moves.erase(i,moves.end());
                return;
            }
}

// updates clocks and just-below-clocks strings
void ChessGame::updateClockAndInfo2(int wclockms, int bclockms,
                                    int blacktomove, char *infoline,
                                    bool sndflag)
{
    if (myboard) {
        myboard->freeze();

        if (StopClock)
            myboard->setClock2(wclockms,bclockms,blacktomove?2:-2,clock_regressive);
        else
            myboard->setClock2(wclockms,bclockms,blacktomove?1:-1,clock_regressive);

        if (infoline)
            myboard->setInfo(2,infoline);

        if (!moves.empty()) {
            myboard->setInfo(4,moves.back().getMaterialString(Variant));
            myboard->setInfo(5,moves.back().getHouseString());
        }
        myboard->update(sndflag);
        myboard->thaw();
        myboard->contentUpdated();
    }
}

void ChessGame::updatePosition2(Position &p,int movenum,int blacktomove,
                                int wclockms,int bclockms,char *infoline,
                                bool sndflag) {
    global.debug("ChessGame","updatePosition");

    if (over)
        return;

    // bughouse partner's game
    if (protodata[2]) {
        global.bugpane->freeze();
        global.bugpane->setPosition(p);

        if (StopClock)
            global.bugpane->setClock2(wclockms,bclockms,blacktomove?2:-2,clock_regressive);
        else
            global.bugpane->setClock2(wclockms,bclockms,blacktomove?1:-1,clock_regressive);

        global.bugpane->thaw();
    }

    if (last_half_move>=0)
        if (! moves.empty() )
            if (p == moves.back() ) {
                updateClockAndInfo2(wclockms,bclockms,blacktomove,infoline,
                                    sndflag);
                return;
            }

    Position np;
    np=p;
    np.setLastMove(infoline);
    //  cerr << "LastMove = [" << infoline << "]\n";

    fixExamineZigZag(np);

    moves.push_back(np);
    cursor=moves.end();
    cursor--;
    last_half_move=(movenum*2)+(blacktomove?0:1);

    updateClockAndInfo2(wclockms,bclockms,blacktomove,infoline,
                        sndflag);

    if (mymovelist)
        mymovelist->updateList(moves);
}

void ChessGame::incrementActiveClock(int secs) {
    ChessClock *c;
    if (myboard) {
        c = myboard->getClock();
        c->incrementClock2(0,secs*1000);
    }
}

void ChessGame::fireWhiteClock(int wval,int bval) {
    myboard->setClock2(wval*1000,bval*1000,-1,clock_regressive);
}

void ChessGame::setBoard(Board *b) {
    myboard=b;
    if (myboard)
        myboard->setGame(this);
}

Board * ChessGame::getBoard() {
    return(myboard);
}

Position & ChessGame::getLastPosition() {
    if (moves.empty())
        return(startpos);
    return(moves.back());
}

Position & ChessGame::getCurrentPosition() {
    if (moves.empty())
        return(startpos);
    if (cursor!=moves.end())
        return(*cursor);
    else
        return(moves.back());
}

Position & ChessGame::getPreviousPosition() {
    list<Position>::iterator pv;
    if (moves.empty())
        return(startpos);
    if (cursor!=moves.end()) {
        pv=cursor;
        if (pv!=moves.begin())
            pv--;
        return(*pv);
    } else {
        pv=moves.end();
        pv--;
        if (pv!=moves.begin())
            pv--;
        return(*pv);
    }
}

void ChessGame::goBack1() {
    if (moves.empty())
        return;
    if (cursor!=moves.begin())
        cursor--;
}

void ChessGame::goBackAll() {
    if (moves.empty())
        return;
    cursor=moves.begin();
}

void ChessGame::goForward1() {
    if (moves.empty())
        return;
    cursor++;
    if (cursor==moves.end())
        cursor--;
}

void ChessGame::goForwardAll() {
    if (moves.empty())
        return;
    cursor=moves.end();
    cursor--;
}

void ChessGame::openMoveList() {
    if (mymovelist) {
        gtk_window_activate_focus(GTK_WINDOW(mymovelist->widget));
    } else {
        mymovelist=new MoveListWindow(PlayerName[0],PlayerName[1],
                                      GameNumber,moves,over,result,ereason);
        mymovelist->setListener(this);
        mymovelist->show();
    }
}

void ChessGame::closeMoveList() {
    if (mymovelist)
        mymovelist->close();
}

void ChessGame::moveListClosed() {
    delete mymovelist;
    mymovelist=0;
}

void ChessGame::editEmpty() {
    Position p;
    int i,j;

    if (!LocalEdit) return;

    moves.clear();
    last_half_move=1;

    for(i=0;i<8;i++) for(j=0;j<8;j++) p.setPiece(i,j,EMPTY);
    p.sidehint=true;

    moves.push_back(p);
    cursor=moves.end();
    cursor--;
    if (myboard) {
        myboard->setInfo(2,(*cursor).getLastMove());
        myboard->setInfo(4,p.getMaterialString());
        myboard->update();
    }
}

void ChessGame::editStartPos() {
    Position p;

    if (!LocalEdit) return;

    moves.clear();
    startpos.setStartPos(); // sidehint might need to be fixed
    last_half_move=1;

    moves.push_back(p);
    cursor=moves.end();
    cursor--;
    if (myboard) {
        myboard->setInfo(2,(*cursor).getLastMove());
        myboard->setInfo(4,p.getMaterialString());
        myboard->update();
    }
}

void ChessGame::sendDrop(piece p, int x, int y) {
    list<Position>::iterator z;

    if (LocalEdit) {
        Position o;
        o = *cursor;
        o.setPiece(x,y,p);

        z=cursor; z++;
        moves.erase(z,moves.end());

        moves.push_back(o);
        last_half_move++;
        cursor=moves.end();
        cursor--;
        if (myboard) { myboard->setInfo(2,(*cursor).getLastMove()); myboard->update(); }
    } else {
        if (global.protocol) global.protocol->sendDrop(p&PIECE_MASK,x,y);
    }

}

void ChessGame::sendMove(int x1,int y1,int x2,int y2) {
    int promote=0;
    piece q;
    list<Position>::iterator z;
    char v[16];

    if (!moves.empty()) {
        Position p;
        p=moves.back();

        if ((p.getPiece(x1,y1)==(PAWN|WHITE))&&
            (y1==6)&&(y2==7))
            promote=1;
        if ((p.getPiece(x1,y1)==(PAWN|BLACK))&&
            (y1==1)&&(y2==0))
            promote=1;
    }

    // ====================== BEGIN SCRATCH BOARD ONLY ===============================
    if (LocalEdit) {
        Position p;
        p = *cursor;

        q=p.getPiece(x1,y1)&COLOR_MASK;

        if (p.isMoveLegalCartesian(x1,y1,x2,y2,q,REGULAR)) {
            p.stdNotationForMove(x1,y1,x2,y2,
                                 promote ?  q | global.promotion->getPiece() : EMPTY,
                                 v, REGULAR);
            p.setLastMove(v);

            p.moveCartesian(x1,y1,x2,y2,REGULAR);
            if (promote) p.setPiece(x2,y2, q | global.promotion->getPiece() );

        } else {

            p.setPiece(x2,y2,p.getPiece(x1,y1));
            p.setPiece(x1,y1,EMPTY);

        }

        z=cursor; z++; if (z!=moves.end()) moves.erase(z,moves.end());

        moves.push_back(p);
        last_half_move++;
        cursor=moves.end();
        cursor--;
        if (myboard) { myboard->setInfo(2,(*cursor).getLastMove()); myboard->update(); }
    }
    // ====================== END SCRATCH BOARD ONLY ===============================

    else {
        if (global.protocol) global.protocol->sendMove(x1,y1,x2,y2,promote);
    }

}

bool ChessGame::getSideHint() {
    return (getCurrentPosition().sidehint);
}

void ChessGame::setSideHint(bool white) {
    getCurrentPosition().sidehint = white;
}

void ChessGame::flipHint(int flip) {
    if (myboard)
        myboard->setFlipped(flip!=0);
}

void ChessGame::enableMoving(bool flag) {
    if (!myboard)
        return;
    myboard->setCanMove(flag);
}

void ChessGame::endGame(char *reason,GameResult _result) {
    if (over)
        return; // can't end twice
    result=_result;

    // bughouse partner game
    if (protodata[2])
        global.bugpane->stopClock();

    if (myboard)
        myboard->stopClock();

    over=1;
    g_strlcpy(ereason,reason,128);

    showResult();
    if (myboard)
        myboard->setCanMove(false);

    if (pgn.empty())
        guessPGNFromInfo();

    if (mymovelist)
        mymovelist->updateList(moves,1,result,ereason);
}

int ChessGame::isOver() {
    return(over);
}

void ChessGame::updateGame(list<Position> &gamedata) {
    list<Position>::iterator li;
    Position startpos;
    int hmn=-1;

    moves.clear();

    for(li=gamedata.begin();li!=gamedata.end();li++) {
        Position p;
        p = *li;
        moves.push_back(p);
        ++hmn;
    }

    // should fix the adding of FEN fields on followed games
    if (moves.empty() && IS_NOT_WILD(Variant)) {
        moves.push_back(Position());
        hmn=0;
    }

    cursor=moves.end();
    cursor--;
    last_half_move=hmn;
    if (mymovelist)
        mymovelist->updateList(moves);
}

void ChessGame::dump() {
    cerr.setf(ios::hex,ios::basefield);
    cerr.setf(ios::showbase);

    cerr << "[game " << ((uint64_t) this) << "] ";

    cerr.setf(ios::dec,ios::basefield);

    cerr << "game#=" << GameNumber << " ";
    cerr << "rated=" << Rated << " ";
    cerr << "white=" << PlayerName[0] << " ";
    cerr << "black=" << PlayerName[1] << " ";
    cerr << "over=" << over << " ";

    cerr.setf(ios::hex,ios::basefield);
    cerr.setf(ios::showbase);

    cerr << "board=[" << ((uint64_t)myboard) << "]" << endl;
}

char * ChessGame::getPlayerString(int index) {
    index%=2;
    PrivateString[0]=0;
    if ((global.ShowRating)&&(strlen(Rating[index])))
        snprintf(PrivateString,96,"%s (%s)",PlayerName[index],Rating[index]);
    else
        strcpy(PrivateString,PlayerName[index]);
    return(PrivateString);
}

// ---- PGN

void ChessGame::guessInfoFromPGN() {
    const char *cp;

    cp=pgn.get("White");
    if (cp!=NULL) g_strlcpy(PlayerName[0],cp,64);

    cp=pgn.get("Black");
    if (cp!=NULL) g_strlcpy(PlayerName[1],cp,64);

    cp=pgn.get("Result");
    result = UNDEF;
    if (cp!=NULL) {
        if (cp[0]=='0')      result=BLACK_WIN;
        else if (cp[1]=='/') result=DRAW;
        else if (cp[0]=='1') result=WHITE_WIN;
    }

    timecontrol.mode = TC_NONE;
    cp=pgn.get("TimeControl");
    if (cp!=NULL) {
        int a,b;
        ExtPatternMatcher FischerClock, XMoves, SecsPerMove;

        FischerClock.set("%N+%N*");
        XMoves.set("%N/%N*");
        SecsPerMove.set("0+%N*");

        if (SecsPerMove.match(cp)) {

            a = atoi(SecsPerMove.getNToken(0));
            timecontrol.setSecondsPerMove(a);

        } else if (FischerClock.match(cp)) {

            a = atoi(FischerClock.getNToken(0));
            b = atoi(FischerClock.getNToken(1));
            timecontrol.setIcs(a,b);

        } else if (XMoves.match(cp)) {

            a = atoi(FischerClock.getNToken(0));
            b = atoi(FischerClock.getNToken(1));
            timecontrol.setXMoves(a,b);

        }
    }

    cp=pgn.get("Variant");
    if (cp!=NULL) Variant = variantFromName(cp);

}

void ChessGame::guessPGNFromInfo() {
    time_t now;
    struct tm *snow;
    char z[64],y[256],x[128];
    Position startpos;

    char rz[64];
    int i,j;

    now=time(0);
    snow=localtime(&now);

    pgn.set("Event","?");
    switch(source) {
    case GS_ICS:
        strcpy(x,variantName(Variant));
        x[0] = toupper(x[0]);

        // ICS Rated Chess Match
        snprintf(y,256,"ICS %s %s Match",
                 Rated?"Rated":"Unrated",
                 x);
        pgn.set("Event",y);
        break;
    case GS_Engine:
        strcpy(x,variantName(Variant));
        x[0] = toupper(x[0]);

        // Engine Chess Match
        snprintf(y,256,"Engine %s Match",x);
        pgn.set("Event",y);
        break;
    case GS_Other:
    case GS_PGN_File:
    default:
        pgn.set("Event","?");
        break;
    }

    pgn.set("Site","?");

    snprintf(z,64,"%d.%.2d.%.2d",1900+snow->tm_year,
             1+snow->tm_mon,snow->tm_mday);

    pgn.set("Date",z);
    pgn.set("Round","?");

    if (strlen(PlayerName[0])) pgn.set("White",PlayerName[0]);
    if (strlen(PlayerName[1])) pgn.set("Black",PlayerName[1]);

    // get rating, but prevent strings like (1967), (----), (UNR)...
    memset(rz,0,64);
    j=strlen(Rating[0]);
    for(i=0;i<j;i++)
        if (isdigit(Rating[0][i]))
            rz[strlen(rz)]=Rating[0][i];
    if (strlen(rz)) pgn.set("WhiteElo",rz);

    memset(rz,0,64);
    j=strlen(Rating[1]);
    for(i=0;i<j;i++)
        if (isdigit(Rating[1][i]))
            rz[strlen(rz)]=Rating[1][i];
    if (strlen(rz)) pgn.set("BlackElo",rz);

    timecontrol.toPGN(rz,64);
    pgn.set("TimeControl",rz);

    if (Variant != REGULAR) {
        pgn.set("Variant",(char *) variantName(Variant));
    }

    //  strcpy(rz,Rated?"yes":"no");
    //  pgn.set("Rated",rz);

    if (over) {
        switch(result) {
        case WHITE_WIN: pgn.set("Result","1-0"); break;
        case BLACK_WIN: pgn.set("Result","0-1"); break;
        case DRAW:      pgn.set("Result","1/2-1/2"); break;
        case UNDEF:     pgn.set("Result","*"); break;
        }
    }

    if ( startpos != moves.front() )
        pgn.set("FEN", moves.front().getFEN() );
}

bool ChessGame::savePGN(char *filename, bool append) {
    char z[512];
    const char *cp;
    list<Position>::iterator li;
    int xp,lm,mn;
    tstring t;
    string sp, path, *p;

    if (moves.size() < 4) {  // TODO:less than 2 or 4 moves?!
        global.status->setText(_("savePGN failed: Won't save game with less than 2 moves"),5);
        return false;
    }

    // FIXME ~username won't work, only ~/something
    if (filename[0]=='~') {
        path=global.env.Home;
        path+='/';
        path+=(&filename[2]);
    } else
        path=filename;

    ofstream f(path.c_str(),append? ios::app : ios::out);

    if (!f) {
        snprintf(z,512,_("savePGN failed: %s"),filename);
        global.status->setText(z,10);
        return false;
    }

    if (pgn.empty())
        guessPGNFromInfo();

    f << pgn << endl;

    lm=0; xp=0;
    for(li=moves.begin();li!=moves.end();li++) {

        sp=(*li).getLastMove();

        if (sp.empty())
            continue;

        t.set(sp);

        mn=t.tokenvalue(".");
        if (!mn)
            continue;
        if (mn>lm) {
            f << mn << ". ";
            lm=mn;
            xp+=(lm>9)?4:3; // MINOR BUG FOR GAMES WITH 100+ MOVES
        }
        p=t.token("(). \t");
        f << (*p) << ' ';
        xp+=1+p->length();

        if (xp>70) {
            xp=0;
            f << endl;
        }
    }

    if (strlen(ereason) > 5) {
        if (xp) f << endl;
        f << '{' << ereason << "} ";
    }

    cp=pgn.get("Result");

    if (cp)
        f << cp;
    else
        f << '*';

    f << endl << endl;

    f.close();

    snprintf(z,512,_("--- %s game to PGN file %s"),
             append?_("Appended"):
             _("Wrote"),filename);
    global.output->append(z,0xc0ff00);
    return true;
}

// indexonly: do not load the move text now
bool ChessGame::ParsePgnGame(zifstream &f,
                             char * filename,
                             bool indexonly,
                             int gameid,
                             variant v,
                             ChessGame *updatee) {
    /*
       ok, now (0.4.x) we do this in a DFA-like manner
       0  = state 0 (whitespace before movetext)

       PGN header states:
       1  = found [, consuming PGN field name
       2  = consuming whitespace between field name and field data
       3  = found " , consuming field data
       4  = found ending " , waiting ]

       6  = reading token
       7  = inside { comment
       8 = like level 0, but can't have headers
       9 = got result, waiting end of line (== end of game)

       10 = initial state ;-)
       whitespace (and comments like in twic pgns) between games
       11 = inside alternate line
    */

    int   state    = 10;
    int   halfmove = 0;
    int   altmoves = 0;
    piece color    = WHITE;
    tstring t;

    list<Position> gl;
    ChessGame *game=0;

    ExtPatternMatcher Legacy0, Legacy1, Legacy2, Result;
    PatternBinder Binder, Binder2;

    char alpha[256], beta[256], omega[256];
    int ap=0, bp=0;
    char line[512], *p;
    long pstart;
    GameResult r;

    static char *whitespace = " \t\n\r";
    static char *token  =     "1234567890*/KQBNRP@kqbnrp=+#abcdefgh-Ox$";
    static char *digits =     "1234567890";
    static char *resultset =  "01*";

    Legacy0.set("%%eboard:variant:%s*");
    Legacy1.set("%%eboard:clock/r:%n:%n:%n*");
    Legacy2.set("%%eboard:clue:*");
    Binder.add(&Legacy0,&Legacy1,&Legacy2,NULL);

    Result.set("*%N-%N*");
    Binder2.add(&Result,NULL);

    ChessGame::initGlyphs();

    pstart=f.tellg();
    if (!updatee) {
        game = new ChessGame();
        game->Variant = v;
        game->GameNumber = gameid;
        game->source = GS_PGN_File;
        game->source_data = filename;

        if (indexonly) {
            game->Loaded=false;
            g_strlcpy(game->PGNSource,filename,256);
            game->SourceOffset = pstart;
        }
    }

    omega[0]=0;

    //  #define PGN_DEBUG 1

#ifdef PGN_DEBUG
    cerr << "entering new game\n";
#endif

    while( memset(line,0,512), f.getline(line,510,'\n') ) {
        line[strlen(line)]='\n';

#ifdef PGN_DEBUG
        cerr << "state = " << state << ", current line is: " << line;
#endif

        // %-comments
        if (line[0]=='%') {
            Binder.prepare(line);

            if (Legacy0.match()) {
                p=Legacy0.getSToken(0);
                v=variantFromName(p);
                if (v!=REGULAR)
                    if (game)
                        game->Variant=v;
                continue;
            } // match Legacy0

            if (Legacy1.match()) {
                if (game) {
                    int a,b;
                    a = atoi(Legacy1.getNToken(0));
                    b = atoi(Legacy1.getNToken(1));
                    game->timecontrol.setIcs(a,b);
                    game->Rated=atoi(Legacy1.getNToken(2));
                }
                continue;
            } // match Legacy1

            if (Legacy2.match()) {
                g_strlcpy(omega,Legacy2.getStarToken(0),128);
                if (omega[strlen(omega)-1]=='\n')
                    omega[strlen(omega)-1]=0;
                continue;
            }

            continue;
        }

        for(p=line;*p;p++) {

            switch(state) {
                // whitespace
            case 10:
                if (*p == '[') { state=1; memset(alpha,0,256); ap=0; }
                break;
            case 0:
                if (*p == '[')         { state=1; memset(alpha,0,256); ap=0; break; }
            case 8:
                if (*p == '{')         { state=7; memset(alpha,0,256); ap=0; break; }

                if (*p == '(')         { state=11; ap=0; altmoves++; break; }

                if (strchr(token, *p)) {
                    state=6;
                    memset(alpha,0,256);
                    alpha[0]=*p;
                    ap=1;
                    break;
                }
                break; // case 0/8

                // header
            case 1:
                if (strchr(whitespace, *p)) { state=2; memset(beta,0,256); bp=0; break; }
                alpha[ap++]=*p;
                break; // case 1

            case 2:
                if (*p == '\"') state = 3;
                break; // case 2

            case 3:
                if (*p == '\"') state = 4; else beta[bp++]=*p;
                break; // case 3

            case 4:
                if (*p == ']') {
                    if (game) {
                        game->pgn.set(alpha,beta);
                        if (!strcmp(alpha,"Variant")) {
                            v=variantFromName(beta);
                            game->Variant=v;
                        }
                    }

                    if (!indexonly)
                        if (!strcmp(alpha,"FEN")) {
                            Position g;
                            g.setFEN(beta);
                            gl.clear();
                            gl.push_back(g);
                        }

                    state = 0;
                }
                break; // case 4

                // token
            case 6:
                if (strchr(whitespace,*p)) {
                    Binder2.prepare(alpha);

#ifdef PGN_DEBUG
                    cerr << "state 6 parsing token " << alpha << endl;
#endif

                    // move number token (1. , 1...)
                    if ( strchr(digits, alpha[0]) && strchr(alpha, '.') ) {
                        state=8;
                        // Account for PGNs where Black moves first
                        if (strstr(alpha, "...") && !halfmove) {
                            color = BLACK;
                            halfmove = 1;
                        }
#ifdef PGN_DEBUG
                        cerr << "v: movenumber\n";
#endif
                        break;
                    }

                    // end game token (1-0, 0-1, 1/2-1/2, *)
                    if (strchr(resultset, alpha[0]))
                        if ((alpha[0]=='*') || Result.match() ) {
#ifdef PGN_DEBUG
                            cerr << "v: end of game\n";
#endif

                            if (!strlen(omega)) strcpy(omega," ");
                            if (alpha[0]=='*') r=UNDEF; else
                                if (alpha[1]=='/') r=DRAW; else
                                    if (alpha[0]=='1') r=WHITE_WIN; else
                                        r=BLACK_WIN;
                            if (game) {
                                game->updateGame(gl);
                                game->endGame(omega,r);
                                game->guessInfoFromPGN();
                            } else
                                updatee->updateGame(gl);

                            if (game)
                                global.GameList.push_back(game);
                            state=9;
                            break;
                        }

                    if (alpha[0]=='$') {
                        int i;
                        t.set((char *) alpha);
                        if (! gl.empty() ) {
                            if (! global.annotator.isOpen() ) {
                                i = t.tokenvalue("$");
                                if (i < Glyphs.size()) {
                                    gl.back().addAnnotation( global.annotator.open() );
                                    global.annotator.append(* (Glyphs[i]));
                                    global.annotator.close();
                                }
                            } else {
                                i = t.tokenvalue("$");
                                if (i < Glyphs.size()) {
                                    global.annotator.append(* (Glyphs[i]));
                                }
                            }
                        }

                        state = 8;
                        break;
                    }

#ifdef PGN_DEBUG
                    cerr << "v: move text\n";
#endif
                    if (indexonly) { state=8; break; }

                    // move tokens
                    if (gl.empty()) gl.push_back(Position());

                    {
                        Position g;
                        g = gl.back();
                        g.clearAnnotation();

                        g.moveAnyNotation(alpha,color,game ? game->Variant : v);
                        snprintf(beta,256,"%d%s %s",1+halfmove/2,
                                 (color==WHITE)?".":". ...",alpha);
                        g.setLastMove(beta);
                        gl.push_back(g);
                    }

                    color^=COLOR_MASK;
                    halfmove++;
                    state=8;

                } else { // non whitespace char

                    // ignore move number in PGNs like 1.e4 (no space between . and move)
                    if ( ap && alpha[ap-1] == '.' )
                        if (strchr(token,*p)) {
                            memset(alpha,0,256);
                            ap=0;
                        }

                    alpha[ap++]=*p;

                    if (game)
                        if ( (*p == '@') && (game->Variant == REGULAR) ) {
                            f.seekg(pstart);
                            gl.clear();

                            delete game;
                            game=0;
                            return(ParsePgnGame(f,filename,indexonly,gameid,CRAZYHOUSE));
                        }

                }
                break; // case 6

                // { comments
            case 7:
                if (*p == '}') {
                    global.annotator.close();
                    state=8;
                    break;
                }

                if (! gl.empty() ) {
                    if (! global.annotator.isOpen() )
                        gl.back().addAnnotation( global.annotator.open() );
                    global.annotator.append(*p >= ' ' ? *p : ' ');
                }

                break; // case 7

            case 9: // game over, waiting end of line
                if (*p == '\n') {
                    gl.clear();
                    return true;
                }
                break; // case 9

            case 11: // Temporarily ignore alternate moves
                if (*p == ')')
                    if (!(--altmoves))
                        state = 8;
                if (*p == '(')
                    altmoves++;
                break;

            } // switch state

        } // for p

    } // while getline

    // avoid issues with files whose last char belongs to a token
    if (state == 9) {
        gl.clear();
        return true;
    }

    if (!gl.empty())
        gl.clear();

    if (game) delete game;
    return false;
} // method

bool ChessGame::loadMoves() {
    char z[512];
    if (Loaded) return true;

    zifstream f(PGNSource);

    if (!f) {
        snprintf(z,512,_("can't load PGN move text from %s (error opening file)"),PGNSource);
        global.output->append(z,0xc0ff00);
        return false;
    }

    if (!f.seekg(SourceOffset)) {
        snprintf(z,512,_("can't seek to offset %lu of %s"),SourceOffset,PGNSource);
        global.output->append(z,0xc0ff00);
        f.close();
        return false;
    }

    if (!ParsePgnGame(f,PGNSource,false,-1,Variant,this)) {
        g_strlcpy(z,_("error parsing PGN data"),512);
        global.output->append(z,0xc0ff00);
        f.close();
        return false;
    }
    f.close();

    Loaded = true;
    return true;
}

void ChessGame::LoadPGN(char *filename) {
    UnboundedProgressWindow *pw;
    int nextid,count=0;

    vector<int> g_ids;
    vector<int>::iterator ii;
    list<ChessGame *>::iterator gi;
    bool id_space_empty=false;

    zifstream f(filename);
    if (!f)
        return;

    //  cerr << "reading " << filename << endl;
    pw=new UnboundedProgressWindow(_("%d games read"));

    // grab a list of existent game ids
    for(gi=global.GameList.begin();gi!=global.GameList.end();gi++)
        g_ids.push_back( (*gi)->GameNumber );

    sort( g_ids.begin(), g_ids.end() );
    nextid = 10000;

    for(ii=g_ids.begin(); ii != g_ids.end(); ii++) {
        if ( *ii >  nextid ) break;
        if ( *ii == nextid ) ++nextid;
    }

    if ( ii == g_ids.end() )
        id_space_empty = true;

    while(!f.eof()) {

        if (ParsePgnGame(f,filename,true,nextid)) {

            ++count;
            ++nextid;
            if (!id_space_empty) {
                for( ; ii != g_ids.end(); ii++) {
                    if ( *ii >  nextid ) break;
                    if ( *ii == nextid ) ++nextid;
                }
                if ( ii == g_ids.end() ) id_space_empty = true;
            }

        } else
            break;

        if (! (count%5) ) pw->setProgress(count);
    }

    f.close();
    g_ids.clear();
    delete pw;
}

GameResult ChessGame::getResult() {
    return(over?result:UNDEF);
}

variant ChessGame::variantFromName(const char *p) {
    variant v=REGULAR;
    variant allv[10] = {CRAZYHOUSE, BUGHOUSE, WILD,   SUICIDE,    LOSERS,
                        GIVEAWAY,   ATOMIC,   WILDFR, WILDCASTLE, WILDNOCASTLE};
    int i;

    for(i=0;i<10;i++)
        if (!strcmp(p,variantName(allv[i])))
            v = allv[i];

    return v;
}

const char * ChessGame::variantName(variant v) {
    switch(v) {
    case REGULAR:      return("chess");
    case CRAZYHOUSE:   return("crazyhouse");
    case SUICIDE:      return("suicide");
    case BUGHOUSE:     return("bughouse");
    case WILD:         return("wild");
    case LOSERS:       return("losers");
    case GIVEAWAY:     return("giveaway");
    case ATOMIC:       return("atomic");
    case WILDFR:       return("fischerandom");
    case WILDCASTLE:   return("wildcastle");
    case WILDNOCASTLE: return("nocastle");
    default: return("unknown");
    }
}

char *ChessGame::getEndReason() {
    if (strlen(ereason)>2)
        return(ereason);
    else
        return 0;
}

// --------------- PGN classes

PGNpair::PGNpair() {
    name="none";
    value="empty";
}

PGNpair::PGNpair(const char *n, char *v) {
    name=n;
    value=v;
}

PGNpair::PGNpair(const char *n, string &v) {
    name=n;
    value=v;
}

PGNheader::~PGNheader() {
    header.clear();
}

void PGNheader::set(const char *n,char *v) {
    vector<PGNpair *>::iterator li;
    for(li=header.begin();li!=header.end();li++)
        if ( (*li)->name == n ) {
            (*li)->value = v;
            return;
        }
    header.push_back(new PGNpair(n,v));
}

void PGNheader::set(const char *n,string &v) {
    vector<PGNpair *>::iterator li;
    for(li=header.begin();li!=header.end();li++)
        if ( (*li)->name == n ) {
            (*li)->value = v;
            return;
        }
    header.push_back(new PGNpair(n,v));
}

void PGNheader::setIfAbsent(const char *n,char *v) {
    vector<PGNpair *>::iterator li;
    for(li=header.begin();li!=header.end();li++)
        if ( (*li)->name == n )
            return;
    header.push_back(new PGNpair(n,v));
}

void PGNheader::remove(const char *n) {
    vector<PGNpair *>::iterator li;
    for(li=header.begin();li!=header.end();li++)
        if ( (*li)->name == n ) {
            delete(*li);
            header.erase(li);
            break;
        }
}

int PGNheader::empty() {
    return(header.empty());
}

int PGNheader::size() {
    return(header.size());
}

PGNpair * PGNheader::get(int index) {
    return(header[index]);
}

const char * PGNheader::get(const char *n) {
    vector<PGNpair *>::iterator li;

    for(li=header.begin();li!=header.end();li++) {
        if ( (*li)->name == n )
            return((*li)->value.c_str());
    }
    return 0;
}

// -- PGN info edit dialog ------

PGNEditInfoDialog::PGNEditInfoDialog(ChessGame *src) :
    ModalDialog(N_("PGN Headers"))
{
    GtkWidget *v,*sw,*hb,*hb2,*l[2],*setb,*hs,*bb,*closeb;
    obj=src;

    gtk_window_set_default_size(GTK_WINDOW(widget),270,300);
    v=gtk_vbox_new(FALSE,4);
    gtk_container_add(GTK_CONTAINER(widget),v);

    sw=gtk_scrolled_window_new(0,0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

    clist=gtk_clist_new(2);
    gtk_widget_set_style(clist, gtk_widget_get_default_style() );
    gtk_clist_set_column_min_width(GTK_CLIST(clist),0,64);
    gtk_clist_set_column_min_width(GTK_CLIST(clist),1,64);
    gtk_clist_set_shadow_type(GTK_CLIST(clist),GTK_SHADOW_IN);
    gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
    gtk_clist_set_column_title(GTK_CLIST(clist),0,_("Key"));
    gtk_clist_set_column_title(GTK_CLIST(clist),1,_("Value"));
    gtk_clist_column_titles_passive(GTK_CLIST(clist));
    gtk_clist_column_titles_show(GTK_CLIST(clist));

    gtk_box_pack_start(GTK_BOX(v),sw,TRUE,TRUE,0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw),clist);

    hb=gtk_hbox_new(FALSE,0);
    del=gtk_button_new_with_label(_(" Remove Field "));
    gtk_box_pack_end(GTK_BOX(hb),del,FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(v),hb,FALSE,FALSE,4);

    hb2=gtk_hbox_new(FALSE,0);
    l[0]=gtk_label_new(_("Key:"));
    l[1]=gtk_label_new(_("Value:"));
    en[0]=gtk_entry_new();
    en[1]=gtk_entry_new();

    setb=gtk_button_new_with_label(_(" Set "));

    gtk_box_pack_start(GTK_BOX(hb2),l[0],FALSE,FALSE,4);
    gtk_box_pack_start(GTK_BOX(hb2),en[0],TRUE,TRUE,4);
    gtk_box_pack_start(GTK_BOX(hb2),l[1],FALSE,FALSE,4);
    gtk_box_pack_start(GTK_BOX(hb2),en[1],TRUE,TRUE,4);
    gtk_box_pack_start(GTK_BOX(hb2),setb,FALSE,FALSE,4);
    gtk_box_pack_start(GTK_BOX(v),hb2,FALSE,FALSE,4);

    hs=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,4);

    bb=gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bb), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bb), 5);
    gtk_box_pack_start(GTK_BOX(v),bb,FALSE,FALSE,2);

    closeb=gtk_button_new_with_label(_("Close"));
    GTK_WIDGET_SET_FLAGS(closeb,GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bb),closeb,TRUE,TRUE,0);
    gtk_widget_grab_default(closeb);

    Gtk::show(closeb,bb,hs,l[0],l[1],en[0],en[1],setb,hb2,
              hb,del,sw,clist,v,NULL);
    setDismiss(GTK_OBJECT(closeb),"clicked");

    populate();

    gtk_widget_set_sensitive(del,FALSE);
    Selection=-1;

    gtk_signal_connect(GTK_OBJECT(setb),"clicked",
                       GTK_SIGNAL_FUNC(pgnedit_set),(gpointer)this);
    gtk_signal_connect(GTK_OBJECT(del),"clicked",
                       GTK_SIGNAL_FUNC(pgnedit_del),(gpointer)this);
    gtk_signal_connect(GTK_OBJECT(clist),"select_row",
                       GTK_SIGNAL_FUNC(pgnedit_rowsel),(gpointer)this);
    gtk_signal_connect(GTK_OBJECT(clist),"unselect_row",
                       GTK_SIGNAL_FUNC(pgnedit_rowunsel),(gpointer)this);
}

void PGNEditInfoDialog::populate() {
    int i,j;
    const char *p[2];
    PGNpair *pp;

    gtk_clist_freeze(GTK_CLIST(clist));
    gtk_clist_clear(GTK_CLIST(clist));

    j=obj->pgn.size();
    for(i=0;i<j;i++) {
        pp=obj->pgn.get(i);
        p[0]=pp->name.c_str();
        p[1]=pp->value.c_str();
        gtk_clist_append(GTK_CLIST(clist),(gchar **)p);
    }
    gtk_clist_thaw(GTK_CLIST(clist));
    Selection=-1;
    gtk_widget_set_sensitive(del,FALSE);
}

void pgnedit_set(GtkWidget *w, gpointer data) {
    PGNEditInfoDialog *me;
    me=(PGNEditInfoDialog *)data;
    char a[64],b[64];
    g_strlcpy(a,gtk_entry_get_text(GTK_ENTRY(me->en[0])),64);
    g_strlcpy(b,gtk_entry_get_text(GTK_ENTRY(me->en[1])),64);
    if (strlen(a) && strlen(b)) {
        me->obj->pgn.set((const char *)a,b);
        me->populate();
    }
}

void pgnedit_del(GtkWidget *w, gpointer data) {
    PGNEditInfoDialog *me;
    const char *k;
    me=(PGNEditInfoDialog *)data;
    if (me->Selection >= 0) {
        k=(me->obj->pgn.get(me->Selection))->name.c_str();
        me->obj->pgn.remove(k);
        me->populate();
    }
}

void pgnedit_rowsel(GtkCList *w, gint row, gint col,
                    GdkEventButton *eb,gpointer data) {
    PGNEditInfoDialog *me;
    PGNpair *pp;
    me=(PGNEditInfoDialog *)data;
    me->Selection=row;
    gtk_widget_set_sensitive(me->del,TRUE);

    pp=me->obj->pgn.get(row);
    if (pp) {
        gtk_entry_set_text(GTK_ENTRY(me->en[0]),pp->name.c_str());
        gtk_entry_set_text(GTK_ENTRY(me->en[1]),pp->value.c_str());
    }
}

void pgnedit_rowunsel(GtkCList *w, gint row, gint col,
                      GdkEventButton *eb,gpointer data) {
    PGNEditInfoDialog *me;
    me=(PGNEditInfoDialog *)data;
    me->Selection=-1;
    gtk_widget_set_sensitive(me->del,FALSE);
}

void ChessGame::initGlyphs() {
    if (GlyphsInited)
        return;

    EboardFileFinder eff;
    string nag, nagpath;
    char line[512];
    int i;

    // fix to look for NLS versions
    nag = "NAG.en.txt";

    if (!eff.find(nag, nagpath)) {
        failGlyphs();
        return;
    }

    ifstream nagf(nagpath.c_str());

    if (!nagf) {
        failGlyphs();
        return;
    }

    memset(line,0,512);
    while(nagf.getline(line,511,'\n')) {
        if (!strlen(line))
            break;
        Glyphs.push_back(new string(line));
    }

    nagf.close();
    GlyphsInited = true;
}

void ChessGame::failGlyphs() {
    int i;
    string *oops;
    oops = new string("PGN NAG file missing");
    for(i=0;i<140;i++)
        Glyphs.push_back(oops);
    GlyphsInited = true;
}
