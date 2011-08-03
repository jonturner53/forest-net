import java.util.*;
import java.awt.*;
import java.io.*;
import java.awt.geom.*;
import java.awt.event.*;
import javax.swing.*;

public class Panel extends JPanel{
	private static final int SIZE = 800;
	private static final double FACTOR = 1;
	private double radius = SIZE/2;
	private double width = 100;
	private double height = 100;
	private Point2D origin;
	private Rectangle2D.Double selection;
	private boolean selecting, moved, dragging;
	private Canvas canvas;
	private JMenuBar mb;
	private JMenu file, add;
	private JMenuItem open, save, close, root, core, node, cltmgr, cc, mon, netmgr;
	private JFileChooser fc;
	private File store;
	private topoComponent min, current;
	private Queue<ActionEvent> paintQ;
	private LinkedList<topoComponent> components, selected;
	private Color c = new Color(210,230,255);

	public Panel(){
		super(new BorderLayout());
		setSize(new Dimension(SIZE, SIZE));
		setPreferredSize(new Dimension(SIZE, SIZE));
		paintQ = new LinkedList<ActionEvent>();
		min = null;
		current = null;
		dragging = false;
		components = new LinkedList<topoComponent>();
		selected = new LinkedList<topoComponent>();
		fc = new JFileChooser();
		mb = new JMenuBar();
		file = new JMenu("File");
		mb.add(file);
		open = new JMenuItem("Open");
		open.addActionListener(new MenuListener());
		file.add(open);
		save = new JMenuItem("Save");
		save.addActionListener(new MenuListener());	
		file.add(save);
		close = new JMenuItem("Close");
		close.addActionListener(new MenuListener());
		file.add(close);
		add = new JMenu("Add");
		mb.add(add);
		JMenu submenu = new JMenu("Add Node");
		root = new JMenuItem("new Root");
		root.addActionListener(new MenuListener());
		submenu.add(root);
		core = new JMenuItem("new Core");
		core.addActionListener(new MenuListener());
		submenu.add(core);
		node = new JMenuItem("new Node");
		node.addActionListener(new MenuListener());
		submenu.add(node);
		add.add(submenu);
		cltmgr = new JMenuItem("new Client Manager");
		cltmgr.addActionListener(new MenuListener());
		add.add(cltmgr);
		cc = new JMenuItem("new ComtreeController");
		cc.addActionListener(new MenuListener());
		add.add(cc);
		mon = new JMenuItem("new Monitor");
		mon.addActionListener(new MenuListener());
		add.add(mon);
		netmgr = new JMenuItem("new NetManager");
		netmgr.addActionListener(new MenuListener());
		add.add(netmgr);

		super.setBackground(c);
		add(mb, BorderLayout.PAGE_START);
		
		canvas = new Canvas();
		add(canvas, BorderLayout.CENTER);
		add(mb, BorderLayout.PAGE_START);

		ClickListener c = new ClickListener();
		addMouseListener(c);
		addMouseMotionListener(c);
		
		setVisible(true);
	}
	
