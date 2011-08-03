import java.util.*;
import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;

public class TopoLink extends TopoComponent{
	protected int bitrate, pktrate;
	protected TopoComponent[] connect;

	public TopoLink(Shape s, int mark){
		super();
		shape = s;
		type = mark;
		connect = new TopoComponent[2];
	}
	
	public void setConnection(TopoComponent[] compts){
		connect = compts;
	}

	public void setConnection(TopoComponent a, TopoComponent b){
		connect[0] =a;
		connect[1] =b;
	}

	public void setBitRate(int bits){
		bitrate = bits;
	}

	public void setPktRate(int pkt){
		pktrate = pkt;
	}

	public int getBitRate(){
		return bitrate;
	}

	public int getPktRate(){
		return pktrate;
	}

	public TopoComponent[] getConnection(){
		return connect;
	}
	
	@Override
	public int getType(){
		return c.LINK;
	}

	@Override
	public String toString(){
		return new String(getClass() + "	"+Integer.toString(bitrate) + "	" + Integer.toString(pktrate));
	}
}
