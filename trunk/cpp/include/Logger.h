/** @file Logger.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <pthread.h>
#include "Packet.h"
#include "CtlPkt.h"

namespace forest {


/** This is a very simple logger class to control the
 *  logging of error messages.
 */
 
class Logger {
public:
	Logger();

	void setLevel(int);

	/** Log message to standard output, based on severity.
	 *  @param s is error message
	 *  @param severity is the severity level of the message
	 *  @param p is a reference to a packet whose contents is to be
	 *  printed with the error message (optional argument)
	 */
	void log(const string& s, int severity) {
		if (severity >= level) logit(s,severity);
	};
	void log(const string& s, int severity, Packet& p) {
		if (severity >= level) logit(s,severity,p);
	};
	void log(const string& s, int severity, CtlPkt& cp) {
		if (severity >= level) logit(s,severity,cp);
	};

private:
	int level;		///< severity level: 0 thru 3
	pthread_mutex_t myLock;	///< to make all method calls thread-safe

	/** text description of severity levels used in error messages */
	char* tag[4];

	// helper method
	void logit(const string& s, int severity);
	void logit(const string& s, int severity, Packet& p);
	void logit(const string& s, int severity, CtlPkt& cp);
};

} // ends namespace


#endif
