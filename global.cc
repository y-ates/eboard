/* $Id: global.cc,v 1.98 2008/02/22 07:32:16 bergo Exp $ */

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <dlfcn.h>

#define GLOBAL_CC 1

#include "global.h"
#include "text.h"
#include "config.h"

#include "chess.h"
#include "tstring.h"
#include "notebook.h"
#include "board.h"
#include "quickbar.h"
#include "pieces.h"
#include "eboard.h"


/* Global variables needed to cleanly close the driver */
extern int (*dgtnixClose)();
extern bool DgtInit;
extern void *dgtnix_dll_handle;
Global global;

// stream ops

ostream & operator<<(ostream &s, WindowGeometry w) {
	s << '(' << w.X << ',' << w.Y << ',';
	s << w.W << ',' << w.H << ')';
	return(s);
}

ostream & operator<<(ostream &s, Desktop d) {
	s << d.wMain << ',' << d.wGames << ',' << d.wLocal << ',';
	s << d.wAds << ',' << d.PanePosition                          ;
	return(s);
}

ostream & operator<<(ostream &s, TimeControl tc) {
	TimeControl x;
	x = tc;
	if (x.mode == TC_SPM) x.value[1] = 0;
	if (x.mode == TC_NONE) x.value[0] = x.value[1] = 0;
	s << '/' << ((int)(x.mode)) << '/' << x.value[0];
	s << '/' << x.value[1];
	return(s);
}

ostream & operator<<(ostream &s, EngineBookmark b) {
	s << b.caption << '^';
	if (b.directory.empty()) s << "NULL"; else s << b.directory;
	s << '^';
	if (b.cmdline.empty()) s << "NULL"; else s << b.cmdline;
	s << '^' << b.humanwhite << '^' << b.timecontrol << '^';
	s << b.maxply  << '^' << b.think << '^';
	s << b.proto << '^' << ((int)b.mode);
	return(s);
}

ostream & operator<<(ostream &s, TerminalColor t) {
	s.setf(ios::hex,ios::basefield);
	s << t.TextDefault    << ',';
	s << t.TextBright     << ',';
	s << t.PrivateTell    << ',';
	s << t.NewsNotify     << ',';
	s << t.Mamer          << ',';
	s << t.KibitzWhisper  << ',';
	s << t.Shouts         << ',';
	s << t.Seeks          << ',';
	s << t.ChannelTell    << ',';
	s << t.Engine         << ',';
	s << t.Background;
	s.setf(ios::dec,ios::basefield);
	return(s);
}

Global::Global() {
	int i;

	input=0;
	output=0;
	network=0;
	status=0;
	protocol=0;
	chandler=0;
	promotion=0;
	ebook=0;
	skgraph2=0;
	inputhistory=0;
	bmlistener=0;
	qbcontainer=0;
	quickbar=0;
	killbox=0;
	lowernotebook=0;
	mainpaned=0;
	bugpane=0;
	pieceset=0;
	toplevelwidget=0;

	LastScratch = 0;

	Quitting=0;
	Version=VERSION;
	SelfInputColor = 0xc0c0ff;

	HilightLastMove=0;
	AnimateMoves=0;
	Premove=1;

	PasswordMode = 0;

	TabPos=0;

	CommLog=0;
	DebugLog=0;
	PauseLog=0;

	MainLevel=0;
	QuitPending=0;

	ScrollBack=1000;
	FicsAutoLogin=1;
	BeepWhenOppMoves=0;
	EnableSounds=0;
	PopupSecondaryGames=1;
	SmartDiscard=0;
	ShowCoordinates=0;

	PlainSquares=0;
	LightSqColor=0xe7cf93;
	DarkSqColor=0x9e8661;

	ShowTimestamp = 0;
	ShowRating=1;
	SpecialChars = 3;

	UseVectorPieces=0;
	CheckLegality=0;
	DrawHouseStock=1;

	AppendPlayed=0;
	AppendObserved=0;
	strcpy(AppendFile,"~/.eboard/mygames.pgn");

	IcsSeekGraph=1;
	HideSeeks=0;

	SplitChannels=0;
	ChannelsToConsoleToo=0;

	SmootherAnimation=0;

	// chess machine had this and FICS admins didn't like it
	// if you turn it on, you may be committing abuse.
	// this is DAV's fault for not implementing a decent
	// way of tracking game observers.
	// I'm not going to delete the code. I'm just removing the
	// user-friendly controls from the config dialog and setting
	// the defaults to safe values.

	IcsAllObPlayed   = 0;
	IcsAllObObserved = 0;

	JoystickFD = -1;

	JSCursorAxis = 0;
	JSBrowseAxis = 2;
	JSMoveButton = 0;
	JSNextTabButton = 5;
	JSPrevTabButton = 4;
	JSMode  = 1;
	JSSpeed = 4;

	joycapture = NULL;

	ShowQuickbar=1;
	LowTimeWarningLimit=5;
	RetrieveChannelNames=1;

	sndevents[1].Pitch=650;
	sndevents[1].Duration=350;

	sndevents[2].Pitch=900;
	sndevents[2].Duration=80;

	sndevents[3].Pitch=444;
	sndevents[3].Duration=40;
	sndevents[3].Count=3;

	sndevents[4].Pitch=740;
	sndevents[4].Duration=120;

	for(i=5;i<N_SOUND_EVENTS;i++) {
		sndevents[i].Pitch=610;
		sndevents[i].Duration=60;
		sndevents[i].enabled=false;
	}

	strcpy(ClockFont,DEFAULT_FONT_CLOK);
	strcpy(PlayerFont,DEFAULT_FONT_PLYR);
	strcpy(InfoFont,DEFAULT_FONT_INFO);
	strcpy(ConsoleFont,DEFAULT_FONT_CONS);
	strcpy(SeekFont,DEFAULT_FONT_SEEK);

	memset(P2PName,0,64);
	env.User.copy(P2PName,63);

	RCKeys.push_back("HilightLastMove"); //0
	RCKeys.push_back("AnimateMoves");
	RCKeys.push_back("Premove");         //2
	RCKeys.push_back("PieceSet");
	RCKeys.push_back("TabPos");          //4
	RCKeys.push_back("ClockFont");
	RCKeys.push_back("PlayerFont");      //6
	RCKeys.push_back("InfoFont");
	RCKeys.push_back("PlainSquares");    //8
	RCKeys.push_back("LightSqColor");
	RCKeys.push_back("DarkSqColor");     //10
	RCKeys.push_back("Host");
	RCKeys.push_back("Antialias");       //12 [deprecated]
	RCKeys.push_back("ShowRating");
	RCKeys.push_back("ScrollBack");      //14
	RCKeys.push_back("FicsAutoLogin");
	RCKeys.push_back("BeepOpp");         //16
	RCKeys.push_back("SoundEvent");
	RCKeys.push_back("EnableSounds");    //18
	RCKeys.push_back("VectorPieces");
	RCKeys.push_back("CheckLegality");   //20
	RCKeys.push_back("AppendPlayed");
	RCKeys.push_back("AppendObserved");  //22
	RCKeys.push_back("AppendFile");
	RCKeys.push_back("ConsoleFont");     //24
	RCKeys.push_back("SeekGraph");
	RCKeys.push_back("HideSeeks");       //26
	RCKeys.push_back("SplitChannels");
	RCKeys.push_back("ChSplitAndCons");  //28
	RCKeys.push_back("DrawHouseStock");
	RCKeys.push_back("SquareSet");       //30
	RCKeys.push_back("PopupSecondaryGames");
	RCKeys.push_back("SmartDiscard");    //32
	RCKeys.push_back("ShowCoordinates");
	RCKeys.push_back("TerminalColors");  //34
	RCKeys.push_back("DesktopState");
	RCKeys.push_back("DesktopStateDC");  //36
	RCKeys.push_back("ShowQuickbar");
	RCKeys.push_back("QuickbarButton");  //38
	RCKeys.push_back("LowTimeWarningLimit");
	RCKeys.push_back("RetrieveChannelNames"); //40
	RCKeys.push_back("SmootherAnimation");
	RCKeys.push_back("EngineBookmark");  // 42
	RCKeys.push_back("SeekFont");
	RCKeys.push_back("IcsAllObPlayed");  // 44
	RCKeys.push_back("IcsAllObObserved");
	RCKeys.push_back("P2PName");         // 46
	RCKeys.push_back("ShowTimestamp");
	RCKeys.push_back("SpecialChars");// 48
	RCKeys.push_back("JSCAxis");
	RCKeys.push_back("JSBAxis"); // 50
	RCKeys.push_back("JSMButton");
	RCKeys.push_back("JSNTButton"); // 52
	RCKeys.push_back("JSPTButton");
	RCKeys.push_back("JSMode"); // 54
	RCKeys.push_back("JSSpeed");

	PopupHelp = false;
}