	private class Canvas extends JLabel{
		private int numItems = 1;
		public Canvas(){
			setPreferredSize(new Dimension(SIZE, SIZE));
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
			super.setBackground(c);
			super.paint(g);
			Graphics2D g2 = ((Graphics2D) g);
			//int radius = ((int) Math.sqrt(SIZE/(paintQ.size()*FACTOR*Math.PI)));
			Ellipse2D.Double circle = new Ellipse2D.Double(radius, radius, width, height);
			circle.setFrame(radius, radius, width, height);
			Rectangle2D.Double square = new Rectangle2D.Double(radius, radius, width, height);
			square.setFrame(radius, radius, width, height);
			while(!paintQ.isEmpty()){
				ActionEvent ae  = paintQ.poll();
				JMenuItem src = (JMenuItem) ae.getSource();
				String name = "";
				if(src.equals(root) || src.equals(core) || src.equals(node)){
					name = "Router" + Integer.toString(numItems++);
					components.add(new topoComponent(name, src, circle));
				}else if(src.equals(cltmgr) || src.equals(mon) || src.equals(cc) || src.equals(netmgr))
					components.add(new topoComponent(name, src, square));
			}
		
			Iterator<topoComponent> itr = components.iterator();
			while(itr.hasNext()){
				topoComponent tc = itr.next();
				Iterator<topoComponent> sec = selected.iterator();
				while(sec.hasNext()){
					topoComponent n = sec.next();
					if(tc.equals(n))
						tc = n;
				}
				JMenuItem jmi = tc.item;
				Shape s = tc.shape;
				Rectangle2D.Double r = ((Rectangle2D.Double) s.getBounds2D());
				if(jmi.equals(root)){
					g.setColor(Color.GRAY);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(jmi.equals(core)){
					g.setColor(Color.GRAY);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(jmi.equals(node)){
					g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);	
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(jmi.equals(cltmgr)){
					g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					tc.name = "CltMgr";
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(jmi.equals(cc)){
					g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);	
					tc.name = "CC";
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(jmi.equals(mon)){
					g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					tc.name = "Mon";
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
				else if(jmi.equals(netmgr)){
					g.setColor(Color.WHITE);
					g2.fill(s);
					g.setColor(Color.BLACK);
					g2.draw(s);
					tc.name = "NetMgr";
					g2.drawString(tc.name, ((int) r.getX()) + ((int) r.getWidth())/3, ((int) r.getY())+((int) r.getHeight())/2);
				}
			}
			if(selection != null){
				g2.setColor(Color.BLACK);
				g2.draw(selection);
			}

		}   
	}

	private class MenuListener implements ActionListener, ItemListener {
		 public void actionPerformed(ActionEvent e) {
			if(e.getSource().equals(open)){
				System.out.println("Opening..");
				int returnVal = fc.showOpenDialog(Panel.this.getParent());
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					store = fc.getSelectedFile();
				}
			}
			else if(e.getSource().equals(save)){
				System.out.println("Saving..");
				int returnVal = fc.showSaveDialog(Panel.this.getParent());
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					
				}
			}else if(e.getSource().equals(close)){

			}else{
				paintQ.offer(e);
				repaint();
		 	}
		 }
		 public void itemStateChanged(ItemEvent e) {}
	}
	
	private class ClickListener implements MouseListener, MouseMotionListener{
		public void mousePressed(MouseEvent e) {
			topoComponent c = inComponents(e);
			if(c != null){
				dragging = true;
				current = c;
			}
			origin = e.getPoint();
		}
		public void mouseDragged(MouseEvent e){
			if(dragging && !selecting){
				if(selected.isEmpty())
					selected.add(current);
				//System.out.println("numSelected: " + selected.size());
				topoComponent close = min(e.getPoint());
				Rectangle2D bnds = close.shape.getBounds2D();
				Point2D.Double loc = close.point;
				double y_offset = bnds.getHeight()/2;
				double x_offset = bnds.getWidth()/2;
				loc.setLocation((int)(e.getX()-x_offset), (int)(e.getY()- y_offset));
				if(close.shape instanceof Ellipse2D.Double)
					close.shape = new Ellipse2D.Double(loc.getX(), loc.getY(), width, height);
				else if(close.shape instanceof Rectangle2D.Double)
					close.shape = new Rectangle2D.Double(loc.getX(), loc.getY(), width, height);

				Iterator<topoComponent> itr = selected.iterator();
				while(itr.hasNext()){
					topoComponent tc = itr.next();
					if(!tc.equals(close)){
						bnds = tc.shape.getBounds2D();
						y_offset = bnds.getHeight()/2;
						x_offset = bnds.getWidth()/2;
						double d = close.point.distance(tc.point);
						System.out.println("dist: " + d);
						if(tc.shape instanceof Ellipse2D.Double)
							tc.shape = new Ellipse2D.Double(tc.point.getX()-d, tc.point.getY()-d, width, height);
						else if(tc.shape instanceof Rectangle2D.Double)
							tc.shape = new Rectangle2D.Double(tc.point.getX()-d, tc.point.getY()-d, width, height);
					}
				}
				moved = true;
			}
			else{ //create selection box or update size
				selecting = true;
				moved = false;
				if(selection == null)
					selection = new Rectangle2D.Double();
				//System.out.println("mouse: " + e.getPoint());
				Rectangle2D bnds = selection.getBounds2D();
				double y_offset = bnds.getHeight()/2;
				double x_offset = bnds.getWidth()/2;
				Point2D.Double loc = new Point2D.Double(((Rectangle2D.Double) bnds).getX(), ((Rectangle2D.Double) bnds).getY());
				loc.setLocation((int)(e.getX()-x_offset), (int)(e.getY()-y_offset));
				double x = origin.getX();
				double y = origin.getY()-mb.getBounds().getHeight();
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
			dragging = false;
			current = null;
			if(selection != null){
				Iterator<topoComponent> itr = components.iterator();
				while(itr.hasNext()){
					topoComponent tc= itr.next();
					if(selection.intersects(tc.shape.getBounds2D()))
						selected.add(tc);
				}
			}
			if(moved){
				selected.clear();
				moved = false;
			}
			selection = null;
			selecting = false;
			repaint();
		}
		public void mouseEntered(MouseEvent e){}
		public void mouseExited(MouseEvent e){}
		public void mouseClicked(MouseEvent e){}
	}
	
	private topoComponent inComponents(MouseEvent e){
		Iterator<topoComponent> itr = components.iterator();
		while(itr.hasNext()){
			topoComponent ct = itr.next();
			if(ct.shape.contains(e.getPoint()))
				return ct;
		}
		return null;
	}
	
	private topoComponent min(Point mouse){
		if(selected == null)
			return null;
		else if(selected.isEmpty())
			return null;
		else if(selected.size() == 1)
			return selected.getFirst();
		else{
			min = selected.getFirst();
			Iterator<topoComponent> itr = selected.iterator();
			while(itr.hasNext()){
				topoComponent next = itr.next();	
				if(next.point.distance(mouse) < min.point.distance(mouse))
					min = next;
			}
		}
		return min;
	}

	private boolean onScreen(Point p){
		return getBounds().contains(p);
	}

	private void unpack(File f){
	
	}
}
