/* $Id: proto_xboard.cc,v 1.52 2008/02/08 14:25:51 bergo Exp $ */

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

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include "eboard.h"
#include "global.h"
#include "network.h"
#include "protocol.h"
#include "chess.h"
#include "status.h"
#include "util.h"
#include "stl.h"
#include "tstring.h"

XBoardProtocol::XBoardProtocol() {
	EngineWhite=0;
	MoveNumber=1;
	MoveBlack=0;
	Variant=REGULAR;
	want_path_pane=1;
	supports_setboard=false;
	requires_usermove=false;
	timeout_id=-1;
	need_handshake=true; // send xboard/protover 2
	Finalized = false;
	InitDone  = false;

	got_init_pos = false;
	just_sent_fen = false;

	timecontrol.setSecondsPerMove(20);

	g_strlcpy(ComputerName,_("Computer"),64);
	strcpy(EngineCommandLine,"gnuchess");
	strcpy(EngineRunDir,"~/.eboard/eng-out");

	Features.set("*feature*");
	IllegalMove.set("Illegal move*");

	Moved[0].set("move%b%r");
	Moved[1].set("%N.%b...%b%r");
	Moved[2].set("%N%b...%b%r");

	WhiteWin[0].set("1-0 {*}*");
	BlackWin[0].set("0-1 {*}*");
	Drawn[0].set   ("1/2-1/2 {*}*");
	WhiteWin[1].set("result 1-0*");
	BlackWin[1].set("result 0-1*");
	Drawn[1].set   ("result 1/2-1/2*");

	Dialect[0].set("White resigns*");
	Dialect[1].set("Black resigns*");
	Dialect[2].set("White %s*");
	Dialect[3].set("Black %s*");
	Dialect[4].set("Draw*");
	Dialect[5].set("computer mates*");
	Dialect[6].set("opponent mates*");
	Dialect[7].set("computer resigns*");
	Dialect[8].set("game is a draw*");
	Dialect[9].set("checkmate*");

	CurrentPosition.setStartPos();
	LegalityBackup.setStartPos();

	ebm=0;
}

void XBoardProtocol::setInitialPosition(Position *p) {

	initpos=(*p);
	got_init_pos=true;

}

// the "dialect" for game ends
char * XBoardProtocol::xlateDialect(char *s) {
	static char result[128];

	if (Dialect[0].match(s)) { strcpy(result,"0-1 { White resigns }"); return result; }
	if (Dialect[1].match(s)) { strcpy(result,"1-0 { Black resigns }"); return result; }
	if (Dialect[2].match(s)) { strcpy(result,"1-0 { White mates }"); return result; }
	if (Dialect[3].match(s)) { strcpy(result,"0-1 { Black mates }"); return result; }
	if (Dialect[4].match(s)) { strcpy(result,"1/2-1/2 { Draw }"); return result; }
	if (Dialect[5].match(s)) { strcpy(result,EngineWhite?"1-0 { White mates }":"0-1 { Black mates }"); return result; }
	if (Dialect[6].match(s)) { strcpy(result,EngineWhite?"0-1 { Black mates }":"1-0 { White mates }"); return result; }
	if (Dialect[7].match(s)) { strcpy(result,EngineWhite?"0-1 { White resigns }":"1-0 { Black resigns }"); return result; }
	if (Dialect[8].match(s)) { strcpy(result,"1/2-1/2 { Draw }"); return result; }
	if (Dialect[9].match(s)) { strcpy(result,EngineWhite?"1-0 { White mates }":"0-1 { Black mates }"); return result; }
	return 0;
}

void XBoardProtocol::receiveString(char *netstring) {
	ChessGame *refgame;
	char *p;
	char a[64],b[64];
	int i,moved;
	int wc,bc;

	p=xlateDialect(netstring);
	if (p)
		netstring=p;

	if (Features.match(netstring)) {
		parseFeatures();
		goto just_print;
	}

	if (IllegalMove.match(netstring)) {

		if (!InitDone) {
			// most likely engine does not support xboard protocol v2.
			// and currently we do not support these.
			if (timeout_id >= 0) {
				gtk_timeout_remove(timeout_id);
				timeout_id = -1;
			}
			finalize();
			global.status->setText(_("<XBoard Protocol> Incompatible Engine Protocol")); // TRANSLATE
			goto just_print;
		}

		if (just_sent_fen)
			if ((CurrentPosition.sidehint && EngineWhite) ||
				(!CurrentPosition.sidehint && !EngineWhite) ) {
				// engine sent illegal move string during HIS move
				// happens when FEN string is sent to GNU Chess 5 and
				// it doesn't understand the castling permissions
				global.output->append(_("engine claimed illegal move but we didn't move, ignoring it."), global.Colors.TextBright, IM_IGNORE);
				return;
			}

		CurrentPosition=LegalityBackup;
		refgame=getGame();
		backMove();
		refgame->retreat(1);

		wc = bc = CLOCK_UNCHANGED;
		if (timecontrol.mode == TC_SPM) {
			if (EngineWhite) bc=0; else wc=0;
		}
		refgame->updatePosition2(CurrentPosition, MoveNumber,
								 EngineWhite?1:0,
								 wc,bc,
								 _("illegal move!"));
		global.status->setText(netstring,15);
		global.BoardList.front()->setCanMove(true);
		return;
	}

	moved=-1;
	for(i=0;i<3;i++)
		if (Moved[i].match(netstring)) {
			moved = i;
			break;
		}

	if (moved>=0) {
		LegalityBackup=CurrentPosition;
		CurrentPosition.SANstring(Moved[moved].getRToken(0),a);
		CurrentPosition.moveAnyNotation(Moved[moved].getRToken(0),
										(MoveBlack?BLACK:WHITE),Variant);
		if (EngineWhite)
			snprintf(b,64,"%d. %s",MoveNumber,a);
		else
			snprintf(b,64,"%d. ... %s",MoveNumber,a);

		refgame=getGame();

		wc = bc = CLOCK_UNCHANGED;
		if (timecontrol.mode == TC_ICS) {
			if (MoveNumber == 1)
				wc = bc = 1000 * timecontrol.startValue();
			else
				refgame->incrementActiveClock(timecontrol.value[1]);
		}
		if (timecontrol.mode == TC_SPM) {
			if (EngineWhite) bc=0; else wc=0;
		}

		refgame->updatePosition2(CurrentPosition, MoveNumber,
								 !MoveBlack,
								 wc, bc,
								 b, true);
		advanceMove();
		global.BoardList.front()->setCanMove(true);
		global.opponentMoved();
		just_sent_fen = false;
		return;
	}

	for(i=0;i<2;i++) {
		if (WhiteWin[i].match(netstring)) {
			gameOver(WhiteWin[i],WHITE_WIN);
			goto just_print;
		}
		if (BlackWin[i].match(netstring)) {
			gameOver(BlackWin[i],BLACK_WIN);
			goto just_print;
		}
		if (Drawn[i].match(netstring)) {
			gameOver(Drawn[i],DRAW);
			goto just_print;
		}
	}

 just_print:
	global.output->append(netstring, global.Colors.Engine, IM_IGNORE);
}


