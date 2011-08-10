/** @file Controller.java */

package remoteDisplay;

import java.util.regex.*;

/**
* representation of a Controller from the topology file. Composed of a name, type, ip address, forest address and x and y coordinate and a zipcode containing the high order portion of its forest address.
*/
public class Controller extends NetObj{
	String zipcode; ///< number to the left of the decimel in a forest address. Otherwise considered the high order portion of the address.
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
