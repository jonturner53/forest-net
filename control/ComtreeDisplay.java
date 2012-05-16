package forest.control;

/** @file ComtreeDisplay.java */

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
import princeton.*;

/** Display comtrees based on the information received from a ComtCtl object.
 *  Connect to the ComtCtl object using a stream socket (port 30133).
 *  On connecting, the ComtCtl sends a sequence of "topology definitions"
 *  identifying the nodes in the network graph and the links connecting them.
 *  After sending the topology definitions, the ComtCtl sends comtree 
 *  comtree definitions, after which it sends status messages whenever
 *  any comtree that it controls changes.
 *
 *  Each message from the ComtCtl is sent on a line by itself, starting
 *  with a keyword and one or more attributes in a fixed order.
 *  More specifically, we can have
 *
 *  node nodeName nodeType lat long address
 *  link (nodeName.5,nodeName.3) length bitRate pktRate
 *  comtree comtreeNum rootNodeName
 *  coreNode comtreeNum nodeName
 *  incLnkCnt comtreeNum rtrName
 *  decLnkCnt comtreeNum rtrName
 *  addComtreeLink comtNum (nodeName,nodeName.5)
 *  	bRateL pRateL bRateR pRateR
 *  dropComtreeLink comtNum (nodeName,nodeName.5)
 *
 *  The node and link messages define the underlying network topology.
 *  The nodeType can be one of router, controller, client. The latitude
 *  and longitude are given in degrees and affect the placement of the
 *  nodes in the graphical display. The links are defined by their endpoints.
 *  For routers, these include a link number. For leaf nodes, they do not.
 *  Lengths are given in km, bit rates in kb/s, packet rates in packets/sec.
 *  The leaf count for a comtree node is the number of leaves in the comtree.
 *  The bit rates for a comtree link are the rights from the left endpoint
 *  (as the link is given) and the right endpoint.
 *  If a comtree message is given without a rootNodeName, it is
 *  interpreted as "delete comtree" message. If a comtreeNode message
 *  is given without a leafCount, it is intepreted a "delete comtree node"
 *  message. If a comtreeLink message omits the link rates, it is interpreted
 *  as a "delete comtree link" message.
 */
public class ComtreeDisplay {
	private static final int maxNode    = 10000;
	private static final int maxLink    = 20000;
	private static final int maxRtr     =  1000;
	private static final int maxCtl     =   100;
        private static final int maxComtree = 20000;

	private static final int SIZE = 700;        	///<size of gui frame
	private static final double PEN_RADIUS = .003;	///<default line weight

	private static InetSocketAddress ccSockAdr; ///< address for ComtCtl
	private static SocketChannel ccChan = null; ///< for ComtCtl connection


	private static int currentComtree; ///< comtree to be displayed
	private static boolean linkDetail; ///< when true, show link details

	private static NetInfo net;	///< contains net toppology and
					///< comtree info
	
