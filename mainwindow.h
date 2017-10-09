/* $Id: mainwindow.h,v 1.74 2008/02/22 14:34:29 bergo Exp $ */

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



#ifndef EBOARD_MAINWINDOW_H
#define EBOARD_MAINWINDOW_H 1

#include "eboard.h"
#include "widgetproxy.h"
#include "notebook.h"
#include "board.h"
#include "text.h"
#include "util.h"
#include "status.h"
#include "protocol.h"
#include "history.h"
#include "dlg_connect.h"
#include "dlg_gamelist.h"
#include "dlg_prefs.h"
#include "help.h"
#include "promote.h"
#include "quickbar.h"
#include "script.h"
#include "stl.h"
#include "global.h"

class ThemeEntry {
 public:
	bool isDupe(ThemeEntry *te);
	bool isNameDupe(ThemeEntry *te);
	string Filename;
	string Text;
};

// implements the chat / command mode switch

class InputModeSelector : public WidgetProxy {
 public:
	InputModeSelector();
	virtual ~InputModeSelector() {}

	bool getChatMode();
	bool getSearchMode();
	void setChatMode(bool m);
	void setSearchMode(bool m);
	string &getPrefix();
	void   setPrefix(char *s);
	void   setPrefix(string &s);


	void flip();

 private:
	bool   ChatMode, SearchMode;
	string prefix;
	GtkWidget *l[2];

	// R,G,B in [0,255]
	void setColor(GtkWidget *w, int R,int G, int B);

	friend void ims_switch(GtkWidget *w, gpointer data);
};

void ims_switch(GtkWidget *w, gpointer data);

class PrefixCache {
 public:

	string * get(int id);
	void     set(int id,string &val);
	void     set(int id,char *val);
	void     setIfNotSet(int id, string &val);

 private:
	vector<int>      ids;
	vector<string *> text;
};

// ask about our multiple inheritance contest

