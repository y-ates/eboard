/* $Id: ntext.cc,v 1.23 2008/02/22 14:34:29 bergo Exp $ */

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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gtk/gtkselection.h>
#include "ntext.h"
#include "global.h"
#include <assert.h>

NLine::NLine() {
	Text = NULL;
	NBytes = 0;
	Color = 0;
	Width = -1;
	time(&Timestamp);
}

NLine::NLine(const char *text, int color, int maxlen) {
	if (maxlen < 0)
		Text = g_strdup(text);
	else
		Text = g_strndup(text,maxlen);
	NBytes = strlen(Text);
	Color  = color;
	Width  = -1;
	time(&Timestamp);
}

NLine::~NLine() {
	if (Text!=NULL)
		free(Text);
}

FLine::FLine() {
	Text = NULL;
	NBytes = 0;
	Color = 0;
	makeStamp();
}

FLine::FLine(const char *text, int color, int maxlen, time_t stamp) {
	if (maxlen < 0)
		Text = g_strdup(text);
	else
		Text = g_strndup(text, maxlen);
	NBytes = strlen(Text);
	Color  = color;
	if (stamp == 0)
		time(&Timestamp);
	else
		Timestamp = stamp;

	makeStamp();
}

FLine::FLine(NLine *src) {
	if (src->Text != NULL)
		Text   = strdup(src->Text);
	else
		Text = NULL;
	NBytes = src->NBytes;
	Color  = src->Color;
	Timestamp = src->Timestamp;
	makeStamp();
}

void FLine::setSource(int src,int off) {
	Src = src;
	Off = off;
}

void FLine::makeStamp() {
	struct tm *lt;
	lt = localtime(&Timestamp);
	snprintf(stamp,11,"[%.2d:%.2d:%.2d] ",lt->tm_hour,lt->tm_min,lt->tm_sec);
}

TPoint::TPoint() {
	LineNum = ByteOffset = 0;
}

TPoint & TPoint::operator=(TPoint &src) {
	LineNum    = src.LineNum;
	ByteOffset = src.ByteOffset;
	return(*this);
}

int     TPoint::operator<(TPoint &src) {
	if (LineNum < src.LineNum) return 1;
	if (LineNum > src.LineNum) return 0;
	if (ByteOffset < src.ByteOffset) return 1;
	return 0;
}

int     TPoint::operator>(TPoint &src) {
	if (LineNum > src.LineNum) return 1;
	if (LineNum < src.LineNum) return 0;
	if (ByteOffset > src.ByteOffset) return 1;
	return 0;
}

int     TPoint::operator==(TPoint &src) {
	return(LineNum == src.LineNum && ByteOffset == src.ByteOffset);
}

int TPoint::operator<(int i)  { return(LineNum<i);  }
int TPoint::operator<=(int i) { return(LineNum<=i); }
int TPoint::operator>(int i)  { return(LineNum>i);  }
int TPoint::operator>=(int i) { return(LineNum>=i); }
int TPoint::operator==(int i) { return(LineNum==i); }

NText::NText() {
	fmtw = 1;
	lw = lh = 0;
	cw = ch = 0;
	leftb = rightb = 1;
	tsskip = -1;
	IgnoreChg = 0;
	WasAtBottom = true;
	MaxLines = 0;
	bgcolor = 0;
	havesel = false;
	dropmup = 0;
	toid = -1;
	lfl  = -1;
	createGui();
}

NText::~NText() {
	int i;
	for(i=0;i<lines.size();i++)
		delete(lines[i]);
	for(i=0;i<xlines.size();i++)
		delete(xlines[i]);
	lines.clear();
	xlines.clear();
	freeOldSelections();
	if (cgc)    gdk_gc_destroy(cgc);
	if (canvas) gdk_pixmap_unref(canvas);
}

