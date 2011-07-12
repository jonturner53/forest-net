/** @file LinkTable.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "LinkTable.h"

// Constructor for LinkTable, allocates space and initializes table.
LinkTable::LinkTable(int nlnk1) : nlnk(nlnk1) {
	nlnk = min(Forest::MAXLNK,nlnk);
	ld = new lnkdata[nlnk+1];
	ht = new UiHashTbl(nlnk);

	iPkt = 0; oPkt = 0; iByt = 0; oByt = 0;
	irPkt = 0; orPkt = 0; icPkt = 0; ocPkt = 0;

	for (int i = 1; i <= nlnk; i++) { disable(i); }
};
	
LinkTable::~LinkTable() { delete [] ld; delete ht; }

/** Allocate an unused link number.
 *  The allocated link number is left in a "disabled" state.
 *  The caller is expected to call addEntry() to initialize the
 *  the essential fields in the link table entry, after which the
 *  entry will be enabled.
 *  @return first unused link number or 0 if all are in use
 */
int LinkTable::alloc() {
	for (int i = 1; i <= nlnk; i++) {
		if (!valid(i)) return i;
	}
	return 0;
}

bool LinkTable::addEntry(int lnk, int intf, ntyp_t pTyp, ipa_t pipa, fAdr_t pfa) {
// Add a link table entry for the specified lnk on the specified interface.
// PTyp is the peer's node type, pipa is the peer's IP address,
// and pfa is the peer's forest address.
// Return true on success, false on failure.
	if (lnk == 0 || lnk > nlnk) return false;

	bool status;
	if (pTyp != ROUTER) status = ht->insert(hashkey(pfa,true),lnk);
	else 		    status = ht->insert(hashkey(pipa,false),lnk);
	if (!status) return false;

        ld[lnk].intf = intf;
	ld[lnk].pipa = pipa; ld[lnk].padr = pfa;
        ld[lnk].ptyp = pTyp;
	ld[lnk].pipp = (pTyp != ROUTER ? 0 : Forest::ROUTER_PORT);
        enable(lnk); // mark as valid entry (even tho some fields not yet set)
        return true;
}

bool LinkTable::removeEntry(int lnk) {
// Remove the table entry for lnk. Return true on success, false on failure.
	if (!valid(lnk)) return false;

	if (ld[lnk].ptyp != ROUTER) ht->remove(hashkey(ld[lnk].padr,true));
	else 		    	    ht->remove(hashkey(ld[lnk].pipa,false));

	disable(lnk);  // mark entry as invalid
}

bool LinkTable::checkEntry(int te) {
// Return true if entry is consistent, else false.
	// the forest address of every peer must be a valid unicast address
	if (!Forest::validUcastAdr(getPeerAdr(te))) return false;

	// the forest address of restricted destination must be unicast
	// or zero
	if (getPeerDest(te) != 0 && !Forest::validUcastAdr(getPeerDest(te)))
		return false;

	// only a router may use the forest port number
	if (getPeerPort(te) == Forest::ROUTER_PORT && getPeerType(te) != ROUTER)
                return false;

	return true;
}

// Read an entry from is and store it in the link table.
// Return the link number for the new link.
// A line is a pure comment line if it starts with a # sign.
// A trailing comment is also allowed at the end of a line by
// including a # sign. All lines that are not pure comment lines
// or blank are assumed to contain a complete link table entry.
// Each entry consists of a link#, the IP address of the local
// endpoint, the IP address and port number of the peer,
// the type of the peer, the forest address of the peer
// and the allowed forest destination address for the peer.
// Finally, the entry includes a max bit rate for the link
// (in Kb/s) and a maximum packet rate (in p/s).
//
// If the link number specified in the input is already in use,
// the call to readEntry will fail, in which case 0 is returned.
// The call can also fail if the input is not formatted correctly.
//
int LinkTable::readEntry(istream& in) {
	int lnk, intf, brate, prate;
	ipa_t pipa; ipp_t pipp;
	ntyp_t ntyp; int pa, pd;
	string typStr;

	Misc::skipBlank(in);
	if ( !Misc::readNum(in,lnk) ||
	     !Misc::readNum(in,intf) ||
	     !Np4d::readIpAdr(in,pipa) || !Misc::verify(in,':') ||
             !Misc::readNum(in,pipp) ||
	     !Misc::readWord(in,typStr) ||
	     !Forest::readForestAdr(in,pa) ||
	     !Forest::readForestAdr(in,pd) ||
	     !Misc::readNum(in,brate) || !Misc::readNum(in,prate)) {
		return 0;
	}
	Misc::cflush(in,'\n');

             if (typStr == "client") ntyp = CLIENT;
        else if (typStr == "server") ntyp = SERVER;
        else if (typStr == "router") ntyp = ROUTER;
        else if (typStr == "controller") ntyp = CONTROLLER;
        else return 0;

	if (!addEntry(lnk,intf,ntyp,pipa,pa)) return 0;
	setPeerPort(lnk,pipp); setPeerDest(lnk,pd);
	setBitRate(lnk,brate); setPktRate(lnk,prate);

	if (!checkEntry(lnk)) { removeEntry(lnk); return 0; }

	return lnk;
}

// Read link table entries from the input. The first line must contain an
// integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
bool LinkTable::read(istream& in) {
	int num;
 	Misc::skipBlank(in);
        if (!Misc::readNum(in,num)) return false;
        Misc::cflush(in,'\n');
	for (int i = 1; i <= num; i++) {
		if (readEntry(in) == 0) {
			cerr << "Error reading link table entry # "
			     << i << endl;
			return false;
		}
	}
	return true;
}

void LinkTable::writeEntry(ostream& out, int i) const {
// Print entry for link i
	out << setw(2) << i << " " << getInterface(i) << " ";
	
	Np4d::writeIpAdr(out,getPeerIpAdr(i));
	out << ":" << getPeerPort(i);

        switch (getPeerType(i)) {
        case CLIENT: 	  out << " client    "; break;
        case SERVER: 	  out << " server    "; break;
        case ROUTER: 	  out << " router    "; break;
        case CONTROLLER:  out << " controller"; break;
        default: fatal("LinkTable::writeEntry: undefined type");
        }

	out << " "; Forest::writeForestAdr(out,getPeerAdr(i));
	out << " "; Forest::writeForestAdr(out,getPeerDest(i));
	
	out << " " << setw(6) << getBitRate(i)
	    << " " << setw(6) << getPktRate(i)
	    << " " << setw(6) << getMinDelta(i)
	    << endl;
}

void LinkTable::write(ostream& out) const {
// Output human readable representation of link table.
	for (int i = 1; i <= nlnk; i++) 
		if (valid(i)) writeEntry(out,i);
}