class MainWindow : public WidgetProxy,
	public InputHandler,
	public ConnectionHandler,
	public GameListListener,
	public StockListListener,
	public AdListListener,
	public ConsoleListener,
	public PaneChangeListener,
	public BookmarkListener,
	public UpdateInterface,
	public IONotificationInterface
{
 public:
	MainWindow();
	virtual ~MainWindow() {}
	void setPasswordMode(int pm);
	void openServer(char *host,int port,Protocol *protocol,char *helper);
	void openServer(NetConnection *conn, Protocol *protocol);
	void openEngine(EngineProtocol *xpp, EngineBookmark *ebm=0);
	void openGameList();
	void openAdList();
	void openStockList();
	void openDetachedConsole();

	void gameListClosed();
	void stockListClosed();
	void adListClosed();
	void consoleClosed();

	void paneChanged(int pgseq,int pgid);
	void userInput(const char *text);
	void peekKeys(GtkWidget *who);
	int  keyPressed(int keyval, int state);
	void updatePrefix();

	void restoreDesk();

	static GdkWindow *RefWindow;

	void update();

	void readAvailable(int handle);
	void writeAvailable(int handle);

	void joystickEvent(JoystickEventType jet, int number, int value);

 private:
	int HideMode; // password
	GtkItemFactory  *gif;
	GtkWidget       *v;
	GtkWidget       *menubar;
	GtkWidget       *inputbox;
	Notebook        *notebook;
	TextSet         *icsout;
	Text            *inconsole, *xconsole;
	Status          *status;
	PromotionPicker *promote;
	QuickBar        *quickbar;

	GameListDialog  *gamelist;
	StockListDialog *stocklist;
	AdListDialog    *adlist;
	ScriptList      *scriptlist;
	DetachedConsole *consolecopy;

	InputModeSelector *ims;
	ExtPatternMatcher asetprefix, arunscript;
	PrefixCache       imscache;

	bool QuickbarVisible;

	GtkTooltips *tooltips;
	GtkWidget *navbar[8];
	bool nav_enable[8];

	GtkWidget *picseal;
	GdkPixmap *sealmap[2];
	GdkBitmap *sealmask[2];
	GtkWidget *vector_checkbox;

	History   *InputHistory;

	int jpd; // joystick previous direction

	void createNavbar(GtkWidget *box);
	void createSealPix(GtkWidget *box);

	void setSealPix(int flag);

	void setTitle(char *msg);
	void searchThemes();
	void updateBookmarks();
	void parseThemeFile(char *name);
	void greet();

	void injectInput();
	void historyUp();
	void historyDown();
	void tryConnect(char *host,int port,Protocol *protocol,char *helper);
	void cleanUpConnection();
	void disconnect();

	void openEngineBookmark(EngineBookmark *bm);
	void openGnuChess4();
	void openCrafty();
	void openSjeng();
	void openXBoardEngine();

	void gameWalk(int op);

	void saveDesk();
	void saveBuffer();

	void newScratchBoard(bool clearboard);
	void cloneOnScratch(ChessGame *cg0);

	void showQuickbar();
	void hideQuickbar();

	list<ThemeEntry *> Themes;

	gint io_tag;

	friend void mainwindow_themeitem(GtkMenuItem *menuitem, gpointer data);
	friend void mainwindow_themeitem2(GtkMenuItem *menuitem, gpointer data);
	friend void mainwindow_themeitem3(GtkMenuItem *menuitem, gpointer data);

	friend int  input_key_press (GtkWidget * wid, GdkEventKey * evt,
								 gpointer data);
	friend int  main_key_press (GtkWidget * wid, GdkEventKey * evt,
								gpointer data);

	friend void     peer_connect_fics(gpointer data);

	friend void     peer_connect_xboard(gpointer data);
	friend void     peer_connect_gnuchess4(gpointer data);
	friend void     peer_connect_sjeng(gpointer data);
	friend void     peer_connect_crafty(gpointer data);

	friend void     peer_scratch_empty(gpointer data);
	friend void     peer_scratch_initial(gpointer data);

	friend void     peer_connect_ask(gpointer data);
	friend void     peer_connect_p2p(gpointer data);
	friend void     peer_disconnect(gpointer data);
	friend void     help_about(gpointer data);
	friend void     help_keys(gpointer data);
	friend void     help_debug(gpointer data);
	friend void     help_starting(gpointer data);
	friend void     mainwindow_icsout_changed(GtkEditable *gtke, gpointer data);

	friend void     mainwindow_connect_bookmark(GtkWidget *w, gpointer data);
	friend void     mainwindow_connect_bookmark2(GtkWidget *w, gpointer data);
	friend void     mainwindow_edit_engbm(GtkWidget *w, gpointer data);

	friend void     navbar_back_all(GtkWidget *w,gpointer data);
	friend void     navbar_back_1(GtkWidget *w,gpointer data);
	friend void     navbar_forward_1(GtkWidget *w,gpointer data);
	friend void     navbar_forward_all(GtkWidget *w,gpointer data);
	friend void     navbar_movelist(GtkWidget *w,gpointer data);
	friend void     navbar_trash(GtkWidget *w,gpointer data);
	friend void     navbar_toscratch(GtkWidget *w,gpointer data);
	friend void     navbar_flip(GtkWidget *w,gpointer data);

	friend void     sett_hilite(GtkWidget *w,gpointer data);
	friend void     sett_animate(GtkWidget *w,gpointer data);
	friend void     sett_premove(GtkWidget *w,gpointer data);
	friend void     sett_coord(GtkWidget *w,gpointer data);
	friend void     sett_beepopp(GtkWidget *w,gpointer data);
	friend void     sett_osound(GtkWidget *w,gpointer data);
	friend void     sett_vector(GtkWidget *w,gpointer data);
	friend void     sett_legal(GtkWidget *w,gpointer data);
	friend void     sett_popup(GtkWidget *w,gpointer data);
	friend void     sett_smarttrash(GtkWidget *w,gpointer data);

	friend gboolean mainwindow_read_agents(gpointer data);

	friend void windows_savedesk(GtkWidget *w, gpointer data);
	friend void windows_savebuffer(GtkWidget *w, gpointer data);

	friend void windows_find(GtkWidget *w, gpointer data);
	friend void windows_findp(GtkWidget *w, gpointer data);


	friend gboolean gtkDgtnixEvent(GIOChannel* channel, GIOCondition cond, gpointer data);


#ifdef HAVE_LINUX_JOYSTICK_H
	friend void mainwindow_joystick(gpointer data,gint source,GdkInputCondition cond);
#endif
};

void sett_prefs(gpointer data);

gint mainwindow_delete  (GtkWidget * widget, GdkEvent * event, gpointer data);
void mainwindow_destroy (GtkWidget * widget, gpointer data);

