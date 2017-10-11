/* $Id: proto_p2p.cc,v 1.12 2008/02/08 14:25:51 bergo Exp $ */

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
#include "protocol.h"
#include "position.h"
#include "chess.h"
#include "global.h"
#include "tstring.h"

P2PProtocol::P2PProtocol() {
	tmpbuf = (char *) malloc(2048);

	Chat.set("C-*");
	ProtoGreet.set("P-(*)-(*)-(*)*"); /* (EBOARDP2P)-(software)-(protover) */
	NameGreet.set("N-(*)*");
	GameProp.set("G-(*)-(*)-(*)-(*)*"); /* amwhite / time / inc / variant */
	Accept.set("A-G");
	Move.set("M-(*)");
	Outcome.set("R-(*)-(*)*");
	DrawOffer.set("D-O");

	Binder.add(&Chat, &ProtoGreet, &NameGreet,
			   &GameProp, &Accept, &Move, &Outcome,
			   &DrawOffer, NULL);

	IdSent = IdReceived = false;

	strcpy(RemotePlayer,"Remote");
	strcpy(RemoteSoftware, "Software");
	RemoteProtover = 1;

	pad = 0;
	GotProp = false;
	GotDrawProp = SentDrawProp = false;
	MyGame = 0;
}

P2PProtocol::~P2PProtocol() {
	if (tmpbuf)
		free(tmpbuf);
	tmpbuf = 0;
}

bool P2PProtocol::requiresLegalityChecking() {
	return true;
}

void P2PProtocol::finalize() {
	if (pad) {
		pad->release();
		pad = 0;
	}
}

void P2PProtocol::hail() {
	sendId();
}

void P2PProtocol::sendId() {
	char z[128];
	if (!IdSent)
		if (global.network)
			if (global.network->isConnected()) {
				snprintf(z,128,"P-(EBOARDP2P)-(eboard %s)-(1)",global.Version);
				global.network->writeLine(z);
				snprintf(z,128,"N-(%s)",global.P2PName);
				global.network->writeLine(z);
				IdSent = true;
			}
}

void P2PProtocol::sendProposal(GameProposal &g) {
	char z[128];
	if (global.network)
		if (global.network->isConnected()) {
			snprintf(z,128,"G-(%d)-(%d)-(%d)-(%s)",
					 g.AmWhite ? 0 : 1,
					 g.timecontrol.value[0], g.timecontrol.value[1], ChessGame::variantName(g.Variant));
			global.network->writeLine(z);
			MyProp = g;
			global.status->setText(_("Game proposal sent."),10);
		}
}

void P2PProtocol::acceptProposal() {
	char z[64];

	if (GotDrawProp) {
		draw();
		return;
	}

	if (GotProp) {

		if (MyGame) {
			global.status->setText(_("Finish the current game first."),10);
			return;
		}
		strcpy(z,"A-G");
		if (global.network)
			if (global.network->isConnected())
				global.network->writeLine(z);
		GotProp = false;
		createGame(HisProp);
	}
}

