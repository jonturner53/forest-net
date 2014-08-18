/** @file Handler.h 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef HANDLER_H
#define HANDLER_H

#include "Quu.h"
#include <thread> 
#include <mutex> 
#include <chrono> 
#include <string>

using std::thread;
using std::mutex;

namespace forest {

/** This is a base class which is extended by the various controllers,
 *  that use the Substrate class. The various specific controller
 *  classes extend it, with whatever data and methods are appropriate for them.
 *  The class includes a static start method that is called by the substrate,
 *  for each worker thread, and it has a virtual method run() which is
 *  provided by the subclass that implements each specific controller.
 */
class Controller {
public:		
		Controller();
		~Controller();
	friend	class Substrate;
protected:
	thread	thred;			///< thread object
	int	myThx;			///< index in thread pool
	Quu<int> inq;			///< per thread input queue 
	static Quu<pair<int,int>> outq;	///< shared output queue

	static bool start(Controller*, int, int);
	virtual bool run();	
};

} // ends namespace

#endif
