/** @file TopologyGen.java */ 

package remoteDisplay.TopologyGen;

import java.util.*;
import java.awt.*;
import java.io.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import remoteDisplay.Comtree;

/**
* JFrame class to drive TopologyGen's subclasses as well as menuing system.
* This class is the top level of the TopologyGen gui and handles much of the interaction between the Panel sub class and TopologyGen's own menus in Comtree Dialog
*/
public class TopologyGen extends JFrame{
	private int width, height; ///< width and height of the frame
	private boolean linking; ///< flag on the binary JCheckBox for if lines should be drawn when a mouse is pressed and dragged
	private JMenuBar mb; ///< menu bar that contains the menuing toolbar system for TopologyGen to save the gui to a formatted text file, create new objects, or close the program
	private JMenu file, add, comt; ///< top level directories for adding, saveing, closing, or editing existing comtrees
	private MenuItem open, save, close, ct, router, controller, client; ///< children menu items of the parent directories such that File> Save, Close Add > New Comtree, new Controller, new Router, and new Client and comt > any new Comtrees
	private JCheckBoxMenuItem lnkMode; ///< binary switch for the drawing of lines
	private JFileChooser fc; ///< Swing component for designating the path to save the formatted Forest topology file
	private File store; ///< the file containing the path for writing the Forest topology file
	private Queue<MenuItem> paintQ; ///< queue of Add ... events to be sent to the Panel for painting
	private Panel panel; ///< main painting surface and ui listener
	private JFrame cont; ///< container for the context menu of all on screen objects
	private ComtreeDialog cd; ///< context menu for seting the attributes of a comtree
	private ArrayList<MenuItem> ctList; ///< list form of the menubar system
	private ArrayList<Comtree> comtrees; ///< list of comtrees as they arrive from the ComtreeDialog
	
	/**
	* Defuault Constructor
	*/
	public TopologyGen(){				
		setPreferredSize(Common.SIZE);
		setLayout(null);
		paintQ = new LinkedList<MenuItem>();
		fc = new JFileChooser();
		setJMenuBar(getTopoMenu());
		cont = new JFrame();
		cont.setTitle("Create Comtree");
		cont.setDefaultCloseOperation(HIDE_ON_CLOSE);
		comtrees = new ArrayList<Comtree>();
		ctList = new ArrayList<MenuItem>();
		cd = new ComtreeDialog();
		panel = new Panel();

		getContentPane().add(panel);
		setTitle("TopologyGen");
		setDefaultCloseOperation(EXIT_ON_CLOSE);
		pack();
		validate();
		setVisible(true);
	}
	
	/**
	* builds the JMenuBar with is associated listeners and Menu directories
	*/
	private JMenuBar getTopoMenu(){
		MenuListener ml = new MenuListener();
		mb = new JMenuBar();
		mb.setDoubleBuffered(true);
		file = new JMenu("File");
		mb.add(file);
		
		save = new MenuItem("Save", Common.SAVE);
		save.addActionListener(ml);	
		file.add(save);
		close = new MenuItem("Close", Common.CLOSE);
		close.addActionListener(ml);
		file.add(close);
		
		add = new JMenu("Add");
		mb.add(add);
		ct = new MenuItem("new Comtree", Common.COMTREE);
		ct.addActionListener(ml);
		add.add(ct);
		router = new MenuItem("new Router", Common.ROUTER);
		router.addActionListener(ml);
		add.add(router);
		controller = new MenuItem("new Controller", Common.CONTROLLER);
		controller.addActionListener(ml);
		add.add(controller);
		client = new MenuItem("new Client", Common.CLIENT);
		client.addActionListener(ml);
		add.add(client);
		comt = new JMenu("Comtrees");
		comt.addActionListener(ml);
		mb.add(comt);
		lnkMode = new JCheckBoxMenuItem("Linking Mode");
		lnkMode.setDoubleBuffered(true);
		lnkMode.addActionListener(ml);
		mb.add(lnkMode);
		return mb;
	}

