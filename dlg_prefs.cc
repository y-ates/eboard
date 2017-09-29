/* $Id: dlg_prefs.cc,v 1.54 2008/02/22 07:32:15 bergo Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "eboard.h"
#include "dlg_prefs.h"
#include "global.h"
#include "tstring.h"

#include "snd_test.xpm"
#include "snd_edit.xpm"

// 3700 widgets for the price of 3699. Only today at dlg_prefs.cc, the
// Widget Warehouse.

typedef void (*sigfunc)(int);

char * PreferencesDialog::FontSample[NFONTS] = {
  "52:38",
  "White Capablanca",
  "38. N@c5+",  
  "AOLer(1): I have a question...",
  "51   Player1   ++++   (U)   5 0"
};

PreferencesDialog::PreferencesDialog() : ModalDialog(N_("Preferences")) {
  GtkWidget *v,*hs,*ok,*apply,*cancel,*bhb,*nb;

  // scroll containers for tabs
  GtkWidget *scr[7]; // ui, ics, colors, fonts, sounds, autosave, joystick

  // gui
  GtkWidget *uil,*uiv,*uih,*tfr,*tr[4],*tv,*uih3,*uih4,*uih5;
  GSList *tl;

  // -- fonts
  GtkWidget *fl, *fv, *ft, *fn[NFONTS], *fb[NFONTS], *frv;

  // -- ics
  GtkWidget *icv, *icl, *ict1, *icl1, *ifr1, *ifr2, *ifv1, *ifv2, 
    *icl2, *icl3,*spf, *sph;
  GSList *spl;

  // -- sound
  static char *zcaption[]={N_("Opponent Moved"),
			   N_("Draw Offered"),
			   N_("Private Tell"),
			   N_("ICS Challenge"),
			   N_("Time Running Out"),
                           N_("Game Won"), 
			   N_("Game Lost"), 
			   N_("Game Started"),
                           N_("Obs'vd Game Ended"), 
			   N_("Move made (Obs'vd/Exm'd Games)") };

  GtkWidget *sv,*sl,*zf,*zv,*zt,*zl,*zhe[N_SOUND_EVENTS], *zbb[N_SOUND_EVENTS];
  GtkStyle *style;
  GdkPixmap *xpmedit, *xpmtest;
  GdkBitmap *bitedit, *bittest;
  GtkWidget *pic[2][N_SOUND_EVENTS];

  // -- autosave
  GtkWidget *gv, *gl, *gll, *gh;

  // -- joystick
  GtkWidget *jv, *jl, *jf, *jv2, *jh1, *jh2, *jh3, *jh4, *jl2;

  // -- colors
  GtkWidget *tcl, *tcv, *tch, *tcv2, *tcdef;
  int i;
  char z[64];

  for(i=0;i< N_SOUND_EVENTS;i++)
    sndcopy[i]=global.sndevents[i];

  jsval[0] = global.JSCursorAxis;
  jsval[1] = global.JSMoveButton;
  jsval[2] = global.JSBrowseAxis;
  jsval[3] = global.JSPrevTabButton;
  jsval[4] = global.JSNextTabButton;
  jstate = -1;

  gtk_window_set_default_size(GTK_WINDOW(widget),600,480);

  v=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(widget),v);

  nb=gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(v),nb,TRUE,TRUE,2);

  gtk_widget_realize(widget);

  // user interface ===================================================================
  
  uil=gtk_label_new(_("Appearance"));
  uiv=gtk_vbox_new(FALSE,2);  
  gtk_container_set_border_width(GTK_CONTAINER(uiv),6);
  tfr=gtk_frame_new(_("Tab Position"));
  gtk_box_pack_start(GTK_BOX(uiv),tfr,FALSE,FALSE,4);
  gtk_frame_set_shadow_type(GTK_FRAME(tfr),GTK_SHADOW_ETCHED_IN);
  tv=gtk_hbox_new(FALSE,2);

  tr[0]=gtk_radio_button_new_with_label(0,_("Right"));
  tl=gtk_radio_button_group(GTK_RADIO_BUTTON(tr[0]));
  tr[1]=gtk_radio_button_new_with_label(tl,_("Left"));
  tl=gtk_radio_button_group(GTK_RADIO_BUTTON(tr[1]));
  tr[2]=gtk_radio_button_new_with_label(tl,_("Top"));
  tl=gtk_radio_button_group(GTK_RADIO_BUTTON(tr[2]));
  tr[3]=gtk_radio_button_new_with_label(tl,_("Bottom"));

  gtk_container_add(GTK_CONTAINER(tfr),tv);
  for(i=0;i<4;i++) {
    gtk_box_pack_start(GTK_BOX(tv),tr[i],FALSE,FALSE,2);
    gshow(tr[i]);
    tabposb[i]=tr[i];
  }

  uih=gtk_hbox_new(FALSE,4);
  plainb=gtk_check_button_new_with_label(_("Use plain color squares"));
  gtk_box_pack_start(GTK_BOX(uih),plainb,FALSE,FALSE,0); 

  lsq=new ColorButton(_("Light Squares..."), global.LightSqColor);
  dsq=new ColorButton(_("Dark Squares..."), global.DarkSqColor);

  gtk_box_pack_start(GTK_BOX(uih),lsq->widget,FALSE,FALSE,4); 
  gtk_box_pack_start(GTK_BOX(uih),dsq->widget,FALSE,FALSE,4); 

  gtk_box_pack_start(GTK_BOX(uiv),uih,FALSE,FALSE,4);
  Gtk::show(plainb,uih,NULL);

  gtset(GTK_TOGGLE_BUTTON(plainb), global.PlainSquares?1:0);

  uih3=gtk_hbox_new(FALSE,4);
  uih4=gtk_hbox_new(FALSE,4);
  uih5=gtk_hbox_new(FALSE,4);

  smoothb=gtk_check_button_new_with_label(_("Smoother animation (eats more CPU)"));
  gtk_box_pack_start(GTK_BOX(uih5),smoothb,FALSE,FALSE,0);

  dhsb=gtk_check_button_new_with_label(_("Graphic representation of crazy/bughouse stock"));
  gtk_box_pack_start(GTK_BOX(uih3),dhsb,FALSE,FALSE,0);

  aqbar=gtk_check_button_new_with_label(_("Show shortcut buttons below board"));
  gtk_box_pack_start(GTK_BOX(uih4),aqbar,FALSE,FALSE,0);

  gtk_box_pack_start(GTK_BOX(uiv),uih5,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(uiv),uih3,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(uiv),uih4,FALSE,FALSE,4);

  Gtk::show(smoothb, dhsb, aqbar, uih3, uih4, uih5, NULL);

  gtset(GTK_TOGGLE_BUTTON(smoothb), global.SmootherAnimation?1:0);
  gtset(GTK_TOGGLE_BUTTON(dhsb), global.DrawHouseStock?1:0);
  gtset(GTK_TOGGLE_BUTTON(aqbar), global.ShowQuickbar?1:0);

  scr[0] = scrollBox(uiv);
  Gtk::show(tv, tfr, uiv, uil, NULL);

  gtk_notebook_append_page(GTK_NOTEBOOK(nb),scr[0],uil);
  gtset(GTK_TOGGLE_BUTTON(tr[global.TabPos]), TRUE);

  // ics ===============================================================================
  icv=gtk_vbox_new(FALSE,2);
  icl=gtk_label_new("ICS");

  gtk_container_set_border_width(GTK_CONTAINER(icv),6);

  ifr1=gtk_frame_new(_("Seek Graph"));
  gtk_frame_set_shadow_type(GTK_FRAME(ifr1),GTK_SHADOW_ETCHED_IN);
  ifr2=gtk_frame_new(_("Channel Tells"));
  gtk_frame_set_shadow_type(GTK_FRAME(ifr2),GTK_SHADOW_ETCHED_IN);
  spf=gtk_frame_new(_("Non-ASCII Character Filtering"));
  gtk_frame_set_shadow_type(GTK_FRAME(spf),GTK_SHADOW_ETCHED_IN);
  sph=gtk_hbox_new(FALSE,2);

  ifv1=gtk_vbox_new(FALSE,2);
  ifv2=gtk_vbox_new(FALSE,2);
  gtk_container_add(GTK_CONTAINER(ifr1),ifv1);
  gtk_container_add(GTK_CONTAINER(ifr2),ifv2);

  showratb=gtk_check_button_new_with_label(_("Show rating next to player name"));
  gtk_box_pack_start(GTK_BOX(icv),showratb,FALSE,FALSE,2);

  autologb=gtk_check_button_new_with_label(_("Run autofics.pl script after connecting to FICS"));
  gtk_box_pack_start(GTK_BOX(icv),autologb,FALSE,FALSE,2);

  showts=gtk_check_button_new_with_label(_("Show timestamps for text lines"));
  gtk_box_pack_start(GTK_BOX(icv),showts,FALSE,FALSE,2);

  /*
  allob[0]=gtk_check_button_new_with_label(FIXTRAN("Show observers on played games"));
  allob[1]=gtk_check_button_new_with_label(FIXTRAN("Show observers on observed games"));

  gtk_box_pack_start(GTK_BOX(icv),allob[0],FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(icv),allob[1],FALSE,FALSE,2);
  */

  gtk_box_pack_start(GTK_BOX(icv),spf,FALSE,FALSE,2);
  special[0] = gtk_radio_button_new_with_label(0,_("None"));
  spl=gtk_radio_button_group(GTK_RADIO_BUTTON(special[0]));
  special[1] = gtk_radio_button_new_with_label(spl,_("Truncate"));
  spl=gtk_radio_button_group(GTK_RADIO_BUTTON(special[1]));
  special[2] = gtk_radio_button_new_with_label(spl,_("Use underscores"));
  spl=gtk_radio_button_group(GTK_RADIO_BUTTON(special[2]));
  special[3] = gtk_radio_button_new_with_label(spl,_("Soft translate"));
  spl=gtk_radio_button_group(GTK_RADIO_BUTTON(special[3]));

  gtk_container_add(GTK_CONTAINER(spf),sph);
  for(i=0;i<4;i++) {
    gtk_box_pack_start(GTK_BOX(sph),special[i],FALSE,FALSE,2);
    gshow(special[i]);
  }

  gtk_box_pack_start(GTK_BOX(icv),ifr1,FALSE,FALSE,2);

  sgb=gtk_check_button_new_with_label(_("Dynamic Seek Graph"));
  gtk_box_pack_start(GTK_BOX(ifv1),sgb,FALSE,FALSE,2);

  hsb=gtk_check_button_new_with_label(_("Inhibit seek lines on console when Seek Graph is active"));
  gtk_box_pack_start(GTK_BOX(ifv1),hsb,FALSE,FALSE,2);

  gtk_box_pack_start(GTK_BOX(icv),ifr2,FALSE,FALSE,2);

  chsb=gtk_check_button_new_with_label(_("Show channel tells in one pane per channel"));
  gtk_box_pack_start(GTK_BOX(ifv2),chsb,FALSE,FALSE,2);

  coct=gtk_check_button_new_with_label(_("Show channel tells on console too (when above option is active)"));
  gtk_box_pack_start(GTK_BOX(ifv2),coct,FALSE,FALSE,2);

  wget=gtk_check_button_new_with_label(_("Retrieve ICS Channel Lists from eboard.sf.net"));
  gtk_box_pack_start(GTK_BOX(ifv2),wget,FALSE,FALSE,0);

  ict1=gtk_table_new(2,3,FALSE);

  icl1=gtk_label_new(_("Scrollback limit (0 = unlimited) :"));
  sbacke=gtk_entry_new();
  snprintf(z,64,"%d",global.ScrollBack);
  gtk_entry_set_text(GTK_ENTRY(sbacke),z);

  gtk_table_attach_defaults(GTK_TABLE(ict1), icl1, 0,1, 0,1);
  gtk_table_attach_defaults(GTK_TABLE(ict1), sbacke, 1,2, 0,1);

  icl2=gtk_label_new(_("Warn when own clock is below: "));
  lowtime=gtk_entry_new();
  snprintf(z,64,"%d",global.LowTimeWarningLimit);
  gtk_entry_set_text(GTK_ENTRY(lowtime),z);
  icl3=gtk_label_new(_("seconds."));

  gtk_table_attach_defaults(GTK_TABLE(ict1), icl2, 0,1, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(ict1), lowtime, 1,2, 1,2);
  gtk_table_attach_defaults(GTK_TABLE(ict1), icl3, 2,3, 1,2);

  gtk_box_pack_start(GTK_BOX(icv),ict1,FALSE,FALSE,2);

  scr[1] = scrollBox(icv);
  Gtk::show(icl1, sbacke, icl2, lowtime, icl3, ict1,
	    ifr1, ifr2, ifv1, ifv2,
	    icl, showratb, autologb, showts, /*allob[0], allob[1],*/
	    sgb, chsb, coct, wget, hsb, icv, spf, sph, NULL);
  
  gtk_notebook_append_page(GTK_NOTEBOOK(nb),scr[1],icl);
  
  gtset(GTK_TOGGLE_BUTTON(showratb), global.ShowRating?1:0);
  gtset(GTK_TOGGLE_BUTTON(autologb), global.FicsAutoLogin?1:0);
  gtset(GTK_TOGGLE_BUTTON(showts),   global.ShowTimestamp?1:0);
  gtset(GTK_TOGGLE_BUTTON(special[global.SpecialChars]), TRUE);
  /*
  gtset(GTK_TOGGLE_BUTTON(allob[0]), global.IcsAllObPlayed?1:0);
  gtset(GTK_TOGGLE_BUTTON(allob[1]), global.IcsAllObObserved?1:0);
  */
  gtset(GTK_TOGGLE_BUTTON(sgb), global.IcsSeekGraph?1:0);
  gtset(GTK_TOGGLE_BUTTON(hsb), global.HideSeeks?1:0);
  gtset(GTK_TOGGLE_BUTTON(chsb), global.SplitChannels?1:0);
  gtset(GTK_TOGGLE_BUTTON(coct), global.ChannelsToConsoleToo?1:0);
  gtset(GTK_TOGGLE_BUTTON(wget), global.RetrieveChannelNames?1:0);

  // colors ========================================================================

  tcl=gtk_label_new(_("Colors"));
  tcv=gtk_vbox_new(FALSE,0);
  tcv2=gtk_vbox_new(FALSE,4);
  tch=gtk_hbox_new(FALSE,4);
  gtk_container_set_border_width(GTK_CONTAINER(tch),6);
  
  textcb[0]=new ColorButton(_("Normal Text"),global.Colors.TextDefault);
  textcb[1]=new ColorButton(_("Bright Text"),global.Colors.TextBright);
  textcb[2]=new ColorButton(_("Private Tells"),global.Colors.PrivateTell);
  textcb[3]=new ColorButton(_("News/Notifications"),global.Colors.NewsNotify);
  textcb[4]=new ColorButton(_("Mamer and TDs"),global.Colors.Mamer);
  textcb[5]=new ColorButton(_("Kibitzes/Whispers"),global.Colors.KibitzWhisper);

  textcb[6]=new ColorButton(_("Shouts"),global.Colors.Shouts);
  textcb[7]=new ColorButton(_("Seek Ads"),global.Colors.Seeks);
  textcb[8]=new ColorButton(_("Channel Tells"),global.Colors.ChannelTell);
  textcb[9]=new ColorButton(_("Chess Programs"),global.Colors.Engine);
  textcb[10]=new ColorButton(_("Background"),global.Colors.Background);

  for(i=0;i<11;i++)
    gtk_box_pack_start(GTK_BOX(tcv),textcb[i]->widget,TRUE,TRUE,0);

  preview=new TextPreview(widget->window,textcb[10]);
  preview->attach(textcb[0],"Statistics for GMFoo(GM)  On for: 1 hr, 16 mins");
  preview->attach(textcb[1],"You accept the challenge of Zoobie");
  preview->attach(textcb[2],"CleverBoy tells you: eboard is the best!");
  preview->attach(textcb[3],"Notification: SsehcEmong has departed.");
  preview->attach(textcb[4],":mamer TOURNEY INFO: blah blah blah");
  preview->attach(textcb[5],"Pulga(TM)(1492)[24] whispers: watch my pawns die");
  preview->attach(textcb[6],"blik(C) shouts: I am wasting my cpu here!");
  preview->attach(textcb[7],"GMFoo (2402) seeking 45 45 rated Standard");
  preview->attach(textcb[8],"mhill(85): yes, that's exactly what I mean.");
  preview->attach(textcb[9],"My move is: d7d5");

  gtk_box_pack_start(GTK_BOX(tch),tcv,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(tch),tcv2,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(tcv2),preview->widget,FALSE,FALSE,4);

  tcdef=gtk_button_new_with_label(_("Revert to Defaults"));
  gtk_box_pack_start(GTK_BOX(tcv2),tcdef,FALSE,FALSE,4);

  scr[2] = scrollBox(tch);
  Gtk::show(preview->widget, tcdef, tch, tcv, tcv2, tcl, NULL);

  gtk_notebook_append_page(GTK_NOTEBOOK(nb),scr[2],tcl);

  gtk_signal_connect(GTK_OBJECT(tcdef),"clicked",
		     GTK_SIGNAL_FUNC(prefs_defcolor),(gpointer)this);

  // fonts ==============================================================================
  fl=gtk_label_new(_("Fonts"));
  fv=gtk_vbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(fv),6);
  ft=gtk_table_new(12,2,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(ft),4);
  gtk_table_set_col_spacings(GTK_TABLE(ft),6);

  gtk_box_pack_start(GTK_BOX(fv),ft,FALSE,FALSE,2);
  
  for(i=0;i<NFONTS;i++)
    fn[i] = gtk_entry_new();

  fm[0]=new BoxedLabel(_("Clock Font"));
  gtk_entry_set_text(GTK_ENTRY(fn[0]),global.ClockFont);
  fb[0]=gtk_button_new_with_label(_("Choose..."));

  fm[1]=new BoxedLabel(_("Player/Color Name Font"));
  gtk_entry_set_text(GTK_ENTRY(fn[1]),global.PlayerFont);
  fb[1]=gtk_button_new_with_label(_("Choose..."));

  fm[2]=new BoxedLabel(_("Game Information Font"));
  gtk_entry_set_text(GTK_ENTRY(fn[2]),global.InfoFont);
  fb[2]=gtk_button_new_with_label(_("Choose..."));

  fm[3]=new BoxedLabel(_("Console Font"));
  gtk_entry_set_text(GTK_ENTRY(fn[3]),global.ConsoleFont);
  fb[3]=gtk_button_new_with_label(_("Choose..."));

  fm[4]=new BoxedLabel(_("Seek Graph Font"));
  gtk_entry_set_text(GTK_ENTRY(fn[4]),global.SeekFont);
  fb[4]=gtk_button_new_with_label(_("Choose..."));

  for(i=0;i<NFONTS;i++) {
    gtk_table_attach(GTK_TABLE(ft),fm[i]->widget,0,1,3*i,3*i+1,GTK_FILL,GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(ft),fn[i],0,1,3*i+1,3*i+2,
		     (GtkAttachOptions)(GTK_EXPAND|GTK_FILL),GTK_FILL,0,0);
    gtk_table_attach(GTK_TABLE(ft),fb[i],1,2,3*i+1,3*i+2,GTK_SHRINK,GTK_FILL,0,0);
  }

  frv=gtk_button_new_with_label(_("Revert to defaults"));
  gtk_table_attach(GTK_TABLE(ft),frv,0,2,14,15,GTK_SHRINK,GTK_FILL,0,16);
  gshow(frv);

  for(i=0;i<NFONTS;i++) {
    fm[i]->show();
    gshow(fn[i]);
    gshow(fb[i]);
    efont[i]=fn[i];
    xfont[i]=fb[i];
  }

  scr[3] = scrollBox(fv);
  Gtk::show(ft,fv,fl,NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(nb),scr[3],fl);

  gtk_signal_connect(GTK_OBJECT(frv),"clicked",
		     GTK_SIGNAL_FUNC(prefs_frevert),(gpointer)this);

  for(i=0;i<NFONTS;i++)
    gtk_signal_connect(GTK_OBJECT(xfont[i]),"clicked",
		       GTK_SIGNAL_FUNC(prefs_cfont),(gpointer)this);

  // sounds ==============================================================================

  sv=gtk_vbox_new(FALSE,4);
  gtk_container_set_border_width(GTK_CONTAINER(sv),6);
  sl=gtk_label_new(_("Sounds"));

  zf=gtk_frame_new(_("Sound Events"));
  gtk_frame_set_shadow_type(GTK_FRAME(zf),GTK_SHADOW_ETCHED_IN);

  zv=gtk_vbox_new(FALSE,4);
  gtk_container_set_border_width(GTK_CONTAINER(zv), 4);

  zt=gtk_table_new(4,N_SOUND_EVENTS+1,FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(zt),0);
  gtk_table_set_col_spacings(GTK_TABLE(zt),2);

  gtk_box_pack_start(GTK_BOX(sv),zf,TRUE,TRUE,2);
  gtk_container_add(GTK_CONTAINER(zf),zv);
  gtk_box_pack_start(GTK_BOX(zv), zt, TRUE, TRUE, 0);

  zl=gtk_label_new(_("The checkbox on the left enables/disables the sound."));

  gtk_table_attach_defaults(GTK_TABLE(zt), zl, 0,4, 0,1);
    
  style=gtk_widget_get_style(widget);
  xpmtest = gdk_pixmap_create_from_xpm_d (widget->window, &bittest,
					  &style->bg[GTK_STATE_NORMAL],
					  (gchar **) snd_test_xpm); 
  xpmedit = gdk_pixmap_create_from_xpm_d (widget->window, &bitedit,
					  &style->bg[GTK_STATE_NORMAL],
					  (gchar **) snd_edit_xpm); 

  for(i=0;i<N_SOUND_EVENTS;i++) {
    sndon[i]=gtk_check_button_new_with_label(eboard_gettext(zcaption[i]));
    gtk_table_attach_defaults(GTK_TABLE(zt), sndon[i], 0,1, i+1, i+2);
    
    zhe[i] = gtk_hbox_new(FALSE,1);
    sndd[i]=gtk_label_new(sndcopy[i].getDescription());
    gtk_table_attach_defaults(GTK_TABLE(zt), zhe[i], 3,4, i+1, i+2);
    gtk_box_pack_start(GTK_BOX(zhe[i]), sndd[i], FALSE, FALSE, 0);

    sndtest[i]=gtk_button_new();
    sndedit[i]=gtk_button_new();

    pic[0][i] = gtk_pixmap_new(xpmtest,bittest);
    pic[1][i] = gtk_pixmap_new(xpmedit,bitedit);
    
    gtk_container_add(GTK_CONTAINER(sndtest[i]), pic[0][i]);
    gtk_container_add(GTK_CONTAINER(sndedit[i]), pic[1][i]);

    zbb[i]=gtk_hbox_new(TRUE,0);

    gtk_table_attach(GTK_TABLE(zt), zbb[i], 1,3, i+1, i+2,GTK_FILL,GTK_SHRINK,0,0);
    gtk_box_pack_start(GTK_BOX(zbb[i]),sndtest[i],TRUE,TRUE,2);
    gtk_box_pack_start(GTK_BOX(zbb[i]),sndedit[i],TRUE,TRUE,2);

    gtset(GTK_TOGGLE_BUTTON(sndon[i]), sndcopy[i].enabled);   
    Gtk::show(sndon[i],sndd[i],zhe[i],sndtest[i],sndedit[i],pic[0][i],
	      pic[1][i],zbb[i],NULL);

    gtk_signal_connect(GTK_OBJECT(sndtest[i]),"clicked",
		       GTK_SIGNAL_FUNC(prefs_sndtest), (gpointer) this);
    gtk_signal_connect(GTK_OBJECT(sndedit[i]),"clicked",
		       GTK_SIGNAL_FUNC(prefs_sndedit), (gpointer) this);
  }

  scr[4] = scrollBox(sv);
  Gtk::show(zt,zv,zf,sv,sl,zl,NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(nb),scr[4],sl);

  // autosave ========================================================================

  gl=gtk_label_new(_("Autosave"));
  gv=gtk_vbox_new(FALSE,4);
  
  gll=gtk_label_new(_("PGN filename:"));
  gh=gtk_hbox_new(FALSE,4);

  asp=gtk_check_button_new_with_label(_("Auto-save played games"));
  aso=gtk_check_button_new_with_label(_("Auto-save observed games"));
  afn=gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(afn),global.AppendFile);

  gtset(GTK_TOGGLE_BUTTON(asp), global.AppendPlayed?1:0);
  gtset(GTK_TOGGLE_BUTTON(aso), global.AppendObserved?1:0);

  gtk_box_pack_start(GTK_BOX(gv),asp,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(gv),aso,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(gv),gh,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(gh),gll,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(gh),afn,TRUE,TRUE,4);

  scr[5] = scrollBox(gv);
  
  Gtk::show(gv,gh,gl,gll,asp,aso,afn,NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(nb),scr[5],gl);

  // joystick ===========================================================

  jl = gtk_label_new(_("Joystick"));
  jv2 = gtk_vbox_new(FALSE,4);
  jv  = gtk_vbox_new(FALSE,4);
  jh1 = gtk_hbox_new(FALSE,4);
  jh2 = gtk_hbox_new(FALSE,4);
  jh3 = gtk_hbox_new(FALSE,4);
  jh4 = gtk_hbox_new(FALSE,4);
  gtk_container_set_border_width(GTK_CONTAINER(jv),6);
  gtk_container_set_border_width(GTK_CONTAINER(jv2),6);

  jf = gtk_frame_new(_("Axis & Buttons"));
  gtk_container_set_border_width(GTK_CONTAINER(jf),6);
  gtk_frame_set_shadow_type(GTK_FRAME(jf),GTK_SHADOW_ETCHED_IN);
  
  jcl = gtk_label_new(NULL);
  jctl = gtk_button_new_with_label(_("Configure Axis & Buttons"));
  
  jmode = gtk_check_button_new_with_label(_("Smooth joystick cursor"));
  jl2 = gtk_label_new(_("Smooth joystick cursor speed:"));
  jspeed = gtk_hscale_new_with_range(1.0,10.0,1.0);

  gtk_box_pack_start(GTK_BOX(jv2),jh3,FALSE,TRUE,4);
  gtk_box_pack_start(GTK_BOX(jv2),jh4,FALSE,TRUE,4);
  gtk_box_pack_start(GTK_BOX(jh3),jmode,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(jh4),jl2,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(jh4),jspeed,TRUE,TRUE,4);

  gtk_box_pack_start(GTK_BOX(jv2),jf,FALSE,TRUE,4);
  gtk_box_pack_start(GTK_BOX(jv),jh1,FALSE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(jh1),jcl,FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(jv),jh2,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(jh2),jctl,FALSE,FALSE,4);
  gtk_container_add(GTK_CONTAINER(jf),jv);

  scr[6] = scrollBox(jv2);
  
  Gtk::show(jv,jl,jcl,jctl,jh1,jh2,jf,jv2,jh3,jh4,jl2,jmode,jspeed,NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(nb),scr[6],jl);
  formatJoystickDescription();

  gtset(GTK_TOGGLE_BUTTON(jmode), global.JSMode?1:0);
  gtk_range_set_value(GTK_RANGE(jspeed), (gdouble) (global.JSSpeed));

  gtk_signal_connect(GTK_OBJECT(jctl),"clicked",
		     GTK_SIGNAL_FUNC(prefs_joyctl),(gpointer) this);

  // bottom
  hs=gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,4);
  bhb=gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bhb), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bhb), 5);
  gtk_box_pack_start(GTK_BOX(v),bhb,FALSE,FALSE,2);
  ok=gtk_button_new_with_label(_("Ok"));
  GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
  apply=gtk_button_new_with_label(_("Apply"));
  GTK_WIDGET_SET_FLAGS(apply,GTK_CAN_DEFAULT);
  cancel=gtk_button_new_with_label(_("Cancel"));
  GTK_WIDGET_SET_FLAGS(cancel,GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(bhb),ok,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(bhb),apply,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(bhb),cancel,TRUE,TRUE,0);
  gtk_widget_grab_default(ok);

  Gtk::show(ok,apply,cancel,bhb,hs,nb,v,NULL);

  setDismiss(GTK_OBJECT(cancel),"clicked");
  gtk_signal_connect(GTK_OBJECT(ok),"clicked",
		     GTK_SIGNAL_FUNC(prefs_ok),(gpointer)this);
  gtk_signal_connect(GTK_OBJECT(apply),"clicked",
		     GTK_SIGNAL_FUNC(prefs_apply),(gpointer)this);
}

