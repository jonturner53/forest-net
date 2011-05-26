/** @file PacketHeader.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKETHEADER_H
#define PACKETHEADER_H

#include "CommonDefs.h"

class PacketHeader {
public:
	void	unpack(buffer_t&);
	void	pack(buffer_t&);

	/** getters */ 
	int 	getVersion() const;
	int 	getLength() const;	
	int 	getHdrLength() const;
        ptyp_t  getPtype() const;
        flgs_t  getFlags() const;
        comt_t 	getComtree() const;
	fAdr_t	getSrcAdr() const;
	fAdr_t	getDstAdr() const;
	int	getInLink() const;
	ipa_t	getTunSrcIp() const;
	ipp_t	getTunSrcPort() const;
	int	getIoBytes() const;

	/** setters */ 
	void 	setVersion(int);
	void 	setLength(int);	
	void 	setHdrLength(int);
        void  	setPtype(ptyp_t);
        void  	setFlags(flgs_t);
        void 	setComtree(comt_t);
	void	setSrcAdr(fAdr_t);
	void	setDstAdr(fAdr_t);
	void	setInLink(int);
	void	setTunSrcIp(ipa_t);
	void	setTunSrcPort(ipp_t);
	void	setIoBytes(int);

	/** buffer error checking */
        bool    hdrErrCheck(buffer_t&) const; 
        bool    payErrCheck(buffer_t&) const;
        void    hdrErrUpdate(buffer_t&);    
        void    payErrUpdate(buffer_t&);   

	/** input/output */
	bool	read(istream&, buffer_t&);
	void	write(ostream&, buffer_t&) const;
private:
	int	ver;			// version number field
	int	lng;			// length field
	ptyp_t	typ;			// packet type field
	flgs_t	flg;			// flags
	comt_t	comt;			// comtree field
	fAdr_t	sadr;			// source address
	fAdr_t	dadr;			// destination address
	int	inlnk;			// link on which packet arrived
	ipa_t	tSrcIp;			// src IP addr from substrate header
	ipp_t	tSrcPort;		// src port # from substrate header
	int	iob;			// number of bytes in buffer
};

/** getters */
inline int PacketHeader::getVersion() const { return ver; }
inline int PacketHeader::getLength() const { return lng; }
inline ptyp_t PacketHeader::getPtype() const { return typ; }
inline flgs_t PacketHeader::getFlags() const { return flg; }
inline comt_t PacketHeader::getComtree() const { return comt; }
inline fAdr_t PacketHeader::getSrcAdr() const { return sadr; }
inline fAdr_t PacketHeader::getDstAdr() const { return dadr; }
inline int PacketHeader::getInLink() const { return inlnk; }
inline ipa_t PacketHeader::getTunSrcIp() const { return tSrcIp; }
inline ipp_t PacketHeader::getTunSrcPort() const { return tSrcPort; }
inline int PacketHeader::getIoBytes() const { return iob; }

/** setters */
inline void PacketHeader::setVersion(int v) { ver = v; }
inline void PacketHeader::setLength(int len) { lng = len; }
inline void PacketHeader::setPtype(ptyp_t t) { typ = t; }
inline void PacketHeader::setFlags(flgs_t f) { flg = f; }
inline void PacketHeader::setComtree(comt_t ct) { comt = ct; }
inline void PacketHeader::setSrcAdr(fAdr_t sa) { sadr = sa; }
inline void PacketHeader::setDstAdr(fAdr_t da) { dadr = da; }
inline void PacketHeader::setInLink(int lnk) { inlnk = lnk; }
inline void PacketHeader::setTunSrcIp(ipa_t sip) { tSrcIp = sip; }
inline void PacketHeader::setTunSrcPort(ipp_t sp) { tSrcPort = sp; }
inline void PacketHeader::setIoBytes(int b) { iob = b; }
#endif