void XBoardProtocol::advanceMove() {
	if (MoveBlack)
		MoveNumber++;
	MoveBlack=!MoveBlack;
}

void XBoardProtocol::backMove() {
	if (!MoveBlack)
		MoveNumber--;
	MoveBlack=!MoveBlack;
}

void XBoardProtocol::sendMove(int x1,int y1,int x2,int y2,int prom) {
	ChessGame *refgame;
	char move[6], umove[15];
	char mymove[12],ppfmm[32]; // [p]retty [p]rint [f]or [m]y [m]ove
	piece pp;
	int wc, bc;

	refgame=getGame();
	LegalityBackup=CurrentPosition;

	pp = prom ? global.promotion->getPiece() : EMPTY;

	CurrentPosition.stdNotationForMove(x1,y1,x2,y2,pp,mymove,refgame->Variant);
	CurrentPosition.moveCartesian(x1,y1,x2,y2,Variant,true);

	if (EngineWhite)
		snprintf(ppfmm,32,"%d. ... %s",MoveNumber,mymove);
	else
		snprintf(ppfmm,32,"%d. %s",MoveNumber,mymove);

	wc = bc = CLOCK_UNCHANGED;
	if (timecontrol.mode == TC_ICS) {
		if (MoveNumber == 1)
			wc = bc = 1000 * timecontrol.startValue();
		else
			refgame->incrementActiveClock(timecontrol.value[1]);
	}
	if (timecontrol.mode == TC_SPM) {
		if (EngineWhite) wc=0; else bc=0;
	}

	refgame->updatePosition2(CurrentPosition,
							 MoveNumber,
							 !MoveBlack,
							 wc,bc,
							 ppfmm);
	move[4]=0;
	move[5]=0;
	move[0]='a'+x1;
	move[1]='1'+y1;
	move[2]='a'+x2;
	move[3]='1'+y2;

	if (prom) {
		switch(pp) {
		case QUEEN:   move[4]='q'; break;
		case ROOK:    move[4]='r'; break;
		case BISHOP:  move[4]='b'; break;
		case KNIGHT:  move[4]='n'; break;
		case KING:    move[4]='k'; break;
		}
	}

	if (requires_usermove) {
		strcpy(umove,"usermove ");
		strcat(umove,move);
		global.network->writeLine(umove);
	} else {
		global.network->writeLine(move);
	}
	advanceMove();
	global.BoardList.front()->setCanMove(false);
}

void XBoardProtocol::finalize() {
	ChessGame *refgame;

	if (!Finalized) {
		Finalized = true;
		refgame=getGame(true);
		if (refgame!=NULL) {
			refgame->GameNumber=global.nextFreeGameId(XBOARD_GAME+1);
			refgame->endGame("interrupted",UNDEF);
		}
		if (global.network!=NULL) {
			global.network->close();
			global.network->sendReadNotify();
		}
	}
}

void XBoardProtocol::gameOver(ExtPatternMatcher &pm,GameResult gr,
							  int hasreason)
{
	char reason[64];
	ChessGame *refgame;

	if (hasreason) {
		g_strlcpy(reason,pm.getStarToken(0),64);
	} else
		strcpy(reason,"Game over"); // I can't think of anything better...

	if ( (gr==BLACK_WIN && EngineWhite) ||
		 (gr==WHITE_WIN && !EngineWhite) )
		global.gameWon();

	if ( (gr==WHITE_WIN && EngineWhite) ||
		 (gr==BLACK_WIN && !EngineWhite) )
		global.gameLost();

	refgame=getGame();
	refgame->endGame(reason,gr);

	if (global.AppendPlayed) {
		if  (refgame->savePGN(global.AppendFile,true)) {
			char z[128];
			snprintf(z,128,_("Game appended to %s"),global.AppendFile);
			global.status->setText(z,10);
		}
	}

	global.network->close();
	global.network->sendReadNotify();
}

