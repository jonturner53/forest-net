package remoteDisplay;
import princeton.StdDraw;
import java.awt.geom.Point2D;
import java.util.regex.*;

public class Link extends NetObj{
	protected Point2D A, B;	
	private String[] pair, zipcode;

	public Link(String nme, String pkt, String bit){
		name = nme;
		int count = 0;
		pair = new String[2];	
		zipcode = new String[2];
		Pattern p = Pattern.compile("\\d+\\.\\d+");
		Matcher m = null;
		for(String s: name.split(":")){
			m = p.matcher(s);
			m.find();
		//	System.out.println(m.group());
                	pair[count++]=m.group();
		}
		count = 0;
		p = Pattern.compile("\\d+\\.");
		m = null;
		for(String s: pair){
			 m = p.matcher(s);
			 m.find();
			 zipcode[count++]=m.group();
		}
		pktrate = pkt;
		bitrate = bit;
		weight = 0;
	}
	public String[]	getZip(){
		return zipcode;
	}

	public String[] getPair(){
		return pair;
	}
	public void setPoints(Point2D l, Point2D m){
		A = l;
		B = m;
	}	 
	@Override
	public String toString() {
		return name + " " + pktrate + " " + bitrate;
	}
}