void Global::dropQuickbarButtons() {
	unsigned int i;
	for(i=0;i<QuickbarButtons.size();i++)
		delete(QuickbarButtons[i]);
	QuickbarButtons.clear();
}

void Global::clearDupes(ChessGame *cg) {
	list<ChessGame *>::iterator gi;
	for(gi=GameList.begin();gi!=GameList.end();gi++)
		if ( (*(*gi)) == cg->GameNumber )
			renumberGame(*gi,nextFreeGameId(8000));
}

void Global::deleteGame(ChessGame *cg) {
	list<ChessGame *>::iterator gi;
	for(gi=GameList.begin();gi!=GameList.end();gi++)
		if ( (*gi) == cg ) {
			GameList.erase(gi);
			return;
		}
}

void Global::appendGame(ChessGame *cg,bool RenumberDupes) {
	if (RenumberDupes) clearDupes(cg);
	GameList.push_back(cg);
}
void Global::prependGame(ChessGame *cg, bool RenumberDupes) {
	if (RenumberDupes) clearDupes(cg);
	GameList.push_front(cg);
}

void Global::renumberGame(ChessGame *cg,int id) {
	int oldid;
	oldid=cg->GameNumber;
	cg->GameNumber=id;
	// renumber notebook references
	if (ebook) ebook->renumberPage(oldid,id);
}

void Global::removeBoard(Board *b) {
	for(BLi=BoardList.begin();BLi!=BoardList.end();BLi++)
		if ( (*BLi) == b ) {
			BoardList.erase(BLi);
			return;
		}
	//  cerr << "<Global::removeBoard> ** board not found\n";
}

bool Global::effectiveLegalityChecking() {
	if (CheckLegality) return true;

	if (protocol != NULL)
		return(protocol->requiresLegalityChecking());

	return false;
}

void Global::statOS() {
	FILE *p;
	p=popen("uname -s","r");
	if (!p) p=popen("/bin/uname -s","r");
	if (!p) p=popen("/sbin/uname -s","r");
	if (!p) p=popen("/usr/bin/uname -s","r");
	if (!p) p=popen("/usr/sbin/uname -s","r");
	if (!p) { strcpy(SystemType,"unknown"); return; }
	SystemType[63]=0;
	fgets(SystemType,64,p);
	pclose(p);
	if (SystemType[strlen(SystemType)-1]=='\n')
		SystemType[strlen(SystemType)-1]=0;
}