void game_resign(GtkWidget *w,gpointer data);
void game_draw(GtkWidget *w,gpointer data);
void game_adjourn(GtkWidget *w,gpointer data);
void game_abort(GtkWidget *w,gpointer data);
void game_retract(GtkWidget *w,gpointer data);

void windows_games(GtkWidget *w, gpointer data);
void windows_sough(GtkWidget *w, gpointer data);
void windows_stock(GtkWidget *w, gpointer data);
void windows_detached(GtkWidget *w, gpointer data);
void windows_script(GtkWidget *w, gpointer data);

// keep focus on input box
gboolean mainwindow_input_focus_out(GtkWidget *widget,GdkEventFocus *event,gpointer user_data);
gboolean forced_focus(gpointer data);

gboolean do_smart_remove(gpointer data);


#define FA_HIGHLIGHT    100
#define FA_ANIMATE      101
#define FA_PREMOVE      102
#define FA_COORDS       103
#define FA_MOVEBEEP     104
#define FA_SOUND        105
#define FA_POPUP        106
#define FA_SMART        107
#define FA_LEGALITY     108
#define FA_VECTOR       109
#define FA_ICSBOOKMARKS 110
#define FA_ENGBOOKMARKS 111
#define FA_LOADTHEME    112
#define FA_LOADPIECES   113
#define FA_LOADSQUARES  114

// friends
void mainwindow_themeitem(GtkMenuItem *menuitem, gpointer data);
void mainwindow_themeitem2(GtkMenuItem *menuitem, gpointer data);
void mainwindow_themeitem3(GtkMenuItem *menuitem, gpointer data);

int  input_key_press (GtkWidget * wid, GdkEventKey * evt,
					  gpointer data);
int  main_key_press (GtkWidget * wid, GdkEventKey * evt,
					 gpointer data);

void     peer_connect_fics(gpointer data);

void     peer_connect_xboard(gpointer data);
void     peer_connect_gnuchess4(gpointer data);
void     peer_connect_sjeng(gpointer data);
void     peer_connect_crafty(gpointer data);

void     peer_scratch_empty(gpointer data);
void     peer_scratch_initial(gpointer data);

void     peer_connect_ask(gpointer data);
void     peer_connect_p2p(gpointer data);
void     peer_disconnect(gpointer data);
void     help_about(gpointer data);
void     help_keys(gpointer data);
void     help_debug(gpointer data);
void     help_starting(gpointer data);
void     mainwindow_icsout_changed(GtkEditable *gtke, gpointer data);

void     mainwindow_connect_bookmark(GtkWidget *w, gpointer data);
void     mainwindow_connect_bookmark2(GtkWidget *w, gpointer data);
void     mainwindow_edit_engbm(GtkWidget *w, gpointer data);

void     navbar_back_all(GtkWidget *w,gpointer data);
void     navbar_back_1(GtkWidget *w,gpointer data);
void     navbar_forward_1(GtkWidget *w,gpointer data);
void     navbar_forward_all(GtkWidget *w,gpointer data);
void     navbar_movelist(GtkWidget *w,gpointer data);
void     navbar_trash(GtkWidget *w,gpointer data);
void     navbar_toscratch(GtkWidget *w,gpointer data);
void     navbar_flip(GtkWidget *w,gpointer data);

void     sett_hilite(GtkWidget *w,gpointer data);
void     sett_animate(GtkWidget *w,gpointer data);
void     sett_premove(GtkWidget *w,gpointer data);
void     sett_coord(GtkWidget *w,gpointer data);
void     sett_beepopp(GtkWidget *w,gpointer data);
void     sett_osound(GtkWidget *w,gpointer data);
void     sett_vector(GtkWidget *w,gpointer data);
void     sett_legal(GtkWidget *w,gpointer data);
void     sett_popup(GtkWidget *w,gpointer data);
void     sett_smarttrash(GtkWidget *w,gpointer data);

gboolean mainwindow_read_agents(gpointer data);

void windows_savedesk(GtkWidget *w, gpointer data);
void windows_savebuffer(GtkWidget *w, gpointer data);

void windows_find(GtkWidget *w, gpointer data);
void windows_findp(GtkWidget *w, gpointer data);

#ifdef HAVE_LINUX_JOYSTICK_H
void mainwindow_joystick(gpointer data,gint source,GdkInputCondition cond);
#endif

#endif
