/** @file RateSpec.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "RateSpec.h"

namespace forest {


/** Read up to first occurrence of a given character.
 *  @param in is the input stream to read from
 *  @param c  is character to look for
 *  @return c or 0, on end-of-file
 */
bool RateSpec::read(istream& in) {
	return	Misc::verify(in,'(') && Misc::readNum(in,bitRateUp) &&
		Misc::verify(in,',') && Misc::readNum(in,bitRateDown) &&
		Misc::verify(in,',') && Misc::readNum(in,pktRateUp) &&
		Misc::verify(in,',') && Misc::readNum(in,pktRateDown) &&
		Misc::verify(in,')');
}

/** Create a string representation of the rate spec.
 *  @param s is a reference to a string in which result is returned
 *  @param return a reference to s
 */
string& RateSpec::toString(string& s) const {
	stringstream ss;
	ss << "(" << this->bitRateUp << "," << this->bitRateDown
	   << "," << this->pktRateUp << "," << this->pktRateDown << ")";
	s = ss.str();
	return s;
}

} // ends namespace

