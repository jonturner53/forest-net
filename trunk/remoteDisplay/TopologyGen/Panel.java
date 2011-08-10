package remoteDisplay.TopologyGen;

/** @file Panel.java */

import java.util.*;
import java.awt.*;
import java.io.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.table.*;
import java.text.*;

/**
* Master painting JPanel with a JLabel for the actual painting methods and multiple screen listeners for painting and moving Graphics2D components
* Recieves cues from the parent TopologyGen JFrame class as to MenuItems selected etc
*/
public class Panel extends JPanel{
	private static final int SIZE = 800; ///< default window size
	private double radius = SIZE/2; ///< wize of objects painted for ellipses
	private double width = 100; ///< size of a painted object's width
	private double height = 100; ///< size of a painted object's height 
	private int rowIndex = 1; ///< initial row number for the interfaces table
	private Point2D origin; ///< initial mouse location before dragging the surrounding selected objects
	private Rectangle2D.Double selection; ///< selection bounding box
	private int numRouters, numControllers, numClients; ///< counts for the default names as follows "Router" + numRouters, "Controller" + numControllers etc
	private boolean selecting, dragging, first; ///< booleans for program states such as dragging objects, selecting objects
	protected boolean linking; ///< state if linkingneeds to be protected so the parent TopologyGen can manipulate it
	private Canvas canvas; ///< name of the nested JLabel used for painting
	private TopoComponent min; ///< object on screen closest to a mouse pressed/clicked
	protected Queue<MenuItem> paintQ; ///< queue if any adding of objects from the parent JMenuBar
	private LinkedList<KeyEvent> keys; ///< list of all depressed keys
	private ArrayList<TopoComponent> components, selected; ///< list of all components on screen versus those in selected, which is a subset of components
	private ArrayList<Line2D.Double> lines; ///< list of all lines on screen connecting other components
	private ContextMenu cm; ///< context menu that pops up on right click over an on screen component
	private Ports p; ///< contextual pane for selecting port # for two components that define a TopoLink

	/**
	* Default Constructor
	*/
	public Panel(){
		super(new BorderLayout());
		setSize(Common.SIZE);
		setPreferredSize(Common.SIZE);
		setFocusable(true);
		setDoubleBuffered(true);
		min = null;
		dragging = false;
		linking = false;
		first = true;
		numRouters = 1;
		numControllers = 1;
		numClients = 1;
		paintQ = new LinkedList<MenuItem>();
		keys = new LinkedList<KeyEvent>();
		lines = new ArrayList<Line2D.Double>();
		components = new ArrayList<TopoComponent>();
		selected = new ArrayList<TopoComponent>();
		super.setBackground( Common.COLOR);
		
		canvas = new Canvas();
		add(canvas, BorderLayout.CENTER);

		ClickListener click = new ClickListener();
		addMouseListener(click);
		addMouseMotionListener(click);
		
		addKeyListener(new keyListener());

		setVisible(true);
	}
	
	/**
	* Context menu that pops up whenever a component is right-clicked. Attributes are set from this pane.
	*/
	private class ContextMenu extends JFrame{
		JPanel panel;
		BoxLayout box;
		TopoComponent comp;
		JButton ok, inf;
		JCheckBox check;
		JTextField name, pkts, bits, ip, fadr;
		JTable iftable, adrTable;
		MyTableModel adrModel, ifModel;
		
