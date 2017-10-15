/* $Id: widgetproxy.cc,v 1.38 2007/05/23 18:28:17 bergo Exp $ */

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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "widgetproxy.h"
#include "global.h"
#include "eboard.h"

// ColorSpace class
#include "pieces.h"

WidgetProxy::WidgetProxy() {
	widget=0;
}

void WidgetProxy::show() {
	gshow(widget);
}

void WidgetProxy::hide() {
	gtk_widget_hide(widget);
}

void WidgetProxy::repaint() {
	gtk_widget_queue_resize(widget);
}

GtkWidget * WidgetProxy::scrollBox(GtkWidget *child) {
	GtkWidget *scr;
	scr = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scr),
										GTK_SHADOW_NONE);
	gtk_container_set_border_width(GTK_CONTAINER(scr),4);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr),
								   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scr), child);
	gtk_widget_show(scr);
	return scr;
}

void WidgetProxy::setIcon(char **xpmdata,const char *text) {
	GdkPixmap *icon;
	GdkBitmap *mask;
	GtkStyle *style;
	style=gtk_widget_get_style(widget);
	icon = gdk_pixmap_create_from_xpm_d (widget->window, &mask,
										 &style->bg[GTK_STATE_NORMAL],
										 (gchar **) xpmdata);
	gdk_window_set_icon (widget->window, NULL, icon, mask);
	gdk_window_set_icon_name(widget->window,(gchar *)text);
}

void WidgetProxy::restorePosition(WindowGeometry *wg) {
	if (!widget) return;
	if (wg->isNull()) return;

	gdk_window_move_resize(widget->window,wg->X,wg->Y,wg->W,wg->H);
}

GtkWidget * WidgetProxy::PixButton(char **xpmicon, char *caption) {
	GdkPixmap *d;
	GdkBitmap *mask;
	GtkWidget *b, *p, *v, *t;
	GtkStyle *style;

	b=gtk_button_new();
	v=gtk_vbox_new(FALSE,1);
	gtk_container_set_border_width(GTK_CONTAINER(v),2);
	gtk_container_add(GTK_CONTAINER(b),v);

	style=gtk_widget_get_style(widget);
	d = gdk_pixmap_create_from_xpm_d (widget->window, &mask,
									  &style->bg[GTK_STATE_NORMAL],
									  (gchar **) xpmicon);
	p=gtk_pixmap_new(d, mask);
	gtk_box_pack_start(GTK_BOX(v),p,FALSE,TRUE,0);
	t=gtk_label_new(caption);
	gtk_box_pack_start(GTK_BOX(v),t,FALSE,TRUE,0);

	Gtk::show(t,p,v,NULL);
	return b;
}

// ------------

BoxedLabel::BoxedLabel(char *text) {
	widget=gtk_hbox_new(FALSE,0);
	label=gtk_label_new(text);
	gtk_label_set_justify(GTK_LABEL(label),GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(widget),label,FALSE,FALSE,0);
	gshow(label);
}

void BoxedLabel::setText(char *msg) {
	gtk_label_set_text(GTK_LABEL(label),msg);
}

// -------------

/* titles must be marked with N_(...) */
ModalDialog::ModalDialog(char *title) {
	widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(widget),eboard_gettext(title));

	gtk_window_set_position(GTK_WINDOW(widget),GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(widget),6);
	gtk_widget_realize(widget);
	focused_widget=0;
	closeable = true;

	gtk_signal_connect(GTK_OBJECT(widget),"delete_event",
					   GTK_SIGNAL_FUNC(modal_closereq),(gpointer)this);
}

void ModalDialog::show() {
	gshow(widget);
	gtk_grab_add(widget);

	if (focused_widget)
		gtk_widget_grab_focus(focused_widget);
}

void ModalDialog::release() {
	releaseWithoutDelete();
	delete this;
}

void ModalDialog::releaseWithoutDelete() {
	gtk_grab_remove(widget);
	gtk_widget_destroy(widget);
}

ModalDialog::~ModalDialog() {

}

void ModalDialog::setDismiss(GtkObject *obj,char *sig) {
	gtk_signal_connect(obj,sig,GTK_SIGNAL_FUNC(modal_release),(gpointer)this);
}

