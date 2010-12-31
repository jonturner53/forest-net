#include "misc.h"

char misc::cflush(istream& is, char c) {
// Read up to first occurrence of c or eof.
// If eof encountered return 0, else return c;
        char c1;
        while (1) {
                is.get(c1);
                if (is.eof()) return 0;
                if (c1 == c) return c;
        }
}

char misc::rflush(istream& is, char c) {
// Read up to first occurrence of character !=c or eof.
// If eof encountered return 0, else return c;
        char c1;
        while (1) {
                is.get(c1);
                if (is.eof()) return 0;
                if (c1 != c) return c1;
        }
}

bool misc::getNode(istream& is, int& u, int n) {
	char c;
        if (n <= 26) {
                if ((c = rflush(is,' ')) == 0) return false;
                u = num(c);
        } else {
                if (!(is >> u)) return false;
        }
	return true;
}

void misc::putNode(ostream& os, int u, int n) {
	if (n <= 26) { 	
		if (u != Null) os << nam(u); 
		else os << "-";
	} else { 
		os << setw(2) << u;
	}
}

bool misc::prefix(string s1, string s2) {
// Return true if s1 is a non-empty substring of s2.
	return s1.length() > 0 && s2.find(s1) == 0;
}

bool misc::getAlpha(istream& is, int& x) {
// Read the next non-whitespace character from is.
// If it is a lower case letter c, make x = (c+1) - 'a'. 
// Otherwise return false. If reach the end of line
// before finding a non-whitespace character,
// return false without reading the newline character.
	char c1, c2;
	while (1) {
		is.get(c1); if (!is.good()) return false;
		if (c1 == '\n') { is.putback(c1); return false; }
		if (isspace(c1)) continue;
		if (!islower(c1)) return false;
		x = num(c1);
		return true;
	}
}

void misc::putAlpha(ostream& os, int x) {
// Output the character corresponding to the integer x.
// That is, output ('a'-1)+x. X is assumed to be in [0,26].
// If x==0, output "Null".
	assert(0 <= x && x <= 26);
	if (x == 0) os << "Null";
	else os << nam(x);
}

bool misc::getWord(istream& is, string& s) {
// Retrieve next word (string containing letters, numbers, underscores, slashes)
// on the current line and return it in s. Return true on success,
// false on failure.
	char c; bool inword;
	s = ""; inword = false;
	while (1) {
		is.get(c); if (!is.good()) return false;
		if (c == '\n') { is.putback(c); return inword; }
		if (isspace(c)) {
			if (inword) return true;
			else continue;
		}
		if (!isalpha(c) && !isdigit(c) && c != '_' && c != '/') {
			is.putback(c); return inword;
		}
		s += c; inword = true;
	}
}

bool misc::getNum(istream& is, int& i) {
// If next thing on current line is a number, read it into i and
// return true. Otherwise return false.
	char c; long long j;
	while (1) {
		is.get(c); if (!is.good()) return false;
		if (c == '\n') { is.putback(c); return false; }
		if (isspace(c)) continue;
		if (!isdigit(c) && c != '-') return false;
		is.putback(c);
		if (is >> j) { i = j; return true; }
		else return false;
	}
}

bool misc::getNum(istream& is, uint32_t& i) {
// If next thing on current line is a number, read it into i and
// return true. Otherwise return false.
	char c;
	while (1) {
		is.get(c); if (!is.good()) return false;
		if (c == '\n') { is.putback(c); return false; }
		if (isspace(c)) continue;
		if (!isdigit(c)) return false;
		is.putback(c);
		if (is >> i) return true;
		else return false;
	}
}

bool misc::getNum(istream& is, uint16_t& i) {
	uint32_t j;
	if (getNum(is,j)) { i = j; return true; }
	return false;
}

bool misc::skipBlank(istream& is) {
// Advance to the first non-blank character, skipping over comments.
// Leave the non-blank character in the input stream.
// A comment is anything that starts with the sharp sign '#' and
// continues to the end of the line. Return false on error or eof.
	char c; bool com;
	com = false;
	while (1) {
		is.get(c); if (!is.good()) return false;
		if (c ==  '#') {com =  true; continue; }
		if (c == '\n') {com = false; continue; }
		if (com || isspace(c)) continue;
		is.putback(c); return true;
	}
}

bool misc::verify(istream& is, char c) {
// If next character on input stream is the character c, read it and
// return true. Otherwise, put it back and return false;
	char c1;
	is.get(c1); if (!is.good()) return false;
	if (c1 == c) return true;
	is.putback(c1);
	return false;
}

bool misc::getIpAdr(istream& is, ipa_t& ipa) {
// If next thing on the current line is an ip address,
// return it in ipa and return true. Otherwise, return false.
// The expected format is dotted-decimal.
// The address is returned in host byte order.
	int b1, b2, b3, b4;
	
	if (!getNum(is,b1) || !verify(is,'.') ||
	    !getNum(is,b2) || !verify(is,'.') ||
	    !getNum(is,b3) || !verify(is,'.') || !getNum(is,b4))
		return false;
	ipa = ((b1 & 0xff) << 24) | ((b2 & 0xff) << 16) | 
	      ((b3 & 0xff) <<  8) |  (b4 & 0xff);
	return true;
}

void misc::genPerm(int n, int p[]) {
// Create random permutation on integers 1..n and return in p.
        int i, j, k;
        for (i = 1; i <= n; i++) p[i] = i;
        for (i = 1; i <= n; i++) {
                j = randint(i,n);
                k = p[i]; p[i] = p[j]; p[j] = k;
        }
}
