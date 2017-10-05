/* $Id: clock.cc,v 1.19 2008/02/08 14:25:50 bergo Exp $ */

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
#include <sys/time.h>
#include <unistd.h>
#include "clock.h"
#include "global.h"
#include "tstring.h"

ClockMaster Chronos;

int ChessClock::freeid=1;

ClockMaster::ClockMaster() {
    timeout_on=0;
    timeout_id=-1;
}

void ClockMaster::append(ChessClock *clockp) {
    clocks.push_back(clockp);
    if (!timeout_on) {
        timeout_id=gtk_timeout_add(90,clockmaster_timeout,this);
        timeout_on=1;
    }
}

void ClockMaster::remove(ChessClock *clockp) {
    list<ChessClock *>::iterator li;
    for(li=clocks.begin();li!=clocks.end();li++)
        if ( (*li) == clockp ) {
            clocks.erase(li);
            return;
        }
}

void ClockMaster::update() {
    list<ChessClock *>::iterator it;
    for(it=clocks.begin();it!=clocks.end();it++)
        (*it)->update();
}

gint clockmaster_timeout(gpointer data) {
    ClockMaster *cm;
    cm=(ClockMaster *)data;
    cm->update();
    return 1;
}

// ==================================================================

ChessClock::ChessClock() {
    active=CLK_STOPPED;
    host=0;
    host2=0;
    mirror=0;
    value[0]=value[1]=0;
    t_ref[0]=t_ref[1]=Timestamp::now();
    val_ref[0]=val_ref[1]=0;
    id=freeid++;
    countdownf=1;

    LastWarning = Timestamp::now();

    Chronos.append(this);
}

ChessClock::~ChessClock() {
    active=CLK_STOPPED;
    host=0;
    Chronos.remove(this);
}

void ChessClock::setMirror(ChessClock *dest) {
    mirror=dest;
}

void ChessClock::setClock2(int whitemsec,int blackmsec,int activep,int countdown)
{
    global.debug("ChessClock","setClock");
    countdownf=countdown;

    //  cerr << "setClock " << whitesec << ',' << blacksec << ',' << activep << endl;
    //  cerr << "oldvals = " << value[0] << ',' << value[1] << endl;

    if (whitemsec != CLOCK_UNCHANGED)
        value[0]=whitemsec;

    if (blackmsec != CLOCK_UNCHANGED)
        value[1]=blackmsec;

    t_ref[0]=t_ref[1]=Timestamp::now();
    val_ref[0]=value[0];
    val_ref[1]=value[1];

    //  cerr << "newvals = " << value[0] << ',' << value[1] << endl;

    active=activep;
    if (mirror)
        mirror->setClock2(whitemsec,blackmsec,activep,countdown);
}

void ChessClock::incrementClock2(int which, int msecs) {
    if (msecs==0)
        return;
    if (which==0)
        which = active;
    if (which == -1) {
        value[0]   += msecs;
        t_ref[0]   = Timestamp::now();
        val_ref[0] = value[0];
    }
    if (which == 1) {
        value[1]   += msecs;
        t_ref[1]   = Timestamp::now();
        val_ref[1] = value[1];
    }
    if (mirror)
        mirror->incrementClock2(which, msecs);
}

void ChessClock::setHost(ClockHost *hostp) {
    host=hostp;
}

void ChessClock::setAnotherHost(ClockHost *hostp) {
    host2=hostp;
}

int  ChessClock::getActive() {
    return active;
}

void ChessClock::update() {
    Timestamp now;
    bool u = false;
    if (host) {
        switch(active) {
        case CLK_WHITE_RUNNING:
            now=Timestamp::now();
            value[0]=val_ref[0]+(countdownf?-1:1)*(int)((now-t_ref[0])*1000.0);
            u = true;
            break;
        case CLK_BLACK_RUNNING:
            now=Timestamp::now();
            value[1]=val_ref[1]+(countdownf?-1:1)*(int)((now-t_ref[1])*1000.0);
            u = true;
            break;
        }
        if (u) {
            host->updateClock();
            if (host2) host2->updateClock();
        }
    }
}