void ModalDialog::setCloseable(bool v) {
	closeable = v;
}

void modal_release(GtkWidget *w,gpointer data) {
	ModalDialog *me;
	me=(ModalDialog *)data;
	me->release();
}

gint modal_closereq(GtkWidget * widget,
					GdkEvent * event, gpointer data)
{
	ModalDialog *me = (ModalDialog *) data;
	return(me->closeable ? FALSE : TRUE);
}

// -- non modal

NonModalDialog::NonModalDialog(char *title) : ModalDialog(title) {

}

NonModalDialog::~NonModalDialog() {

}

void NonModalDialog::show() {
	gshow(widget);
	if (focused_widget)
		gtk_widget_grab_focus(focused_widget);
}

void NonModalDialog::releaseWithoutDelete() {
	gtk_widget_destroy(widget);
}

// ------------ progress
gint upw_delete (GtkWidget * widget, GdkEvent * event, gpointer data);

gint
upw_delete (GtkWidget * widget, GdkEvent * event, gpointer data) {
	return TRUE;
}

UnboundedProgressWindow::UnboundedProgressWindow(const char *_templ) :
	WidgetProxy()
{
	GtkStyle *style;
	GdkColor one;
	int i;

	templ=_templ;

	widget=gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_title(GTK_WINDOW(widget),_("Progress"));
	gtk_window_set_position(GTK_WINDOW(widget),GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(widget),6);

	style=gtk_style_copy( gtk_widget_get_style(widget) );
	one.red=one.green=one.blue=0;
	for(i=0;i<5;i++)
		style->bg[i]=one;
	one.red=one.green=one.blue=0xffff;
	for(i=0;i<5;i++)
		style->fg[i]=one;
	gtk_widget_set_style(widget,style);

	gtk_widget_realize(widget);
	label=gtk_label_new(_("Working..."));
	gtk_widget_set_style(label,style);
	gtk_container_add(GTK_CONTAINER(widget),label);
	Gtk::show(label,widget,NULL);

	gtk_signal_connect(GTK_OBJECT(widget),"delete_event",
					   GTK_SIGNAL_FUNC(upw_delete),0);

	for(i=3;i;i--) // avoid locks with idle hooks
		if(gtk_events_pending())
			gtk_main_iteration();
		else
			break;
}

UnboundedProgressWindow::~UnboundedProgressWindow() {
	gtk_widget_destroy(widget);
}

void UnboundedProgressWindow::setProgress(int v) {
	char z[256];
	int i;
	snprintf(z,256,templ,v);
	gtk_label_set_text(GTK_LABEL(label),z);
	gtk_widget_queue_resize(label);
	for(i=3;i;i--) // avoid locks with idle hooks
		if(gtk_events_pending())
			gtk_main_iteration();
		else
			break;
}

// -----------------------------

ColorButton::ColorButton(char *caption, int value) {
	ColorValue=value;
	widget=gtk_button_new();
	label=gtk_label_new(caption);
	gtk_container_add(GTK_CONTAINER(widget),label);

	notified=0;
	updateButtonFace();

	Gtk::show(label,widget,NULL);
	gtk_signal_connect(GTK_OBJECT(widget),"clicked",
					   GTK_SIGNAL_FUNC(colorb_click),(gpointer)this);
}

void ColorButton::setColor(int value) {
	ColorValue=value;
	updateButtonFace();
}

int  ColorButton::getColor() {
	return(ColorValue);
}

void ColorButton:: hookNotify(UpdateNotify *listen) {
	notified=listen;
}