void Global::ensureDirectories() {
	char z[256];
	DIR *tdir;

	if (env.Home.empty()) {
		cerr << _("[eboard] ** no $HOME") << endl;
		return;
	} else if (strlen(env.Home.c_str()) > 230) {
		cerr << _("[eboard] ** $HOME is too long") << endl;
		return;
	}
	snprintf(z,256,"%s/.eboard",env.Home.c_str());

	tdir=opendir(z);
	if (tdir==NULL)
		PopupHelp = true;
	else
		closedir(tdir);

	if (createDir(z)) return;
	snprintf(z,256,"%s/.eboard/craftylog",env.Home.c_str());
	createDir(z);
	snprintf(z,256,"%s/.eboard/eng-out",env.Home.c_str());
	createDir(z);
	snprintf(z,256,"%s/.eboard/scripts",env.Home.c_str());
	createDir(z);
}

int Global::createDir(char *z) {
	DIR *tdir;
	tdir=opendir(z);
	if (tdir)
		closedir(tdir);
	else
		if (mkdir(z,0755)) {
			cerr << _("[eboard] ** failed to create directory ") << z << endl;
			return -1;
		}
	return 0;
}

void Global::readRC() {
	tstring t;
	string  *p;
	char    line[512],rev[128];

	static char *sep=" \n\t,:\r^";
	static char *sep2="\n\t,:\r";
	static char *sep3=" \n\t:\r";
	static char *sep4=",\n\r";
	static char *sep5="\n\t:\r";

	HostBookmark *hbm;
	EngineBookmark *ebm;
	int i,j;
	QButton *qb;

	if (env.Config.empty())
		return;

	ifstream rc(env.Config.c_str());

	if (!rc)
		return;

	memset(rev,0,128);
	rev['R']=0;
	rev['L']=1;
	rev['T']=2;
	rev['B']=3;

	t.setChomp(true);

	memset(line,0,512);
	while(rc.getline(line,511,'\n')) {
		t.set(line);
		memset(line,0,512);

		p=t.token(sep);
		if (!p) continue;
		if (p->at(0)=='#') continue;

		for(j=0;j<=55;j++) {
			if (! p->compare(RCKeys[j]) ) {
				switch(j) {
				case  0: HilightLastMove =t.tokenvalue(sep); break;
				case  1: AnimateMoves    =t.tokenvalue(sep); break;
				case  2: Premove         =t.tokenvalue(sep); break;
				case  3: setPieceSet(*(t.token(sep3)),true,true); break;
				case  4: p=t.token(sep); TabPos=rev[p->at(0)]; break;
				case  5: p=t.token(sep2);        memset(ClockFont,0,96);
					p->copy(ClockFont,95);  break;
				case  6: p=t.token(sep2);        memset(PlayerFont,0,96);
					p->copy(PlayerFont,95); break;
				case  7: p=t.token(sep2);        memset(InfoFont,0,96);
					p->copy(InfoFont,95);   break;
				case  8: PlainSquares    =t.tokenvalue(sep); break;
				case  9: LightSqColor    =t.tokenvalue(sep,16); break;
				case 10: DarkSqColor     =t.tokenvalue(sep,16); break;
				case 11: hbm=new HostBookmark();
					p=t.token(sep4); p->copy(hbm->host,128);
					hbm->port=t.tokenvalue(sep4);
					p=t.token(sep4); p->copy(hbm->protocol,64);
					HostHistory.push_back(hbm);
					break;
				case 12: break; // deprecated (antialias)
				case 13: ShowRating      =t.tokenvalue(sep); break;
				case 14: ScrollBack      =t.tokenvalue(sep); break;
				case 15: FicsAutoLogin   =t.tokenvalue(sep); break;
				case 16: BeepWhenOppMoves=t.tokenvalue(sep); break;
				case 17: i=t.tokenvalue(sep);
					if (i < N_SOUND_EVENTS) sndevents[i].read(t);
					break;
				case 18: EnableSounds    =t.tokenvalue(sep); break;
				case 19: UseVectorPieces =t.tokenvalue(sep); break;
				case 20: CheckLegality   =t.tokenvalue(sep); break;
				case 21: AppendPlayed    =t.tokenvalue(sep); break;
				case 22: AppendObserved  =t.tokenvalue(sep); break;
				case 23: p=t.token(sep);          memset(AppendFile,0,128);
					p->copy(AppendFile,127); break;
				case 24: p=t.token(sep2);         memset(ConsoleFont,0,96);
					p->copy(ConsoleFont,95); break;
				case 25: IcsSeekGraph        =t.tokenvalue(sep); break;
				case 26: HideSeeks           =t.tokenvalue(sep); break;
				case 27: SplitChannels       =t.tokenvalue(sep); break;
				case 28: ChannelsToConsoleToo=t.tokenvalue(sep); break;
				case 29: DrawHouseStock      =t.tokenvalue(sep); break;
				case 30: p=t.token(sep3);
					if (p->compare(pieceset->getSquareName()))
						setPieceSet(*p,false,true);
					break;
				case 31: PopupSecondaryGames =t.tokenvalue(sep); break;
				case 32: SmartDiscard        =t.tokenvalue(sep); break;
				case 33: ShowCoordinates     =t.tokenvalue(sep); break;
				case 34: Colors.read(t);      break;
				case 35: Desk.read(t);        break;
				case 36: Desk.readConsole(t); break;
				case 37: ShowQuickbar        =t.tokenvalue(sep); break;
				case 38: qb=new QButton(); qb->icon=t.tokenvalue(sep5);
					qb->caption=*(t.token(sep5)); qb->command=*(t.token(sep5));
					QuickbarButtons.push_back(qb);
					break;
				case 39: LowTimeWarningLimit =t.tokenvalue(sep); break;
				case 40: RetrieveChannelNames=t.tokenvalue(sep); break;
				case 41: SmootherAnimation   =t.tokenvalue(sep); break;
				case 42: ebm=new EngineBookmark(); ebm->read(t);
					EnginePresets.push_back(ebm); break;
				case 43: p=t.token(sep2); memset(SeekFont,0,96);
					p->copy(SeekFont,95); break;
					// default: cerr << "ignored [" << (*p) << "]\n";
				case 44: IcsAllObPlayed      =t.tokenvalue(sep); break;
				case 45: IcsAllObObserved    =t.tokenvalue(sep); break;
				case 46: p=t.token(sep2); memset(P2PName,0,64);
					p->copy(P2PName,63); break;
				case 47: ShowTimestamp       =t.tokenvalue(sep); break;
				case 48: SpecialChars        =t.tokenvalue(sep); break;
				case 49: JSCursorAxis = t.tokenvalue(sep); break;
				case 50: JSBrowseAxis = t.tokenvalue(sep); break;
				case 51: JSMoveButton = t.tokenvalue(sep); break;
				case 52: JSNextTabButton = t.tokenvalue(sep); break;
				case 53: JSPrevTabButton = t.tokenvalue(sep); break;
				case 54: JSMode  = t.tokenvalue(sep); break;
				case 55: JSSpeed = t.tokenvalue(sep); break;
				} // switch
			} // compare
		} // for j 0..55
	} // while getline

	rc.close();
}

