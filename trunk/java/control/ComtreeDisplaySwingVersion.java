
/** @file .java
 *
 *  @author Jon Turner and Doowon Kim
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.util.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.awt.geom.*;

import algoLib.dataStructures.basic.*;
import algoLib.dataStructures.graphs.*;
import algoLib.misc.*;
import forest.common.*;
import forest.control.*;

/** Display comtrees based on the information received from a ComtCtl object.
 *  Connect to the ComtCtl object using a stream socket.
 *  On connecting, the ComtCtl sends the initial network configuration in
 *  the form of  NetInfo string. This is read into a local NetInfo object.
 *
 *  Then, we periodically query the ComtCtl object for updates to the
 *  the current comtree. It responds to such requests with a comtree status
 *  string which is used to update the current comtree.
 * 
 *  The current comtree can be changed using the keyboard. The keyboard
 *  can also be used to turn on/off the display of detailed link information.
 */
public class ComtreeDisplaySwingVersion extends JFrame {
    private static final int COMTCTL_PORT = 30121;
    private static final int maxNode    = 10000;
    private static final int maxLink    = 20000;
    private static final int maxRtr     =  1000;
    private static final int maxComtree = 20000;

    private static final int SIZE = 700;            ///<size of gui frame
    private static final double PEN_RADIUS = .003;  ///<default line weight

    private static InetSocketAddress ccSockAdr; ///< address for ComtCtl
    private static SocketChannel ccChan = null; ///< for ComtCtl connection

    private static int ccomt; ///< comtree to be displayed
    private static boolean linkDetail; ///< when true, show link details

    private static NetInfo net; ///< contains net toppology
    private static ComtInfo comtrees; ///< info about comtrees
    private static java.util.TreeSet<Integer> comtSet;

    private static BufferedReader kbStream; ///< for keyboard input

    //GUI
    //menu
    private JMenu jMenu;
    private JMenuBar jMenuBar;
    private JMenuItem jMenuItem;
    
    //jPanel
    private JPanel mainPanel;
    private JPanel topPanel;
    private LeftJPanel leftPanel;
    private JPanel rightPanel;
    private JPopupMenu popupMenu;

    private ArrayList<JLabel> labels;
    private JLabel currentComtreeLabel;

    public static final int MAIN_WIDTH = 700;
    public static final int MAIN_HEIGHT = 700; 
    public static final int TOP_HEIGHT = 35;
    public static final float WIDTH_RATIO = 0.8f;

