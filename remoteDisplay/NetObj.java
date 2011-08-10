package remoteDisplay;

/** @file NetObj.java */

/**
* abtract class used in parsing the Forest topology file
* This wrapper of sorts is used in the various lists of a diverse number of objects in the master ComtreeDisplay program. Thus routers, controllers, links, etc can be sorted and called with a universal set of function calls.
*/
abstract class NetObj implements Comparable{
	protected String name, nodeType, ipAdr, forestAdr, pktrate, bitrate; ///<fields for a default NetObj: name, node type= ROUTER, CONTROLLER, CLOENT or as otherwise defined in the Common class. an internet address, a Forest address, a packet rate and a bit rate
	protected int weight = 1; ///<default weight for painting
	protected double x, y; ///<coordinate position of NetObj
	
	/**
	*@Override Comparable of two NetObjs
	*/
	public int compareTo(Object o){
		NetObj n = (NetObj) o;
		if(weight > n.weight)
			return 1;
		else if(weight < n.weight)
			return -1;
		else
			return 0;
	}

	/**
	* @Override toString method to be implemented by all inheirting classes 
	*/
	public abstract String toString();

}
