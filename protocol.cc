/* $Id: protocol.cc,v 1.28 2007/06/09 11:35:06 bergo Exp $ */

/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2002 Felipe Paulo Guazzi Bergo
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



#include "protocol.h"
#include "global.h"

Protocol::~Protocol() {

}

void Protocol::finalize() {

}

int  Protocol::hasAuthenticationPrompts() {
  return 0;
}

bool Protocol::requiresLegalityChecking() {
  return false;
}

void Protocol::discardGame(int gameid) {

}

void Protocol::queryGameList(GameListConsumer *glc) {
  glc->endOfList();
}

void Protocol::queryAdList(GameListConsumer *glc) {
  glc->endOfList();
}

void Protocol::observe(int gameid) {

}

void Protocol::answerAd(int adid) {

}

void Protocol::exaForward(int n) {

}

void Protocol::exaBackward(int n) {

}

void Protocol::updateVar(ProtocolVar pv) {

}

void Protocol::sendDrop(piece p,int x,int y) {
  // not all protocols need to support {bug,crazy}house
}

void Protocol::retractMove() {
  global.status->setText(_("Sorry, this protocol does not allow to retract a move thru this menu option."), 15);  
}

// the default implementation just sends the line as is.
// only the P2P protocol must encode chat lines
void Protocol::sendUserInput(char *line) {
  if (global.network)
    if (global.network->isConnected())
      global.network->writeLine(line);
}

void Protocol::refreshSeeks(bool create) {
  // not all protocols need to manage a seek table
}

vector<string *> * Protocol::getPlayerActions() {
  return NULL;
}

vector<string *> * Protocol::getGameActions() {
  return NULL;
}

void Protocol::callPlayerAction(char *player, string *action) {

}

void Protocol::callGameAction(int gameid, string *action) {

}

void NullProtocol::receiveString(char *netstring) {
  global.output->append(netstring,0xffffff);
}

void NullProtocol::sendMove(int x1,int y1,int x2,int y2,int prom) {

}

void NullProtocol::resign() {

}

void NullProtocol::draw() {

}

void NullProtocol::adjourn() {

}

void NullProtocol::abort() {

}


