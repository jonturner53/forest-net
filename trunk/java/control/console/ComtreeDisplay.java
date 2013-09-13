package forest.control.console;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseWheelEvent;
import java.awt.geom.AffineTransform;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Line2D;
import java.awt.geom.NoninvertibleTransformException;
import java.awt.geom.Point2D;
import java.awt.geom.Rectangle2D;
import java.util.ArrayList;

import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;
import javax.swing.Timer;

import forest.common.Forest;
import forest.common.Pair;
import forest.common.RateSpec;
import forest.control.ComtInfo;
import forest.control.NetInfo;
import forest.control.console.model.Circle;
import forest.control.console.model.Line;
import forest.control.console.model.Rect;

public class ComtreeDisplay extends JPanel{
	public static final int MAXINUM_ZOOM_LEVEL = 3;
	public static final int MININUM_ZOOM_LEVEL = -5;
	public static final double  ZOOM_MULTIPLICATION_FACTOR = 1.2;

	private ArrayList<Line> lines = new ArrayList<Line>();
	private ArrayList<Circle> circles = new ArrayList<Circle>();
	private ArrayList<Rect> rects = new ArrayList<Rect>();
	
	private JPopupMenu popupMenu;

	private AffineTransform tx = new AffineTransform();
	private int zoomLevel = 0;
	private boolean zoomEnabled = false;
	private Point startDragPointScreen, endDragPointScreen;
	private Point2D.Float startDragPoint = new Point2D.Float();
	private Point2D.Float endDragPoint = new Point2D.Float();
	
	private ConnectionComtCtl connectionComtCtl;
	
	private Timer timer;

	public ComtreeDisplay(ConnectionComtCtl connectionComtCtl){
		this.connectionComtCtl = connectionComtCtl;
		
		setOpaque(true);
		setDoubleBuffered(true);

		popupMenu = new JPopupMenu();
//		this.setComponentPopupMenu(popupMenu);
		this.setVisible(true);
		addMouseListener(new MouseAdapter(){
			@Override
			public void mouseClicked(MouseEvent e) {
				try {
					Point p = e.getPoint();
					Point2D p2 = tx.inverseTransform(p,null);
					//System.out.println(p + " " + p2);
					for(Line l : lines){
						//get a dist
						double dist = Line2D.ptSegDist(l.getLx(), l.getLy(), l.getRx(), 
								l.getRy(), p2.getX(), p2.getY());
						if(dist >= -1.5 && dist <= 1.5){
//							if(e.isPopupTrigger())
							doPop(e, l.getRateSpec().toString(), l.getComtreeRs().toString());
						}
					}
				} catch (NoninvertibleTransformException ex){
					ex.printStackTrace();
				}
			}
			@Override
			public void mousePressed(MouseEvent e){
				zoomEnabled = true;
				startDragPointScreen = e.getPoint();
				endDragPointScreen = null;
			}
		    
			private void doPop(MouseEvent e, String s, String s2){
		        PopUp menu = new PopUp(s, s2);
		        menu.show(e.getComponent(), e.getX(), e.getY());
		    }
		});

		addMouseMotionListener(new MouseAdapter(){
            @Override
            public void mouseDragged(MouseEvent e){
                endDragPointScreen = e.getPoint();
                try {
                    tx.inverseTransform(startDragPointScreen, startDragPoint);
                    tx.inverseTransform(endDragPointScreen, endDragPoint);

                    double dx = endDragPoint.getX() - startDragPoint.getX();
                    double dy = endDragPoint.getY() - startDragPoint.getY();
                    tx.translate(dx, dy);

                    startDragPointScreen = endDragPointScreen;
                    endDragPointScreen = null;
                    repaint();
                } catch (NoninvertibleTransformException ex) {
                    ex.printStackTrace();
                }
            }
        });
		
		addMouseWheelListener(new MouseAdapter(){
			@Override
			public void mouseWheelMoved(MouseWheelEvent e){
				if(e.getScrollType() == MouseWheelEvent.WHEEL_UNIT_SCROLL){
					zoomEnabled = true;
					int wheelRotation = e.getWheelRotation();
					Point p = e.getPoint();
					try {
						Point2D p1 = null, p2 = null;
						p1 = tx.inverseTransform(p, p1);
						p2 = tx.inverseTransform(p, p2);
						if (wheelRotation > 0) {
							if (zoomLevel < MAXINUM_ZOOM_LEVEL) {
								zoomLevel++;
								tx.scale(1 / ZOOM_MULTIPLICATION_FACTOR, 1 / ZOOM_MULTIPLICATION_FACTOR);
								tx.translate(p2.getX() - p1.getX(), p2.getY() - p1.getY());
							}
						} else {
							if (zoomLevel > MININUM_ZOOM_LEVEL) {
								zoomLevel--;
								tx.scale(ZOOM_MULTIPLICATION_FACTOR, ZOOM_MULTIPLICATION_FACTOR);
								tx.translate(p2.getX() - p1.getX(), p2.getY() - p1.getY());
							}
						}
						repaint();
					} catch (NoninvertibleTransformException ex) {
						ex.printStackTrace();
					}
				}
			}
		});
	}

