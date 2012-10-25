/** @file IfaceTable.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "IfaceTable.h"

/** Constructor for IfaceTable, allocates space and initializes private data.
 */
IfaceTable::IfaceTable(int maxIf1) : maxIf(maxIf1) {
	ift = new IfaceInfo[maxIf+1];
	ifaces = new UiSetPair(maxIf);
	defaultIf = 0;
}

/** Destructor for IfaceTable, deletes allocated space */
IfaceTable::~IfaceTable() { delete [] ift; delete ifaces; }

/** Allocate and initialize a new interface table entry.
 *  @param iface is an interface number for an unused interface
 *  @param ipa is the IP address to be associated with the interface
 *  @param brate is the bitrate of the interface
 *  @param prate is the packet rate of the interface
 *  Return true on success, false on failure
 */
bool IfaceTable::addEntry(int iface, ipa_t ipa, int brate, int prate) {
	if (!ifaces->isOut(iface)) return false;
	if (ifaces->firstIn() == 0) // this is the first iface
		defaultIf = iface;
	ifaces->swap(iface);
	ift[iface].ipa = ipa;
	ift[iface].maxbitrate = brate; ift[iface].maxpktrate = prate;
	ift[iface].avbitrate = brate; ift[iface].avpktrate = prate;
	return true;
}

/** Remove an interface from the table.
 *  @param iface is the number of the interface to be removed
 *  No action is taken if the specified interface number is not valid
 */
void IfaceTable::removeEntry(int iface) {
	if (ifaces->isIn(iface)) ifaces->swap(iface);
	if (iface == defaultIf) defaultIf = 0;
}

/** Read a table entry from an input stream and add it to the table.
 *  The input stream is assumed to be positioned at the start of
 *  an interface table entry. An entry is consists of an interface
 *  number, an IP address followed by a colon and port number,
 *  a maximum bit rate (in Kb/s) and a maximum packet rate (in p/s).
 *  Each field is separated by one or more spaces.
 *  Comments in the input stream are ignored. A comment starts with
 *  a # sign and continues to the end of the line. Non-blank lines that do
 *  not start with a comment are expected to contain a complete entry.
 *  If the input is formatted incorrectly, or the interface number
 *  specified in the input is already in use, the operation will fail.
 *  @param in is an open input stream
 *  @return the number of the new interface or 0, if the operation failed
 */ 
int IfaceTable::readEntry(istream& in) {
	int ifnum, brate, prate; ipa_t ipa;

	Misc::skipBlank(in);
	if ( !Misc::readNum(in,ifnum) || !Np4d::readIpAdr(in,ipa) ||
	     !Misc::readNum(in,brate) || !Misc::readNum(in,prate)) {
		return 0;
	}
	Misc::cflush(in,'\n');

	if (!addEntry(ifnum,ipa,brate,prate)) return 0;
	return ifnum;
}

/** Read interface table entries from the input.
 *  The first line must contain an
 *  integer, giving the number of entries to be read. The input may
 *  include blank lines and comment lines (any text starting with '#').
 *  Each entry must be on a line by itself (possibly with a trailing comment).
 *  If the operation fails, a message is sent to cerr, identifying the
 *  interface that triggered the failure
 *  @param in is an open input stream
 *  @return true on success, false on failure
 */
bool IfaceTable::read(istream& in) {
	int num;
 	Misc::skipBlank(in);
        if (!Misc::readNum(in,num)) return false;
        Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (readEntry(in) == 0) {
			cerr << "IfaceTable::read: Error in "
			     << i << "-th entry read from input\n";
			return false;
		}
	}
	return true;
}

/** Create a string representation of an entry.
 *  @param iface is the number of the interface to be written
 *  @param s is a reference to a string inwhich result is returned
 *  @return a reference to s
 */
string& IfaceTable::entry2string(int iface, string& s) const {
	stringstream ss;
	ss << setw(5) << iface << "   "
	   << Np4d::ip2string(ift[iface].ipa,s)
	   << setw(9) << ift[iface].maxbitrate
	   << setw(9) << ift[iface].maxpktrate
	   << endl;
	s = ss.str();
	return s;
}

/** Create a string representation of the interface table.
 *  The output format matches the format expected by the read method.
 *  @param s is a reference to a string in which result is returned
 *  @return a reference to s
 */
string& IfaceTable::toString(string& s) const {
	stringstream ss;
	ss << ifaces->getNumIn() << endl;
	ss << "# iface  ipAddress      bitRate  pktRate\n";
	for (int i = firstIface(); i != 0; i = nextIface(i)) 
		ss << entry2string(i,s);
	s = ss.str();
	return s;
}