		/**
		* Default Constructor
		* @param tc is the TopoComponent that was clicked (may be a TopoLink)
		*/
		public ContextMenu(TopoComponent tc){
			super();
			comp = tc;
			setTitle("Set Attributes");
			setBackground( Common.COLOR);
			setSize((int) Common.SIZE.getWidth(), (int) Common.SIZE.getHeight()/5);
			panel = new JPanel();
			panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
			panel.setSize(getSize());
			panel.setBackground( Common.COLOR);
			actListener act = new actListener();
			
			if(tc instanceof TopoLink){
				TopoLink tl = (TopoLink) tc;
				pkts = new JTextField( Common.FIELD_WIDTH);
				pkts.setText(Integer.toString(tl.getPktRate()));
				bits = new JTextField( Common.FIELD_WIDTH);
				bits.setText(Integer.toString(tl.getBitRate()));
				JLabel lblPkt = new JLabel("pktrate: ");
				lblPkt.setAlignmentX(Component.CENTER_ALIGNMENT);
				panel.add(lblPkt);
				panel.add(pkts);
				JLabel lblBit = new JLabel("bitrate: ");
				lblBit.setAlignmentX(Component.CENTER_ALIGNMENT);
				panel.add(lblBit);
				panel.add(bits);
				panel.add(p.getPanel(tl));
			}else{
				name = new JTextField( Common.FIELD_WIDTH);
				name.setText(tc.getName());
				JLabel lblName = new JLabel("name: ");
				lblName.setAlignmentX(Component.CENTER_ALIGNMENT);
				panel.add(lblName);
				panel.add(name);
				if(tc.isController() || tc.isClient()){
					JLabel lblIp = new JLabel("Ip: ");
					lblIp.setAlignmentX(Component.CENTER_ALIGNMENT);
					panel.add(lblIp);
					ip = new JTextField( Common.FIELD_WIDTH);
					ip.setText(tc.getIp());
					panel.add(ip);
				}
				fadr = new JTextField( Common.FIELD_WIDTH);
				fadr.setText(tc.getForestAdr());
				JLabel lblFadr = new JLabel("ForestAdr: ");
				lblFadr.setAlignmentX(Component.CENTER_ALIGNMENT);
				panel.add(lblFadr);
				panel.add(fadr);

				if(tc.isRouter()){
					String[][] body = {
				{tc.getRange()[0], tc.getRange()[1]}};
					adrModel = new MyTableModel(body, new String[]{"Start", "End"});
					adrTable = new JTable(adrModel);
					if(adrTable.getRowCount()==0){
						adrModel.addRow(new Object[]{});
					}
				/*	
					TableColumnModel m = adrTable.getColumnModel();
					m.getColumn(0).setCellRenderer(new MyCellRenderer());
					m.getColumn(1).setCellRenderer(new MyCellRenderer());
					
					JScrollPane adr = new JScrollPane(adrTable);
					adr.setPreferredSize(new Dimension((int) adrTable.getPreferredSize().getWidth(),
						(int) panel.getPreferredSize().getHeight()/2));
				*/
					String[] columnNames = {"ifaceNum","ifaceIp","ifaceLinks","bitRate","pktRate"};

					String[][] table = new String[tc.getInterface().size()][columnNames.length];
					
					ArrayList<String[]>inter = tc.getInterface();
					
					for(int i = 0; i < inter.size(); i++){
						for(int j = 0; j < inter.get(i).length; j++)
							table[i][j] = inter.get(i)[j];
					}
					ifModel = new MyTableModel(table, columnNames);
					iftable = new JTable(ifModel);
					if(iftable.getRowCount()==0)
						ifModel.addRow(new Object[]{"1", "0.0.0.0", "1", "1000", "1000"});
					TableColumn column = null;
					for (int i = 0; i < iftable.getColumnCount(); i++) {
				    		column = iftable.getColumnModel().getColumn(i);
				        	if (i == 1)
					        	column.setPreferredWidth(100); //third column is bigger
						else
							column.setPreferredWidth(50);
					}
					JScrollPane ifp = new JScrollPane(iftable);
					ifp.setPreferredSize(new Dimension((int) iftable.getPreferredSize().getWidth(),
						(int) panel.getPreferredSize().getHeight()));	
					JLabel lblCAR = new JLabel("Client Address Range:");
					lblCAR.setAlignmentX(Component.CENTER_ALIGNMENT);
					panel.add(lblCAR);
					panel.add(adrTable);
					panel.add(Box.createRigidArea(new Dimension(5,5)));
					JLabel lblIt = new JLabel("Interface Table:");
					lblIt.setAlignmentX(Component.CENTER_ALIGNMENT);					
					panel.add(lblIt);
					
					inf = new JButton("Add Interface");
					inf.addActionListener(act);
					inf.setAlignmentX(Component.CENTER_ALIGNMENT);
					panel.add(inf);
					panel.add(ifp);
				}
			}
			ok = new JButton("OK");
			ok.addActionListener(act);
			ok.setAlignmentX(Component.CENTER_ALIGNMENT);
			panel.add(ok);
			getContentPane().add(panel);
			pack();
			setVisible(true);
		}
		
