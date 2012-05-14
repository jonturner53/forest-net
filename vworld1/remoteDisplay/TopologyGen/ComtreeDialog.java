/** @file ComtreeDialog.java */

package remoteDisplay.TopologyGen;

import java.util.*;
import java.awt.*;
import java.io.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.event.*;
import javax.swing.DefaultListModel;
import remoteDisplay.Comtree;

/**
* GRaphical context menu for setting the attributes of a Comtree. Selecting the cores, which of the cores is root, the comtree number, and the bitrates, for itself and all leaves both up and down as well as the same for packet rates
*/
public class ComtreeDialog extends JPanel{
	private JComboBox cores; ///< Swing JComboBox for selecting cores for the comtree being constructed
	private JTextField name; ///< comtree #
	private MyTableModel tableModel; ///< local build of the regular DefaultTableModel for the rates table
	private JTable table; ///< table that uses the above MyTableModel
	private DefaultListModel listModel; ///< local build of the regular DefaultListModel for selecting cores
	private JList list; ///< list that uses the above Model
	protected JButton ok, cancel; ///< buttons for saving each fields' values or closing the window without saving the values
	protected Stack<Comtree> info; ///< transfer structure for building a new Comtree object and passing it to the TopologyGen class by way of a change listener
	protected Object[] objs; ///< wrapper for the list of values in the JComboBox
	protected boolean built = false; ///< flag set to true after first call of this class
	
	/**
	* Default constructor
	*/
	public ComtreeDialog(){
		super();
		info = new Stack<Comtree>();
	}
	
	/**
	* main method for building the inital Comtree menu
	* @param compts is the list of all routers on screen
	*/
	public void build(ArrayList<TopoComponent> compts){
		if(!built){		
			setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
			listModel = new DefaultListModel();				
			for(Object obj: compts.toArray())
				listModel.addElement(obj);
			list = new JList(listModel);
			list.setVisibleRowCount(compts.size());
			list.addListSelectionListener(new listListener());
			list.setAlignmentX(Component.CENTER_ALIGNMENT);
			list.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
			list.setLayoutOrientation(JList.VERTICAL);
			JScrollPane listScroller = new JScrollPane(list);
			add(listScroller);
			ActionListener actor = new actorListener();
			
			JLabel root = new JLabel("Root: ");
			root.setAlignmentX(Component.CENTER_ALIGNMENT);
			add(root);
			objs = new Object[0];
			cores = new JComboBox(objs);
			add(cores);
			JLabel lblName = new JLabel("Comtree #: ");
			lblName.setAlignmentX(Component.CENTER_ALIGNMENT);
			add(lblName);
			name = new JTextField( Common.FIELD_WIDTH);
			name.setMaximumSize(name.getPreferredSize());
			name.setEditable(true);
			add(name);
			String[][] body = {{"0", "0", "0", "0"}, {"0", "0", "0", "0"}};
			String[] columns= {"bitRate", "pktRate", "leafBitRate", "leafPktRate"};
			tableModel = new MyTableModel(body, columns);
			table = new JTable(tableModel);
			JScrollPane rates = new JScrollPane(table);
			rates.setPreferredSize(new Dimension((int) table.getPreferredSize().getWidth(),
				(int) getPreferredSize().getHeight()));
			add(rates);
			ok = new JButton("OK");
			ok.addActionListener(actor);
			ok.setAlignmentX(Component.CENTER_ALIGNMENT);
			add(ok, actor);
			setSize((int)  Common.SIZE.getWidth(), (int)  Common.SIZE.getHeight()/2);
			setPreferredSize(getSize());
			setBackground( Common.COLOR);
			setFocusable(true);
			setDoubleBuffered(true);
			built = true;
		}else{
			listModel.removeAllElements();
			for(Object obj: compts.toArray())
				listModel.addElement(obj);
			list.setModel(listModel);
		}
	}

	/**
	* listener for the JComboBox of selected values
	*/
	private class listListener implements ListSelectionListener{
		public void valueChanged(ListSelectionEvent e){
			objs =  ((JList)e.getSource()).getSelectedValues();
			cores.removeAllItems();
			for(Object obj: objs)
				cores.addItem(obj);
			revalidate();	
		}
	}

	/**
	* listener for the ok button to build this comtree with the inputted values
	*/
	private class actorListener implements ActionListener{
		public void actionPerformed(ActionEvent e) {
			if(e.getSource().equals(ok)){
				for(Object obj: objs)
					((TopoComponent) obj).setCore(true);
				cores.addActionListener(new actorListener());

				TopoComponent root = (TopoComponent)cores.getSelectedItem();
				root.isRoot = true;
				Comtree tree = new Comtree(name.getText(), root.name);
				tree.setRates((String) table.getValueAt(0, 0), (String) table.getValueAt(1, 0), (String) table.getValueAt(0, 1), (String) table.getValueAt(1, 1),(String)table.getValueAt(0, 2), (String)table.getValueAt(1, 2), (String)table.getValueAt(0, 3), (String)table.getValueAt(1, 3));
				info.push(tree);
			}
		}
	}
}
