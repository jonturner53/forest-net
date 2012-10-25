package forest.control;

/** @file Console.java
*
 *  @author Jon Turner
 *  @date 2012
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.awt.*;
import javax.swing.*;
import java.util.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.awt.Color;
import java.awt.geom.Point2D;

import algoLib.dataStructures.basic.*;
import algoLib.dataStructures.graphs.*;
import algoLib.misc.*;
import forest.common.*;
import forest.control.*;

/** This class implements a simple network management console for
 *  a Forest network.
 */
public class Console extends MouseAdapter implements ActionListener {
	private static final int maxNode    = 10000;
	private static final int maxLink    = 20000;
	private static final int maxRtr     =  1000;
        private static final int maxComtree = 20000;

	private static final int SIZE = 700;        	///<size of gui frame

	private static InetSocketAddress nmSockAdr; ///< address for NetMgr
	private static SocketChannel nmChan = null; ///< for NetMgr connection

	private static NetInfo net;	///< contains net toppology and
					///< comtree info
	
	/** Main method launches user interface */
	public static void main(String[] args) {	
		SwingUtilities.invokeLater(new Runnable() {
			public void run() { new EditMap(); }
		});
	}

	JTextField inFileText, outFileText;
	EMapPanel panel;

	/** Constructor for Console class.
	 *  Setup the various user interface components.
	 */
	Console() {
		JFrame jfrm = new JFrame("Forest Console");
		jfrm.getContentPane().setLayout(new FlowLayout());
		jfrm.setSize(650,720);
		jfrm.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		// text field and button for opening input file
		nmIpText = new JTextField(10);
		nmIpText.addActionListener(this);
		nmIpText.setActionCommand("nmIp");

		// text field for output file and save button
		nmPortText = new JTextField(10);
		nmPortText.addActionListener(this);
		nmPortText.setActionCommand("nmPort");

		// build box containing file controls
		Box b1 = Box.createVerticalBox();
		b1.add(nmIpText); b1.add(nmPortText);

		JButton connectBtn = new JButton("connect");
		connectBtn.addActionListener(this);

		Box b2 = Box.createHorizontalBox();
		b2.add(b1); b2.add(connectBtn);

		// group of buttons for zooming
		JButton zoomInBtn = new JButton("zoom in");
		JButton zoomOutBtn = new JButton("zoom out");
		zoomInBtn.addActionListener(this);
		zoomOutBtn.addActionListener(this);

		Box b3 = Box.createVerticalBox();
		b3.add(zoomInBtn); b3.add(zoomOutBtn);

		// group of buttons for panning
		JButton leftBtn = new JButton("left");
		JButton rightBtn = new JButton("right");
		leftBtn.addActionListener(this);
		rightBtn.addActionListener(this);

		Box b4 = Box.createVerticalBox();
		b4.add(leftBtn); b4.add(rightBtn);

		JButton upBtn = new JButton("up");
		JButton downBtn = new JButton("down");
		upBtn.addActionListener(this);
		downBtn.addActionListener(this);

		Box b5 = Box.createVerticalBox();
		b5.add(upBtn); b5.add(downBtn);

		Box topBar = Box.createHorizontalBox();
		topBar.add(b2); topBar.add(b3);
		topBar.add(b4); topBar.add(b5);

		jfrm.add(topBar);

		panel = new EMapPanel(600);
		panel.addMouseListener(this);
		jfrm.add(panel);

		jfrm.setVisible(true);
	}