void NText::createGui() {
	widget = gtk_hbox_new(FALSE,0);
	body   = gtk_drawing_area_new();

	gtk_widget_set_events(body,GDK_EXPOSURE_MASK|GDK_BUTTON_PRESS_MASK|
						  GDK_BUTTON_RELEASE_MASK|GDK_BUTTON1_MOTION_MASK|GDK_SCROLL_MASK);

	canvas = 0;
	cgc    = 0;
	vsa    = (GtkAdjustment *) gtk_adjustment_new(0.0,
												  0.0,
												  0.0,1.0,10.0,10.0);
	sb     = gtk_vscrollbar_new(vsa);

	gtk_box_pack_start(GTK_BOX(widget), body, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(widget), sb, FALSE, TRUE, 0);

	gtk_signal_connect(GTK_OBJECT(body),"configure_event",
					   GTK_SIGNAL_FUNC(ntext_configure), (gpointer) this);
	gtk_signal_connect(GTK_OBJECT(body),"expose_event",
					   GTK_SIGNAL_FUNC(ntext_expose), (gpointer) this);
	gtk_signal_connect(GTK_OBJECT(vsa),"value_changed",
					   GTK_SIGNAL_FUNC(ntext_sbchange), (gpointer) this);

	gtk_signal_connect(GTK_OBJECT(body),"button_press_event",
					   GTK_SIGNAL_FUNC(ntext_mdown), (gpointer) this);
	gtk_signal_connect(GTK_OBJECT(body),"button_release_event",
					   GTK_SIGNAL_FUNC(ntext_mup), (gpointer) this);
	gtk_signal_connect(GTK_OBJECT(body),"motion_notify_event",
					   GTK_SIGNAL_FUNC(ntext_mdrag), (gpointer) this);
	gtk_signal_connect(GTK_OBJECT(body),"scroll_event",
					   GTK_SIGNAL_FUNC(ntext_scroll), (gpointer) this);

	gtk_signal_connect (GTK_OBJECT(body), "selection_clear_event",
						GTK_SIGNAL_FUNC (ntext_ksel), (gpointer) this);

	gtk_selection_add_target(body,
							 GDK_SELECTION_PRIMARY,
							 GDK_SELECTION_TYPE_STRING, 1);

	gtk_signal_connect (GTK_OBJECT(body), "selection_get",
						GTK_SIGNAL_FUNC (ntext_getsel), (gpointer) this);

	Gtk::show(body,sb,NULL);
}

void NText::repaint() {
	gtk_widget_queue_resize(body);
}

void NText::setFont(const char *f) {
	deque<NLine *>::iterator i;

	L.setFont(0,f);
	fh = 2 + L.fontHeight(0);
	fmtw = -1; /* invalidate current wrapping */

	// invalidate cached line widths
	for(i=lines.begin();i!=lines.end();i++)
		(*i)->Width = -1;
}

void NText::setBG(int c) {
	bgcolor = c;
	if (canvas)
		repaint();
}

void NText::append(const char *text, int len, int color) {
	int i;
	NLine *nl;
	char *p;
	char *s;

	if (len < 0) {
		discardExcess();
		return;
	}

	s = strdup(text);
	assert(s !=  NULL);
	p = strchr(s, '\n');
	if (p!=NULL) {
		*p = 0;
		i = strlen(s);
		nl = new NLine(s, color);
		*p = '\n';
		lines.push_back(nl);
		formatLine(lines.size()-1);
		append(&p[1], len-(i+1), color);
		return;
	}
	free (s);

	// if search for \n failed, this is a single line
	nl = new NLine(text, color);
	lines.push_back(nl);
	formatLine(lines.size()-1);
	discardExcess();
	//fmtw = -1;
}

