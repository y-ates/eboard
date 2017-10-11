/* $Id: proto_fics.cc,v 1.122 2008/02/19 15:58:53 bergo Exp $ */

/*

  eboard - chess client
  http://eboard.sourceforge.net
  Copyright (C) 2000-2008 Felipe Paulo Guazzi Bergo
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
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "config.h"

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <ctype.h>
#include "protocol.h"
#include "global.h"
#include "chess.h"
#include "status.h"
#include "seekgraph.h"
#include "eboard.h"

// <b1> game 45 white [PPPPPNBBR] black [PPNN]

FicsProtocol::FicsProtocol() {
	SetPat *sp;

	global.debug("FicsProtocol","FicsProtocol");

	GActions.push_back(new string("All Observers"));
	GActions.push_back(new string("Game Info"));

	PActions.push_back(new string("Finger"));
	PActions.push_back(new string("Stats"));
	PActions.push_back(new string("History"));
	PActions.push_back(new string("Ping"));
	PActions.push_back(new string("Log"));
	PActions.push_back(new string("Variables"));

	/* parsing state */

	AuthGone           = false;
	InitSent           = false;
	LecBotSpecial      = false;
	LastWasStyle12     = false;
	LastWasPrompt      = false;
	LastColor          = global.Colors.TextBright;
	LastChannel        = -1;
	UseMsecs           = false;

	PendingStatus      = 0;
	RetrievingMoves    = 0;
	RetrievingGameList = false;
	RetrievingAdList   = false;

	lastGLC=0;
	lastALC=0;

	PartnerGame = -1;
	UserPlayingWithWhite = true;

	xxnext=0;
	xxplayer[0][0]=0;
	xxplayer[1][0]=0;
	xxrating[0][0]=0;
	xxrating[1][0]=0;

	Login.set   ("*login:*");
	Password.set("*password:*");
	Prompt.set("*ics%%*");

	WelcomeMessage.append(new ExactStringPat("**** Starting "));
	WelcomeMessage.append(new KleeneStarPat());
	WelcomeMessage.append(new ExactStringPat("session as "));
	WelcomeMessage.append(sp=new PercSSetPat());
	WelcomeMessage.append(new KleeneStarPat());

	CreateGame.set("{Game %n (%S vs. %S) Creating %s %r *}*");
	CreateGame0.set("Creating: %S (*) %S (*)*");
	ContinueGame.set("{Game %n (%S vs. %S) Continuing %s %r *}*");

	BeginObserve.set("You are now observing game %n.");
	EndObserve.set("*Removing game %n from observation list*");
	ObsInfoLine.set("Game %n: %S (*) %S (*) %s %r *");

	EndExamine.set("You are no longer examining game %n*");
	MsSet.set("ms set.*");

	Style12.set("<12> *");
	Style12House.set("<b1> game %n white [*] black [*]*");
	/* As of 2007 FICS is sending this before moves even in crazyhouse games */
	Style12BadHouse.set("<b1> game %n white [*] black [*]%b<-*");

	WhiteWon.set    ("{Game %n (%S vs. %S) *} 1-0*");
	BlackWon.set    ("{Game %n (%S vs. %S) *} 0-1*");
	Drawn.set       ("{Game %n (%S vs. %S) *} 1/2-1/2*");
	Interrupted.set ("{Game %n (%S vs. %S) *} %**");

	DrawOffer.set("%S offers you a draw.*");

	PrivateTell.set ("%A tells you:*");
	//Index of new news items
	News.set("Index of new *news items:*");

	// Ns: 0=game 1=rat.white 2=rat.black
	// Ss: 0=whitep 1=blackp
	// *s: 1=game string
	GameStatus.set("*%n%b%n%b%S%b%n%b%S%b[*]*");

	// 2 (Exam. 2413 Species     1234 Pulga     ) [ br  3   1] W:  1
	// amazingly enough, the indexes are the same as in GameStatus
	ExaGameStatus.set("*%n (Exam.%b%n %S%b%n %S%b)%b[*]*");

	GameListEntry.set("*%n *[*]*:*");
	EndOfGameList.set("*%n games displayed.*");

	AdListEntry.set("*%n%b%n%b%r%b%n%b%n%b%r*");
	EndOfAdList.set("*%n ad* displayed.*");

	//IceBox (1156) vs. damilasa (UNR) --- Sun Apr 22, 09:07 ??? 2001
	MovesHeader.set("%S (*) vs. %S (*) --- *");
	//Unrated blitz match, initial time: 2 minutes, increment: 12 seconds.
	MovesHeader2.set("%s %r match, initial time: *");
	//Move  IceBox             damilasa
	MovesHeader3.set("Move%b%S%b%S*");
	//----  ----------------   ----------------
	MovesHeader4.set("----  ---*");
	//  1.  d4      (0:00)     d5      (0:00)
	// n0   r0      r1         r2      r3
	MovesBody.set("*%n.%b%r%b%r%b%r%b%r*");
	// same thing, line without black move
	MovesBody2.set("*%n.%b%r%b%r*");
	//      {Still in progress} *
	MovesEnd.set("%b{*}*");

	Kibitz.set  ("%A*[%n]%bkibitzes:*");
	Whisper.set ("%A*[%n]%bwhispers:*");
	Say.set     ("%A[%n] says:*");
	Say2.set     ("%A says:*");
	ChannelTell.set("*(%n):*");
	Seek.set("*(*) seeking*(*to respond)");
	Shout.set ("%A shouts: *");
	cShout.set ("%A c-shouts: *");
	It.set ("-->*");
	Notify.set("Notification:*");

	SgClear.set("<sc>*");
	SgRemove.set("<sr>*");
	SgRemove2.set("Ads removed:*");

	// <s> 8 w=visar ti=02 rt=2194  t=4 i=0 r=r tp=suicide c=? rr=0-9999 a=t f=f
	//    n0   s0       r0    n1  *0  n2  n3  s1    r1       r2    r3      s2  s3
	SgAdd.set("<s>%b%n%bw=%S%bti=%r%brt=%n*%bt=%n%bi=%n%br=%s%btp=%r%bc=%r%brr=%r%ba=%s%bf=%s*");
	SgAdd2.set("%b<s>%b%n%bw=%S%bti=%r%brt=%n*%bt=%n%bi=%n%br=%s%btp=%r%bc=%r%brr=%r%ba=%s%bf=%s*");

	// bughouse stuff
	PTell.set("%A (your partner) tells you: *");

	// sample output: Your partner is playing game 75 (aglup vs. cglup).
	GotPartnerGame.set("Your partner is playing game %n (*");

	// challenge
	Challenge.set("Challenge: *");

	// FICS LectureBot fake channel tells
	LectureBotXTell.set(":LectureBot(TD)(67):*");
	LectureBotXTellContinued.set(":   *");

	// AllOb parsing
	AllObNone.set("No one is observing game %n.*");

	// Observing 18 [Fullmer vs. knighttour]: #boffo #HummerESS TieFighter (3 users)
	// Observing 62 [VABORIS vs. Fudpucker]: BillJr(TM) CrouchingTiger(U) #Firefly
	AllObFirstLine.set("Observing %n [*]: *");
	AllObMidLine.set("\\*");
	AllObState = 0;
	memset(AllObAcc,0,1024);

	memset(style12table,0,256*sizeof(piece));
	style12table['-']=EMPTY;
	style12table['P']=PAWN|WHITE;
	style12table['R']=ROOK|WHITE;
	style12table['N']=KNIGHT|WHITE;
	style12table['B']=BISHOP|WHITE;
	style12table['Q']=QUEEN|WHITE;
	style12table['K']=KING|WHITE;
	style12table['p']=PAWN|BLACK;
	style12table['r']=ROOK|BLACK;
	style12table['n']=KNIGHT|BLACK;
	style12table['b']=BISHOP|BLACK;
	style12table['q']=QUEEN|BLACK;
	style12table['k']=KING|BLACK;

	Binder.add(&WelcomeMessage,&UserName,&Login,&Password,&Prompt,
			   &CreateGame,&CreateGame0,&ContinueGame,&BeginObserve,
			   &EndObserve,&NotObserve,&EndExamine,&GameStatus,
			   &ExaGameStatus,&GameListEntry,&EndOfGameList,
			   &AdListEntry,&EndOfAdList,&Style12,&Style12House,
			   &WhiteWon,&BlackWon,&Drawn,&Interrupted,&DrawOffer,
			   &PrivateTell,&News,&MovesHeader,&MovesHeader2,
			   &MovesHeader3,&MovesHeader4,&MovesBody,&MovesBody2,
			   &MovesEnd,NULL);
	Binder.add(&Kibitz,&Whisper,&Say,&Shout,&cShout,&It,&ChannelTell,
			   &Seek,&Notify,&SgClear,&SgAdd,&SgAdd2,&SgRemove,
			   &PTell,&Say2,&Challenge,&LectureBotXTell,
			   &LectureBotXTellContinued,&GotPartnerGame,
			   &AllObNone,&AllObFirstLine,&AllObMidLine,
			   &Style12BadHouse,&ObsInfoLine,&SgRemove2,&MsSet,NULL);
}

