// Header file for header class

#ifndef FHEADER_H
#define FHEADER_H

#include "forest.h"

class header {
public:
	void	unpack(buffer_t&);	// unpack header fields from buffer
	void	pack(buffer_t&);	// pack header fields into buffer

	// access header fields
	int& 	version();		// return version field
	int& 	leng();			// return packet length (in bytes) 
	int& 	hleng();		// return header length (in words)
        ptyp_t&  ptype();      		// return packet type field
        flgs_t&  flags();      		// return flags field
        comt_t& comtree();	     	// return comtree field
	fAdr_t&	srcAdr();		// return source address field
	fAdr_t&	dstAdr();		// return destination address field
	int&	inLink();		// return link on which header arrived
	ipa_t&	tunSrcIp();		// source IP address of tunnel packet
	ipp_t&	tunSrcPort();		// source port number of tunnel packet
	int&	ioBytes();		// number of bytes in buffer

	// buffer error checking
        bool    hdrErrCheck(buffer_t&) const; 
        bool    payErrCheck(buffer_t&) const;
        void    hdrErrUpdate(buffer_t&);    
        void    payErrUpdate(buffer_t&);   

	bool	getPacket(istream&, buffer_t&); // get header from input
	void	print(ostream&, buffer_t&) ; 	// print header
private:
	int	ver;			// version number field
	int	lng;			// length field
	ptyp_t	typ;			// packet type field
	flgs_t	flg;			// flags
	comt_t	comt;			// comtree field
	fAdr_t	sadr;			// source address
	fAdr_t	dadr;			// destination address
	int	inlnk;			// link on which header arrived
	ipa_t	tSrcIp;			// source IP address from substrate header
	ipp_t	tSrcPort;		// source port # from substrate header
	int	iob;			// number of bytes in buffer
};

// Header field access methods - note that references
// are returned, allowing values to be assigned
inline int& header::version()		{ return ver; }
inline int& header::leng()		{ return lng; }
inline ptyp_t& header::ptype()		{ return typ; }
inline flgs_t& header::flags()		{ return flg; }
inline comt_t& header::comtree()	{ return comt; }
inline fAdr_t& header::srcAdr()		{ return sadr; }
inline fAdr_t& header::dstAdr()		{ return dadr; }
inline int& header::inLink()		{ return inlnk; }
inline ipa_t& header::tunSrcIp()	{ return tSrcIp; }
inline ipp_t& header::tunSrcPort()	{ return tSrcPort; }
inline int& header::ioBytes()		{ return iob; }

#endif
