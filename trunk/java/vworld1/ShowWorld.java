package forest.vworld1;

import java.awt.Color;
import java.awt.event.*;
import java.util.HashMap;
import java.util.Map;
import java.util.HashSet;
import java.util.Set;
import java.util.Iterator;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import princeton.StdDraw;
import java.util.Scanner;
/**
 *  ShowWorld provides a visual display of the avatars moving around
 *  within a virtual world. It connects to a remote monitor that
 *  delivers reports of avatar movements and displays the data
 *  in in the form of a simple animation
 *  @author Jon Turner
 */
public class ShowWorld {
	private static final int MON_PORT = 30124; // port # used by monitor
	private static final int INTERVAL = 50;	// time between updates (in ms)
	private static final int GRID = 10000;	// size of one grid space
	private static int MAX_WORLD = 5000; // max extent of virtual world
	private static int MAX_VIEW = 100; // max extent of view
	private static int NUM_ITEMS = 9; // # of items in status report

	private static int worldSize;	// # of squares in full map
	private static int gridSize;	// # of squares in map (x or y)
	private static int viewSize;	// # of map squares viewed at one time
	private static int cornerX = 0; // x coordinate of current view
	private static int cornerY = 0; // y coordinate of current view
	private static int comtree;	// number of comtree to be monitored
	private static Scanner mazeFile; // filename of maze
	private static int[] walls; 	// list of walls
	private static SocketChannel chan = null;// channel to remote monitor
						// or avatar
	private static ByteBuffer repBuf = null; // buffer for report packets
	private static AvatarStatus rep = null;	// most recent report
	private static boolean needData = true;	// true when getReport() needs
						// more data to process
	
	private static Set<Integer> recentIds;	// list of recently seen ids
	private static int idCounter;		// counter used to clear old ids	
	private static Map<Integer, AvatarGraphic> status; // avatar status

	private static InetSocketAddress monSockAdr;

	/**
	 * The ShowWorldNet program displays a visualization of a very
	 * simple virtual world. Avatars moving around in the virtual
	 * world are displayed as they move.
	 *
	 *  Command line arguments
	 *   - name/address of monitor host (or localhost)
	 *   - walls file
	 */
	public static void main(String[] args) {	
		// process command line arguments	
		processArgs(args);	

		// Open channel to monitor and make it nonblocking
		try {
			chan = SocketChannel.open(monSockAdr);
			chan.configureBlocking(false);
		} catch (Exception e) {
			System.out.println("Can't open channel to monitor");
			System.out.println(e);
			System.exit(1);
		}
		//send viewsize to Monitor.cpp
		viewSize = 20;
		ByteBuffer buff = ByteBuffer.allocate(8);
		buff.put((byte) 'v'); buff.putInt(viewSize);
		buff.flip();
		try {
			chan.write(buff);
		} catch(Exception e) {
			System.out.println("Could not write viewSize to "
					+ "Monitor");
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

		// setup hashset for recent ids
		recentIds = new HashSet<Integer>();
		idCounter = 0;

		// Prompt for comtree number and send to monitor
		comtree = readComtree();
		sendCommand('c',comtree);

		AvatarStatus firstRep;
		while ((firstRep = getReport()) == null) {}
		int monTime = firstRep.when + 3000; // build in some delay
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

			try {
				if(StdDraw.hasNextKeyTyped()) {
					processMoves(StdDraw.nextKeyTyped());
				}
			} catch(Exception e) {
				System.out.println("Could not get map "
					+ "movement from key");
				System.out.println(e);
				System.exit(1);
			}
			int newComtree = readComtree();
			if (newComtree >= 0 && newComtree != comtree) {
				comtree = newComtree;
				sendCommand('c',comtree);
			}
		}
	}

	/**
	 *  Process command line arguments
	 *   - name/address of monitor host (or localhost)
	 *   - walls file
	 */
	private static boolean processArgs(String[] args) {
		try {
			monSockAdr = new InetSocketAddress(args[0],MON_PORT);
			readMapFile(args[1]);
		} catch (Exception e) {
			System.out.println("usage: ShowWorld remoteIp mapfile");
			System.out.println(e);
			System.exit(1);
		}
		return true;
	}