FicsProtocol::~FicsProtocol() {
	global.debug("FicsProtocol","~FicsProtocol");
	clearMoveBuffer();
}

void FicsProtocol::finalize() {
	global.debug("FicsProtocol","finalize");
	if (global.network)
		global.network->writeLine("quit");
	if (global.IcsSeekGraph && global.skgraph2)
		global.skgraph2->clear();
}

void FicsProtocol::receiveString(char *netstring) {
	parser1(netstring);
}

/*
  -- eats blank lines after style-12 lines
  -- ensures initialization is sent on non-FICS servers
  -- ensures password mode is off on non-FICS servers
  -- removes the prompt from the line
  -- ignore empty lines
  -- calls the next parser
*/
bool FicsProtocol::parser1(char *T) {
	char pstring[1024];

	/* ignore blank lines from timeseal */
	if (T[0]==0 && LastWasStyle12) {
		LastWasStyle12 = false;
		return true;
	}

	LastWasStyle12 = false;
	Binder.prepare(T);

	/* remove prompts */
	while(Prompt.match()) {
		/* avoid fake prompts in help files */
		if (strlen(Prompt.getStarToken(0)) > 9)
			break;

		/* send initialization to servers that don't tell us our name */
		if (!InitSent) {
			UserName.reset();
			UserName.append(new CIExactStringPat("i n v a l i d"));
			strcpy(nick,"i n v a l i d");
			sendInitialization();
			InitSent = true;
		}

		/* exit auth mode on old ICS code */
		if (!AuthGone) {
			AuthGone = true;
			global.input->setPasswordMode(0);
		}

		/* remove prompt, ignore prompt-only lines */
		if (strlen(Prompt.getStarToken(1))>3) {
			memset(pstring,0,1024);
			g_strlcpy(pstring,Prompt.getStarToken(1),1024);
			T = pstring;
			if (T[0]==' ') T++;
			Binder.prepare(T);
		} else {
			LastWasPrompt = true;
			return true;
		}

	} /* while Prompt.match */

	/* ignore empty lines after prompts (??) */
	if (T[0]==0 && LastWasPrompt)
		return false;

	LastWasPrompt = false;

	/* call next parser */
	return(parser2(T));
}


/*
  -- check for login-time patterns, perform console output
  and control password obfuscation
  -- if login is gone, calls next parser
*/
bool FicsProtocol::parser2(char *T) {

	if (!AuthGone) {
		Binder.prepare(T);

		if (Password.match())
			global.input->setPasswordMode(1);

		if (Login.match())
			global.input->setPasswordMode(0);

		if (WelcomeMessage.match()) {
			global.input->setPasswordMode(0);
			AuthGone = true;
			g_strlcpy(nick,WelcomeMessage.getToken(3),64);
			UserName.reset();
			UserName.append(new KleeneStarPat());
			UserName.append(new CIExactStringPat(nick));
			UserName.append(new KleeneStarPat());

			if (!InitSent) {
				sendInitialization();
				InitSent = true;
			}
		}
		return(doOutput(T,-1,false,global.Colors.TextBright));
	}

	return(parser3(T));
}

/*
  -- parse style-12 and zhouse stock lines, end with no output
  -- parse seek lines, output them depending on user settings, end
  -- parse seekinfo lines and update the seek table accordingly,
  end with no output
  -- call next parser
*/
bool FicsProtocol::parser3(char *T) {

	Binder.prepare(T);

	/* style 12 strings are parsed, end with no output */
	if (Style12.match()) {
		parseStyle12(T);
		LastWasStyle12 = true;
		return true;
	}

	/* bad (early) zhouse stock lines - ignore with no output unless
	   we are playing bughouse */
	if (Style12BadHouse.match()) {
		attachHouseStock(Style12BadHouse,true);
		return true;
	}

	/* zhouse stock line, parse, end with no output */
	if (Style12House.match()) {
		attachHouseStock(Style12House,false);
		return true;
	}

	/* human-readable seek line */
	if (Seek.match()) {

		if (global.HideSeeks && global.IcsSeekGraph)
			return true;

		return(doOutput(T, -1, false, global.Colors.Seeks));
	}

	/* seek table/graph lines */
	if (global.IcsSeekGraph) {

		if (SgClear.match()) {
			ensureSeekGraph();
			global.skgraph2->clear();
			return true;
		}

		if (SgAdd.match()) {
			seekAdd(SgAdd);
			return true;
		}

		if (SgAdd2.match()) {
			seekAdd(SgAdd2);
			return true;
		}

		if (SgRemove.match()) {
			seekRemove(SgRemove);
			return true;
		}
	} else {

		if (SgClear.match()) return true;
		if (SgAdd.match()) return true;
		if (SgAdd2.match()) return true;
		if (SgRemove.match()) return true;

	}

	// Ads Removed: msg
	if (SgRemove2.match()) return true;

	return(parser4(T));
}

/*
  -- treat "Ads on Server" retrieval, end with no output
  -- treat  "Games on Server" retrieval, end with no output
  -- treat move list retrieval, end with no output
  -- call next parser
*/
bool FicsProtocol::parser4(char *T) {

	Binder.prepare(T);

	/* ad list retrieval for Windows -> Ads on Server */
	if (RetrievingAdList) {

		if (AdListEntry.match()) {
			sendAdListEntry(T);
			return true;
		}

		if (EndOfAdList.match()) {
			RetrievingAdList = false;
			lastALC->endOfList();
			lastALC = 0;
			return true;
		}
	}

	/* game list retrieval for Windows -> Games on Server */
	if (RetrievingGameList) {

		if (GameListEntry.match()) {
			sendGameListEntry();
			return true;
		}

		if (EndOfGameList.match()) {
			RetrievingGameList = false;
			lastGLC->endOfList();
			lastGLC = 0;
			return true;
		}

	}

	/* move list retrieval */
	switch(RetrievingMoves) {
	case 0:
		break;
	case 1:
		if (MovesHeader.match()) {
			++RetrievingMoves;
			g_strlcpy(xxplayer[xxnext], MovesHeader.getSToken(0), 64);
			g_strlcpy(xxrating[xxnext], MovesHeader.getStarToken(0), 64);
			xxnext=(++xxnext)%2;
			g_strlcpy(xxplayer[xxnext], MovesHeader.getSToken(1), 64);
			g_strlcpy(xxrating[xxnext], MovesHeader.getStarToken(1), 64);
			xxnext=(++xxnext)%2;
			return true;
		}
		break;
	case 2:
		if (MovesHeader2.match()) {
			++RetrievingMoves;
			return true;
		}
		break;
	case 3:
		if (MovesHeader3.match()) {
			++RetrievingMoves;
			return true;
		}
		break;
	case 4:
		if (MovesHeader4.match()) {
			++RetrievingMoves;
			clearMoveBuffer();
			return true;
		}
		break;
	case 5:
		if (MovesBody.match())  { retrieve1(MovesBody);  return true; }
		if (MovesBody2.match()) { retrieve2(MovesBody2); return true; }
		if (MovesEnd.match()) {
			retrievef();

			if (MoveRetrievals.empty())
				RetrievingMoves = 0;
			else
				RetrievingMoves = 1;

			return true;
		}
		break;

	} /* switch RetrievingMoves */

	return(parser5(T));
}

