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
#include "ListPair.h"

using namespace grafalgo;

namespace forest {

class IfaceTable {
public:
	///< iface table entry
	class Entry {
	public:
	ipa_t	ipa;			///< IP address of interface
	ipp_t	port;			///< port number of interface
	int	sock;			///< socket number of interface
	RateSpec rates;			///< total rates for interface
	RateSpec availRates;		///< available rates for interface

	string	toString() const;
	friend	ostream& operator<<(ostream& out, const Entry& a) {
                return out << a.toString();
        }
	};

		IfaceTable(int);
		~IfaceTable();

	// predicates 
	bool 	valid(int) const;	

	// iteration methods
	int	firstIface() const;
	int	nextIface(int) const;

	// access methods 
	Entry&	getEntry(int);
	int	getDefaultIface() const;
	int	getFreeIface() const;

	// modifiers 
	bool	addEntry(int,ipa_t,ipp_t,RateSpec&);
	void	removeEntry(int);
	void	setDefaultIface(int);

	// io routines
	bool	read(istream&);
	string	toString() const;

	string entry2string(int) const;
private:
	int	maxIf;			///< largest interface number
	int	defaultIf;		///< default interface
	Entry *ift;			///< ift[i]=data for interface i
	ListPair *ifaces;		///< in-use and available iface numbers

	// helper methods
	int	readEntry(istream&);		

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

/** Get a table entry.
 *  @param iface is a valid interface number
 *  @return a reference to the interface table entry
 */
inline IfaceTable::Entry& IfaceTable::getEntry(int iface) {
	if (!valid(iface)) {
		string s = "IfaceTable::Entry::getEntry:: invalid interface "
			   + to_string(iface) + "\n";
                throw IllegalArgumentException(s);
	}
	return ift[iface];
}

/** Set the default interface.
 *  @param iface is the number of a valid existing interface
 */
inline void IfaceTable::setDefaultIface(int iface) {
	if (valid(iface)) defaultIf = iface;
}

} // ends namespace


#endif