void XBoardProtocol::resign() {
	ChessGame *refgame;
	global.gameLost();
	refgame=getGame();
	if (EngineWhite) {
		global.network->writeLine("result 0-1 {Black Resigns}");
		refgame->endGame("Black resigned", WHITE_WIN);
	} else {
		global.network->writeLine("result 1-0 {White Resigns}");
		refgame->endGame("White resigned", WHITE_WIN);
	}
	global.status->setText(_("Resigned."),10);
	if (global.AppendPlayed) {
		if  (refgame->savePGN(global.AppendFile,true)) {
			char z[128];
			snprintf(z,128,_("Game appended to %s"),global.AppendFile);
			global.status->setText(z,10);
		}
	}
	global.network->close();
	global.network->sendReadNotify();
}

void XBoardProtocol::draw() {
	global.network->writeLine("draw");
	global.status->setText(_("<XBoard Protocol> draw request sent"),20);
}

void XBoardProtocol::adjourn() {
	global.status->setText(_("<XBoard Protocol> Adjourning not supported"),10);
}

void XBoardProtocol::abort() {
	finalize();
	global.status->setText(_("<XBoard Protocol> Session Aborted"),15);
}

void XBoardProtocol::retractMove() {
	ChessGame *ref;
	ref = getGame(true);
	if (ref!=NULL && MoveNumber>1) {
		if ((!MoveBlack && EngineWhite)||
			(MoveBlack && !EngineWhite)) {
			global.status->setText(_("You can only retract when it's your turn to move."),15);
		} else {
			Position p;
			char z[64];
			ref->retreat(2);
			p = ref->getLastPosition();
			--MoveNumber;
			memset(z,0,64);
			p.getLastMove().copy(z,63);
			ref->updatePosition2(p,MoveNumber,MoveBlack,
								 CLOCK_UNCHANGED, CLOCK_UNCHANGED,z);
			global.network->writeLine("remove");
			global.status->setText(_("Retracted last move."),15);
			CurrentPosition = p;
		}
	}
}

ChessGame * XBoardProtocol::getGame(bool allowFailure) {
	ChessGame *refgame;
	refgame=global.getGame(XBOARD_GAME);
	if (!allowFailure && refgame==NULL) {
		cerr << _("* game not found: ") << XBOARD_GAME << endl;
		exit(5);
	}
	return refgame;
}

int XBoardProtocol::run() {
	createDialog();
	if (!runDialog()) {
		destroyDialog();
		return 0;
	}

	ebm=0;

	readDialog();

	if (ebm!=0)
		global.addEngineBookmark(ebm);

	ebm=0;

	destroyDialog();
	return(loadEngine());
}

int XBoardProtocol::run(EngineBookmark *bm) {
	loadBookmark(bm);
	return(loadEngine());
}

gboolean xb_initengine_timeout(gpointer data) {
	XBoardProtocol *me;
	me=(XBoardProtocol *) data;

	global.status->setText(_("Initializing engine"),30);

	me->initEngine();
	return FALSE;
}

int XBoardProtocol::loadEngine() {
	PipeConnection *link;
	global.debug("XBoardProtocol","loadEngine",EngineCommandLine);

	if (strlen(EngineRunDir))
		snprintf(FullCommand,1024,"cd %s ; %s",EngineRunDir,EngineCommandLine);
	else
		strcpy(FullCommand,EngineCommandLine);

	link=new PipeConnection("/bin/sh","-c",FullCommand,0,0);
	link->setMaxWaitTime(3000.0);

	// only gnu chess 4 has trouble with it
	if (need_handshake)
		link->setHandshake("xboard\nprotover 2\n");

	if (link->open()==0) {
		global.status->setText(_("Engine started (2 sec timeout)"),30);
		global.network=link;

		// run initEngine after 2 seconds
		timeout_id=gtk_timeout_add(2000,xb_initengine_timeout,(gpointer) this);

		if (global.network->isConnected())
			return 1;
		else {
			global.network=0;
			dumpGame();
		}
	}

	global.status->setText(_("Failed to run engine."),30);

	global.output->append(_("Failed to run engine."),
						  global.Colors.TextBright,IM_NORMAL);
	global.output->append(_("Failed command line:"),
						  global.Colors.TextBright,IM_NORMAL);
	global.output->append(FullCommand, global.Colors.TextBright,IM_NORMAL);
	delete link;
	return 0;
}

void XBoardProtocol::initEngine() {
	char z[256];
	createGame();
	// xboard/protover already sent in open() call
	global.network->writeLine("nopost");

	if (ThinkAlways)
		global.network->writeLine("hard");
	else
		global.network->writeLine("easy");

	global.network->writeLine("new");

	timecontrol.toXBoard(z,256);
	if (z[0])
		global.network->writeLine(z);

	if (MaxDepth > 0) {
		snprintf(z,256,"sd %d",MaxDepth);
		global.network->writeLine(z);
	}

	lateInit();
	endInit();
}

void XBoardProtocol::endInit() {
	char z[256];
	InitDone = true;
	snprintf(z,256,_("%s engine started."), ComputerName);
	global.status->setText(z,15);
}