/*
  -- treat game creation (1), echo line to console, end
  -- treat game creation (2), end with no output
  -- treat adjournment resume, end with no output
  -- treat observation/examination start/end, end with no output
  -- treat game status retrieval, end with output
  -- treat game information line (wild/fr, wild/[012] issue)
  -- call next parser
*/
bool FicsProtocol::parser5(char *T) {

	Binder.prepare(T);

	if (CreateGame0.match()) {
		g_strlcpy(xxplayer[xxnext],CreateGame0.getSToken(0),64);
		g_strlcpy(xxrating[xxnext],CreateGame0.getStarToken(0),64);
		xxnext=(++xxnext)%2;
		g_strlcpy(xxplayer[xxnext],CreateGame0.getSToken(1),64);
		g_strlcpy(xxrating[xxnext],CreateGame0.getStarToken(1),64);
		xxnext=(++xxnext)%2;
		return(doOutput(T,-1,false,global.Colors.TextBright));
	}

	if (CreateGame.match()) {
		createGame(CreateGame);
		return true;
	}

	if (ContinueGame.match()) {
		createGame(ContinueGame);
		/* fics sends 2 moves lists (!?), we ignore one of them */
		retrieveMoves(ContinueGame);
		return true;
	}

	if (BeginObserve.match()) {
		addGame(BeginObserve);
		return true;
	}

	if (EndObserve.match()) {
		int gn;
		ChessGame *cg;
		gn = atoi(EndObserve.getNToken(0));
		cg = global.getGame(gn);
		if (cg!=NULL)
			if (! cg->isOver())
				removeGame(EndObserve); // if not over, user typed unob himself
		return true;
	}

	if (EndExamine.match()) {
		removeGame(EndExamine);
		return true;
	}

	if (PendingStatus>0 && ExaGameStatus.match()) {
		updateGame(ExaGameStatus);
		--PendingStatus;
		return(doOutput(T,-1,false,global.Colors.TextBright));
	}

	if (PendingStatus>0 && GameStatus.match()) {
		updateGame(GameStatus);
		--PendingStatus;

	}

	if (ObsInfoLine.match()) {
		int gm;
		ChessGame *cg;
		gm = atoi(ObsInfoLine.getNToken(0));
		cg = global.getGame(gm);
		if (cg!=NULL) {
			char *p;
			p = ObsInfoLine.getRToken(0);
			if (!strcmp(p,"wild/fr")) {
				cg->Variant = WILDFR;
			} else if (!strcmp(p,"wild/2")) {
				cg->Variant = WILDNOCASTLE;
			} else if ((!strcmp(p,"wild/0"))||(!strcmp(p,"wild/1"))) {
				cg->Variant = WILDCASTLE;
			}
		}
		return(doOutput(T,-1,false,global.Colors.TextBright));
	}

	return(parser6(T));
}

/*
  -- treat game termination, end with output
  -- call next parser
*/
bool FicsProtocol::parser6(char *T) {
	int tb = global.Colors.TextBright;

	Binder.prepare(T);

	if (WhiteWon.match()) {
		gameOver(WhiteWon, WHITE_WIN);
		return(doOutput(T,-1,false,tb));
	}

	if (BlackWon.match()) {
		gameOver(BlackWon, BLACK_WIN);
		return(doOutput(T,-1,false,tb));
	}

	if (Drawn.match()) {
		gameOver(Drawn, DRAW);
		return(doOutput(T,-1,false,tb));
	}

	if (Interrupted.match()) {
		gameOver(Interrupted, UNDEF);
		return(doOutput(T,-1,false,tb));
	}

	return(parser7(T));
}

/*
  -- treat start of partner game in bughouse, end with output
  -- treat the hidden auto-allob feature
  -- call the next parser
*/
bool FicsProtocol::parser7(char *T) {

	Binder.prepare(T);

	if (GotPartnerGame.match()) {
		prepareBugPane();
		return(doOutput(T,-1,false,global.Colors.TextBright));
	}

	if (!AllObReqs.empty() || AllObState!=0) {
		switch(AllObState) {
		case 0:
			if (AllObNone.match()) {
				if (atoi(AllObNone.getNToken(0)) == AllObReqs.back()) {
					snprintf(AllObAcc,1024,"Game %d: no observers.",AllObReqs.back());
					AllObReqs.pop_back();
					global.status->setText(AllObAcc,20);
					return true;
				} else {
					return(doOutput(T,-1,false,global.Colors.TextBright));
				}
			}
			if (AllObFirstLine.match())
				if (doAllOb1())
					return true;
				else
					return(doOutput(T,-1,false,global.Colors.TextBright));
			break; /* case 0 */
		case 1:
			if (AllObMidLine.match()) {
				doAllOb2();
				return true;
			}
			break; /* case 1 */
		case 2:
			if (strstr(T, "user")!=0)
				return true;
			if (T[0] == 0) {
				AllObState = 3;
				return true;
			}
		case 3:
			if (strstr(T,"1 game displayed")!=0) {
				AllObState = 0;
				return true;
			}
			break;
		}
	} /* allob treatment */

	return(parser8(T));
}

/*
  -- colorize and output chat lines
  -- treat ms ivar
*/
bool FicsProtocol::parser8(char *T) {
	int msgcolor  = global.Colors.TextDefault;
	bool personal = false;
	ExtPatternMatcher *tm=0;

	Binder.prepare(T);

	if (UserName.match()) {
		msgcolor = global.Colors.PrivateTell;
		personal = true;
	}

	/* normal continuations */
	if (T[0]=='\\')
		return(doOutput(T,LastChannel,personal,personal?msgcolor:LastColor));

	/* lecturebot's stupid fake channel tells */
	if (LecBotSpecial)
		if (LectureBotXTellContinued.match())
			return(doOutput(T,LastChannel,false,LastColor));
		else
			LecBotSpecial = false;

	if (Challenge.match()) global.challenged();

	if (PrivateTell.match()) tm = &PrivateTell;
	if (Say.match())         tm = &Say;
	if (Say2.match())        tm = &Say2;

	/* update chat mode command after a direct tell */
	if (tm != 0) {
		string x;
		char xn[64], *xp;
		x = "t ";
		g_strlcpy(xn, tm->getAToken(0), 64);
		xp = strchr(xn,'('); if (xp) *xp = 0;
		xp = strchr(xn,'['); if (xp) *xp = 0;
		x += xn;
		global.ConsoleReply = x;
		global.input->updatePrefix();
		global.privatelyTold();
	}

	if (UserName.match()) {
		msgcolor = global.Colors.PrivateTell;
		personal = true;

		if ( (!ChannelTell.match()) &&
			 (!LectureBotXTell.match()) )
			return(doOutput(T,-1,personal,msgcolor));
	}

	if (LectureBotXTell.match()) {
		LecBotSpecial = true;
		if (!personal) msgcolor = global.Colors.ChannelTell;
		return(doOutput(T,67,personal,msgcolor));
	}

	if (PTell.match()) {
		global.privatelyTold();
		global.bugpane->addBugText(PTell.getStarToken(0));
		return(doOutput(T,-1,true,global.Colors.PrivateTell));
	}

	if (PrivateTell.match() || Say.match() || Say2.match()) {
		return(doOutput(T,-1,true,global.Colors.PrivateTell));
	}

	if (DrawOffer.match()) {
		global.drawOffered();
		return(doOutput(T,-1,true,global.Colors.PrivateTell));
	}

	if (News.match() || Notify.match())
		return(doOutput(T,-1,false,global.Colors.NewsNotify));

	if (Shout.match() || cShout.match() || It.match())
		return(doOutput(T,-1,false,global.Colors.Shouts));

	if (Kibitz.match() || Whisper.match())
		return(doOutput(T,-1,false,global.Colors.KibitzWhisper));

	if (ChannelTell.match()) {
		if (strchr(ChannelTell.getStarToken(0),' ')==0) {
			int ch;
			ch=atoi(ChannelTell.getNToken(0));
			if (!personal) msgcolor = global.Colors.ChannelTell;
			return(doOutput(T,ch,personal,msgcolor));
		}
	}

	if (T[0]==':')
		return(doOutput(T,-1,personal,global.Colors.Mamer));

	if (MsSet.match()) {
		UseMsecs = true;
		return(doOutput(T,-1,personal,msgcolor));
	}

	/* nothing matched, so much CPU for nothing ;-) */

	return(doOutput(T,-1,personal,msgcolor));
}

