/* $Id: help.cc,v 1.33 2008/02/18 13:21:05 bergo Exp $ */

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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "util.h"
#include "tstring.h"
#include "global.h"
#include "help.h"
#include "mainwindow.h"
#include "eboard.h"
#include "widgetproxy.h"

#include "gnupiece.xpm"

namespace Help {

  AboutDialog::AboutDialog() : ModalDialog(N_("About eboard")) {
    GtkWidget *v,*hb,*label,*bhb,*hs,*ok,*gnup;
    GtkStyle *style;
    GdkPixmap *gnud;
    GdkBitmap *gnub;
    char z[1024];

    v=gtk_vbox_new(FALSE,4);
    gtk_container_add(GTK_CONTAINER(widget),v);

    hb=gtk_hbox_new(FALSE,4);
    gtk_box_pack_start(GTK_BOX(v),hb,TRUE,TRUE,0);
    
    style=gtk_widget_get_style(widget);
    gnud=gdk_pixmap_create_from_xpm_d(widget->window,&gnub,
				      &style->bg[GTK_STATE_NORMAL],
				      (gchar **)gnupiece_xpm);
    gnup=gtk_pixmap_new(gnud,gnub);
    
    gtk_box_pack_start(GTK_BOX(hb),gnup,FALSE,TRUE,4);
    
    snprintf(z,1024,
	    _("eboard version %s\n"\
	      "(c) 2000-%d Felipe Bergo\n"\
	      "<fbergo@gmail.com>\n"\
	      "http://eboard.sourceforge.net\n\n"\
	      "This program is free software; you can redistribute\n"\
	      "it and/or modify it under the terms of the GNU General\n"\
	      "Public License as published by the Free Software\n"\
	      "Foundation; either version 2 of the License, or\n"\
	      "(at your option) any later version.\n"),
	    global.Version, 2008);
    
    label=gtk_label_new(z);
    gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hb),label,TRUE,TRUE,4);
    
    hs=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,4);
    
    bhb=gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
    
    gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,FALSE,4);
    
    ok=gtk_button_new_with_label(_("Close"));
    GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bhb),ok,TRUE,TRUE,0);
    
    gtk_widget_grab_default(ok);   
    Gtk::show(ok,bhb,hs,label,gnup,hb,v,NULL);
    setDismiss(GTK_OBJECT(ok),"clicked");
  }

  KeysDialog::KeysDialog() : ModalDialog(N_("Help: Keys")) {
    GtkWidget *v,*label,*lbs,*bhb,*hs,*ok;
    char z[1024];
    
    v=gtk_vbox_new(FALSE,4);
    gtk_container_add(GTK_CONTAINER(widget),v);
    
    strcpy(z,
	   _("Anywhere:\n"\
	   "F3: Go to previous pane.\n"\
	   "F4: Go to next pane.\n"\
	   "F5: Go to the main board pane.\n"\
	   "F6: Go to the console pane.\n"\
	   "F7: Go to the seek graph pane (if available).\n"\
	   "F8: Toggle Shortcut Bar visilibity.\n"\
	   "Page Up/Page Down: scrolls the text console (must be visible)\n"\
	   "Ctrl+(Left Arrow): Backward 1 halfmove\n"\
	   "Ctrl+(Right Arrow): Forward 1 halfmove\n"\
	   "Ctrl+F: Find Upwards(main console buffer)\n"\
	   "Ctrl+G: Find Previous\n"\
	   "\nInput box:\n"\
	   "Up/Down (arrows): move thru input history\n"\
	   "Enter: send text line\n"\
	   "Esc: switch Chat/Command Mode\n"\
	   "\nSyntaxisms:\n"\
	   "In main window's input box:\n"\
	   ".. entering %prefix text will set the chat prefix to text.\n"\
	   ".. entering %do scriptname will run scriptname."));

    label=gtk_label_new(z);
    gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
    lbs = scrollBox(label);
    gtk_box_pack_start(GTK_BOX(v),lbs,TRUE,TRUE,4);
    
    hs=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,4);
    
    bhb=gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
    
    gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,FALSE,4);

    ok=gtk_button_new_with_label(_("Close"));
    GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bhb),ok,TRUE,TRUE,0);
    
    gtk_widget_grab_default(ok);
    gtk_window_set_default_size(GTK_WINDOW(widget),450, 450);
    Gtk::show(ok,bhb,hs,label,v,NULL);
    setDismiss(GTK_OBJECT(ok),"clicked");
  }

  DebugDialog::DebugDialog() : ModalDialog(N_("Help: Debug Info")) {
    GtkWidget *v,*label,*hs,*bhb,*ok,*txt;
    char z[2048],y[256],x[256],w[256];
    struct stat s;
    EboardFileFinder eff;

    v=gtk_vbox_new(FALSE,4);
    gtk_container_add(GTK_CONTAINER(widget),v);
    
    z[0]=0;

    // gtk
    snprintf(y,256,"GTK+ version %d.%d.%d\n",
	     gtk_major_version,
	     gtk_minor_version,
	     gtk_micro_version);
    g_strlcat(z,y,2048);

    // gcc (c++) if any
    snprintf(y,256,"GCC says:  %s\n",
	     grabOutput("c++ --version 2>&1"));
    g_strlcat(z,y,2048);

    // perl
    snprintf(y,256,"Perl says:  %s\n",
	     grabOutput("perl -v | grep ^This"));
    g_strlcat(z,y,2048);

    // expect
    snprintf(y,256,"expect says:  %s\n",
	    grabOutput("expect -d -c exit 2>&1"));
    g_strlcat(z,y,2048);

    // kernel
    snprintf(y,256,"kernel info: %s\n",
	     grabOutput("uname -srm"));
    g_strlcat(z,y,2048);

    // timeseal
    snprintf(x,256,"timeseal.%s",global.SystemType);
    if (!eff.find(x,y)) strcpy(y,x);

    if (lstat(y,&s)==0) {
      snprintf(w,256,"Found %s\n",y); strcpy(x,w);
      snprintf(w,256,"Size %lu bytes, Last Modified %s",
	       s.st_size, ctime(&(s.st_mtime)) );
      g_strlcat(x,w,256);
      snprintf(w,256,"Mode %.4o. (seems it i%s enough to be run by eboard)\n",
	       s.st_mode,
	       ( S_ISREG(s.st_mode)
		 &&
		 (
		  ( S_IXUSR&s.st_mode && s.st_uid==getuid() )
		  ||
		  ( S_IXGRP&s.st_mode && s.st_gid==getgid() )
		  ||
		  ( S_IXOTH&s.st_mode ) 
		  )
		 ) ? "s" : "s NOT"
	       );
      g_strlcat(x,w,256);
      
    } else {
      snprintf(x,256,"%s Not Found (errno %d)",y,errno);
    }

    snprintf(y,256,"\nFICS timeseal:\n%s\n",x);
    g_strlcat(z,y,2048);

    label=gtk_label_new(z);
    gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(v),label,TRUE,TRUE,4);

    hs=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,4);
    
    bhb=gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
    
    gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,FALSE,4);

    ok=gtk_button_new_with_label(_("Close"));
    txt=gtk_button_new_with_label(_("Write to Console"));
    GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
    GTK_WIDGET_SET_FLAGS(txt,GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bhb),txt,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(bhb),ok,TRUE,TRUE,0);
    
    gtk_widget_grab_default(ok);
    
    Gtk::show(ok,txt,bhb,hs,label,v,NULL);
    setDismiss(GTK_OBJECT(ok),"clicked");

    gtk_signal_connect(GTK_OBJECT(txt),"clicked",
		       GTK_SIGNAL_FUNC(debugdlg_writecons),
		       (gpointer) this);

    strcpy(pipedata,z);
  }

  char * DebugDialog::grabOutput(char *cmdline) {
    FILE *f;
    bool ok = false;
    char *c;
    f=popen(cmdline,"r");
    if (f) {
      if (fgets(pipedata,127,f))
	ok=true;
      pclose(f);
    }
    if (!ok)
      strcpy(pipedata,"unable to retrieve");
    else
      while( (c=strchr(pipedata,'\n'))!=NULL)
	*c=0;
    return pipedata;
  }

  void debugdlg_writecons(GtkWidget *w, gpointer data) {
    DebugDialog *me;
    tstring t;
    string *c;
    char z[512];

    me = (DebugDialog *) data;
    t.set(me->pipedata);
    global.output->append("=== START DEBUG INFO ===",0xc0ff00);
    while( (c=t.token("\n")) != 0) {
      memset(z,0,512);
      c->copy(z,511);
      global.output->append(z, global.Colors.TextBright);
    }
    global.output->append("=== END DEBUG INFO ===",0xc0ff00);
  }


  GettingStarted::GettingStarted() : NonModalDialog(N_("Help: Getting Started ")) {
    GtkWidget *v, *ts, *bhb, *ok, *fr;

    gtk_window_set_default_size(GTK_WINDOW(widget),620,560);
    v=gtk_vbox_new(FALSE,4);
    gtk_container_add(GTK_CONTAINER(widget),v);

    fr = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(fr),GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(v),fr,TRUE,TRUE,0);

    
    text=gtk_text_view_new();
    g_object_set(G_OBJECT(text),"cursor-visible",FALSE,NULL);
    ts = scrollBox(text);
    gtk_container_add(GTK_CONTAINER(fr),ts);

    bhb=gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
    
    gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,FALSE,4);

    ok=gtk_button_new_with_label(_("Close"));
    GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bhb),ok,TRUE,TRUE,0);
    
    gtk_widget_grab_default(ok);
    
    Gtk::show(ok,bhb,text,fr,v,NULL);
    setDismiss(GTK_OBJECT(ok),"clicked");

    compose();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text),FALSE);
  }

  void GettingStarted::compose() {
    char *T,*p,*q;
    GtkTextBuffer *tb;
    GtkTextIter iter;
    GtkTextTag *tag[6];

    // codes: #L# large #M# medium  #S# small  #B# blue  #K# black

    T=_("#L#Getting Started\n"\
      "#M#Common Tasks in eboard\n\n"\
      "Playing against the computer\n"\
      "#S#eboard does not \"play chess\" itself, but rather is works as interface to programs that do,\n"\
      "called \"engines\", which don't have a graphical interface themselves. You need an engine to\n"\
      "play against the computer. GNU Chess, Crafty and Sjeng are chess engines that are available at\n"\
      "no cost.\n"\
      "Once you have one of them installed, open the #B#Peer#K# menu, then the #B#Play against\n"\
      "engine#K# submenu, and select the appropriate option depending on which engine you have\n"\
      "installed.\n\n"\
      "#M#Playing Chess on the Internet\n"\
      "#S#Eboard supports the FICS protocol. FICS runs at freechess.org, but other servers, such as\n"\
      "US Chess Live, use FICS's software and should work with eboard too. ICC is not supported.\n"\
      "To connect to FICS, open the #B#Peer#K# menu, click #B#Connect to FICS#K#. To connect to\n"\
      "other servers, open the #B#Peer#K# menu, click #B#Connect to Other Server...#K#.\n"\
      "While you can login as guest on FICS, you'll enjoy it better as a registered user. Registration\n"\
      "is done through FICS's site at #B#http://www.freechess.org#K#, and it's free.\n\n"\
      "#M#Browsing PGN Games\n"\
      "#S#PGN is the most common file format to store chess games. It can store moves and comments\n"\
      "(annotations). To open this kind of file within eboard, open the #B#Windows#K# menu, click\n"\
      "#B#Games on Client#K#. In the Local Game List dialog, click #B#Load PGN...#K#. To browse a\n"\
      "game, #B#select it#K# and click #B#Display#K#. A new tab will be created in the main window\n"\
      "with the game."\
      "#L#");

    tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    tag[0] = gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(tb),"tag0",
					"font","Sans 10",
					"foreground","#000000",
					"justification",GTK_JUSTIFY_LEFT,
					"editable",FALSE,
					"weight",PANGO_WEIGHT_NORMAL,
					"wrap-mode",GTK_WRAP_WORD,
					NULL);
    tag[1] = gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(tb),"tag1",
					"font","Sans 12",
					"foreground","#000000",
					"justification",GTK_JUSTIFY_LEFT,
					"editable",FALSE,
					"weight",PANGO_WEIGHT_BOLD,
					"wrap-mode",GTK_WRAP_WORD,
					NULL);
    tag[2] = gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(tb),"tag2",
					"font","Sans 16",
					"foreground","#000000",
					"justification",GTK_JUSTIFY_LEFT,
					"editable",FALSE,
					"weight",PANGO_WEIGHT_BOLD,
					"wrap-mode",GTK_WRAP_WORD,
					NULL);
    tag[3] = gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(tb),"tag3",
					"font","Sans 10",
					"foreground","#0000ff",
					"justification",GTK_JUSTIFY_LEFT,
					"editable",FALSE,
					"weight",PANGO_WEIGHT_NORMAL,
					"wrap-mode",GTK_WRAP_WORD,
					NULL);
    tag[4] = gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(tb),"tag4",
					"font","Sans 12",
					"foreground","#0000ff",
					"justification",GTK_JUSTIFY_LEFT,
					"editable",FALSE,
					"weight",PANGO_WEIGHT_BOLD,
					"wrap-mode",GTK_WRAP_WORD,
					NULL);
    tag[5] = gtk_text_buffer_create_tag(GTK_TEXT_BUFFER(tb),"tag5",
					"font","Sans 16",
					"foreground","#0000ff",
					"justification",GTK_JUSTIFY_LEFT,
					"editable",FALSE,
					"weight",PANGO_WEIGHT_BOLD,
					"wrap-mode",GTK_WRAP_WORD,
					NULL);

				       
    int ctag = 0, tval[2] = {0,0};
    q  = T;
    for(p=T;*p;p++) {
      
      if (*p=='#') {
	if (p-q) {
	  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(tb),&iter);
	  gtk_text_buffer_insert_with_tags(GTK_TEXT_BUFFER(tb),&iter,q,p-q,tag[ctag],NULL);
	}
	q = p + 3;

	switch(p[1]) {
	case 'L': tval[0]=2; break;
	case 'M': tval[0]=1; break;
	case 'S': tval[0]=0; break;
	case 'B': tval[1]=1; break;
	case 'K': tval[1]=0; break;
	}
	ctag = 3*tval[1] + tval[0];
	p += 3;
      }
      
    }

  }

} // namespace Help

