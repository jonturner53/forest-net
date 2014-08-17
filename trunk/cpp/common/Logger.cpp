/** @file Logger.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Logger.h"

namespace forest {


Logger::Logger() {
	level = 1;
	tag[0] = "informational";
	tag[1] = "warning";
	tag[2] = "exceptional event";
	tag[3] = "program error";
}

void Logger::setLevel(int lev) {
	unique_lock<mutex> lck(myMtx);
	level = min(3,max(0,lev));
}

/** Log message to standard output, based on severity.
  *  @param s is error message
  *  @param severity is the severity level of the message
  *  @param p is a reference to a packet whose contents is to be
  *  printed with the error message (optional argument)
  */
void Logger::logit(const string& s, int severity) {
	unique_lock<mutex> lck(myMtx);
	cerr << "Logger: " << s << " (" << tag[severity] << ")\n";
	if (severity > 3) Util::fatal("terminating");
}

void Logger::logit(const string& s, int severity, Packet& p) {
	unique_lock<mutex> lck(myMtx);
	cerr << "Logger: " << s << " (" << tag[severity] << ")\n";
	cerr << p.toString() << endl;
	if (severity > 3) Util::fatal("terminating");
}

void Logger::logit(const string& s, int severity, CtlPkt& cp) {
	unique_lock<mutex> lck(myMtx);
	cerr << "Logger: " << s << " (" << tag[severity] << ")\n";
	cerr << cp.toString() << endl;
	if (severity > 3) Util::fatal("terminating");
}

} // ends namespace