bool FicsProtocol::doOutput(char *msg, int channel,
							bool personal, int msgcolor)
{
	LastChannel = channel;
	LastColor   = msgcolor;

	if (channel<0 || !global.SplitChannels) {
		global.output->append(msg, msgcolor,
							  (personal)?IM_PERSONAL:IM_NORMAL);
		return true;
	}

	global.appendToChannel(channel, msg, msgcolor,
						   (personal)?IM_PERSONAL:IM_NORMAL);

	if (global.ChannelsToConsoleToo)
		global.output->append(msg, msgcolor,
							  (personal)?IM_PERSONAL:IM_NORMAL);

	return true;
}

void FicsProtocol::createExaminedGame(int gameid,char *whitep,char *blackp) {
	ChessGame *cg;
	Board *b;
	char z[64];

	global.debug("FicsProtocol","createExaminedGame");

	cg=new ChessGame();
	cg->clock_regressive=1;
	cg->GameNumber=gameid;
	cg->StopClock=1;
	cg->source = GS_ICS;
	g_strlcpy(cg->PlayerName[0],whitep,64);
	g_strlcpy(cg->PlayerName[1],blackp,64);
	cg->MyColor=WHITE|BLACK;
	cg->protodata[0]=258;

	global.prependGame(cg);
	b=new Board();
	b->reset();
	cg->setBoard(b);
	b->setGame(cg);
	b->setCanMove(true);
	b->setSensitive(true);

	snprintf(z,64,_("Exam.Game #%d"),cg->GameNumber);
	global.ebook->addPage(b->widget,z,cg->GameNumber);
	b->setNotebook(global.ebook,cg->GameNumber);

	if (global.PopupSecondaryGames) {
		b->pop();
		b->repaint();
	}

	cg->acknowledgeInfo();

	snprintf(z,64,"game %d",cg->GameNumber);
	global.network->writeLine(z);
	PendingStatus++;
}

void FicsProtocol::sendInitialization() {
	char z[128];
	if (!InitSent) {

		snprintf(z,128,"set interface eboard %s/%s\niset startpos 1\niset defprompt 1\niset ms 1",
				 global.Version,global.SystemType);
		global.network->writeLine(z);
		if (global.Premove)
			global.network->writeLine("iset premove 1\n");
		else
			global.network->writeLine("iset premove 0\n");
		if (global.IcsSeekGraph)
			global.network->writeLine("iset seekinfo 1\niset seekremove 1");
		else
			global.network->writeLine("iset seekremove 1");
		global.network->writeLine("iset lock 1\nstyle 12");
		InitSent = true;
	}
}

// creates a non-moveable, non-editable position for sposition
void FicsProtocol::createScratchPos(int gameid,char *whitep,char *blackp)
{
	ChessGame *cg;
	Board *b;
	char z[64];

	global.debug("FicsProtocol","createScratchPos");

	cg=new ChessGame();
	cg->clock_regressive=1;
	cg->GameNumber=gameid;
	cg->StopClock=1;
	cg->source=GS_ICS;
	g_strlcpy(cg->PlayerName[0],whitep,64);
	g_strlcpy(cg->PlayerName[1],blackp,64);
	cg->MyColor=WHITE|BLACK;
	cg->protodata[0]=259;

	global.prependGame(cg);
	b=new Board();
	b->reset();
	cg->setBoard(b);
	b->setGame(cg);
	b->setCanMove(false);
	b->setSensitive(false);

	snprintf(z,64,_("Pos: %s vs. %s"),whitep,blackp);
	global.ebook->addPage(b->widget,z,cg->GameNumber);
	b->setNotebook(global.ebook,cg->GameNumber);

	cg->acknowledgeInfo();
}

void FicsProtocol::createGame(ExtPatternMatcher &pm) {
	ChessGame *cg,*rep;
	char *p;

	global.debug("FicsProtocol","createGame");

	cg=new ChessGame();

	cg->source=GS_ICS;
	cg->clock_regressive=1;
	cg->GameNumber=atoi(pm.getNToken(0));
	g_strlcpy(cg->PlayerName[0],pm.getSToken(0),64);
	g_strlcpy(cg->PlayerName[1],pm.getSToken(1),64);

	// FICS sends two "Creating" strings when resuming an adjourned
	// game. Weird.
	rep=global.getGame(cg->GameNumber);
	if (rep) {
		if ( (!strcmp(cg->PlayerName[0],rep->PlayerName[0]))&&
			 (!strcmp(cg->PlayerName[1],rep->PlayerName[1]))&&
			 (rep->isFresh()) ) {
			delete cg;
			return;
		}
	}

	p=knowRating(cg->PlayerName[0]);
	if (p) g_strlcpy(cg->Rating[0],p,32);
	p=knowRating(cg->PlayerName[1]);
	if (p) g_strlcpy(cg->Rating[1],p,32);
	clearRatingBuffer();

	if (!strncmp(cg->PlayerName[0],nick,strlen(cg->PlayerName[0]))) {
		cg->MyColor=WHITE;
		UserPlayingWithWhite = true;
	}
	if (!strncmp(cg->PlayerName[1],nick,strlen(cg->PlayerName[1]))) {
		cg->MyColor=BLACK;
		UserPlayingWithWhite = false;
	}

	if (!strcmp(pm.getSToken(2),"rated"))
		cg->Rated=1;
	else
		cg->Rated=0;

	cg->Variant=REGULAR;
	if (!strcmp(pm.getRToken(0),"crazyhouse"))    cg->Variant=CRAZYHOUSE;
	else if (!strcmp(pm.getRToken(0),"bughouse")) cg->Variant=BUGHOUSE;
	else if (!strcmp(pm.getRToken(0),"suicide"))  cg->Variant=SUICIDE;
	else if (!strcmp(pm.getRToken(0),"losers"))   cg->Variant=LOSERS;
	else if (!strcmp(pm.getRToken(0),"atomic"))   cg->Variant=ATOMIC;
	else if (!strncmp(pm.getRToken(0),"wild",4)) {
		cg->Variant=WILD;
		if (!strncmp(pm.getRToken(0),"wild/0",6))  cg->Variant=WILDCASTLE;
		if (!strncmp(pm.getRToken(0),"wild/1",6))  cg->Variant=WILDCASTLE;
		if (!strncmp(pm.getRToken(0),"wild/2",6))  cg->Variant=WILDNOCASTLE;
		if (!strncmp(pm.getRToken(0),"wild/fr",7)) cg->Variant=WILDFR;
	}

	global.prependGame(cg);
	global.BoardList.front()->reset();

	cg->setBoard(global.BoardList.front());
	global.BoardList.front()->setGame(cg);
	global.BoardList.front()->pop();
	global.BoardList.front()->setCanMove(true);
	global.BoardList.front()->repaint();

	cg->acknowledgeInfo();
	global.status->setText(_("Game started!"),10);
	global.gameStarted();
}

