/* $Id: protocol.h,v 1.29 2007/06/09 11:35:06 bergo Exp $ */

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



#ifndef EBOARD_PROTOCOL_H
#define EBOARD_PROTOCOL_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eboard.h"
#include "util.h"
#include "tstring.h"
#include "position.h"

typedef enum {
	PV_premove
} ProtocolVar;

class Protocol {
 public:
	virtual ~Protocol();
	virtual void receiveString(char *netstring)=0;
	virtual void sendMove(int x1,int y1,int x2,int y2,int prom)=0;
	virtual void sendDrop(piece p,int x,int y);
	virtual void finalize();
	virtual int  hasAuthenticationPrompts();
	virtual bool requiresLegalityChecking();

	virtual void sendUserInput(char *line);

	virtual void resign()=0;
	virtual void draw()=0;
	virtual void adjourn()=0;
	virtual void abort()=0;
	virtual void retractMove();

	virtual void exaForward(int n);
	virtual void exaBackward(int n);

	virtual void discardGame(int gameid);
	virtual void queryGameList(GameListConsumer *glc);
	virtual void queryAdList(GameListConsumer *glc);
	virtual void observe(int gameid);
	virtual void answerAd(int adid);

	virtual void refreshSeeks(bool create);

	virtual void updateVar(ProtocolVar pv);

	// meant for right-click menu on the clock area
	virtual vector<string *> * getPlayerActions();
	virtual vector<string *> * getGameActions();

	virtual void callPlayerAction(char *player, string *action);
	virtual void callGameAction(int gameid, string *action);
};

class NullProtocol : public Protocol {
 public:
	void receiveString(char *netstring);
	void sendMove(int x1,int y1,int x2,int y2,int prom);

	virtual void resign();
	virtual void draw();
	virtual void adjourn();
	virtual void abort();
};

class EngineBookmark;

class EngineProtocol : public Protocol {
 public:

	// both run methods should return 0 in case of failure, anything
	// else in case of success

	// should open a dialog asking parameters for the engine
	virtual int run()=0;

	// should run the engine with parameters from the bookmark
	virtual int run(EngineBookmark *bm)=0;

	virtual void setInitialPosition(Position *p)=0;

};

#include "proto_fics.h"
#include "proto_p2p.h"
#include "proto_xboard.h"

#endif
