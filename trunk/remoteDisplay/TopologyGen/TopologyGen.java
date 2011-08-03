import java.util.*;
import java.awt.*;
import java.io.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

public class TopologyGen extends JFrame{
	private int width, height;
	private boolean linking;
	private JMenuBar mb;
	private JMenu file, add, comt;
	private MenuItem open, save, close, ct, router, controller, client;
	private JCheckBoxMenuItem lnkMode;
	private JFileChooser fc;
	private File store;
	private Queue<MenuItem> paintQ;
	private Panel panel;
	private Common c;
	private JFrame cont;
	private ComtreeDialog cd;
	private ArrayList<MenuItem> ctList;
	private ArrayList<Comtree> comtrees;
	
	public TopologyGen(){				
		setPreferredSize(c.SIZE);
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
	
	private JMenuBar getTopoMenu(){
		MenuListener ml = new MenuListener();
		mb = new JMenuBar();
		mb.setDoubleBuffered(true);
		file = new JMenu("File");
		mb.add(file);
	/*
		open = new MenuItem("Open");
		open.addActionListener(ml);
		file.add(open);
	*/
		save = new MenuItem("Save", c.SAVE);
		save.addActionListener(ml);	
		file.add(save);
		close = new MenuItem("Close", c.CLOSE);
		close.addActionListener(ml);
		file.add(close);
		
		add = new JMenu("Add");
		mb.add(add);
		ct = new MenuItem("new Comtree", c.COMTREE);
		ct.addActionListener(ml);
		add.add(ct);
		router = new MenuItem("new Router", c.ROUTER);
		router.addActionListener(ml);
		add.add(router);
		controller = new MenuItem("new Controller", c.CONTROLLER);
		controller.addActionListener(ml);
		add.add(controller);
		client = new MenuItem("new Client", c.CLIENT);
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

	private class changeListener implements ChangeListener{
		public void stateChanged(ChangeEvent e) {
			if(!cd.info.isEmpty() && e.getSource().equals(cd.ok)){
				Comtree ct = cd.info.pop();
				ct.addCores(panel.getCores());
				ct.addLinks(panel.getLinks());
				comtrees.add(ct);
				MenuItem temp = new MenuItem(ct.getName(), c.COM);
				int inCt = -1;
				for(int n = 0; n < ctList.size(); n++)
					if(ctList.get(n).getText().equals(ct.getName()))
						inCt = n;
				if(inCt > 0)
					ctList.set(inCt, temp);
				else
					ctList.add(new MenuItem(ct.getName(), c.COM));
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

	private class MenuListener implements ActionListener{
		 public void actionPerformed(ActionEvent e) {
			if(e.getSource().equals(lnkMode)){
				panel.linking = lnkMode.getState();
				return;
			}
			MenuItem src = ((MenuItem)e.getSource());
			int type= src.getType();
			if(type == c.COMTREE){
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
				
			if(type == c.SAVE){
				//System.out.println("Saving..");
				int returnVal = fc.showSaveDialog(TopologyGen.this.getParent());
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					store = fc.getSelectedFile();		
					write(store.getPath());
				}
			}else if(type == c.CLOSE)
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
	
	public boolean inAdd(MenuItem m){
		for(Component mi: add.getMenuComponents())
			if(((MenuItem)mi).equals(m))
				return true;
		return false;

	}

	public boolean inComtrees(MenuItem m){
		for(Component mi:  comt.getMenuComponents())
			if(((MenuItem)mi).equals(m))
				return true;
		return false;
	}

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
				out.write("name="+tc.getName()+" type="+c.types[tc.getType()]+" fAdr="+tc.getForestAdr()+"\n"+
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
				out.write("name="+tc.getName()+" type="+c.types[tc.getType()]+" ipAdr="+tc.getIp()+" fAdr="+tc.getForestAdr()+
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
				out.write("\n;\n; # end of comtrees section");
			}

			out.close();
		}catch(Exception e){
			 e.printStackTrace();
		}
	}

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

	public static void main(String[] args){
		TopologyGen gen = new TopologyGen();
	}

}
