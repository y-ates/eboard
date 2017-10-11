/* $Id: position.cc,v 1.61 2008/02/18 13:21:05 bergo Exp $ */

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
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "position.h"
#include "global.h"
#include "eboard.h"

StringCollection::StringCollection() {
	amOpen = false;
	LastId = 0;
}

StringCollection::~StringCollection() {
	collection.clear();
}

int StringCollection::open() {
	IntAndString s;
	if (amOpen) return LastId;
	s.i = ++LastId;
	collection.push_back(s);
	amOpen=true;
	return LastId;
}

void StringCollection::append(char *s) {
	if (amOpen)
		collection.back().s.append(s);
}

void StringCollection::append(string &s) {
	if (amOpen)
		collection.back().s.append(s);
}

void StringCollection::append(char c) {
	if (amOpen)
		collection.back().s.append(1,c);
}

void StringCollection::close() {
	if (amOpen)
		amOpen=false;
}

bool StringCollection::isOpen() {
	return(amOpen);
}

const char * StringCollection::get(int id) {
	list<IntAndString>::iterator li;

	for(li=collection.begin();li!=collection.end();li++)
		if ( (*li).i == id )
			return ((*li).s.c_str());

	return 0;
}

void StringCollection::link(int id) {
	list<IntAndString>::iterator li;
	for(li=collection.begin();li!=collection.end();li++)
		if ( (*li).i == id ) {
			(*li).references++;
			// cout << "link id " << id << " references " << (*li).references << endl;
			break;
		}
}

void StringCollection::unlink(int id) {
	list<IntAndString>::iterator li;
	for(li=collection.begin();li!=collection.end();li++)
		if ( (*li).i == id ) {
			(*li).references--;
			/*
			  if ( ! (*li).references )
			  collection.erase(li);
			*/
			break;
		}
}

// -------------------------------------------------

SMove::SMove(piece p,int sx,int sy,int dx,int dy) {
	Piece=p;
	SX=sx; SY=sy;
	DX=dx; DY=dy;
}

int SMove::valid() {
	return( !((SX==DX)&&(SY==DY)) );
}

int SMove::distance() {
	return((int)hypot((double)(DX-SX),(double)(DY-SY)));
}

char Position::AnnotationBuffer[512];

Position::Position() {
	setStartPos();
	LastMove.erase();
}

Position::~Position() {
	dropAnnotations();
}

void Position::dropAnnotations() {
	list<int>::iterator ai;
	for(ai=Annotations.begin();ai!=Annotations.end();ai++)
		global.annotator.unlink(*ai);
	Annotations.clear();
}

void Position::setStartPos() {
	register int i,j;
	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			square[i][j]=EMPTY;
	square[0][0]=square[7][0]=square[0][7]=square[7][7]=ROOK;
	square[1][0]=square[6][0]=square[1][7]=square[6][7]=KNIGHT;
	square[2][0]=square[5][0]=square[2][7]=square[5][7]=BISHOP;
	for(i=0;i<8;i++)
		square[i][1]=square[i][6]=PAWN;
	square[3][0]=square[3][7]=QUEEN;
	square[4][0]=square[4][7]=KING;
	for(i=0;i<8;i++)
		for(j=0;j<2;j++) {
			square[i][j]|=WHITE;
			square[i][j+6]|=BLACK;
		}
	for(i=0;i<4;i++)
		maycastle[i]=true;
	ep[0]=-1;
	clearStock();
	sidehint=true;
}

int Position::operator==(const Position &p) {
	int i,j;
	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			if (square[i][j]!=p.square[i][j])
				return 0;
	for(i=0;i<5;i++)
		for(j=0;j<2;j++)
			if (House[i][j]!=p.House[i][j])
				return 0;
	return 1;
}

int Position::operator!=(const Position &p) {
	return(!((*this)==p));
}

Position Position::operator=(Position &p) {
	int i,j;
	list<int>::iterator ai;

	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			square[i][j]=p.square[i][j];
	for(i=0;i<5;i++)
		for(j=0;j<2;j++)
			House[i][j]=p.House[i][j];
	for(i=0;i<4;i++)
		maycastle[i]=p.maycastle[i];
	ep[0]=p.ep[0];
	ep[1]=p.ep[1];
	LastMove = p.LastMove;

	dropAnnotations();
	for(ai=p.Annotations.begin();ai!=p.Annotations.end();ai++) {
		Annotations.push_back(*ai);
		global.annotator.link(*ai);
	}

	sidehint=p.sidehint;

	return(*this);
}

void Position::setPiece(int col,int row,piece p) {
	square[col%8][row%8]=p;
}

piece Position::getPiece(int col,int row) {
	return(square[col%8][row%8]);
}

void Position::addAnnotation(int id) {
	Annotations.push_back(id);
	global.annotator.link(id);
}

char * Position::getAnnotation() {
	list<int>::iterator ai;

	if (Annotations.empty())
		return 0;

	AnnotationBuffer[0]=0;
	for(ai=Annotations.begin();ai!=Annotations.end();ai++)
		strncat(AnnotationBuffer, global.annotator.get(*ai), 511);

	return(AnnotationBuffer);
}

void Position::clearAnnotation() {
	Annotations.clear();
}

void Position::invalidate() {
	int i,j;
	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			square[i][j]++;
}

void Position::setLastMove(char *s) {
	LastMove=s;
}

void Position::setLastMove(string &s) {
	LastMove=s;
}

string & Position::getLastMove() {
	return(LastMove);
}

