package remoteDisplay;
import java.util.regex.*;
import java.util.ArrayList;
import princeton.StdDraw;

public class Comtree extends NetObj{
	protected ArrayList<String> lnks, src;
	protected String root, members,  links;

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
