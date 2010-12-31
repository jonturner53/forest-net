#include "vnetTbl.h"

vnetTbl::vnetTbl(int maxv1, qMgr *qm1) : maxv(maxv1), qm(qm1) {
// Constructor for vnetTbl, allocates space and initializes table.
	tbl = new tblEntry[maxv+1];
	for (int i = 0; i <= maxv; i++) {
		tbl[i].links = 0;
		tbl[i].plnk = 0;
		tbl[i].qn = 1;		// all default to queue 1
	}
};
	
vnetTbl::~vnetTbl() { delete [] tbl; }

int vnetTbl::links(int vn, uint16_t* lnks, int limit) const {
// Return all the vnet links for the specified vnet.
// Lnks points to a vector that will hold the links on return.
// The limit argument specifies the maximum number of links that
// may be returned. If the table entry specifies more links, the
// extra ones will be ignored. The value returned by the function
// is the number of links that were returned.
	if (vn < 1 || vn > maxv || !valid(vn)) return 0;
	int i = 0, lnk = 1;
	while (lnk <= 31 && lnk <= limit) {
		if (tbl[vn].links & (1 << lnk)) lnks[i++] = lnk;
		lnk++;
	}
	return i;
}

bool vnetTbl::getVnet(istream& is) {
// Read a vnet from is and initialize its table entry.
	int i, vn, plnk, qn, quant, nlnks;
	int lnk[MAXLNK+1];

        misc::skipBlank(is);
        if (!misc::getNum(is, vn) || vn < 1 || vn > maxv ||
	    !misc::getNum(is, plnk) ||
	    !misc::getNum(is, qn) ||
	    !misc::getNum(is, quant)) { 
                return false;
        }

	nlnks = 0;
	do {
		if (!misc::getNum(is,lnk[nlnks])) return false;
		if (lnk[nlnks] != 0) nlnks++;
		if (nlnks > MAXLNK) return false;
	} while (misc::verify(is,','));
	misc::cflush(is,'\n');

	bool validParent = false;
	addVnet(vn); setPlink(vn,plnk); setQnum(vn,qn);
	for (i = 0; i < nlnks; i++) {
		if (lnk[i] == plnk) validParent = true;
		addLink(vn,lnk[i]);
	}
	if (plnk != Null && !validParent) {
		removeVnet(vn); return false;
	}
	for (i = 0; i < nlnks; i++) qm->setQuantum(lnk[i],qn,quant);

        return true;
}

bool operator>>(istream& is, vnetTbl& vnt) {
// Read vnet table entrees from input stream. The first line must contain
// an integer, giving the number of entries to be read. The input may
// include blank lines and comment lines (any text starting with '#').
// Each entry must be on a line by itself (possibly with a trailing comment).
// An entry consists of a vnet number and a comma-separated list of links.
	int num;
	misc::skipBlank(is);
	if (!misc::getNum(is,num)) return false;
	misc::cflush(is,'\n');
	while (num--) {
		if (!vnt.getVnet(is)) return false;
	}
        return true;
}

void vnetTbl::putVnet(ostream& os, int vn) const {
// Print entry for link i
	os << setw(3) << vn << " "  
	   << setw(2) << plink(vn) << " "
	   << setw(3) << qnum(vn) << " ";
	int i = 1;
	while (i <= 31) {
		if (tbl[vn].links & (1 << i)) { os << i; break; }
		i++;
	}
	i++;
	while (i <= 31) {
		if (tbl[vn].links & (1 << i)) { os << "," << i; }
		i++;
	}
	os << endl;
}

ostream& operator<<(ostream& os, const vnetTbl& vnt) {
// Output human readable representation of vnet table.
        for (int i = 1; i <= vnt.maxv; i++) {
                if (vnt.valid(i)) {
			vnt.putVnet(os,i);
		}
	}
	return os;
}