	/** Main method processes arguments, connects to ComtCtl and
	 *  responds to received commands.
	 */
	public static void main(String[] args){
		// process arguments and setup channel to ComtreeCtl
		InetSocketAddress ccSockAdr = processArgs(args);
		if (ccSockAdr == null)
			Util.fatal("can't build socket address");
		SocketChannel ccChan = connectToComtCtl(ccSockAdr);
		if (ccChan == null)
			Util.fatal("can't connect to ComtCtl");

		net = new NetInfo(maxNode, maxLink, maxRtr, maxCtl, maxComtree);
		try {
			if (!net.read(ccChan.socket().getInputStream()))
				Util.fatal("can't read network topology\n");
		} catch(Exception e) {
			Util.fatal("can't read network topology " + e);
		}

		Scanner ccIn = null;
		try { ccIn = new Scanner(ccChan);
		} catch (Exception e) {
			Util.fatal("can't create scanner for ComtCtl channel "
				   + e);
		}

		// start main loop
		setupDisplay();
		currentComtree = 0;
		while (true) {
			// process commands from ComtCtl
			while (ccIn.hasNext()) {
				String errMsg = processComtCtlCmd(ccIn);
				if (!errMsg.equals(""))
					System.err.println("Error in message "
						+ "from ComtCtl: " + errMsg);
			}
			String lineBuf = readKeyboardIn();
			if (lineBuf.length() > 0) {
				processKeyboardIn(lineBuf);
			}
			if (currentComtree == 0) {
				currentComtree = net.getComtree(
						 net.firstComtIndex());
			}
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

	private static SocketChannel connectToComtCtl(
		InetSocketAddress sockAdr) {
		// Open channel to monitor and make it nonblocking
		SocketChannel chan;
                try {
                        chan = SocketChannel.open(sockAdr);
                        chan.configureBlocking(false);
                } catch (Exception e) {
                        System.out.println("Can't open channel to monitor");
                        System.out.println(e);
                        return null;
                }
		return chan;
	}

	/** Process command from the comtree controller.
	 *  node nodeName nodeType lat long address
 	 *  link (nodeName.5,nodeName.3) length bitRate pktRate
 	 *  comtree comtreeNum rootNodeName
 	 *  coreNode comtreeNum nodeName
	 *  incLnkCnt comtreeNum rtrName
	 *  decLnkCnt comtreeNum rtrName
         *  addComtreeLink comtNum (nodeName,nodeName.5)
	 *  	bRateL pRateL bRateR pRateR
         *  dropComtreeLink comtNum (nodeName,nodeName.5)
	 *
	 *  addModComtree comtreeNum ...
	 *  dropComtree comtreeNum
	 *  incLnkCnt comtreeNum rtrName
         *  decLnkCnt comtreeNum rtrName
	 */
	private static String processComtCtlCmd(Scanner ccChan) {
		if (ccChan.hasNext("addModifyComtree")) {
			ccChan.next();
			if (net.readComt(ccChan).length() == 0) {
				return "could not read comtree in "
					+ "addModifyComtree command";
			}
		} else if (ccChan.hasNext("dropComtree")) {
			ccChan.next();
			if (!ccChan.hasNextInt()) {
				return	"cannot read expected comtree number "
					+ "in dropComtree command";
			}
			int comt = ccChan.nextInt();
			int ctx = net.getComtIndex(comt);
			if (ctx == 0) return "";
			if (!net.removeComtree(ctx)) 
				return "unable to remove comtree " + comt;
		} else if (ccChan.hasNext("modLinkCnt")) {
			ccChan.next();
			if (!ccChan.hasNextInt()) {
				return	"cannot read expected comtree number "
					+ "in modLinkCnt command";
			}
			int comt = ccChan.nextInt();
			int ctx = net.getComtIndex(comt);
			if (ctx == 0)
				return "invalid comtree number in modLinkCnt "
					+ " command";
			if (!ccChan.hasNextInt()) {
				return	"cannot read expected router number "
					+ "in modLinkCnt command";
			}
			int rtr = ccChan.nextInt();
			if (!ccChan.hasNextInt()) {
				return	"cannot read expected increment value "
					+ "in modLinkCnt command";
			}
			int delta = ccChan.nextInt();
			if (delta < 0) net.decComtLnkCnt(ctx,rtr);
			if (delta > 0) net.incComtLnkCnt(ctx,rtr);
		}
		return "";	
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
		} else if (line.hasNext("linkNoDetail")) {
			linkDetail = false;
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
		double lmargin = .1;	// left margin
		double rmargin = .85;	// right margin
		double tmargin = .9;	// top margin
		double bmargin = .1;	// bottom margin

		// list all comtree numbers in the right margin
		double x, y, delta;
		x = rmargin + .07; delta = .02; y = tmargin;
		for (int ctx = net.firstComtIndex(); ctx != 0 && y > bmargin;
			 ctx = net.nextComtIndex(ctx)) {
			StdDraw.setPenColor(Color.BLACK);
			if (net.getComtree(ctx) == currentComtree)
				StdDraw.setPenColor(Color.RED);
			StdDraw.text(x, y, "" + net.getComtree(ctx));
			y -= delta;
		}
		StdDraw.setPenColor(Color.BLACK);

		// find min and max latitude and longitude
		// expressed in degrees
		double minLat  =   90; double maxLat  =  -90;
		double minLong =  180; double maxLong = -180;
		double xdelta  = maxLong - minLong;
		double ydelta  = maxLat  - minLat;
		for (int node = net.firstNode(); node != 0;
			 node = net.nextNode(node)) {
			minLat  = Math.min(minLat,  net.getNodeLat(node));
			maxLat  = Math.max(minLat,  net.getNodeLat(node));
			minLong = Math.min(minLong, net.getNodeLong(node));
			maxLong = Math.max(minLong, net.getNodeLong(node));
		}

		// draw all the nodes in the net
		int ctx = net.getComtIndex(currentComtree);
		double nodeRadius = .03;
		for (int node = net.firstNode(); node != 0;
			 node = net.nextNode(node)) {
			x = lmargin + ((net.getNodeLong(node) - minLong)/xdelta)
				    * (rmargin-lmargin);
			y = bmargin + ((net.getNodeLat(node) - minLat)/ydelta)
				    * (tmargin-bmargin);
			if(net.isComtNode(ctx,node))
				StdDraw.setPenColor(Color.LIGHT_GRAY);
			else
				StdDraw.setPenColor(Color.WHITE);
			if (net.getNodeType(node) == Forest.NodeTyp.ROUTER) {
				StdDraw.filledCircle(x, y, nodeRadius);
				StdDraw.setPenColor(Color.BLACK);
				StdDraw.setPenRadius(PEN_RADIUS);
				StdDraw.circle(x, y, nodeRadius);
				StdDraw.text(x, y+nodeRadius/3,
					     net.getNodeName(node));
				StdDraw.text(x, y-nodeRadius/3,
					     "" + net.getComtLnkCnt(ctx,node));
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
		// draw all the links
		for (int lnk = net.firstLink(); lnk != 0;
			 lnk = net.nextLink(lnk)) {
			int left = net.getLinkL(lnk);
			int right = net.getLinkR(lnk);
			double lx = lmargin +
				     ((net.getNodeLong(left) - minLong)/xdelta)
				     * (rmargin-lmargin);
			double ly = bmargin +
				     ((net.getNodeLat(left) - minLat)/ydelta)
				     * (tmargin-bmargin);
			double rx = lmargin +
				     ((net.getNodeLong(right) - minLong)/xdelta)
				     * (rmargin-lmargin);
			double ry = bmargin +
				     ((net.getNodeLat(right) - minLat)/ydelta)
				     * (tmargin-bmargin);
			StdDraw.setPenColor(Color.BLACK);
			StdDraw.setPenRadius(PEN_RADIUS);
			if (net.isComtLink(ctx,lnk))
				StdDraw.setPenRadius(3*PEN_RADIUS);
			StdDraw.line(lx,ly,rx,ry);
			StdDraw.setPenRadius(PEN_RADIUS);
			if (net.isComtLink(ctx,lnk) && linkDetail) {
				x = (lx+rx)/2; y = (ly+ry)/2;
				StdDraw.setPenColor(Color.WHITE);
				StdDraw.filledSquare(x, y, nodeRadius);
				StdDraw.setPenColor(Color.BLACK);
				StdDraw.setPenRadius(PEN_RADIUS);
				StdDraw.square(x, y, nodeRadius);
				StdDraw.text(x, y+.3*nodeRadius,
					"brU=" + net.getComtBrUp(ctx,lnk));
				StdDraw.text(x, y+.1*nodeRadius,
					"prU=" + net.getComtPrUp(ctx,lnk));
				StdDraw.text(x, y-.1*nodeRadius,
					"brD=" + net.getComtBrDown(ctx,lnk));
				StdDraw.text(x, y-.3*nodeRadius,
					"prD=" + net.getComtPrDown(ctx,lnk));
			}
		}
		StdDraw.show(50);
	}
}