void Position::moveAnyNotation(char *m,piece color,variant Vr) {
	int i,x,y,ml;
	piece pr;
	char xlate[12];
	global.debug("Position","moveAnyNotation",m);

	if (color==WHITE) sidehint=false; else sidehint=true;

	if (m[1]=='@') {
		switch(m[0]) {
		case 'Q':
		case 'q': pr=QUEEN; break;
		case 'R':
		case 'r': pr=ROOK; break;
		case 'B':
		case 'b': pr=BISHOP; break;
		case 'N':
		case 'n': pr=KNIGHT; break;
		case 'P':
		case 'p': pr=PAWN; break;
		case 'K':
		case 'k': pr=KING; break;
		default: return;
		}
		x=m[2]-'a'; y=m[3]-'1';
		moveDrop(pr|color,x,y);
		return;
	}

	ml = strlen(m);

	if ( isupper(m[0]) || m[1]=='x' || m[2]=='x' || m[0]=='o' || ml==2 ||
		 ml == 3 || m[2]=='=' || ( ml >= 4 && ( m[3]=='+' || m[3]=='#' ) )
		 || (ml>=5 && m[4]=='=') )
		{
			if (m[0]=='o') { // cope with o-o for castling (gnu chess 4)
				g_strlcpy(xlate,m,12);
				for(i=0;xlate[i];i++)
					xlate[i]=toupper(xlate[i]);
				moveStdNotation(xlate,color,Vr);
			} else
				moveStdNotation(m,color,Vr);
		} else {
		if (ml < 4)
			return;
		moveCartesian(m[0]-'a',m[1]-'1',x=(m[2]-'a'),y=(m[3]-'1'),Vr);
		if (m[4]!=0) { // promotion
			switch(m[4]) {
			case 'q': case 'Q': pr=QUEEN; break;
			case 'r': case 'R': pr=ROOK; break;
			case 'n': case 'N': pr=KNIGHT; break;
			case 'b': case 'B': pr=BISHOP; break;
			case 'k': case 'K': pr=KING; break;
			default: return;
			}
			square[x][y]=pr|color|WASPAWN;
		}
    }
}

void Position::moveStdNotation(char *m,piece color,variant Vr) {
	int from[2],to[2];
	int istake, isdrop;
	piece what, prom;
	char *p;
	int cy;
	global.debug("Position","moveStdNotation",m);

	if (color==WHITE) sidehint=false; else sidehint=true;

	what=PAWN;
	from[0]=from[1]=to[0]=to[1]=-1;
	istake=0;
	isdrop=0;

	p=m;
	switch(*p) {
	case 'P': what=PAWN;   p++; break;
	case 'N': what=KNIGHT; p++; break;
	case 'R': what=ROOK;   p++; break;
	case 'B': what=BISHOP; p++; break;
	case 'K': what=KING;   p++; break;
	case 'Q': what=QUEEN;  p++; break;
	case 'O':
		cy=(color==BLACK)?7:0;
		if (!strncmp(m,"O-O-O",5)) {
			square[2][cy]=square[4][cy];
			square[3][cy]=square[0][cy];
			square[0][cy]=EMPTY;
			square[4][cy]=EMPTY;
			maycastle[color==BLACK?3:2]=false;
			return;
		}
		if (!strncmp(m,"O-O",3)) {
			square[6][cy]=square[4][cy];
			square[5][cy]=square[7][cy];
			square[7][cy]=EMPTY;
			square[4][cy]=EMPTY;
			maycastle[color==BLACK?1:0]=false;
			return;
		}
		cerr << "WHAT YOU SAY!!! (" << m << ")\n";
		exit(8);
	}

	prom=EMPTY;
	for(;*p;p++) {
		if (*p=='x') { istake=1; continue; }
		if (*p=='@') { isdrop=1; continue; }
		if ( (*p >= 'a') && (*p <= 'h') ) {
			if (to[0]>=0) from[0]=to[0];
			to[0]=*p-'a';
			continue;
		}
		if ( (*p >= '1') && (*p <= '8') ) {
			if (to[1]>=0) from[1]=to[1];
			to[1]=*p-'1';
			continue;
		}

		// promotion. cope also with bad notation in PGNs (a8Q instead of a8=Q)

		if ( what==PAWN && to[1]%7 == 0 && strchr("=QqRrBbNnKk",*p) ) {
			if (*p == '=') ++p;
			switch(*p) {
			case 'Q':
			case 'q': prom=QUEEN; break;
			case 'R':
			case 'r': prom=ROOK; break;
			case 'B':
			case 'b': prom=BISHOP; break;
			case 'N':
			case 'n': prom=KNIGHT; break;
			case 'K':
			case 'k': prom=KING; break;
			}
			continue;
		}
	} // for

	if ((isdrop)&&(getStockCount(what|color))&&
		(to[0]>=0)&&(to[1]>=0)&&(square[to[0]][to[1]]==EMPTY)) {
		square[to[0]][to[1]]=what|color;
		decStockCount(what|color);
		return;
	}

	locate(what|color,from,to,istake,Vr);
	if ((from[0]<0)||(from[1]<0))
		return;

	// stocked takes
	if (istake)
		if ((Vr==BUGHOUSE)||(Vr==CRAZYHOUSE))
			if (square[to[0]][to[1]]!=EMPTY)
				incStockCount( square[to[0]][to[1]]^COLOR_MASK );
			else
				if (what==PAWN)
					incStockCount( square[from[0]][from[1]] ); // en passant

	// en passant takes
	if ((istake)&&(square[to[0]][to[1]]==EMPTY)&&(what==PAWN)) {
		square[to[0]][to[1]+((color==WHITE)?-1:1)]=EMPTY;
		if (ep[0]>=0) ep[0]=-1;
	}
	square[to[0]][to[1]]=square[from[0]][from[1]];
	square[from[0]][from[1]]=EMPTY;
	if (prom!=EMPTY) {
		square[to[0]][to[1]]&=COLOR_MASK;
		square[to[0]][to[1]]|=prom|WASPAWN;
	}

	// check whether kings and rooks moved
	if (IS_NOT_WILD(Vr) && (Vr!=SUICIDE))
		checkCastlingPossibility();

	// handle explosions in atomic
	if (Vr==ATOMIC && istake)
		makeAtomicExplosion(to[0],to[1]);
}

