/** @file NetBuffer.cpp
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "NetBuffer.h"

namespace forest {

/** Constructor for NetBuffer with no initialization.
 *  @param socket is the number of the socket to use when reading;
 *  should be an open, blocking stream socket
 *  @param size is the number of bytes the buffer can hold at once
 */
NetBuffer::NetBuffer(int socket, int size1) : sock(socket), size(size1) { 
	buf = new char[size];
	rp = wp = buf;
}

NetBuffer::~NetBuffer() { delete buf; }

/** Add more data to the buffer from socket.
 *  @return false if no space available in buffer or connection was
 *  closed by peer
 */
bool NetBuffer::refill() {
	if (full()) return false;
	int n, len;
	if (wp < rp) {
		len = (rp-1) - wp;
	} else {
		len = buf+size - wp;
		if (rp == buf) len--;
	}
	n = recv(sock,wp,len,0);
	if (n <= 0) return false;
	advance(wp,n);
	return true;
}

/** Extract and return a portion of buffer contents.
 *  @param len is the number of characters to extract
 *  @param s is a string in which the extracted characters are returned
 */
void NetBuffer::extract(int len, string& s) {
	if (rp+len <= buf+size) {
		s.assign(rp,len);
	} else {
		int len1 = buf+size-rp;
		s.assign(rp,len1);
		s.append(buf,len-len1);
	}
	advance(rp,len);
}

/** Skip white space in the buffer.
 *  Advances rp to first non-space character.
 *  @return true if successful, false if connection terminates before
 *  delivering a non-space character.
 */
bool NetBuffer::skipSpace() {
	char* p = rp;
	while (true) {
		if (p == wp && !refill()) return false;
		if (!isspace(*p)) break;
		advance(p);
	}
	rp = p; return true;
}

/** Skip white space in the current line.
 *  Advances rp to first non-space character or newline.
 *  @return true if successful, false if connection terminates before
 *  delivering a non-space character or newline.
 */
bool NetBuffer::skipSpaceInLine() {
	char* p = rp;
	while (true) {
		if (p == wp && !refill()) return false;
		if (!isspace(*p) || *p == '\n') break;
		advance(p);
	}
	rp = p; return true;
}

/** Read a line of input.
 *  We scan the buffer for a newline and return all characters
 *  up to the newline (but not including it). Lines are assumed to
 *  be no longer than the size of the buffer. If no newline is found
 *  even when the buffer is full, the operation fails and the contents
 *  of the buffer is not changed.
 *  @param line is a reference to a string in which result is returned
 *  @return true on success; operation fails if there is an error
 *  on the socket, if eof is encountered before a newline, or if
 *  the buffer fills before a newline is received.
 */
bool NetBuffer::readLine(string& line) {
	char* p = rp; int len = 0;
	while (true) {
		if (p == wp && !refill()) return false;
		if (*p == '\n') {
			extract(len,line);
			rp++; if (rp >= buf+size) rp = buf;
			return true;
		}
		len++; advance(p);
	}
}

/** Read next word.
 *  A word contains letters, numbers, underscores, @-signs, periods, slashes.
 *  @param s is a reference to a string in which result is returned
 *  @return true on success, false on failure
 *  on the current line and return it in s. Return true on success,
 *  false on failure.
 */
bool NetBuffer::readWord(string& s) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (!isWordChar(*p)) return false;
	int len = 0;
	while (true) {
		if (p == wp && !refill()) return false;
		if (!isWordChar(*p)) {
			if (len == 0) return false;
			extract(len,s); return true;
		}
		advance(p); len++;
	}
}

/** Read next non-blank of alphabetic characters.
 *  @param s is a reference to a string in which result is returned
 *  @return true on success, false on failure
 */
bool NetBuffer::readAlphas(string& s) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (!isalpha(*p)) return false;
	int len = 0;
	while (true) {
		if (p == wp && !refill()) {
			return false;
		}
		if (!isalpha(*p)) {
			if (len == 0) return false;
			extract(len,s);
			return true;
		}
		advance(p); len++;
	}
}

/** Read a name from the socket.
 *  Here a name starts with a letter and may also contain digits and
 *  underscores.
 *  @param s is a reference to a string in which result is returned
 *  @return true on success, false on failure
 *  on the current line and return it in s. Return true on success,
 *  false on failure.
 */
bool NetBuffer::readName(string& s) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (!isalpha(*p)) return false;
	int len = 0;
	while (true) {
		if (p == wp && !refill()) return false;
		if (!isalpha(*p) && !isdigit(*p) && *p != '_') {
			if (len == 0) return false;
			extract(len,s); return true;
		}
		advance(p); len++;
	}
}

