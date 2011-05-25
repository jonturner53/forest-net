/** \file Misc.cpp */

#include "Misc.h"

/** Read up to first occurrence of a given character.
 *  @param in is the input stream to read from
 *  @param c  is character to look for
 *  @return c or 0, on end-of-file
 */
char Misc::cflush(istream& in, char c) {
        char c1;
        while (1) {
                in.get(c1);
                if (in.eof()) return 0;
                if (c1 == c) return c;
        }
}

/** Read up to first character c1 not equal to a specified character.
 *  @param in is the input stream to read from
 *  @param c  is character to look for
 *  @return c1 or 0, on end-of-file
 */
char Misc::rflush(istream& in, char c) {
        char c1;
        while (1) {
                in.get(c1);
                if (in.eof()) return 0;
                if (c1 != c) return c1;
        }
}

/** Read a graph "node" from the input.
 *  @param in is the input stream to read from
 *  @param c  is character to look for
 *  @return c1 or 0, on end-of-file
 */
bool Misc::readNode(istream& in, int& u, int n) {
	char c;
        if (n <= 26) {
                if ((c = rflush(in,' ')) == 0) return false;
                u = num(c);
        } else {
                if (!(in >> u)) return false;
        }
	return true;
}

void Misc::writeNode(ostream& os, int u, int n) {
	if (n <= 26) { 	
		if (u != Null) os << nam(u); 
		else os << "-";
	} else { 
		os << setw(2) << u;
	}
}

/** Return true if s1 is a non-empty substring of s2. */
bool Misc::prefix(string s1, string s2) {
	return s1.length() > 0 && s2.find(s1) == 0;
}

/** Read the next non-whitespace character from is.
 *  If it is a lower case letter c, make x = (c+1) - 'a'. 
 *  Otherwise return false. If reach the end of line
 *  before finding a non-whitespace character,
 *  return false without reading the newline character.
 */
bool Misc::readAlpha(istream& in, int& x) {
	char c1, c2;
	while (1) {
		in.get(c1); if (!in.good()) return false;
		if (c1 == '\n') { in.putback(c1); return false; }
		if (isspace(c1)) continue;
		if (!islower(c1)) return false;
		x = num(c1);
		return true;
	}
}

/** Output the character corresponding to the integer x.
 *  That is, output ('a'-1)+x. X is assumed to be in [0,26].
 *  If x==0, output "Null".
 */
void Misc::writeAlpha(ostream& out, int x) {
	assert(0 <= x && x <= 26);
	if (x == 0) out << "Null";
	else out << nam(x);
}

/** Read next word (string containing letters, numbers, underscores, slashes)
 *  on the current line and return it in s. Return true on success,
 *  false on failure.
 */
bool Misc::readWord(istream& in, string& s) {
	char c; bool inword;
	s = ""; inword = false;
	while (1) {
		in.get(c); if (!in.good()) return false;
		if (c == '\n') { in.putback(c); return inword; }
		if (isspace(c)) {
			if (inword) return true;
			else continue;
		}
		if (!isalpha(c) && !isdigit(c) && c != '_' && c != '/') {
			in.putback(c); return inword;
		}
		s += c; inword = true;
	}
}

/** If next thing on current line is a number, read it into i and
 *  return true. Otherwise return false.
 */
bool Misc::readNum(istream& in, int& i) {
	char c; long long j;
	while (1) {
		in.get(c); if (!in.good()) return false;
		if (c == '\n') { in.putback(c); return false; }
		if (isspace(c)) continue;
		if (!isdigit(c) && c != '-') return false;
		in.putback(c);
		if (in >> j) { i = j; return true; }
		else return false;
	}
}

/** If next thing on current line is an unsigned number, read it into i and
 *  return true. Otherwise return false.
 */
bool Misc::readNum(istream& in, uint32_t& i) {
	char c;
	while (1) {
		in.get(c); if (!in.good()) return false;
		if (c == '\n') { in.putback(c); return false; }
		if (isspace(c)) continue;
		if (!isdigit(c)) return false;
		in.putback(c);
		if (in >> i) return true;
		else return false;
	}
}

bool Misc::readNum(istream& in, uint16_t& i) {
	uint32_t j;
	if (readNum(in,j)) { i = j; return true; }
	return false;
}

bool Misc::readNum(istream& in, char& i) {
	uint32_t j;
	if (readNum(in,j)) { i = j; return true; }
	return false;
}

/** Advance to the first non-blank character, skipping over comments.
 *  Leave the non-blank character in the input stream.
 *  A comment is anything that starts with the sharp sign '#' and
 *  continues to the end of the line. Return false on error or eof.
 */
bool Misc::skipBlank(istream& in) {
	char c; bool com;
	com = false;
	while (1) {
		in.get(c); if (!in.good()) return false;
		if (c ==  '#') {com =  true; continue; }
		if (c == '\n') {com = false; continue; }
		if (com || isspace(c)) continue;
		in.putback(c); return true;
	}
}

/** If next character on input stream is the character c, read it and
 *  return true. Otherwise, put it back and return false;
 */
bool Misc::verify(istream& in, char c) {
	char c1;
	in.get(c1); if (!in.good()) return false;
	if (c1 == c) return true;
	in.putback(c1);
	return false;
}

/** Create random permutation on integers 1..n and return in p.
 */
void Misc::genPerm(int n, int p[]) {
        int i, j, k;
        for (i = 1; i <= n; i++) p[i] = i;
        for (i = 1; i <= n; i++) {
                j = randint(i,n);
                k = p[i]; p[i] = p[j]; p[j] = k;
        }
}

/** Replacement for the missing strnlen function.
 */
int Misc::strnlen(char* s, int n) {
	for (int i = 0; i < n; i++) 
		if (*s++ == '\0') return i+1;
	return n;
}

/** Return time expressed as a free-running microsecond clock
 *
 *  Uses the gettimeofday system call, but converts result to
 *  simple microsecond clock for greater convenience.
 */
uint32_t Misc::getTime() {
        /** note use of static variables */
        static uint32_t now;
        static struct timeval prevTimeval = { 0, 0 };

        if (prevTimeval.tv_sec == 0 && prevTimeval.tv_usec == 0) {
                // first call to getTime(); initialize and return 0
                if (gettimeofday(&prevTimeval, NULL) < 0)
                        fatal("Misc::getTime: gettimeofday failure");
                now = 0;
                return 0;
        }
        // normal case
        struct timeval nowTimeval;
        if (gettimeofday(&nowTimeval, NULL) < 0)
                fatal("Misc::getTime: gettimeofday failure");
	uint32_t dsec = nowTimeval.tv_sec; dsec -= prevTimeval.tv_sec;
	uint32_t dusec = nowTimeval.tv_usec - prevTimeval.tv_usec;
	if (nowTimeval.tv_usec < prevTimeval.tv_usec) {
		dusec = nowTimeval.tv_usec + (1000000 - prevTimeval.tv_usec);
		dsec--;
	}
	now += 1000000*dsec + dusec;
        prevTimeval = nowTimeval;

        return now;
}
