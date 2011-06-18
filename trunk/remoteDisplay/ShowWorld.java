package remoteDisplay;

import java.awt.Color;
import java.lang.Math;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;

import princeton.StdDraw;

/**
 *  ShowWorld provides a visual display of the avatars moving around
 *  within a virtual world. It connects to a remote monitor that
 *  delivers reports of avatar movements and displays the data
 *  in in the form of a simple animation
 *  @author Jon Turner
 */
public class ShowWorld {
	private static Map<Integer, AvatarGraphic> status; // avatar status
	private static final int INTERVAL = 50;	// time between updates (in ms)
	private static int comtree;	// number of comtree to be monitored
	
	private static final int MON_PORT = 30124; // port number used by monitor
	private static final String WALLS = "0010100101110100100101101"; // list of solid walls
	private static SocketChannel monChan = null;	// channel to remote monitor
	private static ByteBuffer repBuf = null; 	// buffer for report packets
	private static AvatarStatus rep = null;		// most recent report

	private static boolean needData = true;		// true when getReport() needs
							// more data to process
	
	/**
	 * Get the next status report from a buffered status packet.
	 * Receive additional status packets as needed using nonblocking IO.
	 * @return the AvatarStatus object for the most recent status report
	 * or null if there is no report available
	 */
	private static AvatarStatus getReport() {
		if (repBuf == null) {
			repBuf = ByteBuffer.allocate(36*36);
			repBuf.clear();
		}
		
		if (needData) {
			if (repBuf.position() == repBuf.limit())
				repBuf.clear();
			while (needData) {
				try {					
					if (monChan.read(repBuf) == 0)
						return null;
					if (repBuf.position() >= 36) {
						repBuf.flip(); needData = false;
					}
				} catch(Exception x) {
					System.out.println("Exception when " + 
						  "attempting to read report");
					System.out.println(x);
					System.exit(1);
				}    	    	   
			}
		}		
		
		if (rep == null) rep = new AvatarStatus();
		rep.when = repBuf.getInt()/1000;
		rep.id = repBuf.getInt();
		rep.x = repBuf.getInt()/1000000.0;
		rep.y = repBuf.getInt()/1000000.0;
		rep.dir = repBuf.getInt();
		repBuf.getInt(); // discard value
		rep.numVisible = repBuf.getInt();
		rep.numNear = repBuf.getInt();
		rep.comtree = repBuf.getInt();
		if (repBuf.remaining() < 36) needData = true;
			
		return rep;		
	}

	private static AvatarStatus lastRep = null; // used by processFrame
	
	/**
	 * Process reports for which the timestamp is <= bound
	 * and redraw all markers in their new positions.
	 * @param bound is the time bound for which reports should
	 * be processed
	 * @return the timestamp of the first report processed
	 * or -1 if there are no reports
	 */
	private static int processFrame(int bound) {
		AvatarStatus rep;	 
		rep = (lastRep != null ? lastRep : getReport());
		int firstTimeStamp = (rep != null ? rep.when : -1);
		while (rep != null) {
			if (rep.when > bound) {		
				lastRep = rep; return firstTimeStamp;
			}
			AvatarGraphic m = status.get(rep.id);
			if (m == null) {
				m = new AvatarGraphic(rep);				
				status.put(rep.id, m);
			} else {
				m.update(rep);
			}
			rep = getReport();
		}
		return firstTimeStamp;
	}
	
	private static BufferedReader inStream = null;
	
	/**
	 * Attempt to read a new comtree number from System.in.
	 * Return the comtree number or -1 if no input is available.
	 */
	private static int readComtree() {
		int comtree = 0;
		try {
			if (inStream == null) {
				inStream = new BufferedReader(
						new InputStreamReader(System.in));
				System.out.print("\ncomtree=");
				comtree = Integer.parseInt(inStream.readLine());
			} else if (inStream.ready()) {
				comtree = Integer.parseInt(inStream.readLine());
			} else return -1;
		} catch(Exception e) {
			System.out.println("input error: type comtree number\n" + e);
		}
		System.out.print("\ncomtree=");
		return comtree;
	}
	