PreferencesDialog::~PreferencesDialog() {
  int i;
  for(i=0;i<3;i++)
    delete(fm[i]);
  for(i=0;i<11;i++)
    delete(textcb[i]);
  delete lsq;
  delete dsq;
  delete preview;
  global.joycapture = NULL;
}

void PreferencesDialog::ApplyCheckBox(GtkWidget *cb,int *curval,int *ch1, int *ch2) {
  int nval;
  nval=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb))?1:0;
  if (nval != *curval) {
    if (ch1) *ch1=1;
    if (ch2) *ch2=1;
    *curval=nval;
  }  
}

void PreferencesDialog::ApplyEntry(GtkWidget *entry,char *curval, int sz,
				   int *ch1, int *ch2)
{
  const char *nv;
  nv=gtk_entry_get_text(GTK_ENTRY(entry));
  if (strcmp(curval,nv)) {
    g_strlcpy(curval,nv,sz);
    if (ch1) *ch1=1;
    if (ch2) *ch2=1;
  }
}

void PreferencesDialog::ApplyColorButton(ColorButton *cb,int *curval,
					 int *ch1, int *ch2)
{
  if (cb->getColor() != *curval) {
    if (ch1) *ch1=1;
    if (ch2) *ch2=1;
    *curval=cb->getColor();
  }  
}