void Position::checkCastlingPossibility() {
	piece p;

	p=square[4][0]&CUTFLAGS;
	if (p!=(WHITE|KING)) maycastle[0]=maycastle[2]=false;
	p=square[4][7]&CUTFLAGS;
	if (p!=(BLACK|KING)) maycastle[1]=maycastle[3]=false;

	p=square[7][0]&CUTFLAGS; if (p!=(WHITE|ROOK)) maycastle[0]=false;
	p=square[7][7]&CUTFLAGS; if (p!=(BLACK|ROOK)) maycastle[1]=false;
	p=square[0][0]&CUTFLAGS; if (p!=(WHITE|ROOK)) maycastle[2]=false;
	p=square[0][7]&CUTFLAGS; if (p!=(BLACK|ROOK)) maycastle[3]=false;
}

void Position::locate(piece p,int *src,int *dest,int istake,
					  variant Vr) {
	int minx,maxx,miny,maxy;
	int i,j,dc,dr,m,n;
	piece kind,color;
	int reject;

#define RETURN_IF_LEGAL if (isMoveLegalCartesian(src[0],src[1],dest[0],dest[1],square[src[0]][src[1]]&COLOR_MASK,Vr)) { return; } else { break; }

	minx=0; maxx=7; miny=0; maxy=7;
	if (src[0]>=0) minx=maxx=src[0];
	if (src[1]>=0) miny=maxy=src[1];

	for(i=minx;i<=maxx;i++)
		for(j=miny;j<=maxy;j++) {
			if ((square[i][j]&CUTFLAGS)!=p)
				continue;
			kind=p&PIECE_MASK;
			color=p&COLOR_MASK;
			switch(kind) {
			case PAWN:
				if (istake) {
					if (
						((color==WHITE)&&((dest[1]-j)==1)&&(abs(dest[0]-i)==1)) ||
						((color==BLACK)&&((j-dest[1])==1)&&(abs(dest[0]-i)==1))
						) { src[0]=i; src[1]=j; RETURN_IF_LEGAL }
				} else {
					if (
						((color==WHITE) && (i==dest[0]) && ((dest[1]-j)==1)) ||
						((color==WHITE) && (i==dest[0]) && ((dest[1]-j)==2)&&(j==1)) ||
						((color==BLACK) && (i==dest[0]) && ((j-dest[1])==1)) ||
						((color==BLACK) && (i==dest[0]) && ((j-dest[1])==2)&&(j==6))
						) { src[0]=i; src[1]=j; RETURN_IF_LEGAL }
				}
				break;
			case KNIGHT:
				dc=abs(dest[0]-i);
				dr=abs(dest[1]-j);
				if ((dc<1)||(dc>2))
					break;
				if ( (dc+dr) == 3 ) {
					src[0]=i; src[1]=j;
					RETURN_IF_LEGAL
						}
				break;
			case BISHOP:
				dc=abs(dest[0]-i);
				dr=abs(dest[1]-j);
				if ((dc!=dr)||(dc==0))
					break;
				dc=((dest[0]-i)>0) ? 1 : -1;
				dr=((dest[1]-j)>0) ? 1 : -1;
				reject=0;
				for(m=i+dc,n=j+dr;m!=dest[0];m+=dc,n+=dr) {
					if (square[m][n]!=EMPTY) {
						reject=1;
						break;
					}
				}
				if (!reject) { src[0]=i; src[1]=j; RETURN_IF_LEGAL }
				break;
			case ROOK:
				dc=dest[0]-i;
				dr=dest[1]-j;
				if ((dc!=0)&&(dr!=0))
					break;
				if (dc!=0) dc/=abs(dc);
				if (dr!=0) dr/=abs(dr);
				reject=0;
				for(m=i+dc,n=j+dr;(m!=dest[0] || n!=dest[1]);m+=dc,n+=dr) {
					if (square[m][n]!=EMPTY) {
						reject=1;
						break;
					}
				}
				if (!reject) { src[0]=i; src[1]=j; RETURN_IF_LEGAL }
				break;
			case QUEEN:
				dc=dest[0]-i;
				dr=dest[1]-j;
				if ( (abs(dc)!=abs(dr)) && (dc!=0) && (dr!=0) )
					break;
				if (dc!=0) dc/=abs(dc);
				if (dr!=0) dr/=abs(dr);
				reject=0;
				for(m=i+dc,n=j+dr;(m!=dest[0] || n!=dest[1]);m+=dc,n+=dr) {
					if (square[m][n]!=EMPTY) {
						reject=1;
						break;
					}
				}
				if (!reject) { src[0]=i; src[1]=j; RETURN_IF_LEGAL }
				break;
			case KING:
				dc=abs(dest[0]-i);
				dr=abs(dest[1]-j);
				if ((dc>1)||(dr>1))
					break;
				src[0]=i;
				src[1]=j;
				RETURN_IF_LEGAL
					} // switch kind
		} // for j
} // method

void Position::moveDrop(piece p,int x2,int y2) {
	if (getStockCount(p)) {
		square[x2][y2]=p;
		decStockCount(p);
		if (ep[0]>=0)
			ep[0]=-1;
	}
}

