/* $Id: util.cc,v 1.36 2007/06/09 11:35:06 bergo Exp $ */

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


#include "util.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include "global.h"
#include "stl.h"

#include "config.h"
#include "eboard.h"

FileFinder::FileFinder() {

}

FileFinder::~FileFinder() {
	path.clear();
}

int FileFinder::getPathCount() {
	return(path.size());
}

string &FileFinder::getPath(unsigned int i) {
	static string dot(".");
	if ((i<0)||(i>=path.size()))
		return(dot);
	return(path[i]);
}

void FileFinder::addDirectory(char *dir)   { path.push_back(string(dir)); }
void FileFinder::addDirectory(string &dir) { path.push_back(string(dir)); }

void FileFinder::addMyDirectory(char *dir) {
	string s;
	s=global.env.Home; s+='/'; s+=dir;
	path.push_back(s);
}

void FileFinder::addMyDirectory(string &dir) {
	string s;
	s=global.env.Home; s+='/'; s+=dir;
	path.push_back(s);
}

int FileFinder::find(string &name, string &out) {
	int i,j;
	string fullname;

	ifstream f(name.c_str());
	if (!f) {
		j=path.size();
		for(i=0;i<j;i++) {
			fullname = path[i];
			fullname += '/';
			fullname += name;

			ifstream g(fullname.c_str());
			if (g) {
				out=fullname;
				g.close();
				return 1;
			}
		}
		return 0;
	}
	f.close();

	// avoid problems with scripts when . is not in the PATH
	out="./";
	out+=name;

	return 1;
}

int FileFinder::find(char *name,char *fullpath) {
	int i,j;
	char fullname[512];

	ifstream f(name);
	if (!f) {
		j=path.size();
		for(i=0;i<j;i++) {
			g_strlcpy(fullname,path[i].c_str(),512);
			g_strlcat(fullname,"/",512);
			g_strlcat(fullname,name,512);
			ifstream g(fullname);
			if (g) {
				strcpy(fullpath,fullname);
				g.close();
				return 1;
			}
		}
		return 0;
	}
	f.close();
	g_strlcpy(fullpath,"./",512);
	g_strlcat(fullpath,name,512);
	return 1;
}

EboardFileFinder::EboardFileFinder() {
	addDirectory(".");
	addMyDirectory(".eboard");
	addMyDirectory("share/eboard");

	addDirectory(DATADIR "/eboard");
	addDirectory("/usr/share/eboard");
	addDirectory("/usr/local/share/eboard");

}

// ----------------------------------------  pattern matching

PercASetPat      percA;
PercUpperASetPat percUA;
PercBSetPat      percB;
PercNSetPat      percN;
PercUpperNSetPat percUN;
PercRSetPat      percR;
PercSSetPat      percS;
PercUpperSSetPat percUS;

Pattern::Pattern() {
	eternal=false;
}

Pattern::~Pattern() {

}

void Pattern::reset() {

}

ExactStringPat::ExactStringPat(char *pat) {
	int i,j;
	j=strlen(pat);
	for(i=0;i<j;i++)
		pattern.push_back(pat[i]);
}

int ExactStringPat::tryMatch(list<char>::iterator & first,
							 list<char>::iterator & last) {
	list<char>::iterator mine,their;

	for(mine=pattern.begin(), their=first;
		(mine!=pattern.end()) && (their!=last);
		mine++, their++) {
		if (*mine != *their) return -1;
	}
	if (mine!=pattern.end()) return -1;
	last=their;
	return 0;
}

int CIExactStringPat::tryMatch(list<char>::iterator & first,
							   list<char>::iterator & last) {
	list<char>::iterator mine,their;

	for(mine=pattern.begin(), their=first;
		(mine!=pattern.end()) && (their!=last);
		mine++, their++) {
		if (toupper(*mine) != toupper(*their)) return -1;
	}
	if (mine!=pattern.end()) return -1;
	last=their;
	return 0;
}

