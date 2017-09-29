/* $Id: proto_p2p.h,v 1.7 2007/01/08 21:46:13 bergo Exp $ */

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

#ifndef EBOARD_PROTO_P2P_H
#define EBOARD_PROTO_P2P_H 1

#include "eboard.h"
#include "widgetproxy.h"
#include "util.h"
#include "chess.h"

#define P2P_GAME 8888

class P2PProtocol;

class GameProposal {
 public:
  GameProposal();
  GameProposal operator=(GameProposal &g);

  bool        AmWhite;
  TimeControl timecontrol;
  variant     Variant;
}; 

class P2PPad : public NonModalDialog {
 public:
  P2PPad(P2PProtocol *_proto);
  virtual ~P2PPad();

  void setProposal(GameProposal &g);
  void setDrawProp();
  void resetDrawProp();

 private:
  P2PProtocol *proto;
  DropBox *color;
  GtkWidget *wtime, *winc, *wprop, *wacc;
  BoxedLabel *bl[3];

  bool PropIsDraw;

  void clearProposal();

  friend void p2ppad_accept(GtkWidget *w, gpointer data);
  friend void p2ppad_propose(GtkWidget *w, gpointer data);
};

void p2ppad_accept(GtkWidget *w, gpointer data);
void p2ppad_propose(GtkWidget *w, gpointer data);

class P2PProtocol : public Protocol {
 public:
  P2PProtocol();
  virtual ~P2PProtocol();

  void receiveString(char *netstring);
  void sendMove(int x1,int y1,int x2,int y2,int prom);
  void sendUserInput(char *line);
  void finalize();

  virtual bool requiresLegalityChecking();

  void resign();
  void draw();
  void adjourn();
  void abort();

  void hail();

  void sendProposal(GameProposal &g);
  void acceptProposal();

  char *getRemotePlayer();

 private:

  void sendId();
  void createGame(GameProposal &g);
  void gameOver(GameResult gr, char *desc);

  PatternBinder     Binder;
  ExtPatternMatcher Chat, ProtoGreet, NameGreet, 
    GameProp, Accept, Move, Outcome, DrawOffer;

  char *tmpbuf;

  char RemotePlayer[64];
  char RemoteSoftware[128];
  int  RemoteProtover;

  bool IdSent, IdReceived;

  GameProposal MyProp, HisProp, Current;
  bool GotProp, GotDrawProp, SentDrawProp;

  int        MoveNumber;
  ChessGame *MyGame;

  P2PPad *pad;
};

#endif
