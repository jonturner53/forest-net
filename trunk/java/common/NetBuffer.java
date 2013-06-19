package forest.common;

/** @file NetBuffer.java
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.lang.*;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.charset.*;

/** This class implements a simple user-space buffer for use when
 *  reading from a stream socket. It provides methods for reading
 *  higher level chunks of data from a socket, while hiding the
 *  library calls required to refill the buffer when necessary.
 *  Note that using a NetBuffer while also reading from socket
 *  directly is asking for trouble.
 */
public class NetBuffer {
	private SocketChannel chan;	///< channel to use when reading
	private ByteBuffer bbuf;	///< temporary byte buffer
	private CharBuffer buf;		///< decoded character buffer
	private int rp;			///< read pointer used by read ops
	private int size;		///< size of the buffers
	private Charset ascii;		///< character set for decoding
	private CharsetDecoder dec;	///< character set for decoding

	/** Constructor for NetBuffer with no initialization.
	 *  @param socket is the number of the socket to use when reading;
	 *  should be an open, blocking stream socket
	 *  @param size is the number of bytes the buffer can hold at once
	 */
	public NetBuffer(SocketChannel chan, int size) {
		this.chan = chan; this.size = size;
		bbuf = ByteBuffer.allocate(size);
		buf = CharBuffer.allocate(size);
		// make buffers look empty
		bbuf.position(bbuf.limit()); buf.position(buf.limit());
		ascii = Charset.forName("US-ASCII");
		dec = ascii.newDecoder();
       		dec.onMalformedInput(CodingErrorAction.REPLACE);
       		dec.onUnmappableCharacter(CodingErrorAction.REPLACE);
	}
	
	public boolean full() {
		return buf.remaining() == buf.capacity();
	}

	public boolean empty() { return buf.remaining() == 0; }
	
	private boolean isWordChar(char c) {
		return Character.isLetter(c) || Character.isDigit(c) ||
			c == '_' || c == '/' || c == '@' || c == '.' ||
			c == '-';
	}

	/** Add more data to the buffer from channel.
	 *  @return false if no space available in buffer or connection was
	 *  closed by peer
	 */
	private boolean refill() {
		if (full()) return false;
		// configure buf to accept more data
		int pos;
		if (buf.position() > buf.capacity()/2 ||
		    buf.limit() == buf.capacity()) {
			rp -= buf.position(); // adjust read pointer
			buf.compact();
			pos = 0;
		} else {
			pos = buf.position();
			buf.position(buf.limit());
			buf.limit(buf.capacity());
		}
		// read bytes into bbuf, but no more than buf can take
		bbuf.clear(); bbuf.limit(buf.remaining());
		int n;
		try {
			n = chan.read(bbuf);
		} catch(Exception e) {
			// reconfigure buf to resume get operations
			buf.limit(buf.position()); buf.position(pos);
			return false;
		}
		if (n <= 0) {
			// reconfigure buf to resume get operations
			buf.limit(buf.position()); buf.position(pos);
			return false;
		}
		// decode ascii characters in bbuf into chars in buf
		bbuf.flip();
		dec.decode(bbuf,buf,false);

		// reconfigure buf to resume get operations
		buf.limit(buf.position()); buf.position(pos);
		return true;
	}
	
	/** Extract and return a portion of buffer contents.
	 *  @param len is the number of characters to extract
	 *  @return a string containing the extracted characters
	 */
	private String extract(int len) {
		CharBuffer sub = buf.slice();
		sub.limit(sub.position()+len);
		String s = sub.toString();
		buf.position(buf.position()+len);
		return s;
	}

	/** Skip white space in the buffer.
	 *  Advances rp to first non-space character.
	 *  @return true if successful, false if connection terminates before
	 *  delivering a non-space character.
	 */
	public boolean skipSpace() {
		rp = buf.position();
		while (true) {
			if (rp == buf.limit() && !refill()) return false;
			if (!Character.isWhitespace(buf.get(rp))) break;
			rp++;
		}
		buf.position(rp); return true;
	}
	