KleeneStarPat::KleeneStarPat() {
	reset();
}

int KleeneStarPat::tryMatch(list<char>::iterator & first,
							list<char>::iterator & last) {
	int i;
	list<char>::iterator dupe1;

	dupe1=first;
	for(i=CallCount;i;i--) {
		if (dupe1==last) return -1;
		dupe1++;
	}
	++CallCount;
	last=dupe1;
	return 1;
}

void KleeneStarPat::reset() {
	CallCount=0;
}

// ------------------------------------------ the set patterns

SetPat::SetPat() {
	myset=0;
}

inline void SetPat::addToSet(char c) {
	if (myset)
		myset[(unsigned char)c]=true;
}

void SetPat::addToSet(char *s) {
	while(*s)
		addToSet(*(s++));
}

int SetPat::tryMatch(list<char>::iterator & first,
					 list<char>::iterator & last) {
	list<char>::iterator mine;
	if (! inSet(*first) )
		return -1;

	for(mine=first;mine!=last;mine++)
		if (!inSet(*mine)) {
			last=mine;
			return 0;
		}
	last=mine;
	return 0;
}

inline bool SetPat::inSet(char c) {
	return( myset[(unsigned char)c] );
}

void SetPat::clearSet() {
	if (myset)
		for(int i=0;i<256;i++)
			myset[i]=false;
}

void SetPat::includeUppercase() {
	char c;
	for(c='A';c<='Z';c++)
		addToSet(c);
}

void SetPat::includeLowercase() {
	char c;
	for(c='a';c<='z';c++)
		addToSet(c);
}

void SetPat::includeLetters() {
	includeLowercase();
	includeUppercase();
}

void SetPat::includeNumbers() {
	char c;
	for(c='0';c<='9';c++)
		addToSet(c);
}

// ----------------------------------- generic CharSet

CharSetPat::CharSetPat() {
	myset=theset;
	clearSet();
}

// ----------------------------------- the percent-patterns

/*
  %N = [0-9]
  %n = [0-9] U {+,-}
  %s = [A-Za-z]
  %S = [A-Za-z0-9]
  %a = [A-Za-z] U {(,),*}
  %A = [A-Za-z0-9] U {(,),*}
  %b = {space} U {tab}
  %r = [0-9A-Za-z] U "()[]{}.,;:!@#$%^&*_-+=\\|<>?/~"
*/

bool PercNSetPat::theset[256];
bool PercBSetPat::theset[256];
bool PercSSetPat::theset[256];
bool PercASetPat::theset[256];
bool PercRSetPat::theset[256];
bool PercUpperNSetPat::theset[256];
bool PercUpperASetPat::theset[256];
bool PercUpperSSetPat::theset[256];

PercNSetPat::PercNSetPat() {
	myset=theset;
	clearSet();
	includeNumbers();
	addToSet("+-");
}

PercSSetPat::PercSSetPat() {
	myset=theset;
	clearSet();
	includeLetters();
}

PercUpperSSetPat::PercUpperSSetPat() {
	myset=theset;
	clearSet();
	includeLetters();
	includeNumbers();
	addToSet("_");
}

PercASetPat::PercASetPat() {
	myset=theset;
	clearSet();
	includeLetters();
	addToSet("()*");
}

PercUpperASetPat::PercUpperASetPat() {
	myset=theset;
	clearSet();
	includeLetters();
	includeNumbers();
	addToSet("()*");
}

PercBSetPat::PercBSetPat() {
	myset=theset;
	clearSet();
	addToSet(" \t");
}

PercRSetPat::PercRSetPat() {
	myset=theset;
	clearSet();
	includeLetters();
	includeNumbers();
	addToSet("()[]{}.,;:!@#$%^&*_-+=\\|<>?/~");
}

PercUpperNSetPat::PercUpperNSetPat() {
	myset=theset;
	clearSet();
	includeNumbers();
}

// -----------------------------------