void Global::writeRC() {

	string div;

	list<HostBookmark *>::iterator bi;
	list<EngineBookmark *>::iterator ei;
	static char *tabpos="RLTB";
	unsigned int i;

	if (env.Config.empty())
		return;

	div="::";

	ofstream rc(env.Config.c_str());
	if (!rc)
		return;

	rc << RCKeys[0] << div << HilightLastMove << endl;
	rc << RCKeys[1] << div << AnimateMoves << endl;
	rc << RCKeys[2] << div << Premove << endl;
	rc << RCKeys[3] << div << pieceset->getName() << endl;
	rc << RCKeys[4] << div << tabpos[TabPos%4] << endl;
	rc << RCKeys[5] << div << ClockFont << endl;
	rc << RCKeys[6] << div << PlayerFont << endl;
	rc << RCKeys[7] << div << InfoFont << endl;
	rc << RCKeys[8] << div << PlainSquares << endl;

	rc.setf(ios::hex, ios::basefield);
	rc << RCKeys[9]  << div << LightSqColor << endl;
	rc << RCKeys[10] << div << DarkSqColor << endl;
	rc.setf(ios::dec, ios::basefield);

	for(bi=HostHistory.begin();bi!=HostHistory.end();bi++)
		rc << RCKeys[11] << ',' << (*bi)->host << ',' <<
			(*bi)->port << ',' << (*bi)->protocol << endl;

	// 12: antialias deprecated
	rc << RCKeys[13] << div << ShowRating << endl;
	rc << RCKeys[14] << div << ScrollBack << endl;
	rc << RCKeys[15] << div << FicsAutoLogin<< endl;
	rc << RCKeys[16] << div << BeepWhenOppMoves << endl;

	for(i=0;i< N_SOUND_EVENTS;i++)
		rc << RCKeys[17] << div << i << ',' << sndevents[i] << endl;

	rc << RCKeys[18] << div << EnableSounds << endl;
	rc << RCKeys[19] << div << UseVectorPieces << endl;
	rc << RCKeys[20] << div << CheckLegality << endl;
	rc << RCKeys[21] << div << AppendPlayed << endl;
	rc << RCKeys[22] << div << AppendObserved << endl;
	rc << RCKeys[23] << div << AppendFile << endl;

	rc << RCKeys[24] << div << ConsoleFont << endl;
	rc << RCKeys[25] << div << IcsSeekGraph << endl;
	rc << RCKeys[26] << div << HideSeeks << endl;
	rc << RCKeys[27] << div << SplitChannels << endl;
	rc << RCKeys[28] << div << ChannelsToConsoleToo << endl;
	rc << RCKeys[29] << div << DrawHouseStock << endl;
	rc << RCKeys[30] << div << pieceset->getSquareName() << endl;
	rc << RCKeys[31] << div << PopupSecondaryGames << endl;
	rc << RCKeys[32] << div << SmartDiscard << endl;
	rc << RCKeys[33] << div << ShowCoordinates << endl;

	rc << RCKeys[34] << div << Colors << endl;
	rc << RCKeys[35] << div << Desk << endl;

	Desk.writeConsoles(rc,RCKeys[36]);

	rc << RCKeys[37] << div << ShowQuickbar << endl;

	for(i=0;i<QuickbarButtons.size();i++)
		rc << RCKeys[38] << div << (*QuickbarButtons[i]) << endl;

	rc << RCKeys[39] << div << LowTimeWarningLimit << endl;
	rc << RCKeys[40] << div << RetrieveChannelNames << endl;
	rc << RCKeys[41] << div << SmootherAnimation << endl;

	for(ei=EnginePresets.begin();ei!=EnginePresets.end();ei++)
		rc << RCKeys[42] << '^' << (*(*ei)) << endl;

	rc << RCKeys[43] << div << SeekFont << endl;

	rc << RCKeys[44] << div << IcsAllObPlayed << endl;
	rc << RCKeys[45] << div << IcsAllObObserved << endl;
	rc << RCKeys[46] << div << P2PName << endl;
	rc << RCKeys[47] << div << ShowTimestamp << endl;
	rc << RCKeys[48] << div << SpecialChars << endl;
	rc << RCKeys[49] << div << JSCursorAxis << endl;
	rc << RCKeys[50] << div << JSBrowseAxis << endl;
	rc << RCKeys[51] << div << JSMoveButton << endl;
	rc << RCKeys[52] << div << JSNextTabButton << endl;
	rc << RCKeys[53] << div << JSPrevTabButton << endl;
	rc << RCKeys[54] << div << JSMode << endl;
	rc << RCKeys[55] << div << JSSpeed << endl;

	rc.close();
}

ChessGame * Global::getGame(int num) {
	list<ChessGame *>::iterator gi;
	for(gi=GameList.begin();gi!=GameList.end();gi++)
		if ( (*(*gi)) == num )
			return(*gi);
	return NULL;
}

