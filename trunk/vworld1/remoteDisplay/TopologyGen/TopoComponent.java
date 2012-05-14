package remoteDisplay.TopologyGen;

/** @file TopoComponent.java */

import java.util.*;
import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import remoteDisplay.*;
import remoteDisplay.TopologyGen.Common.*;

/**
* Object used as a data representation of the front end gui for building a topology file
* TopoComponents represent Routers, Clients, Controllers, and one end of a TopoLink, which extends TopoComponent
*/
public class TopoComponent implements Comparable{
	protected MenuItem item = null; ///< MenuItem that spawned this TopoComponent
	protected String name, port, fAdr, ip; //< attributes of a TopoComponent: name, forest address and ip address
	protected String[] range; ///< client port # range
	protected String[] ifc; ///< cantains a row from the interface table
	protected ArrayList<String[]> interfaces; ///< contains all rows from the interface table
	protected Shape shape; ///< the graphical representation of this TopoComponent on screen
	protected ArrayList<TopoComponent> lnks = new ArrayList<TopoComponent>(); ///< list of all TopoLinks abstracted as their parent TopoComponents
	protected double dx, dy, dx2, dy2; ///< original distance from another object on screen before any translation
	protected int type; ///< type as defined in the Common class
	protected Integer weight = 0; ///< default place in order for being painted on screen
	protected boolean isRoot; ///< is root of a comtree - iff type equals ROUTER
	protected boolean isCore; ///< is a core of a comtree - iff type equals ROUTER
	protected boolean selected; ///< is selected from SHIFT key or depressed mouse
	protected ArrayList<String> avaPorts = new ArrayList<String>(); //< avalible ports on this TopoComponent

	/**
	* Empty Constructor
	*/
	TopoComponent(){}

	/**
	* Default Constructor
	* @param nme is the name of this Component apart from its graphical representation
	* @param jmi is the MenuItem that spawns this type of TopoComponent
	* @param s is the graphical representation of this TopoComponent
	*/
	TopoComponent(String nme, MenuItem jmi, Shape s){
		name = nme;
		item = jmi;
		type = jmi.getType();
		shape = s;
		range = new String[2];
		interfaces = new ArrayList<String[]> ();
		isRoot = false;
		isCore = false;
		selected = false;
		ip = "127.0.0.1";
		StringBuilder sb = new StringBuilder();
		for(int n = 1; n <= Common.NUMPORTS; n++){
			String num = Integer.toString(n);
			if(n != Common.NUMPORTS)
				sb.append(num+",");
			else
				sb.append(num);
			avaPorts.add(num);
		}
	}
	
	/**
	* @param l is a TopoLink
	* @return true if this link is shared by this TopoComponent
	*/
	public boolean compareLinks(TopoComponent l){
		return lnks.contains(l);
	}

	/**
	* Used for plotting the next location of the shape on the screen
	* @param xdiff is the initial x-axis distance from a mouse press to this Shape (x1)
	* @param ydiff is the initial y0axis distance from a mouse press to this Shape (y1)
	*/
	public void setDif(double xdiff, double ydiff){
		dx = xdiff;
		dy = ydiff;
	}

	/**
	* Used for plotting the next location of a line on the screen
	* @param xdiff is the initial distance form mouse press to the shape (x1)
	* @param ydiff is the initial distance form mouse press to the shape (y1)
	* @param x2diff is the initial distance form mouse press to the shape (x2)
	* @param y2diff is the initial distance form mouse press to the shape (y2)
	*/
	public void setDiff(double xdiff, double ydiff, double x2diff, double y2diff){
		dx = xdiff;
		dy = ydiff;
		dx2 = x2diff;
		dy2 = y2diff;
	}
	/**
	* @return all possible ports for a component to use.
	*/
	public String[] getAllPorts(){
		String[] ports = new String[Common.NUMPORTS];
		for(int n = 1; n <= Common.NUMPORTS; n++)
			ports[n-1] = Integer.toString(n);
		return ports;
	}
	
	public boolean hasPort(){
		return (isRouter() || isClient());
	}
	/**
	* @return all ports minus those already used by itself in connection with other components
	*/
	public String[] getAvaPorts(){
		ArrayList<Integer> sorter = new ArrayList<Integer>();
		if(avaPorts.isEmpty())
			return null;
		for(String s: avaPorts)
			if(s != null)
				sorter.add(Integer.parseInt(s));
		Collections.sort(sorter);
		String[] array = new String[sorter.size()];
		for(int n = 0; n < sorter.size(); n++)
			array[n] = Integer.toString(sorter.get(n));
		return array;
	}

