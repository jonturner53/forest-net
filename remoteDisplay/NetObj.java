package remoteDisplay;
import princeton.StdDraw;


public abstract class NetObj implements Comparable{
	protected String name, nodeType, ipAdr, forestAdr, pktrate, bitrate;
	protected int weight = 1;
	protected double x, y;
	
	@Override
	public int compareTo(Object o){
		NetObj n = (NetObj) o;
		if(weight > n.weight)
			return 1;
		else if(weight < n.weight)
			return -1;
		else
			return 0;
	}
	public abstract String toString();

}