    public ComtreeDisplaySwingVersion(){
        this.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        this.setTitle("ComTree Display");
        this.setPreferredSize(new Dimension(MAIN_WIDTH, MAIN_HEIGHT + TOP_HEIGHT));

        labels = new ArrayList<JLabel>();

        //menu
        jMenu = new JMenu();
        jMenuBar = new JMenuBar();
        jMenuItem = new JMenuItem();
        jMenu.setText("File");
        jMenuItem.setText("Close");
        jMenuItem.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                jMenuItemActionPerformed(evt);
            }
        });
        jMenu.add(jMenuItem);
        jMenuBar.add(jMenu);
        this.setJMenuBar(jMenuBar);

        //mainPanel
        mainPanel = new JPanel(new BorderLayout());
        mainPanel.setBorder(BorderFactory.createTitledBorder(""));

        //topPanel
        topPanel = new JPanel();
        topPanel.setBorder(BorderFactory.createTitledBorder(""));
        topPanel.setPreferredSize(new Dimension(MAIN_WIDTH, TOP_HEIGHT));
        currentComtreeLabel = new JLabel("Current Comtree: ");
        topPanel.add(currentComtreeLabel);
        mainPanel.add(topPanel, BorderLayout.PAGE_START);

        //left panel
        leftPanel = new LeftJPanel();
        leftPanel.setBorder(BorderFactory.createTitledBorder(""));
        leftPanel.setPreferredSize(new Dimension((int)(MAIN_WIDTH * WIDTH_RATIO), MAIN_HEIGHT));
        // popupMenu = new JPopupMenu(); //PopupMenu
        // popupMenu.add("Test");
        // leftPanel.setComponentPopupMenu(popupMenu);
        mainPanel.add(leftPanel, BorderLayout.LINE_START);

        //right panel
        rightPanel = new JPanel();
        rightPanel.setBorder(BorderFactory.createTitledBorder(""));
        rightPanel.setPreferredSize(new Dimension((int)(MAIN_WIDTH * (0.9 - WIDTH_RATIO)), MAIN_HEIGHT));
        rightPanel.setLayout(new BoxLayout(rightPanel, BoxLayout.Y_AXIS));
        addAJLabel("Lists", rightPanel, Color.BLACK);
        addAJLabel(" ", rightPanel, Color.BLACK);
        mainPanel.add(rightPanel, BorderLayout.LINE_END);

        this.add(mainPanel);
        this.pack();
    }

    /**
    *
    */
    private void addAJLabel(String text, Container container, Color c) {
        for(JLabel l : labels){
            if (l.getText().equals(text)){
                return;
            }
        }
        JLabel label = new JLabel(text);
        label.setAlignmentX(Component.CENTER_ALIGNMENT);
        label.setForeground(c);
        container.add(label);
        labels.add(label);
    }

    private void jMenuItemActionPerformed(ActionEvent evt) {                                           
        System.exit(0);
    }      
    
    /** Read a response to a getComtSet request.
     *  The response consists of the string "comtSet" followed by a
     *  list of comtree numbers, separated by commas and surrounded
     *  by parentheses.
     */
    public void readComtSet(PushbackReader in) {
        String word = Util.readWord(in);
        if (word == null || !word.equals("comtSet")) 
            Util.fatal("readComtSet: unexpected response " + word);
        if (!Util.verify(in,'(')) 
            Util.fatal("readComtSet: expected left paren");
        
        comtSet.clear();
        Util.MutableInt cnum = new Util.MutableInt();
        while (true) {
            if (!Util.readNum(in,cnum))
                Util.fatal("readComtSet: could not read " + "expected comtree number");
            comtSet.add(cnum.val);
            if (Util.verify(in,')')) return;
            if (!Util.verify(in,','))
                Util.fatal("readComtSet: expected comma");
        }
    }

    /** Process command line arguments.
     *  There is only one argument, which can be either the name
     *  or the IP address of the host running the comtree controller.
     *  @param args - command line arguments for ComtreeDisplay
     *  @return the socket address of the ComtCtl program or null
     *  on failure
     */
    public InetSocketAddress processArgs(String[] args) {
        InetSocketAddress sockAdr;
        if (args.length < 1) {
            System.out.println("usage: ComtreeDisplay comtCtl");
            return null;
        }
        try {
            InetAddress adr = InetAddress.getByName(args[0]);
            int port = COMTCTL_PORT;
            sockAdr = new InetSocketAddress(adr,port);
        } catch (Exception e) {
            System.out.println("usage: ComtreeDisplay comtCtl");
            System.out.println(e);
            return null;
        }
        return sockAdr;
    }

    public SocketChannel connectToComtCtl(InetSocketAddress sockAdr) {
        // Open channel to ComtCtl
        SocketChannel chan;
        try {
            chan = SocketChannel.open(sockAdr);
        } catch (Exception e) {
            System.out.println("Can't open channel to ComtCtl");
            System.out.println(e);
            return null;
        }
        return chan;
    }

    private void updateDisplay(){
        currentComtreeLabel.setText("Current ComTree: " + Integer.toString(ccomt));
        for (int c : comtSet) {
            if (c == ccomt) continue;
            addAJLabel(Integer.toString(c), rightPanel, Color.BLACK);
        }
        
        if(ccomt == 0){
            return;
        }

        // find min and max latitude and longitude
        // expressed in degrees
        double xorigin = MAIN_WIDTH * WIDTH_RATIO / 2;
        double yorigin = MAIN_HEIGHT / 2;
        double minLat  =   90; double maxLat  =  -90;
        double minLong =  180; double maxLong = -180;
        Pair<Double,Double> loc = new Pair<Double,Double>(new Double(0), new Double(0));
        for (int node = net.firstNode(); node != 0; node = net.nextNode(node)) {
            net.getNodeLocation(node,loc);
            minLat  = Math.min(minLat,  loc.first);
            maxLat  = Math.max(maxLat,  loc.first);
            minLong = Math.min(minLong, loc.second);
            maxLong = Math.max(maxLong, loc.second);
        }

        // System.out.println(maxLong + " " + minLong);
        // System.out.println(maxLat + " " + minLat);

        double xcenter = (maxLong + minLong)/2;
        double ycenter = (maxLat  + minLat)/2;
        double xscale  = (MAIN_WIDTH - 150)* WIDTH_RATIO / (maxLong - minLong);
        double yscale  = MAIN_HEIGHT  / (maxLat  - minLat);
        double scale   = Math.min(xscale,yscale);

        // System.out.println(xcenter + " " + ycenter + " " + xscale + " " + yscale + " " + scale);

        int ctx = comtrees.getComtIndex(ccomt);
        double nodeRadius = .05;

        //draw all the links
        for (int lnk = net.firstLink(); lnk != 0; lnk = net.nextLink(lnk)) {
            int left = net.getLeft(lnk);
            int right = net.getRight(lnk);
            net.getNodeLocation(left,loc);
            double lx = xorigin + (loc.second - xcenter)*scale;
            double ly = MAIN_HEIGHT - (yorigin + (loc.first - ycenter)*scale); //Swing coordinates start from the left top.
            net.getNodeLocation(right,loc);
            double rx = xorigin + (loc.second - xcenter)*scale;
            double ry = MAIN_HEIGHT - (yorigin + (loc.first - ycenter)*scale); //Swing coordinates start from the left top.

            //get LinkRate
            RateSpec rs = new RateSpec(0); 
            net.getLinkRates(lnk,rs);

            // System.out.println(lx + " " + ly + " " + rx + " " + ry);
            boolean strong = false;
            if (comtrees.isComtLink(ctx,lnk)){
                strong = true;
            }

            leftPanel.addLine(new Line((int)lx, (int)ly, (int)rx, (int)ry, strong, rs));
        }

        // draw all the nodes in the net
        for (int node = net.firstNode(); node != 0; node = net.nextNode(node)) {
            net.getNodeLocation(node,loc);
            double x = xorigin + (loc.second - xcenter)*scale;
            double y = MAIN_HEIGHT - (yorigin + (loc.first - ycenter)*scale); //Swing coordinates start from the left top.

            int lnkCnt = 0;
            Color nodeColor = Color.LIGHT_GRAY;
            int nodeAdr = net.getNodeAdr(node);
            if (comtrees.isComtNode(ctx,nodeAdr)) {
                if (net.isRouter(node))
                    lnkCnt=comtrees.getLinkCnt(ctx,nodeAdr);
                nodeColor = Color.WHITE;
                if (nodeAdr == comtrees.getRoot(ctx))
                    nodeColor = Color.YELLOW;
            }
            if (net.isRouter(node)) {
                leftPanel.addCircle(new Circle((int)x - 25 , (int)y - 25, 50, 50, 
                    nodeColor, net.getNodeName(node), lnkCnt));
            } else {
                leftPanel.addRect(new Rect((int)x - 25 , (int)y - 25, 50, 50, 
                    nodeColor, net.getNodeName(node), Forest.fAdr2string(net.getNodeAdr(node))));
            }
        }

        //refreshing GUI
        validate();
        repaint();
    }

    /** Attempt to read a line of input from System.in.
    *  Return the line or "" if there is no input available to read.
    */
    public String readKeyboardIn() {
        String lineBuf = "";
        try {
            if (kbStream == null) {
                kbStream = new BufferedReader(
                    new InputStreamReader(System.in));
                System.out.print("ComtreeDisplay: ");
            } else if (kbStream.ready()) {
                lineBuf = kbStream.readLine();
                System.out.print("ComtreeDisplay: ");
            }
        } catch(Exception e) {
            System.out.println("input error: " + e);
            return null;
        }
        return lineBuf;
    }

    public boolean processKeyboardIn(String lineBuf) {
        Scanner line = new Scanner(lineBuf);
        if (line.hasNextInt()) {
            ccomt = line.nextInt();
        } else if (line.hasNext("linkDetail")) {
            linkDetail = true;
        } else if (line.hasNext("noLinkDetail")) {
            linkDetail = false;
        } else if (line.hasNext("quit")) {
            System.exit(0);
        } else return false;
        
        return true;
    }

    public LeftJPanel getLeftPanel(){
        return leftPanel;
    }

    public static void main(String args[]){
        ComtreeDisplaySwingVersion  comtreeDisplay= new ComtreeDisplaySwingVersion(); //GUI
        comtreeDisplay.setVisible(true);

        net = new NetInfo(maxNode, maxLink, maxRtr);
        comtrees = new ComtInfo(maxComtree, net);

        // process arguments and setup channel to ComtreeCtl
        InetSocketAddress ccSockAdr = comtreeDisplay.processArgs(args);
        if (ccSockAdr == null)
            Util.fatal("can't build socket address");
        SocketChannel ccChan = comtreeDisplay.connectToComtCtl(ccSockAdr);
        if (ccChan == null) 
            Util.fatal("can't connect to ComtCtl");

        // setup output stream to ComtCtl
        OutputStream out = null;
        try {
            out = ccChan.socket().getOutputStream();
        } catch(Exception e) {
            Util.fatal("can't open output stream to ComtCtl " + e);
        }

        // setup input stream and get network topology
        InputStream in = null;
        PushbackReader inRdr = null;
        String s;
        comtSet = new java.util.TreeSet<Integer>();
        try {
            s = "getNet\n";
            out.write(s.getBytes());
            in = ccChan.socket().getInputStream();
            inRdr = new PushbackReader(new InputStreamReader(in));
            if (!comtreeDisplay.net.read(inRdr))
                Util.fatal("can't read network topology\n");
            s = "getComtSet\n";
            out.write(s.getBytes());
            comtreeDisplay.readComtSet(inRdr);
        } catch(Exception e) {
            Util.fatal("can't read network topology/comtrees\n" + e);
        }

        // initial display (no comtrees yet)
        linkDetail = false;

        comtreeDisplay.updateDisplay();

        // main loop
        ccomt = 0;
        int prev_ccomt = ccomt;
        while (true) {
            String lineBuf = comtreeDisplay.readKeyboardIn();
            if (lineBuf.length() > 0) comtreeDisplay.processKeyboardIn(lineBuf);
            if (prev_ccomt == ccomt) {
                continue;
            } else{
                prev_ccomt = ccomt;
            }
            if (!comtSet.contains(ccomt)) continue;
            try {
                s = "getComtSet\n";
                out.write(s.getBytes());
                comtreeDisplay.readComtSet(inRdr);
                s = "getComtree " + ccomt + "\n";
                out.write(s.getBytes());
                    // now read updated list and comtree status
                int recvdComt = comtrees.readComtStatus(inRdr);
                if (recvdComt != ccomt) {
                    System.out.println("recvdComt=" + recvdComt);
                    Util.fatal("received comtree info does "
                       + "not match request");
                }
            } catch(Exception e) {
                Util.fatal("can't update status " + e);
            }
            comtreeDisplay.getLeftPanel().clearLines();
            comtreeDisplay.getLeftPanel().clearCircles();
            comtreeDisplay.getLeftPanel().clearRects();
            comtreeDisplay.updateDisplay();
        }
    }
}

