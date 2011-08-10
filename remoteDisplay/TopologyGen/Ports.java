package remoteDisplay.TopologyGen;

/** @file Ports.java */

import java.awt.event.*;
import java.awt.*;
import javax.swing.*;

/**
* Context menu for selecting ports for a TopoComponent
*/
public class Ports extends JFrame{
	JPanel control;
	JComboBox a, b;
	JButton ok;
	JLabel first, last;
	TopoLink link;
	TopoComponent[] compts;
	Common c;
	
	/**
	* Empty constructor
	*/
	public Ports(){}
	
	/**
	* Default constructor
	* @param link is the link that binds two TopoComponents and whose individual ports that use this link are being set
	*/
	public Ports(TopoLink lnk){
		super();
		setTitle("Set Ports");
		setBackground(c.COLOR);
		setSize((int) c.SIZE.getWidth(), (int) c.SIZE.getHeight()/5);
		
		getContentPane().add(setPanel(lnk));
		pack();
		setVisible(true);
	}
	
	/**
	* build panel based on the type of TopoComponents
	* @param lnk that binds the two TopoComponents
	* @return jpanel with labels, boxes, and listeners included
	*/
	public JPanel setPanel(TopoLink lnk){
		control = new JPanel();
		control.setSize((int) c.SIZE.getWidth(), (int) c.SIZE.getHeight()/5);
		control.setBackground(c.COLOR);
		
		ok = new JButton("OK");
		actListener act = new actListener();
		ok.addActionListener(act);
		link = lnk;
		compts = lnk.getConnection();
		first = new JLabel(compts[0].getName());
		last = new JLabel(compts[1].getName());
		if(!(compts[0].isController()))
			a = new JComboBox(((TopoComponent)compts[0]).getAvaPorts());
		if(!(compts[1].isController()))
			b = new JComboBox(((TopoComponent)compts[1]).getAvaPorts());
		control.add(first);
		if(a!=null)
			control.add(a);
		control.add(new JLabel("- - - - - -"));
		control.add(last);
		if(b!=null)
			control.add(b);
		control.add(ok);
		control.setVisible(true);
		return control;

	}
	
	/**
	* called if setPanel has already been called
	* @param lnk that binds the two TopoComponents
	* @return jpanel with labels, boxes, and listeners included
	*/
	public JPanel getPanel(TopoLink lnk){
		control = new JPanel();
		control.setSize((int) c.SIZE.getWidth(), (int) c.SIZE.getHeight()/5);
		control.setBackground(c.COLOR);
		a= null;
		b= null;
		link = lnk;
		compts = lnk.getConnection();
		first = new JLabel(compts[0].getName());
		last = new JLabel(compts[1].getName());
		if(!(compts[0].isController())){
			a = new JComboBox(((TopoComponent)compts[0]).getAllPorts());
			a.setSelectedIndex(Integer.parseInt(compts[0].getPort())-1);
		}
		if(!(compts[1].isController())){
			b = new JComboBox(((TopoComponent)compts[1]).getAllPorts());
			b.setSelectedIndex(Integer.parseInt(compts[1].getPort())-1);
		}
		control.add(first);
		if(a!=null)
			control.add(a);
		control.add(new JLabel("- - - - - -"));
		control.add(last);
		if(b!=null)
			control.add(b);
		control.setVisible(true);
		return control;
	
	}
	
	/**
	* set ports of the TopoComponents and add these two TopoComponents to the parent link
	*/
	public void  fire(){
		if(!(compts[0].isController()))
			compts[0].bindPort((String) a.getSelectedItem());					
		if(!(compts[1].isController()))
			compts[1].bindPort((String) b.getSelectedItem());
		link.setConnection(compts);	
	}
	
	/**
	* Button listener to fire ok button
	*/
	private class actListener implements ActionListener{
		public void actionPerformed(ActionEvent e) {
			if(e.getSource().equals(ok)){
				fire();
			}
			Ports.this.dispose();
		}
	}
}