void P2PProtocol::receiveString(char *netstring) {
	int j;
	char *p, z[256], y[128];

	j = strlen(netstring);
	if (j<2) return;
	if (netstring[1] != '-') return;

	Binder.prepare(netstring);

	if (Chat.match()) {
		global.output->append(Chat.getStarToken(0),
							  global.Colors.PrivateTell,
							  IM_PERSONAL);
		return;
	}

	if (MyGame && Move.match()) {
		Position cur;
		int wc, bc;

		cur  = MyGame->getLastPosition();
		cur.SANstring(Move.getStarToken(0), y);
		cur.moveAnyNotation(Move.getStarToken(0),
							Current.AmWhite ? BLACK : WHITE,
							Current.Variant);
		if (Current.AmWhite)
			snprintf(z,256,"%d. ... %s",MoveNumber,y);
		else
			snprintf(z,256,"%d. %s",MoveNumber,y);

		if (MoveNumber == 1)
			wc = bc = 1000 * Current.timecontrol.value[0];
		else {
			MyGame->incrementActiveClock(Current.timecontrol.value[1]);
			wc = bc = CLOCK_UNCHANGED;
		}

		MyGame->updatePosition2(cur, MoveNumber,
								cur.sidehint?0:1,
								wc, bc, z, true);

		if (Current.AmWhite) ++MoveNumber;
		global.BoardList.front()->setCanMove(true);
		global.opponentMoved();

		if (GotDrawProp) pad->resetDrawProp();
		GotDrawProp = SentDrawProp = false;
		return;
	}

	if (ProtoGreet.match()) {
		p = ProtoGreet.getStarToken(0);
		if (strcmp(p, "EBOARDP2P")!=0) {
			global.status->setText(_("Protocol mismatch, disconnecting."),10);
			global.network->close();
			return;
		}
		p = ProtoGreet.getStarToken(1);
		memset(RemoteSoftware,0,128);
		g_strlcpy(RemoteSoftware,p,128);

		RemoteProtover = atoi(ProtoGreet.getStarToken(2));
		if (RemoteProtover < 1) {
			global.status->setText(_("Protocol mismatch, disconnecting."),10);
			global.network->close();
			return;
		}

		snprintf(z,256,"Remote software: %s (eboard/p2p v.%d)",
				 RemoteSoftware,RemoteProtover);
		global.output->append(z,global.Colors.TextBright,IM_NORMAL);
		IdReceived = true;
		sendId();

		if (!pad) {
			pad = new P2PPad(this);
			pad->show();
		}

		return;
	}

	if (NameGreet.match()) {
		memset(RemotePlayer,0,64);
		g_strlcpy(RemotePlayer,NameGreet.getStarToken(0),64);
		snprintf(z,256,"Remote player name: %s",
				 RemotePlayer);
		global.output->append(z,global.Colors.TextBright,IM_NORMAL);
		sendId();
		return;
	}

	if (GameProp.match()) {
		GameProposal g;
		int a,b;

		g.AmWhite = (atoi(GameProp.getStarToken(0)) != 0);
		a = atoi(GameProp.getStarToken(1));
		b = atoi(GameProp.getStarToken(2));
		g.timecontrol.setIcs(a,b);
		g.Variant = ChessGame::variantFromName(GameProp.getStarToken(3));

		HisProp = g;
		GotProp = true;
		if (pad)
			pad->setProposal(g);
		snprintf(z,256,_("Received a game proposal from %s."),RemotePlayer);
		global.output->append(z,global.Colors.TextBright,IM_NORMAL);
		return;
	}

	if (Accept.match()) {
		snprintf(z,256,_("%s accepted your game proposal."),RemotePlayer);
		global.output->append(z,global.Colors.TextBright,IM_NORMAL);
		createGame(MyProp);
		return;
	}

	if (DrawOffer.match()) {
		if (SentDrawProp) {
			draw();
			return;
		} else {
			char z[128];
			pad->setDrawProp();
			snprintf(z,128,_("%s offers a draw."),RemotePlayer);
			global.output->append(z,global.Colors.TextBright,IM_PERSONAL);
			GotDrawProp = true;
			GotProp = false;
			return;
		}
	}

	if (Outcome.match()) {
		p=Outcome.getStarToken(0);
		y[0] = *p;
		p=Outcome.getStarToken(1);
		y[1] = *p;

		switch(y[1]) {
		case 'S': // stalemate
			gameOver(DRAW, _("Stalemate"));
			return;
		case 'N': // no material
			gameOver(DRAW, _("No material to mate"));
			return;
		case 'M': // checkmate
			gameOver(y[0]=='W'?WHITE_WIN:BLACK_WIN,_("Checkmate"));
			return;
		case 'R': // resign
			gameOver(y[0]=='W'?WHITE_WIN:BLACK_WIN,_("Player resigns"));
			return;
		case 'A': //abort
			gameOver(UNDEF,_("Game Aborted"));
			return;
		case 'D': // draw by agreement
			gameOver(DRAW, _("Drawn by agreement"));
			return;
		default:
			gameOver(UNDEF, _("Unknown result"));
			return;
		}
	}

	if (strlen(netstring) < 200)
		snprintf(z,256,"Got garbage: [%s]",netstring);
	else
		snprintf(z,256,"Got too much garbage.");
	global.output->append(z,global.Colors.Engine,IM_NORMAL);
}

void P2PProtocol::sendUserInput(char *line) {
	int j;

	memset(tmpbuf,0,2048);
	snprintf(tmpbuf,2048,"C-%s> ",global.P2PName);
	j = strlen(line);
	strncat(tmpbuf, line, 1900);
	if (global.network)
		if (global.network->isConnected())
			global.network->writeLine(tmpbuf);
}