void NText::formatBuffer() {
	int i;
	//  Timestamp a,b,c;

	//  a = Timestamp::now();

	for(i=0;i<xlines.size();i++)
		delete(xlines[i]);
	xlines.clear();

	//  b = Timestamp::now();

	tsskip = L.stringWidth(0,"[88:88:88] ");

	for(i=0;i<lines.size();i++)
		formatLine(i);
	fmtw = lw;

	//  c = Timestamp::now();
	//  cerr << "AB: " << b-a << "   BC: " << c-b << "  AC: " << c-a << endl;
}

void NText::formatLine(unsigned int i) {
	int j,k,l,w,color;
	bool fit = false;
	NLine *sl;
	FLine *fl;
	const char *tp;
	int elw;


	if (lines.size() <= i || canvas==NULL) // !font
		return;

	sl = lines[i];

	elw = lw;
	if (global.ShowTimestamp)
		elw -= tsskip;

	tp = sl->Text;
	k = sl->NBytes;
	j = 0;
	color = sl->Color;

	// empty line
	if (k==0) {
		fl = new FLine(sl);
		fl->setSource(i,0);
		xlines.push_back(fl);
		return;
	}

	while(k-j > 0) {
		fit = false;

		// try full-fit for for unwrapped of last chunk of wrapping

		if (j==0 && sl->Width >= 0) {
			w = sl->Width;
		} else {
			if (!g_utf8_validate(tp+j,k-j,NULL)) continue;
			w = L.substringWidth(0,tp+j,k-j);
			if (j==0) sl->Width = w;
		}
		if (w <= elw) {
			fl = new FLine(tp+j, color, -1, sl->Timestamp);
			fl->setSource(i,j);
			xlines.push_back(fl);
			return;
		}

		for(l=k-1;l>=j;l--)
			if (tp[l] == ' ' || tp[l] == '\t') {
				if (!g_utf8_validate(tp+j,k-j,NULL)) continue;
				w = L.substringWidth(0,tp+j,l-j);
				if (w <= elw) {
					fl = new FLine(tp+j,color,l-j, sl->Timestamp);
					fl->setSource(i,j);
					xlines.push_back(fl);
					j = l+1;
					fit = true;
					break;
				}
			}

		// no blanks to wrap ?
		if (l<j && !fit) {
			fl = new FLine(tp+j,color,-1,sl->Timestamp);
			fl->setSource(i,j);
			xlines.push_back(fl);
			j = k;
		}
	}

}

// topline:   index of line rendered at the top of the widget
// linecount: number of lines in the buffer
// nlines:    how many lines can be fit in the current widget
void NText::setScroll(int topline, int linecount, int nlines) {
	GtkAdjustment prev;
	memcpy(&prev,vsa,sizeof(GtkAdjustment));

	//  printf("this=%x setScroll topline=%d linecount=%d nlines=%d\n",
	//     (int)this,topline,linecount,nlines);

	// all text fits in the widget
	if (nlines >= linecount) {
		vsa->value = 0.0;
		vsa->lower = 0.0;
		vsa->upper = nlines;
		vsa->step_increment = 1.0;
		vsa->page_increment = nlines;
		vsa->page_size      = nlines;
	} else {
		vsa->value = topline;
		vsa->lower = 0.0;
		vsa->upper = linecount;
		vsa->step_increment = 1.0;
		vsa->page_increment = nlines;
		vsa->page_size      = nlines;
	}

	if (memcmp(&prev,vsa,sizeof(GtkAdjustment))!=0) {
		++IgnoreChg;
		gtk_adjustment_changed(vsa);
		gtk_adjustment_value_changed(vsa);
	}
}

void NText::pageUp(float pages) {
	int nval, j, disp;
	j = xlines.size() - (int)(vsa->page_size);
	disp = (int) (pages*vsa->page_size);
	nval = (int) (vsa->value - disp);

	//  printf("this=%x pageUp disp=%d nval=%d j=%d\n",
	//          (int)this, disp, nval, j);

	if (nval < 0) nval = 0;
	if (nval > j) nval = j;
	WasAtBottom = false;
	setScroll(nval, xlines.size(), (int) (vsa->page_size));
	repaint();
}

