/* $Id: notebook.cc,v 1.31 2007/05/23 18:28:17 bergo Exp $ */

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
#include "eboard.h"
#include "global.h"
#include "notebook.h"

#include "dead.xpm"
#include "trash2.xpm"

void page_trash(GtkWidget *w, gpointer data) {
  Page *p;
  p=(Page *) data; 
  global.removeRemovablePage(p->number);
}

Page::Page() {
  global.debug("Page","Page","1");
  number=0;
  content=0;
  title[0]=0;
  naughty=false;
  level=IM_ZERO;
  label=gtk_label_new("<none>");
  gshow(label);
}

Page::Page(int _number,GtkWidget *_content, char *_title, bool _removable) {
  
  GtkWidget *b,*p;

  global.debug("Page","Page","2");
  number=_number;
  content=_content;
  level=IM_ZERO;
  naughty=false;
  removable=_removable;

  g_strlcpy(title,_title,256);

  if (removable) {

    labelframe=gtk_hbox_new(FALSE,1);

    Notebook::ensurePixmaps();
    b=gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(b), GTK_RELIEF_HALF);
    p=gtk_pixmap_new(Notebook::trashface, Notebook::trashmask);
    gtk_container_add(GTK_CONTAINER(b), p);
    gtk_box_pack_start(GTK_BOX(labelframe), b, FALSE, FALSE, 0);

    label=gtk_label_new(title);
    gtk_box_pack_start(GTK_BOX(labelframe), label, FALSE, TRUE, 1);

    Gtk::show(p,b,label,labelframe,NULL);

    gtk_signal_connect(GTK_OBJECT(b),"clicked",
		       GTK_SIGNAL_FUNC(page_trash), (gpointer) this);

  } else {

    label=gtk_label_new(title);
    gshow(label);
    labelframe=label;

  }
}

int Page::operator==(int n) {
  return(n==number);
}

int Page::under(Importance imp) {
  return(level < imp);
}

int Page::above(Importance imp) {
  return(level > imp);
}

void Page::setLevel(Importance imp) {
  level=imp;
}

void Page::setTitle(char *_title) {
  g_strlcpy(title,_title,256);
  gtk_label_set_text(GTK_LABEL(label),title);
  gtk_widget_queue_resize(label);
}

char * Page::getTitle() {
  return(title);
}

void Page::renumber(int newid) {
  char z[258];
  number=newid;
  snprintf(z,258,"{%s}",title);
  setTitle(z);
}

void Page::dump() {
  cerr.setf(ios::hex,ios::basefield);
  cerr.setf(ios::showbase);
  cerr << "[page " << ((uint64_t)this) << "] ";
  cerr.setf(ios::dec,ios::basefield);
  cerr << "pageid=" << number << " ";
  cerr << "title=" << title << " ";
  cerr << "level=" << ((int)level) << endl;
}

GdkPixmap * Notebook::trashface=0;
GdkBitmap * Notebook::trashmask=0;
GtkWidget * Notebook::swidget=0;

void Notebook::ensurePixmaps() {
  GtkStyle *style;

  if (swidget==0) return;

  if (trashface==0) {
    style=gtk_widget_get_style(swidget);
    trashface = gdk_pixmap_create_from_xpm_d (swidget->window, &trashmask,
					      &style->bg[GTK_STATE_NORMAL],
					      (gchar **) trash2_xpm);
  }
}

Notebook::Notebook() {
  global.debug("Notebook","Notebook");
  pcl=0;
  widget=gtk_notebook_new();
  swidget=widget;
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget),GTK_POS_RIGHT);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(widget),TRUE);
  gtk_signal_connect(GTK_OBJECT(widget),"switch_page",
		     GTK_SIGNAL_FUNC(notebook_switch_page),
		     (gpointer)this);
}

void Notebook::addPage(GtkWidget *what,char *title,int id,bool removable) {
  Page *pg;
  global.debug("Notebook","addPage",title);
  pg=new Page(id,what,title,removable);
  pages.push_back(pg);  
  gtk_notebook_append_page(GTK_NOTEBOOK(widget),what,pg->labelframe);

  /* to test graphics only, adds dead.xpm to console tab
  if (pages.size() == 2)
    addBombToTab(pg);
  */
}

void Notebook::removePage(int id) {
  vector<Page *>::iterator pi;
  int i;
  global.debug("Notebook","removePage");
  for(i=0,pi=pages.begin();pi!=pages.end();pi++,i++)
    if ( (*(*pi)) == id ) {
      gtk_notebook_remove_page(GTK_NOTEBOOK(widget),i);
      delete(*pi);
      pages.erase(pi);
      return;
    }  
}

unsigned int Notebook::getCurrentPage() {
  return((unsigned int)gtk_notebook_get_current_page(GTK_NOTEBOOK(widget)));
}

int Notebook::getCurrentPageId() {
  unsigned int i;
  i=getCurrentPage();
  if (i >= pages.size())
    return 0;
  return( pages[i]->number );
}

void Notebook::setTabColor(int pageid,int color,Importance imp) {
  vector<Page *>::iterator pi;
  int i;
  global.debug("Notebook","setTabColor","1");
  for(i=0,pi=pages.begin();pi!=pages.end();pi++,i++)
    if ( (*(*pi)) == pageid ) {
      setTabColor(i,color,0,imp);
      return;
    }
}

