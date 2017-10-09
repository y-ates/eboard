/* $Id: history.cc,v 1.3 2002/09/10 19:08:28 bergo Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

History::History(unsigned int len) {
	Length=len;
	cursor=thehistory.end();
}

void History::appendString(char *s) {
	if (!thehistory.empty())
		if (!strcmp(s,thehistory.back())) {
			free(s);
			cursor=thehistory.end();
			return;
		}
	thehistory.push_back(s);
	cursor=thehistory.end();
	if (thehistory.size() > Length) {
		free(thehistory.front());
		thehistory.pop_front();
	}
}

char * History::moveUp() {
	return(moveUp(cursor));
}

char * History::moveDown() {
	return(moveDown(cursor));
}

char * History::moveUp(iterator &alt) {
	if (thehistory.empty())
		return("\0");
	if (alt!=thehistory.begin())
		alt--;
	return(*alt);
}

char * History::moveDown(iterator &alt) {
	if (thehistory.empty())
		return("\0");
	if (alt!=thehistory.end())
		alt++;
	if (alt!=thehistory.end())
		return(*alt);
	else
		return("\0");
}

History::iterator History::getCursor() {
	return(thehistory.end());
}
