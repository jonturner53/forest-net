/** @file Controller.cpp 
 *
 *  @author Jon Turner
 *  @date 2014
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "Controller.h"

using namespace forest;

namespace forest {

/** Start a new Controller thread.
 *  @param self is a pointer to a Controller object; more precisely,
 *  an object belonging to some specific subclass of the Controller class
 *  @param myThx is the index of this thread in its thread pool
 *  @param qsiz is the size of the thread's input queue
 *  @return the value retured by the Controller objects run method.
 */
bool Controller::start(Controller* self, int myThx, int qsiz) {
	self->myThx = myThx; self->inq.resize(qsiz);
	return self->run();
}

} // ends namespace
