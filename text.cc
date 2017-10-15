/* $Id: text.cc,v 1.42 2008/02/22 14:34:30 bergo Exp $ */

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
#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include "text.h"
#include "widgetproxy.h"
#include "global.h"
#include "tstring.h"
#include "stl.h"
#include "eboard.h"

#include "icon-console.xpm"
#include "addcons.xpm"

gboolean dc_entry_focus_out(GtkWidget *widget,GdkEventFocus *event,
							gpointer user_data);
gboolean dc_entry_force_focus(gpointer data);

OutputPane::~OutputPane() {

}

Text::Text() {
	GdkFont *fxd;

	linecount=0;
	LastMatch=-1;

	setBG(global.Colors.Background);
	setFont(global.ConsoleFont);
	updateScrollBack();
}

Text::~Text() {

}

void Text::execSearch() {
	LastMatch = -1;
	findTextNext();
}

void Text::findTextNext() {
	char z[256];
	bool wrapped = false;

	if (SearchString.empty()) {
		global.status->setText(_("No previous search."),5);
		return;
	}

	if (LastMatch == 0) wrapped = true;
	if (LastMatch < 0)  LastMatch = 0;

	if (findTextUpward(0,LastMatch - 1,SearchString.c_str())) {

		LastMatch = getLastFoundLine();
		global.ebook->goToPageId(-2);
		gotoLine(LastMatch);

		if (!wrapped)
			snprintf(z,256,_("Match Found at Line %d."), LastMatch+1);
		else
			snprintf(z,256,_("(Wrapped) Match Found at Line %d."), LastMatch+1);
		global.status->setText(z,10);

	} else {

		LastMatch = -1;
		global.status->setText(_("Search text not found."),5);

	}
}

void Text::saveBuffer() {
	FileDialog *fd;

	fd=new FileDialog(_("Save Buffer As..."));

	if (fd->run()) {
		if (saveTextBuffer(fd->FileName)) {
			global.status->setText(_("Console buffer saved."),5);
		} else {
			global.status->setText(_("Buffer Save failed."),10);
		}
	}

	delete fd;
}

void Text::updateFont() {
	setFont(global.ConsoleFont);
	repaint();
}

void Text::updateScrollBack() {
	setScrollBack(global.ScrollBack);
}

void Text::pageUp()   { NText::pageUp(0.75f); }
void Text::pageDown() { NText::pageDown(0.75f); }

void Text::append(char *msg,int color,Importance imp) {
	if (Filter.accept(msg)) {
		pushLevel(imp);
		NText::append(msg,strlen(msg),color);
		contentUpdated();
		repaint();
	}
}

void Text::append(char *msg,char *msg2,int color, Importance imp) {
	char *d;
	d=(char *)g_malloc0(strlen(msg)+strlen(msg2)+1);
	strcpy(d,msg);
	strcat(d,msg2);
	append(d,color,imp);
	g_free(d);
}

void Text::setBackground(int color) {
	setBG(color);
}

// -----------------
// textset
// -----------------

TextSet::TextSet() {

}

TextSet::~TextSet() {
	while(!targets.empty())
		removeTarget(targets.front());
}

void TextSet::addTarget(Text *target) {
	targets.push_back(target);
}

void TextSet::removeTarget(Text *target) {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		if ( (*ti) == target ) {
			delete(*ti);
			targets.erase(ti);
			return;
		}
}

void TextSet::append(char *msg,int color,Importance imp) {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		(*ti)->append(msg,color,imp);
}

void TextSet::append(char *msg,char *msg2,int color,Importance imp) {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		(*ti)->append(msg,msg2,color,imp);
}

void TextSet::pageUp() {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		(*ti)->pageUp();
}

void TextSet::pageDown() {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		(*ti)->pageDown();
}

void TextSet::updateScrollBack() {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		(*ti)->updateScrollBack();
}

void TextSet::updateFont() {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		(*ti)->updateFont();
}

void TextSet::setBackground(int color) {
	list<Text *>::iterator ti;
	for(ti=targets.begin();ti!=targets.end();ti++)
		(*ti)->setBackground(color);
}

// Detached text console

int DetachedConsole::ConsoleCount=0;

