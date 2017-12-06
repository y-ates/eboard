/* $Id: main.cc,v 1.27 2007/02/01 21:29:39 pierrebou Exp $ */

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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/types.h>
#include <signal.h>
#include "config.h"
#include "mainwindow.h"
#include "global.h"
#include "sound.h"
#include "help.h"
#include "eboard.h"
#include "dgtboard.h"

#ifdef HAVE_LINUX_JOYSTICK_H
#include <linux/joystick.h>
#endif

void sigint_handler(int par);

#ifdef HAVE_LINUX_JOYSTICK_H
void openJoystick(MainWindow *mw);
void closeJoystick();
#endif

int main(int argc,char **argv) {
    list<int>::iterator zomb;
    int i;
    MainWindow *z;
    char *dgtport = NULL;

    if (strstr(argv[0],"beeper")) {
        SoundSlave slave;
        slave.waitForEvents();
        return 0;
    }

    g_strlcpy(global.argv0,argv[0],512);

    signal(SIGPIPE,SIG_IGN);
    //  signal(SIGUSR1,dump_tables); // for debugging
    signal(SIGINT,sigint_handler);

#ifdef ENABLE_NLS
    langs_prepare(0, "eboard", DATADIR "/eboard:/usr/share/eboard:/usr/local/share/eboard:.:multilang");
#endif

    gtk_set_locale ();

    gtk_init(&argc,&argv);
    gdk_rgb_init();

    for(i=1;i<argc;i++) {
        if (!strcmp(argv[i],"-log"))
            global.CommLog=1;
        if (!strcmp(argv[i],"-debug"))
            global.DebugLog=1;
        if (!strcmp(argv[i],"-dgtport")) {
            if (i<argc-1) dgtport = argv[++i];
        }
    }

    global.LogAppend("------------ started -------------");

    global.statOS();
    global.ensureDirectories();
    z=new MainWindow();
    z->show();
    z->restoreDesk();

    if (global.PopupHelp)
        (new Help::GettingStarted())->show();

    if (dgtport!=NULL) {
        dgtInit(dgtport, z);
        dgtSetBoard(global.BoardList.front());
    }

#ifdef HAVE_LINUX_JOYSTICK_H
    openJoystick(z);
#endif

    gtk_main();

#ifdef HAVE_LINUX_JOYSTICK_H
    closeJoystick();
#endif

    for (zomb=global.TheOffspring.begin(); zomb!=global.TheOffspring.end();
         zomb++)
        kill(*zomb, SIGKILL);

    global.LogAppend("finished gracefully");

    return 0;
}

void sigint_handler(int par) {
    static bool told_to_quit_already = false;
    global.LogAppend("SIGINT caught");

    if (told_to_quit_already) {
        exit(-2);
    } else {
        told_to_quit_already = true;
        gtk_main_quit();
    }
}

#ifdef HAVE_LINUX_JOYSTICK_H

void openJoystick(MainWindow *mw) {
    int i, fd;
    char devname[64];

    for (i=0; i<10; i++) {
        snprintf(devname,64,"/dev/js%d",i);
        fd = open(devname,O_RDONLY);
        if (fd >= 0) break;
    }

    if (fd < 0) return;

    global.JoystickFD = fd;
    gdk_input_add(fd, GDK_INPUT_READ,(GdkInputFunction) mainwindow_joystick,
                  (gpointer) mw);
}

void closeJoystick() {
    if (global.JoystickFD > 0) {
        gdk_input_remove(global.JoystickFD);
        close(global.JoystickFD);
        global.JoystickFD = -1;
    }
}

#endif