int ChessClock::getValue2(int black) {
    return(value[black?1:0]);
}

bool ChessClock::lowTimeWarning(piece mycolor) {
    Timestamp now;

    // only countdown clocks can issue warnings
    if (!countdownf) return false;

    now = Timestamp::now();
    if (now - LastWarning < 1.0) return false;

    if ( (active==CLK_WHITE_RUNNING) &&
         (mycolor==WHITE) && (value[0] <= 1000*global.LowTimeWarningLimit) ) {
        global.timeRunningOut();
        LastWarning = Timestamp::now();
        return true;
    }

    if ( (active==CLK_BLACK_RUNNING) &&
         (mycolor==BLACK) && (value[1] <= 1000*global.LowTimeWarningLimit) ) {
        global.timeRunningOut();
        LastWarning = Timestamp::now();
        return true;
    }

    return false;
}

TimeControl::TimeControl() {
    mode = TC_NONE;
}

TimeControl & TimeControl::operator=(TimeControl &src) {
    mode = src.mode;
    value[0] = src.value[0];
    value[1] = src.value[1];
    return(*this);
}

int TimeControl::operator==(TimeControl &src) {
    if (mode != src.mode) return 0;
    switch(mode) {
    case TC_SPM:
        return(value[0] == src.value[0]);
    case TC_XMOVES:
    case TC_ICS:
        return(value[0] == src.value[0] && value[1] == src.value[1]);
    default:
        return 1;
    }
}

int TimeControl::operator!=(TimeControl &src) { return(!(src==(*this))); }

void TimeControl::setSecondsPerMove(int seconds) {
    mode = TC_SPM;
    value[0] = seconds;
}

// the serialization is "/a/b/c" (no quotes), a=mode, b,c=value
void TimeControl::fromSerialization(const char *s) {
    tstring t;
    int i;

    // cope with bookmarks from previous versions which just wrote
    // the secs per move value
    if (s[0]>='0' && s[0]<='9') {
        setSecondsPerMove(atoi(s));
        return;
    }

    t.set(s);
    t.setFail(-1);
    i = t.tokenvalue("/");
    if (i<0 || (i>2 && i!=99) ) {
        mode = TC_NONE;
        return;
    }
    mode = (TimeControlMode) i;
    value[0] = t.tokenvalue("/");
    value[1] = t.tokenvalue("/");
}

void TimeControl::setIcs(int base /* secs */, int increment /* secs */) {
    mode = TC_ICS;
    value[0] = base;
    value[1] = increment;
}

void TimeControl::setXMoves(int nmoves, int nsecs) {
    mode = TC_XMOVES;
    value[0] = nmoves;
    value[1] = nsecs;
}

// make sure dest is not shorter than char[32] to avoid
// trouble
void TimeControl::toXBoard(char *dest, int maxlen, int flags) {
    switch(mode) {
    case TC_NONE:
        dest[0] = 0;
        return;
    case TC_SPM:
        if (flags & TCF_GNUCHESS4QUIRK) {
            if (value[0]%60)
                snprintf(dest,maxlen,"level 1 %d:%02d",value[0]/60,value[0]%60);
            else
                snprintf(dest,maxlen,"level 1 %d",value[0]/60);
        } else {
            snprintf(dest,maxlen,"st %d",value[0]);
        }
        break;
    case TC_ICS:
        if (value[0]%60)
            snprintf(dest,maxlen,"level 0 %d:%02d %d",value[0]/60,value[0]%60,value[1]);
        else
            snprintf(dest,maxlen,"level 0 %d %d",value[0]/60,value[1]);
        break;
    case TC_XMOVES:
        if (value[1]%60)
            snprintf(dest,maxlen,"level %d %d:%02d 0",value[0],value[1]/60,value[1]%60);
        else
            snprintf(dest,maxlen,"level %d %d 0",value[0],value[1]/60);
        break;
    }
}