/** Read next string (enclosed in double quotes).
 *  The string may not contain double quotes. The quotes are not
 *  included in the string that is returned.
 *  @param s is a reference to a string in which result is returned
 *  @return true on success, false on failure
 */
bool NetBuffer::readString(string& s) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (*p != '\"') return false;
	advance(p); rp = p; int len = 0;
	while (true) {
		if (p == wp && !refill()) return false;
		if (*p == '\"') {
			extract(len,s); advance(rp); return true;
		}
		len++; advance(p);
	}
}

/** Read an integer from the socket.
 *  Initial white space is skipped and input terminates on first non-digit
 *  following a digit string.
 *  @param i is a reference to an integer in which result is returned
 *  @return true on success, false on failure
 */
bool NetBuffer::readInt(int& i) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (!isdigit(*p) && *p != '-') return false;
	advance(p); int len = 1;
	while (true) {
		if (p == wp && !refill()) return false;
		if (!isdigit(*p)) {
			if (len == 0) return false;
			string s; extract(len,s);
			istringstream ss(s); ss >> i;
			return true;
		}
		len++; advance(p);
	}
}

bool NetBuffer::readInt(uint64_t& i) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (!isdigit(*p) && *p != '-') return false;
	advance(p); int len = 1;
	while (true) {
		if (p == wp && !refill()) return false;
		if (!isdigit(*p)) {
			if (len == 0) return false;
			string s; extract(len,s);
			istringstream ss(s); ss >> i;
			return true;
		}
		len++; advance(p);
	}
}

/** Read a Forest unicast address and return it as a string.
 *  Initial white space is skipped and input terminates on first non-digit
 *  following the second part of the address.
 *  @param s is a reference to a string in which result is returned
 *  @return true on success, false on failure
 */
bool NetBuffer::readForestAddress(string& s) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (!isdigit(*p)) return false;
	int len = 0; int dotCount = 0;
	while (true) {
		if (p == wp && !refill()) return false;
		if (*p == '.' && dotCount < 1) {
			dotCount++;
		} else if (!isdigit(*p) && *p != '.') {
			if (len == 0) return false;
			extract(len,s);
			return true;
		}
		len++; advance(p);
	}
}

/** Read an Ip address in dotted decimal form and return it as a string.
 *  Initial white space is skipped and input terminates on first non-digit
 *  following the last part of the address.
 *  @param s is a reference to a string in which result is returned
 *  @return true on success, false on failure
 */
bool NetBuffer::readIpAddress(string& s) {
	if (!skipSpace()) return false;
	char* p = rp;
	if (!isdigit(*p)) return false;
	int len = 0; int dotCount = 0;
	while (true) {
		if (p == wp && !refill()) return false;
		if (*p == '.' && dotCount < 3) {
			dotCount++;
		} else if (!isdigit(*p) && *p != '.') {
			if (len == 0) return false;
			extract(len,s);
			return true;
		}
		len++; advance(p);
	}
}

/** Verify next character.
 *  @param c is a non-space character
 *  @return true if the next non-space character on the current line
 *  is equal to c, else false
 */
bool NetBuffer::verify(char c) {
	if (!skipSpaceInLine()) return false;
	if (*rp != c) return false;
	advance(rp); return true;
}

/** Advance to next line of input
 *  @return true if a newline is found in the input, else false
 */
bool NetBuffer::nextLine() {
	char* p = rp;
	while (true) {
		if (p == wp && !refill()) return false;
		if (*p == '\n') break;
		advance(p);
	}
	rp = p; advance(rp); return true;
}

/** Flush the remaining contents from the buffer.
 *  @param leftOver is a reference to a string in which unread buffer
 *  contents is returned.
 */
void NetBuffer::flushBuf(string& leftOver) {
	int len = (rp <= wp ? wp-rp : size - (rp-wp));
	extract(len,leftOver);
	rp = wp = 0;
}

/** Clear the buffer, discarding contents.
 */
void NetBuffer::clear() { rp = wp = 0; }

string& NetBuffer::toString(string& s) {
	stringstream ss;
	ss << "rp=" << (rp-buf) << " wp=" << (wp-buf) << endl;
	s = ss.str();
cerr << "toString s=" << s;
	if (rp <= wp) {
		s.append(rp,wp-rp);
cerr << "toString s=" << s;
	} else {
		int len1 = buf+size-rp;
		s.append(rp,len1);
		s.append(buf,wp-buf);
cerr << "toString s=" << s;
	}
	s += "\n";
	return s;
}

} // ends namespace