	@Override
	public void paintComponent(Graphics g) {
		super.paintComponent(g);
		Graphics2D g2D = (Graphics2D) g;
		FontMetrics fm = g2D.getFontMetrics(); //for font size
		if(zoomEnabled)
			g2D.setTransform(tx);

		for(Line l : lines){
			if(l.isStrong()){
				g2D.setStroke(new BasicStroke(3));
			} else {
				g2D.setStroke(new BasicStroke(1));
			}
			g2D.drawLine(l.getLx(), l.getLy(), l.getRx(), l.getRy());
		}

		for(Circle c : circles){
			g2D.setColor(Color.BLACK);
			Ellipse2D e = new Ellipse2D.Double(c.getX(), c.getY(), c.getWidth(), c.getHeight());
			g2D.setStroke(new BasicStroke(3));
			g2D.draw(e);
			g2D.setColor(c.getColor());
			g2D.fill(e);

			Rectangle2D r2 = fm.getStringBounds(c.getName(), g2D);
			g2D.setColor(Color.BLACK);
			int x = (int)(c.getX() + c.getWidth()/2 - r2.getWidth()/2);
			int y = (int)(c.getY() + c.getHeight()/2 + r2.getHeight()/4);
			g2D.drawString(c.getName(), x, y);
			g2D.drawString(Integer.toString(c.getLinkCnt()), x, (int)(y+r2.getHeight()) ); 
		}

		for(Rect r : rects){
			g2D.setStroke(new BasicStroke(2));
			Rectangle2D rec = new Rectangle2D.Double(r.getX(), r.getY(), r.getWidth(), r.getHeight());
			g2D.draw(rec);
			g2D.setColor(r.getColor());
			g2D.fill(rec);

			g2D.setColor(Color.BLACK);
			Rectangle2D r2 = fm.getStringBounds(r.getName(), g2D);
			int x = (int)(r.getX() + r.getWidth()/2 - r2.getWidth()/2);
			int y = (int)(r.getY() + r.getHeight()/2 + r2.getHeight()/4);
			g2D.drawString(r.getName(), x, y);
			r2 = fm.getStringBounds(r.getNodeAddr(), g2D);
			x = (int)(r.getX() + r.getWidth()/2 - r2.getWidth()/2);
			y = (int)(r.getY() + r.getHeight()/2 + r2.getHeight()/2 + r2.getHeight());
			g2D.drawString(r.getNodeAddr(), x, y);
		}
	}
	/**
	 * retrieve netinfo and comtree, and populate table models to update display
	 * @param ccomt
	 * @param netInfo
	 * @param comtrees
	 */
	public void updateDisplay(int ccomt, NetInfo netInfo, ComtInfo comtrees){
        // find min and max latitude and longitude
        // expressed in degrees
	    int MAIN_WIDTH = 700;
	    int MAIN_HEIGHT = 700; 
	    float WIDTH_RATIO = 0.8f;
	    
        double xorigin = MAIN_WIDTH * WIDTH_RATIO / 2;
        double yorigin = MAIN_HEIGHT / 2;
        double minLat  =   90; double maxLat  =  -90;
        double minLong =  180; double maxLong = -180;
        Pair<Double,Double> loc = new Pair<Double,Double>(new Double(0), new Double(0));
        for (int node = netInfo.firstNode(); node != 0; node = netInfo.nextNode(node)) {
        	netInfo.getNodeLocation(node,loc);
            minLat  = Math.min(minLat,  loc.first);
            maxLat  = Math.max(maxLat,  loc.first);
            minLong = Math.min(minLong, loc.second);
            maxLong = Math.max(maxLong, loc.second);
        }

        double xcenter = (maxLong + minLong)/2;
        double ycenter = (maxLat  + minLat)/2;
        double xscale  = (MAIN_WIDTH - 150)* WIDTH_RATIO / (maxLong - minLong);
        double yscale  = MAIN_HEIGHT  / (maxLat  - minLat);
        double scale   = Math.min(xscale,yscale);


        int ctx = comtrees.getComtIndex(ccomt);

        double nodeRadius = .05;

        //draw all the links
        for (int lnk = netInfo.firstLink(); lnk != 0; lnk = netInfo.nextLink(lnk)) {
            int left = netInfo.getLeft(lnk);
            int right = netInfo.getRight(lnk);
            netInfo.getNodeLocation(left,loc);
            double lx = xorigin + (loc.second - xcenter)*scale;
            //Swing coordinates start from the left top.
            double ly = MAIN_HEIGHT - (yorigin + (loc.first - ycenter)*scale); 
            netInfo.getNodeLocation(right,loc);
            double rx = xorigin + (loc.second - xcenter)*scale;
            //Swing coordinates start from the left top.
            double ry = MAIN_HEIGHT - (yorigin + (loc.first - ycenter)*scale);

            //get LinkRate
            RateSpec rs = new RateSpec(0); 
            RateSpec availableRs = new RateSpec();
            RateSpec comtreeRs = new RateSpec();
            netInfo.getLinkRates(lnk,rs);
            netInfo.getAvailRates(lnk, availableRs);
            
       
            boolean strong = false;
            if (comtrees.isComtLink(ctx,lnk)){
                strong = true;
                //comtree rate
                int child = -1;
                int leftNode = netInfo.getLeft(lnk);
                int lNodeAddr = netInfo.getNodeAdr(leftNode);
                int pLink = comtrees.getPlink(ctx, lNodeAddr);
                if( pLink == lnk){
                	child = netInfo.getLeft(lnk);
                }
                else{
                	child = netInfo.getRight(lnk);
                }
               
                int nodeAddr = netInfo.getNodeAdr(child);
                comtrees.getLinkRates(ctx, nodeAddr, comtreeRs);
            }
            
            addLine(new Line((int)lx, (int)ly, (int)rx, (int)ry, 
            					strong, rs, availableRs, comtreeRs));
        }

        // draw all the nodes in the net
        for (int node = netInfo.firstNode(); node != 0; node = netInfo.nextNode(node)) {
        	netInfo.getNodeLocation(node,loc);
            double x = xorigin + (loc.second - xcenter)*scale;
            //Swing coordinates start from the left top.
            double y = MAIN_HEIGHT - (yorigin + (loc.first - ycenter)*scale); 

            int lnkCnt = 0;
            Color nodeColor = Color.LIGHT_GRAY;
            int nodeAdr = netInfo.getNodeAdr(node);
            if (comtrees.isComtNode(ctx,nodeAdr)) {
                if (netInfo.isRouter(node))
                    lnkCnt=comtrees.getLinkCnt(ctx,nodeAdr);
                nodeColor = Color.WHITE;
                if (nodeAdr == comtrees.getRoot(ctx))
                    nodeColor = Color.YELLOW;
            }
            if (netInfo.isRouter(node)) {
            	addCircle(new Circle((int)x - 25 , (int)y - 25, 50, 50, 
            						nodeColor, netInfo.getNodeName(node), lnkCnt));
            } else {
            	addRect(new Rect((int)x - 25 , (int)y - 25, 50, 50, 
            						nodeColor, netInfo.getNodeName(node), 
            						Forest.fAdr2string(netInfo.getNodeAdr(node))));
            }
        }
        //refreshing GUI
        validate();
        repaint();
	}
	
