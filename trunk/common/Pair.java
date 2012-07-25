/** @file RateSpec.java
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

package forest.common;

/** This class stores a pair of arbitrary type.  */
public class Pair<T1,T2> {
	public T1 first; public T2 second;

	public Pair(T1 first, T2 second) {
		this.first = first; this.second = second;
	}
}