PatternMatcher::PatternMatcher() {
	token=(char *)malloc(bufsize=4096);
	bound=&data;
}

PatternMatcher::~PatternMatcher() {
	list<Pattern *>::iterator pi;
	for(pi=pattern.begin();pi!=pattern.end();pi++)
		if (! (*pi)->eternal )
			delete(*pi);
	pattern.clear();
	free(token);
}

char * PatternMatcher::getToken(int index) {
	int sz,i;
	list<char>::iterator ci,p0,p1;
	list<list<char>::iterator *>::iterator trav;

	for(i=0,trav=matches.begin();i<index;i++) {
		if (trav==matches.end()) return("<null>");
		trav++;
	}

	p0=*(*trav);
	trav++;
	p1=*(*trav);

	for(sz=0,ci=p0;ci!=p1;ci++)
		sz++;
	if (sz>bufsize)
		token=(char *)realloc(token,bufsize=sz+16);
	memset(token,0,bufsize);
	for(i=0,ci=p0;ci!=p1;ci++,i++)
		token[i]=*ci;
	return token;
}

void PatternMatcher::append(Pattern *p) {
	cleanUp();
	pattern.push_back(p);
}

void PatternMatcher::reset() {
	list<Pattern *>::iterator pi;
	cleanUp();
	for(pi=pattern.begin();pi!=pattern.end();pi++)
		if (! (*pi)->eternal )
			delete(*pi);
	pattern.clear();
}

void PatternMatcher::bindData(list<char> *newdata) {
	bound=newdata;
}

int PatternMatcher::match() {
	int i;
	list<Pattern *>::iterator ip;
	list<char>::iterator ic1,ic2;
	list<char>::iterator *gnd;

	lesserCleanUp();

	if (!bound)
		return 0;
	i=recursiveMatch(ip=pattern.begin(),ic1=bound->begin(),ic2=bound->end());
	if (i) {
		gnd=new list<char>::iterator();
		*gnd=bound->end();
		matches.push_back(gnd);
	}
	return i;
}

int PatternMatcher::match(const char *stryng) {
	int i,j;
	list<Pattern *>::iterator ip;
	list<char>::iterator ic1,ic2;
	list<char>::iterator *gnd;

	cleanUp();
	j=strlen(stryng);
	for(i=0;i<j;i++)
		data.push_back(stryng[i]);
	bound=&data;

	i=recursiveMatch(ip=pattern.begin(),ic1=data.begin(),ic2=data.end());
	if (i) {
		gnd=new list<char>::iterator();
		*gnd=data.end();
		matches.push_back(gnd);
	}
	return i;
}

inline void PatternMatcher::lesserCleanUp() {
	list<list<char>::iterator *>::iterator ti;
	for(ti=matches.begin();ti!=matches.end();ti++)
		delete(*ti);
	matches.clear();
}

void PatternMatcher::cleanUp() {
	lesserCleanUp();
	data.clear();
}

int PatternMatcher::recursiveMatch(list<Pattern *>::iterator & pat,
								   list<char>::iterator & p0,
								   list<char>::iterator & p1) {
	list<char>::iterator b,*z,ic1;
	list<Pattern *>::iterator pat2;
	int t,s;

	// null pattern matches null string
	if ((pat==pattern.end())&&(p0==bound->end()))
		return 1;
	// else null pattern matches nothing
	if (pat==pattern.end())
		return 0;

	b=p1;
	pat2=pat;

	(*pat)->reset();
	t=(*pat)->tryMatch(p0,b);

	switch(t) {
	case -1: // no match
		return 0;
	case 0:  // one match
		z=new list<char>::iterator();
		(*z)=p0;
		matches.push_back(z);
		pat2++;
		s=recursiveMatch(pat2,b,ic1=bound->end());
		if (!s) { delete z; matches.pop_back(); }
		return s;
	case 1:  // matched, and may match again
		z=new list<char>::iterator();
		(*z)=p0;
		matches.push_back(z);
		pat2++;

		do {
			s=recursiveMatch(pat2,b,ic1=bound->end());
			if (!s) {
				b=p1;
				t=(*pat)->tryMatch(p0,b);
			} else
				return s;
		} while(t>=0);

		delete z; matches.pop_back();
		return 0;
	}
	return 0;
}

