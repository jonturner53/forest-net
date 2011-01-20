// Header file for GenTbl class.
//
// Defines a generic table using templates. Can be
// customized to implement various specific router
// configuration tables. The rowTyp template argument
// defines the fields in each row and the methods used
// to access the fields. The template argument N defines
// the maximum number of rows in the table (numbered 1..N).
//

#ifndef GENTBL_H
#define GENTBL_H

#include "forest.h"

template<class RowTyp, int N> class GenTbl {
public:
		GenTbl();
		~GenTbl();

	bool	validRow(int);			// return true for in-use row
	RowTyp& getRow(int);			// return reference to row
	int	assignRow(int=0);	 	// make an unused row valid
	bool	releaseRow(int);		// free a valid row
	void	assignKey(uint64_t,int);	// assign a key to a row
	int	lookup(uint64_t);		// return row for given key

	bool	read(istream&);
	void	write(ostream&);
	
private:
	RowTyp 	row[N+1];		// row[i] is i-th row in table

	ListPair lp;			// contains list of valid rows
					// and list of free rows
};

// Return true if rn is the number of a valid row in the table.
inline template<class rowTyp, int N>
bool GenTbl<rowTyp,N>::validRow(int rn) {
	return 1 <= rn && rn <= N && lp.onL1();
}

// Return reference to row with specified number
inline template<class rowTyp, int N>
RowTyp& GenTbl<rowTyp,N>::getRow(int rn) {
	return validRow(rn) ? row[rn] : 0;
}

#endif
