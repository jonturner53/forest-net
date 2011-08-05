import princeton.StdDraw;

/**
* abtract class used in parsing the topology file
* weight allows sorting for painting purposes
*/
abstract class NetObj implements Comparable{
	protected String name, nodeType, ipAdr, forestAdr, pktrate, bitrate; //fields for a default NetObj
	protected int weight = 1; // default weight for painting
	protected double x, y; //coordinate position of NetObj
	
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
