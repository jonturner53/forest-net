/** @file TopoLink.java */

package remoteDisplay.TopologyGen;

import java.util.*;
import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;

/**
* Component to represent links in the topology file and borrows some of the same attributes as a TopoComponent for easy storage in the Panel class
*/
public class TopoLink extends TopoComponent{
	protected int bitrate, pktrate; ///< bitrate and packet rate values for this link
	protected TopoComponent[] connect; ///< the pair of TopoComponents that this line intersects

	/**
	* default Constructor
	* @param s is the Line2D.Double shape representation of a TopoLink
	* @param mark is the constant from Common.java designating this as Common.LINK
	*/
	public TopoLink(Shape s, int mark){
		super();
		shape = s;
		type = mark;
		connect = new TopoComponent[2];
	}
	
	/**
	* @param compts is a two-element array of TopoComponents that define the termeni of this line
	*/
	public void setConnection(TopoComponent[] compts){
		connect = compts;
	}
	
	/**
	* @param a is the first TopoComponent 
	* @param b is the second TopoComponent
	* both a and b define the termeni of this line
	*/
	public void setConnection(TopoComponent a, TopoComponent b){
		connect[0] =a;
		connect[1] =b;
	}
	
	/**
	* set the bitrate of this link
	* @param bits is the bitrate of this link in the Forest network
	*/
	public void setBitRate(int bits){
		bitrate = bits;
	}

	/**
	* set the packetrate of this link
	* @param pkt is the packet rate
	*/
	public void setPktRate(int pkt){
		pktrate = pkt;
	}

	/**
	* @return the bitrate of this link
	*/
	public int getBitRate(){
		return bitrate;
	}
	
	/**
	* @return the packet rate of this link
	*/
	public int getPktRate(){
		return pktrate;
	}

	/**
	* @return the two TopoComponents that define this link
	*/
	public TopoComponent[] getConnection(){
		return connect;
	}
	
	/**
	* @Override
	* return the type of this to Common.LINK
	*/
	public int getType(){
		return Common.LINK;
	}

	@Override
	public String toString(){
		return new String(getClass() + "	"+Integer.toString(bitrate) + "	" + Integer.toString(pktrate));
	}
}
