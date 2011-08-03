import java.util.*;
import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;

public class topoComponent{
	protected JMenuItem item;
	protected String name;
	protected Shape shape;
	protected Point2D.Double point;

	topoComponent(String nme, JMenuItem jmi, Shape s){
		name = nme;
		item = jmi;
		shape = s;
		Rectangle2D.Double bnds = (Rectangle2D.Double) shape.getBounds2D();
		point = new Point2D.Double(bnds.getX(), bnds.getY());
	}

	@Override
	public String toString(){
		return this.getClass() + " " + name + " " + point;
	}

}