	/** Respond to user interface events.
	 *  ae is an action event relating to one of the user interface
	 *  elements.
	 */
	public void actionPerformed(ActionEvent ae) {
		// TODO - rework actions
		if (ae.getActionCommand().equals("left")) {
			if (cornerX > 0) {
				cornerX--; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("right")) {
			if (cornerX + viewSize < worldSize) {
				cornerX++; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("down")) {
			if (cornerY > 0) {
				cornerY--; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("up")) {
			if (cornerY + viewSize < worldSize) {
				cornerY++; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("zoom in")) {
			if (viewSize > 0) {
				viewSize--; panel.setViewSize(viewSize);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("zoom out")) {
			if (cornerX + viewSize < worldSize &&
			    cornerY + viewSize < worldSize) {
				viewSize++; panel.setViewSize(viewSize);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("connectBtn")) {
			// connect to NetMgr
		}
	}

	/** Respond to mouse click in the network map.
	 *  @param e is a mouse event corresponding to a click in the panel.
	 */
	public void mouseClicked(MouseEvent e) {
		// When mouse is pressed down, for some map object at that
		// location and if one is found, display the menu
	}

	/** Read an input file.
	 *  @param fileName is the name of the input file to be read
	 */
	private void readFile(String fileName) {
		try {
			Scanner imap = new Scanner(new File(fileName));
			int y = 0; walls = null;
			while (imap.hasNextLine() && y >= 0) {
				String temp = imap.nextLine();
				if (walls == null) {
					worldSize = temp.length();
					y = worldSize-1;
					viewSize = Math.min(10,worldSize);
					walls = new int[worldSize*worldSize];
				} else if (temp.length() != worldSize) {
					System.out.println("Map file has "
						 + "unequal line lengths");
					System.exit(1);
				}
				for(int x = 0; x < worldSize; x++) {
					if(temp.charAt(x) == '+') {
						walls[y * worldSize + x] = 3;
					} else if(temp.charAt(x) == '-') {
						walls[y * worldSize + x] = 2;
					} else if(temp.charAt(x) == '|') {
						walls[y * worldSize + x] = 1;
					} else if(temp.charAt(x) == ' ') {
						walls[y * worldSize + x] = 0;
					} else {
						System.out.println(
						  "Unrecognized symbol in map "
					  	  + "file!");
					}
				}
				y--;
			}
			if (y >= 0) {
				System.out.println("Map file incomplete");
				System.exit(1);
			}
		} catch (Exception e) {
			System.out.println("EditMap cannot read " + fileName);
			System.out.println(e);
			System.exit(1);
		}
		panel.setWalls(walls,worldSize);
	}

	/** Write an output file.
	 *  @param fileName is the name of the input file to be read
	 */
	private void writeFile(String fileName) {
		try {
			FileWriter omap = new FileWriter(new File(fileName));
			for (int y = worldSize-1; y >= 0; y--) {
				for (int x = 0; x < worldSize; x++) {
					switch (walls[x+y*worldSize]) {
					case 0:
						omap.write(' '); break;
					case 1:
						omap.write('|'); break;
					case 2:
						omap.write('-'); break;
					case 3:
						omap.write('+'); break;
					}
				}
				omap.write('\n');
			}
			omap.close();
		} catch (Exception e) {
			System.out.println("EditMap cannot write " + fileName);
			System.out.println(e);
			System.exit(1);
		}
	}


/*

	/** Main method processes arguments, connects to NetMgr and
	 *  responds to received commands.
	 */
	public static void main(String[] args){
		// process arguments and setup channel to NetMgr
		InetSocketAddress nmSockAdr = processArgs(args);
		if (nmSockAdr == null)
			Util.fatal("can't build socket address");
		SocketChannel nmChan = connectToNetMgr(nmSockAdr);
		if (nmChan == null)
			Util.fatal("can't connect to NetMgr");

		net = new NetInfo(maxNode, maxLink, maxRtr, maxComtree);
		InputStream in = null;
		try {
			in = nmChan.socket().getInputStream();
			if (!net.read(in))
				Util.fatal("can't read network topology\n");
		} catch(Exception e) {
			Util.fatal("can't read network topology\n" + e);
		}

		Scanner nmIn = null;
		try { nmIn = new Scanner(nmChan);
		} catch (Exception e) {
			Util.fatal("can't create scanner for NetMgr channel "
				   + e);
		}
		OutputStream out = null;
		try {
			out = nmChan.socket().getOutputStream();
		} catch(Exception e) {
			Util.fatal("can't open output stream to NetMgr " + e);
		}

		// initial display
		linkDetail = false;
		setupDisplay();
		int ctx = net.firstComtIndex();
		if (ctx == 0) Util.fatal("Network contains no comtrees");
		currentComtree = net.getComtree(ctx);
		updateDisplay();
		// start main loop
		while (true) {
			String lineBuf = readKeyboardIn();
			if (lineBuf.length() > 0) processKeyboardIn(lineBuf);
			if (currentComtree == 0) break;
			// send currentComtree to ComtCtl
			Integer CurComt = currentComtree;
			String s = CurComt.toString() + "\n";
			try {
				out.write(s.getBytes());
			} catch(Exception e) {
				Util.fatal("can't write to ComtCtl " + e);
			}
			// now read updated status and update comtree
			NetInfo.ComtreeInfo modComt;
			modComt = net.readComtStatus(in);
			if (modComt.comtreeNum != currentComtree) {
				Util.fatal("received comtree info does not "
					   + "match request");
			}
			if (net.validComtree(currentComtree)) 
				net.updateComtree(modComt);
			else 
				net.addComtree(modComt);
			updateDisplay();
		}
	}

	/** Process command line arguments.
	 *  @param args - command line arguments for ComtreeDisplay
	 *  @return the socket address of the ComtCtl program or null
 	 *  on failure
	 */
	private static InetSocketAddress processArgs(String[] args) {
		InetSocketAddress sockAdr;
                try {
			InetAddress adr = InetAddress.getByName(args[0]);
			int port = Integer.parseInt(args[1]);
                        sockAdr = new InetSocketAddress(adr,port);
                } catch (Exception e) {
                        System.out.println("usage: ComtreeDisplay ipAdr port");
                        System.out.println(e);
                        return null;
                }
                return sockAdr;
        }

	private static SocketChannel
	connectToComtCtl(InetSocketAddress sockAdr) {
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

	private static BufferedReader kbStream; ///< for keyboard input

	/** Attempt to read a line of input from System.in.
         *  Return the line or "" if there is no input available to read.
         */
        private static String readKeyboardIn() {
                String lineBuf = "";
                try {
                        if (kbStream == null) {
                                kbStream = new BufferedReader(
					new InputStreamReader(System.in));
                                System.out.print("::");
                        } else if (kbStream.ready()) {
				lineBuf = kbStream.readLine();
                                System.out.print("::");
			}
                } catch(Exception e) {
                        System.out.println("input error: " + e);
			return null;
                }
                return lineBuf;
        }

	private static boolean processKeyboardIn(String lineBuf) {
		Scanner line = new Scanner(lineBuf);
		if (line.hasNextInt()) {
			currentComtree = line.nextInt();
		} else if (line.hasNext("linkDetail")) {
			linkDetail = true;
		} else if (line.hasNext("noLinkDetail")) {
			linkDetail = false;
		} else if (line.hasNext("quit")) {
			System.exit(0);
		} else return false;
		return true;
	}

	private static void setupDisplay() {
		int numObj = 0;
		StdDraw.setCanvasSize(SIZE, SIZE);
		StdDraw.setPenRadius(PEN_RADIUS);
                Color c = new Color(210,230,255);
		StdDraw.clear(c);
		StdDraw.show(50); 
	}

	private static void updateDisplay() {
                Color c = new Color(210,230,255);
		StdDraw.clear(c);
		double lmargin = .05;	// left margin
		double rmargin = .85;	// right margin
		double tmargin = .95;	// top margin
		double bmargin = .05;	// bottom margin
		double xorigin = (rmargin + lmargin)/2;
		double yorigin = (tmargin + bmargin)/2;

		// list all comtree numbers in the right margin
		double x, y, delta;
		x = rmargin + .12; delta = .03; y = tmargin;
		StdDraw.setPenColor(Color.RED);
		StdDraw.text(x, y, "" + currentComtree);
		y -= 2*delta;
		StdDraw.setPenColor(Color.BLACK);
		for (int ctx = net.firstComtIndex(); ctx != 0 && y > bmargin;
			 ctx = net.nextComtIndex(ctx)) {
			if (net.getComtree(ctx) == currentComtree) continue;
			StdDraw.text(x, y, "" + net.getComtree(ctx));
			y -= delta;
		}
		StdDraw.setPenColor(Color.BLACK);

		// find min and max latitude and longitude
		// expressed in degrees
		double minLat  =   90; double maxLat  =  -90;
		double minLong =  180; double maxLong = -180;
		for (int node = net.firstNode(); node != 0;
			 node = net.nextNode(node)) {
			minLat  = Math.min(minLat,  net.getNodeLat(node));
			maxLat  = Math.max(maxLat,  net.getNodeLat(node));
			minLong = Math.min(minLong, net.getNodeLong(node));
			maxLong = Math.max(maxLong, net.getNodeLong(node));
		}
		double xdelta  = maxLong - minLong;
		double ydelta  = maxLat  - minLat;
		double xcenter = (maxLong + minLong)/2;
		double ycenter = (maxLat  + minLat)/2;
		double xscale  = (rmargin-lmargin)/(maxLong-minLong);
		double yscale  = (tmargin-bmargin)/(maxLat-minLat);
		double scale   = Math.min(xscale,yscale);

		int ctx = net.getComtIndex(currentComtree);
		double nodeRadius = .05;
		// draw all the links
		for (int lnk = net.firstLink(); lnk != 0;
			 lnk = net.nextLink(lnk)) {
			int left = net.getLinkL(lnk);
			int right = net.getLinkR(lnk);
			double lx = xorigin +
				     (net.getNodeLong(left) - xcenter)*scale;
			double ly = yorigin +
				     (net.getNodeLat(left) - ycenter)*scale;
			double rx = xorigin +
				     (net.getNodeLong(right) - xcenter)*scale;
			double ry = yorigin +
				     (net.getNodeLat(right) - ycenter)*scale;
			StdDraw.setPenColor(Color.BLACK);
			StdDraw.setPenRadius(PEN_RADIUS);
			if (net.isComtLink(ctx,lnk))
				StdDraw.setPenRadius(5*PEN_RADIUS);
			StdDraw.line(lx,ly,rx,ry);
			StdDraw.setPenRadius(PEN_RADIUS);
			if (net.isComtLink(ctx,lnk) && linkDetail) {
				x = (lx+rx)/2; y = (ly+ry)/2;
				StdDraw.setPenColor(Color.WHITE);
				StdDraw.filledSquare(x, y, 1.5*nodeRadius);
				StdDraw.setPenColor(Color.BLACK);
				StdDraw.setPenRadius(PEN_RADIUS);
				StdDraw.square(x, y, 1.5*nodeRadius);
				int bru, pru, brd, prd;
				if (net.isLeaf(left) || net.isLeaf(right)) {
					bru = net.getComtLeafBrUp(ctx);
					pru = net.getComtLeafPrUp(ctx);
					brd = net.getComtLeafBrDown(ctx);
					prd = net.getComtLeafPrDown(ctx);
				} else {
					bru = net.getComtBrUp(ctx);
					pru = net.getComtPrUp(ctx);
					brd = net.getComtBrDown(ctx);
					prd = net.getComtPrDown(ctx);
				}
				StdDraw.text(x, y+.8*nodeRadius, "brU=" + bru);
				StdDraw.text(x, y+.3*nodeRadius, "prU=" + pru);
				StdDraw.text(x, y-.3*nodeRadius, "brD=" + brd);
				StdDraw.text(x, y-.8*nodeRadius, "prD=" + prd);
			}
		}

		// draw all the nodes in the net
		for (int node = net.firstNode(); node != 0;
			 node = net.nextNode(node)) {
			x = xorigin + (net.getNodeLong(node) - xcenter)*scale;
			y = yorigin + (net.getNodeLat(node) - ycenter)*scale;

			int lnkCnt = 0;
			Color nodeColor = Color.LIGHT_GRAY;
			if (net.isComtNode(ctx,node)) {
				lnkCnt = net.getComtLnkCnt(ctx,node);
				nodeColor = Color.WHITE;
				if (node == net.getComtRoot(ctx))
					nodeColor = Color.YELLOW;
			}
			StdDraw.setPenColor(nodeColor);
			if (net.getNodeType(node) == Forest.NodeTyp.ROUTER) {
				StdDraw.filledCircle(x, y, nodeRadius);
				StdDraw.setPenColor(Color.BLACK);
				if (net.isComtCoreNode(ctx,node))
					StdDraw.setPenRadius(3*PEN_RADIUS);
				StdDraw.circle(x, y, nodeRadius);
				StdDraw.setPenRadius(PEN_RADIUS);
				StdDraw.text(x, y+nodeRadius/3,
					     net.getNodeName(node));
				StdDraw.text(x, y-nodeRadius/3, "" + lnkCnt);
			} else {
				StdDraw.filledSquare(x, y, nodeRadius);
				StdDraw.setPenColor(Color.BLACK);
				StdDraw.setPenRadius(PEN_RADIUS);
				StdDraw.square(x, y, nodeRadius);
				StdDraw.text(x, y+nodeRadius/3,
					        net.getNodeName(node));
				StdDraw.text(x, y-nodeRadius/3,
					        "" + Forest.fAdr2string(
						     net.getNodeAdr(node)));
			}
		}
		StdDraw.show(50);
	}
*/
}