void Position::moveCartesian(int x1,int y1,int x2,int y2,
							 variant Vr,
							 bool resolvepromotion)
{
	piece color;
	int   istake=0;

	color=square[x1][y1]&COLOR_MASK;
	if (color==WHITE) sidehint=false; else sidehint=true;

	// castling
	if ((x1==4)&&(x2==6)&&(!(y1%7))&&(!(y2%7))&&
		( (square[x1][y1]&PIECE_MASK)==KING) ) {
		moveStdNotation("O-O",square[x1][y1]&COLOR_MASK);
		return;
	}
	if ((x1==4)&&(x2==2)&&(!(y1%7))&&(!(y2%7))&&
		( (square[x1][y1]&PIECE_MASK) == KING)) {
		moveStdNotation("O-O-O",square[x1][y1]&COLOR_MASK);
		return;
	}

	// en passant (calling moveStdNotation for it would cause an infinite loop)
	if ( ( (square[x1][y1]&PIECE_MASK) == PAWN ) && (square[x2][y2]==EMPTY)&&
		 (x1!=x2)) {
		square[x2][y2+((color==WHITE)?-1:1)]=EMPTY;
		if (ep[0]>=0) ep[0]=-1;
		istake = 1;
	}

	if ((Vr==BUGHOUSE)||(Vr==CRAZYHOUSE))
		if (square[x2][y2]!=EMPTY)
			incStockCount( square[x2][y2]^COLOR_MASK );

	// en passant
	if (((square[x1][y1]&PIECE_MASK)==PAWN)&&(abs(y2-y1)==2)) {
		ep[0]=x1;
		ep[1]=(y2+y1)/2;
	} else {
		if (ep[0]>=0) ep[0]=-1;
	}

	if (square[x2][y2] != EMPTY) istake=1;

	square[x2][y2]=square[x1][y1];
	square[x1][y1]=EMPTY;

	// sometimes we don't want promotions solved by the GUI widget,
	// like when loading moves from a PGN file. In these cases,
	// lack of indication of the piece is a gross error in notation.
	if (resolvepromotion) {

		if (
			( (y2==0) && (square[x2][y2] == (PAWN|BLACK) ) ) ||
			( (y2==7) && (square[x2][y2] == (PAWN|WHITE) ) )
			)
			{
				square[x2][y2]&=COLOR_MASK;
				square[x2][y2]|=WASPAWN | global.promotion->getPiece();
			}
	}

	if (IS_NOT_WILD(Vr) && (Vr!=SUICIDE))
		checkCastlingPossibility();

	// handle explosions in atomic
	if (Vr==ATOMIC && istake)
		makeAtomicExplosion(x2,y2);
}

/* does not test variant or if a capture actually occurred */
void Position::makeAtomicExplosion(int x,int y) {
	int i,j;
	square[x][y] = EMPTY;
	for(i=x-1;i<=x+1;i++) if (i>=0 && i<=7)
							  for(j=y-1;j<=y+1;j++) if (j>=0 && j<=7)
														if ( (square[i][j] & PIECE_MASK) != PAWN)
															square[i][j] = EMPTY;
}

void Position::SANstring(char *src,char *dest) {
	int i,x1,y1,x2,y2;
	Position after;
	piece prom;
	global.debug("Position","SANstring",src);

	// piece dropping, capitalize piece letter
	if (src[1]=='@') {
		strcpy(dest,src);
		dest[0]=toupper(dest[0]);
		// FIXME: if the drop results in check, no '+' is added
		// here. problem: I don't have a way to know which side
		// is dropping.
		return;
	}

	// already SAN, just capitalize castling if needed
	if ( (isupper(src[0])) || (src[1]=='x') ||
		 (src[2]=='x') || (src[0]=='o') || (strlen(src)==2) || (src[2]=='=') ||
		 ((src[2]=='+' || src[2]=='#') && (strlen(src)==3)) ) {
		strcpy(dest,src);
		if (dest[0]=='o')
			for(i=0;dest[i];i++)
				dest[i]=toupper(dest[i]);
		return;
	}
	x1=src[0]-'a';
	y1=src[1]-'1';
	x2=src[2]-'a';
	y2=src[3]-'1';
	switch(src[4]) {
	case 'q': prom=QUEEN; break;
	case 'r': prom=ROOK; break;
	case 'n': prom=KNIGHT; break;
	case 'b': prom=BISHOP; break;
	case 'k': prom=KING; break;
	default: prom=EMPTY;
	}
	stdNotationForMove(x1,y1,x2,y2,prom,dest);
}

void Position::stdNotationForMove(int x1,int y1,int x2,int y2,piece prom,char *m,variant Vr) {
	Position after;
	stdNotationForMoveInternal(x1,y1,x2,y2,m);
	switch(prom) {
	case QUEEN: strcat(m,"=Q"); break;
	case ROOK: strcat(m,"=R"); break;
	case KNIGHT: strcat(m,"=N"); break;
	case BISHOP: strcat(m,"=B"); break;
	case KING: strcat(m,"=K"); break;
	}
	after=(*this);
	after.moveCartesian(x1,y1,x2,y2);
	if (prom!=EMPTY) {
		after.square[x2][y2]&=COLOR_MASK;
		after.square[x2][y2]|=prom|WASPAWN;
	}
	if ( after.isInCheck( (square[x1][y1]&COLOR_MASK) ^ COLOR_MASK ) )
		if ( after.isMate((square[x1][y1]&COLOR_MASK) ^ COLOR_MASK, Vr ) )
			strcat(m,"#");
		else
			strcat(m,"+");
}