void NText::pageDown(float pages) {
	int nval, j, disp;
	j = xlines.size() - (int)(vsa->page_size);
	disp = (int) (pages*vsa->page_size);
	nval = (int) (vsa->value + disp);

	// printf("this=%x pageDown disp=%d nval=%d j=%d\n",
	//  (int)this, disp, nval, j);

	if (nval < 0) nval = 0;
	if (nval > j) nval = j;
	WasAtBottom = false;
	setScroll(nval, xlines.size(), (int) (vsa->page_size));
	repaint();
}

void NText::lineUp(int n) {
	int nval, j;
	j = xlines.size() - (int)(vsa->page_size);
	nval = (int) (vsa->value - n);
	if (nval < 0) nval = 0;
	if (nval > j) nval = j;
	WasAtBottom = false;
	setScroll(nval, xlines.size(), (int) (vsa->page_size));
	repaint();
}

void NText::lineDown(int n) {
	int nval, j;
	j = xlines.size() - (int)(vsa->page_size);
	nval = (int) (vsa->value + n);
	if (nval < 0) nval = 0;
	if (nval > j) nval = j;
	WasAtBottom = false;
	setScroll(nval, xlines.size(), (int) (vsa->page_size));
	repaint();
}

void NText::setScrollBack(int n) {
	if (n != (signed)MaxLines ) {
		if (n < 0) n=0;
		MaxLines = (unsigned) n;
		discardExcess(true);
	}
}

void NText::discardExcess(bool _repaint) {
	if (MaxLines != 0)
		if (lines.size() > MaxLines) {
			discardLines(lines.size() - MaxLines);
			if (_repaint) repaint();
		}
}

void NText::discardLines(int n) {
	int i,top;

	if (n > (signed) lines.size()) n = lines.size();
	if (!n) return;

	if (!xlines.size()) return;

	top  = (int) (vsa->value);
	top  = xlines[top]->Src;
	top -= n;

	// fix selection
	if (havesel) {
		A.LineNum -= n;
		B.LineNum -= n;

		if (A.LineNum < 0 || B.LineNum < 0) {
			havesel = false;
			gtk_selection_owner_set(NULL,GDK_SELECTION_PRIMARY,time(0));
		}
	}

	// pop entries from main line index
	for(i=0;i<n;i++) {
		delete(lines.front());
		lines.pop_front();
	}

	// pop entries from formatted index
	while(xlines.front()->Src < n) {
		delete(xlines.front());
		xlines.pop_front();
	}

	deque<FLine *>::iterator j;
	for(j=xlines.begin();j!=xlines.end();j++)
		(*j)->Src -= n;

	// fix scroll values
	if (top < 0) top = 0;
	for(i=0;i<xlines.size();i++)
		if (xlines[i]->Src == top) {
			top = i;
			break;
		}

	if (top < 0) top = 0;
	setScroll(top, xlines.size(), (int) (vsa->page_size));

	// does NOT repaint the widget, do it yourself
}

void NText::gotoLine(int n) {
	int i,j,xn;

	j = xlines.size();
	xn = -1;
	for(i=0;i<j;i++)
		if (xlines[i]->Src == n) {
			xn = i;
			break;
		}
	if (xn < 0)
		return;

	if (xn < ((int)(vsa->value))  ||
		xn >= ((int)(vsa->value+vsa->page_size))) {

		WasAtBottom = false;
		setScroll(xn, xlines.size(), (int) (vsa->page_size));
		scheduleRepaint(50);

	}
}


bool NText::findTextUpward(int top, int bottom, const char *needle,
						   bool select)
{
	int a,b,i;

	a=0; b=lines.size()-1;
	if (top     >= 0 && top     <= b) a=top;
	if (bottom  >= 0 && bottom  <= b) b=bottom;
	for(i=b;i>=a;i--)
		if (matchTextInLine(i,needle,select)) {
			lfl = i;
			return true;
		}
	return false;
}