void PreferencesDialog::Apply() {
  int i,nval=0,changed=0,fchg=0, schg=0, psetchg=0, tbg=0;
  GtkPositionType ntp;
  const char *p;

  for(i=0;i<4;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tabposb[i]))) nval=i;

  if (nval!=global.TabPos) {
    global.TabPos=nval%4;
    changed=1;
    switch(global.TabPos) {
    case 0: ntp=GTK_POS_RIGHT; break;
    case 1: ntp=GTK_POS_LEFT; break;
    case 2: ntp=GTK_POS_TOP; break;
    case 3: ntp=GTK_POS_BOTTOM; break;
    default: ntp=GTK_POS_RIGHT;
    }
    global.ebook->setTabPosition(ntp);
  }

  for(i=0,nval=0;i<4;i++)
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(special[i]))) nval=i;

  if (nval!=global.SpecialChars) {
    global.SpecialChars = nval%4;
    changed = 1;
  }

  ApplyEntry(efont[0],global.ClockFont,96,&changed,NULL);
  ApplyEntry(efont[1],global.PlayerFont,96,&changed,NULL);
  ApplyEntry(efont[2],global.InfoFont,96,&changed,NULL);
  ApplyEntry(efont[3],global.ConsoleFont,96,&changed,&fchg);
  ApplyEntry(efont[4],global.SeekFont,96,&changed,&schg);

  ApplyCheckBox(plainb, &global.PlainSquares, &changed, &psetchg);
  ApplyCheckBox(smoothb, &global.SmootherAnimation, &changed, NULL);
  ApplyCheckBox(dhsb, &global.DrawHouseStock, &changed, NULL);
  ApplyCheckBox(aqbar, &global.ShowQuickbar, &changed, NULL);
  ApplyCheckBox(showratb, &global.ShowRating, &changed, NULL);
  ApplyCheckBox(autologb, &global.FicsAutoLogin, &changed, NULL);
  ApplyCheckBox(showts, &global.ShowTimestamp, &changed, &fchg);
  /*
  ApplyCheckBox(allob[0], &global.IcsAllObPlayed, &changed, NULL);
  ApplyCheckBox(allob[1], &global.IcsAllObObserved, &changed, NULL);
  */
  ApplyCheckBox(sgb, &global.IcsSeekGraph, &changed, NULL);
  ApplyCheckBox(hsb, &global.HideSeeks, &changed, NULL);
  ApplyCheckBox(chsb, &global.SplitChannels, &changed, NULL);
  ApplyCheckBox(coct, &global.ChannelsToConsoleToo, &changed, NULL);
  ApplyCheckBox(wget, &global.RetrieveChannelNames, &changed, NULL);
  ApplyCheckBox(asp, &global.AppendPlayed, &changed, NULL);
  ApplyCheckBox(aso, &global.AppendObserved, &changed, NULL);
  ApplyCheckBox(jmode, &global.JSMode, &changed, NULL);

  {
    int spd = (int) gtk_range_get_value(GTK_RANGE(jspeed));
    if (global.JSSpeed != spd) {
      global.JSSpeed = spd;
      changed = 1;
    }
  }

  ApplyEntry(afn,global.AppendFile,128,&changed,NULL);

  if (lsq->getColor() != global.LightSqColor) {
    changed=1; psetchg=1;
    global.LightSqColor=lsq->getColor();
  }

  ApplyColorButton(textcb[0],&global.Colors.TextDefault,&changed,NULL);
  ApplyColorButton(textcb[1],&global.Colors.TextBright,&changed,NULL);
  ApplyColorButton(textcb[2],&global.Colors.PrivateTell,&changed,NULL);
  ApplyColorButton(textcb[3],&global.Colors.NewsNotify,&changed,NULL);
  ApplyColorButton(textcb[4],&global.Colors.Mamer,&changed,NULL);
  ApplyColorButton(textcb[5],&global.Colors.KibitzWhisper,&changed,NULL);
  ApplyColorButton(textcb[6],&global.Colors.Shouts,&changed,NULL);
  ApplyColorButton(textcb[7],&global.Colors.Seeks,&changed,NULL);
  ApplyColorButton(textcb[8],&global.Colors.ChannelTell,&changed,NULL);
  ApplyColorButton(textcb[9],&global.Colors.Engine,&changed,NULL);
  ApplyColorButton(textcb[10],&global.Colors.Background,&changed,&tbg);

  if (dsq->getColor() != global.DarkSqColor) {
    changed=1; psetchg=1;
    global.DarkSqColor=dsq->getColor();
  }

  p=gtk_entry_get_text(GTK_ENTRY(sbacke));
  nval=atoi(p);
  if (nval!=global.ScrollBack) {
    changed=1;
    global.ScrollBack=nval;
    global.updateScrollBacks();
  }

  p=gtk_entry_get_text(GTK_ENTRY(lowtime));
  nval=atoi(p);
  if (nval!=global.LowTimeWarningLimit) {
    changed=1;
    global.LowTimeWarningLimit=nval;
  }

  for(i=0;i<N_SOUND_EVENTS;i++) {
    sndcopy[i].enabled = false;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sndon[i])))
      sndcopy[i].enabled = true;
    if (sndcopy[i] != global.sndevents[i]) {
      global.sndevents[i]=sndcopy[i];
      changed=1;
    }
  }

  if (global.JSCursorAxis != jsval[0] ||
      global.JSMoveButton != jsval[1] ||
      global.JSBrowseAxis != jsval[2] ||
      global.JSPrevTabButton != jsval[3] ||
      global.JSNextTabButton != jsval[4])
    changed = 1;

  global.JSCursorAxis =    jsval[0];
  global.JSMoveButton =    jsval[1];
  global.JSBrowseAxis =    jsval[2];
  global.JSPrevTabButton = jsval[3];
  global.JSNextTabButton = jsval[4];

  if (changed)
    for(global.BLi=global.BoardList.begin();
	global.BLi!=global.BoardList.end(); // isn't c++ cute ?
	global.BLi++) {
      (*global.BLi)->reloadFonts();
      if (psetchg)
	global.respawnPieceSet();
      else
	(*global.BLi)->invalidate();
      (*global.BLi)->queueRepaint();
    }
  if (fchg) {
    global.output->updateFont();
    global.updateFont();
  }

  if (changed && global.IcsSeekGraph!=0)
    if (global.network!=NULL && global.protocol!=NULL)
      if (global.network->isConnected())
	global.protocol->refreshSeeks(true);
  
  if (changed && global.IcsSeekGraph==0 && global.skgraph2 != NULL) {
    global.skgraph2->clear();
  }
  
  if (schg && global.skgraph2!=NULL) {
    global.skgraph2->updateFont();
    global.skgraph2->repaint();
  }

  if (tbg)
    global.output->setBackground(global.Colors.Background);

  if (changed)
    global.qbcontainer->update();
   
  if (changed)
    global.writeRC();
}

