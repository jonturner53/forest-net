// Header file for header class

#ifndef HEADER_H
#define HEADER_H

#include "lfs.h"
#include "list.h"

class header {
public:
	void	unpack(buffer_t&);	// unpack header fields from buffer
	void	pack(buffer_t&);	// pack header fields into buffer

	// access header fields
	int& 	leng();			// return packet length (in bytes) 
	int& 	hleng();		// return header length (in words)
	int&	proto();		// return protocol field
	ipa_t&	srcAdr();		// return source address field
	ipa_t&	dstAdr();		// return destination address field
	int&	optCode();		// return option code field
	int&	optLeng();		// return option length
	int&	lfsOp();		// return LFS operation field
	int&	lfsFlags();		// return LFS flags
	int& 	lfsRrate();		// return LFS requested rate
	int& 	lfsArate();		// return LFS allocated rate
	int& 	lfsTrace();		// return LFS trace field
	int&	inLink();		// return link on which header arrived
	ipa_t&	srcIp();		// source IP address of tunnel packet
	int&	srcPort();		// source port number of tunnel packet
	int&	ioBytes();		// number of bytes in buffer

	// buffer error checking
        bool    hdrErrCheck(buffer_t&) const; 
        bool    payErrCheck(buffer_t&) const;
        void    hdrErrUpdate(buffer_t&);    
        void    payErrUpdate(buffer_t&);   

	bool	getPacket(istream&, buffer_t&); // get header from input
	void	print(ostream&, buffer_t&) ; 	// print header
private:
	int	lng;			// length field from IP header
	int	hlng;			// header length field
	int	prot;			// protocol field
	ipa_t	sadr;			// source address
	ipa_t	dadr;			// destination address
	int	optcode;		// IP option code
	int	optleng;		// IP option length
	int	lfsop;			// LFS operation code
	int	lfsflags;		// LFS flags
	int	lfsrrate;		// LFS requested rate
	int	lfsarate;		// LFS allocated rate
	int	lfstrace;		// LFS trace field
	int	inlnk;			// link on which header arrived
	ipa_t	sIp;			// source IP address of tunnel packet
	int	sPort;			// source port # of tunnel packet
	int	iob;			// number of bytes in buffer
};

// Header field access methods - note, can be assigned to
inline int& header::leng()	{ return lng; }
inline int& header::hleng()	{ return hlng; }
inline int& header::proto()	{ return prot; }
inline ipa_t& header::srcAdr()	{ return sadr; }
inline ipa_t& header::dstAdr()	{ return dadr; }
inline int& header::optCode()	{ return optcode; }
inline int& header::optLeng()	{ return optleng; }
inline int& header::lfsOp()	{ return lfsop; }
inline int& header::lfsFlags()	{ return lfsflags; }
inline int& header::lfsRrate()	{ return lfsrrate; }
inline int& header::lfsArate()	{ return lfsarate; }
inline int& header::lfsTrace()	{ return lfstrace; }
inline int& header::inLink()	{ return inlnk; }
inline ipa_t& header::srcIp()	{ return sIp; }
inline int& header::srcPort()	{ return sPort; }
inline int& header::ioBytes()	{ return iob; }

#endif