int Global::nextFreeGameId(int base) {
	int v;
	for(v=base;getGame(v)!=0;v++) ;
	return v;
}

void Global::WrappedMainIteration() {
	MainLevel++;
	gtk_main_iteration();
	MainLevel--;
	if ((!MainLevel)&&(QuitPending))
		Global::WrappedMainQuit();
}

void Global::WrappedMainQuit() {
	if (MainLevel) {
		QuitPending++;
		return;
	}
	QuitPending=0;
	signal(SIGCHLD,SIG_DFL); // prevent the crash reported by gcp
	/* close dgtnix driver and dll */
	if(DgtInit)
		{
			dgtnixClose();
			dlclose(dgtnix_dll_handle);
		}
	gtk_main_quit();
}

void Global::addAgent(NetConnection *ag) {
	Agents.push_back(ag);
	ag->notifyReadReady(iowatcher);
}

void Global::removeAgent(NetConnection *ag) {
	list<NetConnection *>::iterator ni;
	for(ni=Agents.begin();ni!=Agents.end();ni++)
		if ( (*ni) == ag ) {
			Agents.erase(ni);
			return;
		}
}

void Global::agentBroadcast(char *z) {
	list<NetConnection *>::iterator ni;
	if (Agents.empty())
		return;
	for(ni=Agents.begin();ni!=Agents.end();ni++)
		if ((*ni)->isConnected())
			(*ni)->writeLine(z);
}

int  Global::receiveAgentLine(char *dest,int limit) {
	list<NetConnection *>::iterator ni;
	global.debug("Global","receiveAgentLine");
	if (Agents.empty())
		return 0;
	for(ni=Agents.begin();ni!=Agents.end();ni++)
		if ( (*ni)->isConnected())
			if ((*ni)->readLine(dest,limit)==0)
				return 1;
	return 0;
}

void Global::opponentMoved() {
	if (BeepWhenOppMoves && sndevents[0].enabled) {
		if (AnimateMoves)
			SoundStack.push(0);
		else
			sndevents[0].safePlay();
	}
}

/*
  void Global::clearSoundStack() {
  while(!SoundStack.empty())
  SoundStack.pop();
  }
*/

void Global::flushSound() {
	if (!SoundStack.empty()) {
		sndevents[SoundStack.top()].safePlay();
		SoundStack.pop();
	}
}

void Global::drawOffered()    { playOther(1); }
void Global::privatelyTold()  { playOther(2); }
void Global::challenged()     { playOther(3); }
void Global::timeRunningOut() { playOther(4); }
void Global::gameWon()        { playOther(5); }
void Global::gameLost()       { playOther(6); }
void Global::gameStarted()    { playOther(7); }
void Global::gameFinished()   { playOther(8); }

void Global::moveMade() {
	if (EnableSounds && sndevents[9].enabled) {
		if (AnimateMoves)
			SoundStack.push(9);
		else
			sndevents[9].safePlay();
	}
}

void Global::playOther(int i) {
	if (i>=N_SOUND_EVENTS) return;
	if (EnableSounds && sndevents[i].enabled)
		sndevents[i].safePlay();
}

void Global::repaintAllBoards() {
	respawnPieceSet();
}

bool Global::hasSoundFile(string &p) {
	int i,j;
	j=SoundFiles.size();
	for(i=0;i<j;i++)
		if ( ! SoundFiles[i].compare(p) )
			return true;
	return false;
}

void Global::setPasswordMode(int pm) {
	list<DetachedConsole *>::iterator i;
	PasswordMode = pm;
	for(i=Consoles.begin();i!=Consoles.end();i++)
		(*i)->setPasswordMode(pm);
}

void Global::debug(char *klass,char *method,char *data) {
	char z[256];
	time_t now;
	string rm;

	if (!DebugLog)
		return;

	if (env.Home.empty())
		return;

	snprintf(z,256,"%s/DEBUG.eboard",env.Home.c_str());

	ofstream f(z,ios::app);
	if (!f) return;

	rm="+ ";
	rm+=klass;
	rm+="::";
	rm+=method;
	if (data) { rm+=" ["; rm+=data; rm+=']'; }

	now=time(0);
	strftime(z,255,"%Y-%b-%d %H:%M:%S",localtime(&now));

	f << z << " [" << ((int) getpid()) << "] " << rm << endl;
	f.close();
}

void Global::LogAppend(char *msg) {
	char z[256],*p;
	static char hexa[17]="0123456789abcdef";
	time_t now;
	string s;

	if (env.Home.empty())
		return;

	if (PauseLog)
		msg=_("(message obfuscated -- password mode ?)");

	if (CommLog) {
		snprintf(z,256,"%s/LOG.eboard",env.Home.c_str());

		ofstream f(z,ios::app);
		if (!f) return;

		for(p=msg;*p;p++)
			switch(*p) {
			case '\n': s+="\\n"; break;
			case '\r': s+="\\r"; break;
			default:
				if (*p < 32) {
					s+="(0x"; s+=hexa[(*p)>>4]; s+=hexa[(*p)&0xf]; s+=')';
				} else
					s+=*p;
			}

		now=time(0);
		strftime(z,255,"%Y-%b-%d %H:%M:%S",localtime(&now));

		f << z << "[ " << ((int) getpid()) << "] " << s << endl;
		f.close();
	}
}

void Global::dumpGames() {
	cerr.setf(ios::dec,ios::basefield);
	cerr << " GAME LIST (" << GameList.size() << " elements)\n";
	cerr << "--------------------------------------------------------------------------\n";
	for(GLi=GameList.begin();GLi!=GameList.end();GLi++)
		(*GLi)->dump();
	cerr << "--------------------------------------------------------------------------\n";
}