void ColorButton::updateButtonFace() {
	GtkStyle *style;
	int i,dark=1;

	style=gtk_style_copy( gtk_widget_get_style(widget) );
	for(i=0;i<5;i++) {
		style->bg[i].red=(ColorValue>>16)<<8;
		style->bg[i].green=(ColorValue&0x00ff00);
		style->bg[i].blue=(ColorValue<<8)&0x00ff00;
	}

	if ( (style->bg[0].red>>8) + (style->bg[0].green>>8) + (style->bg[0].blue>>8) > 384)
		dark=0;

	style->bg[GTK_STATE_PRELIGHT].red=0xe000;
	style->bg[GTK_STATE_PRELIGHT].green=0xe000;
	style->bg[GTK_STATE_PRELIGHT].blue=0xe000;

	for(i=0;i<5;i++) {
		style->fg[i].red=dark?0xffff:0x0000;
		style->fg[i].green=dark?0xffff:0x0000;
		style->fg[i].blue=dark?0xffff:0x0000;
	}

	style->fg[GTK_STATE_PRELIGHT].red=0x0000;
	style->fg[GTK_STATE_PRELIGHT].green=0x0000;
	style->fg[GTK_STATE_PRELIGHT].blue=0x0000;

	gtk_widget_set_style(widget,style);
	gtk_widget_ensure_style(widget);

	gtk_widget_set_style(label,style);
	gtk_widget_queue_draw( label );

	if (notified) notified->update();
}

void colorb_click(GtkWidget *b,gpointer data) {
	ColorButton *me;
	GtkWidget *dlg;
	gdouble c[3];

	me=(ColorButton *)data;

	dlg=gtk_color_selection_dialog_new(_("Color Selection"));
	me->colordlg=dlg;
	c[0]=( (gdouble) ((me->ColorValue>>16)) )/256.0;
	c[1]=( (gdouble) ((me->ColorValue>>8)&0xff) ) / 256.0;
	c[2]=( (gdouble) ((me->ColorValue&0xff)) )/256.0;
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dlg)->colorsel),c);
	gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (dlg)->ok_button),
						"clicked", GTK_SIGNAL_FUNC(colorb_csok),data);
	gtk_signal_connect_object(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG (dlg)->cancel_button),
							  "clicked",
							  GTK_SIGNAL_FUNC (gtk_widget_destroy),
							  GTK_OBJECT (dlg));
	gshow(dlg);
	gtk_grab_add(dlg);
}

void colorb_csok(GtkWidget *b,gpointer data) {
	ColorButton *me;
	me=(ColorButton *)data;
	gdouble c[3];
	int v[3];
	gtk_color_selection_get_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(me->colordlg)->colorsel),c);
	v[0]=(int)(c[0]*255.0);
	v[1]=(int)(c[1]*255.0);
	v[2]=(int)(c[2]*255.0);
	me->ColorValue=(v[0]<<16)|(v[1]<<8)|v[2];
	gtk_grab_remove(me->colordlg);
	gtk_widget_destroy(me->colordlg);
	me->updateButtonFace();
}

// ===============

TextPreview::TextPreview(GdkWindow *wnd, ColorButton *_bg) {
	frozen=false;
	pending=false;
	bg=_bg;
	bg->hookNotify(this);

	pixmap=gdk_pixmap_new(wnd, 400, 300, -1);

	widget=gtk_drawing_area_new();
	gtk_widget_set_events(widget,GDK_EXPOSURE_MASK);
	gtk_drawing_area_size(GTK_DRAWING_AREA(widget),400,300);
	gtk_signal_connect(GTK_OBJECT(widget),"expose_event",
					   GTK_SIGNAL_FUNC(preview_expose),(gpointer)this);

	pl = gtk_widget_create_pango_layout(widget,NULL);
	pfd = NULL;
}

TextPreview::~TextPreview() {
	colors.clear();
	examples.clear();
	gdk_pixmap_unref(pixmap);
	if (pl!=NULL) g_object_unref(G_OBJECT(pl));
}

void TextPreview::attach(ColorButton *cb, const char *sample) {
	string s;
	s=sample;
	colors.push_back(cb);
	examples.push_back(s);
	cb->hookNotify(this);
	if (!frozen) update(); else pending=true;
}

void TextPreview::freeze() {
	frozen=true;
}

void TextPreview::thaw() {
	frozen=false;
	if (pending) update();
}