void Position::stdNotationForMoveInternal(int x1,int y1,int x2,int y2,char *m) {
	vector<int> ax,ay;
	int istake, hasamb,needrow,needcol;
	unsigned int i;
	piece p,c;
	char *z;

	memset(m,0,8);
	istake=(square[x2][y2]!=EMPTY);
	p=square[x1][y1]&PIECE_MASK;
	c=square[x1][y1]&COLOR_MASK;

	if ((p==KING)&&(c==WHITE)&&(y1==0)&&(y2==0)&&(x1==4)&&(x2==6)) {
		strcpy(m,"O-O"); return;
	}
	if ((p==KING)&&(c==WHITE)&&(y1==0)&&(y2==0)&&(x1==4)&&(x2==2)) {
		strcpy(m,"O-O-O"); return;
	}
	if ((p==KING)&&(c==BLACK)&&(y1==7)&&(y2==7)&&(x1==4)&&(x2==6)) {
		strcpy(m,"O-O"); return;
	}
	if ((p==KING)&&(c==BLACK)&&(y1==7)&&(y2==7)&&(x1==4)&&(x2==2)) {
		strcpy(m,"O-O-O"); return;
	}

	hasamb=needrow=needcol=0;
	ambiguityCheck(p,c,x2,y2,x1,y1,ax,ay);

	if (!ax.empty()) {
		hasamb=1;
		for(i=0;i<ax.size();i++) {
			if (ax[i]==x1) needrow=1;
			if (ay[i]==y1) needcol=1;
		}
		if ((!needcol)&&(!needrow))
			needcol=1;
	}

	ax.clear();
	ay.clear();

	z=m;
	switch(p) {
	case ROOK:   *(z++)='R'; break;
	case KNIGHT: *(z++)='N'; break;
	case BISHOP: *(z++)='B'; break;
	case QUEEN:  *(z++)='Q'; break;
	case KING:   *(z++)='K'; break;
	}

	if ((needcol)||((p==PAWN)&&(istake))) *(z++)='a'+x1;
	if (needrow) *(z++)='1'+y1;
	if (istake) *(z++)='x';
	*(z++)='a'+x2;
	*(z++)='1'+y2;
}

void Position::ambiguityCheck(piece p, piece c,int destx,int desty,
							  int exclx,int excly,
							  vector<int> &ambx,vector<int> &amby)
{
	int i,j;
	for(i=0;i<8;i++) {
		for(j=0;j<8;j++) {
			if ((i==exclx)&&(j==excly))
				continue;
			if (square[i][j]==EMPTY)
				continue;
			if ((square[i][j]&COLOR_MASK)!=c)
				continue;
			if ((square[i][j]&PIECE_MASK)!=p)
				continue;
			if (canMove(p,c,i,j,destx,desty)) {
				ambx.push_back(i);
				amby.push_back(j);
			}
		}
	}
}

bool Position::canMove(piece p,piece c,int sx,int sy,int dx,int dy) {
	int dc,dr,m,n;

	// doesn't check castling! (but isMoveLegalCartesian() checks)

	if ((square[dx][dy]&COLOR_MASK)==c)
		return false;

	switch(p) {
	case PAWN:
		dc=dx-sx;
		dr=dy-sy;
		if (abs(dc)>1)
			return false;
		if (dc==0) {
			if (square[dx][dy]!=EMPTY) return false; // can't take ahead
			if ((c==WHITE)&&(dr==1)) return true;
			if ((c==BLACK)&&(dr==-1)) return true;
			if ((c==WHITE)&&(dr==2)&&(sy==1)&&(square[sx][2]==EMPTY)) return true;
			if ((c==BLACK)&&(dr==-2)&&(sy==6)&&(square[sx][5]==EMPTY)) return true;
			return false;
		} else {
			// e.p.
			if ( (c==WHITE)&&(sy==4)&&(square[dx][dy]==EMPTY)&&
				 (square[dx][dy-1]==(PAWN|BLACK)) && (dr==1) ) {

				if (((ep[0]==dx)&&(ep[1]==dy))||(ep[0]==-2))
					return true;
				else
					return false;

			}
			if ( (c==BLACK)&&(sy==3)&&(square[dx][dy]==EMPTY)&&
				 (square[dx][dy+1]==(PAWN|WHITE)) && (dr==-1) ) {

				if (((ep[0]==dx)&&(ep[1]==dy))||(ep[0]==-2))
					return true;
				else
					return false;

			}
			// normal take
			if ( (c==WHITE)&&(dr==1)&&(square[dx][dy]!=EMPTY) ) return true;
			if ( (c==BLACK)&&(dr==-1)&&(square[dx][dy]!=EMPTY) ) return true;
			return false;
		}
	case ROOK:
		if ((sx!=dx)&&(sy!=dy))
			return false;
		dc=dx-sx;
		dr=dy-sy;
		if (dc) dc/=abs(dc);
		if (dr) dr/=abs(dr);
		for(m=sx+dc,n=sy+dr;((m!=dx)||(n!=dy));m+=dc,n+=dr)
			if (square[m][n]!=EMPTY)
				return false;
		return true;
	case KNIGHT:
		dc=dx-sx;
		dr=dy-sy;
		dc=abs(dc);
		dr=abs(dr);
		if  ((dc==0) || (dr==0) || ( (dr+dc)!=3 ) )
			return false;
		return true;
	case BISHOP:
		dc=dx-sx;
		dr=dy-sy;
		if (abs(dc)!=abs(dr))
			return false;
		if (dc) dc/=abs(dc);
		if (dr) dr/=abs(dr);
		for(m=sx+dc,n=sy+dr;((m!=dx)||(n!=dy));m+=dc,n+=dr)
			if (square[m][n]!=EMPTY)
				return false;
		return true;
	case QUEEN:
		dc=dx-sx;
		dr=dy-sy;
		if ( (dc!=0)&&(dr!=0)&& ( abs(dc)!=abs(dr) ) )
			return false;
		if (dc) dc/=abs(dc);
		if (dr) dr/=abs(dr);
		for(m=sx+dc,n=sy+dr;((m!=dx)||(n!=dy));m+=dc,n+=dr)
			if (square[m][n]!=EMPTY)
				return false;
		return true;
	case KING:
		dc=dx-sx;
		dr=dy-sy;
		if ((abs(dc)>1)||(abs(dr)>1))
			return false;
		return true;
	}
	return false;
}

