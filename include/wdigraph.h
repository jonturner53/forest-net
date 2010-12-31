// Header file for data structure representing directed graph
// with edge lengths.

#ifndef WDIGRAPH_H
#define WDIGRAPH_H

#include "stdinc.h"
#include "misc.h"
#include "digraph.h"

typedef int length;

class wdigraph : public digraph {
public:		wdigraph(int=26,int=100);
		wdigraph(const wdigraph&);
		~wdigraph();
        length  len(edge) const;          		// return length of e
        void    changeLen(edge,length);  		// change length of e
	void	randLen(int,int);			// assign random lengths
	wdigraph&  operator=(const wdigraph&); 		// assignment
	void    putEdge(ostream&,edge,vertex) const; 	// output edge
	friend	int operator>>(istream&, wdigraph&); 	// IO operators
	friend	ostream& operator<<(ostream&, const wdigraph&);
protected:
	edge	*lng;				// edge length
        void    makeSpace();    		// allocate dynamic storage
        void    virtual mSpace();
        void    freeSpace();    		// release dynamic storage
        void    virtual fSpace();
        void    copyFrom(const wdigraph&); 	// copy-in from given graph
        void    cFrom(const wdigraph&);
	bool	virtual getEdge(istream&,edge&);	 // input edge
	void	shuffle(int*, int*);		// permute vertices,edges
};

// Return length of e
inline length wdigraph::len(vertex e) const { 
	assert(1 <= e && e <= M);
	return lng[e];
}

// Return first edge incident to x.
inline void wdigraph::changeLen(edge e, length ww) { 
	assert(1 <= e && e <= M);
	lng[e] = ww;
}

#endif