	/** Skip white space in the current line.
	 *  Advances rp to first non-space character or newline.
	 *  @return true if successful, false if connection terminates before
	 *  delivering a non-space character or newline.
	 */
	public boolean skipSpaceInLine() {
		rp = buf.position();
		while (true) {
			if (rp == buf.limit() && !refill()) return false;
			char c = buf.get(rp);
			if (!Character.isWhitespace(c) || c == '\n') break;
			rp++;
		}
		buf.position(rp); return true;
	}
	
	/** Read a line of input.
	 *  We scan the buffer for a newline and return all characters
	 *  up to the newline (but not including it). Lines are assumed to
	 *  be no longer than the size of the buffer. If no newline is found
	 *  even when the buffer is full, the operation fails and the contents
	 *  of the buffer is not changed.
	 *  @return true on success; operation fails if there is an error
	 *  on the socket, if eof is encountered before a newline, or if
	 *  the buffer fills before a newline is received.
	 */
	public String readLine() {
		rp = buf.position(); int len = 0;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			if (buf.get(rp) == '\n') {
				String line = extract(len);
				buf.get();
				return line;
			}
			rp++; len++;
		}
	}
	
	/** Read next word.
	 *  A word contains letters, numbers, underscores, @-signs, periods,
	 *  slashes.
	 *  @return a string containing the next word on success, or null
 	 *  on failure.
	 */
	public String readWord() {
		if (!skipSpace()) return null;
		rp = buf.position();
		if (!isWordChar(buf.get(rp))) return null;
		int len = 0;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			if (!isWordChar(buf.get(rp))) {
				if (len == 0) return null;
				return extract(len);
			}
			rp++; len++;
		}
	}
	
	/** Read next non-blank of alphabetic characters.
	 *  @param s is a reference to a string in which result is returned
	 *  @return true on success, false on failure
	 */
	public String readAlphas() {
		if (!skipSpace()) return null;
		rp = buf.position();
		if (!Character.isLetter(buf.get(rp))) return null;
		int len = 0;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			if (!Character.isLetter(buf.get(rp))) {
				if (len == 0) return null;
				return extract(len);
			}
			rp++; len++;
		}
	}
	
	/** Read a name from the channel.
	 *  Here a name starts with a letter and may also contain digits and
	 *  underscores.
	 *  @param s is a reference to a string in which result is returned
	 *  @return true on success, false on failure
	 *  on the current line and return it in s. Return true on success,
	 *  false on failure.
	 */
	public String readName() {
		if (!skipSpace()) return null;
		rp = buf.position();
		if (!Character.isLetter(buf.get(rp))) return null;
		int len = 0;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			char c = buf.get(rp);
			if (!Character.isLetter(c) && !Character.isDigit(c) &&
			    c != '_') {
				if (len == 0) return null;
				return extract(len);
			}
			rp++; len++;
		}
	}
	
	/** Read next string (enclosed in double quotes).
	 *  The string may not contain double quotes. The quotes are not
	 *  included in the string that is returned.
	 *  @param s is a reference to a string in which result is returned
	 *  @return true on success, false on failure
	 */
	public String readString() {
		if (!skipSpace()) return null;
		rp = buf.position();
		if (buf.get(rp) != '\"') return null;
		rp++; buf.get(); int len = 0;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			if (buf.get(rp) == '\"') {
				String s = extract(len);
				buf.get();
				return s;
			}
			len++; rp++;
		}
	}
	
	/** Read an integer from the channel.
	 *  Initial white space is skipped and input terminates on first
	 *  non-digit following a digit string.
	 *  @return an Integer object on success, null on failure
	 */
	public Integer readInt() {
		if (!skipSpace()) return null;
		rp = buf.position();
		char c = buf.get(rp);
		if (!Character.isDigit(c) && c != '-') return null;
		rp++; int len = 1;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			if (!Character.isDigit(buf.get(rp))) {
				if (len == 0) return null;
				String s = extract(len);
				return Integer.parseInt(s);
			}
			len++; rp++;
		}
	}

	public Long readLong() {
		if (!skipSpace()) return null;
		rp = buf.position();
		char c = buf.get(rp);
		if (!Character.isDigit(c) && c != '-') return null;
		rp++; int len = 1;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			if (!Character.isDigit(buf.get(rp))) {
				if (len == 0) return null;
				String s = extract(len);
				return Long.parseLong(s);
			}
			len++; rp++;
		}
	}
	
	/** Read a Forest unicast address and return it as a string.
	 *  Initial white space is skipped and input terminates on first
	 *  non-digit following the second part of the address.
	 *  @return a string containing the address on success, null on
	 *  failure
	 */
	public String readForestAddress() {
		if (!skipSpace()) return null;
		rp = buf.position();
		if (!Character.isDigit(buf.get(rp))) return null;
		int len = 0; int dotCount = 0;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			char c = buf.get(rp);
			if (c == '.' && dotCount < 1) {
				dotCount++;
			} else if (!Character.isDigit(c) && c != '.') {
				if (len == 0) return null;
				return extract(len);
			}
			len++; rp++;
		}
	}
	
	/** Read an Ip address in dotted decimal form and return it as a string.
	 *  Initial white space is skipped and input terminates on first
	 *  non-digit following the last part of the address.
	 *  @param s is a reference to a string in which result is returned
	 *  @return true on success, false on failure
	 */
	public String readIpAddress() {
		if (!skipSpace()) return null;
		rp = buf.position();
		if (!Character.isDigit(buf.get(rp))) return null;
		int len = 0; int dotCount = 0;
		while (true) {
			if (rp == buf.limit() && !refill()) return null;
			char c = buf.get(rp);
			if (c == '.' && dotCount < 3) {
				dotCount++;
			} else if (!Character.isDigit(c) && c != '.') {
				if (len == 0) return null;
				return extract(len);
			}
			len++; rp++;
		}
	}

	public RateSpec readRspec() {
		Integer bru, brd, pru, prd;
		if (!verify('(')) return null;
		bru = readInt(); if (bru == null) return null;
		if (!verify(',')) return null;
		brd = readInt(); if (brd == null) return null;
		if (!verify(',')) return null;
		pru = readInt(); if (pru == null) return null;
		if (!verify(',')) return null;
		prd = readInt(); if (prd == null) return null;
		if (!verify(')')) return null;
		return (new RateSpec(bru,brd,pru,prd));
	}
	
	/** Verify next non-space character.
	 *  @param c is a non-space character
	 *  @return true if the next non-space character on the current line
	 *  is equal to c, else false
	 */
	public boolean verify(char c) {
		if (!skipSpaceInLine()) return false;
		if (buf.get(buf.position()) != c) return false;
		buf.get(); return true;
	}
	
	public boolean verifyNext(char c) {
		if (buf.get(buf.position()) != c) return false;
		buf.get(); return true;
	}

	/** Advance to next line of input
	 *  @return true if a newline is found in the input, else false
	 */
	public boolean nextLine() {
		rp = buf.position();
		while (true) {
			if (rp == buf.limit() && !refill()) return false;
			if (buf.get(rp) == '\n') break;
			rp++;
		}
		buf.position(rp+1); return true;
	}
	
	/** Flush the remaining contents from the buffer.
	 *  @return the string with the remaining contents of the buffer.
	 */
	public String flushBuf() {
		String s = buf.toString(); buf.clear(); return s;
	}
	
	/** Clear the buffer, discarding contents.
	 */
	public void clear() { buf.clear(); }
	
	public String toString() { return buf.toString(); }
	
	} // ends namespace
	
