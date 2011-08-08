/** @file Controller.java */

package remoteDisplay;

import princeton.StdDraw;
import java.util.regex.*;

/**
* representation of a Controller from the topology file
*/
public class Controller extends NetObj{

	/**
	* default constructor
	* @param nme - name of controller
	* @param type - "controller"
	* @param ip - ip address
	* @param fAdr - forest address
	* @param xStr - x coordinate
	* @param yStr - y coordinate
	*/
	public Controller(String nme, String type, String ip, String fAdr, String xStr, String yStr){
		name = nme;
		nodeType = type;
		ipAdr = ip;
		forestAdr = fAdr;
		x = Integer.parseInt(xStr);
		y = Integer.parseInt(yStr);
		weight = 0;
	
		Pattern p = Pattern.compile("(\\d+\\.)");
		Matcher m = p.matcher(forestAdr);
		StringBuffer sb = new StringBuffer();
		while(m.find())
			sb.append(m.group());
		String tmp = sb.toString();
		int index = tmp.indexOf('.');
		zipcode = tmp.substring(0, index);
//		System.out.println("Cont Zip: "+zipcode);

	}
	
	public String getZipCode(){ return zipcode;}

	@Override
	public String toString() {
		return name + " " + nodeType + " " + ipAdr + " " + forestAdr;
	}
}
