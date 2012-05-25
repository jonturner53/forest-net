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
	private static final int maxNode    = 10000;
	private static final int maxLink    = 20000;
	private static final int maxRtr     =  1000;
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

		net = new NetInfo(maxNode, maxLink, maxRtr, maxComtree);
		InputStream in = null;
		try {
			in = ccChan.socket().getInputStream();
			if (!net.read(in))
				Util.fatal("can't read network topology\n");
		} catch(Exception e) {
			Util.fatal("can't read network topology\n" + e);
		}

		Scanner ccIn = null;
		try { ccIn = new Scanner(ccChan);
		} catch (Exception e) {
			Util.fatal("can't create scanner for ComtCtl channel "
				   + e);
		}
		OutputStream out = null;
		try {
			out = ccChan.socket().getOutputStream();
		} catch(Exception e) {
			Util.fatal("can't open output stream to ComtCtl " + e);
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
		// Open channel to monitor
		SocketChannel chan;
                try {
                        chan = SocketChannel.open(sockAdr);
                } catch (Exception e) {
                        System.out.println("Can't open channel to monitor");
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
}
