import java.util.regex.*;
import java.util.ArrayList;
import princeton.StdDraw;

/**
* forest Comtree representation from topology file
*/
public class Comtree extends NetObj{
	protected ArrayList<String> lnks, src;
	protected String root, members,  links;

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
		members = core;
		links= lk;
		lnks = new ArrayList<String>();
		src = new ArrayList<String>();
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
			Matcher m = p.matcher(members);
		  	while(m.find()){
		          //	System.out.println("comtree lnk: " + m.group());
		    		src.add(m.group());
		 	}       
		 }
	}
	
	@Override
	public String toString() {
		return name + " " + root + " " + members + " " + links;
	}

}