DetachedConsole::DetachedConsole(TextSet *yourset, ConsoleListener *cl) {
	GtkWidget *vb,*hb,*sf,*ac,*ae;
	GdkPixmap *ad;
	GdkBitmap *ab;
	GtkStyle *style;
	char tmp[64];

	ConsoleCount++;
	global.Consoles.push_back(this);

	myset=yourset;
	listener=cl;
	// my copy of Stroustrup says one thing about the stringstream class
	// and my libstdc++ says another, better not risk compiling problems,
	// so I'm using C constructs here. Ack.
	snprintf(tmp,64,_("eboard: Console #%d"),ConsoleCount);
	basetitle=tmp;

	widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_events(widget,GDK_KEY_PRESS_MASK);
	gtk_window_set_default_size(GTK_WINDOW(widget),650,400);
	gtk_window_set_title(GTK_WINDOW(widget),basetitle.c_str());
	gtk_window_set_position(GTK_WINDOW(widget),GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(widget),4);
	gtk_widget_realize(widget);

	vb=gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(widget),vb);

	inner=new Text();
	gtk_box_pack_start(GTK_BOX(vb),inner->widget,TRUE,TRUE,0);

	hb=gtk_hbox_new(FALSE,2);

	inputbox=gtk_entry_new();
	gtk_widget_set_events(inputbox,(GdkEventMask)(gtk_widget_get_events(inputbox)|GDK_FOCUS_CHANGE_MASK));

	focus_sig_id=(int)gtk_signal_connect(GTK_OBJECT(inputbox),"focus_out_event",
										 GTK_SIGNAL_FUNC(dc_entry_focus_out),(gpointer)inputbox);

	gtk_box_pack_start(GTK_BOX(vb),hb,FALSE,TRUE,4);
	gtk_box_pack_start(GTK_BOX(hb),inputbox,TRUE,TRUE,4);

	flabel=gtk_label_new(_("Filter: (none)"));

	sf=gtk_button_new_with_label(_("Set Filter..."));
	gtk_box_pack_start(GTK_BOX(hb),flabel,FALSE,FALSE,4);
	gtk_box_pack_start(GTK_BOX(hb),sf,FALSE,FALSE,4);

	// add new console
	ac=gtk_button_new();
	style=gtk_widget_get_style(widget);
	ad = gdk_pixmap_create_from_xpm_d (widget->window, &ab,
									   &style->bg[GTK_STATE_NORMAL],
									   (gchar **) addcons_xpm);
	ae=gtk_pixmap_new(ad,ab);
	gtk_container_add(GTK_CONTAINER(ac),ae);
	gshow(ae);

	gtk_box_pack_start(GTK_BOX(hb),ac,FALSE,FALSE,4);

	Gtk::show(inputbox,sf,flabel,ac,hb,vb,NULL);
	inner->show();

	yourset->addTarget(inner);

	setIcon(icon_console_xpm,_("Console"));

	gtk_signal_connect(GTK_OBJECT(widget),"delete_event",
					   GTK_SIGNAL_FUNC(detached_delete),(gpointer)this);
	gtk_signal_connect(GTK_OBJECT(widget),"destroy",
					   GTK_SIGNAL_FUNC(detached_destroy),(gpointer)this);

	gtk_signal_connect (GTK_OBJECT (inputbox), "key_press_event",
						GTK_SIGNAL_FUNC (dc_input_key_press), (gpointer)this);

	gtk_signal_connect (GTK_OBJECT (sf), "clicked",
						GTK_SIGNAL_FUNC (dc_set_filter), (gpointer)this);

	gtk_signal_connect (GTK_OBJECT (ac), "clicked",
						GTK_SIGNAL_FUNC (dc_new_console), (gpointer)this);

	hcursor=global.inputhistory->getCursor();

	setPasswordMode(global.PasswordMode);
}

void DetachedConsole::clone() {
	DetachedConsole *dc;
	dc=new DetachedConsole(myset,0);
	//global.input->peekKeys(dc->widget);
	dc->show();
}

DetachedConsole::~DetachedConsole() {
	myset->removeTarget(inner);
	global.Consoles.erase( find(global.Consoles.begin(), global.Consoles.end(), this) );
}

void DetachedConsole::setPasswordMode(int pm) {
	gtk_entry_set_visibility(GTK_ENTRY(inputbox),(pm?FALSE:TRUE));
}