	/** Read an input file.
	 *  @param fileName is the name of the input file to be read
	 */
	private static void readMapFile(String fileName) {
		try {
			Scanner imap = new Scanner(new File(fileName));
			int y = 0; walls = null;
			boolean horizRow = true;
			while (imap.hasNextLine() && y >= 0) {
				String temp = imap.nextLine();
				if (walls == null) {
					worldSize = temp.length()/2;
					y = worldSize-1;
					viewSize = Math.min(10,worldSize);
					walls = new int[worldSize*worldSize];
				} else if (temp.length()/2 != worldSize) {
					System.out.println("Map file has "
						 + "mismatched line lengths");
					System.exit(1);
				}
				for(int xx = 0; xx < 2*worldSize; xx++) {
					int pos = y * worldSize + xx/2;
					if (horizRow) {
						if ((xx&1) == 0) continue;
						if (temp.charAt(xx) == '-')
							walls[pos] |= 2;
						continue;
					}
					if ((xx&1) != 0) {
						if (temp.charAt(xx) == 'x')
							walls[pos] |= 4;
					} else if (temp.charAt(xx) == '|') 
						walls[pos] |= 1;
				}
				horizRow = !horizRow;
				if (horizRow) y--;
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
	}

	/** Process movement commands.
	 *  This method processes characters typed in the graphics window,
	 *  interpreting them as commands to change the current view
	 *  of the virtual world. The commands modify local variables
	 *  describing the current view and the new values of the variables
	 *  describing the current view are forwarded to the Monitor,
	 *  so that it can adjust its view.
	 */
	private static void processMoves(char c) throws IOException {
		ByteBuffer buff = ByteBuffer.allocate(8);
		boolean didMove = false;
		switch (c) {
		case 'a':
			if (cornerX > 0) {
				sendCommand('x',--cornerX);
			}
			break;
		case 'A':
			if (cornerX > 0) {
				cornerX = Math.max(0,cornerX-viewSize);
				sendCommand('x',cornerX);
			}
			break;
		case 'd':
			if (cornerX < worldSize-viewSize) {
				sendCommand('x',++cornerX);
			}
			break;
		case 'D':
			if (cornerX < worldSize-viewSize) {
				cornerX = Math.min(cornerX+viewSize,
					      	   worldSize-viewSize);
				sendCommand('x',cornerX);
			}
			break;
		case 's':
			if (cornerY > 0) {
				sendCommand('y',--cornerY);
			}
			break;
		case 'S':
			if (cornerY > 0) {
				cornerY = Math.max(0,cornerY-viewSize);
				sendCommand('y',cornerY);
			}
			break;
		case 'w':
			if (cornerY < worldSize-viewSize) {
				sendCommand('y',++cornerY);
			}
			break;
		case 'W':
			if (cornerY < worldSize-viewSize) {
				cornerY = Math.min(cornerY+viewSize,
					      	   worldSize-viewSize);
				sendCommand('y',cornerY);
			}
			break;
		case 'o':
			if (viewSize > 1) {
				sendCommand('v',--viewSize);
			}
			break;
		case 'O':
			if (viewSize > 1) {
				viewSize /= 2;
				sendCommand('v',viewSize);
			}
			break;
		case 'p':
			if (viewSize < MAX_VIEW &&
			    viewSize < worldSize &&
			    cornerX + viewSize < worldSize &&
			    cornerY + viewSize < worldSize) {
				sendCommand('v',++viewSize);
			}
			break;
		case 'P':
			if (viewSize < MAX_VIEW &&
			    viewSize < worldSize &&
			    cornerX + viewSize < worldSize &&
			    cornerY + viewSize < worldSize) {
				viewSize = Math.min(2*viewSize,
					   Math.min(worldSize-cornerX,
					   Math.min(worldSize-cornerY,
					   	    MAX_VIEW)));
				sendCommand('v',viewSize);
			}
			break;
		}
	}

	private static ByteBuffer sendBuf = null;    // used by sendCommand

	/** Send a command to the Monitor.
	 *  Each command consists of a character denoting the command
	 *  and an integer parameter. These are sent over the channel
	 *  to the Monitor.
	 *  @param c is the character denoting the command
	 *  @param p is a parameter associated with the command
	 */
	private static void sendCommand(char c, int p) {
		try { 		
			if (sendBuf == null)
				sendBuf = ByteBuffer.allocate(8); 	
			sendBuf.put((byte) c);
			sendBuf.putInt(p);
			sendBuf.flip();
			chan.write(sendBuf);
			chan.socket().getOutputStream().flush();
			sendBuf.clear();
		} catch(Exception e) { 
			System.out.println("Can't send command to Monitor"); 
			System.out.println(e);
			System.exit(1); 
		}	
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
				lastRep = rep; break;
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
		// clear an item from status, if no report in 4 frames
		idCounter = (++idCounter)%4;
		if (idCounter == 3) {
			Iterator<Integer> iter = status.keySet().iterator();
			while (iter.hasNext()) {
				Integer i = iter.next();
				if(!recentIds.contains(i)) iter.remove();
			}
			recentIds.clear();
		}
		return firstTimeStamp;
	}
	/**
	 * Get the next status report from a buffered status packet.
	 * Receive additional status packets as needed using nonblocking IO.
	 * @return the AvatarStatus object for the most recent status report
	 * or null if there is no report available
	 */
	private static AvatarStatus getReport() {
		if (repBuf == null) {
			repBuf = ByteBuffer.allocate(1000*4*NUM_ITEMS);
			repBuf.clear();
		}
		
		if (needData) {
			if (repBuf.position() == repBuf.limit())
				repBuf.clear();
			while (needData) {
				try {					
					if (chan.read(repBuf) == 0)
						return null;
					if (repBuf.position() >= 4*NUM_ITEMS) {
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
		// remap xy coordinates to (0,1) scale
		rep.x = (repBuf.getInt() - cornerX*GRID)
			/ ((double) viewSize*GRID);
		rep.y = (repBuf.getInt() - cornerY*GRID)
			/ ((double) viewSize*GRID);
		rep.dir = repBuf.getInt();
		repBuf.getInt(); // discard value
		rep.numVisible = repBuf.getInt();
		rep.numNear = repBuf.getInt();
		rep.comtree = repBuf.getInt();
		if (repBuf.remaining() <  4*NUM_ITEMS) needData = true;

		recentIds.add(rep.id);
		return rep;
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
				System.out.print("\nShowWorld: ");
				comtree = Integer.parseInt(inStream.readLine());
			} else if (inStream.ready()) {
				comtree = Integer.parseInt(inStream.readLine());
			} else return -1;
		} catch(Exception e) {
			System.out.println("input error: type comtree number\n"
					    + e);
		}
		System.out.print("\nShowWorld: ");
		return comtree;
	}

	/** Draw grid with a background color of c.
	 */
	private static void drawGrid(Color c) {
		StdDraw.clear(c);
		StdDraw.setPenRadius(.002);
		double frac = 1.0/viewSize;
		Color lightBlue = new Color(210,230,255);

		// draw squares
		for (int x = cornerX; x < cornerX+viewSize; x++) {
			for (int y = cornerY; y < cornerY+viewSize; y++) {
				int xy = x + y * worldSize;
				if ((walls[xy]&4) != 0)
					StdDraw.setPenColor(Color.gray);
				else
					StdDraw.setPenColor(lightBlue);
				int row = y - cornerY; int col = x - cornerX;
				double cx = frac*(col+.5);
				double cy = frac*(row+.5);
				StdDraw.filledRectangle(cx,cy,frac/2,frac/2);
			}
		}


		// draw grid lines
		StdDraw.setPenColor(Color.lightGray);
		if (viewSize <= 40) {
			for (int i = 0; i <= viewSize; i++) {
				StdDraw.line(0,frac*i,1,frac*i);
				StdDraw.line(frac*i,0,frac*i,1);
			}
		}

		StdDraw.setPenRadius(Math.min(.006,
				     Math.max(.001,
					      .006*(4/Math.sqrt(viewSize)))));
		StdDraw.setPenColor(Color.BLACK);
		for (int x = cornerX; x < cornerX + viewSize; x++) {
			for (int y = cornerY; y < cornerY + viewSize; y++) {
				int xy = x + y * worldSize;
				int row = y - cornerY; int col = x - cornerX;
				if ((walls[xy]&1) != 0) {
					StdDraw.line(frac*col, frac*row,
						     frac*col, frac*(row+1));
				} 
				if ((walls[xy]&2) != 0) {
					StdDraw.line(frac*col,    frac*(row+1),
						     frac*(col+1),frac*(row+1));
				}
			}
		}
		// draw bottom walls where appropriate
		for (int x = cornerX; x < cornerX + viewSize; x++) {
			int y = cornerY - 1;
			if (y < 0) break;
			int xy = x + y * worldSize;
			int row = y - cornerY; int col = x - cornerX;
			if (walls[xy] == 2 || walls[xy] == 3) {
				StdDraw.line(frac*col,    frac*(row+1),
					     frac*(col+1),frac*(row+1));
			}
		}
		// draw right-side walls where appropriate
		for (int y = cornerY; y < cornerY + viewSize; y++) {
			int x = cornerX + viewSize;
			if (x >= worldSize) break;
			int xy = x + y * worldSize;
			int row = y - cornerY; int col = x - cornerX;
			if (walls[xy] == 1 || walls[xy] == 3) {
				StdDraw.line(frac*col, frac*row,
					     frac*col, frac*(row+1));
			} 
		}
		// draw boundary walls if all the way at edge of world
		if (cornerX == 0) StdDraw.line(0,0,0,1);
		if (cornerX == worldSize-viewSize) StdDraw.line(1,0,1,1);
		if (cornerY == 0) StdDraw.line(0,0,1,0);
		if (cornerY == worldSize-viewSize) StdDraw.line(0,1,1,1);

		StdDraw.setPenRadius(.003);
		StdDraw.text(.05, -.025, "(" + cornerX + "," + cornerY + ")");
		StdDraw.text(.9, -.025, "comtree: " + comtree);
	}
}
