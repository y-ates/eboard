/* $Id: clock.h,v 1.15 2008/02/08 14:25:50 bergo Exp $ */

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

#ifndef CLOCK_H
#define CLOCK_H 1

#include <sys/time.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <time.h>

#include "stl.h"
#include "eboard.h"
#include "util.h"
#include "widgetproxy.h"

class ClockHost {
 public:
    virtual void updateClock()=0;
};

// values for getActive()
#define CLK_WHITE_STOPPED -2
#define CLK_WHITE_RUNNING -1
#define CLK_STOPPED        0
#define CLK_BLACK_RUNNING  1
#define CLK_BLACK_STOPPED  2

class ChessClock : public UpdateInterface {
 public:
    ChessClock();
    virtual ~ChessClock();

    void setClock2(int whitemsec,int blackmsec,int activep,int countdown);
    void setHost(ClockHost *hostp);
    void setAnotherHost(ClockHost *hostp);
    void setMirror(ChessClock *dest);

    int  getActive();
    int  getValue2(int black);

    // which= -1=white 0=active one 1=black
    void incrementClock2(int which, int msecs);

    bool lowTimeWarning(piece mycolor);

    void update();

    int id;

 private:
    int value[2],val_ref[2];
    Timestamp t_ref[2];
    int active;
    int countdownf;
    ClockHost *host, *host2;
    ChessClock *mirror;
    Timestamp LastWarning;

    static int freeid;
};


class ClockMaster {
 public:
    ClockMaster();
    void append(ChessClock *clockp);
    void remove(ChessClock *clockp);

 private:
    void update();

    list<ChessClock *> clocks;
    int timeout_on;
    int timeout_id;

    friend gint clockmaster_timeout(gpointer data);
};

gint clockmaster_timeout(gpointer data);

#define TCF_GNUCHESS4QUIRK  0x01

class TimeControl {
 public:
    TimeControl();
    TimeControl &operator=(TimeControl &src);
    int          operator==(TimeControl &src);
    int          operator!=(TimeControl &src);

    void fromSerialization(const char *s);

    static void secondsToString(char *dest, int maxlen, int secs, bool units=false);
    static int  stringToSeconds(char *src);

    void setSecondsPerMove(int seconds);
    void setIcs(int base /* secs */, int increment /* secs */);
    void setXMoves(int nmoves, int nsecs);

    bool isRegressive();
    int  startValue();

    void toXBoard(char *dest, int maxlen, int flags = 0);
    void toString(char *dest, int maxlen);
    void toShortString(char *dest, int maxlen);
    void toPGN(char *dest, int maxlen); // format as a TimeControl tag

    TimeControlMode mode;
    int value[2];
};

class TimeControlEditDialog : public ModalDialog {
 public:
    TimeControlEditDialog(TimeControl *tc, bool allownone=true);
    virtual ~TimeControlEditDialog() {}
    void setUpdateListener(UpdateInterface *ui);

 private:
    GtkWidget *mi[4], *e[5], *f[3];
    TimeControl     *src;
    TimeControl     local;
    UpdateInterface *listener;

    friend void tced_ok(GtkWidget *w, gpointer data);
    friend void tced_dropmenu(GtkMenuItem *w, gpointer data);
};

void tced_ok(GtkWidget *w, gpointer data);
void tced_dropmenu(GtkMenuItem *w, gpointer data);

#endif