void FicsProtocol::addGame(ExtPatternMatcher &pm) {
	ChessGame *cg;
	Board *b;
	bool is_partner_game=false;
	char tab[96];

	global.debug("FicsProtocol","addGame");

	cg=new ChessGame();

	cg->clock_regressive=1;
	cg->GameNumber=atoi(pm.getNToken(0));
	cg->Rated=0;
	cg->Variant=REGULAR;
	cg->source = GS_ICS;

	global.prependGame(cg);
	b=new Board();
	b->reset();
	cg->setBoard(b);
	b->setGame(cg);
	b->setCanMove(false);
	b->setSensitive(false);

	if (cg->GameNumber == PartnerGame) {
		is_partner_game=true;
		PartnerGame = -1;
		cg->protodata[2] = 1;
	}

	snprintf(tab,96,_("Game #%d"),cg->GameNumber);
	global.ebook->addPage(b->widget,tab,cg->GameNumber);
	b->setNotebook(global.ebook,cg->GameNumber);

	if (!is_partner_game)
		if (global.PopupSecondaryGames) {
			b->pop();
			b->repaint();
		}

	cg->acknowledgeInfo();

	// to gather info about time, variant, player names...
	snprintf(tab,96,"game %d",cg->GameNumber);
	global.network->writeLine(tab);
	PendingStatus++;
}

void FicsProtocol::discardGame(int gameid) {
	ChessGame *cg;
	Board *b;
	char z[64];

	global.debug("FicsProtocol","discardGame");

	cg=global.getGame(gameid);
	if (!cg)
		return;

	switch(cg->protodata[0]) {
	case 257: // observed
		snprintf(z,64,"unobserve %d",cg->GameNumber);
		break;
	case 258: // examined
		strcpy(z,"unexamine");
		break;
	case 259: // isolated position
		z[0]=0;
		break;
	default:
		cerr << "protodata for game " << gameid << " is " << cg->protodata[0] << endl;
		return;
	}

	if (! cg->isOver())
		if (strlen(z))
			global.network->writeLine(z);

	b=cg->getBoard();
	if (b==0) return;
	global.ebook->removePage(cg->GameNumber);
	global.removeBoard(b);
	delete(b);
	cg->GameNumber=global.nextFreeGameId(10000);
	cg->setBoard(0);

}

void FicsProtocol::removeGame(ExtPatternMatcher &pm) {
	int gm;
	global.debug("FicsProtocol","removeGame");

	gm=atoi(pm.getNToken(0));

	if (global.SmartDiscard && global.ebook->isNaughty(gm))
		return;

	innerRemoveGame(gm);
}

void FicsProtocol::innerRemoveGame(int gameid) {
	Board *b;
	ChessGame *cg;

	cg=global.getGame(gameid);
	if (cg==NULL)
		return;

	// bughouse partner game ?
	if (cg->protodata[2])
		global.bugpane->stopClock();

	b=cg->getBoard();
	if (b==NULL) return;
	global.ebook->removePage(gameid);
	global.removeBoard(b);
	delete(b);
	cg->GameNumber=global.nextFreeGameId(10000);
	cg->setBoard(0);
}

void FicsProtocol::sendDrop(piece p,int x,int y) {
	piece k;
	char drop[5];

	global.debug("FicsProtocol","sendDrop");

	k=p&PIECE_MASK;
	drop[4]=0;
	switch(k) {
	case PAWN:   drop[0]='P'; break;
	case ROOK:   drop[0]='R'; break;
	case KNIGHT: drop[0]='N'; break;
	case BISHOP: drop[0]='B'; break;
	case QUEEN:  drop[0]='Q'; break;
	default: return;
	}
	drop[1]='@';
	drop[2]='a'+x;
	drop[3]='1'+y;
	global.network->writeLine(drop);
}

void FicsProtocol::sendMove(int x1,int y1,int x2,int y2,int prom) {
	char move[7];
	piece pp;

	global.debug("FicsProtocol","sendMove");

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
	global.network->writeLine(move);
}

// attach stock info to bug/zhouse games
void FicsProtocol::attachHouseStock(ExtPatternMatcher &pm, bool OnlyForBughouse) {
	ChessGame *cg;
	int gid;
	char *p;
	Position *pos;

	global.debug("FicsProtocol","attachHouseStock");

	// Style12House or Style12BadHouse
	//  <b1> game %n white [*] black [*]*
	//  <b1> game %n white [*] black [*]%b<-*

	gid=atoi(pm.getNToken(0));

	cg=global.getGame(gid);
	if (!cg)
		return;

	if (OnlyForBughouse && cg->Variant != BUGHOUSE)
		return;

	pos=&(cg->getLastPosition());

	pos->clearStock();
	p=pm.getStarToken(0);

	//  cerr << "[" << p << "]\n";

	for(;*p;p++)
		pos->incStockCount(style12table[*p]);

	p=pm.getStarToken(1);

	// cerr << "[" << p << "]\n";

	for(;*p;p++)
		pos->incStockCount(style12table[tolower(*p)]);

	cg->updateStock();
}