void P2PProtocol::createGame(GameProposal &g) {
	ChessGame *cg;
	Position *p;

	Current = g;

	cg = new ChessGame();
	cg->source = GS_Other;
	cg->clock_regressive = 1;
	cg->GameNumber=global.nextFreeGameId(P2P_GAME+1);
	g_strlcpy(cg->PlayerName[g.AmWhite?0:1],global.P2PName,64);
	g_strlcpy(cg->PlayerName[g.AmWhite?1:0],RemotePlayer,64);
	cg->MyColor = g.AmWhite ? WHITE : BLACK;
	cg->Rated = 0;
	cg->Variant = g.Variant;
	cg->timecontrol = g.timecontrol;
	cg->AmPlaying = true;

	global.prependGame(cg);
	global.BoardList.front()->reset();

	cg->setBoard(global.BoardList.front());
	global.BoardList.front()->setGame(cg);
	global.BoardList.front()->pop();
	global.BoardList.front()->setCanMove(g.AmWhite);
	global.BoardList.front()->repaint();
	global.BoardList.front()->setFlipped(!g.AmWhite);

	cg->acknowledgeInfo();
	cg->fireWhiteClock(g.timecontrol.value[0], g.timecontrol.value[1]);

	p = new Position();
	cg->updatePosition2(*p,1,p->sidehint ? 0:1,
						1000*g.timecontrol.value[0],
						1000*g.timecontrol.value[0],
						"0. startpos");
	delete p;

	global.status->setText(_("Game started!"),10);
	global.gameStarted();

	MyGame = cg;
	MoveNumber = 1;
}

void P2PProtocol::sendMove(int x1,int y1,int x2,int y2,int prom) {
	char move[7], xm[12], san[16], xsan[20];
	piece pp;
	Position cur;
	int wc,bc;

	global.debug("P2PProtocol","sendMove");

	if (!MyGame)
		return;

	pp = prom ? global.promotion->getPiece() : EMPTY;

	cur = MyGame->getLastPosition();

	if (!cur.isMoveLegalCartesian(x1,y1,x2,y2,
								  Current.AmWhite?WHITE:BLACK,
								  Current.Variant)) {
		global.status->setText(_("Illegal move, not sent."), 10);
		return;
	}

	cur.stdNotationForMove(x1,y1,x2,y2,pp,san,MyGame->Variant);
	cur.moveCartesian(x1,y1,x2,y2,MyGame->Variant,true);

	if (Current.AmWhite)
		snprintf(xsan,20,"%d. %s",MoveNumber,san);
	else
		snprintf(xsan,20,"%d. ... %s",MoveNumber,san);

	if (MoveNumber == 1)
		wc = bc = 1000 * Current.timecontrol.value[0];
	else {
		wc = bc = CLOCK_UNCHANGED;
		MyGame->incrementActiveClock(Current.timecontrol.value[1]);
	}

	if (!Current.AmWhite) ++MoveNumber;

	MyGame->updatePosition2(cur,MoveNumber,
							cur.sidehint ? 0:1, wc,bc, xsan);

	/* -- */

	move[4]=0;
	move[5]=0;
	move[6]=0;
	move[0]='a'+x1;
	move[1]='1'+y1;
	move[2]='a'+x2;
	move[3]='1'+y2;

	if (prom) {
		pp=global.promotion->getPiece();
		move[4]='=';
		switch(pp) {
		case QUEEN:   move[5]='Q'; break;
		case ROOK:    move[5]='R'; break;
		case BISHOP:  move[5]='B'; break;
		case KNIGHT:  move[5]='N'; break;
		case KING:    move[5]='K'; break;
		}
	}
	snprintf(xm,12,"M-(%s)",move);
	global.network->writeLine(xm);
	global.BoardList.front()->setCanMove(false);

	/* check if move ends the game */

	if (cur.isStalemate(Current.AmWhite?BLACK:WHITE,Current.Variant)) {
		snprintf(xm,12,"R-(D)-(S)");
		global.network->writeLine(xm);
		gameOver(DRAW, _("Stalemate"));
		return;
	}
	if (cur.isNMDraw(Current.Variant)) {
		snprintf(xm,12,"R-(D)-(N)");
		global.network->writeLine(xm);
		gameOver(DRAW, _("No material to mate"));
		return;
	}
	if (cur.isMate(Current.AmWhite?BLACK:WHITE,Current.Variant)) {
		snprintf(xm,12,"R-(%c)-(M)", Current.AmWhite?'W':'B');
		global.network->writeLine(xm);
		gameOver(Current.AmWhite?WHITE_WIN:BLACK_WIN, _("Checkmate"));
		return;
	}

	if (GotDrawProp) pad->resetDrawProp();
	GotDrawProp = SentDrawProp = false;
}

