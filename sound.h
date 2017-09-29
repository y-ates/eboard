/* $Id: sound.h,v 1.11 2007/01/01 18:29:03 bergo Exp $ */

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

#ifndef EBOARD_SOUND_H
#define EBOARD_SOUND_H

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include "tstring.h"
#include "widgetproxy.h"
#include "stl.h"

typedef enum {
  INT_WAVE,
  EXT_WAVE,
  EXT_PROGRAM,
  PLAIN_BEEP
} SoundEventType;

class SoundEventChangeListener {
 public:
  virtual void SoundEventChanged()=0;
};

class SoundEvent {
 public:
  SoundEvent();

  SoundEvent operator=(SoundEvent &se);
  int operator==(SoundEvent &se);
  int operator!=(SoundEvent &se);

  void read(tstring &rcline);

  void safePlay();
  void play();
  void playHere();
  void edit(SoundEventChangeListener *listener);

  char *getDescription();

  SoundEventType type;
  int  Duration;
  int  Pitch;
  int  Count;
  char Device[64];
  char ExtraData[256];
  bool enabled;

 private:
  void sine_beep(char *device,int pitch,int duration);
  char pvt[128];
};

ostream & operator<<(ostream &s,  SoundEvent e);

class SoundEventDialog : public ModalDialog {
 public:
  SoundEventDialog(SoundEvent *src, SoundEventChangeListener *listener);
 private:
  SoundEvent *obj;
  SoundEventChangeListener *hearer;
  GtkWidget *en[5], *rd[4], *fdlg, *tme;
  vector<GtkWidget *> sthemes;

  void apply(SoundEvent *dest);

  friend void snddlg_ok(GtkWidget *w,gpointer data);
  friend void snddlg_test(GtkWidget *w,gpointer data);
  friend void snddlg_browse(GtkWidget *w,gpointer data);
  friend void snddlg_picktheme(GtkMenuItem *w,gpointer data);
};

void snddlg_ok(GtkWidget *w,gpointer data);
void snddlg_test(GtkWidget *w,gpointer data);
void snddlg_browse(GtkWidget *w,gpointer data);
void snddlg_picktheme(GtkMenuItem *w,gpointer data);

class SoundSlave {
 public:
  SoundSlave();
  ~SoundSlave();
  void play(SoundEvent &se);
  void waitForEvents();

 private:
  int sout[2];
  int pid;

  bool alive();
  bool kicking();
  void kill();
  void run();
};

#endif

