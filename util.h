/* $Id: util.h,v 1.20 2007/06/09 11:35:06 bergo Exp $ */

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



#ifndef EBOARD_UTIL_H
#define EBOARD_UTIL_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stl.h"

class FileFinder {
 public:
	FileFinder();
	~FileFinder();
	void addDirectory(char *dir);
	void addDirectory(string &dir);
	void addMyDirectory(char *dir);
	void addMyDirectory(string &dir);

	int  find(char *name,char *fullpath);
	int  find(string &name, string &out);

	int getPathCount();
	string &getPath(unsigned int i);

 private:
	vector<string> path;
};

class EboardFileFinder : public FileFinder {
 public:
	EboardFileFinder();
};

// pattern matching wizardry

class Pattern {
 public:
	Pattern();
	virtual ~Pattern();

	// results: 0=match -1 = no match 1=multiple matches; on
	// matches, last contains the first character after the match
	virtual int tryMatch(list<char>::iterator & first,
						 list<char>::iterator & last)=0;
	virtual void reset();

	bool eternal;
};

class ExactStringPat : public Pattern {
 public:
	ExactStringPat(char *pat);
	virtual ~ExactStringPat() {}

	virtual int tryMatch(list<char>::iterator & first,
						 list<char>::iterator & last);
 protected:
	list<char> pattern;
};

// same thing, case insensitive
class CIExactStringPat : public ExactStringPat {
 public:
 CIExactStringPat(char *pat) : ExactStringPat(pat) { }
	virtual ~CIExactStringPat() {}

	int tryMatch(list<char>::iterator & first,
				 list<char>::iterator & last);
};

// you probably know this simply as the '*' wildcard
class KleeneStarPat : public Pattern {
 public:
	KleeneStarPat();
	int tryMatch(list<char>::iterator & first,
				 list<char>::iterator & last);
	void reset();
 private:
	int CallCount;
};

// string of any size (will try to maximize) made up of
// characters from a given set
class SetPat : public Pattern {
 public:
	SetPat();

	void addToSet(char c);
	void addToSet(char *s);
	void clearSet();

	void includeUppercase();
	void includeLowercase();
	void includeLetters();
	void includeNumbers();

	int tryMatch(list<char>::iterator & first,
				 list<char>::iterator & last);
 protected:
	bool *myset;
 private:
	bool inSet(char c);
};

// generic (eats 256 bytes per object -- unwise)
class CharSetPat : public SetPat {
 public:
	CharSetPat();
 private:
	bool theset[256];
};

// calling include___() or addToSet() or clearSet() on
// any object of the *SetPat classes below is a shooting
// offense -> you'll mess with all other objects of the
// same class.
// these classes eat 256 bytes _per class_, not per instance.

// %n - digits, '+' and '-'
class PercNSetPat : public SetPat {
 public:
	PercNSetPat();
 private:
	static bool theset[256];
};

// %s - A-Z,a-z
class PercSSetPat : public SetPat {
 public:
	PercSSetPat();
 private:
	static bool theset[256];
};

// %S - A-Z,a-z,0-9,_
class PercUpperSSetPat : public SetPat {
 public:
	PercUpperSSetPat();
 private:
	static bool theset[256];
};

// %a - A-Z,a-z,()*
class PercASetPat : public SetPat {
 public:
	PercASetPat();
 private:
	static bool theset[256];
};

// %A - %a U [0-9]
class PercUpperASetPat : public SetPat {
 public:
	PercUpperASetPat();
 private:
	static bool theset[256];
};

// %b - spaces and tabs
class PercBSetPat : public SetPat {
 public:
	PercBSetPat();
 private:
	static bool theset[256];
};

// %r - almost everything except spaces and tabs
class PercRSetPat : public SetPat {
 public:
	PercRSetPat();
 private:
	static bool theset[256];
};

// %N - digits only
class PercUpperNSetPat : public SetPat {
 public:
	PercUpperNSetPat();
 private:
	static bool theset[256];
};

class PatternMatcher {
 public:
	PatternMatcher();
	virtual ~PatternMatcher();

	void append(Pattern *p);
	void reset();
	int  match(const char *stryng); // binds local 'data' list
	int  match();             // uses PatternBinder bound data

	char *getToken(int index);

	void bindData(list<char> *newdata);

 protected:
	void cleanUp();

 private:
	int recursiveMatch(list<Pattern *>::iterator & pat,
					   list<char>::iterator & p0,
					   list<char>::iterator & p1);

	void lesserCleanUp();

	list<Pattern *> pattern;
	list<char>      data;
	list<list<char>::iterator *> matches;
	list<char> *    bound;

	int bufsize;
	char *token;
};

class ExtPatternMatcher : public PatternMatcher {
 public:
	ExtPatternMatcher();

	void set(char *hrp); // set from a human readable pattern

	// for patterns created with "set", these are the %s , %n
	char *getSToken(int index);
	char *getNToken(int index);
	char *getAToken(int index);
	char *getBToken(int index);
	char *getRToken(int index);
	char *getStarToken(int index);

 private:
	char *getXToken(vector<int> &v, int index);

	vector<int> atokens;
	vector<int> btokens;
	vector<int> rtokens;
	vector<int> stokens;
	vector<int> ntokens;
	vector<int> startokens;
};

// helper class to encapsulate char * -> list<char>
// conversion for pattern matchers that will perform
// matching over a common string
class PatternBinder {
 public:
	void add(PatternMatcher *pm0,...); // arguments are pointers to PatternMatcher objects
	void prepare(const char *target);

 private:
	list<PatternMatcher *> group;
	list<char> data;
};

// read from a (possibly) compressed stream. currently
// supports gzip only
// somewhat compatible to istream (not everything is implemented)
// gzip must be in the path. /tmp must be writeable and must have enough
// space for the uncompressed file

// whether the file is gzip'ed or not, it's determined by the .gz
// extension in the name

class zifstream {

 public:
	zifstream();
	zifstream(char *name);

	void          open(char *name);
	bool          getline(char *ptr, int len, char delim='\n');

	bool          seekg(unsigned long offset);
	unsigned long tellg();

	bool          operator!();
	bool          eof();
	void          close();
	bool          fail();
	bool          is_open();

 private:
	ifstream x;
	bool isopen;
	bool compressed;
	int  failure;
	char tmpfile[280];
	char ungzfile[280];
	char origfile[256];

	void copen();
	void cclose();

};

class Timestamp {
 public:
	Timestamp(int sec, int usec);
	Timestamp();

	Timestamp & operator=(Timestamp &t);
	double operator-(Timestamp &t);
	static Timestamp & now();

 private:
	int S,U;

};

#endif
