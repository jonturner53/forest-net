// Miscellaneous routines, mostly to assist with io.
// Organized as a class with static member functions.

#ifndef MISC_H
#define MISC_H

#include "stdinc.h"

// shorthands for internet address/port types
typedef in_addr_t ipa_t;
typedef in_port_t ipp_t;

class Misc {
public:
	// basic io helper functions
	static char cflush(istream&, char);
	static char rflush(istream&, char);
	static bool verify(istream&, char);
	static bool skipBlank(istream&);
	static bool readNum(istream&, int&);
	static bool readNum(istream&, char&);	
	static bool readNum(istream&, uint16_t&);	
	static bool readNum(istream&, uint32_t&);	
	static bool readWord(istream&, string&);

	// functions to facilitate use of single character 
	// node names in small data structures
	static char nam(int);			
	static int num(char);			
	static bool readNode(istream&, int&, int);
	static void writeNode(ostream&, int, int);
	static bool readAlpha(istream&, int&);	
	static void writeAlpha(ostream&, int);

	// other stuff
	static bool prefix(string, string);	
	static void genPerm(int, int*);	
	static int strnlen(char*, int);
};

/** Convert a small integer to a lower-case letter.
 *  @param u is an integer in the range 1..26
 *  @return the corresponding lower-case letter
 *  (1 becomes 'a', 2 becomes 'b', etc)
 */
inline char Misc::nam(int u) { return char(u + ('a'-1)); }

/** Convert a lower-case letter to a small integer.
 *  @param c is a lower-case letter
 *  @return the corresponding integer
 *  ('a' becomes 1, 'b' becomes 2, etc)
 */
inline int Misc::num(char c) { return int(c - ('a'-1)); }

#endif
