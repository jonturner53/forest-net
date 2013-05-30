package forest.control;

/** @file ComtreeDisplay.java
 *
 *  @author Jon Turner
 *  @date 2012
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.util.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.awt.*;
import java.awt.Color;
import java.awt.geom.Point2D;

import algoLib.dataStructures.basic.*;
import algoLib.dataStructures.graphs.*;
import algoLib.misc.*;
import forest.common.*;
import forest.control.*;
import princeton.*;

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
 *  can also be used to turn on/off the display of  detailed link information.
 */
public class ComtreeDisplay {
	private static final int COMTCTL_PORT = 30121;
	private static final int maxNode    = 10000;
	private static final int maxLink    = 20000;
	private static final int maxRtr     =  1000;
        private static final int maxComtree = 20000;

	private static final int SIZE = 700;        	///<size of gui frame
	private static final double PEN_RADIUS = .003;	///<default line weight

	private static InetSocketAddress ccSockAdr; ///< address for ComtCtl
	private static SocketChannel ccChan = null; ///< for ComtCtl connection

	private static int ccomt; ///< comtree to be displayed
	private static boolean linkDetail; ///< when true, show link details

	private static NetInfo net;	///< contains net toppology
	private static ComtInfo comtrees; ///< info about comtrees
	private static java.util.TreeSet<Integer> comtSet;
					///< current set of comtrees
	
	/** Main method processes arguments, connects to ComtCtl and
	 *  responds to received commands.
	 */
	public static void main(String[] args){
		net = new NetInfo(maxNode, maxLink, maxRtr);
		comtrees = new ComtInfo(maxComtree,net);

		// process arguments and setup channel to ComtreeCtl
		InetSocketAddress ccSockAdr = processArgs(args);
		if (ccSockAdr == null)
			Util.fatal("can't build socket address");
		SocketChannel ccChan = connectToComtCtl(ccSockAdr);
		if (ccChan == null) Util.fatal("can't connect to ComtCtl");

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
			if (!net.read(inRdr))
				Util.fatal("can't read network topology\n");
			s = "getComtSet\n";
			out.write(s.getBytes());
			readComtSet(inRdr);
		} catch(Exception e) {
			Util.fatal("can't read network topology/comtrees\n"	
				   + e);
		}

		// initial display (no comtrees yet)
		linkDetail = false;
		setupDisplay();
		updateDisplay();

		// main loop
		ccomt = 0;
		while (true) {
			String lineBuf = readKeyboardIn();
			if (lineBuf.length() > 0) processKeyboardIn(lineBuf);
			if (!comtSet.contains(ccomt)) continue;
			try {
				s = "getComtSet\n";
				out.write(s.getBytes());
				readComtSet(inRdr);
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
			updateDisplay();
		}
	}

