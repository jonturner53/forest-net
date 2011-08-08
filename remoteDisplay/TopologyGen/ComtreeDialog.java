/** @file ComtreeDialog.java */

package TopologyGen;

import java.util.*;
import java.awt.*;
import java.io.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.event.*;
import javax.swing.DefaultListModel;

public class ComtreeDialog extends JPanel{
	private JComboBox cores;
	private JTextField name;
	private MyTableModel tableModel;
	private JTable table;
	private DefaultListModel listModel;
	private JList list;
	protected JButton ok, cancel;
	protected Stack<Comtree> info;
	protected Object[] objs;
	protected boolean built = false;
	private Common c;
	
	public ComtreeDialog(){
		super();
		info = new Stack<Comtree>();
	}

	public ComtreeDialog(Comtree ct){
		
	}
	
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
			name = new JTextField(c.FIELD_WIDTH);
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
			setSize((int) c.SIZE.getWidth(), (int) c.SIZE.getHeight()/2);
			setPreferredSize(getSize());
			setBackground(c.COLOR);
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

	private class listListener implements ListSelectionListener{
		public void valueChanged(ListSelectionEvent e){
			objs =  ((JList)e.getSource()).getSelectedValues();
			cores.removeAllItems();
			for(Object obj: objs)
				cores.addItem(obj);
			revalidate();	
		}
	}

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