	public void clearLines(){
		lines.clear();
	}
	public void addLine(Line line){
		lines.add(line);
	}
	public void clearCircles(){
		circles.clear();
	}
	public void addCircle(Circle circle){
		circles.add(circle);
	}
	public void clearRects(){
		rects.clear();
	}
	public void addRect(Rect rect){
		rects.add(rect);
	}

	/**
	 * A thread to refresh display
	 * @param ccomt
	 */
	public void autoUpdateDisplay(final int ccomt){
		timer = new Timer(2000, new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				NetInfo netInfo = connectionComtCtl.getNetInfo();
				ComtInfo comtrees = connectionComtCtl.getComtrees();
				String s = connectionComtCtl.getComtree(ccomt);
				updateDisplay(ccomt, netInfo, comtrees);
				
				if(!connectionComtCtl.isAutoRefresh()) {
					timer.stop();
				}
			}    
		});
		timer.start();
	}
	
	/**
	 * Clean Comtree Display 
	 */
	public void clearUI(){
		if(timer != null)
			timer.stop();
		clearLines();
		clearCircles();
		clearRects();
		updateUI();
	}
}

/**
 * Popup class to display rates on comtree
 * @author Doowon Kim
 *
 */
class PopUp extends JPopupMenu{
	JMenuItem item;
	JMenuItem item2;
	public PopUp(String s, String s2){
		item = new JMenuItem("Link Rate: " + s);
		item2 = new JMenuItem("Comtree Rate: " + s2);
		add(item);
		add(item2);
	}
}
