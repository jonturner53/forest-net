/** @file NetBuffer.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef NETBUFFER_H
#define NETBUFFER_H

#include "Forest.h"

namespace forest {


/** This class implements a simple user-space buffer for use when
 *  reading from a stream socket. It provides methods for reading
 *  higher level chunks of data from a socket, while hiding the
 *  system calls required to refill the buffer when necessary.
 *  Note that using a NetBuffer while also reading from socket
 *  directly is asking for trouble.
 */
class NetBuffer {
public:
	// constructors/destructor
		NetBuffer(int,int);
		~NetBuffer();

	// predicates
	bool	full() const;
	bool	empty() const;
	bool	isWordChar(char) const;

	// operations on buffer
	void	clear();
	void	flushBuf(string&);
	bool	readWord(string&);
	bool	readName(string&);
	bool	readString(string&);
	bool	readAlphas(string&);
	bool	readLine(string&);
	bool	readInt(int&);
	bool	readInt(uint64_t&);
	bool	readForestAddress(string&);
	bool	readIpAddress(string&);
	bool	nextLine();
	bool	skipSpace();
	bool	skipSpaceInLine();
	bool	verify(char);

	string& toString(string&);
private:
	int	sock;		///< socket to use when reading
	char*	rp;		///< read pointer (next character to read)
	char*	wp;		///< write pointer (next empty position)

	char*	buf;		///< socket used to store input characters
	int	size;		///< number of bytes in buffer

	// internal helper methods
	void	advance(char*&,int=1);
	bool	refill();
	void	extract(int,string&);
};

inline bool NetBuffer::full() const {
	return wp+1 == rp || (rp == buf && wp == buf+(size-1));
}

inline bool NetBuffer::empty() const { return rp == wp; }

inline void NetBuffer::advance(char*& p, int len) {
	p += len; if (p >= buf+size) p -= size;
}

inline bool NetBuffer::isWordChar(char c) const {
	return isalpha(c) || isdigit(c) || c == '_' || c == '/' ||
               c == '@' || c == '.' || c == '-';
}

} // ends namespace


#endif
