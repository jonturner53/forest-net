/** @file Router.java */

package remoteDisplay;

import princeton.StdDraw;
import java.util.regex.*;

/**
* Programmatic representation of the topology file's Node object
extends NetObj's protected variables
*/
public class Router extends NetObj{
	private int numClients;
	private String zipcode;
	
	/**
	* sole constructor of a Router
	* @param nme - name of router
	* @param type - "router"
	* @param forest address
	* @param x coordinate
	* @param y coordinate
	*/
	public Router(String nme, String type, String fAdr, String xStr, String yStr){
		name = nme;
		nodeType = type;
		forestAdr = fAdr;
		x = Integer.parseInt(xStr);
		y = Integer.parseInt(yStr);
		Pattern p = Pattern.compile("(\\d+)");
		Matcher m = p.matcher(name);
		StringBuffer sb = new StringBuffer();
		while(m.find())
			sb.append(m.group());
		zipcode = sb.toString();
	}
	
	/**
	* @param num - number of clients on this router
	*/
	public void setNumClients(int num){
		numClients = num;
	}	
	
	/**
	* @return number of clients on router
	*/
	public int getNumClients(){
		return numClients;
	}

	/**
	* @return high order of the forest address x.xxx
	*/
	public String getZip(){
		return zipcode;
	}
	
	/**
	* @param high order portion of a zipcode
	* @return forest address from the high order zipcode
	*/
	public String getForestAdr(String zip){
		Pattern p = Pattern.compile("\\d+\\.");
		Matcher m = p.matcher(forestAdr);
		m.find();
		if(zip.equals(m.group()))
			return forestAdr;
		else
			return null;
	}

	@Override
	public String toString() {
		return name + " " + nodeType + " " + forestAdr;
	}

}