void XBoardProtocol::lateInit() {
	if (got_init_pos) {

		global.network->writeLine("force");

		setupPosition();

		// the a2a3 thing suggested in the protocol spec
		// makes GNU chess 4 go nuts

		if (initpos.sidehint)
			global.network->writeLine("white");
		else
			global.network->writeLine("black");

		global.network->writeLine("go");

	} else {

		if (EngineWhite) {
			global.network->writeLine("white");
			global.network->writeLine("go");
		}

	}
}

void XBoardProtocol::setupPosition() {
	char z[256];

	if (!got_init_pos) return;

	if (supports_setboard) {
		string s;

		s="setboard ";
		s+=initpos.getFEN();
		g_strlcpy(z,s.c_str(),256);
		global.network->writeLine(z);
		just_sent_fen = true;

	} else {
		char assoc[128];
		int i,j;
		piece p;

		global.network->writeLine("edit");
		global.network->writeLine("#");

		assoc[PAWN]  ='P';
		assoc[KNIGHT]='N';
		assoc[BISHOP]='B';
		assoc[ROOK]  ='R';
		assoc[QUEEN] ='Q';
		assoc[KING]  ='K';

		for(i=0;i<8;i++)
			for(j=0;j<8;j++) {
				p=initpos.getPiece(i,j);
				if ((p&COLOR_MASK) == WHITE) {
					snprintf(z,256,"%c%c%d",assoc[p&PIECE_MASK],'a'+i,1+j);
					global.network->writeLine(z);
				}
			}

		global.network->writeLine("c");

		for(i=0;i<8;i++)
			for(j=0;j<8;j++) {
				p=initpos.getPiece(i,j);
				if ((p&COLOR_MASK) == BLACK) {
					snprintf(z,256,"%c%c%d",assoc[p&PIECE_MASK],'a'+i,1+j);
					global.network->writeLine(z);
				}
			}

		global.network->writeLine(".");
		just_sent_fen = true;
	}

}

void XBoardProtocol::createGame() {
	ChessGame *cg;
	Position *pos;
	global.debug("XBoardProtocol","createGame");

	MoveNumber=1;
	MoveBlack=0;

	cg=new ChessGame();
	cg->GameNumber=XBOARD_GAME;
	cg->source = GS_Engine;
	cg->timecontrol = timecontrol;
	g_strlcpy(cg->PlayerName[EngineWhite?0:1],getComputerName(),64);
	g_strlcpy(cg->PlayerName[EngineWhite?1:0],global.env.User.c_str(),64);

	cg->MyColor=EngineWhite?BLACK:WHITE;

	timecontrol.toString(cg->info0,64);

	cg->clock_regressive=timecontrol.isRegressive()?1:0;
	cg->Rated=0;
	cg->Variant=Variant;
	global.appendGame(cg);
	global.BoardList.front()->reset();
	cg->setBoard(global.BoardList.front());
	global.BoardList.front()->setGame(cg);
	global.BoardList.front()->pop();
	global.BoardList.front()->setCanMove(true);
	global.BoardList.front()->repaint();
	if (EngineWhite)
		global.BoardList.front()->setFlipped(true);
	cg->acknowledgeInfo();
	cg->fireWhiteClock(timecontrol.startValue(),
					   timecontrol.startValue());

	pos=new Position();

	if (got_init_pos) {
		(*pos)=initpos;
		CurrentPosition = initpos;
		LegalityBackup  = initpos;
		MoveBlack = pos->sidehint ? 0 : 1;
	}

	cg->updatePosition2(*pos,1,pos->sidehint ? 0:1,
						1000*timecontrol.startValue(),
						1000*timecontrol.startValue(),
						"0. startpos");
	delete pos;

	global.gameStarted();
}

void XBoardProtocol::dumpGame() {
	ChessGame *cg;

	cg=global.getGame(XBOARD_GAME);
	if (cg==NULL) return;

	if (!cg->isOver()) cg->endGame("*",UNDEF);
	global.BoardList.front()->reset();

}

void XBoardProtocol::parseFeatures() {
	ExtPatternMatcher sf,nf;
	ChessGame *cg;
	char *cand,*p;
	int n;
	bool gotdone=false;

	sf.set("%b%s=\"*\"*");
	nf.set("%b%s=%n*");
	cand=Features.getStarToken(1);

	while(1) {
		if (sf.match(cand)) {
			p=sf.getSToken(0);
			if (!strcmp(p,"myname")) {
				ComputerName[63]=0;
				g_strlcpy(ComputerName,sf.getStarToken(0),64);
				global.status->setText(ComputerName,10);
				cg=global.getGame(XBOARD_GAME);
				if (cg) {
					g_strlcpy(cg->PlayerName[EngineWhite?0:1],ComputerName,64);
					cg->acknowledgeInfo();
				}
			}
			cand=sf.getStarToken(1);
			continue;
		}
		if (nf.match(cand)) {
			p=nf.getSToken(0);

			if (!strcmp(p,"setboard")) {
				n=atoi(nf.getNToken(0));
				if (n) supports_setboard=true;
			}
			if (!strcmp(p,"usermove")) {
				n=atoi(nf.getNToken(0));
				if (n) requires_usermove=true;
			}

			if (!strcmp(p,"done")) {
				n=atoi(nf.getNToken(0));
				if (n)
					gotdone=true;
				else {
					// engine asked more time
					global.status->setText(_("Engine asked more time to startup, waiting."),120);
					if (timeout_id >= 0)
						gtk_timeout_remove(timeout_id);

					// 1 hour, as per Mann's doc
					timeout_id=gtk_timeout_add(3600000, xb_initengine_timeout,
											   (gpointer) this);
				}

			}

			cand=nf.getStarToken(0);
			continue;
		}
		break;
	}

	if (gotdone && timeout_id >= 0) {
		gtk_timeout_remove(timeout_id);
		timeout_id=-1;
		global.status->setText(_("Engine loaded."),20);
		initEngine();
	}
}