void TextPreview::update() {
	GdkGC *gc;
	PangoRectangle prl, pri;
	int i,j,y;

	pending=false;

	if (!pixmap) return;
	gc=gdk_gc_new(pixmap);
	gdk_rgb_gc_set_foreground(gc, bg->getColor() );
	gdk_draw_rectangle(pixmap,gc,TRUE,0,0,400,300);

	if (pfd!=NULL) {
		pango_font_description_free(pfd);
		pfd = NULL;
	}
	pfd =  pango_font_description_from_string(global.ConsoleFont);
	if (pfd == NULL) {
		cerr << "[eboard] TextPreview unable to set font " << global.ConsoleFont << endl;
		return;
	}

	pango_layout_set_font_description(pl, pfd);

	j=examples.size();
	y=2;
	for(i=0;i<j;i++) {
		gdk_rgb_gc_set_foreground(gc, colors[i]->getColor() );
		pango_layout_set_text(pl, examples[i].c_str(), -1);
		pango_layout_get_pixel_extents(pl, &pri, &prl);
		gdk_draw_layout(pixmap,gc,5,y,pl);
		y += prl.height + 2;
	}

	gdk_gc_destroy(gc);
	gtk_widget_queue_resize(widget);
}

gboolean preview_expose(GtkWidget *widget,GdkEventExpose *ee,
						gpointer data) {
	TextPreview *me;
	GdkGC *gc;
	me=(TextPreview *)data;
	if (me->pixmap == NULL) return false;
	gc = gdk_gc_new(widget->window);
	gdk_draw_pixmap(widget->window,gc,me->pixmap,0,0,0,0,400,300);
	gdk_gc_destroy(gc);
	return true;
}

// FileDialog

FileDialog::FileDialog(char *title) : WidgetProxy() {

	widget=gtk_file_selection_new(title);

	FileName[0] = 0;
	gotresult = false;

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (widget)->ok_button),
						"clicked", GTK_SIGNAL_FUNC(filedlg_ok), (gpointer) this );
	gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION
										   (widget)->cancel_button),
							   "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
							   GTK_OBJECT (widget));
	gtk_signal_connect (GTK_OBJECT (widget), "destroy",
						GTK_SIGNAL_FUNC(filedlg_destroy), (gpointer) this );
}

FileDialog::~FileDialog() {

}

bool FileDialog::run() {
	gshow(widget);
	gtk_grab_add(widget);

	gotresult = false;
	destroyed = false;
	while(!gotresult) {
		if (gtk_events_pending())
			global.WrappedMainIteration();
		else
			usleep(50000); // prevent busy waiting
	}

	if (!destroyed) {
		g_strlcpy(FileName,gtk_file_selection_get_filename( GTK_FILE_SELECTION(widget) ), 128 );
		gtk_grab_remove(widget);
		gtk_widget_destroy(widget);
		return true;
	}

	return false;
}

void filedlg_ok (GtkWidget * w, gpointer data) {
	FileDialog *fd;
	fd=(FileDialog *)data;
	fd->gotresult = true;
}

void filedlg_destroy (GtkWidget * w, gpointer data) {
	FileDialog *fd;
	fd=(FileDialog *)data;
	fd->destroyed=true;
	fd->gotresult=true;
}

// ----------- layout box for the clock (previous code was getting out of hand)

LayoutBox::LayoutBox() {
	int i;
	GC=NULL;
	Target=NULL;
	X=Y=W=H=0;
	for(i=0;i<5;i++)
		pfd[i] = NULL;
	pl = NULL;
}

LayoutBox::~LayoutBox() {
	int i;
	if (pl!=NULL)
		g_object_unref(G_OBJECT(pl));
	for(i=0;i<5;i++)
		if (pfd[i] != NULL)
			pango_font_description_free(pfd[i]);
}

void LayoutBox::prepare(GtkWidget *w, GdkPixmap *t, GdkGC *gc, int a, int b, int c, int d) {
	int i;
	Target = t;
	GC = gc;
	X=a; Y=b, W=c; H=d;

	// leak, tomv
	if (pl!=NULL)
		g_object_unref(G_OBJECT(pl));
	for(i=0;i<5;i++)
		if (pfd[i]!=NULL)
			pango_font_description_free(pfd[i]);

	for(i=0;i<5;i++)
		pfd[i] = NULL;

	pl = gtk_widget_create_pango_layout(w,NULL);
}