void Position::print() {
	int i,j;

	cerr << "+---+---+---+---+---+---+---+---+\n";

	for(i=7;i>=0;i--) {

		cerr << "|";
		for(j=0;j<8;j++) {
			cerr << " ";
			switch(square[j][i]&CUTFLAGS) {
			case PAWN|WHITE: cerr << "P"; break;
			case ROOK|WHITE: cerr << "R"; break;
			case KNIGHT|WHITE: cerr << "N"; break;
			case BISHOP|WHITE: cerr << "B"; break;
			case QUEEN|WHITE: cerr << "Q"; break;
			case KING|WHITE: cerr << "K"; break;
			case PAWN|BLACK: cerr << "p"; break;
			case ROOK|BLACK: cerr << "r"; break;
			case KNIGHT|BLACK: cerr << "n"; break;
			case BISHOP|BLACK: cerr << "b"; break;
			case QUEEN|BLACK: cerr << "q"; break;
			case KING|BLACK: cerr << "k"; break;
			default: cerr << " ";
			}
			cerr << " |";
		}
		cerr << "\n+---+---+---+---+---+---+---+---+\n";
	}
	cerr << endl;

	cerr << "stock = ";
	for(i=0;i<2;i++)
		for(j=0;j<5;j++)
			cerr << House[j][i] << " ";
	cerr << endl;

}

/* find the diff from current position to np */
void Position::diff(Position &np,vector<SMove *> &sl) {
	bool sieve[64];
	unsigned int ui;
	int i,j,pend,pi=0,qi=0;
	int sx=0,sy=0,dx=0,dy=0;
	piece p;

	// clear output vector
	for(ui=0;ui<sl.size();ui++)
		delete(sl[ui]);
	sl.clear();

	// set sieve
	pend = 0; // different squares left
	for(i=0;i<8;i++)
		for(j=0;j<8;j++) {
			sieve[i*8+j] = ( getPiece(i,j) != np.getPiece(i,j) );
			if (sieve[i*8+j]) ++pend;
		}

	while(pend > 1) {
		dx = -1;
		for(i=0;i<64;i++)
			if (sieve[i] && np.getPiece(i/8,i%8)!=EMPTY && getPiece(i/8,i%8)==EMPTY) {
				dx = i/8; dy = i%8; pi = i;
				break;
			}

		// no plain moves left, consider captures
		if (dx<0) {
			for(i=0;i<64;i++)
				if (sieve[i] && np.getPiece(i/8,i%8)!=EMPTY && getPiece(i/8,i%8)!=EMPTY) {
					dx = i/8; dy = i%8; pi = i;
					break;
				}
		}

		if (dx<0)
			return; // I don't know what to do, bail out. shouldn't ever happen

		p = np.getPiece(dx,dy);

		sx = -1;
		for(i=0;i<64;i++)
			if (sieve[i] && i!=pi && getPiece(i/8,i%8)==p) {
				sx = i/8;  sy = i%8; qi = i;
				break;
			}

		// promotion ? (WASPAWN can't be checked, FICS positions won't have it)
		if (sx < 0 && ((p&WHITE && dy==7) || (p&BLACK && dy==0))) {
			p = PAWN | (p&COLOR_MASK);
			for(i=0;i<64;i++)
				if (sieve[i] && i!=pi && getPiece(i/8,i%8)==p) {
					sx = i/8;  sy = i%8; qi = i;
					break;
				}
		}

		// unable to match (?,?)@this -> (dx,dy)@np
		// (will happen on crazyhouse drops, for example)
		if (sx < 0) {
			sieve[pi] = false;
			--pend;
		} else {
			sl.push_back(new SMove(p, sx,sy, dx,dy));
			sieve[pi] = sieve[qi] = false;
			pend -= 2;
		}
		// and since we always decrement pend, we can ensure that this
		// loop finishes
	}
}

void  Position::intersection(Position &b) {
	int i,j;

	// this = current pos
	// b    = previous

	for(i=0;i<8;i++)
		for(j=0;j<8;j++) {
			if (b.square[i][j]==EMPTY) {
				square[i][j]=EMPTY; // remove piece that moved
				continue;
			}
			if ((b.square[i][j]!=EMPTY)&&(square[i][j]!=EMPTY)) {
				square[i][j]=b.square[i][j]; // keep taken piece
				continue;
			}
		}
}

string & Position::getHouseString() {
	// [PPPNNB] - [NBQ]
	int i,j,e=1;
	static char xlate[6]="PRNBQ";

	HouseString.erase();

	for(i=0;i<5;i++)
		for(j=0;j<2;j++)
			if (House[i][j]) {
				e=0;
				break;
			}

	if (e) return(HouseString);

	HouseString+='[';

	for(i=0;i<5;i++)
		for(j=0;j<House[i][0];j++)
			HouseString+=xlate[i];

	HouseString+="] - [";

	for(i=0;i<5;i++)
		for(j=0;j<House[i][1];j++)
			HouseString+=xlate[i];

	HouseString+=']';
	return(HouseString);
}

string & Position::getMaterialString(variant Vr) {
	int w=0,b=0,i,j,v;
	char tmp[64];

	for(i=0;i<8;i++)
		for(j=0;j<8;j++) {

			switch(square[i][j]&PIECE_MASK) {
			case PAWN: v=1; break;
			case ROOK: v=5; break;
			case KNIGHT:
			case BISHOP: v=3; break;
			case QUEEN: v=9; break;
			default: v=0;
			}

			if ((Vr==SUICIDE)||(Vr==LOSERS)||(Vr==GIVEAWAY)) {
				if ((square[i][j]&PIECE_MASK)==KING)
					v=1;
				if (v>0)
					v=1;
			}

			if ((square[i][j]&COLOR_MASK)==WHITE)
				w+=v;
			else
				b+=v;
		}

	snprintf(tmp,64,_("Material: %d - %d"),w,b);
	MaterialString=tmp;
	return(MaterialString);
}