char * XBoardProtocol::getDialogName() {
	return(_("Play against Engine"));
}

char * XBoardProtocol::getComputerName() {
	return(ComputerName);
}

// ----- the bloated world of GUI -------

void XBoardProtocol::createDialog() {
	GtkWidget *v,*v2;
	GtkWidget *cfr,*rd[2],*ok,*cancel,*bhb,*hs;
	GtkWidget *f2,*tabl,*l2,*tch,*tce,*tl1,*tl2,*te1,*tv;
	GSList *rg;
	char z[64];

	GtkWidget *bp_l,*bp_v, *pp_l=0, *pp_v=0;
	GtkWidget *h3=0,*l3=0,*path3,*b31,*l31,*h4=0,*l4=0,*path4;

	eng_dlg=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(eng_dlg),getDialogName());
	gtk_window_set_position(GTK_WINDOW(eng_dlg),GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(eng_dlg),6);
	gtk_widget_realize(eng_dlg);

	v=gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(eng_dlg),v);

	eng_book=gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(v),eng_book,TRUE,TRUE,0);

	// basic pane
	bp_v=gtk_vbox_new(FALSE,0);
	bp_l=gtk_label_new(_("Side & Time"));
	gtk_container_set_border_width(GTK_CONTAINER(bp_v),4);
	cfr=gtk_frame_new(_("Side Selection"));
	gtk_frame_set_shadow_type(GTK_FRAME(cfr),GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(bp_v),cfr,FALSE,TRUE,4);
	v2=gtk_vbox_new(TRUE,2);
	gtk_container_add(GTK_CONTAINER(cfr),v2);
	rd[0]=grnew( 0, _("Human White vs. Computer Black") );
	rg=grg(GTK_RADIO_BUTTON(rd[0]));
	rd[1]=grnew(rg, _("Computer White vs. Human Black") );
	gtset(GTK_TOGGLE_BUTTON(rd[0]), TRUE);
	gtk_box_pack_start(GTK_BOX(v2),rd[0],FALSE,TRUE,2);
	gtk_box_pack_start(GTK_BOX(v2),rd[1],FALSE,TRUE,2);
	// time per move
	f2=gtk_frame_new(_("Time Control"));
	gtk_frame_set_shadow_type(GTK_FRAME(f2),GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(bp_v),f2,FALSE,TRUE,4);
	tv=gtk_vbox_new(FALSE,2);
	gtk_container_add(GTK_CONTAINER(f2),tv);

	tch = gtk_hbox_new(FALSE,2);
	gtk_box_pack_start(GTK_BOX(tv),tch,TRUE,TRUE,0);

	timecontrol.toString(z,64);
	l2=gtk_label_new(z);
	gtk_box_pack_start(GTK_BOX(tch),l2,TRUE,TRUE,0);

	tce = gtk_button_new_with_label(_("Time Control..."));
	gtk_box_pack_start(GTK_BOX(tch),tce,FALSE,TRUE,0);

	tabl=gtk_table_new(2,2,FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(tabl),4);
	gtk_table_set_row_spacings(GTK_TABLE(tabl),2);
	gtk_table_set_col_spacings(GTK_TABLE(tabl),2);
	gtk_box_pack_start(GTK_BOX(tv),tabl,FALSE,FALSE,2);

	tl2=gtk_label_new(_("Depth Limit:"));
	te1=gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(te1),"0");
	gtk_table_attach_defaults(GTK_TABLE(tabl),tl2,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(tabl),te1,1,2,0,1);

	tl1=gtk_label_new(_("Set depth limit to 0 to use the engine's default."));
	gtk_box_pack_start(GTK_BOX(tv),tl1,FALSE,FALSE,2);

	think=gtk_check_button_new_with_label(_("Think on opponent's time"));
	gtk_table_attach_defaults(GTK_TABLE(tabl),think,0,2,1,2);
	gtset(GTK_TOGGLE_BUTTON(think), 1);

	bookmark=gtk_check_button_new_with_label(_("Add to Peer/Engine Bookmarks menu"));
	gtk_box_pack_start(GTK_BOX(v),bookmark,FALSE,FALSE,2);
	gtset(GTK_TOGGLE_BUTTON(bookmark), 1);

	Gtk::show(bookmark,think,te1,tl2,tl1,tabl,tv,f2,tch,l2,tce,rd[1],rd[0],
			  v2,cfr,NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(eng_book),bp_v,bp_l);
	Gtk::show(bp_v,bp_l,NULL);

	// path pane
	if (want_path_pane) {
		pp_v=gtk_vbox_new(FALSE,0);
		pp_l=gtk_label_new(_("Engine Command"));
		gtk_container_set_border_width(GTK_CONTAINER(pp_v),4);
		h3=gtk_hbox_new(FALSE,0);
		l3=gtk_label_new(_("Engine command line"));
		gtk_box_pack_start(GTK_BOX(pp_v),h3,FALSE,FALSE,2);
		gtk_box_pack_start(GTK_BOX(h3),l3,FALSE,FALSE,0);
	}
	path3=gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(path3),EngineCommandLine);
	if (want_path_pane) {
		gtk_box_pack_start(GTK_BOX(pp_v),path3,FALSE,TRUE,2);
		h4=gtk_hbox_new(FALSE,0);
		l4=gtk_label_new(_("Directory to run from (e.g., where book files are)"));
		gtk_box_pack_start(GTK_BOX(pp_v),h4,FALSE,FALSE,2);
		gtk_box_pack_start(GTK_BOX(h4),l4,FALSE,FALSE,0);
	}
	path4=gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(path4),EngineRunDir);
	if (want_path_pane) {
		gtk_box_pack_start(GTK_BOX(pp_v),path4,FALSE,TRUE,2);
		b31=gtk_hbox_new(FALSE,0);
		l31=gtk_label_new(_("The engine will be run with\n/bin/sh -c 'cd directory ; command line'"));
		gtk_label_set_justify(GTK_LABEL(l31),GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(b31),l31,FALSE,FALSE,2);
		gtk_box_pack_start(GTK_BOX(pp_v),b31,FALSE,FALSE,2);

		Gtk::show(h3,l3,l31,b31,path3,h4,l4,path4,NULL);
		gtk_notebook_append_page(GTK_NOTEBOOK(eng_book),pp_v,pp_l);
		Gtk::show(pp_v,pp_l,NULL);
	}

	// bottom box
	hs=gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(v),hs,FALSE,TRUE,4);

	bhb=gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
	gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,FALSE,4);
	ok=gtk_button_new_with_label    (_("OK"));
	GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
	cancel=gtk_button_new_with_label(_("Cancel"));
	GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bhb),ok,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(bhb),cancel,TRUE,TRUE,0);
	gtk_widget_grab_default(ok);

	Gtk::show(ok,cancel,bhb,hs,v,NULL);

	gtk_signal_connect(GTK_OBJECT(ok), "clicked",
					   GTK_SIGNAL_FUNC(xboard_eng_ok),this);
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
					   GTK_SIGNAL_FUNC(xboard_eng_cancel),this);
	gtk_signal_connect(GTK_OBJECT(eng_dlg), "delete_event",
					   GTK_SIGNAL_FUNC(xboard_eng_delete),0);

	gtk_signal_connect(GTK_OBJECT(tce), "clicked",
					   GTK_SIGNAL_FUNC(xboard_edit_time),
					   (XBoardProtocol *) this);

	gshow(eng_book);
	gtk_notebook_set_page(GTK_NOTEBOOK(eng_book),0);

	engctl_lbl=l2;
	engctl_ply=te1;
	engctl_ewhite=rd[1];
	engctl_engcmd=path3;
	engctl_engdir=path4;

	if (got_init_pos) {
		if (initpos.sidehint) // white to move
			gtset(GTK_TOGGLE_BUTTON(rd[1]), TRUE);
		else
			gtset(GTK_TOGGLE_BUTTON(rd[0]), TRUE);

		gtk_widget_set_sensitive(rd[0],FALSE);
		gtk_widget_set_sensitive(rd[1],FALSE);
	}
}