// Ext

ExtPatternMatcher::ExtPatternMatcher() : PatternMatcher() {

}

void ExtPatternMatcher::set(char *hrp) {
	int i,L,tc;
	char exactbuf[256],ebsz;

	cleanUp();

	if (!percA.eternal) {
		percA.eternal=true;
		percUA.eternal=true;
		percB.eternal=true;
		percN.eternal=true;
		percUN.eternal=true;
		percR.eternal=true;
		percS.eternal=true;
		percUS.eternal=true;
	}

	tc=0; // token count
	L=strlen(hrp);
	memset(exactbuf,0,256);
	ebsz=0;
	for(i=0;i<L;i++) {
		switch(hrp[i]) {
		case '*':
			if (ebsz) {
				append(new ExactStringPat(exactbuf));
				++tc;
				memset(exactbuf,0,256);
				ebsz=0;
			}
			append(new KleeneStarPat());
			startokens.push_back(tc++);

			break;
		case '%':
			if (i==(L-1)) {
				cerr << _("<PatternMatcher::set> ** bad pattern string: ") << hrp << endl;
				exit(67);
			}
			++i;
			if (hrp[i]=='*') {
				exactbuf[ebsz++]='*';
				break;
			}
			if (hrp[i]=='%') {
				exactbuf[ebsz++]='%';
				break;
			} else {
				if (ebsz) {
					append(new ExactStringPat(exactbuf));
					++tc;
					memset(exactbuf,0,256);
					ebsz=0;
				}
				switch(hrp[i]) {
				case 's':
					append(&percS);
					stokens.push_back(tc++);
					break;
				case 'S':
					append(&percUS);
					stokens.push_back(tc++);
					break;
				case 'n':
					append(&percN);
					ntokens.push_back(tc++);
					break;
				case 'N':
					append(&percUN);
					ntokens.push_back(tc++);
					break;
				case 'a':
					append(&percA);
					atokens.push_back(tc++);
					break;
				case 'A':
					append(&percUA);
					atokens.push_back(tc++);
					break;
				case 'b':
					append(&percB);
					btokens.push_back(tc++);
					break;
				case 'r':
					append(&percR);
					rtokens.push_back(tc++);
					break;
				default:
					cerr << _("<PatternMatcher::set> ** bad pattern string: ") << hrp << endl;
					exit(68);
				}
			}
			break;
		default:
			exactbuf[ebsz++]=hrp[i];
			break;
		}
	}
	if (ebsz)
		append(new ExactStringPat(exactbuf));
}

char *ExtPatternMatcher::getXToken(vector<int> &v, int index) {
	if (index >= (signed int) (v.size()) )
		return 0;
	return(getToken(v[index]));
}

char * ExtPatternMatcher::getSToken(int index) {
	return(getXToken(stokens,index));
}

char * ExtPatternMatcher::getNToken(int index) {
	return(getXToken(ntokens,index));
}

char * ExtPatternMatcher::getStarToken(int index) {
	return(getXToken(startokens,index));
}

char * ExtPatternMatcher::getAToken(int index) {
	return(getXToken(atokens,index));
}

char * ExtPatternMatcher::getBToken(int index) {
	return(getXToken(btokens,index));
}

char * ExtPatternMatcher::getRToken(int index) {
	return(getXToken(rtokens,index));
}

// ----------------------- PatternBinder

void PatternBinder::add(PatternMatcher *pm0,...) {
	va_list ap;
	PatternMatcher *pm;
	group.push_back(pm0);
	va_start(ap,pm0);
	for(;;) {
		pm=va_arg(ap,PatternMatcher *);
		if (!pm) break;
		group.push_back(pm);
	}
	va_end(ap);
}

