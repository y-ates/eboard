/* $Id: langs.h,v 1.3 2007/01/01 14:38:58 bergo Exp $ */

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

#ifndef LANGS_H
#define LANGS_H 1

#include <stdio.h>
#include "stl.h"

class TEntry {
 public:
  TEntry(const char *K, const char *D);
  virtual ~TEntry();

  bool  match(const char *testkey);
  int   calcHash();
  char *getData();

  static int hashOf(const char *x);

 private:
  char *key, *data;
};

class Translator {
 public:
  Translator();
  virtual ~Translator();

  /* language = 0 will guess based on the environment */
  void setContext(const char *language, const char *package, 
		  const char *searchpath);

  const char *translate(const char *key);

 private:
  bool ready;

  char Lang[3];
  char SubLang[3];
  char Package[32];

  void guessLanguage();
  void setLanguage(const char *locale);
  bool loadDictionary(FILE *f);

  list<TEntry *> dict[128];
};

void langs_prepare(const char *language, 
		   const char *package, 
		   const char *searchpath);

char * langs_translate(const char *key);

#endif
