/* $Id: tstring.cc,v 1.8 2003/02/05 19:57:52 bergo Exp $ */

/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2002 Felipe Paulo Guazzi Bergo
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

#include <stdlib.h>
#include <string.h>
#include "tstring.h"

tstring::tstring() {
  chomp=false;
  fail=0;
}

void tstring::set(string &s) {
  pos=0;
  src=s;
  ptoken.erase();
}

void tstring::set(const char *s) {
  pos=0;
  src=s;
  ptoken.erase();
}

string * tstring::token(char *t) {
  int j;
  j=src.length();
  
  if (pos>=j) return 0;

  ptoken.erase();
  
  // skip to first position of token
  while ( strchr(t, src[pos]) ) {
    pos++;
    if (pos>=j)
      return 0;
  }

  while ( ! strchr(t, src[pos]) ) {
    ptoken+=src[pos];
    pos++;
    if (pos>=j)
      break;
  }

  if ( chomp && pos<j )
    ++pos;

  return(&ptoken);
}

int tstring::tokenvalue(char *t, int base) {
  string *v;
  int n;
  v=token(t);
  if (!v) return fail;
  n=(int) strtol(v->c_str(),0,base);
  return n;
}

bool tstring::tokenbool(char *t, bool defval) {
  string *v;
  int n;
  v=token(t);
  if (!v) return defval;
  n=(int) atoi(v->c_str());
  return(n!=0);
}

void tstring::setChomp(bool v) {
  chomp=v;
}

void tstring::setFail(int v) {
  fail=v;
}

void tstring::reset() {
  pos=0;
  ptoken.erase();
}