void Global::dumpBoards() {
	cerr.setf(ios::dec,ios::basefield);
	cerr << " BOARD LIST (" << BoardList.size() << " elements)\n";
	cerr << "--------------------------------------------------------------------------\n";
	for(BLi=BoardList.begin();BLi!=BoardList.end();BLi++)
		(*BLi)->dump();
	cerr << "--------------------------------------------------------------------------\n";
}

void Global::dumpPanes() {
	cerr.setf(ios::dec,ios::basefield);
	cerr << " PANE LIST\n";
	cerr << "--------------------------------------------------------------------------\n";
	ebook->dump();
	cerr << "--------------------------------------------------------------------------\n";
}

void Global::addHostBookmark(HostBookmark *hbm) {
	list<HostBookmark *>::iterator bi;

	for(bi=HostHistory.begin();bi!=HostHistory.end();bi++)
		if ( (*(*bi)) == hbm ) {
			delete hbm;
			return;
		}
	HostHistory.push_front(hbm);
	if (HostHistory.size() > 16) {
		delete(HostHistory.back());
		HostHistory.pop_back();
	}

	writeRC();
	if (bmlistener != 0) bmlistener->updateBookmarks();
}

void Global::addEngineBookmark(EngineBookmark *ebm) {
	list<EngineBookmark *>::iterator ei;

	for(ei=EnginePresets.begin();ei!=EnginePresets.end();ei++)
		if ( (*(*ei)) == ebm ) {
			delete ebm;
			return;
		}

	EnginePresets.push_front(ebm);
	if (EnginePresets.size() > 16) {
		delete(EnginePresets.back());
		EnginePresets.pop_back();
	}

	writeRC();
	if (bmlistener != 0) bmlistener->updateBookmarks();
}

void Global::updateScrollBacks() {
	output->updateScrollBack();
	updateChannelScrollBacks();
}

Notebook * Global::getNotebook() {
	return(ebook);
}

char * Global::filter(char *s) {
	int i,j;
	string t;
	gunichar uc;
	char *c;

	if (SpecialChars==0) return s; // no filtering

	j = strlen(s);
	for(i=0;i<j;i++)
		if (s[i] & 0x80 != 0)
			break;
	if (i==j) return(s); // ascii-clean, just return it

	for(c=s;*c!=0;c=g_utf8_next_char(c)) {
		uc = g_utf8_get_char(c);
		if (uc<128)
			t.append( 1, (char) uc );
		else {
			switch(SpecialChars) {
			case 1: break; // truncate
			case 2: t.append(1,'_'); break; // underscores
			case 3: unicodeNormalize(t,uc); break; // canonical decomposition
			}
		}
	}

	return(strdup(t.c_str()));
}

void Global::unicodeNormalize(string &dest, gunichar src) {
	gunichar *tmp;
	gsize i,len;
	tmp = g_unicode_canonical_decomposition(src, &len);
	for(i=0;i<len;i++) {
		if (tmp[i] > 128) {
			switch(tmp[i]) {
			case 0x300:             tmp[i] = '`';  break; // grave
			case 0xb4:  case 0x301: tmp[i] = '\''; break; // acute
			case 0x302:             tmp[i] = '^';  break; // circumflex
			case 0x303:             tmp[i] = '~';  break; // tilde
			case 0xb8:  case 0x327: tmp[i] = ',';  break; // cedil
			case 0x2d9: case 0x307: tmp[i] = '.';  break; // dot above
			case 0x308:             tmp[i] = '\"'; break; // diaeresis
			case 0x323:             tmp[i] = '.';  break; // dot below
			default:
				//cout << "not found: " << ((int) tmp[i]) << endl;
				tmp[i] = '_';
			}
		}
		dest.append( 1, (char) (tmp[i]&0x7f) );
	}

	g_free(tmp);
}

void Global::gatherConsoleState() {
	list<DetachedConsole *>::iterator i;

	// please make Desk.consoles empty before calling this. Thanks.

	for(i=Consoles.begin();i!=Consoles.end();i++)
		Desk.addConsole(*i);
}

// malloc has the stupid idea of segfaulting when
// allocating a word-incomplete size
void * Global::safeMalloc(int nbytes) {
	return(malloc(nbytes + (nbytes % 4)));
}

void Global::setPieceSet(string &filename,bool chgPieces,bool chgSquares) {
	string oldp,olds;
	PieceSet *oldset=0;

	if (pieceset) {
		oldp=pieceset->getName();
		olds=pieceset->getSquareName();
		oldset=pieceset;
	} else {
		chgPieces=true;
		chgSquares=true;
	}

	pieceset=new PieceSet(chgPieces?filename:oldp,chgSquares?filename:olds);
	if (oldset)
		delete oldset;

	respawnPieceSet();
}

void Global::respawnPieceSet() {
	list<PieceChangeListener *>::iterator i;

	// notify all objects that use the pieceset
	for(i=PieceClients.begin();i!=PieceClients.end();i++)
		(*i)->pieceSetChanged();
}

void Global::addPieceClient(PieceChangeListener *pcl) {
	global.debug("Global","addPieceClient");
	PieceClients.push_back(pcl);
}

void Global::removePieceClient(PieceChangeListener *pcl) {
	list<PieceChangeListener *>::iterator i;

	global.debug("Global","removePieceClient");

	for(i=PieceClients.begin();
		i!=PieceClients.end();
		i++)
		if ( (*i) == pcl ) {
			PieceClients.erase(i);
			return;
		}
}

// ----------

HostBookmark::HostBookmark() {
	memset(host,0,128);
	memset(protocol,0,64);
	port=0;
}

int HostBookmark::operator==(HostBookmark *hbm) {
	if (strcmp(host,hbm->host)) return 0;
	if (port!=hbm->port) return 0;
	if (strcmp(protocol,hbm->protocol)) return 0;
	return 1;
}