void prefs_defcolor(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  TerminalColor tc;
  me=(PreferencesDialog *)data;

  me->preview->freeze();
  me->textcb[0]->setColor(tc.TextDefault);
  me->textcb[1]->setColor(tc.TextBright);
  me->textcb[2]->setColor(tc.PrivateTell);
  me->textcb[3]->setColor(tc.NewsNotify);
  me->textcb[4]->setColor(tc.Mamer);
  me->textcb[5]->setColor(tc.KibitzWhisper);
  me->textcb[6]->setColor(tc.Shouts);
  me->textcb[7]->setColor(tc.Seeks);
  me->textcb[8]->setColor(tc.ChannelTell);
  me->textcb[9]->setColor(tc.Engine);
  me->textcb[10]->setColor(tc.Background);
  me->preview->thaw();
}

void prefs_apply(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  me=(PreferencesDialog *)data;
  me->Apply();
}

void prefs_ok(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  me=(PreferencesDialog *)data;
  prefs_apply(w,data);
  me->release();
}

void prefs_fcancel(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  me=(PreferencesDialog *)data;
  gtk_grab_remove(me->fontdlg);
  gtk_widget_destroy(me->fontdlg);
}

void prefs_fok(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  char *p;
  me=(PreferencesDialog *)data;
  p=gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(me->fontdlg)); 
  if (p!=NULL)
    gtk_entry_set_text(GTK_ENTRY(me->efont[me->FontBeingEdited]),p);
  gtk_grab_remove(me->fontdlg);
  gtk_widget_destroy(me->fontdlg);
}