void P2PProtocol::gameOver(GameResult gr, char *desc) {
	char z[256];
	if (!MyGame) return;
	MyGame->endGame(desc, gr);

	snprintf(z,256,_("Game over: %s"), desc);
	global.output->append(z, global.Colors.TextBright,
						  IM_NORMAL);

	if (global.AppendPlayed) {
		if  (MyGame->savePGN(global.AppendFile,true)) {
			snprintf(z,256,_("Game appended to %s"),global.AppendFile);
			global.status->setText(z,10);
		}
	}

	if ((gr==WHITE_WIN && Current.AmWhite)||
		(gr==BLACK_WIN && !Current.AmWhite))
		global.gameWon();

	if ((gr==WHITE_WIN && !Current.AmWhite)||
		(gr==BLACK_WIN && Current.AmWhite))
		global.gameLost();

	MyGame = 0;
	GotDrawProp = SentDrawProp = false;
}

void P2PProtocol::resign() {
	char z[64];
	if (MyGame) {
		snprintf(z,64,"R-(%c)-(R)",Current.AmWhite?'B':'W');
		global.network->writeLine(z);
		gameOver(Current.AmWhite?BLACK_WIN:WHITE_WIN,_("Player resigns"));
	}
}

void P2PProtocol::draw() {
	char z[128];
	if (GotDrawProp) {
		gameOver(DRAW,_("Drawn by agreement"));
		global.network->writeLine("R-(D)-(D)");
	} else {
		global.network->writeLine("D-O");
		SentDrawProp = true;
		g_strlcpy(z,_("Draw offer sent."),128);
		global.status->setText(z,5);
	}
}

void P2PProtocol::adjourn() {
	global.status->setText("Adjournment is not supported.",10);
}

void P2PProtocol::abort() {
	char z[64];
	if (MyGame) {
		snprintf(z,64,"R-(D)-(A)");
		global.network->writeLine(z);
		gameOver(UNDEF,_("Game Aborted"));
	}
}

char * P2PProtocol::getRemotePlayer() { return(RemotePlayer); }

/* =================================== */

P2PPad::P2PPad(P2PProtocol *_proto) : NonModalDialog("Direct Connection") {
	GtkWidget *v, *t, *f, *h, *match, *f2, *v2, *h2;
	int i;

	proto = _proto;
	setCloseable(false);
	PropIsDraw = false;

	v = gtk_vbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(widget), v);

	t = gtk_table_new(4,4,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(t), 4);
	gtk_table_set_col_spacings(GTK_TABLE(t), 4);
	gtk_container_set_border_width(GTK_CONTAINER(t), 4);

	f = gtk_frame_new(_("Propose Game"));
	gtk_frame_set_shadow_type(GTK_FRAME(f), GTK_SHADOW_ETCHED_IN);
	gtk_container_set_border_width(GTK_CONTAINER(f), 4);

	gtk_box_pack_start(GTK_BOX(v), f, TRUE, TRUE, 2);
	gtk_container_add(GTK_CONTAINER(f), t);

	bl[0] = new BoxedLabel(_("Your color:"));
	bl[1] = new BoxedLabel(_("Initial time ([mm:]ss):"));
	bl[2] = new BoxedLabel(_("Increment (secs):"));

	color   = new DropBox(_("White"),
						  _("Black"),NULL);

	wtime   = gtk_entry_new_with_max_length(8);
	winc    = gtk_entry_new_with_max_length(4);

	gtk_entry_set_text(GTK_ENTRY(wtime),"45:00");
	gtk_entry_set_text(GTK_ENTRY(winc),"0");

	gtk_table_attach_defaults(GTK_TABLE(t), bl[0]->widget, 0,1, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(t), color->widget, 1,2, 0,1);
	gtk_table_attach_defaults(GTK_TABLE(t), bl[1]->widget, 0,1, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(t), wtime, 1,2, 1,2);
	gtk_table_attach_defaults(GTK_TABLE(t), bl[2]->widget, 0,1, 2,3);
	gtk_table_attach_defaults(GTK_TABLE(t), winc, 1,2, 2,3);

	h=gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(h), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(h), 5);

	gtk_table_attach_defaults(GTK_TABLE(t), h, 0,2, 3,4);

	match = gtk_button_new_with_label(_("Propose"));
	gtk_box_pack_start(GTK_BOX(h), match, TRUE, TRUE, 0);

	gtk_signal_connect(GTK_OBJECT(match),"clicked",
					   GTK_SIGNAL_FUNC(p2ppad_propose), (gpointer) this);

	color->show();
	for(i=0;i<3;i++) bl[i]->show();

	f2 = gtk_frame_new(_("Last Proposal Received"));
	gtk_frame_set_shadow_type(GTK_FRAME(f2), GTK_SHADOW_ETCHED_IN);
	gtk_container_set_border_width(GTK_CONTAINER(f2), 4);
	gtk_box_pack_start(GTK_BOX(v), f2, TRUE, TRUE, 2);

	v2 = gtk_vbox_new(FALSE,4);
	gtk_container_add(GTK_CONTAINER(f2), v2);

	wprop = gtk_label_new(_("No proposals received."));
	gtk_label_set_justify(GTK_LABEL(wprop), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(v2), wprop, TRUE, TRUE, 2);

	h2=gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(h2), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(h2), 5);
	gtk_box_pack_start(GTK_BOX(v2), h2, FALSE, TRUE, 2);

	wacc = gtk_button_new_with_label(_("Accept"));
	gtk_box_pack_start(GTK_BOX(h2), wacc, TRUE, TRUE, 0);

	gtk_widget_set_sensitive(wacc, FALSE);

	gtk_signal_connect(GTK_OBJECT(wacc),"clicked",
					   GTK_SIGNAL_FUNC(p2ppad_accept), (gpointer) this);

	Gtk::show(wtime,winc,f,t,h,match, wacc, h2, wprop, v2, f2, v, NULL);
}