int Position::getStockCount(piece p) {
	int i;
	piece k,c;

	k=p&PIECE_MASK;
	c=p&COLOR_MASK;

	switch(k) {
	case PAWN:   i=0; break;
	case ROOK:   i=1; break;
	case KNIGHT: i=2; break;
	case BISHOP: i=3; break;
	case QUEEN:  i=4; break;
	default: return 0;
	}
	return(House[i][c==WHITE?0:1]);
}

void Position::clearStock() {
	int i,j;
	for(i=0;i<5;i++)
		for(j=0;j<2;j++)
			House[i][j]=0;
}

void Position::incStockCount(piece p) {
	int i;
	piece k,c;

	k=p&PIECE_MASK;
	c=p&COLOR_MASK;

	if (p&WASPAWN) k=PAWN;

	switch(k) {
	case PAWN:   i=0; break;
	case ROOK:   i=1; break;
	case KNIGHT: i=2; break;
	case BISHOP: i=3; break;
	case QUEEN:  i=4; break;
	default: return;
	}
	House[i][c==WHITE?0:1]++;

	/*
	  for(i=0;i<5;i++)
	  for(int j=0;j<2;j++)
      cerr << House[i][j] << ",";
	  cerr << endl;
	*/
}

void Position::decStockCount(piece p) {
	int i;
	piece k,c;

	k=p&PIECE_MASK;
	c=p&COLOR_MASK;

	switch(k) {
	case PAWN:   i=0; break;
	case ROOK:   i=1; break;
	case KNIGHT: i=2; break;
	case BISHOP: i=3; break;
	case QUEEN:  i=4; break;
	default: return;
	}
	if (House[i][c==WHITE?0:1])
		House[i][c==WHITE?0:1]--;
}

// legality checking

bool Position::isMoveLegalCartesian(int x0,int y0,int x1,int y1,piece mycolor,variant vari)
{
	Position after;
	int dx;

	//  cerr << "mycolor = " << mycolor << endl;

	// basic sanity checks
	if (square[x0][y0]==EMPTY) return false;
	if ((square[x0][y0]&COLOR_MASK)!=mycolor) return false;
	if ((square[x1][y1]&COLOR_MASK)==mycolor) return false;

	if (vari == ATOMIC)
		return true;

	// castling
	if ((square[x0][y0]&PIECE_MASK)==KING) {
		dx=abs(x0-x1);
		if (dx>1) {
			if ((mycolor==WHITE)&&((y0!=y1)||(y0!=0))) return false;
			if ((mycolor==BLACK)&&((y0!=y1)||(y0!=7))) return false;
			if (vari==WILD || vari==WILDFR || vari==WILDCASTLE) return true; // wild castling accepted always
			if (vari==SUICIDE) return false; // suicide can't castle
			if (vari==WILDNOCASTLE) return false; // nocastle shuffle chess
			if (dx!=2) return false;
			if ((x0==4)&&(x1==6)&&(maycastle[mycolor==WHITE?0:1]))
				if ((square[5][y0]==EMPTY)&&(square[6][y0]==EMPTY))
					return (!( (isSquareInCheck(4,y0,mycolor,vari)) ||
							   (isSquareInCheck(5,y0,mycolor,vari)) ||
							   (isSquareInCheck(6,y0,mycolor,vari)) ) );
			if ((x0==4)&&(x1==2)&&(maycastle[mycolor==WHITE?2:3]))
				if ((square[1][y0]==EMPTY)&&(square[2][y0]==EMPTY)&&(square[3][y0]==EMPTY))
					return (!( (isSquareInCheck(4,y0,mycolor,vari)) ||
							   (isSquareInCheck(3,y0,mycolor,vari)) ||
							   (isSquareInCheck(2,y0,mycolor,vari)) ) );
			return false;
		}
	}

	// not checked for these
	if ((vari==SUICIDE)||(vari==GIVEAWAY)||(vari==LOSERS))
		return true;

	if (!canMove(square[x0][y0]&PIECE_MASK,mycolor,x0,y0,x1,y1))
		return false;

	after=(*this);
	after.moveCartesian(x0,y0,x1,y1);

	if (after.isInCheck(mycolor,vari))
		return false;

	return true;
}

bool Position::isDropLegal(piece p,int x1,int y1,piece mycolor,variant vari)
{
	Position after;

	if ((vari!=CRAZYHOUSE)&&(vari!=BUGHOUSE))
		return false;

	if (square[x1][y1]!=EMPTY) return false;

	after=(*this);
	after.square[x1][y1]=p|mycolor;

	if (after.isInCheck(mycolor,vari))
		return false;

	return true;
}

// dc = defender color
bool Position::isSquareInCheck(int x,int y,piece dc,variant Vr) {
	int i,j;

	//  cerr << "is square in check " << (char)('a'+x) << (y+1) << " ";
	//  cerr << "color = " << dc << endl;

	// FIXME: check rules for giveaway and losers variants
	if (Vr==SUICIDE)
		return false;

	for(i=0;i<8;i++)
		for(j=0;j<8;j++) {
			if (square[i][j]==EMPTY) continue;
			if ((square[i][j]&COLOR_MASK) == dc) continue;

			//      cerr << "i=" << i << " j=" << j << " ";
			//      cerr << "value = " << square[i][j] << endl;
			//      cerr << "color = " << (square[i][j]&COLOR_MASK) << endl;

			if (canMove(square[i][j]&PIECE_MASK,dc^COLOR_MASK,
						i,j,x,y)) {
				//  cerr << "yes from " << (char)('a'+i) << j << endl;
				return true;
			}
		}
	//  cerr << "no" << endl;
	return false;
}

bool Position::isInCheck(piece c,variant Vr) {
	int i,j,kx,ky=0;

	// FIXME: check rules for giveaway and losers variants
	if (Vr==SUICIDE)
		return false;

	kx=-1;
	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			if (square[i][j]==(KING|c)) {
				kx=i; ky=j; i=8; j=8;
				break;
			}
	if (kx<0)
		return false;

	return(isSquareInCheck(kx,ky,c,Vr));
}