int NText::getLastFoundLine() { return lfl; }

bool NText::matchTextInLine(int i, const char *needle, bool select) {
	int j;
	char *q;

	j = lines.size();
	if (i>=j || i<0) return false;
	if (lines[i]->Text == NULL) return false;

	q = strstr(lines[i]->Text, needle);
	if (q!=NULL && select) {
		j = q-lines[i]->Text;
		selectRegion(i,j,i,j+strlen(needle)-1);
	}

	return(q!=NULL);
}

void NText::selectRegion(int startline, int startoff, int endline, int endoff) {
	int i,j;

	A.LineNum    = startline;
	A.ByteOffset = startoff;

	B.LineNum    = endline;
	B.ByteOffset = endoff;

	havesel = true;
	if (body != NULL) {
		if (!GTK_WIDGET_REALIZED(body))
			gtk_widget_realize(body);
		gtk_selection_owner_set(body, GDK_SELECTION_PRIMARY,time(0));
	}
	scheduleRepaint();
}

bool NText::saveTextBuffer(const char *path) {
	int i;

	ofstream txt(path);

	if (!txt)
		return false;

	for(i=0;i<lines.size();i++) {
		txt.write(lines[i]->Text, lines[i]->NBytes);
		txt << endl;

		if (!txt) {
			txt.close();
			return false;
		}
	}

	txt.close();
	return true;
}

gboolean ntext_expose(GtkWidget *widget, GdkEventExpose *ee,
					  gpointer data)
{
	NText *me = (NText *) data;

	if (me->canvas == NULL) return FALSE;
	gdk_draw_pixmap(widget->window,widget->style->black_gc,
					me->canvas,
					ee->area.x, ee->area.y,
					ee->area.x, ee->area.y,
					ee->area.width, ee->area.height);
	return TRUE;
}