void TimeControl::toShortString(char *dest, int maxlen) {
    char z[64],y[64];
    switch(mode) {
    case TC_NONE:
        snprintf(dest,maxlen,_("untimed")); // TRANSLATE
        break;
    case TC_SPM:
        TimeControl::secondsToString(z,64,value[0],true);
        snprintf(dest,maxlen,_("%s/move"),z); // TRANSLATE
        break;
    case TC_ICS:
        snprintf(dest,maxlen,"%d %d",value[0]/60,value[1]);
        break;
    case TC_XMOVES:
        TimeControl::secondsToString(z,64,value[1],true);
        snprintf(dest,maxlen,_("%d moves in %s"),value[0],z); // OK
        break;
    }
}

void TimeControl::toString(char *dest, int maxlen) {
    char z[64],y[64];
    switch(mode) {
    case TC_NONE:
        snprintf(dest,maxlen,_("no time control set"));
        break;
    case TC_SPM:
        TimeControl::secondsToString(z,64,value[0],true);
        snprintf(dest,maxlen,_("%s per move"),z);
        break;
    case TC_ICS:
        TimeControl::secondsToString(z,64,value[0],true);
        TimeControl::secondsToString(y,64,value[1],true);
        snprintf(dest,maxlen,_("initial time %s, increment %s"),z,y);
        break;
    case TC_XMOVES:
        TimeControl::secondsToString(z,64,value[1],true);
        snprintf(dest,maxlen,_("%d moves in %s"),value[0],z);
        break;
    }
}

void TimeControl::toPGN(char *dest, int maxlen) {
    switch(mode) {
    case TC_NONE:
        g_strlcpy(dest,"?",maxlen);
        break;
    case TC_SPM:
        snprintf(dest,maxlen,"0+%d",value[0]);
        break;
    case TC_ICS:
        snprintf(dest,maxlen,"%d+%d",value[0],value[1]);
        break;
    case TC_XMOVES:
        snprintf(dest,maxlen,"%d/%d",value[0],value[1]);
        break;
    }
}

bool TimeControl::isRegressive() {
    return(mode == TC_ICS || mode == TC_XMOVES);
}

int TimeControl::startValue() {
    if (mode == TC_NONE || mode == TC_SPM)
        return 0;
    else
        return(mode==TC_XMOVES?value[1]:value[0]);
}

int TimeControl::stringToSeconds(char *src) {
    tstring t;
    int x, v=0;
    if (src[0] == '-')
        src++;
    t.set(src);
    t.setFail(-1);

    x=t.tokenvalue(":hms");
    if (x>=0) {
        v = x;
        x=t.tokenvalue(":hms");
        if (x>=0) {
            v = 60*v + x;
            x=t.tokenvalue(":hms");
            if (x>=0)
                v = 60*v + x;
        }
    }
    return v;
}

void TimeControl::secondsToString(char *dest, int maxlen, int secs, bool units) {
    if (secs < 0) {
        secs = -secs;
        dest[0] = '-';
        ++dest;
        --maxlen;
    }
    if (secs < 60)
        snprintf(dest,maxlen,"%d%c",secs, units?'s':0);
    else if (secs < 3600)
        snprintf(dest,maxlen,"%d%c%02d%c",
                 secs/60,units?'m':':',secs%60,units?'s':0);
    else
        snprintf(dest,maxlen,"%d%c%02d%c%02d%c",
                 secs/3600,     units?'h':':',
                 (secs%3600)/60,units?'m':':',
                 (secs%3600)%60,units?'s':0);
}