void FicsProtocol::parseStyle12(char *data) {
	static char *sep=" \t\n";
	tstring t;
	int i,j;
	string *sp;
	Position pos;
	int blacktomove;
	int game;
	int rel;
	int itime,inc,flip;
	int str[2],movenum,clk[2];
	ChessGame *refgame;
	char stra[64],strb[64],strc[64];
	char plyr[2][64];
	int ackp=0;
	int stopclock = -1; /* -1: keep as it is, 0: stop 1: moving */
	bool spawnExamination;
	bool someSound;

	Position prevpos;

	global.debug("FicsProtocol","parseStyle12");

	t.set(data);
	t.token(sep); // <12>

	// the position itself
	for(i=7;i>=0;i--) {
		sp=t.token(sep);
		for(j=0;j<8;j++)
			pos.setPiece(j,i,style12table[sp->at(j)]);
	}

	sp=t.token(sep); // B / W (color to move)
	blacktomove=(sp->at(0)=='B');

	pos.ep[0]=t.tokenvalue(sep); // double pawn push
	if (blacktomove)
		pos.ep[1]=2;
	else
		pos.ep[1]=5;

	// white castle short
	pos.maycastle[0]=t.tokenvalue(sep)?true:false;

	// white castle long
	pos.maycastle[2]=t.tokenvalue(sep)?true:false;

	// black castle short
	pos.maycastle[1]=t.tokenvalue(sep)?true:false;

	// black castle long
	pos.maycastle[3]=t.tokenvalue(sep)?true:false;

	t.token(sep); // mv count for draw

	game=t.tokenvalue(sep); // game number

	memset(plyr[0],0,64);
	memset(plyr[1],0,64);
	t.token(sep)->copy(plyr[0],63); // white's name
	t.token(sep)->copy(plyr[1],63); // black's name

	rel=t.tokenvalue(sep); // relationship

	switch(rel) {
	case 2: // I am the examiner of this game
		spawnExamination=false;
		refgame=global.getGame(game);

		if (refgame==NULL)
			spawnExamination=true;
		else // v_examine=1 on FICS: played game still on main board but inactive
			if ( (refgame->protodata[0]!=258) && (refgame->isOver()) )
				spawnExamination=true;

		if (spawnExamination)
			createExaminedGame(game,plyr[0],plyr[1]);
		break;

	case -3: // isolated position (e.g.: sposition command)
		game=global.nextFreeGameId(10000);
		createScratchPos(game,plyr[0],plyr[1]);
		break;
	case -4: // initial position of wild game
		WildStartPos = pos;
		return;
	case -2: // observing examined game
	case -1: // I am playing, it's my opponent's turn
	case 1: // I am playing, it's my turn
	case 0: // live observation
		break;
	}

	itime=t.tokenvalue(sep); // initial time (minutes)
	inc=t.tokenvalue(sep);   // increment (secs)

	str[0]=t.tokenvalue(sep);  // white strength
	str[1]=t.tokenvalue(sep);  // black strength

	clk[0]=t.tokenvalue(sep);  // white rem time
	clk[1]=t.tokenvalue(sep);  // black rem time

	if (!UseMsecs) {
		clk[0] *= 1000;
		clk[1] *= 1000;
	}

	movenum=t.tokenvalue(sep); // move # to be made

	t.token(sep);               // verbose for previous move

	memset(stra,0,64);
	memset(strb,0,64);
	t.token(sep)->copy(stra,63); // time taken for previous move
	t.token(sep)->copy(strb,63); // pretty print for previous move

	flip=t.tokenvalue(sep);    // flip board ?

	t.setFail(-1);
	stopclock = t.tokenvalue(sep); // undocumented pause/unpause clock flag
	if (stopclock != -1)
		stopclock = 1-stopclock;

	refgame=global.getGame(game);
	if (refgame==NULL) {
		cerr << "no such game: " << game << endl;
		return;
	}

	switch(rel) {
	case  0: // observed
	case -2: // observing examined game
		refgame->protodata[0]=257;
		refgame->AmPlaying=false;
		break;
	case 1:
	case -1:
		refgame->protodata[0]=0;
		refgame->MyColor=(flip)?BLACK:WHITE;
		refgame->AmPlaying=true;
		if ((global.IcsSeekGraph)&&(global.skgraph2))
			global.skgraph2->clear();
		break;
	case 2: // examiner mode
		refgame->protodata[0]=258;
		refgame->setFree();
		refgame->AmPlaying=false;
		break;
	case -3: // isolated position
		refgame->protodata[0]=259;
		refgame->AmPlaying=false;
		break;
	}

	if (strcmp(refgame->PlayerName[0],plyr[0])) {
		strcpy(refgame->PlayerName[0],plyr[0]);
		ackp=1;
	}

	if (strcmp(refgame->PlayerName[1],plyr[1])) {
		strcpy(refgame->PlayerName[1],plyr[1]);
		ackp=1;
	}

	if ( (refgame->timecontrol.value[0] != itime*60)||(refgame->timecontrol.value[1] != inc) ) {
		refgame->timecontrol.setIcs(itime*60,inc);
		ackp=1;
	}

	if (ackp)
		refgame->acknowledgeInfo();

	g_strlcat(strb," ",64);
	g_strlcat(strb,stra,64);
	snprintf(strc,64,"%d.%s%s",
			 (blacktomove?movenum:movenum-1),
			 (blacktomove?" ":" ... "),
			 strb);
	if (strc[0]=='0')
		strcpy(strc,"0. startpos");

	if ((rel==-1)&&(global.AnimateMoves))
		refgame->getBoard()->supressNextAnimation();

	if (rel==-2) // don't let it tick when observing examined games
		refgame->StopClock=1;

	someSound = (rel==1 || rel==0 || rel==2 || rel==-2) && (movenum!=1 || blacktomove);

	if (stopclock >= 0)
		refgame->StopClock = stopclock;

	prevpos = refgame->getLastPosition();

	refgame->updatePosition2(pos, (movenum-(blacktomove?0:1)), blacktomove,
							 clk[0], clk[1], strc, someSound);

	/*
	  if ( ((rel==-1)&&(blacktomove)) || ((rel==1)&&(!blacktomove)) ) {
	  if (flip) cerr << "I'm white and <12> told me to flip!\n";
	  }

	  if ( ((rel==1)&&(blacktomove)) || ((rel==-1)&&(!blacktomove)) ) {
	  if (!flip) cerr << "I'm black and <12> told me not to flip!\n";
	  }
	*/

	if (rel==1 && (movenum!=1 || blacktomove) )
		global.opponentMoved();

	if (rel==0 || rel==-2 || rel==2 && (movenum!=1 || blacktomove))
		if (prevpos != pos)
			global.moveMade();

	if (rel!=0) refgame->flipHint(flip);
	if (rel==1) refgame->enableMoving(true);
	if (rel==-1) refgame->enableMoving(false);

	if (global.IcsAllObPlayed && (rel==1 || rel==-1) )
		if (refgame->protodata[3] == 0) {
			refgame->protodata[3] = gtk_timeout_add(30*1000, fics_allob, (void *) refgame);
			fics_allob((gpointer) refgame);
		}

	if (global.IcsAllObObserved && (rel==0) )
		if (refgame->protodata[3] == 0) {
			refgame->protodata[3] = gtk_timeout_add(30*1000, fics_allob, (void *) refgame);
			fics_allob((gpointer) refgame);
		}

}

void FicsProtocol::updateGame(ExtPatternMatcher &pm) {
	ChessGame *refgame;
	char *p,z[32];
	int gm;

	global.debug("FicsProtocol","updateGame");

	// Ns: 0=game 1=rat.white 2=rat.black
	// Ss: 0=whitep 1=blackp
	// *s: 1=game string
	//  GameStatus.set("*%n%b%n%b%S%b%n%b%S%b[*]*");
	//  ExaGameStatus.set("*%n (Exam.%b%n %S%b%n %S%b)%b[*]*");

	gm=atoi(pm.getNToken(0));

	refgame=global.getGame(gm);
	if (refgame==NULL) {
		PendingStatus++;
		return;
	}

	g_strlcpy(refgame->PlayerName[0],pm.getSToken(0),64);
	g_strlcpy(refgame->PlayerName[1],pm.getSToken(1),64);

	p=pm.getStarToken(1);

	switch(p[1]) {
	case 's':
	case 'b':
	case 'l':
		refgame->Variant=REGULAR;
		break;
	case 'z':
		refgame->Variant=CRAZYHOUSE;
		break;
	case 'S':
		refgame->Variant=SUICIDE;
		break;
	case 'w':
		if (IS_NOT_WILD(refgame->Variant)) refgame->Variant=WILD;
		break;
	case 'B':
		refgame->Variant=BUGHOUSE;
		break;
	case 'L':
		refgame->Variant=LOSERS;
		break;
	case 'x':
		refgame->Variant=ATOMIC;
		break;
	}

	refgame->Rated=(p[2]=='r');

	memset(z,0,8);
	memcpy(z,&p[3],3);

	refgame->timecontrol.setIcs(60*atoi(z),atoi(&p[7]));
	refgame->acknowledgeInfo();

	// bughouse partner game, set up names in the bugpane
	if (refgame->protodata[2]!=0) {
		global.bugpane->setPlayerNames(refgame->PlayerName[0],
									   refgame->PlayerName[1]);
	}

	if (refgame->protodata[0]!=258) {
		MoveRetrievals.push_front(gm);
		snprintf(z,32,"moves %d",gm);
		global.network->writeLine(z);
		RetrievingMoves=1;
	}

}

void FicsProtocol::gameOver(ExtPatternMatcher &pm, GameResult result) {
	int game;
	char reason[64];
	ChessGame *refgame;

	global.debug("FicsProtocol","gameOver");

	game=atoi(pm.getNToken(0));
	reason[63]=0;
	g_strlcpy(reason,pm.getStarToken(0),64);

	refgame=global.getGame(game);
	if (refgame==NULL) {
		cerr << _("no such game: ") << game << endl;
		return;
	}

	// allob timeout
	if (refgame->protodata[3]!=0) {
		gtk_timeout_remove(refgame->protodata[3]);
		refgame->protodata[3] = 0;
	}

	if (refgame->protodata[0]==0) {
		if ( (   UserPlayingWithWhite  && result==WHITE_WIN) ||
			 ( (!UserPlayingWithWhite) && result==BLACK_WIN) )
			global.gameWon();

		if ( (   UserPlayingWithWhite  && result==BLACK_WIN) ||
			 ( (!UserPlayingWithWhite) && result==WHITE_WIN) )
			global.gameLost();
	}

	if (refgame->protodata[0]==257) {
		global.gameFinished();
	}

	refgame->endGame(reason,result);

	if (((refgame->protodata[0]==0)&&(global.AppendPlayed))
		||
		((refgame->protodata[0]==257)&&(global.AppendObserved)))
		{
			if  (refgame->savePGN(global.AppendFile,true)) {
				char z[128];
				snprintf(z,128,_("Game appended to %s"),global.AppendFile);
				global.status->setText(z,5);
			}
		}

	// refresh seek graph
	if ((global.IcsSeekGraph)&&(refgame->protodata[0]==0))
		refreshSeeks(true);

	// remove board ?
	// if game relation is observation and user has the Smart Discard setting on:
	//  discard now if user is not looking to the board or schedule removal to
	//  next pane switch if user _is_ looking to the board
	if ((refgame->protodata[0]==257)&&(global.SmartDiscard)) {
		if ( global.ebook->getCurrentPageId() == refgame->GameNumber )
			global.ebook->setNaughty(refgame->GameNumber,true);
		else
			innerRemoveGame(refgame->GameNumber);
	}
}

