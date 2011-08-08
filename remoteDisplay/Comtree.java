/** @file Comtree.java */

package remoteDisplay;

import java.util.regex.*;
import java.util.ArrayList;
import princeton.StdDraw;

/**
* forest Comtree representation from topology file
*/
public class Comtree extends NetObj{
	protected String root, cores, links;
	protected String[] rates = new String[8];
	protected ArrayList<TopoComponent> src;
	protected ArrayList<TopoLink> lnks;
	
	/**
	* @param num is the comtree number
	* @param rt is the name of the root node
	* Sole Constructor
	*/
	public Comtree(String num, String rt){
		name = num;
		root = rt;
		lnks = new ArrayList<TopoLink>();
		src = new ArrayList<TopoComponent>();
	}

/*
	//deprecated
	/**
	* Default Constructor
	* @param num - comtree number
	* @param rt - string name of comtree root
	* @param core - string list of cores
	* @param lk - string list of comtree links in the same syntax as a link in the links section
	*/

	public Comtree(String num, String rt, String core, String lk){
		name = num;
		root = rt;
		cores = core;
		links= lk;
		lnks = new ArrayList<TopoLink>();
		src = new ArrayList<TopoComponent>();
		Pattern p = Pattern.compile("\\d+\\.\\d+");
		for(String s: links.split(",")){
			Matcher m = p.matcher(s);
			while(m.find()){
		//		System.out.println("comtree lnk: " + m.group());
				lnks.add(m.group());
			}
		}
		p = Pattern.compile("\\d+");
		for(String s: links.split(",")){
			Matcher m = p.matcher(cores);
		  	while(m.find()){
		          //	System.out.println("comtree lnk: " + m.group());
		    		src.add(m.group());
		 	}       
		 }
	}
*/	

	/**
	* @param a is bitRateUp
	* @param b is bitRateDown
	* @param c is pktRateUp
	* @param d is pktRateDown
	* @param e is leafBitRateUp
	* @param f is leafBitRateDown
	* @param g is leafPktRateUp
	* @param h is leafPktRateDown
	* sets rates for the comtree
	*/
	public void setRates(String a, String b, String c, String d, String e, String f, String g, String h){
		String[] temp = {a, b, c, d, e, f, g, h};
		rates = temp;
	}

	public String getName(){return name;}
	public String getRoot(){return root;}
	public ArrayList<TopoComponent> getCores(){return src;}
	public ArrayList<TopoLink> getLinks(){return lnks;}	
	public String[] getRates(){return rates;}

	/**
	* @param core is the list of cores for this comtree
	* Adds a list of cores of the type TopoComponent
	*/
	public void addCores(ArrayList<TopoComponent> core){
		for(TopoComponent tc: core)
			src.add(tc);
	}
	
	/**
	* @param l is the list of links for this comtree
	* Adds a list of links of the type TopoLink which inherits from TopoComponent
	*/
	public void addLinks(ArrayList<TopoLink> l){
		for(TopoLink lnk: l)
			lnks.add(lnk);
	}
	
	@Override
	public String toString() {
		return name + " " + root + " " + cores;
	}

}