P2PPad::~P2PPad(){
	int i;
	for(i=0;i<3;i++)
		delete(bl[i]);
	delete color;
}

void P2PPad::setProposal(GameProposal &g) {
	char z[512];

	snprintf(z,512,_("%s (white) vs. %s (black)\n%s\n%d:%.2d %d"),
			 g.AmWhite   ? global.P2PName : proto->getRemotePlayer(),
			 (!g.AmWhite) ? global.P2PName : proto->getRemotePlayer(),
			 ChessGame::variantName(g.Variant),
			 g.timecontrol.value[0] / 60,
			 g.timecontrol.value[0] % 60,
			 g.timecontrol.value[1]);
	gtk_widget_set_sensitive(wacc, TRUE);
	gtk_label_set_text(GTK_LABEL(wprop), z);
	gtk_widget_queue_resize(wprop);
	gtk_widget_queue_resize(widget);
	PropIsDraw = false;
}

void P2PPad::setDrawProp() {
	char z[128];
	snprintf(z,128,_("%s offers a draw."),proto->getRemotePlayer());
	gtk_widget_set_sensitive(wacc, TRUE);
	gtk_label_set_text(GTK_LABEL(wprop), z);
	gtk_widget_queue_resize(wprop);
	gtk_widget_queue_resize(widget);
	PropIsDraw = true;
	global.drawOffered();
}

void P2PPad::resetDrawProp() {
	clearProposal();
}

void P2PPad::clearProposal() {
	char z[64];
	g_strlcpy(z,_("No proposals left."),64);
	gtk_widget_set_sensitive(wacc, FALSE);
	gtk_label_set_text(GTK_LABEL(wprop), z);
	gtk_widget_queue_resize(wprop);
	gtk_widget_queue_resize(widget);
	PropIsDraw = false;
}

void p2ppad_accept(GtkWidget *w, gpointer data) {
	P2PPad *me = (P2PPad *) data;
	if (global.protocol)
		me->proto->acceptProposal();
	me->clearProposal();
}

void p2ppad_propose(GtkWidget *w, gpointer data) {
	P2PPad *me = (P2PPad *) data;
	GameProposal g;
	char z[64];
	int T,a,b;
	tstring t;
	string *p;

	t.set(gtk_entry_get_text(GTK_ENTRY(me->wtime)));
	a = 0;
	while((p=t.token(":"))!=0)
		a = (60*a) + atoi(p->c_str());

	b = atoi(gtk_entry_get_text(GTK_ENTRY(me->winc)));
	g.timecontrol.setIcs(a,b);
	g.Variant = REGULAR;
	g.AmWhite = (me->color->getSelection() == 0);

	if (global.protocol)
		me->proto->sendProposal(g);
}

GameProposal::GameProposal() {
	AmWhite = false;
	timecontrol.mode = TC_NONE;
	Variant = REGULAR;
}

GameProposal GameProposal::operator=(GameProposal &g) {
	AmWhite     = g.AmWhite;
	timecontrol = g.timecontrol;
	Variant     = g.Variant;
}
