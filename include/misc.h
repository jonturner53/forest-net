// Miscellaneous routines, mostly to assist with io.
// Organized as a class with static member functions.

#ifndef MISC_H
#define MISC_H

#include "stdinc.h"

// IP address and port number definitions
typedef uint32_t ipa_t;
typedef uint16_t ipp_t;

class misc {
public:
	// io helper functions
	static char nam(int);			// return char name for node
	static int num(char);			// return node number for char
	static char cflush(istream&, char);	// read up to specified char
	static char rflush(istream&, char);	// read past copies of char
	static bool getNode(istream&, int&, int); // read a data structure node
	static void putNode(ostream&, int, int);  // print a node
	static bool getAlpha(istream&, int&);	// get node name as int
	static void putAlpha(ostream&, int);	// print node as name
	static bool getNum(istream&, int&);	// read a number on current line
	static bool getNum(istream&, uint16_t&);	
	static bool getNum(istream&, uint32_t&);	
	static bool getWord(istream&, string&);	// read a word on current line
	static bool getIpAdr(istream&, ipa_t&);	// read IP address
	static ipa_t ipAddress(char*); 		// return IP addr for string
	static char* ipString(ipa_t ipa);	// return str for IP addr
	static bool verify(istream&, char);	// verify next character
	static bool skipBlank(istream&);	// skip over blanks, comments

	// other stuff
	static bool prefix(string, string);	// true if s1 a prefix of s2
	static void genPerm(int, int*);		// generate random permutation
};

// u is assumed to be between 1 and 26
inline char misc::nam(int u) { return char(u + ('a'-1)); }

// c is assumed to be lower case
inline int misc::num(char c) { return int(c - ('a'-1)); }

#endif
