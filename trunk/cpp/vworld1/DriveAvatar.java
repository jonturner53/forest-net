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
 *  DriveAvatar provides a visual display of the avatars moving around
 *  within a virtual world from the perspective of a single avatar.
 *  @author Jon Turner
 */
public class DriveAvatar {
	private static final int INTERVAL = 50;	// time between updates (in ms)
	private static final int GRID = 10000;	// size of one grid space
	private static int MAX_WORLD = 30000; // max extent of virtual world
	private static int MAX_VIEW = 1000; // max extent of view
	private static int NUM_ITEMS = 10; // # of items in status report

	private static int worldSize;	// # of squares in full map
	private static int gridSize;	// # of squares in map (x or y)
	private static int viewSize;	// # of map squares viewed at one time
	private static int cornerX = 0; // x coordinate of current view
	private static int cornerY = 0; // y coordinate of current view
	private static int avx = 0; 	// x coordinate of avatar (in squares)
	private static int avy = 0; 	// y coordinate of avatar (in squares)
	private static int comtree;	// number of comtree to be monitored
	private static Scanner mazeFile; // filename of maze
	private static int[] walls; 	// list of walls
	private static SocketChannel chan = null;// channel to remote  avatar
	private static ByteBuffer repBuf = null; // buffer for report packets
	private static AvatarStatus rep = null;	// most recent report
	private static boolean needData = true;	// true when getReport() needs
						// more data to process
	
	private static Set<Integer> recentIds;	// list of recently seen ids
	private static int idCounter;		// counter used to clear old ids	
	private static Map<Integer, AvatarGraphic> status; // avatar status

	private static InetSocketAddress avaSockAdr;


