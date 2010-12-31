// Header file for vnetTbl class.
//
// Maintains set of tuples of the form (vnet, links)
// where vnet is the vnet number for this entry and
// links is the set of all incident links that are
// in the vnet. In this version, links is implemented
// as a 32 bit vector, limiting the number of links
// to 32 (well actually 31, as we don't use 0).
// Also, this version only supports vnet numbers up to
//

#ifndef VNETTBL_H
#define VNETTBL_H

#include "wunet.h"
#include "qMgr.h"

class vnetTbl {
public:
		vnetTbl(int, qMgr*);
		~vnetTbl();

	void	addVnet(vnet_t); 	 	// add vnet
	void	removeVnet(vnet_t);		// remove vnet
	void 	addLink(vnet_t, int);
	void 	removeLink(vnet_t, int);
	
	bool	valid(int) const;		// return true for valid vnet
	int	links(int,uint16_t*,int) const;	// return vector of links
	bool	inVnet(vnet_t,int) const;	// return true if link in vnet
	int	plink(vnet_t) const;		// return the link to parent
	void	setPlink(vnet_t,int);		// set the parent link vnet
	int	qnum(vnet_t) const;		// return the q number for vnet
	void	setQnum(vnet_t,int);		// set the q number for vnet

	bool 	getVnet(istream&);		// read table entry from input
	friend	bool operator>>(istream&, vnetTbl&); // read in table
	void	putVnet(ostream&, int) const; // print a single entry
	friend	ostream& operator<<(ostream&, const vnetTbl&);
private:
	int	maxv;			// largest vnet number
	struct tblEntry { 
		int links;		// bit vector of links
		int plnk;		// parent link in comtree
		int qn;			// number of queue for vnet
	} *tbl;				// tbl[i] contains data for vnet[i]
	qMgr	*qm;			// pointer to queue manager
};

inline void vnetTbl::addVnet(vnet_t vn) {
// Add the specified vnet to the set of vnets in use.
// Use bit 0 of links field to denote a valid vnet.
	if (vn > 0 && vn <= maxv) tbl[vn].links = 1;
	tbl[vn].plnk = 0;
}

inline void vnetTbl::removeVnet(vnet_t vn) {
// Remove the specified vnet from the set of vnets in use.
	if (vn > 0 && vn <= maxv) tbl[vn].links = 0;
}

// return true for valid entry
inline bool vnetTbl::valid(int vn) const {
	return vn > 0 && vn <= maxv && (tbl[vn].links & 1);
}

inline void vnetTbl::addLink(vnet_t vn, int lnk) {
// Add the link to the set of valid links for vn.
	if (vn > 0 && vn <= maxv && valid(vn)) tbl[vn].links |= (1 << lnk);
}

inline void vnetTbl::removeLink(vnet_t vn, int lnk) {
// Remove the link from the set of valid links for vn.
	if (vn > 0 && vn <= maxv && valid(vn)) tbl[vn].links &= ~(1 << lnk);
}

// return true for link in vnet
inline bool vnetTbl::inVnet(vnet_t vn, int lnk) const {
	return vn > 0 && vn <= maxv && valid(vn) && (tbl[vn].links & (1 << lnk));
}

// return the parent link for the vnet
inline int vnetTbl::plink(vnet_t vn) const {
	if (vn > 0 && vn <= maxv && valid(vn)) return tbl[vn].plnk;
}

// Set the parent link for the vnet
inline void vnetTbl::setPlink(vnet_t vn, int plnk) {
	if (vn > 0 && vn <= maxv && valid(vn)) tbl[vn].plnk = plnk;
}
// return the queue number for the vnet
inline int vnetTbl::qnum(vnet_t vn) const {
	if (vn > 0 && vn <= maxv && valid(vn)) return tbl[vn].qn;
}

// Set the queue number for the vnet
inline void vnetTbl::setQnum(vnet_t vn, int qn) {
	if (vn > 0 && vn <= maxv && valid(vn)) tbl[vn].qn = qn;
}

#endif