	private static InetSocketAddress monSockAdr;
	
	/**
	 *  Process command line arguments
	 *   - name of monitor host (or localhost)
	 */
	private static boolean processArgs(String[] args) {
		try {
			monSockAdr = new InetSocketAddress(args[0], MON_PORT);
System.out.println(monSockAdr);
		} catch (Exception e) {
			System.out.println("usage: ShowWorldNet monIp");
			System.out.println(e);
			System.exit(1);
		}
		return true;
	}
	
	private static ByteBuffer sendBuf = null;    // buffer for comtree pkt
	
	/** Send a comtree number to the remote Monitor.
	 *  @param comtree is the comtree number to send
	 */
	private static void sendComtree(int comtree) {
		if (sendBuf == null) sendBuf = ByteBuffer.allocate(16); 	
		//  send comtree # to remote monitor
		try { 		
			sendBuf.putInt(comtree);
			sendBuf.flip();
			do {
				monChan.write(sendBuf);
			} while (sendBuf.position() == 0);
			sendBuf.clear();
		} catch(Exception e) { 
			System.out.println("Can't send comtree to Monitor"); 
			System.out.println(e);
			System.exit(1); 
		}	
	}

	/** Draw grid with a background color of c.
	 */
	private static void drawGrid(Color c) {
		StdDraw.clear(c);
		StdDraw.setPenColor(Color.GRAY);
		for (int i = 0; i <= 5; i++) {
			StdDraw.line(0,.2*i,1,.2*i);
			StdDraw.line(.2*i,0,.2*i,1);
		}
		StdDraw.setPenColor(Color.BLACK);
		StdDraw.text(.08, -.02, "comtree: " + comtree);
	}
	
	/**
	 * The ShowWorldNet program displays a visualization of a very
	 * simple virtual world. Avatars moving around in the virtual
	 * world are displayed as they move.
	 * 
	 * There are two command line arguments. The first is the IP address
	 * of the remote Monitor program from which we receive reports
	 * concerning the virtual world. The second is the comtree 
	 * to be monitored
	 */
	public static void main(String[] args) {	
		// process command line arguments	
		processArgs(args);
				
		// Open channel to monitor and make it nonblocking
		try {
			monChan = SocketChannel.open(monSockAdr);
			monChan.configureBlocking(false);
		} catch (Exception e) {
			System.out.println("Can't open channel to monitor");
			System.out.println(e);
			System.exit(1);
		}

		// setup canvas
		comtree = 0;
		StdDraw.setCanvasSize(700,700);
		StdDraw.setPenRadius(.003);
		Color c = new Color(210,230,255);
		drawGrid(c);
		StdDraw.show(INTERVAL);

		// setup hashmap for reports from each host
		status = new HashMap<Integer, AvatarGraphic>();

		// Prompt for comtree number and send to monitor
		comtree = readComtree();
		sendComtree(comtree);

		AvatarStatus firstRep;
		while ((firstRep = getReport()) == null) {}
		int monTime = firstRep.when - 200; // build in some delay
		
		long localTime = System.nanoTime()/1000000;
		long targetLocalTime = localTime;

		while (true) {
			processFrame(monTime+INTERVAL);
			
			Set<Integer> idSet = status.keySet();
			
			drawGrid(c);
			for (Integer id : idSet) {
				AvatarGraphic m = status.get(id);
				if (m.getComtree() == comtree)
					m.draw();
			}
		
			localTime = System.nanoTime()/1000000;
			if (localTime < targetLocalTime + INTERVAL) {
				StdDraw.show((int) ((targetLocalTime+INTERVAL)
						     - localTime));
			} else {
				StdDraw.show(0);
			}

			monTime += INTERVAL; targetLocalTime += INTERVAL;
			
			int newComtree = readComtree();
			if (newComtree >= 0 && newComtree != comtree) {
				comtree = newComtree; sendComtree(comtree);
			}
		}
	}
}
