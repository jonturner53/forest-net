/** @file IfaceTable.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef IFACETABLE_H
#define IFACETABLE_H

#include "CommonDefs.h"
#include "UiSetPair.h"

class IfaceTable {
public:
		IfaceTable(int);
		~IfaceTable();

	// predicates 
	bool 	valid(int) const;	

	// iteration methods
	int	firstIface() const;
	int	nextIface(int) const;

	// access methods 
	ipa_t	getIpAdr(int) const;	
	int	getMaxBitRate(int) const;
	int	getMaxPktRate(int) const;
	int	getAvailBitRate(int) const;
	int	getAvailPktRate(int) const;
	int	getDefaultIface() const;
	int	getFreeIface() const;

	// modifiers 
	bool	addEntry(int,ipa_t,int,int);
	void	removeEntry(int);
	void	setDefaultIface(int);
	void	setMaxBitRate(int, int);
	void	setMaxPktRate(int, int);
	bool	setAvailBitRate(int, int);
	bool	setAvailPktRate(int, int);
	bool	addAvailBitRate(int, int);
	bool	addAvailPktRate(int, int);

	// io routines
	bool	read(istream&);
	string&	toString(string&) const;
private:
	int	maxIf;			///< largest interface number
	int	defaultIf;		///< default interface
	struct IfaceInfo {		///< information describing an iface
	ipa_t	ipa;			///< IP address of interface
	int	sock;			///< socket number of interface
	int	maxbitrate;		///< max bit rate for interface (Kb/s)
	int	maxpktrate;		///< max packet rate for interface
	int	avbitrate;		///< max bit rate for interface (Kb/s)
	int	avpktrate;		///< max packet rate for interface
	};
	IfaceInfo *ift;			///< ift[i]=data for interface i
	UiSetPair *ifaces;		///< in-use and available iface numbers

	// helper methods
	int	readEntry(istream&);		
	string& entry2string(int, string&) const;
};

/** Check an interface number for validity.
 *  @param iface is the interface to be checked
 *  @return true if iface is a valid interface number, else false
 */
inline bool IfaceTable::valid(int iface) const {
	return ifaces->isIn(iface);
}

/** Get the "first" interface number.
 *  This method used to iterate through all the interfaces.
 *  The choice of which interface is first is arbitrary and
 *  programs should not rely on any particular iterface being the first.
 *  @return the interface number of the first interface, or 0 if no
 *  interfaces have been defined for this router
 */
inline int IfaceTable::firstIface() const {
	return ifaces->firstIn();
}

/** Get the "next" interface number.
 *  This method used to iterate through all the interfaces.
 *  The order of the interfaces is arbitrary and programs
 *  should not rely on any particular order.
 *  @param iface is a valid interface number
 *  @return the next interface number following iface, or 0 if
 *  there is no next interface, or iface is not valid
 */
inline int IfaceTable::nextIface(int iface) const {
	return ifaces->nextIn(iface);
}

/** Get the number of the default interface.
 *  At the time the first interface is added, the default interface
 *  is initialized with the same interface number. The default
 *  interface can be changed using the setDefaultIface() method.
 *  @return the interface number of the default interface, or 0
 *  if there is no default interface
 */
inline int IfaceTable::getDefaultIface() const {
	return defaultIf;
}

/** Get an an unused interface number.
 *  @return the number of an interface that is not yet in use, or 0
 *  if there are no available interface numbers
 */
inline int IfaceTable::getFreeIface() const {
	return ifaces->firstOut();
}

/** Get the IP address associated with a given interface.
 *  @param iface is the interface number
 *  @return the associated IP address
 */
inline ipa_t IfaceTable::getIpAdr(int iface) const { return ift[iface].ipa; }	

/** Get the maximum bit rate allowed for this interface.
 *  @param i is the interface number
 *  @return the bit rate in Kb/s
 */
inline int IfaceTable::getMaxBitRate(int i) const { return ift[i].maxbitrate; }

/** Get the maximum packet rate allowed for this interface.
 *  @param i is the interface number
 *  @return the packet rate in p/s
 */
inline int IfaceTable::getMaxPktRate(int i) const { return ift[i].maxpktrate; }

/** Get the available bit rate for this interface.
 *  @param i is the interface number
 *  @return the available bit rate in Kb/s
 */
inline int IfaceTable::getAvailBitRate(int i) const {
	return ift[i].avbitrate;
}

/** Get the available packet rate for this interface.
 *  @param i is the interface number
 *  @return the available packet rate in p/s
 */
inline int IfaceTable::getAvailPktRate(int i) const {
	return ift[i].avpktrate;
}

/** Set the default interface.
 *  @param iface is the number of a valid existing interface
 */
inline void IfaceTable::setDefaultIface(int iface) {
	if (valid(iface)) defaultIf = iface;
}

/** Set the maximum bit rate for an interface.
 *  If specified rate is outside the allowed range, the rate is set to
 *  the endpoint of the range.
 *  @param iface is a valid interface number
 *  @param r is the max bit rate in Kb/s
 */
inline void IfaceTable::setMaxBitRate(int iface, int r) {
	ift[iface].maxbitrate = min(max(r,Forest::MINBITRATE),
					  Forest::MAXBITRATE);
}

/** Set the maximum packet rate for an interface.
 *  If specified rate is outside the allowed range, the rate is set to
 *  the endpoint of the range.
 *  @param iface is a valid interface number
 *  @param r is the max packet rate in Kb/s
 */
inline void IfaceTable::setMaxPktRate(int iface, int r) {
	ift[iface].maxpktrate = min(max(r,Forest::MINPKTRATE),
					  Forest::MAXPKTRATE);
}

/** Set the available bit rate for this interface.
 *  Operation fails if one attempts to over-allocate the interface.
 *  @param iface is the interface number
 *  @param r is the available bit rate in Kb/s
 *  @return true on success, false on failure
 */
inline bool IfaceTable::setAvailBitRate(int iface, int r) {
	if (r > ift[iface].maxbitrate) return false;
	ift[iface].avbitrate = max(0,r);
	return true;
}

/** Set the available packet rate for this interface.
 *  Operation fails if one attempts to over-allocate the interface.
 *  @param i is the interface number
 *  @param r is the available packet rate in p/s
 *  @return true on success, false on failure
 */
inline bool IfaceTable::setAvailPktRate(int iface, int r) {
	if (r > ift[iface].maxpktrate) return false;
	ift[iface].avpktrate = max(0,r);
	return true;
}

/** Add to the available bit rate for this interface.
 *  Operation fails if one attempts to over-allocate the interface.
 *  @param iface is the interface number
 *  @param r is the amount to add in Kb/s
 *  @return true on success, false on failure
 */
inline bool IfaceTable::addAvailBitRate(int iface, int r) {
	int s = r + ift[iface].avbitrate;
	if (s < 0 || s > ift[iface].maxbitrate) return false;
	ift[iface].avbitrate = s;
	return true;
}

/** Add to the available packet rate for this interface.
 *  Operation fails if one attempts to over-allocate the interface.
 *  @param iface is the interface number
 *  @param r is the amount to add in p/s
 *  @return true on success, false on failure
 */
inline bool IfaceTable::addAvailPktRate(int iface, int r) {
	int s = r + ift[iface].avpktrate;
	if (s < 0 || s > ift[iface].maxpktrate) return false;
	ift[iface].avpktrate = s;
	return true;
}

#endif