		/**
		* Button listener for adding interfaces to the Interface table or firing the ok button and processing all attribute fields
		*/
		private class actListener implements ActionListener{
			public void actionPerformed(ActionEvent e) {
				if(e.getSource().equals(inf)){
					int prev = ifModel.getRowCount();
					rowIndex++;
					ifModel.addRow(new Object[]{Integer.toString(rowIndex), "0.0.0.0", "1", "1000", "1000"});
					ifModel.fireTableRowsInserted(prev, prev+1);
					Panel.this.repaint();
					return;
				}
				rowIndex = 1;
				if(comp instanceof TopoLink){
					p.fire();
					((TopoLink) comp).setPktRate(Integer.parseInt(pkts.getText()));
					((TopoLink) comp).setBitRate(Integer.parseInt(bits.getText()));
				}else{
					if(comp.isRouter()){
						comp.setRange((String)adrModel.getValueAt(0, 0), (String)adrModel.getValueAt(0, 1));
						for(int i =0; i < ifModel.getRowCount(); i++)
							comp.addInterface((String) ifModel.getValueAt(i, 0), (String) ifModel.getValueAt(i, 1), (String) ifModel.getValueAt(i, 2), (String) ifModel.getValueAt(i, 3), (String) ifModel.getValueAt(i, 4));
					}
					comp.setName(name.getText());
					comp.setForestAdr(fadr.getText());
					if(ip != null)
						comp.setIp(ip.getText());
				}
				Panel.this.repaint();
				ContextMenu.this.dispose();
			}
		}
		
		/**
		* Local copy of a tble model for use in the interface table
		*/
		private class MyTableModel extends DefaultTableModel{
			Object[] columnNames;
			Object[][] data;
			public MyTableModel(Object[][] body, Object[] columns){
				super(body, columns);
				columnNames=columns;
				data = body;
			}
		}
		
		/**
		* not used
		* Local table cell renderer for decimel formatting of text
		*/
		private class MyCellRenderer extends DefaultTableCellRenderer{
			private DecimalFormat formatter = new DecimalFormat( "#.0000" );
			public MyCellRenderer(){ super();}

			public void setValue(Object value){
				setText(((value==null)? "":formatter.format((Number)Double.parseDouble(value.toString()))));
			}
		}
	}
	
	/**
	* Painting surface
	*/
	private class Canvas extends JLabel{
		
		/**
		* Default Constructor
		*/
		public Canvas(){
			setPreferredSize( Common.SIZE);
			setLayout(null);
			setDoubleBuffered(true);
			setVisible(true);
			setOpaque(true);
		}   

		@Override
		public void update(Graphics g) { 
			paint(g);
		} 
		