void DetachedConsole::show() {
	WidgetProxy::show();
	gtk_widget_grab_focus(inputbox);
}

void DetachedConsole::injectInput() {
	global.input->userInput(gtk_entry_get_text(GTK_ENTRY(inputbox)));
	gtk_entry_set_text(GTK_ENTRY(inputbox),"\0");
	hcursor=global.inputhistory->getCursor();
}

void DetachedConsole::historyUp() {
	gtk_entry_set_text(GTK_ENTRY(inputbox),
					   global.inputhistory->moveUp(hcursor));
}

void DetachedConsole::historyDown() {
	gtk_entry_set_text(GTK_ENTRY(inputbox),
					   global.inputhistory->moveDown(hcursor));
}

const char * DetachedConsole::getFilter() {
	return(inner->Filter.getString());
}

void DetachedConsole::setFilter(char *s) {
	inner->Filter.set(s);
	updateFilterLabel();
}

void DetachedConsole::updateFilterLabel() {
	char z[256];
	g_strlcpy(z,_("Filter: "),256);
	g_strlcat(z,inner->Filter.getString(),256);
	if (strlen(z)==strlen(_("Filter: ")))
		g_strlcat(z,_("(none)"),256);
	gtk_label_set_text(GTK_LABEL(flabel),z);
	gtk_widget_queue_resize(flabel);
}


gint detached_delete  (GtkWidget * widget, GdkEvent * event, gpointer data) {
	DetachedConsole *me;
	me=(DetachedConsole *)data;
	gtk_signal_disconnect(GTK_OBJECT(me->inputbox),me->focus_sig_id);
	return FALSE;
}

void detached_destroy (GtkWidget * widget, gpointer data) {
	DetachedConsole *me;
	me=(DetachedConsole *)data;
	if (me->listener)
		me->listener->consoleClosed();
	else
		delete me;
}

int dc_input_key_press (GtkWidget * wid, GdkEventKey * evt,
						gpointer data)
{
	DetachedConsole *me;
	me=(DetachedConsole *)data;
	switch(evt->keyval) {
	case GDK_Up:
		gtk_signal_emit_stop_by_name(GTK_OBJECT(wid), "key_press_event");
		me->historyUp();
		return 1;
	case GDK_Down:
		gtk_signal_emit_stop_by_name(GTK_OBJECT(wid), "key_press_event");
		me->historyDown();
		return 1;
	case GDK_KP_Enter:
		gtk_signal_emit_stop_by_name(GTK_OBJECT(wid), "key_press_event");
	case GDK_Return:
		me->injectInput();
		return 1;
	case GDK_Page_Up:
		me->inner->pageUp();
		return 1;
	case GDK_Page_Down:
		me->inner->pageDown();
		return 1;
	default:
		return(global.input->keyPressed(evt->keyval, evt->state));
	}
	return 0;
}

gboolean dc_entry_focus_out(GtkWidget *widget,GdkEventFocus *event,gpointer user_data)
{
	gtk_timeout_add(50,dc_entry_force_focus,user_data);
	return FALSE;
}

gboolean dc_entry_force_focus(gpointer data)
{
	gtk_widget_grab_focus(GTK_WIDGET(data));
	return FALSE;
}

void dc_set_filter(GtkWidget *w,gpointer data) {
	DetachedConsole *me;
	me=(DetachedConsole *)data;
	(new TextFilterDialog(me->inner,me->flabel))->show();
}

void dc_new_console(GtkWidget *w,gpointer data) {
	DetachedConsole *me;
	me=(DetachedConsole *)data;
	me->clone();
}

// --- set filter... dialog