TimeControlEditDialog::TimeControlEditDialog(TimeControl *tc, bool allownone) :
    ModalDialog(N_("Edit Time Control"))
{
    GtkWidget *v, *om, *hs, *bhb, *ok, *cancel;
    GtkWidget *h[6], *l[6], *V[3];
    GtkMenu *ddm;
    int i;
    char z[64];

    listener = 0;
    src      = tc;
    local    = (*src);

    if (local.mode == TC_NONE)
        allownone = true;

    gtk_window_set_default_size(GTK_WINDOW(widget),300,150);

    v=gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(widget),v);

    om = gtk_option_menu_new();

    ddm = GTK_MENU(gtk_menu_new());
    mi[0] = gtk_menu_item_new_with_label(_("Type: Fixed Time per Move"));
    mi[1] = gtk_menu_item_new_with_label(_("Type: X Moves per Time Period"));
    mi[2] = gtk_menu_item_new_with_label(_("Type: Fischer Clock (ICS-like)"));
    mi[3] = gtk_menu_item_new_with_label(_("Type: Use engine's default setting"));
    for(i=0;i< (allownone ? 4 : 3);i++) {
        gtk_menu_shell_append(GTK_MENU_SHELL(ddm), mi[i]);
        gtk_signal_connect(GTK_OBJECT(mi[i]),"activate",
                           GTK_SIGNAL_FUNC(tced_dropmenu),
                           (gpointer) this);
        Gtk::show(mi[i],NULL);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(om),GTK_WIDGET(ddm));

    gtk_box_pack_start(GTK_BOX(v),om,FALSE,TRUE,0);

    for(i=0;i<3;i++) {
        f[i] = gtk_frame_new(0);
        V[i] = gtk_vbox_new(FALSE,2);
        h[i] = gtk_hbox_new(TRUE,2);
        h[3+i] = gtk_hbox_new(TRUE,2);
        gtk_container_add(GTK_CONTAINER(f[i]),V[i]);
        gtk_container_set_border_width(GTK_CONTAINER(V[i]),4);
        gtk_box_pack_start(GTK_BOX(V[i]),h[i],FALSE,TRUE,2);
        gtk_box_pack_start(GTK_BOX(V[i]),h[i+3],FALSE,TRUE,2);
        Gtk::show(h[i],h[i+3],V[i],NULL);
    }

    e[0] = gtk_entry_new();
    l[1] = gtk_label_new(_("per move"));
    gtk_box_pack_start(GTK_BOX(h[0]),e[0],TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(h[0]),l[1],FALSE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(v), f[0], FALSE, TRUE, 4);

    e[1] = gtk_entry_new();
    l[2] = gtk_label_new(_("moves in"));
    e[2] = gtk_entry_new();
    l[3] = gtk_label_new(_("(time period)"));
    gtk_box_pack_start(GTK_BOX(h[1]),e[1],TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(h[1]),l[2],FALSE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(h[1+3]),e[2],TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(h[1+3]),l[3],FALSE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(v), f[1], FALSE, TRUE, 2);

    l[4] = gtk_label_new(_("Starting Time:"));
    e[3] = gtk_entry_new();
    l[5] = gtk_label_new(_("Increment:"));
    e[4] = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(h[2]),l[4],FALSE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(h[2]),e[3],TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(h[2+3]),l[5],FALSE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(h[2+3]),e[4],TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(v), f[2], FALSE, TRUE, 2);

    l[0] = gtk_label_new(_("Times can be given as hh:mm:ss , mm:ss or ss"));
    gtk_box_pack_start(GTK_BOX(v),l[0],FALSE,TRUE,4);

    bhb=gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
    gtk_box_pack_end(GTK_BOX(v),bhb,FALSE,FALSE,4);
    ok=gtk_button_new_with_label    (_("OK"));
    GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
    cancel=gtk_button_new_with_label(_("Cancel"));
    GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bhb),ok,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(bhb),cancel,TRUE,TRUE,0);
    gtk_widget_grab_default(ok);

    hs=gtk_hseparator_new();
    gtk_box_pack_end(GTK_BOX(v),hs,FALSE,TRUE,4);

    gtk_signal_connect(GTK_OBJECT(ok), "clicked",
                       GTK_SIGNAL_FUNC(tced_ok), (gpointer) this);

    setDismiss(GTK_OBJECT(cancel),"clicked");

    Gtk::show(om,GTK_WIDGET(ddm),l[0],l[1],l[2],l[3],l[4],l[5],
              e[0],e[1],e[2],e[3],e[4],
              hs,ok,cancel,bhb,v,NULL);

    switch(local.mode) {
    case TC_SPM:
        gtk_option_menu_set_history(GTK_OPTION_MENU(om), 0);
        TimeControl::secondsToString(z,64,local.value[0]);
        gtk_entry_set_text(GTK_ENTRY(e[0]),z);
        gshow(f[0]);
        break;
    case TC_XMOVES:
        gtk_option_menu_set_history(GTK_OPTION_MENU(om), 1);
        snprintf(z,64,"%d",local.value[0]);
        gtk_entry_set_text(GTK_ENTRY(e[1]),z);
        TimeControl::secondsToString(z,64,local.value[1]);
        gtk_entry_set_text(GTK_ENTRY(e[2]),z);
        gshow(f[1]);
        break;
    case TC_ICS:
        gtk_option_menu_set_history(GTK_OPTION_MENU(om), 2);
        TimeControl::secondsToString(z,64,local.value[0]);
        gtk_entry_set_text(GTK_ENTRY(e[3]),z);
        TimeControl::secondsToString(z,64,local.value[1]);
        gtk_entry_set_text(GTK_ENTRY(e[4]),z);
        gshow(f[2]);
        break;
    case TC_NONE:
        gtk_option_menu_set_history(GTK_OPTION_MENU(om), 3);
        break;
    }
}

