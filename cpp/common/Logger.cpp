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
	if (pthread_mutex_init(&myLock,NULL) != 0)
		fatal("Logger::Logger: could not initialize lock\n");
	tag[0] = "informational";
	tag[1] = "warning";
	tag[2] = "exceptional event";
	tag[3] = "program error";
}

void Logger::setLevel(int lev) {
	pthread_mutex_lock(&myLock);
	level = min(3,max(0,lev));
	pthread_mutex_unlock(&myLock);
}

/** Log message to standard output, based on severity.
  *  @param s is error message
  *  @param severity is the severity level of the message
  *  @param p is a reference to a packet whose contents is to be
  *  printed with the error message (optional argument)
  */
void Logger::logit(const string& s, int severity) {
	pthread_mutex_lock(&myLock);
	cerr << "Logger: " << s << "(" << tag[severity] << ")\n";
	if (severity > 3) fatal("terminating");
	pthread_mutex_unlock(&myLock);
}

void Logger::logit(const string& s, int severity, Packet& p) {
	pthread_mutex_lock(&myLock);
	cerr << "Logger: " << s << "(" << tag[severity] << ")\n";
	string s1;
	cerr << p.toString(s1) << endl;
	if (severity > 3) fatal("terminating");
	pthread_mutex_unlock(&myLock);
}

void Logger::logit(const string& s, int severity, CtlPkt& cp) {
	pthread_mutex_lock(&myLock);
	cerr << "Logger: " << s << "(" << tag[severity] << ")\n";
	string s1;
	cerr << cp.toString(s1) << endl;
	if (severity > 3) fatal("terminating");
	pthread_mutex_lock(&myLock);
}

} // ends namespace

