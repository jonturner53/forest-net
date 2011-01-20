#include "GenTbl.h"

template<class rowTyp, int N>
GenTbl<rowTyp,N>::GenTbl() {
	lp = new ListPair(N);
}

template<class rowTyp, int N>
bool GenTbl<rowTyp,N>::assignRow(int rn) {
// Make an unused row valid. Return the row number on
// success, 0 on failure.
	if (rn < 0 || rn > N) return 0;
	if (rn == 0) rn = lp->firstL2();
	if (lp.onL2(rn)) lp->swap(rn);
	return rn;
}
	
template<class rowTyp, int N>
bool GenTbl<rowTyp,N>::releaseRow(int rn) {
// Make an in-use row invalid. Return the row number on
// success, 0 on failure.
	if (rn <= 0 || rn > N) return 0;
	if (lp.onL2(rn)) return 0;
	lp->swap(rn);
	return rn;
}
