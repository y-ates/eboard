/* $Id: langs.cc,v 1.4 2007/01/20 15:58:43 bergo Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "langs.h"
#include "tstring.h"

TEntry::TEntry(const char *K, const char *D) {
	int i,j;

	key = 0;
	data = 0;

	i = strlen(K);
	j = strlen(D);
	key  = (char *) malloc(i + 1);
	data = (char *) malloc(j + 1);

	if (!key || !data) {
		cerr << "langs.cc: malloc error\n";
		return;
	}

	strcpy(key, K);
	strcpy(data, D);
}

TEntry::~TEntry() {
	if (data) free(data);
	if (key)  free(key);
}

bool  TEntry::match(const char *testkey) {
	return(strcmp(key, testkey)==0);
}

int   TEntry::calcHash() {
	return(TEntry::hashOf(key));
}

char * TEntry::getData() {
	return(data);
}

int TEntry::hashOf(const char *x) {
	int i,j,k;
	j = strlen(x);
	k = 0;
	for(i=0;i<j;i++)
		k += ((int) x[i]) * (1 + (i%4));
	return(k%128);
}

Translator::Translator() {
	ready = false;
}

Translator::~Translator() {
	int i;
	list<TEntry *>::iterator li;

	for(i=0;i<128;i++) {
		for(li=dict[i].begin();li!=dict[i].end();li++)
			delete(*li);
		dict[i].clear();
	}
	ready = false;
}

void Translator::setContext(const char *language,
							const char *package,
							const char *searchpath)
{
	tstring t;
	string *s;
	FILE *f;
	char fname[64], fname2[64], dicfile[512];

	if (language == 0)
		guessLanguage();
	else
		setLanguage(language);

	if (strlen(package) > 31)
		return;
	strcpy(Package, package);

	snprintf(fname2,64,"%s.%s.dict",Package,Lang);
	if (SubLang[0] != 0)
		snprintf(fname,64,"%s.%s_%s.dict",Package,Lang,SubLang);
	else
		strcpy(fname, fname2);

	t.set(searchpath);

	while((s=t.token(":"))!=0) {
		snprintf(dicfile,512,"%s/%s",s->c_str(), fname);
		f = fopen(dicfile,"r");
		if (f==0) {
			snprintf(dicfile,512,"%s/%s",s->c_str(), fname2);
			f = fopen(dicfile,"r");
		}
		if (f!=0) {
			ready = loadDictionary(f);
			fclose(f);
			return;
		}
	}
}

void Translator::guessLanguage() {
	char *lang;
	lang = getenv("LC_MESSAGES");
	if (!lang) lang = getenv("LC_ALL");
	if (!lang) lang = getenv("LANGUAGE");
	if (!lang) lang = getenv("LANG");
	if (!lang) lang = "C";
	setLanguage(lang);
}

void Translator::setLanguage(const char *locale) {
	if (strlen(locale) >= 2) {
		Lang[0] = locale[0];
		Lang[1] = locale[1];
		Lang[2] = 0;
	} else
		strcpy(Lang, locale);

	SubLang[0]=0;
	if (strlen(locale) >= 5)
		if (locale[2] == '_') {
			SubLang[0] = locale[3];
			SubLang[1] = locale[4];
			SubLang[2] = 0;
		}
}

bool Translator::loadDictionary(FILE *f) {
	TEntry *e;
	char *tmpk = 0, *tmpd = 0;
	int tmpsz;
	int i,j,k;
	int m;
	char c;

	tmpsz = 4096;
	tmpk = (char *) malloc(tmpsz);
	tmpd = (char *) malloc(tmpsz);
	if (!tmpk || !tmpd) return false;

	for(;;) {
		c = fgetc(f);
		if (c!='L') break;

		k = 0;
		i = 0;
		j = 0;
		for(;;) {
			c = fgetc(f);
			if (c == ' ') { ++k; continue; }
			if (c == '\n') { break; }
			if (c >= '0' && c <= '9') {
				if (k==0)
					i = 10*i + (c-'0');
				else
					j = 10*j + (c-'0');
			}
		}

		if (i==0 || j==0) return false;

		if ( (i > (tmpsz-1)) || (j > (tmpsz-1)) ) {
			tmpsz = i > j ? (i+1) : (j+1);
			tmpk = (char *) realloc(tmpk, tmpsz);
			tmpd = (char *) realloc(tmpd, tmpsz);
			if (!tmpk || !tmpd) return false;
		}

		if (fread(tmpk,1,i+1,f) != (i+1)) return false;
		if (fread(tmpd,1,j+1,f) != (j+1)) return false;
		tmpk[i] = 0;
		tmpd[j] = 0;

		/* substitute \n for real line breaks */
		for(m=0;m<i-1;m++)
			if (tmpk[m]=='\\' && tmpk[m+1] == 'n') {
				tmpk[m] = '\n';
				memmove(&tmpk[m+1],&tmpk[m+2],i-m);
				--m;
			}
		for(m=0;m<j-1;m++)
			if (tmpd[m]=='\\' && tmpd[m+1] == 'n') {
				tmpd[m] = '\n';
				memmove(&tmpd[m+1],&tmpd[m+2],j-m);
				--m;
			}

		e = new TEntry(tmpk, tmpd);
		if (!e) return false;

		i = e->calcHash();
		dict[i].push_back(e);
	}
	free(tmpk);
	free(tmpd);
	return true;
}

const char * Translator::translate(const char *key) {
	int h;
	list<TEntry *>::iterator li;

	if (!ready) return key;

	h = TEntry::hashOf(key);

	for(li=dict[h].begin();li!=dict[h].end();li++)
		if ( (*li)->match(key) )
			return ( (*li)->getData() );

	return key;
}

Translator T;

void langs_prepare(const char *language,
				   const char *package,
				   const char *searchpath)
{
	T.setContext(language, package, searchpath);
}

char * langs_translate(const char *key) {
	return( (char *) ((void *) (T.translate(key))) );
}