void prefs_cfont(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  GtkWidget *fd;
  const gchar *cmd;
  int i, fbe;

  me=(PreferencesDialog *)data;  

  me->FontBeingEdited = -1;
  for(i=0;i<NFONTS;i++)
    if (w == me->xfont[i]) {
      me->FontBeingEdited = i;
      break;
    }
  if (me->FontBeingEdited < 0) {
    cerr << "**** eboard internal error: you should never see this, email bergo@seul.org\n";
    return;
  }
  fbe = me->FontBeingEdited;

  cmd = gtk_entry_get_text(GTK_ENTRY(me->efont[fbe]));

  fd=gtk_font_selection_dialog_new(_("Choose Font"));

  if (strlen(cmd))
    gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(fd),cmd);

  gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(fd),
					     me->FontSample[fbe]);

  gtk_signal_connect (GTK_OBJECT (GTK_FONT_SELECTION_DIALOG (fd)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(prefs_fok),data);
  gtk_signal_connect(GTK_OBJECT (GTK_FONT_SELECTION_DIALOG (fd)->cancel_button),
		     "clicked", GTK_SIGNAL_FUNC (prefs_fcancel), data);
  me->fontdlg=fd;
  gshow(fd);
  gtk_grab_add(fd);
}

void prefs_frevert(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  me=(PreferencesDialog *)data;
  gtk_entry_set_text(GTK_ENTRY(me->efont[0]),DEFAULT_FONT_CLOK);
  gtk_entry_set_text(GTK_ENTRY(me->efont[1]),DEFAULT_FONT_PLYR);
  gtk_entry_set_text(GTK_ENTRY(me->efont[2]),DEFAULT_FONT_INFO);
  gtk_entry_set_text(GTK_ENTRY(me->efont[3]),DEFAULT_FONT_CONS);
  gtk_entry_set_text(GTK_ENTRY(me->efont[4]),DEFAULT_FONT_SEEK);
}