		/**
		* @Override
		* Paints components
		*/
		public void paint(Graphics g){
			super.setBackground( Common.COLOR);
			super.paint(g);
			Graphics2D g2 = ((Graphics2D) g);
			//int radius = ((int) Math.sqrt(SIZE/(paintQ.size()*FACTOR*Math.PI)));
			Ellipse2D.Double circle = new Ellipse2D.Double(radius, radius, width, height);
			circle.setFrame(radius, radius, width, height);
			Rectangle2D.Double square = new Rectangle2D.Double(radius, radius, width/1.5, height/1.5);
			square.setFrame(radius, radius, width/1.5, height/1.5);

			while(!paintQ.isEmpty()){
				MenuItem mi  = paintQ.poll();
				int type = mi.getType();
				String name = "";
				TopoComponent next = null;
				if(type == Common.ROUTER){
					name = "Router" + numRouters;
					next = new TopoComponent(name, mi, circle);
					numRouters++;
				}else if(type == Common.CONTROLLER){
					name = "Ctrl"+numControllers;
					next = new TopoComponent(name, mi, square);
					numControllers++;
				}else if(type == Common.CLIENT){
					name="Client"+numClients;
					next = new TopoComponent(name, mi, square);
					numClients++;
				}
				if(next != null){
					next.weight = 1;
					components.add(next);
				}
			}
			Stroke basic = g2.getStroke();
			//System.out.println("comps: " + components);	
			Collections.sort(components);
			for(int i = 0; i < components.size(); i++){
				TopoComponent tc = components.get(i);
				
				Shape s = tc.shape;
				Rectangle2D r = s.getBounds2D();
				Integer type = tc.getType();
				if(type == Common.LINK){
					BasicStroke line = new BasicStroke(4.0f);
					g2.setStroke(line);
					if(selected.contains(tc))
						g.setColor(Color.BLACK);
					else
						g.setColor(Color.GRAY);
					g2.draw(s);
					g2.setStroke(basic);
				}
				else if(type == Common.ROUTER){
					if(tc.isSelected() && tc.isCore())
						g.setColor(Color.DARK_GRAY);
					else if(tc.isCore())
						g.setColor(Color.LIGHT_GRAY);
					else if(tc.isSelected())
						g.setColor(Color.LIGHT_GRAY);
					else
						g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);	
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(type == Common.CONTROLLER){
					if(tc.isSelected())
						g.setColor(Color.LIGHT_GRAY);
					else
						g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					g2.drawString(tc.getName(), ((int) r.getX()) + ((int) r.getWidth())/4, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(type == Common.CLIENT){
					if(tc.isSelected())
						g.setColor(new Color(0,191,255));
					else
						g.setColor(new Color(135,206,250));

					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					g2.drawString(tc.getName(), ((int) r.getX()) + ((int) r.getWidth())/4, ((int) r.getY())+((int) r.getHeight())/2);
				}
			}
			if(selection != null){
				g2.setColor(Color.BLACK);
				g2.draw(selection);
			}
		}
	}
	
	/**
	* Mouse listener for the Canvas
	*/
	private class ClickListener implements MouseListener, MouseMotionListener{
		Point2D begin, end;
		Point init = null;
		double xdiff = 0;
		double ydiff = 0;
		Line2D.Double cLine = null;
		
		public void mousePressed(MouseEvent e) {
			requestFocusInWindow();
			TopoComponent[] c = getComponents(e.getPoint());
			if(e.getButton()==e.BUTTON3 && c != null){
				if(c.length > 1)
					cm = new ContextMenu(c[1]);
				else
					cm = new ContextMenu(c[0]);
				cm.setLocation(e.getPoint());
				cm.setVisible(true);
			}
			if(e.getButton()!=e.BUTTON1)
				return;

			origin = e.getPoint();
			if(c != null){
				dragging = true;
				if(!selected.contains(c[0])){
					c[0].selected = true;
					selected.add(c[0]);
				}
				if(linking && begin == null)
					begin = new Point2D.Double(e.getPoint().getX(), e.getPoint().getY());
			}
			else if(!shiftDown()){
				clear();
			}
			repaint();

		}
		public void mouseDragged(MouseEvent e){
			if(e.getButton()!=e.BUTTON1)
				return;
			if(linking && begin == null && getComponents(e.getPoint())!=null)
				begin = new Point2D.Double(e.getPoint().getX(), e.getPoint().getY());
			else if(linking && begin != null){
				end = e.getPoint();
				if(cLine == null)
					cLine = new Line2D.Double(begin, end);
				if(lines.contains(cLine)){
					lines.get(lines.indexOf(cLine)).setLine(begin, end);
					((Line2D.Double) getComponent(cLine).shape).setLine(begin, end);
				}else{
					lines.add(cLine);
					components.add(new TopoLink(cLine, Common.LINK));
				}

			}else if(dragging && !selecting && !linking){ //move selected component
				TopoComponent close = min(e.getPoint());
				ArrayList<TopoComponent> lnks = null;
				if(!(close instanceof TopoLink))
					lnks = close.getLinks();

				Rectangle2D bnds = close.shape.getBounds2D();
				if(first)
					init = close.getPoint();
				
				Point loc = close.getPoint();
				double y_offset = bnds.getHeight()/2;
				double x_offset = bnds.getWidth()/2;
				loc.translate((int)((e.getX()-x_offset)-loc.getX()), (int)((e.getY()- y_offset)-loc.getY()));
				if(close.shape instanceof Ellipse2D.Double)
					((Ellipse2D.Double) close.shape).setFrame(loc.getX(), loc.getY(), width, height);
				else if(close.getType()== Common.CLIENT || close.getType()== Common.CONTROLLER)
					((Rectangle2D.Double) close.shape).setRect(loc.getX(), loc.getY(), width/1.5, height/1.5);
				/*
				if(lnks!=null){
					for(TopoLink l: lnks){
						System.out.println("LINE: " + l);
					}
				}
			
				else if(close.shape instanceof Line2D.Double){
					Line2D.Double ln = (Line2D.Double) close.shape;
					x1 = ln.getX1();
					x2 = ln.getX2();
					y1 = ln.getY1();
					y2 = ln.getY2();
					if(ln.getX2() < ln.getX1()){
						x1 = ln.getX2();
						x2 = ln.getX1();
						y1 = ln.getY2();
						y2 = ln.getY1();
					}
				
					Point p1 = new Point((int) x1, (int) y1);
					Point p2 = new Point((int) x2, (int) y2);
					double dx = (loc.getX()+tc.dx)-pos.getX();
					double dy = (loc.getY()+tc.dy)-pos.getY();
					p1.translate((int) dx, (int) dy);
					p2.translate((int)((loc.getX()+tc.dx2)-p2.getX()), (int)((loc.getY()+tc.dy2)-p2.getY()));
					ln.setLine(p1, p2);
					close.shape = ln;
				}
				*/
				for(int n = 0; n < selected.size(); n++){
					TopoComponent tc = selected.get(n);
					if(!tc.equals(close)){
						//System.out.println("trans: " + tc.name + " " + tc.shape.getBounds());
						bnds = tc.shape.getBounds2D();
						y_offset = bnds.getWidth()/2;
						x_offset = bnds.getWidth()/2; 
						double x1 = 0;
						double y1 = 0;
						double x2 = 0;
						double y2 = 0;
						//calculate how much to move graphic
						Point pos = null;
						Point p2 = null;
						if(tc instanceof TopoLink){
							Line2D.Double ln = ((Line2D.Double) tc.shape);
							x1 = ln.getX1();
							x2 = ln.getX2();
							y1 = ln.getY1();
							y2 = ln.getY2();
							if(ln.getX2() < ln.getX1()){
								x1 = ln.getX2();
								x2 = ln.getX1();
								y1 = ln.getY2();
								y2 = ln.getY1();
							}

							pos = new Point((int) x1, (int)y1);
							p2 = new Point((int) x2, (int) y2);
						}else
							pos = tc.getPoint();
						if(first){
							if(tc.shape instanceof Line2D.Double){
								tc.setDiff(x1-init.getX(), y1-init.getY(), x2-init.getX(), y2-init.getY());
							}else
								tc.setDif(pos.getX()-init.getX(), pos.getY()-init.getY());
						}
						//System.out.println(tc);
						//System.out.println("pos: " + pos);
						double dx = (loc.getX()+tc.dx)-pos.getX();
						double dy = (loc.getY()+tc.dy)-pos.getY();
						pos.translate((int)dx, (int)dy);
						//System.out.println("postTrans: " + pos);
						if(tc.shape instanceof Ellipse2D.Double)
							((Ellipse2D.Double) tc.shape).setFrame(pos.getX(), pos.getY(), width, height);
						else if(tc.shape instanceof Rectangle2D.Double)
							((Rectangle2D.Double) tc.shape).setRect(pos.getX(), pos.getY(), width/1.5, height/1.5);
						else if(tc.shape instanceof Line2D.Double){
							p2.translate((int)((loc.getX()+tc.dx2)-p2.getX()), (int)((loc.getY()+tc.dy2)-p2.getY()));
							((Line2D.Double) tc.shape).setLine(pos, p2); 
						}
					}
				}
				//for(TopoComponent tc: selected)
				//	System.out.println("selected: " + tc.name + " " + tc.shape.getBounds());
				
				if(first)
					first = false;
			}
			else if(!linking){ //create selection box or update size
				selecting = true;
				if(selection == null)
					selection = new Rectangle2D.Double();
				//System.out.println("mouse: " + e.getPoint());
				Rectangle2D bnds = selection.getBounds2D();
				double y_offset = bnds.getHeight()/2;
				double x_offset = bnds.getWidth()/2;
				Point2D.Double loc = new Point2D.Double(((Rectangle2D.Double) bnds).getX(), ((Rectangle2D.Double) bnds).getY());
				loc.setLocation((int)(e.getX()-x_offset), (int)(e.getY()-y_offset));
				double x = origin.getX();
				double y = origin.getY();
				double w = e.getX()-origin.getX();
				double h = e.getY()-origin.getY();
				if(w < 0 && h < 0)
					selection.setRect(x+w,y+h,-w,-h);
				else if(w < 0)
					selection.setRect(x+w,y,-w,h);
				else if(h < 0)
					selection.setRect(x,y+h,w,-h);
				else
					selection.setRect(x,y,w,h);
			}
			revalidate();
			repaint();
		}
		public void mouseMoved(MouseEvent e){}
		public void mouseReleased(MouseEvent e){
			if(e.getButton()!=e.BUTTON1)
				return;
			dragging = false;
			if(!shiftDown())
				clear();
			end = new Point2D.Double(e.getPoint().getX(), e.getPoint().getY());
			if(linking && begin != null && cLine != null && getComponents(e.getPoint())!=null){
					
				if(lines.contains(cLine)){
					if(getComponents(e.getPoint()).length > 2){
						lines.remove(cLine);
						components.remove(getComponent(cLine));
						cLine = null;
						begin = null;
						end = null;
						clear();
						return;
					}
					lines.get(lines.indexOf(cLine)).setLine(begin, end);
					((Line2D.Double) getComponent(cLine).shape).setLine(begin, end);
					TopoComponent[] pair = addLink((TopoLink) getComponent(cLine));
					if(pair != null){
						if(pair[1]!= null){
							TopoLink lnk = (TopoLink) getComponent(cLine); 
							lnk.setConnection(pair[0], pair[1]);
							p = new Ports(lnk);
						}
					}
				}
				//cLine = (Line2D.Double) cLine.clone();
				cLine = null;
				begin = null;
				end = null;
				clear();
			}
			else{ // end terminus not over a TopoComponent
				if(lines.contains(cLine)){
					if(getComponents(e.getPoint()).length > 1){
						lines.remove(cLine);
						components.remove(getComponent(cLine));
					}
				}
			}
			if(selection != null){
				for(TopoComponent tc:components){
					if(selection.intersects(tc.shape.getBounds2D()) && !selected.contains(tc)){
						//System.out.println("add: " + tc.name + " " + tc.shape.getBounds());
						tc.setSelected(true);
						selected.add(tc);
					}
				}
				//System.out.println("numSelected: " + selected.size());
				//System.out.println(selected);
			}
			
			selection = null;
			selecting = false;
			first = true;
			repaint();
		}
		public void mouseEntered(MouseEvent e){}
		public void mouseExited(MouseEvent e){}
		public void mouseClicked(MouseEvent e){
			if(e.getButton()!=e.BUTTON1)
				return;
			if(shiftDown()){	
				TopoComponent[] c = getComponents(e.getPoint());
				if(c != null){
					for(TopoComponent tc: c){
						if(!(tc.isSelected() || selected.contains(tc))){
							tc.setSelected(true);
							selected.add(tc);
						}
					}
				}
			}
			if(getComponents(e.getPoint())==null){
				clear();
			}
			repaint();
		}
	}
	
	/**
	* Keyboard listener for Canvas
	*/
	private class keyListener implements KeyListener{
		public void keyPressed(KeyEvent e){
			if(!keys.contains(e))
				keys.add(e);
		}		
		public void keyReleased(KeyEvent e){
			if(e.getKeyCode()==KeyEvent.VK_SHIFT){
				clear();
			}
			if(e.getKeyCode()==KeyEvent.VK_BACK_SPACE){
				delete();
			}
			keys.clear();
			repaint();
		}
		public void keyTyped(KeyEvent e) {
			if(e.getKeyChar()==KeyEvent.VK_BACK_SPACE){
				delete();
			}
			keys.clear();
			repaint();
		}
	}
	
	/**
	* if BACKSPACE key is pressed, the selected object(s) are removed from the screen
	*/
	private void delete(){
		for(TopoComponent tc:selected){
			components.remove(tc);
			int type = tc.getType();
			if(type == Common.ROUTER || type == Common.CLIENT)
				tc.releasePort();
			if(type == Common.ROUTER) numRouters--;
			else if(type == Common.CONTROLLER) numControllers--;
			else if(type == Common.CLIENT) numClients--;
		}
		clear();
		lines.clear();
	}

	/**
	* Check if the SHIFT key is depressed
	*/
	private boolean shiftDown(){
		if(keys.isEmpty())
			return false;
		for(KeyEvent ke: keys)
			if(ke.getKeyCode()==KeyEvent.VK_SHIFT)
				return true;
		return false;
	}

	/**
	* @param l is the selected TopoLink
	* @return the two TopoComponents that intersect with this line on screen
	*/
	private TopoComponent[] addLink(TopoLink l){
		TopoComponent[] ends = new TopoComponent[2];
		int count = 0;
		boolean empty = true;
		for(TopoComponent tc: components){
			if(l.shape.intersects(tc.shape.getBounds2D()) && !(tc instanceof TopoLink)){
				tc.addLink(l);
				empty = false;
				ends[count++]=tc;
			}
		}
		if(empty)
			return null;
		else
			return ends;
	}
	
	/**
	* @param e is the point at mouse click
	* @return all objects at point e along the z-axis
	*/
	private TopoComponent[] getComponents(Point2D e){
		ArrayList<TopoComponent> inRange = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(tc instanceof TopoLink){
				Line2D.Double ln = ((Line2D.Double) tc.shape);
				double x = e.getX();
				double y = e.getY();
				double x1 = ln.getX1();
				double x2 = ln.getX2();
				double y1 = ln.getY1();
				double y2 = ln.getY2();
				if(ln.getX2() < ln.getX1()){
					x1 = ln.getX2();
					x2 = ln.getX1();
					y1 = ln.getY2();
					y2 = ln.getY1();
				}
				double dx = Math.abs(ln.getX1()-ln.getX2()); 
				double dy = Math.abs(ln.getY1()-ln.getY2());
				double m, diff;
				//System.out.println("dx: " + dx + " dy " + dy);
				//double angle = Math.abs(Math.atan2(ln.getY2()-ln.getY1(), ln.getX2()-ln.getX1())*(180/Math.PI));
				//System.out.println("angle: " + angle);
				//System.out.println("x1: " + x1 + " x2: " + x2 + " y1: " + y1 + " y2: " + y2);
				if(dy >= dx){ //>= 45 degrees
					m = ((double)(x2-x1)/(double)(y2-y1));
					diff = (x-x1) - m*(y-y1);
				}else{
					m = ((double)(y2-y1)/(double)(x2-x1));
					diff = (y-y1) - m*(x-x1);
				}
				//System.out.println("m: " + m+"\n"+"d: " + Math.abs(diff));
				if(Math.abs(diff) < 5)
					inRange.add(tc);
			}
			else if(tc.shape.getBounds2D().contains(e))
				inRange.add(tc);
		}
		if(inRange.isEmpty())
			return null;
		else{
			TopoComponent[] array = new TopoComponent[inRange.size()];
			for(int n = 0; n < inRange.size(); n++){
				array[n] = inRange.get(n);
			}
			return array;
		}
	}
	
