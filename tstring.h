/* $Id: tstring.h,v 1.6 2003/02/05 19:57:54 bergo Exp $ */

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

#ifndef TSTRING_H
#define TSTRING_H

#include "stl.h"

// string tokenizer
// differs from the one in gps (http://gps.seul.org) and eboard 0.4.3
// since it only matches strtok behavior after setChomp(true).
// it's cleaner too.

class tstring {
 public:

  /* creates a new string tokenizer, with empty string, chomp false
     and fail value 0 */
  tstring();

  /* sets chomp flag. when true, the character that caused the token
     to end is not considered a candidate for the next token */
  void setChomp(bool v);

  /* sets the value tokenvalue returns when there are no more tokens */
  void setFail(int v);

  /* sets the working string and resets position to its start */
  void set(string &s);
  void set(const char *s);

  /* keeps working string but resets position to the start */
  void reset();

  /* returns a pointer to the next token, t is string of delimiter
     characters */
  string *token(char *t);

  /* returns the integer value of the next token, delimited by
     characters of t and assumed to be in the given base */
  int    tokenvalue(char *t, int base=10);

  /* returns false if number is zero, true if not, defval if no
     more tokens */
  bool   tokenbool(char *t, bool defval);

 private:
  string ptoken, src;
  int    pos;
  bool   chomp;
  int    fail;
};

#endif