gboolean ntext_configure(GtkWidget *widget, GdkEventConfigure *ee,
						 gpointer data)
{
	NText *me = (NText *) data;
	int i, j, ww, wh, fh, lb, ri;
	int nl, tl, tb, ofh, tk;
	GdkGC *gc;
	GdkPixmap *g;
	LayoutBox *L;

	bool havesel;

	if (widget->window == NULL)
		return FALSE;

	gdk_window_get_size(widget->window, &ww, &wh);
	L = &(me->L);

	me->lw = ww - (me->leftb + me->rightb);
	me->lh = wh;

	if (me->canvas == NULL) {
		me->canvas = gdk_pixmap_new(widget->window, ww, wh, -1);
		me->cw = ww;
		me->ch = wh;
		me->cgc = gdk_gc_new(me->canvas);
		L->prepare(me->widget,me->canvas,me->cgc,0,0,ww,wh);
		L->setFont(0,global.ConsoleFont);

		if (me->canvas == NULL)
			cerr << "bug1\n";
	} else {
		if (ww > me->cw || wh > me->ch) {
			gdk_pixmap_unref(me->canvas);
			if (me->cgc) gdk_gc_destroy(me->cgc);
			me->canvas = gdk_pixmap_new(widget->window, ww, wh, -1);
			me->cw = ww;
			me->ch = wh;
			me->cgc = gdk_gc_new(me->canvas);
			L->prepare(me->widget,me->canvas,me->cgc,0,0,ww,wh);
			L->setFont(0,global.ConsoleFont);

			if (me->canvas == NULL)
				cerr << "bug2\n";
		}
	}

	ofh = me->fh;
	me->fh = L->fontHeight(0) + 2;

	if (me->fmtw != me->lw || me->fh != ofh)
		me->formatBuffer();

	gc = me->cgc;
	g  = me->canvas;
	fh = me->fh;
	lb = me->leftb;
	havesel = me->havesel;

	tb = wh % fh;
	if (tb > 2) tb-=2;

	L->setColor(me->bgcolor);
	L->drawRect(0,0,ww,wh,true);

	/* scrollbar calc */
	j = me->xlines.size();
	nl = wh / me->fh;

	if (nl >= j)
		tl = 0;
	else
		tl = (int) (me->vsa->value);

	// keep autoscrolling if seeing th bottom of the buffer
	if (me->WasAtBottom) {
		tl = j-nl;
		if (tl < 0) tl=0;
	}

	me->setScroll(tl,j,nl);
	if (!tl) tb=0;

	for(i=0;i<j;i++)
		me->xlines[i]->valid = false;

	TPoint a,b,c,d;
	FLine *fl;
	int o1,o2,pw,ox;
	char *pc,*nc;

	if (havesel)
		if (me->A < me->B) {
			a=me->A; b=me->B;
		} else {
			a=me->B; b=me->A;
		}

	tk = global.ShowTimestamp ? me->tsskip : 0;

	for(i=0;i<nl;i++) {
		ri = i+tl;
		if (ri >= j)
			break;

		fl = me->xlines[ri];

		if (tk != 0) {
			L->setColor(global.Colors.TextDefault);
			L->drawString(lb,tb+fh*i,0,fl->stamp);
		}

		L->setColor(fl->Color);

		if (!havesel) {
			L->drawSubstring(lb+tk,tb+fh*i,0,fl->Text,fl->NBytes);
		} else {

			c.LineNum = fl->Src;
			c.ByteOffset = fl->Off;

			d.LineNum = fl->Src;
			d.ByteOffset = fl->Off + fl->NBytes - 1;

			if (b < c || d < a)
				L->drawSubstring(lb+tk,tb+fh*i,0,fl->Text,fl->NBytes);
			else {

				o1 = 0;
				if (c < a) o1 = a.ByteOffset - fl->Off;

				o2 = fl->NBytes - 1;
				if (b < d) o2 = b.ByteOffset - fl->Off;

				nc = fl->Text+o2;
				pc = g_utf8_next_char(nc);
				o2 += (pc-nc-1);

				ox = 0;
				if (o1 > 0) {
					ox = L->substringWidth(0,fl->Text,o1);
					L->setColor(fl->Color);
					L->drawSubstring(lb+tk,tb+fh*i,0,fl->Text,o1);
				}

				pw = L->substringWidth(0,fl->Text+o1,o2-o1+1);
				L->setColor(fl->Color);
				L->drawRect(lb+ox+tk,tb+fh*i,pw,fh,true);
				L->setColor(me->bgcolor);
				L->drawSubstring(lb+ox+tk,tb+fh*i,0,fl->Text+o1,o2-o1+1);

				ox += pw;
				L->setColor(fl->Color);

				if (o2+1 != fl->NBytes)
					L->drawString(lb+ox+tk,tb+fh*i,0,fl->Text+o2+1);
			}
		}

		fl->valid = true;
		fl->X = lb+tk;
		fl->Y = tb+fh*i;
		fl->H = fh;
	}

	me->WasAtBottom = (i+tl == j);
	return TRUE;
}

void ntext_sbchange(GtkAdjustment *adj, gpointer data) {
	NText *me = (NText *) data;
	if (me->IgnoreChg)
		me->IgnoreChg--;
	else {
		me->WasAtBottom = false;
		me->repaint();
	}
}

void NText::freeOldSelections() {
	char *x;
	while(!prevsel.empty()) {
		x = prevsel.top();
		free(x);
		prevsel.pop();
	}
}