// sounds (the error codes are plain random ;-)

void prefs_sndtest(GtkWidget *w,gpointer data) {
  PreferencesDialog *me = (PreferencesDialog *)data;
  for(int i=0;i<N_SOUND_EVENTS;i++)
    if (w == me->sndtest[i]) {
      me->sndcopy[i].safePlay();
      return;
    }
  cerr << "** eboard ** odd internal error 5B7X (email the author).\n"; 
}

void prefs_sndedit(GtkWidget *w,gpointer data) {
  PreferencesDialog *me = (PreferencesDialog *)data;
  for(int i=0;i<N_SOUND_EVENTS;i++)
    if (w == me->sndedit[i]) {
      me->sndcopy[i].edit(me);
      return;
    }
  cerr << "** eboard ** odd internal error 4FN8 (email the author).\n"; 
}

void PreferencesDialog::SoundEventChanged() {
  int i;
  for(i=0;i<N_SOUND_EVENTS;i++) {
    gtk_label_set_text(GTK_LABEL(sndd[i]), sndcopy[i].getDescription());
    gtk_widget_queue_resize(sndd[i]);
  }
}

void PreferencesDialog::formatJoystickDescription() {
  char s[256];
  
  switch(jstate) {
  case 0:
    g_strlcpy(s,_("Move the axis to be used for selecting pieces."),256);
    break;
  case 1:
    g_strlcpy(s,_("Press the button to be used for selecting a square."),256);
    break;
  case 2:
    g_strlcpy(s,_("Move the axis to be used for moving back and forth\nthrough moves of a game."),256);
    break;
  case 3:
    g_strlcpy(s,_("Press the button to be used for going to the previous tab."),256);
    break;
  case 4:
    g_strlcpy(s,_("Press the button to be used for going to the next tab."),256);
    break;
  default:
    snprintf(s,256,_("Board Cursor Axis: %d\nBoard Select Button: %d\nBoard Browsing Axis: %d\nPrevious Tab Button: %d\nNext Tab Button: %d\n"),
	    jsval[0],jsval[1],jsval[2],jsval[3],jsval[4]);
  }
  gtk_label_set_text(GTK_LABEL(jcl), s);
  gtk_widget_queue_resize(jcl);
}

