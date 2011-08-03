import java.util.*;
import java.awt.*;
import java.io.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.table.*;
import java.text.*;

public class Panel extends JPanel{
	private static final int SIZE = 800;
	private static final double FACTOR = 1;
	private double radius = SIZE/2;
	private double width = 100;
	private double height = 100;
	private int rowIndex = 1;
	private Point2D origin;
	private Rectangle2D.Double selection;
	private int numRouters, numControllers, numClients;
	private boolean selecting, dragging, first, linksSet;
	protected boolean linking;
	private Canvas canvas;
	private TopoComponent min;
	protected Queue<MenuItem> paintQ;
	private LinkedList<KeyEvent> keys;
	private ArrayList<TopoComponent> components, selected;
	private ArrayList<Line2D.Double> lines;
	private ContextMenu cm;
	private Ports p;
	private Common c;

	public Panel(){
		super(new BorderLayout());
		setSize(c.SIZE);
		setPreferredSize(c.SIZE);
		setFocusable(true);
		setDoubleBuffered(true);
		min = null;
		dragging = false;
		linking = false;
		first = true;
		linksSet = false;
		numRouters = 1;
		numControllers = 1;
		numClients = 1;
		paintQ = new LinkedList<MenuItem>();
		keys = new LinkedList<KeyEvent>();
		lines = new ArrayList<Line2D.Double>();
		components = new ArrayList<TopoComponent>();
		selected = new ArrayList<TopoComponent>();
		super.setBackground(c.COLOR);
		
		canvas = new Canvas();
		add(canvas, BorderLayout.CENTER);

		ClickListener click = new ClickListener();
		addMouseListener(click);
		addMouseMotionListener(click);
		
		addKeyListener(new keyListener());

		setVisible(true);
	}
	
	private class ContextMenu extends JFrame{
		JPanel panel;
		BoxLayout box;
		TopoComponent comp;
		JButton ok, inf;
		JCheckBox check;
		JTextField name, pkts, bits, ip, fadr;
		JTable iftable, adrTable;
		MyTableModel adrModel, ifModel;

		public ContextMenu(TopoComponent tc){
			super();
			comp = tc;
			setTitle("Set Attributes");
			setBackground(c.COLOR);
			setSize((int) c.SIZE.getWidth(), (int) c.SIZE.getHeight()/5);
			panel = new JPanel();
			panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
			panel.setSize(getSize());
			panel.setBackground(c.COLOR);
			actListener act = new actListener();
			
			if(tc instanceof TopoLink){
				TopoLink tl = (TopoLink) tc;
				pkts = new JTextField(c.FIELD_WIDTH);
				pkts.setText(Integer.toString(tl.getPktRate()));
				bits = new JTextField(c.FIELD_WIDTH);
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
				name = new JTextField(c.FIELD_WIDTH);
				name.setText(tc.getName());
				JLabel lblName = new JLabel("name: ");
				lblName.setAlignmentX(Component.CENTER_ALIGNMENT);
				panel.add(lblName);
				panel.add(name);
				if(tc.isController()){
					JLabel lblIp = new JLabel("Ip: ");
					lblIp.setAlignmentX(Component.CENTER_ALIGNMENT);
					panel.add(lblIp);
					ip = new JTextField(c.FIELD_WIDTH);
					ip.setText(tc.getIp());
					panel.add(ip);
				}
				fadr = new JTextField(c.FIELD_WIDTH);
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
				}
				Panel.this.repaint();
				ContextMenu.this.dispose();
			}
		}

		private class MyTableModel extends DefaultTableModel{
			Object[] columnNames;
			Object[][] data;
			public MyTableModel(Object[][] body, Object[] columns){
				super(body, columns);
				columnNames=columns;
				data = body;
			}
		}

		private class MyCellRenderer extends DefaultTableCellRenderer{
			private DecimalFormat formatter = new DecimalFormat( "#.0000" );
			public MyCellRenderer(){ super();}