bool NText::calcTP(TPoint &t, int x,int y) {
	int i,j,lastvalid;
	int l,r; /* binary search limits of length */
	int p,q;

	char *fc, *nc, *pc;

	if (!L.prepared())
		return false;

	lastvalid = -1;
	for(i=0;i<xlines.size();i++)
		if (xlines[i]->valid) {
			lastvalid = i;
			if (y>=xlines[i]->Y && y<(xlines[i]->Y+xlines[i]->H)) {

				t.LineNum = xlines[i]->Src;

				if (x <= xlines[i]->X) {
					t.ByteOffset = xlines[i]->Off;
					return true;
				}

				fc = xlines[i]->Text;
				nc = fc;
				p=q=0;
				do {
					pc = g_utf8_next_char(nc);
					t.ByteOffset = (nc-fc) + xlines[i]->Off;
					q = L.substringWidth(0,fc,pc-fc);
					q += xlines[i]->X;
					nc = pc;
				} while(nc[0]!=0 && q < x);

				return true;
			}
		}

	// set point at the end of text when the user clicks past the end */
	if (lastvalid >= 0) {
		t.LineNum = xlines[lastvalid]->Src;
		t.ByteOffset = xlines[lastvalid]->NBytes - 1;
		return true;
	}
	return false; /* no match (no visible lines at all ?!?) */
}

/* scroll wheel for GTK 2 */
gboolean ntext_scroll(GtkWidget *widget, GdkEventScroll *es, gpointer data) {
	NText *me = (NText *) data;
	if (es->direction == GDK_SCROLL_UP) me->lineUp(1);
	if (es->direction == GDK_SCROLL_DOWN) me->lineDown(1);
	return TRUE;
}

/*
  1. clears the selection
  2. prepares A end point
*/
gboolean ntext_mdown(GtkWidget *widget, GdkEventButton *eb,
					 gpointer data)
{
	NText *me = (NText *) data;
	TPoint c;
	char *tl,*l,*r,*ml,*mr;

	if (!eb) return FALSE;

	if (eb->button == 1) {

		switch(eb->type) {
		case GDK_2BUTTON_PRESS: // select word
			if (me->calcTP(c, (int)(eb->x), (int)(eb->y))) {
				tl = me->lines[c.LineNum]->Text;
				if (isspace(tl[c.ByteOffset]))
					return FALSE;
				me->A = c;
				me->B = c;

				l = r = &tl[c.ByteOffset];
				ml = tl;
				mr = &tl[ me->lines[c.LineNum]->NBytes - 1];

				// walk left
				while(l>=ml && !isspace(*l))
					l = g_utf8_prev_char(l);
				if (l!=ml)
					l = g_utf8_next_char(l);

				// walk right
				while(r<=mr && !isspace(*r))
					r = g_utf8_next_char(r);
				if (r!=mr)
					r = g_utf8_prev_char(r);

				me->A.ByteOffset = l - tl;
				me->B.ByteOffset = r - tl;
				me->havesel = true;
				me->dropmup++;
				gtk_selection_owner_set(me->body, GDK_SELECTION_PRIMARY,eb->time);
				me->scheduleRepaint();
			}
			break;
		case GDK_3BUTTON_PRESS: // select whole line
			if (me->calcTP(c, (int)(eb->x), (int)(eb->y))) {
				me->havesel = true;
				me->A = c;
				me->B = c;
				me->A.ByteOffset = 0;
				me->B.ByteOffset = me->lines[c.LineNum]->NBytes - 1;
				me->dropmup++;
				gtk_selection_owner_set(me->body, GDK_SELECTION_PRIMARY,eb->time);
				me->scheduleRepaint();
			}
			break;
		default:
			me->havesel = me->calcTP(me->A, (int)(eb->x), (int)(eb->y));
			if (me->havesel) {
				me->B = me->A;
				gtk_selection_owner_set(me->body, GDK_SELECTION_PRIMARY,eb->time);
			}
		}
	}
	return TRUE;
}

