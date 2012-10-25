/** @file Misc.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef MISC_H
#define MISC_H

#include "stdinc.h"

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
	static void addNum2string(string&, int);
	static void addNum2string(string&, uint64_t);
	static string& num2string(int, string&);
	static string& num2string(uint64_t, string&);
	static string& nstime2string(uint64_t, string&);

	// functions to facilitate use of single character 
	// node names in small data structures
	static char nam(int);			
	static int num(char);			
	static bool readNode(istream&, int&, int);
	static void writeNode(ostream&, int, int);
	static bool readAlpha(istream&, int&);	
	static void writeAlpha(ostream&, int);
	static void addNode2string(string&, int, int);
	static string& node2string(int, int, string&);

	// other stuff
	static bool prefix(string, string);	
	static void genPerm(int, int*);	
	static int strnlen(char*, int);
	static uint32_t getTime();
	static uint64_t getTimeNs();
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

/** Add the string representation of an integer to the end of a given string.
 *  @param s points to the string to be extended
 *  @param i is the integer whose value is to be appended to *s
 */
inline void Misc::addNum2string(string& s, int i) {
	stringstream ss; ss << i;
        s += ss.str();
}

inline void Misc::addNum2string(string& s, uint64_t i) {
	stringstream ss; ss << i;
        s += ss.str();;
}

/** Create a string representation of a numeric value.
 *  @param i is the integer whose value to be converted to a string
 *  @param s is the string in which the value is returned
 *  @return a reference to the string
 */
inline string& Misc::num2string(int i, string& s) {
        char buf[16]; sprintf(buf,"%d",i); s = buf;
	return s;
}
inline string& Misc::num2string(uint64_t i, string& s) {
	stringstream ss; ss << i; s = ss.str();
	return s;
}

/** Create a string representation of a time value based on a ns time value.
 *  The returned string gives the time as seconds and fractions of a second.
 *  @param t is the ns time
 *  @param s is the string in which the value is to be returned
 *  @return a reference to s
 */
inline string& Misc::nstime2string(uint64_t t, string& s) {
	uint64_t sec = t/1000000000;
	uint64_t frac = (t/1000)%1000000;
	
	stringstream ss;
	ss << sec << "." << setw(6) << frac;
	s = ss.str();
	return s;
}

/** Add the string representation of a data structure node to the
 *  end of a given string.
 *  @param s points to the string to be extended
 *  @param u is the node to be added to the end of the string
 *  @param n is the number of nodes in the data structure;
 *  if 1 <= n <= 26, a single lower case character is added
 *  to the string; otherwise, the numeric value of u is added
 */
inline void Misc::addNode2string(string& s, int u, int n) {
        if (1 <= n && n <= 26) s += nam(u);
        else addNum2string(s,u);
}

/** Create a string representation of a data structure node.
 *  @param u is the node
 *  @param n is the number of nodes in the data structure;
 *  if 1 <= n <= 26, a single lower case character is returned
 *  as the string; otherwise, the numeric value of u is added
 *  @param s points to the string to be in which the value is returned
 *  @param return a reference to the modified string
 */
inline string& Misc::node2string(int u, int n, string& s) {
        if (1 <= n && n <= 26) s = nam(u);
        else num2string(u,s);
	return s;
}

#endif
