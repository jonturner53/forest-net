/** @file IfaceTable.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef IFACETABLE_H
#define IFACETABLE_H

#include "Forest.h"
#include "RateSpec.h"
#include "UiSetPair.h"

namespace forest {


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
	RateSpec& getRates(int) const;
	RateSpec& getAvailRates(int) const;
	int	getDefaultIface() const;
	int	getFreeIface() const;

	// modifiers 
	bool	addEntry(int,ipa_t,RateSpec&);
	void	removeEntry(int);
	void	setDefaultIface(int);

	// io routines
	bool	read(istream&);
	string&	toString(string&) const;
private:
	int	maxIf;			///< largest interface number
	int	defaultIf;		///< default interface
	struct IfaceInfo {		///< information describing an iface
	ipa_t	ipa;			///< IP address of interface
	int	sock;			///< socket number of interface
	RateSpec rates;			///< total rates for interface
	RateSpec availRates;		///< available rates for interface
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

/** Get the maximum rates allowed for this interface.
 *  @param i is the interface number
 *  @return the a reference to the rate spec for the interface; currently
 *  interface rates are symmetric and upstream and downstream fields
 *  must be set to identical values.
 */
inline RateSpec& IfaceTable::getRates(int i) const { return ift[i].rates; }

/** Get the available rates allowed for this interface.
 *  @param i is the interface number
 *  @return the a reference to the available rate spec for the interface;
 *  currently rates are symmetric and upstream and downstream fields
 *  must be set to identical values.
 */
inline RateSpec& IfaceTable::getAvailRates(int i) const {
	return ift[i].availRates;
}

/** Set the default interface.
 *  @param iface is the number of a valid existing interface
 */
inline void IfaceTable::setDefaultIface(int iface) {
	if (valid(iface)) defaultIf = iface;
}

} // ends namespace


#endif