bool LayoutBox::prepared() {
	return(GC!=NULL && Target!=NULL && pl!=NULL && pfd[0]!=NULL);
}

void LayoutBox::setFont(int i,const char *fontname) {
	i%=5;
	if (pfd[i] != NULL) {
		pango_font_description_free(pfd[i]);
		pfd[i] = NULL;
	}

	pfd[i] = pango_font_description_from_string(fontname);
	if (pfd[i] == NULL)
		cerr << "[eboard] unable to set font " << fontname << endl;
}

void LayoutBox::setColor(int c) {
	gdk_rgb_gc_set_foreground(GC,c);
}

void LayoutBox::drawRect(int x,int y,int w,int h,bool fill) {
	gdk_draw_rectangle(Target,GC,fill?TRUE:FALSE,x,y,w,h);
}

void LayoutBox::drawLine(int x,int y,int w,int h) {
	gdk_draw_line(Target,GC,x,y,w,h);
}

void LayoutBox::drawEllipse(int x,int y,int w,int h,bool fill) {
	gdk_draw_arc(Target,GC,fill?TRUE:FALSE,x,y,w,h,0,360*64);
}

void LayoutBox::consumeTop(int v) { Y+=v; H-=v; }
void LayoutBox::consumeBottom(int v) { H-=v; }

void LayoutBox::drawString(int x,int y, int f, const char *s) {
	if (pl != NULL) {
		pango_layout_set_font_description(pl, pfd[f%5]);
		pango_layout_set_text(pl,s,-1);
		gdk_draw_layout(Target,GC,x,y,pl);

		/*
		  PangoRectangle prl, pri;
		  pango_layout_get_pixel_extents(pl, &pri, &prl);

		  gdk_draw_rectangle(Target,GC,FALSE,x+prl.x,y+prl.y,
		  prl.width,prl.height);
		  gdk_draw_line(Target,GC,x+prl.x,y+prl.y,
          x+prl.x+prl.width,y+prl.y+prl.height);
		  gdk_draw_line(Target,GC,x+prl.x,y+prl.y+prl.height,
          x+prl.x+prl.width,y+prl.y);
		*/

	}
}

void LayoutBox::drawString(int x,int y, int f, string &s) {
	drawString(x,y,f,s.c_str());
}

void LayoutBox::drawSubstring(int x,int y, int f, const char *s,int len) {
	if (pl != NULL) {
		pango_layout_set_font_description(pl, pfd[f%5]);
		pango_layout_set_text(pl,s,len);
		gdk_draw_layout(Target,GC,x,y,pl);
	}
}

int LayoutBox::stringWidth(int f,const char *s) {
	PangoRectangle prl, pri;
	if (pl != NULL) {
		pango_layout_set_font_description(pl, pfd[f%5]);
		pango_layout_set_text(pl,s,-1);
		pango_layout_get_pixel_extents(pl, &pri, &prl);
		return(prl.width);
	} else
		return 0;
}

int LayoutBox::substringWidth(int f,const char *s,int len) {
	PangoRectangle prl, pri;
	if (pl != NULL) {
		pango_layout_set_font_description(pl, pfd[f%5]);
		pango_layout_set_text(pl,s,len);
		pango_layout_get_pixel_extents(pl, &pri, &prl);
		return(prl.width);
	} else
		return 0;
}

int LayoutBox::fontHeight(int f) {
	PangoRectangle prl, pri;
	if (pl != NULL) {
		pango_layout_set_font_description(pl, pfd[f%5]);
		pango_layout_set_text(pl,"Aypb178XW@",-1);
		pango_layout_get_pixel_extents(pl, &pri, &prl);
		return(prl.height);
	} else
		return 0;
}