void XBoardProtocol::update() {
	char z[64];
	timecontrol.toString(z,64);
	gtk_label_set_text(GTK_LABEL(engctl_lbl), z);
	gtk_widget_queue_resize(engctl_lbl);
}

void XBoardProtocol::readDialog() {

	EngineWhite=0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(engctl_ewhite)))
		EngineWhite=1;
	MaxDepth=atoi(gtk_entry_get_text(GTK_ENTRY(engctl_ply)));
	g_strlcpy(EngineCommandLine,gtk_entry_get_text(GTK_ENTRY(engctl_engcmd)),512);
	g_strlcpy(EngineRunDir,gtk_entry_get_text(GTK_ENTRY(engctl_engdir)),256);
	ThinkAlways=(bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(think));

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(bookmark))) {
		ebm=new EngineBookmark();
		ebm->timecontrol =timecontrol;
		ebm->maxply      =MaxDepth;
		ebm->humanwhite  =1-EngineWhite;
		ebm->think       =ThinkAlways?1:0;
		ebm->directory   =EngineRunDir;
		ebm->cmdline     =EngineCommandLine;
		ebm->mode        =REGULAR;
		ebm->proto       =0; // base xboard protocol

		makeBookmarkCaption();

		// addition is done in run() to allow all virtual versions of this
		// to run before doing it

	} else {
		ebm=0; // derivated classes should check ebm!=0 before doing oddities
	}
}

void XBoardProtocol::makeBookmarkCaption() {
	tstring t,u;
	string x, *p;
	char caption[1024], z[128];

	if (!ebm) return;

	t.set(EngineCommandLine);
	x=EngineCommandLine;
	if (x.empty()) x="<blank?>";

	p=t.token(" ");
	if (p != 0) {
		u.set(*p);
		while( (p=u.token("/")) != 0 )
			x=*p;
	}

	ebm->timecontrol.toString(z,128);
	snprintf(caption,1024,
			 _("Play %s as %s vs. %s (%s, maxdepth %d, think always: %s)"),
			 ChessGame::variantName(ebm->mode),
			 ebm->humanwhite?_("White"):
			 _("Black"),
			 x.c_str(),
			 z,
			 ebm->maxply,
			 ebm->think?_("yes"):
			 _("no"));
	ebm->caption = caption;
}