bool Position::isMate(piece c,variant Vr) {
	int i,j,m,n;
	Position after;

	if (Vr!=REGULAR)
		return false;

	if (!isInCheck(c,Vr))
		return false;

	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			for(m=0;m<8;m++)
				for(n=0;n<8;n++) {
					if ((m==i)&&(n==j)) continue;
					if ((square[i][j]&COLOR_MASK)!=c) continue;
					if ((square[m][n]&COLOR_MASK)==c) continue;
					if (!isMoveLegalCartesian(i,j,m,n,c,Vr)) continue;

					after=(*this);
					after.moveCartesian(i,j,m,n,Vr);
					if (!after.isInCheck(c,Vr))
						return false;
				}
	return true;
}

bool Position::isStalemate(piece c,variant Vr) {
	int i,j,m,n;
	Position after;

	if (Vr!=REGULAR)
		return false;

	if (isInCheck(c,Vr))
		return false;

	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			for(m=0;m<8;m++)
				for(n=0;n<8;n++) {
					if ((m==i)&&(n==j)) continue;
					if ((square[i][j]&COLOR_MASK)!=c) continue;
					if ((square[m][n]&COLOR_MASK)==c) continue;
					if (!isMoveLegalCartesian(i,j,m,n,c,Vr)) continue;
					after=(*this);
					after.moveCartesian(i,j,m,n,Vr);
					if (!after.isInCheck(c,Vr))
						return false; /* found a legal move, no stalemate */
				}
	return true;
}

/* draw situations recognized : K vs K
   K vs KN
   K vs KB
*/
bool Position::isNMDraw(variant Vr) {
	int n,b;
	int i,j;
	piece p;

	if (Vr!=REGULAR) return false;

	n=b=0;

	for(i=0;i<8;i++) for(j=0;j<8;j++) {
			p = square[i][j] & PIECE_MASK;
			if (p==QUEEN || p==ROOK || p==PAWN) return false;
			if (p==BISHOP) ++b;
			if (p==KNIGHT) ++n;
			if ( (b+n) > 1 ) return false;
		}
	return true;
}

// FEN

string & Position::getFEN() {
	static string fen;
	int i,j,ec;
	piece p,c=EMPTY;
	char n=0;

	fen="\0";

	for(i=7;i>=0;i--) {
		ec=0;
		for(j=0;j<8;j++) {
			p=square[j][i]&PIECE_MASK;
			c=square[j][i]&COLOR_MASK;

			if ((p!=EMPTY)&&(ec)) {
				fen+=(char)('0'+ec);
				ec=0;
			}

			switch(p) {
			case PAWN:    n='P'; break;
			case ROOK:    n='R'; break;
			case KNIGHT:  n='N'; break;
			case BISHOP:  n='B'; break;
			case QUEEN:   n='Q'; break;
			case KING:    n='K'; break;
			case EMPTY:
				++ec;
				n=0;
				break;
			}

			if (n) {
				if (c==BLACK)
					n=(char)tolower(n);
				fen+=n;
			}
		}

		if (ec)   fen+=(char)('0'+ec);
		if (i>0) fen+='/';

	}

	if (sidehint)
		fen+=" w ";
	else
		fen+=" b ";

	ec=0;
	if (maycastle[0]) { fen+="K"; ++ec; }
	if (maycastle[2]) { fen+="Q"; ++ec; }
	if (maycastle[1]) { fen+="k"; ++ec; }
	if (maycastle[3]) { fen+="q"; ++ec; }

	if (!ec) fen+="-";
	fen+=" ";

	if (ep[0]>=0) { fen+='a'+ep[0]; fen+='1'+ep[1]; }
	else { fen+='-'; }
	fen+=" ";

	fen+="0 1";
	return(fen);
}

void Position::setFEN(const char *fen) {
	int c,r;
	const char *p;
	char z,u;
	piece pp;

	for(c=0;c<8;c++)
		for(r=0;r<8;r++)
			square[c][r]=EMPTY;

	p=fen;
	while(*p==' ') ++p;
	if (! *p) return;

	c=0; r=7;
	for(;(*p)!=' ';p++) {
		z=*p;

		if (z==0) return;
		if (z=='/') { c=0; r--; continue; }
		if (isdigit(z)) { c+=z-0x30; continue; }

		u=(char)toupper(z);
		switch(u) {
		case 'P': pp=PAWN; break;
		case 'R': pp=ROOK; break;
		case 'N': pp=KNIGHT; break;
		case 'B': pp=BISHOP; break;
		case 'Q': pp=QUEEN; break;
		case 'K': pp=KING; break;
		default: pp=EMPTY;
		}

		if (pp==EMPTY)
			continue;

		if (z==u) pp|=WHITE; else pp|=BLACK;
		square[c++][r]=pp;
	}

	if (strlen(p)<2)
		return;

	while(*p==' ') ++p;
	if (! *p) return;

	if (toupper(*p) == 'W') sidehint=true; else sidehint=false;

	while(*p==' ') ++p;
	if (! *p) return;

	for(c=0;c<4;c++)
		maycastle[c]=false;
	if ((*p)!='-') {
		for(;*p!=' ';p++) {
			switch(*p) {
			case 'K': maycastle[0]=true; break;
			case 'Q': maycastle[2]=true; break;
			case 'k': maycastle[1]=true; break;
			case 'q': maycastle[3]=true; break;
			case 0: return;
			}
		}
	} else
		++p;

	if (strlen(p)<2)
		return;
	++p;

	ep[0]=-1;
	if ((*p)!='-') {
		ep[0]=(*p++)-'a';
		ep[1]=(*p)-'1';
	}
}