	/**
	* Listens to state changes from the ComtreeDialog class and adds a new Comtree to the JMenu of Comtrees.
	*/
	private class changeListener implements ChangeListener{
		public void stateChanged(ChangeEvent e) {
			if(!cd.info.isEmpty() && e.getSource().equals(cd.ok)){
				Comtree ct = cd.info.pop();
				ct.addCores(panel.getCores());
				ct.addLinks(panel.getLinks());
				comtrees.add(ct);
				MenuItem temp = new MenuItem(ct.getName(), Common.COM);
				int inCt = -1;
				for(int n = 0; n < ctList.size(); n++)
					if(ctList.get(n).getText().equals(ct.getName()))
						inCt = n;
				if(inCt > 0)
					ctList.set(inCt, temp);
				else
					ctList.add(new MenuItem(ct.getName(), Common.COM));
				comt.removeAll();
				Collections.sort(ctList);
				for(MenuItem mi: ctList){
					mi.addActionListener(new MenuListener());
					comt.add(mi);
				}
				validate();
			}
			cont.hide();
		}
	}

	/**
	* Listenens to the master JMenuBar for click and selection events and maps them to a particular action that is passed down to the Panel class.
	*/
	private class MenuListener implements ActionListener{
		 public void actionPerformed(ActionEvent e) {
			if(e.getSource().equals(lnkMode)){
				panel.linking = lnkMode.getState();
				return;
			}
			MenuItem src = ((MenuItem)e.getSource());
			int type= src.getType();
			if(type == Common.COMTREE){
				if(!(panel.getRouters().isEmpty() || panel.getLinks().isEmpty() || panel.getAllLinks().isEmpty())){
					cd.build(panel.getRouters());
					changeListener change = new changeListener();
					cd.ok.addChangeListener(change);
					cont.setPreferredSize(cd.getSize());
					cont.getContentPane().add(cd);
					cont.pack();
					cont.show();
				}else{
					StringBuilder sb = new StringBuilder();
					if(panel.getRouters().isEmpty())
						sb.append("No Routers\n");
					if(panel.getAllLinks().isEmpty())
						sb.append("No Links\n");
					if(panel.getLinks().isEmpty())
						sb.append("No Links Selected");
					JOptionPane.showMessageDialog(TopologyGen.this, sb.toString(), "Comtree Build Error", JOptionPane.ERROR_MESSAGE);
				}
			}
				
			if(type == Common.SAVE){
				//System.out.println("Saving..");
				int returnVal = fc.showSaveDialog(TopologyGen.this.getParent());
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					store = fc.getSelectedFile();		
					write(store.getPath());
				}
			}else if(type == Common.CLOSE)
				System.exit(1);
			else if(inAdd(src))
				paintQ.offer(src);
			else if(inComtrees(src)){
				cd.build(panel.getRouters());
				changeListener change = new changeListener();
				cd.ok.addChangeListener(change);
				cont.setPreferredSize(cd.getSize());
				cont.getContentPane().add(cd);
				cont.pack();
				cont.show();
				System.out.println(src.getText());
				panel.repaint();
			}
			panel.paintQ = paintQ;	
		 	repaint();
			validate();
		 }
	}
	
	/**
	* @param MenuItem m item checked for in Add menu directory
	* @return true if MenuItem exists in the Add menu directory, false otherwise
	*/
	public boolean inAdd(MenuItem m){
		for(Component mi: add.getMenuComponents())
			if(((MenuItem)mi).equals(m))
				return true;
		return false;

	}

	/**
	* @param MenuItem m item checked for in Comtree menu directory
	* @return true if MenuItem exists in the Add menu directory, false otherwise
	*/
	public boolean inComtrees(MenuItem m){
		for(Component mi:  comt.getMenuComponents())
			if(((MenuItem)mi).equals(m))
				return true;
		return false;
	}

	/**
	* @param filename is the pathname passed in via the FileChooser Dialog that is opened after selecting File>Save
	* writes a textfile to the specified absolute path in the format of a forest topology file.
	*/
	public void write(String filename){
		try{
			FileWriter fw = new FileWriter(filename);
			BufferedWriter out = new BufferedWriter(fw);
			//print node data
			out.write("Routers # starts section that defines routers\n");
			ArrayList<TopoComponent> nodes = panel.getNodes();
			for(TopoComponent tc: nodes){
				Rectangle2D r = tc.shape.getBounds2D();
				String x = Integer.toString((int) r.getX());
				String y = Integer.toString((int) r.getY());
				out.write("\n# define "+tc.getName()+" router\n");
				out.write("name="+tc.getName()+" type="+Common.types[tc.getType()]+" fAdr="+tc.getForestAdr()+"\n"+
				"location=("+x+","+y+")"+" clientAdrRange=("+tc.getRange()[0]+"-"+tc.getRange()[1]+")\n\n");
				//print interfaces
				out.write("interfaces # router interfaces:\n# ifaceNum\tifaceIp\tifaceLinks\tbitRate\tpktRate\n");
				for(String[] inf: tc.getInterface()){
					for(int n = 0; n < inf.length; n++){
						if(n < inf.length-1)
							out.write(inf[n]+"\t");
						else
							out.write(inf[n]+";\n");
					}
				}
				out.write("end\n;\n");
			}
			//print leaf nodes
			out.write(";\n"+
			"\nLeafNodes # starts section defining other leaf nodes\n"+
			"\n# name\tnodeType\tipAdr\tforestAdr\tx-coord\ty-coord\n");
			for(TopoComponent tc: panel.getLeafNodes()){
				Rectangle2D r = tc.shape.getBounds2D();
				String x = Integer.toString((int) r.getX());
				String y = Integer.toString((int) r.getY());
				out.write("name="+tc.getName()+" type="+Common.types[tc.getType()]+" ipAdr="+tc.getIp()+" fAdr="+tc.getForestAdr()+
				"\nlocation=("+x+","+y+");\n");
			}
			out.write(";\n\n");
			//print all links
			out.write("Links # starts section that defines links\n# name.link#:name.link#\tpacketRate\tbitRate\n");
			for(TopoLink link: panel.getAllLinks()){
				out.write("link=("+printLink(link)+")"+" bitRate="+link.getBitRate()+" pktRate="+link.getPktRate()+";\n");
			}
			out.write(";\n\n");
			//print comtrees
			out.write("Comtrees # starts section that defines comtrees\n");
			for(Comtree ct: comtrees){
				out.write("\ncomtree="+ct.getName()+" root="+ct.getRoot());
				for(TopoComponent core: ct.getCores())
					out.write(" core="+core.getName());
				out.write("# may have multiple core=x assignments\n");
				String[] rates = ct.getRates();
				out.write("bitRateUp="+rates[0]+" bitRateDown="+rates[1]+" pktRateUp="+rates[2]+" pktRateDown="+rates[3]+
					"\nleafBitRateUp="+rates[4]+" leafBitRateDown="+rates[5]+" leafPktRateUp="+rates[6]+" leafPktRateDown="+rates[7]+"\n");
				for(TopoLink tl: ct.getLinks())
					out.write("link=("+printLink(tl)+") ");
			}
			out.write("\n;\n; # end of comtrees section");
			out.close();
		}catch(Exception e){
			 e.printStackTrace();
		}
	}

	/**
	* @param TopoLink l is the TopoLink used for building a string representation that conforms to the forest topology
	* @return a String with the pair of TopoComponent's names seperated by a comma
	*/
	private String printLink(TopoLink l){
		TopoComponent[] link = l.getConnection();
		StringBuilder sb = new StringBuilder();
		if(link[0].hasPort())
			sb.append(link[0].getName()+"."+link[0].getPort());
		else
			sb.append(link[0].getName());
		sb.append(",");
		if(link[1].hasPort())
			sb.append(link[1].getName()+"."+link[1].getPort());
		else
			sb.append(link[1].getName());

		return sb.toString();
	}
	
	/**
	* main function that launches TopologyGen
	*/
	public static void main(String[] args){
		TopologyGen gen = new TopologyGen();
	}

}