	/**
	* set name of this TopoComponent
	* @param n is the new name of thsi component
	*/
	public void setName(String n){
		name = n;
	}
	
	/**
	* set state of isCore
	* @param bool if this TopoComponent is a core router in the Forest network
	*/
	public void setCore(boolean bool){
		isCore = bool;
	}
	
	/**
	* set this component is selected or not
	* @param bool if this TopoComponent is selected on screen
	*/
	public void setSelected(boolean bool){
		selected = bool;
	}
	
	/**
	* this parameters are the fields from the Interface table in the context menu if this TopoComponent's type equals Router
	* @param num is the row number of this interface beggining with 1
	* @param ip is the ip address from the table
	* @param lnks is the specified range of values such as 1 or min-max for the number of links to this interface
	* @param bit is the bitrate of this interface
	* @param pkt is the packet rate of this interface
	*/
	public void addInterface(String num, String ip, String lnks, String bit, String pkt){
		String[] temp = {num, ip, lnks, bit, pkt};
		ifc = temp;
		interfaces.add(ifc);
	}
	
	/**
	* set the forest address
	* @param f is the forest address of this component in the form of [high order].[low order]
	*/
	public void setForestAdr(String f){
		fAdr = f;
	}
	
	/**
	* set the ip address
	* @param adr is the ip address of this component
	*/
	public void setIp(String adr){
		ip = adr;
	}
	
	/**
	* set port range
	* @param lowBound is the lower bound of the port range
	* @param upBound is the upper bound of the port range
	*/
	public void setRange(String lowBound, String upBound){
		range[0] = lowBound;
		range[1] = upBound;
	}
	
	/**
	* Remove a port # from the avalible pool
	* @param p is the port being removed from the pool of avalible ports
	*/
	public void bindPort(String p){
		avaPorts.remove(p);
	}

	/**
	* Add port # back into the avalible pool
	* @param s is the port # being added to the pool
	*/
	public void releasePort(String p){
		if(!avaPorts.contains(p))
			avaPorts.add(p);
	}

	/**
	* @return true if type is Common.ROUTER
	*/
	public boolean isRouter(){
		return (type == Common.ROUTER);
	}

	/**
	* @return true if type is Common.CONTROLLER
	*/
	public boolean isController(){
		return (type== Common.CONTROLLER);
	}
	
	/**
	* @return true if type is Common.CLIENT
	*/
	public boolean isClient(){
		return (type== Common.CLIENT);
	}
	
	/**
	* @return true if this is root
	*/
	public boolean isRoot(){
		return isRoot;
	}

	/**
	* @return true if this is a core
	*/
	public boolean isCore(){
		return isCore;
	}

	/**
	* @reurn true if this is selected
	*/
	public boolean isSelected(){
		return selected;
	}
	
	/**
	* @return TopoComponent's type from MenuItem
	*/
	public int getType(){
		return item.getType();
	}

	/**
	* @return TopoComponent's name
	*/
	public String getName(){
		return name;
	}
	
	/**
	* @return forest address
	*/
	public String getForestAdr(){
		return fAdr;
	}
	
	/**
	* @return ip address
	*/
	public String getIp(){
		return ip;
	}

	/**
	* @return the upper and lower bounds of link ranges
	*/
	public String[] getRange(){
		return range;
	}
	
	/**
	* @return the line from the Interfaces table for this TopoComponent
	*/
	public ArrayList<String[]> getInterface(){
		return interfaces;
	}
	
	/**
	* @param l is really a TopoLink
	* adds a link to the list of links belonging to this component
	*/
	public void addLink(TopoComponent l){
		lnks.add(l);
	}
	
	/**
	* @return the list of TopoLinks belonging to this component
	*/
	public ArrayList<TopoComponent> getLinks(){
		return lnks;
	}

	/**
	* @return the graphical point from the bounds of the shape
	*/
	public Point getPoint(){
		return new Point((int) shape.getBounds().getX(), (int) shape.getBounds().getY());
	}
	
	@Override
	public int compareTo(Object obj){
		TopoComponent tc = (TopoComponent) obj;
		return weight.compareTo(tc.weight);
	}
	
	@Override
	public String toString(){
		return name;
		//return  name +" " + shape.getBounds2D().getX() + " " + shape.getBounds2D().getY() + " " + lnks+"\n"+"IP: " + ip + " FADR: " + fAdr;
	}

}