void FicsProtocol::refreshSeeks(bool create) {
	if (create) ensureSeekGraph();
	if ((global.IcsSeekGraph)&&(global.skgraph2)) {
		global.skgraph2->clear();
		global.network->writeLine("iset seekinfo 1");
	}
}

int  FicsProtocol::hasAuthenticationPrompts() {
	return 1;
}

void FicsProtocol::resign() {
	global.network->writeLine("resign");
	global.status->setText(_("Resigned."),10);
}

void FicsProtocol::draw() {
	global.network->writeLine("draw");
	global.status->setText(_("<Fics Protocol> draw request sent"),30);
}

void FicsProtocol::adjourn() {
	global.network->writeLine("adjourn");
	global.status->setText(_("<Fics Protocol> adjourn request sent"),30);
}

void FicsProtocol::abort() {
	global.network->writeLine("abort");
	global.status->setText(_("<Fics Protocol> abort request sent"),30);
}

void FicsProtocol::clearMoveBuffer() {
	global.debug("FicsProtocol","clearMoveBuffer");
	MoveBuf.clear();
}

void FicsProtocol::retrieve1(ExtPatternMatcher &pm) {
	char z[32],mv[32];
	char tmp[2][32];
	int itmp;
	Position p,q;
	int gm;
	ChessGame *cg;
	variant Vr=REGULAR;

	global.debug("FicsProtocol","retrieve1");

	gm=MoveRetrievals.back();
	cg=global.getGame(gm);
	if (cg!=NULL) Vr=cg->Variant;

	if (MoveBuf.empty()) {
		if (IS_WILD(Vr))
			MoveBuf.push_back(WildStartPos);
		else
			MoveBuf.push_back(Position());
	}
	p = MoveBuf.back();


	itmp=atoi(pm.getNToken(0));
	g_strlcpy(tmp[0],pm.getRToken(0),32);
	g_strlcpy(tmp[1],pm.getRToken(1),32);
	snprintf(z,32,"%.2d. %s %s",itmp,tmp[0],tmp[1]);
	g_strlcpy(mv,tmp[0],32);

	p.moveStdNotation(mv,WHITE,Vr);
	p.setLastMove(z);

	MoveBuf.push_back(p);

	q=p;

	itmp=atoi(pm.getNToken(0));
	g_strlcpy(tmp[0],pm.getRToken(2),32);
	g_strlcpy(tmp[1],pm.getRToken(3),32);
	snprintf(z,32,"%.2d. ... %s %s",itmp,tmp[0],tmp[1]);
	g_strlcpy(mv,tmp[0],32);

	q.moveStdNotation(mv,BLACK,Vr);
	q.setLastMove(z);

	MoveBuf.push_back(q);
}

void FicsProtocol::retrieve2(ExtPatternMatcher &pm) {
	Position p;
	char z[32],mv[32];
	char tmp[2][32];
	int itmp, gm;
	ChessGame *cg;
	variant Vr=REGULAR;

	global.debug("FicsProtocol","retrieve2");

	gm=MoveRetrievals.back();
	cg=global.getGame(gm);
	if (cg!=NULL) Vr=cg->Variant;

	if (MoveBuf.empty()) {
		if (IS_WILD(Vr))
			MoveBuf.push_back(WildStartPos);
		else
			MoveBuf.push_back(Position());
	}
	p = MoveBuf.back();

	itmp=atoi(pm.getNToken(0));
	g_strlcpy(tmp[0],pm.getRToken(0),32);
	g_strlcpy(tmp[1],pm.getRToken(1),32);
	snprintf(z,32,"%.2d. %s %s",itmp,tmp[0],tmp[1]);
	g_strlcpy(mv,tmp[0],32);

	p.moveStdNotation(mv,WHITE,Vr);
	p.setLastMove(z);

	MoveBuf.push_back(p);
}

void FicsProtocol::retrievef() {
	int gm;
	ChessGame *cg;
	char *p;

	global.debug("FicsProtocol","retrievef");

	if (MoveRetrievals.empty())
		return;

	gm=MoveRetrievals.back();

	cg=global.getGame(gm);
	if (cg==NULL) {
		cerr << _("no such game: ") << gm << endl;
		return;
	}
	MoveRetrievals.pop_back();

	p=knowRating(cg->PlayerName[0]);
	if (p) g_strlcpy(cg->Rating[0],p,32);
	p=knowRating(cg->PlayerName[1]);
	if (p) g_strlcpy(cg->Rating[1],p,32);
	clearRatingBuffer();

	cg->updateGame(MoveBuf);
	cg->acknowledgeInfo();
	clearMoveBuffer();
	WildStartPos.setStartPos();
}

void FicsProtocol::queryGameList(GameListConsumer *glc) {
	if (RetrievingGameList) {
		if (glc!=lastGLC)
			glc->endOfList();
		return;
	}

	RetrievingGameList=1;
	lastGLC=glc;
	global.network->writeLine("games");
}

void FicsProtocol::queryAdList(GameListConsumer *glc) {
	if (RetrievingAdList) {
		if (glc!=lastALC)
			glc->endOfList();
		return;
	}
	RetrievingAdList=1;
	lastALC=glc;
	global.network->writeLine("sough");
}

void FicsProtocol::sendGameListEntry() {
	char frank[256];
	int gn;
	//  GameListEntry.set("*%n *[*]*:*");

	gn=atoi(GameListEntry.getNToken(0));

	g_strlcpy(frank,GameListEntry.getStarToken(1),256);
	g_strlcat(frank,"[",256);
	g_strlcat(frank,GameListEntry.getStarToken(2),256);
	g_strlcat(frank,"]",256);
	g_strlcat(frank,GameListEntry.getStarToken(3),256);
	g_strlcat(frank,":",256);
	g_strlcat(frank,GameListEntry.getStarToken(4),256);

	lastGLC->appendGame(gn,frank);
}

void FicsProtocol::sendAdListEntry(char *pat) {
	ExtPatternMatcher rpat;
	int gn;

	rpat.set("*%n*");
	if (!rpat.match(pat))
		return;

	gn=atoi(rpat.getNToken(0));
	lastALC->appendGame(gn,rpat.getStarToken(1));
}

void FicsProtocol::observe(int gameid) {
	char z[64];
	snprintf(z,64,"observe %d",gameid);
	global.network->writeLine(z);
}

void FicsProtocol::answerAd(int adid) {
	char z[64];
	snprintf(z,64,"play %d",adid);
	global.network->writeLine(z);
}

char * FicsProtocol::knowRating(char *player) {
	int i;
	for(i=0;i<2;i++)
		if (strlen(xxplayer[i]))
			if ( !strncmp(xxplayer[i],player,strlen(player)) )
				return(xxrating[i]);
	return 0;
}

void FicsProtocol::clearRatingBuffer() {
	xxplayer[0][0]=0;
	xxplayer[1][0]=0;
}

void FicsProtocol::exaForward(int n) {
	char z[64];
	snprintf(z,64,"forward %d",n);
	global.network->writeLine(z);
}

void FicsProtocol::exaBackward(int n) {
	char z[64];
	snprintf(z,64,"backward %d",n);
	global.network->writeLine(z);
}

void FicsProtocol::retrieveMoves(ExtPatternMatcher &pm) {
	ChessGame *refgame;
	int gm;
	char z[64];
	global.debug("FicsProtocol","retrieveMoves");

	gm=atoi(pm.getNToken(0));
	refgame=global.getGame(gm);
	if (refgame==NULL)
		return;

	if (refgame->protodata[0]!=258) {
		MoveRetrievals.push_front(gm);
		snprintf(z,64,"moves %d",gm);
		global.network->writeLine(z);
		RetrievingMoves=1;
	}
}