void XBoardProtocol::loadBookmark(EngineBookmark *bm) {
	timecontrol = bm->timecontrol;
	MaxDepth    = bm->maxply;
	ThinkAlways = (bm->think!=0);
	EngineWhite = 1-bm->humanwhite;
	Variant     = bm->mode;
	g_strlcpy(EngineRunDir, bm->directory.c_str(), 256 );
	g_strlcpy(EngineCommandLine, bm->cmdline.c_str(), 512 );

	if (got_init_pos)
		EngineWhite=initpos.sidehint ? 1 : 0;
}

int XBoardProtocol::runDialog() {
	GotResult=0;
	gshow(eng_dlg);
	gtk_grab_add(eng_dlg);
	while(!GotResult)
		global.WrappedMainIteration();
	gtk_grab_remove(eng_dlg);
	return(GotResult==1);
}

void XBoardProtocol::destroyDialog() {
	gtk_widget_destroy(eng_dlg);
}

void xboard_edit_time(GtkWidget *w,gpointer data) {
	XBoardProtocol *me = (XBoardProtocol *) data;
	TimeControlEditDialog *dlg;

	dlg = new TimeControlEditDialog(&(me->timecontrol));
	dlg->setUpdateListener( (UpdateInterface *) me);
	dlg->show();
}

void xboard_eng_ok(GtkWidget *w,gpointer data) {
	XBoardProtocol *me;
	me=(XBoardProtocol *)data;
	me->GotResult=1;
}

void xboard_eng_cancel(GtkWidget *w,gpointer data) {
	XBoardProtocol *me;
	me=(XBoardProtocol *)data;
	me->GotResult=-1;
}

gboolean xboard_eng_delete(GtkWidget *w,GdkEvent *e,gpointer data) {
	return TRUE;
}


/* ************************************************** */
/* *********** specific engines ********************* */
/* ************************************************** */

// =============== CRAFTY ============================

CraftyProtocol::CraftyProtocol() : XBoardProtocol() {
	strcpy(ComputerName,"Crafty");
	want_path_pane=0;
}

char * CraftyProtocol::getDialogName() {
	return(_("Play against Crafty"));
}

void CraftyProtocol::readDialog() {
	XBoardProtocol::readDialog();
	resolvePaths();
	snprintf(EngineCommandLine,512,"crafty bookpath=%s logpath=%s tbpath=%s",
			 BookPath,LogPath,LogPath);
	if (!global.env.Home.empty())
		snprintf(EngineRunDir,512,"%s/.eboard/craftylog",global.env.Home.c_str());
	else
		strcpy(EngineRunDir,"/tmp");

	if (ebm) {
		ebm->proto     = 1;
		ebm->cmdline   = EngineCommandLine;
		ebm->directory = EngineRunDir;
		makeBookmarkCaption();
	}
}

void CraftyProtocol::initEngine() {
	global.network->writeLine("log off");
	XBoardProtocol::initEngine();
}

// keep this and Documentation/Crafty.txt coherent
void CraftyProtocol::resolvePaths() {
	FileFinder ff;
	char z[256],zz[256],*p;

	// book path
	if (!global.env.Home.empty())
		snprintf(z,256,"%s/.eboard",global.env.Home.c_str());
	else
		strcpy(z,"/tmp");

	ff.addMyDirectory(".eboard");
	ff.addMyDirectory(".");
	ff.addDirectory("/usr/local/share/eboard");
	ff.addDirectory("/usr/share/eboard");
	ff.addDirectory("/usr/local/share/crafty");
	ff.addDirectory("/usr/share/crafty");

	if (ff.find("book.bin",zz)) {
		p=strrchr(zz,'/');
		*p=0;
	} else
		strcpy(zz,z);

	strcpy(BookPath,zz);

	// log path
	if (!global.env.Home.empty())
		snprintf(z,256,"%s/.eboard/craftylog",global.env.Home.c_str());
	else
		strcpy(z,"/tmp");
	strcpy(LogPath,z);
}

// =============== SJENG ============================

SjengProtocol::SjengProtocol() : XBoardProtocol() {
	strcpy(ComputerName,"Sjeng");
	strcpy(EngineCommandLine,"sjeng");
	want_path_pane=1;
}

void SjengProtocol::initEngine() {
	char z[32];
	createGame();
	global.network->writeLine("xboard");
	global.network->writeLine("protover 2");
	global.network->writeLine("nopost");

	if (ThinkAlways)
		global.network->writeLine("hard");
	else
		global.network->writeLine("easy");

	global.network->writeLine("new");

	switch(Variant) {
	case CRAZYHOUSE: global.network->writeLine("variant crazyhouse"); break;
	case SUICIDE:    global.network->writeLine("variant suicide"); break;
	case LOSERS:     global.network->writeLine("variant losers"); break;
	case GIVEAWAY:   global.network->writeLine("variant giveaway"); break;
	default: z[31]=0; /* nothing! */ break;
	}

	timecontrol.toXBoard(z,32);
	if (z[0])
		global.network->writeLine(z);

	if (MaxDepth>0) {
		snprintf(z,32,"sd %d",MaxDepth);
		global.network->writeLine(z);
	}

	lateInit();
	endInit();
}

