package remoteDisplay;

import java.awt.Color;
import java.awt.event.*;
import java.lang.Math;
import java.util.HashMap;
import java.util.Map;
import java.util.HashSet;
import java.util.Set;
import java.util.BitSet;
import java.util.Iterator;
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
public class MazeWorld2 {
	private static Map<Integer, MazeAvatarGraphic> status; // avatar status
	private static final int INTERVAL = 50;	// time between updates (in ms)
	private static int SIZE;	//size of full map
	private static int gridSize;	//number of squares in map (x or y direction)
	private static final int AV_PORT = 30130; // port number used by AvatarController.cpp
	private static String WALLS; // list of solid walls in hex
	private static BitSet wallsSet; // list of walls in bitset form
	private static SocketChannel avaChan = null;
	private static ByteBuffer repBuf = null; 	// buffer for report packets
	private static MazeAvatarStatus rep = null;		// most recent report
	private static int GRID = 200000;		// size of one grid space
	private static boolean needData = true;		// true when getReport() needs
							// more data to process
	private static Set<Integer> recentIds;		//list of recently seen ids
	private static int idCounter;			//counter with which to clear old ids	
	/**
	 * Get the next status report from a buffered status packet.
	 * Receive additional status packets as needed using nonblocking IO.
	 * @return the AvatarStatus object for the most recent status report
	 * or null if there is no report available
	 */
	private static MazeAvatarStatus getReport() {
		if (repBuf == null) {
			repBuf = ByteBuffer.allocate(1000*40);
			repBuf.clear();
		}
		
		if (needData) {
			if (repBuf.position() == repBuf.limit())
				repBuf.clear();
			while (needData) {
				try {					
					if (avaChan.read(repBuf) == 0)
						return null;
					if (repBuf.position() >= 40) {
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
		if (rep == null) rep = new MazeAvatarStatus();
		rep.when = repBuf.getInt()/1000;
		rep.id = repBuf.getInt();
		rep.x = repBuf.getInt()/(double)SIZE;
		rep.y = repBuf.getInt()/(double)SIZE;
		rep.dir = repBuf.getInt();
		repBuf.getInt(); // discard value
		rep.numVisible = repBuf.getInt();
		rep.numNear = repBuf.getInt();
		rep.comtree = repBuf.getInt();
		rep.type = repBuf.getInt();
		if (repBuf.remaining() < 40) needData = true;

		recentIds.add(rep.id);
		return rep;
	}

	private static MazeAvatarStatus lastRep = null; // used by processFrame
	
	/**
	 * Process reports for which the timestamp is <= bound
	 * and redraw all markers in their new positions.
	 * @param bound is the time bound for which reports should
	 * be processed
	 * @return the timestamp of the first report processed
	 * or -1 if there are no reports
	 */
	private static int processFrame(int bound) {
		MazeAvatarStatus rep;
		rep = (lastRep != null ? lastRep : getReport());
		int firstTimeStamp = (rep != null ? rep.when : -1);
		while (rep != null) {
			if (rep.when > bound) {		
				lastRep = rep; return firstTimeStamp;
			}
			MazeAvatarGraphic m = status.get(rep.id);
			if (m == null) {
				m = new MazeAvatarGraphic(rep);		
				status.put(rep.id, m);
			} else {
				m.update(rep);
			}
			rep = getReport();
			idCounter = (++idCounter)%50;
			if(idCounter==49) {
				Set<Integer> temp = new HashSet<Integer>();
				for(Iterator<Integer> iter = status.keySet().iterator();iter.hasNext();) {
					Integer i = iter.next();
					if(!recentIds.contains(i))
						temp.add(i);
				}
				for(Integer i : temp)
					status.remove(i);
				recentIds.clear();
			}
		}
		return firstTimeStamp;
	}
	
	private static InetSocketAddress avaSockAdr;
	
	/**
	 *  Process command line arguments
	 *   - name of monitor host (or localhost)
	 */
	private static boolean processArgs(String[] args) {
		try {
			avaSockAdr = new InetSocketAddress(args[0], AV_PORT);
			WALLS = args[1];
			wallsSet = new BitSet();
			for(int i = 0; i < WALLS.length(); i++) {
				int x = Integer.parseInt(String.valueOf(WALLS.charAt(i)),16);
				String j = Integer.toBinaryString(x);
				for(int k = j.length(); k <4; k++)
					j = "0" + j;
				wallsSet.set(i*4,j.charAt(0)=='1');
				wallsSet.set(i*4+1,j.charAt(1)=='1');
				wallsSet.set(i*4+2,j.charAt(2)=='1');
				wallsSet.set(i*4+3,j.charAt(3)=='1');
			}
			gridSize = Integer.parseInt(args[2]);
			SIZE = GRID*gridSize;
		} catch (Exception e) {
			System.out.println("usage: ShowWorldNet monIp walls size");
			System.out.println(e);
			System.exit(1);
		}
		return true;
	}
	
	private static ByteBuffer sendBuf = null;    // buffer for comtree pkt
	
	/** Take in keystroke and send movements to AvatarController.cpp
	*/
	private static void sendMovements(int c) throws IOException {
		ByteBuffer buff = ByteBuffer.allocate(4);
		switch(c) {
			case KeyEvent.VK_LEFT:
				buff.putInt(1);
				break;
			case KeyEvent.VK_UP:
				buff.putInt(2);
				break;
			case KeyEvent.VK_RIGHT:
				buff.putInt(3);
				break;
			case KeyEvent.VK_DOWN:
				buff.putInt(4);
				break;
		}
		buff.flip();
		avaChan.write(buff);
		avaChan.socket().getOutputStream().flush();
	}


	/** Draw grid with a background color of c.
	 */
	private static void drawGrid(Color c) {
		StdDraw.clear(c);
		StdDraw.setPenColor(Color.GRAY);
		double frac = 1.0/gridSize;
		for (int i = 0; i <= gridSize; i++) {
			StdDraw.line(0,frac*i,1,frac*i);
			StdDraw.line(frac*i,0,frac*i,1);
		}
		StdDraw.setPenColor(Color.BLACK);
		StdDraw.setPenRadius(.006);
		for(int i = 0; i < wallsSet.length(); i++) {
			if(!wallsSet.get(i))
				StdDraw.line(frac*(i%gridSize),frac*(i/gridSize),frac*(i%gridSize)+frac,frac*(i/gridSize));
			else
				StdDraw.line(frac*(i%gridSize),frac*(i/gridSize),frac*(i%gridSize),frac*(i/gridSize)+frac);
		}
		StdDraw.setPenRadius(.003);
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
				
		//Open socket to avatar
		try {
			avaChan = SocketChannel.open(avaSockAdr);
			avaChan.configureBlocking(false);
			avaChan.socket().setTcpNoDelay(true);
		} catch(Exception e) {
			System.out.println("Can't open channel to AvatarController");
			System.out.println(e);
			System.exit(1);
		}

		// setup canvas
		StdDraw.setCanvasSize(700,700);
		StdDraw.setPenRadius(.003);
		Color c = new Color(210,230,255);
		drawGrid(c);
		StdDraw.show(INTERVAL);

		// setup hashmap for reports from each host
		status = new HashMap<Integer, MazeAvatarGraphic>();

		// setup hashset for recent ids
		recentIds = new HashSet<Integer>();
		idCounter = 0;

		MazeAvatarStatus firstRep;
		while ((firstRep = getReport()) == null) {}
		int monTime = firstRep.when + 3000; // build in some delay
		long localTime = System.nanoTime()/1000000;
		long targetLocalTime = localTime;

		while (true) {
			processFrame(monTime+INTERVAL);
			Set<Integer> idSet = status.keySet();
			drawGrid(c);
			for (Integer id : idSet) {
				MazeAvatarGraphic m = status.get(id);
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
			
			//write movement commands to avaChan socket
			try {
				if(StdDraw.hasNextKeyPressed())
					sendMovements(StdDraw.nextKeyPressed());
			} catch(Exception e) {
				System.out.println("Could not connect to AvatarController.cpp");
				System.out.println(e);
				System.exit(1);
			}
		}
	}
}