int EngineBookmark::operator==(EngineBookmark *ebm) {
	if (humanwhite != ebm->humanwhite) return 0;
	if (timecontrol != ebm->timecontrol) return 0;
	if (maxply != ebm->maxply) return 0;
	if (think != ebm->think) return 0;
	if (proto != ebm->proto) return 0;
	if (mode != ebm->mode) return 0;
	if (directory.compare(ebm->directory)) return 0;
	if (cmdline.compare(ebm->cmdline))     return 0;
	return 1;
}

void EngineBookmark::read(tstring &t) {
	static char *sep="^\n\r";
	string *p;

	caption     = *(t.token(sep));
	directory   = *(t.token(sep));
	cmdline     = *(t.token(sep));
	humanwhite  = t.tokenvalue(sep);

	p = t.token(sep);
	if (p)
		timecontrol.fromSerialization(p->c_str());

	maxply      = t.tokenvalue(sep);
	think       = t.tokenvalue(sep);
	proto       = t.tokenvalue(sep);
	mode        = (variant) t.tokenvalue(sep);

	if (!directory.compare("NULL")) directory.erase();
	if (!cmdline.compare("NULL")) cmdline.erase();
}

// -------------------------------- channel splitting

IcsChannel::IcsChannel(char *s) {
	static char *sep="\t\r\n";
	tstring t;
	t.set(s);
	number = t.tokenvalue(sep);
	name   = * (t.token(sep));
}

void ChannelSplitter::getChannels(char *ipaddr) {
	char destname[512], url[512];
	struct stat age;
	time_t now, d;
	pid_t kid;

	global.debug("ChannelSplitter","getChannels",ipaddr);
	channels.clear();

	if (! global.RetrieveChannelNames)
		return;

	snprintf(destname,512,"/tmp/eboard-chlist-%s-%d.tmp", ipaddr, getuid() );
	chlist=destname;

	if (stat(destname, &age)==0) {
		now=time(0);
		d = now - age.st_mtime;
		// list expires after 8 hours
		if (d < 28800)
			goto cs_gc_use_current;
	}

	snprintf(url,512,"http://eboard.sourceforge.net/ics/%s.txt",ipaddr);

		kid=fork();
		if (kid==0) {

			execlp("wget","wget","-q","-O",destname,url,0);
			_exit(0);

		} else {

			global.zombies.add(kid, this);

		}

 cs_gc_use_current:
		parseChannelList();

}

void ChannelSplitter::ZombieNotification(int pid) {
	parseChannelList();
}

void ChannelSplitter::parseChannelList() {
	char s[512];

	global.debug("ChannelSplitter","parseChannelList");
	channels.clear();

	ifstream f(chlist.c_str());
	if (!f) {
		global.debug("ChannelSplitter","parseChannelList","can't read file");
		return;
	}

	if (memset(s,0,512), f.getline(s,511,'\n')) {

		if (strstr(s,"text/ics-channel-list")) {
			while( memset(s,0,512), f.getline(s,511,'\n') ) {
				if (!isdigit(s[0])) break;
				channels.push_back( IcsChannel(s) );
			}
		}

	}

	f.close();
}

const char * ChannelSplitter::getChannelTitle(int n) {
	int i,j;
	static char z[128];
	j=channels.size();
	for(i=0;i<j;i++) {
		if (n==channels[i].number) {
			snprintf(z,128,"#%d: %s",n,channels[i].name.c_str());
			return z;
		}
	}
	snprintf(z,128,"#%d",n);
	return z;
}

void ChannelSplitter::ensurePane(int ch) {
	int i,j;
	j=panes.size();
	for(i=0;i<j;i++)
		if (numbers[i]==ch)
			return; // already exists
	createPane(ch);
}

void ChannelSplitter::createPane(int ch) {
	Notebook *nb;
	Text *op;
	char z[64];
	nb=getNotebook();
	if (!nb) return;
	op=new Text();

	snprintf(z,64,"%s",getChannelTitle(ch) );

	op->show();
	nb->addPage(op->widget,z,-200-ch,true);
	op->setNotebook(nb,-200-ch);
	numbers.push_back(ch);
	panes.push_back(op);
}

void ChannelSplitter::channelPageUp(int ch) {
	int i,j;
	j=panes.size();
	for(i=0;i<j;i++)
		if (numbers[i] == ch) {
			panes[i]->pageUp();
			return;
		}
}

void ChannelSplitter::channelPageDown(int ch) {
	int i,j;
	j=panes.size();
	for(i=0;i<j;i++)
		if (numbers[i] == ch) {
			panes[i]->pageDown();
			return;
		}
}

void ChannelSplitter::appendToChannel(int ch,char *msg,int color,Importance im) {
	int i,j;
	ensurePane(ch);
	j=panes.size();
	for(i=0;i<j;i++)
		if (numbers[i]==ch) {
			panes[i]->append(msg,color,im);
			panes[i]->contentUpdated();
			return;
		}
}

void ChannelSplitter::removeRemovablePage(int n) {
	int rn;
	int i,j;
	Notebook *nb;

	rn= -n;
	rn-=200;
	nb=getNotebook();

	j=panes.size();
	for(i=0;i<j;i++)
		if (numbers[i]==rn) {
			nb->removePage(n);
			delete panes[i];
			panes.erase(panes.begin() + i);
			numbers.erase(numbers.begin() + i);
		}
}

void ChannelSplitter::updateChannelScrollBacks() {
	int i,j;
	j=panes.size();
	for(i=0;i<j;i++)
		panes[i]->updateScrollBack();
}

void ChannelSplitter::updateFont() {
	int i,j;
	j=panes.size();
	for(i=0;i<j;i++)
		panes[i]->updateFont();
}