			public void setValue(Object value){
				setText(((value==null)? "":formatter.format((Number)Double.parseDouble(value.toString()))));
			}
		}
	}
	
	private class Canvas extends JLabel{
		public Canvas(){
			setPreferredSize(c.SIZE);
			setLayout(null);
			setDoubleBuffered(true);
			setVisible(true);
			setOpaque(true);
		}   

		@Override
		public void update(Graphics g) { 
			paint(g);
		} 

		@Override
		public void paint(Graphics g){
			super.setBackground(c.COLOR);
			super.paint(g);
			Graphics2D g2 = ((Graphics2D) g);
			//int radius = ((int) Math.sqrt(SIZE/(paintQ.size()*FACTOR*Math.PI)));
			Ellipse2D.Double circle = new Ellipse2D.Double(radius, radius, width, height);
			circle.setFrame(radius, radius, width, height);
			Rectangle2D.Double square = new Rectangle2D.Double(radius, radius, width, height);
			square.setFrame(radius, radius, width, height);
			while(!paintQ.isEmpty()){
				MenuItem mi  = paintQ.poll();
				int type = mi.getType();
				String name = "";
				TopoComponent next = null;
				if(type == c.ROUTER){
					name = "Router" + numRouters;
					next = new TopoComponent(name, mi, circle);
					numRouters++;
				}else if(type == c.CONTROLLER){
					name = "Controller"+numControllers;
					next = new TopoComponent(name, mi, square);
					numControllers++;
				}else if(type == c.CLIENT){
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
				if(type == c.LINK){
					BasicStroke line = new BasicStroke(4.0f);
					g2.setStroke(line);
					if(selected.contains(tc))
						g.setColor(Color.BLACK);
					else
						g.setColor(Color.GRAY);
					g2.draw(s);
					g2.setStroke(basic);
				}
				else if(type == c.ROUTER){
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
				else if(type == c.CONTROLLER){
					if(tc.isSelected())
						g.setColor(Color.LIGHT_GRAY);
					else
						g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					g2.drawString(tc.getName(), ((int) r.getX()) + ((int) r.getWidth())/6, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(type == c.CLIENT){
					if(tc.isSelected())
						g.setColor(new Color(0,191,255));
					else
						g.setColor(new Color(135,206,250));

					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					g2.drawString(tc.getName(), ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
			}
			if(selection != null){
				g2.setColor(Color.BLACK);
				g2.draw(selection);
			}
		}
	}

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
					components.add(new TopoLink(cLine, c.LINK));
				}

			}else if(dragging && !selecting && !linking){ //move selected component
				TopoComponent close = min(e.getPoint());
				TopoComponent[] stack = getComponents(e.getPoint());
				Rectangle2D bnds = close.shape.getBounds2D();
				if(first)
					init = close.getPoint();
				
				Point loc = close.getPoint();
				double y_offset = bnds.getHeight()/2;
				double x_offset = bnds.getWidth()/2;
				loc.translate((int)((e.getX()-x_offset)-loc.getX()), (int)((e.getY()- y_offset)-loc.getY()));
				if(close.shape instanceof Ellipse2D.Double)
					((Ellipse2D.Double) close.shape).setFrame(loc.getX(), loc.getY(), width, height);
				else if(close.shape instanceof Rectangle2D.Double)
					((Rectangle2D.Double) close.shape).setRect(loc.getX(), loc.getY(), width, height);
				for(TopoComponent tc: stack){
					if(!(tc.equals(close))){
						if(tc instanceof TopoLink)
							System.out.println("LINE");
					}
				}
				/*
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
							((Rectangle2D.Double) tc.shape).setRect(pos.getX(), pos.getY(), width, height);
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
					TopoComponent[] pair = addLink(cLine);
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
	
	private void delete(){
		for(TopoComponent tc:selected){
			components.remove(tc);
			int type = tc.getType();
			if(type == c.ROUTER || type == c.CLIENT)
				tc.releasePort();
			if(type == c.ROUTER) numRouters--;
			else if(type == c.CONTROLLER) numControllers--;
			else if(type == c.CLIENT) numClients--;
		}
		clear();
		lines.clear();
	}

	private boolean shiftDown(){
		if(keys.isEmpty())
			return false;
		for(KeyEvent ke: keys)
			if(ke.getKeyCode()==KeyEvent.VK_SHIFT)
				return true;
		return false;
	}


	private TopoComponent[] addLink(Line2D.Double l){
		TopoComponent[] ends = new TopoComponent[2];
		int count = 0;
		boolean empty = true;
		for(TopoComponent tc: components){
			if(l.intersects(tc.shape.getBounds2D()) && !(tc instanceof TopoLink)){
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

	private TopoComponent getComponent(Shape s){
		if(components.isEmpty())
			return null;
		for(TopoComponent tc: components)
			if(tc.shape.equals(s))
				return tc;
		return null;
	}

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

	private void clear(){
		if(selected.isEmpty())
			return;
		for(TopoComponent tc: selected)
			tc.setSelected(false);
		selected.clear();
	}
	
	public ArrayList<TopoComponent> getRouters(){
		ArrayList<TopoComponent> routers = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(tc.isRouter())
				routers.add(tc);
		}
		return routers;
	}

	public ArrayList<TopoComponent> getCores(){
		ArrayList<TopoComponent> cores = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(!(tc instanceof TopoLink))
				if(tc.isCore())
					cores.add(tc);	
		}
		return cores;
	}
	
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

	public ArrayList<TopoComponent> getNodes(){
		ArrayList<TopoComponent> nodes = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(!(tc.shape instanceof Line2D.Double)){
				int type = tc.item.getType();
				if(type == c.ROUTER)
					nodes.add(tc);
			}
		}
		return nodes;
	}

	public ArrayList<TopoComponent> getControllers(){
		ArrayList<TopoComponent> ctrl = new ArrayList<TopoComponent>();
		for(TopoComponent tc: components){
			if(!(tc.shape instanceof Line2D.Double)){
				int type = tc.item.getType();
				if(type == c.CONTROLLER)
					ctrl.add(tc);
	
			}
		}
		return ctrl;
	}
	
	public ArrayList<TopoComponent> getClients(){
		ArrayList<TopoComponent> clts = new ArrayList<TopoComponent>();
		for(TopoComponent tc: clts){
			if(!(tc.shape instanceof Line2D.Double)){
				int type = tc.item.getType();
				if(type == c.CLIENT)
					clts.add(tc);
	
			}
		}
		return clts;
	}

	public ArrayList<TopoComponent> getLeafNodes(){
		ArrayList<TopoComponent> container = getControllers();
		container.addAll(getClients());
		return container;
	}


}