void PreferencesDialog::joystickEvent(JoystickEventType jet, int number, int value) {
  switch(jstate) {
  case 0:
  case 2:
    if (jet==JOY_AXIS) { 
      jsval[jstate] = number & 0xfffe;
      jstate++;
      formatJoystickDescription();
    }
    break;
  case 1: 
  case 3: 
  case 4: 
    if (jet==JOY_BUTTON && value==1) { 
      jsval[jstate] = number;
      jstate++;
      if (jstate == 5) {
	gtk_button_set_label(GTK_BUTTON(jctl),_("Configure Axis & Buttons"));
	jstate=-1;
	global.joycapture = NULL;
      }
      formatJoystickDescription();
    }
    break;
  }
}

void prefs_joyctl(GtkWidget *w,gpointer data) {
  PreferencesDialog *me;
  me = (PreferencesDialog *) data;

  if (me->jstate < 0) {
    me->jstate = 0;
    gtk_button_set_label(GTK_BUTTON(me->jctl),_("Cancel Joystick Configuration"));
    global.joycapture = (JoystickListener *) me;
    me->formatJoystickDescription();
  } else {
    me->jstate = -1;
    gtk_button_set_label(GTK_BUTTON(me->jctl),_("Configure Axis & Buttons"));
    global.joycapture = NULL;
    me->formatJoystickDescription();
  }

}