void Notebook::setTabColor(int page_num,int color,int poly,
			   Importance imp) {
  vector<Page *>::iterator pi;
  GdkColor nc;
  GtkStyle *style;
  int i;
  i=page_num;

  global.debug("Notebook","setTabColor","2");
  for(pi=pages.begin();i;pi++,i--) ;

  if ((*pi)->above(imp))
    return;

  if (imp==IM_RESET) imp=IM_ZERO;
  (*pi)->setLevel(imp);

  nc.red=((color>>16)<<8);
  nc.green=(color&0x00ff00);
  nc.blue=((color&0xff)<<8);
  style=gtk_style_copy( gtk_widget_get_style((*pi)->label) );

  for(i=0;i<5;i++)
    style->fg[i]=nc;

  gtk_widget_set_style( (*pi)->label, style );
  gtk_widget_ensure_style( (*pi)->label );
  gtk_widget_queue_draw( (*pi)->label );

  gtk_style_unref(style); // tomv, leak
}

void Notebook::goToPageId(int id) {
  vector<Page *>::iterator pi;
  int i;
  for(i=0,pi=pages.begin();pi!=pages.end();pi++,i++)
    if ( (*(*pi)) == id ) {
      gtk_notebook_set_page(GTK_NOTEBOOK(widget),i);
      return;
    }
}

void Notebook::setTabPosition(GtkPositionType pos) {
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget),pos);
}

void Notebook::dump() {
  vector<Page *>::iterator pi;
  for(pi=pages.begin();pi!=pages.end();pi++)
    (*pi)->dump();
}

void Notebook::goToNext() {
  gtk_notebook_next_page(GTK_NOTEBOOK(widget));
}

void Notebook::goToPrevious() {
  gtk_notebook_prev_page(GTK_NOTEBOOK(widget));
}

void Notebook::renumberPage(int oldid,int newid) {
  vector<Page *>::iterator i;
  for(i=pages.begin();i!=pages.end();i++)
    if ( (*(*i)) == oldid )
      (*i)->renumber(newid);
}

void Notebook::setListener(PaneChangeListener *listener) {
  pcl=listener;
}

void Notebook::setNaughty(int pageid, bool val) {
  vector<Page *>::iterator i;
  for(i=pages.begin();i!=pages.end();i++)
    if ( (*(*i)) == pageid ) {

      if ( (val)&&(! (*i)->naughty) )
	addBombToTab(*i);
      (*i)->naughty=val;

    }
}

void Notebook::getNaughty(vector<int> &evil_ids) {
  vector<Page *>::iterator i;
  evil_ids.clear();
  for(i=pages.begin();i!=pages.end();i++)
    if ( (*i)->naughty )
      evil_ids.push_back( (*i)->number );
}

bool Notebook::isNaughty(int pageid) {
  vector<Page *>::iterator i;
  for(i=pages.begin();i!=pages.end();i++)
    if ( (*i)->number == pageid )
      if ((*i)->naughty)
	return true;
  return false;
}

bool Notebook::hasNaughty() {
  vector<Page *>::iterator i;
  for(i=pages.begin();i!=pages.end();i++)
    if ( (*i)->naughty )
      return true;
  return false;
}

// SOMEONE SET UP US THE BOMB!
void Notebook::addBombToTab(Page *pg) {
  GtkWidget *hb,*label,*pm;
  GdkPixmap *d;
  GdkBitmap *b;
  GtkStyle *style;

  style=gtk_widget_get_default_style();

  d = gdk_pixmap_create_from_xpm_d (widget->window, &b,
				    &style->bg[GTK_STATE_NORMAL],
				    (gchar **) dead_xpm);
  pm=gtk_pixmap_new(d,b);

  label=gtk_label_new(pg->getTitle());
  hb=gtk_hbox_new(FALSE,2);
  
  gtk_box_pack_start(GTK_BOX(hb),label,FALSE,FALSE,4);
  gtk_box_pack_start(GTK_BOX(hb),pm,FALSE,FALSE,4);

  Gtk::show(label,pm,hb,NULL);
  
  pg->label=label;
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(widget),pg->content,hb);
}

void Notebook::pretendPaneChanged() {
  if (pcl)
    pcl->paneChanged( getCurrentPage(), getCurrentPageId() );
}

void
notebook_switch_page(GtkNotebook *notebook,
		     GtkNotebookPage *page,
		     gint page_num,
		     gpointer data) {
  Notebook *me;
  unsigned int pg = (unsigned int) page_num;
  me=(Notebook *)data;
  me->setTabColor(pg,0,0,IM_RESET);
  if ( (me->pcl) && (me->pages.size() > pg) )
    me->pcl->paneChanged(pg, me->pages[pg]->number);
}

// ================================================================

NotebookInsider::NotebookInsider() {
  mynotebook=0;
  pgid=0;
}

void NotebookInsider::setNotebook(Notebook *nb,int pageid) {
  pgid=pageid;
  mynotebook=nb;
}

void NotebookInsider::contentUpdated() {
  int color=0xff0000;
  Importance maxi=IM_NORMAL;
  global.debug("NotebookInsider","contentUpdated");
  if (!impstack.empty()) {
    maxi=IM_IGNORE;
    while(!impstack.empty()) {
      if (impstack.top() > maxi) maxi=impstack.top();
      impstack.pop();
    }
    switch(maxi) {
    case IM_IGNORE:
      return;
    case IM_NORMAL:
      color=0xff0000;
      break;
    case IM_PERSONAL:
      color=0x0000ff;
      break;
    default:
      color=0x00ff00; /* will never happen */
      break;
    }
  }
  if (!mynotebook)
    return;
  if (mynotebook->getCurrentPageId() != pgid)
    mynotebook->setTabColor(pgid,color,maxi);
}

void NotebookInsider::pop() {
  global.debug("NotebookInsider","pop");
  if (!mynotebook)
    return;
  mynotebook->goToPageId(pgid);
}

void NotebookInsider::pushLevel(Importance imp) {
  impstack.push(imp);
}

int NotebookInsider::getPageId() {
  return(pgid);
}
