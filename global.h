/* $Id: global.h,v 1.87 2008/02/22 07:32:17 bergo Exp $ */

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

#ifndef GLOBAL_H
#define GLOBAL_H 1

#include <gtk/gtk.h>
#include "eboard.h"
#include "text.h"
#include "network.h"
#include "protocol.h"
#include "position.h"
#include "chess.h"
#include "sound.h"
#include "seekgraph.h"
#include "history.h"
#include "bugpane.h"
#include "clock.h"
#include "stl.h"

class Board;
class OutputPane;
class TextSet;
class NetConnection;
class Status;
class Protocol;
class ChessGame;
class Notebook;
class QuickBar;
class QButton;
class SeekGraph;
class SoundSlave;
class StringCollection;
class BugPane;
class PieceSet;
class VectorPieces;

class HostBookmark {
 public:
  HostBookmark();
  int operator==(HostBookmark *hbm);
  char host[128];
  int  port;
  char protocol[64];
};

class EngineBookmark {
 public:
  int operator==(EngineBookmark *ebm);
  void read(tstring &t);

  string      caption;
  string      directory;
  string      cmdline;
  int         humanwhite;
  TimeControl timecontrol;
  int         maxply;
  int         think;  
  int         proto;
  variant     mode;

};

class ZombieHunter {
 public:
  ZombieHunter();
  ~ZombieHunter();
  void add(int pid, SigChildHandler *sigh);

 private:
  void handleSigChild();
  friend void zh_sigchild_handler(int sig);

  vector<int> pids;
  vector<SigChildHandler *> handlers;
};

void zh_sigchild_handler(int sig);

class IcsChannel {
 public:
  IcsChannel(char *s); // s like "49\tmamer tourneys\n"
  string name;
  int    number;
};

class ChannelSplitter : public SigChildHandler
{
 public:
  virtual ~ChannelSplitter() {}
  void appendToChannel(int ch,char *msg,int color,Importance im=IM_NORMAL);
  virtual Notebook * getNotebook()=0;

  void removeRemovablePage(int n);
  void getChannels(char *ipaddr);
  void channelPageUp(int ch);
  void channelPageDown(int ch);

  void ZombieNotification(int pid);
  void updateFont();

 protected:
  void updateChannelScrollBacks();

 private:
  void ensurePane(int ch);
  void createPane(int ch);

  void parseChannelList();
  const char * getChannelTitle(int n);
  vector<Text *>  panes;
  vector<int>     numbers;
  string chlist;
  vector<IcsChannel> channels;
};

class TerminalColor {
 public:
  TerminalColor();
  void read(tstring &t);

  int TextDefault;
  int TextBright;
  int PrivateTell;
  int NewsNotify;
  int Mamer;
  int KibitzWhisper;
  int Shouts;
  int Seeks;
  int ChannelTell;
  int Engine;
  int Background;
};

class WindowGeometry {
 public:
  WindowGeometry(int a,int b,int c,int d);
  WindowGeometry();

  void read(tstring &t);
  void print();

  void retrieve(GtkWidget *w);
  bool isNull();
  void setNull();
  int X,Y,W,H;
};

class Desktop {
 public:

  Desktop();
  void clear();

  void read(tstring &t);
  void readConsole(tstring &t);
  void writeConsoles(ostream &s, const char *key);

  void addConsole(DetachedConsole *dc);
  void spawnConsoles(TextSet *ts);


  WindowGeometry wMain, wGames, wLocal, wAds;
  int PanePosition;

 private:
  vector<WindowGeometry *> consoles;
  vector<string *> cfilters;
};

class Environment {
 public:
  Environment();

  string Home; 
  string User;
  string Config;
};

class Global : public ChannelSplitter
{
 public:
  Global();
  virtual ~Global() {}
  
  ChessGame * getGame(int num);
  void removeBoard(Board *b);
  int  nextFreeGameId(int base);
  void repaintAllBoards();

  void appendGame(ChessGame *cg,bool RenumberDupes=true);
  void prependGame(ChessGame *cg,bool RenumberDupes=true);
  void deleteGame(ChessGame *cg);

  void statOS();
  void ensureDirectories();
  void readRC();
  void writeRC();

  void addHostBookmark(HostBookmark *hbm);
  void addEngineBookmark(EngineBookmark *ebm);

  void WrappedMainIteration();
  void WrappedMainQuit();
  void LogAppend(char *msg);
  void debug(char *klass,char *method,char *data=0);

  void dumpGames();
  void dumpBoards();
  void dumpPanes();

  void gatherConsoleState();

  void addAgent(NetConnection *ag);
  void removeAgent(NetConnection *ag);
  void agentBroadcast(char *z);
  int  receiveAgentLine(char *dest,int limit);