bool LayoutBox::drawBoxedString(int x,int y,int w,int h,
								int f,char *s,char *sep)
{
	int cy, lskip;
	char *first, *p, *last;

	lskip = fontHeight(f) + 2;

	// base case: string fits in one line
	if (stringWidth(f,s) <= w) {
		drawString(x,y,f,s);
		return true;
	}

	cy = y;
	last = s + strlen(s) - 1;
	first = s;

	while( *first == ' ' ) ++first;
	while(first < last) {

		if ( (cy+lskip) > (y+h) )
			return false;

		for(p=last;p!=first;p--) {
			if (!strchr(sep,*p)) continue;
			if (substringWidth(f,first,p-first+1) <= w) {
				drawSubstring(x,cy,f,first,p-first+1);
				first = p+1;
				cy += lskip;
				break;
			} // if substring...
		} // for

		// couldn't fit line, repeat the step but break anywhere :-(
		if (p==first) {
			for(p=last;p!=first;p--) {
				if (substringWidth(f,first,p-first+1) <= w) {
					drawSubstring(x,cy,f,first,p-first+1);
					first = p+1;
					cy += lskip;
					break;
				}
			}
		} // if p==first

		while( *first == ' ' ) ++first;

	} // while
	return true;

} // method

int LayoutBox::drawButton(int x, int y, int w, int h, char *s, int font,
						  int fg,int bg)
{
	ColorSpace cs;
	int i,j;

	if (s) {
		j=stringWidth(font,s);
		i=fontHeight(font);
	} else {
		j=16;
		i=10;
	}

	if (w<0) w=j+10;

	setColor(bg);
	drawRect(x,y,w,h,true);

	setColor(cs.darker(bg));
	drawRect(x+1,y+1,w-2,h-2,false);
	drawRect(x+2,y+2,w-4,h-4,false);

	setColor(cs.lighter(bg));
	drawLine(x+1,y+1,x+w-1,y+1);
	drawLine(x+2,y+2,x+w-2,y+2);
	drawLine(x+1,y+1,x+1,y+h-1);
	drawLine(x+2,y+2,x+2,y+h-2);

	setColor(fg);
	drawRect(x,y,w,h,false);

	if (s)
		drawString(x+(w-j)/2, y + ((3+h-i)/2) - 1,font,s);

	return w;
}

DropBox::DropBox(char *option, ...) {
	va_list ap;
	GtkMenu *m;
	GtkWidget *mi;
	char *p;
	int i;

	m = GTK_MENU(gtk_menu_new());

	p = option;
	mi = gtk_menu_item_new_with_label(p);
	gtk_menu_shell_append(GTK_MENU_SHELL(m),mi);

	gtk_signal_connect(GTK_OBJECT(mi), "activate",
					   GTK_SIGNAL_FUNC(dropbox_select),
					   (gpointer) this);
	gshow(mi);
	options.push_back(mi);

	va_start(ap, option);

	for(;;) {
		p=va_arg(ap,char *);
		if (!p) break;

		mi = gtk_menu_item_new_with_label(p);
		gtk_menu_shell_append(GTK_MENU_SHELL(m),mi);
		gtk_signal_connect(GTK_OBJECT(mi), "activate",
						   GTK_SIGNAL_FUNC(dropbox_select),
						   (gpointer) this);
		gshow(mi);
		options.push_back(mi);
	}
	va_end(ap);

	gshow(GTK_WIDGET(m));

	widget = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(widget), GTK_WIDGET(m));
	selection = 0;
	listener = 0;
}

DropBox::~DropBox() {
	options.clear();
}

int  DropBox::getSelection() {
	return selection;
}

void DropBox::setSelection(int i) {
	gtk_option_menu_set_history(GTK_OPTION_MENU(widget), i);
	selection = i;
}

void DropBox::setUpdateListener(UpdateInterface *ui) {
	listener = ui;
}

void dropbox_select(GtkWidget *w, gpointer data) {
	DropBox *me = (DropBox *) data;
	int i,j;

	j = me->options.size();

	for(i=0;i<j;i++)
		if (w == me->options[i]) {
			me->selection = i;
			if (me->listener)
				me->listener->update();
			break;
		}
}

namespace Gtk {

	void show(GtkWidget *w, ...) {
		va_list ap;
		GtkWidget *x;
		va_start(ap,w);
		gshow(w); // gtk_widget_show , defined in eboard.h
		for(;;) {
			x=va_arg(ap,GtkWidget *);
			if (!x) break;
			gshow(x);
		}
		va_end(ap);
	}

} // namespace Gtk
