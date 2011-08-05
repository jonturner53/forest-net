import java.util.*;
import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;

public class TopoComponent implements Comparable{
	protected MenuItem item = null;
	protected String name, port, fAdr, ip, ports;
	protected String[] range;
	protected String[] ifc;
	protected ArrayList<String[]> interfaces;
	protected Shape shape;
	protected ArrayList<TopoLink> lnks = new ArrayList<TopoLink>();
	protected double dx, dy, dx2, dy2;
	protected int type;
	protected Integer weight = 0;
	protected boolean isRoot;
	protected boolean isCore;
	protected boolean selected;
	protected ArrayList<String> avaPorts = new ArrayList<String>();
	protected Common c;

	TopoComponent(){}

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
		for(int n = 1; n <= c.NUMPORTS; n++){
			String num = Integer.toString(n);
			if(n != c.NUMPORTS)
				sb.append(num+",");
			else
				sb.append(num);
			avaPorts.add(num);
		}
		ports = sb.toString();
	}
	
	public boolean compareLinks(TopoLink l){
		return lnks.contains(l);
	}

	public void setDif(double xdiff, double ydiff){
		dx = xdiff;
		dy = ydiff;
	}

	public void setDiff(double xdiff, double ydiff, double x2diff, double y2diff){
		dx = xdiff;
		dy = ydiff;
		dx2 = x2diff;
		dy2 = y2diff;
	}
	
	public String[] getAllPorts(){
		String[] ports = new String[c.NUMPORTS];
		for(int n = 1; n <= c.NUMPORTS; n++)
			ports[n-1] = Integer.toString(n);
		return ports;
	}

	public String[] getAvaPorts(){
		ArrayList<Integer> sorter = new ArrayList<Integer>();
		for(String s: avaPorts)
			sorter.add(Integer.parseInt(s));
		Collections.sort(sorter);
		String[] array = new String[sorter.size()];
		for(int n = 0; n < sorter.size(); n++)
			array[n] = Integer.toString(sorter.get(n));
		return array;
	}

	public void setName(String n){
		name = n;
	}
	
	public void setCore(boolean bool){
		isCore = bool;
	}
	
	public void setSelected(boolean bool){
		selected = bool;
	}
	
	public void addInterface(String num, String ip, String lnks, String bit, String pkt){
		String[] temp = {num, ip, lnks, bit, pkt};
		ifc = temp;
		interfaces.add(ifc);
	}
	
	public void setForestAdr(String f){
		fAdr = f;
	}

	public void setIp(String adr){
		ip = adr;
	}
	
	public void setRange(String lowBound, String upBound){
		range[0] = lowBound;
		range[1] = upBound;
	}

	public boolean isRouter(){
		return (type == c.ROUTER);
	}

	public boolean isController(){
		return (type== c.CONTROLLER);
	}
	
	public boolean isClient(){
		return (type== c.CLIENT);
	}
	public boolean isRoot(){
		return isRoot;
	}

	public boolean isCore(){
		return isCore;
	}

	public boolean isSelected(){
		return selected;
	}
	
	public void bindPort(String s){
		port = s;
		avaPorts.remove(port);
	}

	public void releasePort(){
		avaPorts.add(port);
		port = "";
	}
	
	public boolean hasPort(){
		if(port == null)
			return false;
		if(port.equals(""))
			return false;
		return true;
	}

	public int getType(){
		return item.getType();
	}

	public String getName(){
		return name;
	}

	public String getForestAdr(){
		return fAdr;
	}
	
	public String getIp(){
		return ip;
	}

	public String getPort(){
		return port;
	}

	public String[] getRange(){
		return range;
	}

	public ArrayList<String[]> getInterface(){
		return interfaces;
	}

	public void addLink(TopoLink l){
		lnks.add(l);
	}
	
	public ArrayList<TopoLink> getLinks(){
		return lnks;
	}

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
		//return  name +"."+port+ " " + shape.getBounds2D().getX() + " " + shape.getBounds2D().getY() + " " + lnks+"\n"+"IP: " + ip + " FADR: " + fAdr;
	}

}
