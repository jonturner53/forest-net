/** @file Repeater.h 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef REPEATER_H
#define REPEATER_H

#include <queue>
#include <map>

#include "stdinc.h"
#include "Forest.h"
#include "Pair.h"
#include "Dheap.h"
#include "Hash.h"
#include "HashMap.h"

using namespace std::chrono;
using namespace grafalgo;

namespace forest {

/** Class to manage repeated sending of control packets. */
class Repeater {
public:
		Repeater(int);
		~Repeater();

	int	saveReq(int, int64_t, int64_t, int=0);
	pair<int,int> deleteMatch(int64_t);
	pair<int,int> overdue(int64_t);
private:
	int	n;
	HashMap<int64_t, Pair<int,int>, Hash::s64> *pmap;
					///< maps seqNum->(pktx,repCount)
	Dheap<int64_t> *deadlines;	///< heap of packets waiting on replies
};

} // ends namespace

#endif