/**
 * ComTree Display Panel Class
 */
class LeftJPanel extends JPanel{
    public static final int MAXINUM_ZOOM_LEVEL = 3;
    public static final int MININUM_ZOOM_LEVEL = -5;
    public static final double  ZOOM_MULTIPLICATION_FACTOR = 1.2;

    ArrayList<Line> lines = new ArrayList<Line>();
    ArrayList<Circle> circles = new ArrayList<Circle>();
    ArrayList<Rect> rects = new ArrayList<Rect>();
    final JWindow popup = new JWindow();
    final JLabel popupLabel = new JLabel();

    AffineTransform tx = new AffineTransform();
    int zoomLevel = 0;
    boolean zoomEnabled = false;
    Point startDragPointScreen, endDragPointScreen;
    Point2D.Float startDragPoint = new Point2D.Float();
    Point2D.Float endDragPoint = new Point2D.Float();

    public LeftJPanel(){
        setOpaque(true);
        setDoubleBuffered(true);

        popup.setPreferredSize(new Dimension(200,30));
        popup.setBackground(Color.WHITE);
        popup.getContentPane().add(popupLabel);
        popup.pack();
        popup.setFocusable(false);
        popup.setVisible(true);
        
        addMouseListener(new MouseAdapter(){
            @Override
            public void mouseClicked(MouseEvent e) {
                try {
                    Point p = e.getPoint();
                    Point2D p2 = tx.inverseTransform(p,null);
                    //System.out.println(p + " " + p2);
                    for(Line l : lines){
                        //get a dist
                        double dist = Line2D.ptSegDist(l.getLx(), l.getLy(), l.getRx(), l.getRy(), p2.getX(), p2.getY());
                        if(dist >= -1.5 && dist <= 1.5){
                            if(popup != null){
                                popupLabel.setText(l.getRateSpec().toString());
                                popup.setVisible(true);
                                popup.setLocation((int)(p.getX() + 1), (int)(p.getY() + 2 * ComtreeDisplaySwingVersion.TOP_HEIGHT));
                                popup.toFront();
                            }
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
}

class Line{
    private int lx, ly, rx, ry = 0;
    private boolean strong = false;
    private RateSpec rs;
    public Line(int lx, int ly, int rx, int ry, boolean strong, RateSpec rs){
        this.lx = lx; this.ly = ly;
        this.rx = rx; this.ry = ry;
        this.strong = strong;
        this.rs = rs;
    }
    public int getLx(){ return lx;}
    public int getLy(){ return ly;}
    public int getRx(){ return rx;}
    public int getRy(){ return ry;}
    public boolean isStrong(){return strong;}
    public RateSpec getRateSpec(){return rs;}
}

class Rect{
    private int x, y, width, height = 0;
    private String name;
    private String nodeAddr;
    private Color color;
    public Rect(int x, int y, int width, int height, Color color, String name, String nodeAddr){
        this.x = x; this.y = y; 
        this.width = width; this.height = height;
        this.color = color;
        this.name = name; this.nodeAddr = nodeAddr;
    }
    public int getX(){      return x;}
    public int getY(){      return y;}
    public int getWidth(){  return width;}
    public int getHeight(){ return height;}
    public Color getColor(){return color;}
    public String getName(){return name;}
    public String getNodeAddr(){return nodeAddr;}
}

class Circle{
    private int x, y, width, height = 0;
    private String name;
    private int linkCnt;
    private Color color;
    public Circle(int x, int y, int width, int height, Color color, String name, int linkCnt){
        this.x = x; this.y = y; 
        this.width = width; this.height = height;
        this.color = color;
        this.name = name; 
        this.linkCnt = linkCnt;
    }
    public int getX(){      return x;}
    public int getY(){      return y;}
    public int getWidth(){  return width;}
    public int getHeight(){ return height;}
    public Color getColor(){return color;}
    public String getName(){return name;}
    public int getLinkCnt(){return linkCnt;}
}