	/**
	* @param s is the shape that may be part of a TopoComponent
	* @return the TopoComponent that contains s
	*/
	private TopoComponent getComponent(Shape s){
		if(components.isEmpty())
			return null;
		for(TopoComponent tc: components)
			if(tc.shape.equals(s))
				return tc;
		return null;
	}
	
	/**
	* @param mouse is the point at mouse pressed
	* @return the closest component on the screen to the point passed in.
	*/
	private TopoComponent min(Point mouse){
		if(selected == null)
			return null;
		else if(selected.isEmpty())
			return null;
		else if(selected.size() == 1)
			return selected.get(0);
		else{
			min = selected.get(0);
			for(TopoComponent next: selected){
				if(next.getPoint().distance(mouse) < min.getPoint().distance(mouse))
					min = next;
			}
		}
		return min;
	}
	
	/**
	* Clear all selected item lists and set all selected values to false
	*/
	private void clear(){
		if(selected.isEmpty())
			return;
		for(TopoComponent tc: selected)
			tc.setSelected(false);
		selected.clear();
	}
	
	/**
	* @return list of components that are routers
	*/
	public ArrayList<TopoComponent> getRouters(){
		ArrayList<TopoComponent> routers = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(tc.isRouter())
				routers.add(tc);
		}
		return routers;
	}