void PatternBinder::prepare(const char *target) {
	list<PatternMatcher *>::iterator gi;
	int i,j;

	data.clear();

	j=strlen(target);
	for(i=0;i<j;i++)
		data.push_back(target[i]);

	for(gi=group.begin();gi!=group.end();gi++)
		(*gi)->bindData(&data);
}

// ----------------------- zifstream

zifstream::zifstream() {
	compressed  = false;
	isopen      = false;
	failure     = 0;
	tmpfile[0]  = 0;
	origfile[0] = 0;
}

zifstream::zifstream(char *name) {
	compressed  = false;
	isopen      = false;
	failure     = 0;
	tmpfile[0]  = 0;
	origfile[0] = 0;
	open(name);
}

void zifstream::open(char *name) {
	int n;

	n = strlen(name);
	if (n>255) {
		failure = 1;
		return;
	}

	strcpy(origfile,name);

	if (n>3)
		if (!strcmp(&origfile[n-3],".gz")) {
			compressed = true;
			copen();
			return;
		}

	x.open(origfile,ios::in);

	if (!x) failure = 1;
	if (x.is_open()) isopen = true;
}

bool zifstream::getline(char *ptr, int len, char delim) {
	if (x.getline(ptr,len,delim))
		return true;
	else
		return false;
}

bool zifstream::seekg(unsigned long offset) {
	x.seekg(offset);
	return((bool)(x.good()));
}

unsigned long zifstream::tellg() {
	return( (unsigned long) x.tellg() );
}

bool zifstream::operator!() {
	return(x.fail()!=0);
}

bool zifstream::eof() {
	return((bool)(x.eof()));
}

void zifstream::close() {
	x.close();
	if (isopen && compressed)
		cclose();
	isopen=false;
}

bool zifstream::fail() {
	return(failure!=0 || x.fail()!=0);
}

bool zifstream::is_open() {
	return(isopen);
}

void zifstream::copen() {
	int i,n;
	char cmd[1024];

	n = snprintf(tmpfile,1024,"/tmp/eb%d-%s",(int) getpid(), origfile);

	if (n >= 1024) {
		failure = 1;
		return;
	}

	for(i=5;i<n;i++)
		if (tmpfile[i] == '/') tmpfile[i] = '_';

	strcpy(ungzfile, tmpfile);
	ungzfile[n-3] = 0;

	n = snprintf(cmd,1024,"cp -f %s %s",origfile, tmpfile);

	if (n>=1024 || system(cmd)!=0) {
		failure = 1;
		return;
	}

	n = snprintf(cmd,1024,"gzip -d %s",tmpfile);

	if (n>=1024 || system(cmd)!=0) {
		failure = 1;
		unlink(tmpfile);
		return;
	}

	x.open(ungzfile,ios::in);
	if (!x) failure = 1;
	if (x.is_open()) isopen = true;

	if (!isopen) {
		unlink(tmpfile);
		unlink(ungzfile);
	}
}

void zifstream::cclose() {
	unlink(ungzfile);
}

Timestamp::Timestamp(int sec, int usec) {
	S=sec;
	U=usec;
}

Timestamp::Timestamp() {
	(*this) = Timestamp::now();
}

Timestamp & Timestamp::operator=(Timestamp &t) {
	S = t.S; U=t.U;
	return(*this);
}

double Timestamp::operator-(Timestamp &t) {
	int msec;
	double sec;
	msec = 1000*(S-t.S);
	msec += (U/1000 - t.U/1000);
	sec = (double) (msec / 1000.0);
	return sec;
}

Timestamp & Timestamp::now() {
	static Timestamp t(0,0);
	struct timeval tv;
	gettimeofday(&tv,0);
	t.S = (int) tv.tv_sec;
	t.U = (int) tv.tv_usec;
	return(t);
}