	/**
	 * The DriveAvatar program displays a visualization of a very
	 * simple virtual world from the perspective of a single Avatar.
	 * It also allows the user to control the Avatar using the arrow
	 * keys.
	 *
	 *  Command line arguments
	 *   - name/address of Avatar host (or localhost, when using tunnel)
	 *   - port number used by Avatar host (or local tunnel endpoint)
	 *   - walls file
	 */
	public static void main(String[] args) {	
		// process command line arguments and set initial view
		processArgs(args);	
		cornerX = cornerY = 0;
		viewSize = Math.min(10,worldSize);

		// Open channel to monitor and make it nonblocking
		try {
			chan = SocketChannel.open(avaSockAdr);
			chan.configureBlocking(false);
		} catch (Exception e) {
			System.out.println("Can't open channel to Avatar");
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
		status = new HashMap<Integer, AvatarGraphic>();

		// setup hashset for recent ids
		recentIds = new HashSet<Integer>();
		idCounter = 0;

		comtree = 0;	// first report sets comtree
		AvatarStatus firstRep;
		while ((firstRep = getReport()) == null) {}
		int avaTime = firstRep.when + 3000; // build in some delay
		long localTime = System.nanoTime()/1000000;
		long targetLocalTime = localTime;

		while (true) {
			// process reports for current frame
			processFrame(avaTime+INTERVAL);

			// draw the grid and avatars, based on reports
			Set<Integer> idSet = status.keySet();
			drawGrid(c);
			for (Integer id : idSet) {
				AvatarGraphic m = status.get(id);
				if (m.getComtree() == comtree) m.draw();
			}
			localTime = System.nanoTime()/1000000;
			if (localTime < targetLocalTime + INTERVAL) {
				StdDraw.show((int) ((targetLocalTime + INTERVAL)
						     - localTime));
			} else {
				StdDraw.show(0);
			}
			avaTime += INTERVAL; targetLocalTime += INTERVAL;

			// check for input from the graphics window and respond
			try {
				if(StdDraw.hasNextKeyTyped()) {
					processKeys(StdDraw.nextKeyTyped());
				}
			} catch(Exception e) {
				System.out.println("Could not get keyboard "
					+ "input");
				System.out.println(e);
				System.exit(1);
			}

			// keep avatar in view
			stayInBounds();

			// check for a new comtree and tell Avatar to switch
			int newComtree = readComtree();
			if (newComtree >= 0 && newComtree != comtree) {
				comtree = newComtree;
				sendCommand('c',comtree);
			}
		}
	}

	/**
	 *  Process command line arguments
	 *   - name/address of Avatar host (or localhost when using tunnel)
	 *   - port number used by Avatar (or local tunnel endpoint)
	 *   - walls file
	 */
	private static boolean processArgs(String[] args) {
		try {
			int port = Integer.parseInt(args[1]);
			avaSockAdr = new InetSocketAddress(args[0],port);
			mazeFile = new Scanner(new File(args[2]));
			int y = 0; walls = null;
			while (mazeFile.hasNextLine() && y >= 0) {
				String temp = mazeFile.nextLine();
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
						System.exit(1);
					}
				}
				y--;
			}
			if (y >= 0) {
				System.out.println("Map file incomplete");
				System.exit(1);
			}
		} catch (Exception e) {
			System.out.println("usage: DriveAvatar remoteIp mapfile");
			System.out.println(e);
			System.exit(1);
		}
		return true;
	}
	
	private static BufferedReader inStream = null;
	
	/** Attempt to read a new comtree number from System.in.
	 *  Return the comtree number or -1 if no input is available.
	 */
	private static int readComtree() {
		int comtree = 0;
		try {
			if (inStream == null) {
				inStream = new BufferedReader(
					   new InputStreamReader(System.in));
				System.out.print("\ncomtree=");
			} else if (inStream.ready()) {
				comtree = Integer.parseInt(inStream.readLine());
			} else return -1;
		} catch(Exception e) {
			System.out.println("can't read comtree number" + e);
			System.exit(1);
		}
		System.out.print("\ncomtree=");
		return comtree;
	}

	private static ByteBuffer sendBuf = null;    // used by sendCommand

	/** Send a command to the Avatar.
	 *  Each command consists of a character denoting the command
	 *  and an integer parameter. These are sent over the channel
	 *  to the Avatar.
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
			System.out.println("Can't send command to Avatar"); 
			System.out.println(e);
			System.exit(1); 
		}	
	}

	/** Process input keys.
	 *  This method processes characters typed in the graphics window,
	 *  interpreting them as commands to change the current view
	 *  of the virtual world or change the way the Avatar moves.
	 *
	 *  The keys and their commands are summarized below.
	 *   a	shift current view to the left by one square
	 *   d	shift current view to the right by one square
	 *   s	shift current view down by one square
	 *   w	shift current view up by one square
	 *   o  reduce the view size by one
	 *   p  increase the view size by one
	 *   j  steer the avatar to the left
	 *   l  steer the avatar to the right
	 *   i  speed up the avatar
	 *   k  slow down the avatar
	 *
	 *  There are also upper case versions of the adsw and op
	 *  keys that make larger adjustments to the window.
	 */
	private static void processKeys(char c) throws IOException {
		switch (c) {
		case 'a':
			if (cornerX > 0) {
				cornerX--; 
			}
			break;
		case 'A':
			if (cornerX > 0) {
				cornerX = Math.max(0,cornerX-viewSize);
			}
			break;
		case 'd':
			if (cornerX < worldSize-viewSize) {
				cornerX++; 
			}
			break;
		case 'D':
			if (cornerX < worldSize-viewSize) {
				cornerX = Math.min(cornerX+viewSize,
					      	   worldSize-viewSize);
			}
			break;
		case 's':
			if (cornerY > 0) {
				cornerY--; 
			}
			break;
		case 'S':
			if (cornerY > 0) {
				cornerY = Math.max(0,cornerY-viewSize);
			}
			break;
		case 'w':
			if (cornerY < worldSize-viewSize) {
				cornerY++; 
			}
			break;
		case 'W':
			if (cornerY < worldSize-viewSize) {
				cornerY = Math.min(cornerY+viewSize,
					      	   worldSize-viewSize);
			}
			break;
		case 'o':
			if (viewSize > 1) {
				viewSize--; 
			}
			break;
		case 'O':
			if (viewSize > 1) {
				viewSize /= 2; 
			}
			break;
		case 'p':
			if (viewSize < MAX_VIEW &&
			    viewSize < worldSize &&
			    cornerX + viewSize < worldSize &&
			    cornerY + viewSize < worldSize) {
				viewSize++; 
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
			}
			break;
		case 'j': case 'k': case 'l': case 'i':
			sendCommand(c,0); break;
		}
	}

	private static void stayInBounds() {
		if (avx < cornerX) {
			cornerX = avx;
		} else if (avx >= cornerX + viewSize) {
			cornerX = (avx - viewSize) + 1;
		}
		if (avy < cornerY) {
			cornerY = avy;
		} else if (avy >= cornerY + viewSize) {
			cornerY = (avy - viewSize) + 1;
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
			// assign initial comtree or filter out reports from
			// other comtrees
			if (comtree == 0) comtree = rep.comtree;
			else if (rep.comtree != comtree) {
				rep = getReport(); continue; 
			}
			// track square containing this avatar
			if (rep.type == 1) {
				avx = ((int) rep.x)/GRID;
				avy = ((int) rep.y)/GRID;
			}
			// filter out reports from out of view Avatars
			if (rep.x < cornerX*GRID ||
			    rep.x >= (cornerX + viewSize)*GRID ||
		    	    rep.y < cornerY*GRID ||
			    rep.y >= (cornerY + viewSize)*GRID) {
				rep = getReport(); continue;
			}
			// remap xy coordinates to (0,1) scale
			rep.x = (rep.x - cornerX*GRID)/((double) viewSize*GRID);
			rep.y = (rep.y - cornerY*GRID)/((double) viewSize*GRID);

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

	/** Get the next status report from a buffered status packet.
	 *  Receive additional status packets as needed using nonblocking IO.
	 *  @return the AvatarStatus object for the most recent status report
	 *  or null if there is no report available
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
		rep.x = repBuf.getInt();
		rep.y = repBuf.getInt();
		rep.dir = repBuf.getInt();
		repBuf.getInt(); // discard value
		rep.numVisible = repBuf.getInt();
		rep.numNear = repBuf.getInt();
		rep.comtree = repBuf.getInt();
		rep.type = repBuf.getInt();
		if (repBuf.remaining() <  4*NUM_ITEMS) needData = true;

		recentIds.add(rep.id);
		return rep;
	}
	
	/** Draw grid with a background color of c.
	 */
	private static void drawGrid(Color c) {
		StdDraw.clear(c);
		StdDraw.setPenRadius(.002);
		StdDraw.setPenColor(Color.GRAY);
		double frac = 1.0/viewSize;

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
				if (walls[xy] == 1 || walls[xy] == 3) {
					StdDraw.line(frac*col,frac*row,
						     frac*col,frac*(row+1));
				} 
				if (walls[xy]==2 || walls[xy] == 3) {
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