// --- seek graph

void FicsProtocol::ensureSeekGraph() {
	if (global.skgraph2==NULL) {
		global.skgraph2=new SeekGraph2();
		global.skgraph2->show();
		global.ebook->addPage(global.skgraph2->widget,_("Seek Graph"),-3);
		global.skgraph2->setNotebook(global.ebook,-3);
	}
}

void FicsProtocol::seekAdd(ExtPatternMatcher &pm) {
	SeekAd *ad;
	char *p;
	int flags;

	global.debug("FicsProtocol","seekAdd");

	// <s> 8 w=visar ti=02 rt=2194  t=4 i=0 r=r tp=suicide c=? rr=0-9999 a=t f=f
	//    n0   s0       r0    n1  *0  n2  n3  s1    r1       r2    r3      s2  s3

	ensureSeekGraph();
	ad=new SeekAd();

	// %n
	ad->id      = atoi(pm.getNToken(0));
	ad->rating  = pm.getNToken(1);
	ad->clock   = atoi(pm.getNToken(2));
	ad->incr    = atoi(pm.getNToken(3));

	p=pm.getStarToken(0);
	if ((p[0]=='P')&&(atoi(ad->rating.c_str())==0)) ad->rating="++++";

	// %s
	ad->player  = pm.getSToken(0);

	p=pm.getSToken(1);
	if (p[0]=='r') ad->rated=true;

	ad->kind    = pm.getRToken(1);

	// %r
	flags = strtol(pm.getRToken(0),NULL,16);

	ad->flags=" ";

	if (flags&0x01) ad->flags += "(U)";
	if (flags&0x02) ad->flags += "(C)";
	if (flags&0x04) ad->flags += "(GM)";
	if (flags&0x08) ad->flags += "(IM)";
	if (flags&0x10) ad->flags += "(FM)";
	if (flags&0x20) ad->flags += "(WGM)";
	if (flags&0x40) ad->flags += "(WIM)";
	if (flags&0x80) ad->flags += "(WFM)";

	p=pm.getRToken(2);
	switch(p[0]) {
	case '?': ad->color=" "; break;
	case 'W': ad->color=_("white"); break;
	case 'B': ad->color=_("black"); break;
	}

	ad->range=pm.getRToken(3);

	p=pm.getSToken(2); if (p[0]=='t') ad->automatic=true;
	p=pm.getSToken(3); if (p[0]=='t') ad->formula=true;

	global.skgraph2->add(ad);
}

void FicsProtocol::seekRemove(ExtPatternMatcher &pm) {
	tstring t;
	int i;
	t.setFail(-1);
	t.set(pm.getStarToken(0));
	ensureSeekGraph();
	while( (i=t.tokenvalue(" \t\n")) >= 0 ) {
		global.skgraph2->remove(i);
	}
}

vector<string *> * FicsProtocol::getPlayerActions() {
	return(&PActions);
}

vector<string *> * FicsProtocol::getGameActions() {
	return(&GActions);
}

void FicsProtocol::callPlayerAction(char *player, string *action) {
	for(unsigned int i=0;i<PActions.size();i++)
		if ( (*PActions[i]) == (*action) ) { doPlayerAction(player, i); break; }
}

void FicsProtocol::callGameAction(int gameid, string *action) {
	for(unsigned int i=0;i<GActions.size();i++)
		if ( (*GActions[i]) == (*action) ) { doGameAction(gameid, i); break; }
}

void FicsProtocol::doPlayerAction(char *player, int id) {
	char z[256],w[256];
	bool ok=true;

	switch(id) {
	case 0: snprintf(z,256,"finger %s",player); break;
	case 1: snprintf(z,256,"stat %s",player); break;
	case 2: snprintf(z,256,"history %s",player); break;
	case 3: snprintf(z,256,"ping %s",player); break;
	case 4: snprintf(z,256,"log %s",player); break;
	case 5: snprintf(z,256,"var %s",player); break;
	default: ok=false;
	}

	if (ok) {
		snprintf(w,256,_("> [issued from menu] %s"),z);
		global.output->append(w,global.SelfInputColor);
		global.network->writeLine(z);
	}
}

void FicsProtocol::doGameAction(int gameid, int id) {
	char z[256], w[256];
	bool ok=true;

	switch(id) {
	case 0: snprintf(z,256,"allob %d",gameid); break;
	case 1: snprintf(z,256,"ginfo %d",gameid); break;
	default: ok=false;
	}

	if (ok) {
		snprintf(w,256,_("> [issued from menu] %s"),z);
		global.output->append(w,global.SelfInputColor);
		global.network->writeLine(z);
	}
}

void FicsProtocol::prepareBugPane() {
	char z[64];
	PartnerGame = atoi(GotPartnerGame.getNToken(0));
	snprintf(z,64,"observe %d",PartnerGame);
	global.network->writeLine(z);
}

void FicsProtocol::updateVar(ProtocolVar pv) {
	char z[256];
	switch(pv) {
	case PV_premove:
		snprintf(z,256,"iset premove %d\n",global.Premove ? 1 : 0);
		global.network->writeLine(z);
		break;
	default:
		break;
	}
}

gboolean fics_allob(gpointer data) {
	ChessGame *cg = (ChessGame *) data;
	FicsProtocol *proto;
	char z[64];

	if (global.protocol == 0)  return FALSE;
	if (cg == 0)               return FALSE;
	if (cg->protodata[3] == 0) return FALSE;
	if (cg->isOver())          return FALSE;
	if (cg->source != GS_ICS)  return FALSE;
	if (global.network == 0)   return FALSE;

	proto = (FicsProtocol *) (global.protocol);
	proto->AllObReqs.push_front(cg->GameNumber);

	snprintf(z,64,"allob %d",cg->GameNumber);
	global.network->writeLine(z);

	return TRUE;
}

// AllObFirstLine Matched
bool FicsProtocol::doAllOb1() {
	int n,i;
	tstring t;
	string *p;
	bool islast = false;

	//  cerr << "allob2\n";

	n = atoi(AllObFirstLine.getNToken(0));

	// cerr << "n0=[" << AllObFirstLine.getNToken(0) << "]\n";
	// cerr << "n=" << n << ", back=" << AllObReqs.back() << endl;
	if (n!=AllObReqs.back())
		return false;

	t.set(AllObFirstLine.getStarToken(1));

	snprintf(AllObAcc,1024,"Observing game %d: ",n);
	i=0;
	while((p=t.token(" "))!=0) {

		if ( (*p)[0] == '(' ) {
			islast = true;
			break;
		}

		if(i!=0) strcat(AllObAcc,", ");
		g_strlcat(AllObAcc,p->c_str(),1024);
		++i;

		//    cerr << "partial: [" << AllObAcc << "]\n";

		if (strlen(AllObAcc) > 128) {
			g_strlcat(AllObAcc,"...",1024);
			break;
		}
	}

	if (islast) {
		AllObState = 2;
		global.status->setText(AllObAcc,28);
		AllObReqs.pop_back();
	} else
		AllObState = 1;

	return true;
}

// AllObMidLine matched
void FicsProtocol::doAllOb2() {
	int i;
	bool islast = false;
	tstring t;
	string *p;

	t.set(AllObFirstLine.getStarToken(0));

	while((p=t.token(" "))!=0) {

		if ( (*p)[0] == '(' ) {
			islast = true;
			break;
		}
		if (strlen(AllObAcc) > 128)
			continue;

		g_strlcat(AllObAcc,", ",1024);
		g_strlcat(AllObAcc,p->c_str(),1024);
		++i;
		if (strlen(AllObAcc) > 128) {
			g_strlcat(AllObAcc,"...",1024);
			break;
		}
	}

	if (islast) {
		AllObState = 2;
		global.status->setText(AllObAcc,28);
		AllObReqs.pop_back();
	} else
		AllObState = 1;

}
