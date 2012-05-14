package remoteDisplay.TopologyGen;

/** @file Ports.java */

import java.awt.event.*;
import java.awt.*;
import javax.swing.*;

/**
* Context menu for selecting ports for a TopoComponent. Ports may only be selected for Routers and Clients,  not for Controllers. This in the relation Router - Controller, only a port on the Router side is selected but in the relation Router - Client a port is individually selected for both.
*/
public class Ports extends JFrame{
	JPanel control; ///< JPanel on which all JLabels, Button, and Listeners are added to
	JComboBox a, b; ///< ports for TopoComponent A and TopoComponent B
	JButton ok; ///< button that, when fired, closes the window and sets the selected port # to its respective TopoComponent
	JLabel first, last; ///< name of TopoComponent A and TopoComponent B
	TopoLink link; ///< the TopoLink that binds TopoComponent A and B
	TopoComponent[] compts; ///< the pair of TopoComponents A and B
	String[] ports; ///< the pair of port #s for TopoComponents A and B

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
		setBackground( Common.COLOR);
		setSize((int) Common.SIZE.getWidth(), (int) Common.SIZE.getHeight()/5);	
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
		control.setSize((int) Common.SIZE.getWidth(), (int) Common.SIZE.getHeight()/5);
		control.setBackground( Common.COLOR);
		
		ok = new JButton("OK");
		actListener act = new actListener();
		ok.addActionListener(act);
		link = lnk;
		compts = lnk.getConnection();
		ports = lnk.getPorts();
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
		control.setSize((int) Common.SIZE.getWidth(), (int) Common.SIZE.getHeight()/5);
		control.setBackground( Common.COLOR);
		a= null;
		b= null;
		link = lnk;
		compts = lnk.getConnection();
		ports = lnk.getPorts();
		first = new JLabel(compts[0].getName());
		last = new JLabel(compts[1].getName());
		if(!(compts[0].isController())){
			a = new JComboBox(((TopoComponent)compts[0]).getAllPorts());	
			if(ports[0] != null)
				a.setSelectedIndex(Integer.parseInt(ports[0])-1);
		}
		if(!(compts[1].isController())){
			b = new JComboBox(((TopoComponent)compts[1]).getAllPorts());
			if(ports[1] != null)
				b.setSelectedIndex(Integer.parseInt(ports[1])-1);
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
		if(!(compts[0].isController()) && !(compts[1].isController())){
			compts[0].releasePort(ports[0]);
			compts[1].releasePort(ports[1]);
			compts[0].bindPort((String) a.getSelectedItem());					
			compts[1].bindPort((String) b.getSelectedItem());
			link.setPorts((String) a.getSelectedItem(), (String) b.getSelectedItem());
		}
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