/*
  1. prepare B end point
  2. if A==B (offset-wise), clear selection
*/
gboolean ntext_mup(GtkWidget *widget, GdkEventButton *eb,
				   gpointer data)
{
	NText *me = (NText *) data;
	TPoint c;
	bool   dirty = false;

	if (!eb) return FALSE;

	if (eb->button == 1) {
		if (!me->havesel) return FALSE;
		if (me->dropmup) {
			me->dropmup--;
			return FALSE;
		}
		if (me->calcTP(c, (int) (eb->x), (int) (eb->y))) {
			me->B = c;
			dirty = true;
		}
		if (me->A == me->B) {
			me->havesel = false;
			dirty = true;
		}
		if (dirty)
			me->repaint();
		if (me->havesel)
			gtk_selection_owner_set(me->body, GDK_SELECTION_PRIMARY,eb->time);
	}
	return TRUE;
}

/*
  what it does:
  1. prepares B end point
*/
gboolean ntext_mdrag(GtkWidget *widget, GdkEventMotion *em,
					 gpointer data)
{
	NText *me = (NText *) data;
	TPoint c;
	int x,y;

	if (!em) return FALSE;

	if (!me->havesel) return FALSE;
	if (em->state & GDK_BUTTON1_MASK) {

		x = (int)(em->x);
		y = (int)(em->y);
		if (y > me->ch) {
			me->lineDown(1);
			return TRUE;
		}
		if (y < 0) {
			me->lineUp(1);
			return TRUE;
		}
		if (me->calcTP(c, x, y)) {
			me->B = c;
			me->scheduleRepaint();
		}
	}
	return TRUE;
}

gboolean ntext_ksel(GtkWidget * widget,
					GdkEventSelection * event, gpointer data)
{
	NText *me = (NText *) data;
	me->havesel = false;
	me->scheduleRepaint();
	me->freeOldSelections();
	return TRUE;
}

void     ntext_getsel(GtkWidget * widget,
					  GtkSelectionData * seldata,
					  guint info, guint time, gpointer data)
{
	NText *me = (NText *) data;
	int i,sz;
	char *txt,*p,*q;
	TPoint a,b;

	if (me->havesel) {
		if (me->A < me->B) {
			a=me->A; b=me->B;
		} else {
			a=me->B; b=me->A;
		}

		if (a.LineNum == b.LineNum) {

			sz = b.ByteOffset - a.ByteOffset + 2;

		} else {

			sz  = me->lines[a.LineNum]->NBytes - a.ByteOffset;
			sz += b.ByteOffset + 1;
			sz++; // \n from first line

			for(i=a.LineNum+1;i<b.LineNum;i++)
				sz += me->lines[i]->NBytes + 1;
			sz++; // nul terminator
		}

		// account for a long UTF-8 sequence at TPoint b
		p = me->lines[b.LineNum]->Text + b.ByteOffset;
		q = g_utf8_next_char(p);
		sz += q-p-1;

		txt = (char*) malloc(sz);
		memset(txt,0,sz);

		if (a.LineNum == b.LineNum) {

			g_strlcpy(txt,me->lines[a.LineNum]->Text + a.ByteOffset,sz);

		} else {

			g_strlcpy(txt,me->lines[a.LineNum]->Text + a.ByteOffset,sz);
			g_strlcat(txt,"\n",sz);

			for(i=a.LineNum+1;i<b.LineNum;i++) {
				g_strlcat(txt,me->lines[i]->Text,sz);
				g_strlcat(txt,"\n",sz);
			}

			g_strlcat(txt,me->lines[b.LineNum]->Text,sz);
		}

		gtk_selection_data_set_text(seldata, txt, -1);
		me->freeOldSelections();
		me->prevsel.push(txt);
	}

}

gboolean ntext_redraw(gpointer data) {
	NText *me = (NText *) data;
	me->toid = -1;
	if (me->canvas)
		me->repaint();
	return FALSE;
}

void NText::scheduleRepaint(int latency) {
	if (toid < 0)
		toid = gtk_timeout_add(latency,ntext_redraw,(gpointer) this);
}