TextFilterDialog::TextFilterDialog(Text *target,GtkWidget *label2update)
    : ModalDialog(N_("Set Filter"))
{
	GtkWidget *v,*h,*lb,*lb2,*hs,*bb,*ok,*can;

	obj=target;
	ulabel=label2update;

	v=gtk_vbox_new(FALSE,4);
	gtk_container_add(GTK_CONTAINER(widget),v);

	h=gtk_hbox_new(FALSE,4);
	lb=gtk_label_new(_("Match Pattern: "));
	lb2=gtk_label_new(_("Only lines that match the above pattern will be added\n"\
						"to this text pane. Patterns can be OR'ed with the | (pipe)\n"\
						"character. A * (star) can be used to match anything.\n"\
						"Examples:\n"\
						"'(20)|(22)' shows only lines from channels 20 and 22\n"\
						"'blik * bored' shows lines containing 'blik '(...)' bored'."));
	gtk_label_set_justify(GTK_LABEL(lb2),GTK_JUSTIFY_LEFT);

	gtk_box_pack_start(GTK_BOX(h),lb,FALSE,FALSE,2);
	pattern=gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(h),pattern,TRUE,TRUE,2);

	gtk_box_pack_start(GTK_BOX(v),h,FALSE,FALSE,4);
	gtk_box_pack_start(GTK_BOX(v),lb2,FALSE,FALSE,4);

	hs=gtk_hseparator_new();
	gtk_box_pack_start(GTK_BOX(v),hs,FALSE,FALSE,4);

	bb=gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bb), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bb), 5);
	gtk_box_pack_start(GTK_BOX(v),bb,FALSE,FALSE,2);

	ok=gtk_button_new_with_label(_("Ok"));
	GTK_WIDGET_SET_FLAGS(ok,GTK_CAN_DEFAULT);
	can=gtk_button_new_with_label(_("Cancel"));
	GTK_WIDGET_SET_FLAGS(can,GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bb),ok,TRUE,TRUE,0);
	gtk_box_pack_start(GTK_BOX(bb),can,TRUE,TRUE,0);
	gtk_widget_grab_default(ok);

	Gtk::show(ok,can,bb,hs,lb2,lb,pattern,h,v,NULL);
	gtk_entry_set_text(GTK_ENTRY(pattern),obj->Filter.getString());

	setDismiss(GTK_OBJECT(can),"clicked");
	gtk_signal_connect(GTK_OBJECT(ok),"clicked",
					   GTK_SIGNAL_FUNC(tfd_ok),(gpointer)this);

	focused_widget=pattern;
}

void tfd_ok(GtkWidget *w, gpointer data)
{
	TextFilterDialog *me;
	char z[1024];
	me=(TextFilterDialog *)data;
	me->obj->Filter.set(gtk_entry_get_text(GTK_ENTRY(me->pattern)));

	if (me->ulabel) {
		g_strlcpy(z,_("Filter: "),1024);
		g_strlcat(z,me->obj->Filter.getString(),1024);
		if (strlen(z)==strlen(_("Filter: ")))
			g_strlcat(z,_("(none)"),1024);
		gtk_label_set_text(GTK_LABEL(me->ulabel),z);
		gtk_widget_queue_resize(me->ulabel);
	}

	me->release();
}

// --- text filter class

TextFilter::TextFilter() {
	FilterString="";
	AcceptedLast=false;
}

TextFilter::~TextFilter() {
	cleanUp();
}

void TextFilter::set(const char *t) {
	tstring T;
	ExtPatternMatcher *epm;
	char local[1024],single[512];
	const char *p;
	char *q;
	string *P;

	cleanUp();
	if (!strlen(t))
		return;

	memset(local,0,1024);

	for(p=t,q=local;*p;p++) {
		if ((*p)=='%') *(q++)='%';
		*q++=*p;
	}

	FilterString=local;

	T.set(local);

	while( ( P=T.token("|\n\r") ) != 0 ) {
		if (! P->empty() ) {
			epm=new ExtPatternMatcher();
			single[0]=0;
			if (P->at(0) != '*')
				g_strlcat(single,"*",512);
			g_strlcat(single,P->c_str(),512);
			if (P->at(P->length()-1) != '*')
				g_strlcat(single,"*",512);
			epm->set(single);
			thefilter.push_back(epm);
		}
	}
}

const char * TextFilter::getString() {
	return(FilterString.c_str());
}

bool TextFilter::accept(char *textline) {
	int i,j;

	if (thefilter.empty()) return true;

	if ((textline[0]=='\\')&&(AcceptedLast))
		return true;

	j=thefilter.size();
	for(i=0;i<j;i++) {
		if ( thefilter[i]->match(textline) ) {
			AcceptedLast=true;
			return true;
		}
	}
	AcceptedLast=false;
	return false;
}

void TextFilter::cleanUp() {
	int i,j;
	AcceptedLast=false;
	j=thefilter.size();
	for(i=0;i<j;i++)
		delete(thefilter[i]);
	thefilter.clear();
	FilterString="";
}
