import java.util.regex.*;
import java.util.ArrayList;
import princeton.StdDraw;

public class Comtree extends NetObj{
	protected String root, cores, links;
	protected String[] rates = new String[8];
	protected ArrayList<TopoComponent> src;
	protected ArrayList<TopoLink> lnks;

	public Comtree(String num, String rt){
		name = num;
		root = rt;
		lnks = new ArrayList<TopoLink>();
		src = new ArrayList<TopoComponent>();
	}

/*
	//deprecated
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
	public void setRates(String a, String b, String c, String d, String e, String f, String g, String h){
		String[] temp = {a, b, c, d, e, f, g, h};
		rates = temp;
	}

	public String getName(){return name;}
	public String getRoot(){return root;}
	public ArrayList<TopoComponent> getCores(){return src;}
	public ArrayList<TopoLink> getLinks(){return lnks;}	
	public String[] getRates(){return rates;}

	public void addCores(ArrayList<TopoComponent> core){
		for(TopoComponent tc: core)
			src.add(tc);
	}

	public void addLinks(ArrayList<TopoLink> l){
		for(TopoLink lnk: l)
			lnks.add(lnk);
	}
	
	@Override
	public String toString() {
		return name + " " + root + " " + cores;
	}

}