void SjengProtocol::createDialog() {
	int i;
	GtkWidget *fr,*v,*rd[5],*vl;
	GSList *rg;

	XBoardProtocol::createDialog();

	// add variant pane
	fr=gtk_frame_new(_("Variant"));
	gtk_frame_set_shadow_type(GTK_FRAME(fr),GTK_SHADOW_ETCHED_IN);
	v=gtk_vbox_new(TRUE,2);
	gtk_container_add(GTK_CONTAINER(fr),v);

	rd[0]=grnew( 0, _("Normal Chess") );
	rg=grg(GTK_RADIO_BUTTON(rd[0]));
	rd[1]=grnew(rg, _("Crazyhouse") );
	rg=grg(GTK_RADIO_BUTTON(rd[1]));
	rd[2]=grnew(rg, _("Suicide") );
	rg=grg(GTK_RADIO_BUTTON(rd[2]));
	rd[3]=grnew(rg, _("Losers") );
	rg=grg(GTK_RADIO_BUTTON(rd[3]));
	rd[4]=grnew(rg, _("Giveaway") );

	gtset(GTK_TOGGLE_BUTTON(rd[0]), TRUE);
	for(i=0;i<5;i++) {
		gtk_box_pack_start(GTK_BOX(v),rd[i],FALSE,TRUE,2);
		gshow(rd[i]);
		varbutton[i]=rd[i];
	}
	Gtk::show(v,fr,NULL);
	vl=gtk_label_new(_("Variant"));
	gshow(vl);
	gtk_notebook_append_page(GTK_NOTEBOOK(eng_book),fr,vl);
	gtk_notebook_set_page(GTK_NOTEBOOK(eng_book),0);
}

void SjengProtocol::readDialog() {
	XBoardProtocol::readDialog();
	if (gtget(GTK_TOGGLE_BUTTON(varbutton[0]))) Variant=REGULAR;
	if (gtget(GTK_TOGGLE_BUTTON(varbutton[1]))) Variant=CRAZYHOUSE;
	if (gtget(GTK_TOGGLE_BUTTON(varbutton[2]))) Variant=SUICIDE;
	if (gtget(GTK_TOGGLE_BUTTON(varbutton[3]))) Variant=LOSERS;
	if (gtget(GTK_TOGGLE_BUTTON(varbutton[4]))) Variant=GIVEAWAY;

	if (ebm) {
		ebm->proto=2;
		ebm->mode=Variant;

		makeBookmarkCaption();
	}
}

char * SjengProtocol::getDialogName() {
	return(_("Play against Sjeng"));
}

void SjengProtocol::sendDrop(piece p,int x,int y) {
	ChessGame *refgame;
	char mymove[12],ppfmm[32]; // [p]retty [p]rint [f]or [m]y [m]ove
	piece clp;

	refgame=getGame();
	LegalityBackup=CurrentPosition;

	snprintf(mymove,12,"P@%c%c",'a'+x,'1'+y);
	clp=p&PIECE_MASK;
	switch(clp) {
	case PAWN:    mymove[0]='P'; break;
	case ROOK:    mymove[0]='R'; break;
	case KNIGHT:  mymove[0]='N'; break;
	case BISHOP:  mymove[0]='B'; break;
	case QUEEN:   mymove[0]='Q'; break;
	case KING:    mymove[0]='K'; break;
	}
	CurrentPosition.moveDrop(p|(refgame->MyColor),x,y);

	if (EngineWhite)
		snprintf(ppfmm,32,"%d. ... %s",MoveNumber,mymove);
	else
		snprintf(ppfmm,32,"%d. %s",MoveNumber,mymove);

	refgame->updatePosition2(CurrentPosition,
							 MoveNumber,!MoveBlack,
							 CLOCK_UNCHANGED,
							 CLOCK_UNCHANGED,
							 ppfmm);

	global.network->writeLine(mymove);
	advanceMove();
	global.BoardList.front()->setCanMove(false);
}

// ==================== GNU CHESS 4 =========================

GnuChess4Protocol::GnuChess4Protocol() : XBoardProtocol() {
	strcpy(ComputerName,"GNU Chess");
	want_path_pane=0;
	need_handshake=false; // sending xboard confuses GNU chess 4
}

void GnuChess4Protocol::initEngine() {
	char z[32];
	createGame();
	global.network->writeLine("nopost");

	global.network->writeLine("hard");
	if (!ThinkAlways)
		global.network->writeLine("easy");

	global.network->writeLine("new");

	timecontrol.toXBoard(z,32,TCF_GNUCHESS4QUIRK);
	if (z[0])
		global.network->writeLine(z);

	if (MaxDepth > 0) {
		snprintf(z,32,"depth %d",MaxDepth);
		global.network->writeLine(z);
	}

	lateInit();
	endInit();
}

void GnuChess4Protocol::readDialog() {
	XBoardProtocol::readDialog();
	strcpy(EngineCommandLine,"gnuchessx");
	EngineRunDir[0]=0;

	if (ebm) {
		ebm->proto=3;
		ebm->cmdline=EngineCommandLine;
		ebm->directory=EngineRunDir;
		makeBookmarkCaption();
	}
}

char * GnuChess4Protocol::getDialogName() {
	return(_("Play against GNU Chess 4"));
}