	/**
	* @return list of components that are cores
	*/
	public ArrayList<TopoComponent> getCores(){
		ArrayList<TopoComponent> cores = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(!(tc instanceof TopoLink))
				if(tc.isCore())
					cores.add(tc);	
		}
		return cores;
	}
	
	/**
	* @return the subset of lines from the selected list
	*/
	public ArrayList<TopoLink> getLinks(){
		ArrayList<TopoLink> l = new ArrayList<TopoLink>();
		for(TopoComponent lnk: selected){	
			if(lnk instanceof TopoLink){
				TopoComponent[] pair = new TopoComponent[2];
				int count = 0;
				for(TopoComponent next: components){
					if(next.compareLinks((TopoLink) lnk))	
						pair[count++]=next;
					if(count == 2){
						((TopoLink) lnk).setConnection(pair[0], pair[1]);
						count = 0;
					}
				}
				l.add((TopoLink) lnk);
			}
		}
		return l;
	}
	
	/**
	* @return the list of all links
	*/
	public ArrayList<TopoLink> getAllLinks(){
		ArrayList<TopoLink> l = new ArrayList<TopoLink>();
		for(TopoComponent lnk: components){	
			if(lnk instanceof TopoLink){
				TopoComponent[] pair = new TopoComponent[2];
				int count = 0;
				for(TopoComponent next: components){
					if(next.compareLinks((TopoLink) lnk))	
						pair[count++]=next;
					if(count == 2){
						((TopoLink) lnk).setConnection(pair[0], pair[1]);
						count = 0;
					}
				}
				l.add((TopoLink) lnk);
			}
		}
		return l;
	}
	
	/**
	* @return a list of all TopoComponents with type ROUTER
	*/
	public ArrayList<TopoComponent> getNodes(){
		ArrayList<TopoComponent> nodes = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(!(tc.shape instanceof Line2D.Double)){
				int type = tc.item.getType();
				if(type == Common.ROUTER)
					nodes.add(tc);
			}
		}
		return nodes;
	}
	
	/**
	* @return a list of all TopoComponents with type CONTROLLER
	*/
	public ArrayList<TopoComponent> getControllers(){
		ArrayList<TopoComponent> ctrl = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(!(tc.shape instanceof Line2D.Double)){
				int type = tc.item.getType();
				if(type == Common.CONTROLLER)
					ctrl.add(tc);
	
			}
		}
		return ctrl;
	}
	
	/**
	* @return a list of all TopoComponents with type CLIENT
	*/
	public ArrayList<TopoComponent> getClients(){
		ArrayList<TopoComponent> clts = new ArrayList<TopoComponent>();
		for(TopoComponent tc: clts){
			if(!(tc.shape instanceof Line2D.Double)){
				int type = tc.item.getType();
				if(type == Common.CLIENT)
					clts.add(tc);
	
			}
		}
		return clts;
	}
	
	/**
	* @return a list of all TopoComponents with type CONTROLLER or CLIENT
	*/
	public ArrayList<TopoComponent> getLeafNodes(){
		ArrayList<TopoComponent> container = getControllers();
		container.addAll(getClients());
		return container;
	}


}