  bool effectiveLegalityChecking();

  /* issue sound events */
  void opponentMoved();
  void drawOffered();
  void privatelyTold();
  void challenged();
  void timeRunningOut();
  void gameWon();
  void gameLost();
  void gameStarted();
  void gameFinished();
  void moveMade();

  void flushSound();
  void setPasswordMode(int pm);
  //  void clearSoundStack();

  void updateScrollBacks();
  void dropQuickbarButtons();

  bool hasSoundFile(string &p);

  Notebook * getNotebook();

  void setPieceSet(string &filename,bool chgPieces,bool chgSquares);
  void respawnPieceSet();
  void addPieceClient(PieceChangeListener *pcl);
  void removePieceClient(PieceChangeListener *pcl);

  char * filter(char *s);

  // free with free() as usual
  static void * safeMalloc(int nbytes);

  list<ChessGame *> GameList;
  list<ChessGame *>::iterator GLi;
  list<Board *> BoardList;
  list<Board *>::iterator BLi;
  list<int> TheOffspring;

  list<NetConnection *> Agents;
  list<DetachedConsole *> Consoles;
  list<PieceChangeListener *> PieceClients;

  VectorPieces       vpieces;
  PieceSet           *pieceset;

  InputHandler       *input;
  OutputPane         *output;
  NetConnection      *network;
  Status             *status;
  Protocol           *protocol;
  ConnectionHandler  *chandler;
  PieceProvider      *promotion;
  Notebook           *ebook;
  SeekGraph2         *skgraph2;
  History            *inputhistory;
  BookmarkListener   *bmlistener;
  UpdateInterface    *qbcontainer;
  QuickBar           *quickbar;
  GtkWidget          *killbox;
  GtkWidget          *lowernotebook;
  GtkWidget          *mainpaned;
  GtkWidget          *toplevelwidget;
  BugPane            *bugpane;
  IONotificationInterface *iowatcher;
  JoystickListener   *joycapture;

  StringCollection   annotator;
  SoundSlave         sndslave;
  ZombieHunter       zombies;

  int                SelfInputColor;
  int                PasswordMode;

  int                LastScratch;

  int HilightLastMove;
  int AnimateMoves;
  int Premove;

  int TabPos; // 0=R 1=L 2=T 3=B

  char ClockFont[96];
  char PlayerFont[96];
  char InfoFont[96];
  char ConsoleFont[96];
  char SeekFont[96];

  int PlainSquares;
  int LightSqColor;
  int DarkSqColor;

  int ShowTimestamp;
  int ShowRating;
  int ScrollBack;
  int FicsAutoLogin;
  int IcsSeekGraph;
  int HideSeeks;
  int BeepWhenOppMoves;
  int EnableSounds;
  int UseVectorPieces;
  int CheckLegality;
  int SpecialChars; // 0=none 1=truncate 2=underscore 3=soft translate (a^)

  int SplitChannels;
  int ChannelsToConsoleToo;
  int DrawHouseStock;

  int AppendPlayed;
  int AppendObserved;
  char AppendFile[128];

  int PopupSecondaryGames;
  int SmartDiscard;

  int ShowCoordinates;
  int ShowQuickbar;
  int RetrieveChannelNames;
  bool PopupHelp;
  int LowTimeWarningLimit;
  int SmootherAnimation;
  int IcsAllObPlayed;
  int IcsAllObObserved;

  int JSCursorAxis;
  int JSBrowseAxis;
  int JSMoveButton;
  int JSNextTabButton;
  int JSPrevTabButton;
  int JSMode;
  int JSSpeed;

  char P2PName[64];

  TerminalColor Colors;
  Desktop Desk;
  
  // 0=opponent moved, 1=draw offer 2=pvt tell 3=challenged
  // 4=time running out
  // 5=won game        6=lost game  7=game started
  // 8=game over (observed)
  // 9=moved (observed)
  SoundEvent sndevents[N_SOUND_EVENTS];

  int CommLog;
  int DebugLog;
  int PauseLog;

  int Quitting;

  list<HostBookmark *>   HostHistory;
  list<EngineBookmark *> EnginePresets;

  vector<string> SoundFiles;
  vector<QButton *> QuickbarButtons;

  char * Version;
  char   SystemType[64];
  char   argv0[512];

  Environment env;

  string ConsoleReply;

  int JoystickFD;

 private:
  int createDir(char *z);
  void renumberGame(ChessGame *cg,int id);
  void clearDupes(ChessGame *cg);

  void playOther(int i);

  void unicodeNormalize(string &dest, gunichar src);

  int MainLevel;
  int QuitPending;

  vector<const char *> RCKeys;
  stack<int> SoundStack;
};

#ifndef GLOBAL_CC
extern Global global;
#endif

#endif
