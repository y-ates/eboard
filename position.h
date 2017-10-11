/* $Id: position.h,v 1.30 2003/07/01 04:38:26 bergo Exp $ */

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



#ifndef POSITION_H
#define POSITION_H 1

#include "eboard.h"
#include "stl.h"

class IntAndString {
 public:
	IntAndString() { references=0; }

	string s;
	int    i;
	int    references;
};

// if only glibc would malloc() corectly, we wouldn't need this ugly duck
class StringCollection {
 public:
	StringCollection();
	~StringCollection();

	int open();
	void append(char *s);
	void append(string &s);
	void append(char c);
	void close();

	void link(int id);
	void unlink(int id);

	bool isOpen();

	const char *get(int id);

 private:
	list<IntAndString> collection;
	int LastId;
	bool amOpen;
};

class SMove {
 public:
	SMove(piece p,int sx,int sy,int dx,int dy);
	int distance();
	int valid();
	int SX,SY,DX,DY;
	piece Piece;
};

class Position {
 public:
	Position();
	~Position();

	int operator==(const Position &p);
	int operator!=(const Position &p);
	Position operator=(Position &p);

	void  setStartPos();
	void  setPiece(int col,int row,piece p);
	piece getPiece(int col,int row);
	void  setLastMove(char *s);
	void  setLastMove(string &s);
	string &getLastMove();
	string &getMaterialString(variant Vr=REGULAR);
	string &getHouseString();

	void addAnnotation(int id);
	char *getAnnotation();
	void clearAnnotation();

	string &getFEN();
	void setFEN(const char *fen);

	int  getStockCount(piece p);
	void clearStock();
	void incStockCount(piece p);
	void decStockCount(piece p);

	void  print();
	void  diff(Position &np,vector<SMove *> &sl);
	void  intersection(Position &b);

	void moveAnyNotation(char *m,piece color,variant Vr=REGULAR);
	void moveStdNotation(char *m,piece color,variant Vr=REGULAR);
	void moveCartesian(int x1,int y1,int x2,int y2,variant Vr=REGULAR,
					   bool resolvepromotion=false);
	void moveDrop(piece p,int x2,int y2);

	void stdNotationForMove(int x1,int y1,int x2,int y2,piece prom,char *m,
							variant Vr=REGULAR);
	void SANstring(char *src,char *dest);

	// randomly changes the position to make it "different"
	void  invalidate();

	// checking

	bool isMoveLegalCartesian(int x0,int y0,int x1,int y1,piece mycolor,variant vari);
	bool isDropLegal(piece p,int x1,int y1,piece mycolor,variant vari);

	bool isMate(piece c,variant Vr=REGULAR);
	bool isStalemate(piece c,variant Vr=REGULAR);
	bool isNMDraw(variant Vr=REGULAR); /* no-material draw */

	bool maycastle[4]; // 0=white short, 1=black short, 2=white long, 3=black long
	signed char ep[2]; // enpassant square (ep[0]=-1 for none, ep[0]=-2 for allow any)
	bool sidehint; // used by scratch boards only

 private:
	piece  square[8][8];
	string LastMove;
	string MaterialString;
	string HouseString;

	short int House[5][2]; // {crazy,bug}house stock (P,R,N,B,Q)
	list<int> Annotations;

	static char AnnotationBuffer[512];

	void locate(piece p,int *src,int *dest,int istake,variant Vr=REGULAR);
	void ambiguityCheck(piece p, piece c,int destx,int desty,
						int exclx,int excly,
						vector<int> & ambx, vector<int> & amby);
	bool canMove(piece p,piece c,int sx,int sy,int dx,int dy);
	bool isInCheck(piece c,variant Vr=REGULAR);
	bool isSquareInCheck(int x,int y,piece dc,variant Vr=REGULAR);
	void checkCastlingPossibility();

	void stdNotationForMoveInternal(int x1,int y1,int x2,int y2,char *m);
	void makeAtomicExplosion(int x,int y);

	void dropAnnotations();
};

#endif