	/** Read a response to a getComtSet request.
	 *  The response consists of the string "comtSet" followed by a
	 *  list of comtree numbers, separated by commas and surrounded
	 *  by parentheses.
	 */
	public static void readComtSet(PushbackReader in) {
		String word = Util.readWord(in);
		if (word == null || !word.equals("comtSet")) 
			Util.fatal("readComtSet: unexpected response " + word);
		if (!Util.verify(in,'(')) 
			Util.fatal("readComtSet: expected left paren");
		comtSet.clear();
		Util.MutableInt cnum = new Util.MutableInt();
		while (true) {
			if (!Util.readNum(in,cnum))
				Util.fatal("readComtSet: could not read "
					   + "expected comtree number");
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
	private static InetSocketAddress processArgs(String[] args) {
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

	private static boolean processKeyboardIn(String lineBuf) {
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

	private static void setupDisplay() {
		int numObj = 0;
		StdDraw.setCanvasSize(SIZE, SIZE);
		StdDraw.setPenRadius(PEN_RADIUS);
                Color c = new Color(210,230,255);
		StdDraw.clear(c);
		StdDraw.show(50); 
	}

	private static void updateDisplay() {
                Color bgColor = new Color(210,230,255);
		StdDraw.clear(bgColor);
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
		StdDraw.text(x, y, "" + ccomt);
		y -= 2*delta;
		StdDraw.setPenColor(Color.BLACK);
		Font bigFont = new Font("SansSerif", Font.PLAIN, 16);
		StdDraw.setFont(bigFont);
		for (int c : comtSet) {
			if (y <= bmargin) break;
			if (c == ccomt) continue;
			StdDraw.text(x, y, "" + c);
			y -= delta;
		}
		if (ccomt == 0) { // quite early
			StdDraw.show(500); return;
		}

		StdDraw.setPenColor(Color.BLACK);

		// find min and max latitude and longitude
		// expressed in degrees
		double minLat  =   90; double maxLat  =  -90;
		double minLong =  180; double maxLong = -180;
		Pair<Double,Double> loc = new Pair<Double,Double>(
					new Double(0), new Double(0));
		for (int node = net.firstNode(); node != 0;
			 node = net.nextNode(node)) {
			net.getNodeLocation(node,loc);
			minLat  = Math.min(minLat,  loc.first);
			maxLat  = Math.max(maxLat,  loc.first);
			minLong = Math.min(minLong, loc.second);
			maxLong = Math.max(maxLong, loc.second);
		}
		double xdelta  = maxLong - minLong;
		double ydelta  = maxLat  - minLat;
		double xcenter = (maxLong + minLong)/2;
		double ycenter = (maxLat  + minLat)/2;
		double xscale  = (rmargin-lmargin)/(maxLong-minLong);
		double yscale  = (tmargin-bmargin)/(maxLat-minLat);
		double scale   = Math.min(xscale,yscale);

		int ctx = comtrees.getComtIndex(ccomt);
		double nodeRadius = .05;
		// draw all the links
		for (int lnk = net.firstLink(); lnk != 0;
			 lnk = net.nextLink(lnk)) {
			int left = net.getLeft(lnk);
			int right = net.getRight(lnk);
			net.getNodeLocation(left,loc);
			double lx = xorigin + (loc.second - xcenter)*scale;
			double ly = yorigin + (loc.first - ycenter)*scale;
			net.getNodeLocation(right,loc);
			double rx = xorigin + (loc.second - xcenter)*scale;
			double ry = yorigin + (loc.first - ycenter)*scale;

			StdDraw.setPenColor(Color.BLACK);
			StdDraw.setPenRadius(PEN_RADIUS);
			if (comtrees.isComtLink(ctx,lnk))
				StdDraw.setPenRadius(5*PEN_RADIUS);
			StdDraw.line(lx,ly,rx,ry);
			StdDraw.setPenRadius(PEN_RADIUS);
		}

		// draw all the nodes in the net
		for (int node = net.firstNode(); node != 0;
			 node = net.nextNode(node)) {
			net.getNodeLocation(node,loc);
			x = xorigin + (loc.second - xcenter)*scale;
			y = yorigin + (loc.first - ycenter)*scale;

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
			StdDraw.setPenColor(nodeColor);
			if (net.isRouter(node)) {
				StdDraw.filledCircle(x, y, nodeRadius);
				StdDraw.setPenColor(Color.BLACK);
				if (comtrees.isCoreNode(ctx,node))
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
		// finally, draw all the link details
		Font smallFont = new Font("SansSerif", Font.PLAIN, 12);
		StdDraw.setFont(smallFont);
		for (int lnk = net.firstLink(); lnk != 0;
			 lnk = net.nextLink(lnk)) {
			int left = net.getLeft(lnk);
			int right = net.getRight(lnk);
			net.getNodeLocation(left,loc);
			double lx = xorigin + (loc.second - xcenter)*scale;
			double ly = yorigin + (loc.first - ycenter)*scale;
			net.getNodeLocation(right,loc);
			double rx = xorigin + (loc.second - xcenter)*scale;
			double ry = yorigin + (loc.first - ycenter)*scale;

			StdDraw.setPenColor(Color.BLACK);
			StdDraw.setPenRadius(PEN_RADIUS);
			if (comtrees.isComtLink(ctx,lnk) && linkDetail) {
				x = (lx+rx)/2; y = (ly+ry)/2;
				StdDraw.setPenColor(Color.WHITE);
				StdDraw.filledSquare(x, y, nodeRadius);
				StdDraw.setPenColor(Color.BLACK);
				StdDraw.setPenRadius(PEN_RADIUS);
				StdDraw.square(x, y, nodeRadius);
				RateSpec rs = new RateSpec(0);
				int childAdr = comtrees.getChild(ctx,lnk);
				comtrees.getLinkRates(ctx,childAdr,rs);
				StdDraw.text(x, y+.6*nodeRadius,
						"brU=" + rs.bitRateUp);
				StdDraw.text(x, y+.2*nodeRadius,
						"brD=" + rs.bitRateDown);
				StdDraw.text(x, y-.2*nodeRadius,
						"prU=" + rs.pktRateUp);
				StdDraw.text(x, y-.6*nodeRadius,
						"prD=" + rs.pktRateDown);
			}
		}
		StdDraw.show(500);
	}
}
