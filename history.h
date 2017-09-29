/* $Id: history.h,v 1.2 2002/09/10 19:08:28 bergo Exp $ */

/*

    eboard - chess client
    http://eboard.sourceforge.net
    Copyright (C) 2000-2001 Felipe Paulo Guazzi Bergo
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

#ifndef EBOARD_HISTORY_H
#define EBOARD_HISTORY_H 1

#include "stl.h"

class History {
 public:
  History(unsigned int len);

  typedef list<char *>::iterator iterator;

  void appendString(char *s);
  char *moveUp();
  char *moveDown();
  char *moveUp(iterator &alt);
  char *moveDown(iterator &alt);
  iterator getCursor();
  
 private:
  unsigned int Length;
  list<char *> thehistory;
  iterator cursor;
};

#endif