void TimeControlEditDialog::setUpdateListener(UpdateInterface *ui) {
    listener = ui;
}

void tced_dropmenu(GtkMenuItem *w, gpointer data) {
    TimeControlEditDialog *me = (TimeControlEditDialog *) data;
    GtkWidget *ww = GTK_WIDGET(w);

    switch(me->local.mode) {
    case TC_SPM:    gtk_widget_hide(me->f[0]); break;
    case TC_XMOVES: gtk_widget_hide(me->f[1]); break;
    case TC_ICS:    gtk_widget_hide(me->f[2]); break;
    default:
        break;
    }

    if (ww == me->mi[0]) {
        me->local.mode = TC_SPM;
        gshow(me->f[0]);
        gtk_widget_grab_focus(me->e[0]);
    } else if (ww == me->mi[1]) {
        me->local.mode = TC_XMOVES;
        gshow(me->f[1]);
        gtk_widget_grab_focus(me->e[1]);
    } else if (ww == me->mi[2]) {
        me->local.mode = TC_ICS;
        gshow(me->f[2]);
        gtk_widget_grab_focus(me->e[3]);
    } else if (ww == me->mi[3]) {
        me->local.mode = TC_NONE;
    }
}

void tced_ok(GtkWidget *w, gpointer data) {
    char z[128], y[128];

    TimeControlEditDialog *me = (TimeControlEditDialog *) data;

    switch(me->local.mode) {
    case TC_SPM:
        g_strlcpy(z, gtk_entry_get_text(GTK_ENTRY(me->e[0])), 128);
        me->local.setSecondsPerMove( TimeControl::stringToSeconds(z) );
        break;
    case TC_XMOVES:
        g_strlcpy(z, gtk_entry_get_text(GTK_ENTRY(me->e[1])), 128);
        g_strlcpy(y, gtk_entry_get_text(GTK_ENTRY(me->e[2])), 128);
        me->local.setXMoves( atoi(z), TimeControl::stringToSeconds(y) );
        break;
    case TC_ICS:
        g_strlcpy(z, gtk_entry_get_text(GTK_ENTRY(me->e[3])), 128);
        g_strlcpy(y, gtk_entry_get_text(GTK_ENTRY(me->e[4])), 128);
        me->local.setIcs(TimeControl::stringToSeconds(z),
                         TimeControl::stringToSeconds(y) );
        break;
    }

    *(me->src) = me->local;
    if (me->listener)
        me->listener->update();
    me->release();
}
