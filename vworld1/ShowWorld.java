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
	private static Map<Integer, AvatarGraphic> status; // avatar status
	private static final int INTERVAL = 50;	// time between updates (in ms)
	private static int comtree;	// number of comtree to be monitored
	private static int SIZE;	// size of full map
	private static int gridSize;	// # of squares in map (x or y)
	private static int viewSize;	// # of map squares viewed at one time
	private static int cornerX = 0; //x and y of the bottom left of the viewing window
	private static int cornerY = 0;
	private static final int MON_PORT = 30124; // port # used by monitor
	private static Scanner mazeFile; // filename of maze
	private static int[] walls; 	// list of walls
	private static int[] currWalls; // list of walls currently in view
	private static SocketChannel chan = null;// channel to remote monitor
						// or avatar
	private static ByteBuffer repBuf = null; // buffer for report packets
	private static AvatarStatus rep = null;	// most recent report
	private static final int GRID = 10000;	// size of one grid space
	private static boolean needData = true;	// true when getReport() needs
						// more data to process
	
	private static boolean avaConn = false;	// true if connecting to avatar
						// false if monitor
	private static Set<Integer> recentIds;	// list of recently seen ids
	private static int idCounter;		// counter used to clear old ids	
	/**
	 * Get the next status report from a buffered status packet.
	 * Receive additional status packets as needed using nonblocking IO.
	 * @return the AvatarStatus object for the most recent status report
	 * or null if there is no report available
	 */
	private static AvatarStatus getReport() {
		if (repBuf == null) {
			repBuf = ByteBuffer.allocate(1000*(avaConn ? 40 :36));
			repBuf.clear();
		}
		
		if (needData) {
			if (repBuf.position() == repBuf.limit())
				repBuf.clear();
			while (needData) {
				try {					
					if (chan.read(repBuf) == 0)
						return null;
					if (repBuf.position() >=
					     (avaConn ? 40 : 36)) {
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
		rep.x = (repBuf.getInt()-cornerX*GRID)/(double)SIZE;
		rep.y = (repBuf.getInt()-cornerY*GRID)/(double)SIZE;
		rep.dir = repBuf.getInt();
		repBuf.getInt(); // discard value
		rep.numVisible = repBuf.getInt();
		rep.numNear = repBuf.getInt();
		rep.comtree = repBuf.getInt();
		if (avaConn) {
			if (comtree == 0) comtree = rep.comtree;
			rep.type = repBuf.getInt();
		}
		if (repBuf.remaining() < (avaConn ? 40 : 36)) needData = true;

		recentIds.add(rep.id);
		return rep;
	}

	private static AvatarStatus lastRep = null; // used by processFrame
	private static void viewMovements(int c) throws IOException {
		if(avaConn) return;
		ByteBuffer buff = ByteBuffer.allocate(8);
		boolean didMove = true;
		switch(c) {
			case 97:
				buff.putLong(1L<<32);
				if(cornerX!=0) cornerX--;
				else didMove = false;
				break;
			case 119:
				buff.putLong(2L<<32);
				if(cornerY!=gridSize-viewSize) cornerY++;
				else didMove = false;
				break;
			case 100:
				buff.putLong(3L<<32);
				if(cornerX!=gridSize-viewSize) cornerX++;
				else didMove = false;
				break;
			case 115:
				buff.putLong(4L<<32);
				if(cornerY!=0) cornerY--;
				else didMove = false;
				break;
			case 111:
				buff.putLong(5L<<32);
				if(viewSize!=1) viewSize--;
				else didMove = false;
				break;
			case 112:
				buff.putLong(6L<<32);
				if(viewSize!=gridSize) viewSize++;
				else didMove = false;
				break;
		}
		if(didMove) {
			buff.flip();
			chan.write(buff);
			updateCurrWalls();
			chan.socket().getOutputStream().flush();
		}
	}
	private static void updateCurrWalls() {
		if(cornerX+viewSize>gridSize) cornerX=gridSize-viewSize;
		if(cornerY+viewSize>gridSize) cornerY=gridSize-viewSize;
		SIZE = viewSize*GRID;
		currWalls = new int[viewSize*viewSize];
		for(int i = 0; i < viewSize; i++) {
			for(int j = 0; j < viewSize; j++) {
				currWalls[i*viewSize+j] =
				  walls[(i+cornerY)*gridSize+j+cornerX];
			}
		}
	}
	private static void sendMovements(int c) throws IOException {
		if (!avaConn) return;
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
		chan.write(buff);
		chan.socket().getOutputStream().flush();
	}
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
			System.out.println("input error: type comtree number\n"
					    + e);
		}
		System.out.print("\ncomtree=");
		return comtree;
	}
	
	private static InetSocketAddress monSockAdr;
	private static InetSocketAddress avaSockAdr;
	/**
	 *  Process command line arguments
	 *   - name/address of monitor host (or localhost)
	 *   - walls file
	 *   - size of view
	 *   - port # when connecting to avatar (rather than monitor)
	 */
	private static boolean processArgs(String[] args) {
		try {
			mazeFile = new Scanner(new File(args[1]));
			viewSize = Integer.parseInt(args[2]);
			if(args.length > 3) {
				avaConn = true;
				avaSockAdr = new InetSocketAddress(args[0],
					Integer.valueOf(args[3]));
			} else {
				monSockAdr = new InetSocketAddress(args[0],
					    	     MON_PORT);
			}
			int counter = 1;
			while (mazeFile.hasNextLine()) {
				String temp = mazeFile.nextLine();
				if (walls==null) {
					gridSize = temp.length();
					SIZE = viewSize*GRID;
					walls = new int[gridSize*gridSize];
				}
				currWalls = new int[viewSize*viewSize];
				for(int i = 0; i < gridSize; i++) {
					if(temp.charAt(i)=='+') {
						walls[(gridSize-counter)
							*gridSize+i] = 3;
					} else if(temp.charAt(i)=='-') {
						walls[(gridSize-counter)
							*gridSize+i] = 2;
					} else if(temp.charAt(i)=='|') {
						walls[(gridSize-counter)
							*gridSize+i] = 1;
					} else if(temp.charAt(i)==' ') {
						walls[(gridSize-counter)
							*gridSize+i] = 0;
					} else {
						System.out.println(
						  "Unrecognized symbol in map "
					  	  + "file!");
					}
				}
				counter++;
			}
			updateCurrWalls();
		} catch (Exception e) {
			System.out.println("usage: ShowWorldNet remoteIp "
					    + "wallfile [port]");
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
		if (sendBuf == null) sendBuf = ByteBuffer.allocate(8); 	
		//  send comtree # to remote monitor
		try { 		
			sendBuf.putLong((long)comtree);
			sendBuf.flip();
			chan.write(sendBuf);
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
		StdDraw.setPenRadius(.002);
		StdDraw.setPenColor(Color.GRAY);
		double frac = 1.0/viewSize;
		for (int i = 0; i <= viewSize; i++) {
			StdDraw.line(0,frac*i,1,frac*i);
			StdDraw.line(frac*i,0,frac*i,1);
		}
		StdDraw.setPenColor(Color.BLACK);
		StdDraw.setPenRadius(.006);
		//StdDraw.line(0,0,0,1); StdDraw.line(0,0,1,0);
		//StdDraw.line(1,1,0,1); StdDraw.line(1,1,1,0);
		for(int i = 0; i < viewSize*viewSize; i++) {
			int row = i/viewSize; int col = i%viewSize;
			if(currWalls[i] == 1 || currWalls[i] == 3) {
				StdDraw.line(frac*col,frac*row,
					     frac*col,frac*(row+1));
			} 
			if(currWalls[i]==2 || currWalls[i] == 3) {
				StdDraw.line(frac*col,     frac*(row+1),
					     frac*(col+1), frac*(row+1));
			}
		}
		StdDraw.setPenRadius(.003);
		StdDraw.text(.08, -.02, "comtree: " + comtree);
	}
	
	/**
	 * The ShowWorldNet program displays a visualization of a very
	 * simple virtual world. Avatars moving around in the virtual
	 * world are displayed as they move.
	 *
	 *  Command line arguments
	 *   - name/address of monitor host (or localhost)
	 *   - walls file
	 *   - size of view
	 *   - port # when connecting to avatar (rather than monitor)
	 */
	public static void main(String[] args) {	
		// process command line arguments	
		processArgs(args);	
		// Open channel to monitor and make it nonblocking
		try {
			if (avaConn) {
				chan = SocketChannel.open(avaSockAdr);
			} else {
				chan = SocketChannel.open(monSockAdr);
			}
			chan.configureBlocking(false);
		} catch (Exception e) {
			System.out.println("Can't open channel to "
					    + "monitor/avatar");
			System.out.println(e);
			System.exit(1);
		}
		//send viewsize to Monitor.cpp
		ByteBuffer buff = ByteBuffer.allocate(8);
		buff.putLong(7L<<32|viewSize);
		buff.flip();
		try {
			chan.write(buff);
		}catch(Exception e) {
			System.out.println("Could not"+
			" write viewSize to Monitor.cpp");
			System.exit(0);
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
		if (!avaConn) {
			comtree = readComtree();
			sendComtree(comtree);
		}
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
			//StdDraw.show(INTERVAL);
			if (!avaConn) {
				try {
					if(StdDraw.hasNextKeyTyped()) {
						viewMovements(StdDraw.nextKeyTyped());
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
					sendComtree(comtree);
				}
			} else {
				//write movement commands to avaChan socket
	                        try {
	                                if (StdDraw.hasNextKeyPressed()) {
	           				int nextKey = StdDraw.nextKeyPressed();
						sendMovements(nextKey);
					}
					
	                        } catch(Exception e) {
	                                System.out.println("Could not send "
						+ "movement command to Avatar");
	                                System.out.println(e);
	                                System.exit(1);
	                        }
			}
		}
	}
}
