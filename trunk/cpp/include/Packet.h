/** @file Packet.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef PACKET_H
#define PACKET_H

#include "Forest.h"
#include "CtlPkt.h"

/** Class that defines fields in Forest packets.
 *  Note, packet fields and buffer are directly accessible. Use with care.
 */
class Packet {
public:
		Packet();
		~Packet();

	int	static const HDRLEN = 5*sizeof(uint32_t);
	int	static const OVERHEAD = HDRLEN + sizeof(uint32_t);

	bool	unpack();
	bool	pack();
	uint32_t* payload() const { return &((*buffer)[5]); };

	/** buffer error checking */
        bool    hdrErrCheck() const; 
        bool    payErrCheck() const;
        void    hdrErrUpdate();    
        void    payErrUpdate();   

	/** input/output */
	bool	read(istream&);
	string&	toString(string&) const;

	// packet fields - note: all public
	int	version;		///< version number field
	int	length;			///< length field
	ptyp_t	type;			///< packet type field
	flgs_t	flags;			///< flags
	comt_t	comtree;		///< comtree field
	fAdr_t	srcAdr;			///< source address
	fAdr_t	dstAdr;			///< destination address
	int	inLink;			///< link on which packet arrived
	ipa_t	tunIp;			///< peer IP addr from substrate header
	ipp_t	tunPort;		///< peer port # from substrate header
	int	bufferLen;		///< number of valid bytes in buffer
	buffer_t* buffer;		///< pointer to packet buffer

private:
};

#endif
