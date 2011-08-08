 package TopologyGen;

import java.awt.event.*;
import javax.swing.*;

public class MenuItem extends JMenuItem implements Comparable{
	private Integer type = null;

	public MenuItem(String text){
		super(text);
	}
	
	public MenuItem(String text, int mark){
		super(text);
		type = mark;
	}

	public Integer getType(){
		return type;
	}

	@Override
	public int compareTo(Object obj){
		MenuItem mi = (MenuItem) obj;
		return getText().compareTo(mi.getText());
	}
}
