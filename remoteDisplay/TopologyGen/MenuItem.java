 package remoteDisplay.TopologyGen;

/** @file MenuItem.java */

import java.awt.event.*;
import javax.swing.*;

/**
* MenuItem extends Java's JMenuItem and gives each MenuItem an integer type as defined in the Common class
*/
public class MenuItem extends JMenuItem implements Comparable{
	private Integer type = null;

	/**
	* constructor
	* @param text is the name of the MenuItem that is displayed in a JMenuBar
	*/
	public MenuItem(String text){
		super(text);
	}
	
	/**
	* constructor
	* @param text is the name of the MenuItem
	* @param mark is a constant int value as defined in the Common class
	* examples include ROUTER, CONTROLER, CLIENT, etc
	*/
	public MenuItem(String text, int mark){
		super(text);
		type = mark;
	}

	/**
	* @return the integer constant representing this MenuItem
	*/
	public Integer getType(){
		return type;
	}

	@Override
	public int compareTo(Object obj){
		MenuItem mi = (MenuItem) obj;
		return getText().compareTo(mi.getText());
	}
}
