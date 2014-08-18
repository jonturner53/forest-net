/** @file RepeatHandler.h 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef REPEATHANDLER_H
#define REPEATHANDLER_H

#include <queue>
#include <map>
#include <chrono>

#include "stdinc.h"
#include "Forest.h"
#include "Pair.h"
#include "Dheap.h"
#include "Hash.h"
#include "HashMap.h"

using namespace std::chrono;
using namespace grafalgo;

namespace forest {

/** Class to aid in handling repeated control packets. */
class RepeatHandler {
public:
		RepeatHandler(int);
		~RepeatHandler();

	int	find(fAdr_t, int64_t);
	bool	saveReq(int, fAdr_t, int64_t, int64_t);
	int	saveRep(int, fAdr_t, int64_t);
	int	expired(int64_t);
private:
	int	n;
	HashMap<Pair<fAdr_t,int64_t>, int, Hash::s32s64> *pmap;
					///< maps seqNum->(pktx,repCount)
	Dheap<int64_t> *deadlines;	///< heap of packets waiting on replies
};

} // ends namespace

#endif