TerminalColor::TerminalColor() {
	TextDefault   = 0xeeeeee;
	TextBright    = 0xffffff;
	PrivateTell   = 0xffff00;
	NewsNotify    = 0xff8080;
	Mamer         = 0xffdd00;
	KibitzWhisper = 0xd38fd3;
	Shouts        = 0xddffdd;
	Seeks         = 0x80ff80;
	ChannelTell   = 0x3cd9d1;
	Engine        = 0xc0ff60;
	Background    = 0;
}

void TerminalColor::read(tstring &t) {
	static char *comma=",:\n\r \t";

	TextDefault   = t.tokenvalue(comma,16);
	TextBright    = t.tokenvalue(comma,16);
	PrivateTell   = t.tokenvalue(comma,16);
	NewsNotify    = t.tokenvalue(comma,16);
	Mamer         = t.tokenvalue(comma,16);
	KibitzWhisper = t.tokenvalue(comma,16);
	Shouts        = t.tokenvalue(comma,16);
	Seeks         = t.tokenvalue(comma,16);
	ChannelTell   = t.tokenvalue(comma,16);
	Engine        = t.tokenvalue(comma,16);
	Background    = t.tokenvalue(comma,16);
}

// ---- desktop saving

WindowGeometry::WindowGeometry(int a,int b,int c,int d) {
	X=a; Y=b; W=c; H=d;
}

WindowGeometry::WindowGeometry() {
	setNull();
}

void WindowGeometry::print() {
	cout << "X,Y,W,H = " << X << "," << Y << "," << W << "," << H << endl;
}

void WindowGeometry::retrieve(GtkWidget *w) {
	gint a[7];
	gdk_window_get_geometry(w->window,a,a+1,a+2,a+3,a+4);
	gdk_window_get_origin(w->window,a+5,a+6);
	X=a[5]-a[0];
	Y=a[6]-a[1];
	W=a[2];
	H=a[3];
}

bool WindowGeometry::isNull() {
	return( (X==0)&&(Y==0)&&(W==0)&&(H==0) );
}

void WindowGeometry::setNull() {
	X=Y=W=H=0;
}

void WindowGeometry::read(tstring &t) {
	static char *sep=":,()\n\t\r ";
	X=t.tokenvalue(sep);
	Y=t.tokenvalue(sep);
	W=t.tokenvalue(sep);
	H=t.tokenvalue(sep);
}

// --------

Desktop::Desktop() {
	clear();
}

void Desktop::clear() {
	vector<WindowGeometry *>::iterator i;
	vector<string *>::iterator j;

	wMain.setNull();
	wGames.setNull();
	wLocal.setNull();
	wAds.setNull();

	for(i=consoles.begin();i!=consoles.end();i++)
		delete(*i);

	for(j=cfilters.begin();j!=cfilters.end();j++)
		delete(*j);

	consoles.clear();
	cfilters.clear();
	PanePosition = 0;
}

void Desktop::read(tstring &t) {
	static char *sep=":,()\n\t\r ";
	wMain.read(t);
	wGames.read(t);
	wLocal.read(t);
	wAds.read(t);
	global.Desk.PanePosition = t.tokenvalue(sep);
}

void Desktop::writeConsoles(ostream &s, const char *key) {
	int i,j;
	j=consoles.size();
	for(i=0;i<j;i++) {
		s << key << "::" << (*consoles[i]);
		s << (*(cfilters[i])) << endl;
	}
}

void Desktop::readConsole(tstring &t) {
	WindowGeometry *wg;
	string *p,*s;
	static char *sep="\n\r";

	wg=new WindowGeometry();
	wg->read(t);

	p=t.token(sep);
	s=new string();
	if (p) (*s)=(*p);

	consoles.push_back(wg);
	cfilters.push_back(s);
}

void Desktop::addConsole(DetachedConsole *dc) {
	WindowGeometry *wg;
	wg=new WindowGeometry();
	wg->retrieve(dc->widget);
	consoles.push_back(wg);
	cfilters.push_back(new string(dc->getFilter()));
}

void Desktop::spawnConsoles(TextSet *ts) {
	int i,j;
	char tmp[512];
	DetachedConsole *dc;
	j=consoles.size();

	for(i=0;i<j;i++) {
		dc=new DetachedConsole(ts,0);
		dc->show();
		dc->restorePosition(consoles[i]);
		if (cfilters[i]->size()) {
			g_strlcpy(tmp,cfilters[i]->c_str(),512);
			dc->setFilter(tmp);
		}
	}
}

// ------- ah, the zombies

ZombieHunter::ZombieHunter() {
	signal(SIGCHLD,zh_sigchild_handler);
}

ZombieHunter::~ZombieHunter() {
	pids.clear();
	handlers.clear();
}

void ZombieHunter::add(int pid, SigChildHandler *sigh) {
	pids.push_back(pid);
	handlers.push_back(sigh);
}

void ZombieHunter::handleSigChild() {
	pid_t epid;
	unsigned int i;
	int s;

	while ( ( epid = waitpid(-1,&s,WNOHANG) ) > 0 ) {
		for(i=0;i<pids.size();i++)
			if (pids[i] == epid) {
				if (handlers[i] != 0) handlers[i]->ZombieNotification(epid);
				pids.erase(pids.begin() + i);
				handlers.erase(handlers.begin() + i);
				break;
			}
	}
}

void zh_sigchild_handler(int sig) {
	if (sig == SIGCHLD)
		global.zombies.handleSigChild();
}

Environment::Environment() {
	char *p;

	p=getenv("HOME");
	if (p) {
		Home=p;
	} else {
		Home.erase();
		cerr << _("** eboard ** warning: HOME environment variable not set\n");
	}

	p=getenv("USER");
	if (p) {
		User=p;
	} else {
		User=_("Human");
	}

	if (!Home.empty()) {
		Config=Home;
		Config+="/.eboard/eboard.conf";
	} else {
		Config.erase();
	}
}
