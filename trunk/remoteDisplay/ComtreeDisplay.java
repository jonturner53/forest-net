package remoteDisplay;

/** @file ComtreeDisplay.java */

import java.util.*;
import java.util.regex.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.awt.Color;
import java.awt.geom.Point2D;
import princeton.*;

/**
* A GUI for viewing comtrees based on the packets recieved from the ComtreeController.
* Moving between comtrees, each "router" displays the number of avatars subscribing to that router.
* A thick line around a circle means a root of a comtree
* Blue links are links for that particular comtree.
* Grey circles represent cores of the comtree. White circles represent normal nodes.
* White squares represent the controller programs.
*/
public class ComtreeDisplay{
	
	private static final String nodes = "nodes"; ///<name of nodes section from topology file
	private static final String interfaces = "interfaces";  ///<name of interfaces section from topology file
	private static final String controller = "controller"; ///<name of controller section from topology file
	private static final String links = "links";  ///<name of links section from topology file
	private static final String comtrees = "comtrees";  ///<name of comtrees section from topology file
	private static final int SIZE = 700; ///<size of gui frame
	private static final int FACTOR = 5; ///<fraction of screen covered by StdDraw
	private static final double PEN_RADIUS = .003; ///<default stddraw line thickness
	private static final int COM_PORT = 30133; ///<port ComtreeDisplay listens to for incoming packets from ComtreeController
	private static ByteBuffer repBuf = null;        ///< buffer for report packets
	private static BufferedReader inStream = null; ///<write packets to a file for debugging
	private static SocketChannel comChan = null; ///<socket for network communication with the ComtreeController
	private static InetSocketAddress comSockAdr; ///<address for the comChan
	private static TreeMap<Pair, Router> routers; ///<list of Router objects
	private static TreeMap<Pair, Link> lnks; ///<list of Link objects
	private static TreeMap<String, Comtree> ct; ///<list of Comtree objects
	private static ArrayList<NetObj> topology; ///<data representation of topology file

	private static boolean needData = true; ///<data buffer needs data from byte stream
	
	/**
	* Sole constructor
	*/
	public ComtreeDisplay(){
		topology = new ArrayList<NetObj>();
		routers = new TreeMap<Pair, Router>();
		lnks = new TreeMap<Pair, Link>();
		ct = new TreeMap<String, Comtree>();
	}
	
	/**
         * Get the next status report from a buffered status packet.
         * Receive additional status packets as needed using nonblocking IO.
         * @return the AvatarStatus object for the most recent status report
         * or null if there is no report available
         */
        private static int[] getReport() {
                int[] packet = new int[4];
		int size = packet.length*4;
		if (repBuf == null) {
                        repBuf = ByteBuffer.allocate(size);
                        repBuf.clear();
                }

                if (needData) {
                        if (repBuf.position() == repBuf.limit())
                                repBuf.clear();
                        while (needData) {
                                try { 
					 if (comChan.read(repBuf) == 0)
                                                return null; 
                                        if (repBuf.position() >= size) {
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
		packet[0] = repBuf.getInt(); //comtree
		packet[1] = repBuf.getInt(); //router
		packet[2] = repBuf.getInt(); //numClients
		packet[3] = repBuf.getInt(); //now
		if (repBuf.remaining() < size) needData = true;
                return packet;
        }

	/**
	* @param args - command line arguments for the ComtreeDisplay
	* "localhost" is the only required member of args
	* @return boolean - successful comSockAdr openning on COM_PORT
	*/
	private static boolean processArgs(String[] args) {
                try {
                        comSockAdr = new InetSocketAddress(args[0], COM_PORT);
                } catch (Exception e) {
                        System.out.println("usage: ComtreeDisplayNet comIp");
                        System.out.println(e);
                        System.exit(1);
                }
                return true;
        }
	
	/**
         * Attempt to read a new comtree number from System.in.
         * Return the comtree number or -1 if no input is available.
         */
        private static String readComtree() {
                String comtree = "";
                try {
                        if (inStream == null) {
                                inStream = new BufferedReader(
                                                new InputStreamReader(System.in));
                                System.out.print("\ncomtree=");
				String str = inStream.readLine();
				while(!validate(str))
				{
					System.out.print("\nInvalid Input\ncomtree=");
                                	str = inStream.readLine();
				}
                                comtree = str;
                        } else if (inStream.ready()) {
                        	String str = inStream.readLine();
                                while(!validate(str))
                                {
                                        System.out.print("\nInvalid Input\ncomtree=");
                                        str = inStream.readLine();
                                }
                                comtree = str;
			} else return "-1";
                } catch(Exception e) {
                        System.out.println("input error: type comtree number\n" + e);
                }
                System.out.print("\ncomtree=");
                return comtree;
        }
	
	/**
	* validates a comtree number aginst the list of comtrees liste in the Forest topology file
	* @param s - user inputted comtree number
	* @return boolean - s is a name of one of the comtrees in the topology file
	*/
	private static boolean validate(String s){
		if(s.equals(""))
			return false;
		for(NetObj n: topology)
			if(n instanceof Comtree && n.name.equals(s))
				return true;
		return false;
	}
	
	/**
	* @param path - full path name to the topology file
	* parses a custom topology file into topology<NetObj> data structure.
	* The resulting structure is a two-dimensional ArrayList< ArrayList<NetObj> > that based on the header in the topology file, builds a router, link, controller etc and represents them all as NetObjs
	*/
	public static void parse(String path) throws FileNotFoundException{
		Scanner in = new Scanner(new FileInputStream(path));
		ArrayList<ArrayList<String>> topology_raw = new ArrayList<ArrayList<String>>();
		ArrayList<String> builder = new ArrayList<String>();
		while(in.hasNextLine()){	
			//filters comments
			String s = in.nextLine().split("#")[0];
			if(!s.isEmpty()){
				if(s.trim().equalsIgnoreCase(".")){
					topology_raw.add(builder);
					builder = new ArrayList<String>();
				}
				else
					builder.add(s.trim());
			}
		}
		
		//build routers into data structure
		int count = 0;
		String[] temp = new String[4];
		Pattern p = Pattern.compile("[\\s]+");// Split input with the pattern excluding all whitespaces
		for(int i = 0; i < topology_raw.size(); i++)
			for(int j = 1; j < topology_raw.get(i).size(); j++){
				String[] s = p.split(topology_raw.get(i).get(j));
				String header = topology_raw.get(i).get(0).trim().replace(":", "");
				
				if(header.equalsIgnoreCase(nodes))
					topology.add(new Router(s[0], s[1], s[2], s[3], s[4]));
				else if(header.equalsIgnoreCase(interfaces))
					topology.add(new Interface(s[0], s[1], s[2], s[3]));
				else if(header.equalsIgnoreCase(controller))
					topology.add(new Controller(s[0], s[1], s[2], s[3], s[4], s[5]));
				else if(header.equalsIgnoreCase(links))
					topology.add(new Link(s[0], s[1], s[2]));
				else if(header.equalsIgnoreCase(comtrees)){
					temp[count%4] = s[0];
					if(count%4 == 3){
						Comtree c = new Comtree(temp[0], temp[1], temp[2], temp[3]);
						topology.add(c);	
						ct.put(c.name, c);
					}
					count++;
				}
			}
	}
	
	/**
	* @return String representation of the topology<NetObj> array based on NetObj's toString().
	*/
	public String toString(){
		StringBuilder sb = new StringBuilder();
		for(NetObj n: topology)
			sb.append(n + "\n");
		return sb.toString();
	}
	
	/**
	* Main method that opens all TCP connections to the ComtreeController and then parses a local copy of the Forest topology file. After painting a graphical representation of the master topology data structure, the socket listener listens for incoming bit streams from the ComtreeController. Contained in each stream is as follows:
	{Comtree number, router number, number of avatars subscribed, timestamp}The first three values are used to find the appropriate comtree and router and update that router's number of avatars subscribed. The timestamp is ignored and only used for debugging purposes. During runtime the comtree on screen can be changed by a commandline argument and the values are updated as new information arrives from the comtree controller pretaining to the specified comtree.
	*/
	public static void main(String[] args){
		processArgs(args);
		ComtreeDisplay com = new ComtreeDisplay();
		try {
			parse("topology");
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}
	
		// Open channel to monitor and make it nonblocking
                try {
                        comChan = SocketChannel.open(comSockAdr);
                        comChan.configureBlocking(false);
                } catch (Exception e) {
                        System.out.println("Can't open channel to monitor");
                        System.out.println(e);
                        System.exit(1);
                }
	
		System.out.println("Avalible Comtrees: " + ct.keySet());
		
		String comtree = readComtree();
		//Populate HashMaps
		Pair key = null;
		for(String num: ct.keySet()){
			for(int j = 0; j < ct.get(num).lnks.size(); j++){
				String pair = ct.get(num).lnks.get(j);
				for(NetObj n: topology){
						if(n instanceof Router){
							Router r = ((Router) n);
							if(r.getZip().charAt(0) == pair.charAt(0)){
								key = new Pair(num, r.getZip());
					//			System.out.println("Adding new router: " + key+ " " + r);
								routers.put(key, r);
					//			System.out.println("map: " + routers);
							}
						}
						else if(n instanceof Link){
							Link lnk = ((Link) n);

							if(pair.equals(lnk.getPair()[0]))
								key = new Pair(num, lnk.getPair()[0]);
							else if(pair.equals(lnk.getPair()[1]))
								 key = new Pair(num, lnk.getPair()[1]);
					//		System.out.println("Adding new link: " + key+" "+ lnk);
							lnks.put(key, lnk);
					//		System.out.println("map: " + lnks);
						}
				}
			}	
		}
		int numObj = 0;
		StdDraw.setCanvasSize(SIZE, SIZE);
		StdDraw.setPenRadius(PEN_RADIUS);
                Color c = new Color(210,230,255);
		StdDraw.clear(c);
		double minX = -1;
		double minY = -1;
		double maxX = -1;
		double maxY = -1;
		//find scale
		for(NetObj n: topology){
			if(n instanceof Router || n instanceof Controller){
			numObj++;
			double x = n.x;
                        double y = n.y;
                        	if(x < minX || minX == -1)
                                	minX = x;
                        	if(x > maxX)
                                	maxX = x;
                        	if(y < minY || minY == -1)
                                	minY = y;
                        	if(y > maxY)
                         	       maxY = y;
			}
		}
		double dx = maxX-minX;
		double dy = maxY-minY;
		int radius = ((int) Math.sqrt(SIZE/(numObj*FACTOR*Math.PI)));	
		StdDraw.setXscale(minX-radius, maxX+radius);
		StdDraw.setYscale(minY-radius, maxY+radius);
		boolean linksPainted = false;
		boolean routersPainted = false;
		boolean numPainted = false;

		while(true){

			int[] pkt = getReport();
			try {
 				BufferedWriter out = new BufferedWriter(new FileWriter("output.txt", true));
				if(pkt != null)
					out.write(pkt[0] + " " + pkt[1] + " " + pkt[2] + "\n");
				out.close();
			} catch (IOException e) {System.out.println("FileIOException: output.txt");}
			
			Collections.sort(topology);
			for(NetObj n: topology){
				if(n instanceof Router && !routersPainted){
					for(Pair p: routers.keySet()){
						Router r = routers.get(p);
						if(r != null){
						if(ct.get(comtree).src.contains(r.getZip()))
							StdDraw.setPenColor(Color.LIGHT_GRAY);
						else
							StdDraw.setPenColor(Color.WHITE);
						StdDraw.filledCircle(r.x, r.y, radius);
						StdDraw.setPenColor(Color.BLACK);
						StdDraw.setPenRadius(PEN_RADIUS);
						StdDraw.circle(r.x, r.y, radius);
						StdDraw.text(r.x, r.y, r.name);
						
						if(pkt!=null){
							if(pkt[0] == Integer.parseInt(comtree) && pkt[1]==Integer.parseInt(r.getZip()))
								r.setNumClients(pkt[2]);
						}

						StdDraw.text(r.x, r.y-radius/2, Integer.toString(r.getNumClients()));
						}
					}
					routersPainted = true;
				}else if(n instanceof Controller){
				//	System.out.println(routers.keySet());	
				//	for(Pair p: routers.keySet())
				//		System.out.println(routers.get(p));
				//	System.out.println("controller comtree fadr: " + comtree + " " + n.forestAdr);
					Router parent = routers.get(new Pair(comtree, ((Controller)n).getZipCode()));
				//	System.out.println(parent);
					StdDraw.setPenRadius(PEN_RADIUS*2);
					StdDraw.setPenColor(Color.BLACK);
					StdDraw.line(parent.x, parent.y, n.x, n.y);
					StdDraw.setPenColor(Color.WHITE);
					StdDraw.filledSquare(n.x, n.y, radius);
					StdDraw.setPenColor(Color.BLACK);
                                	StdDraw.setPenRadius(PEN_RADIUS);
					StdDraw.square(n.x, n.y, radius);
					StdDraw.text(n.x, n.y, n.name);
				}else if(n instanceof Comtree){
					Comtree ct = ((Comtree) n);	
					//designate the root
					if(ct.name.equals(comtree)){
						StdDraw.setPenColor(Color.BLACK);
						Pattern pat = Pattern.compile("\\d+");
						Matcher m = pat.matcher(ct.root);
						m.find();
						Router rt = null;
						for(Pair rtr: routers.keySet()){
							Router next = routers.get(rtr);
							if(next.getZip().equals(m.group())){
								rt = next;
								break;
							}
						}
						StdDraw.setPenRadius(PEN_RADIUS*2);
						StdDraw.circle(rt.x, rt.y, radius);
					}
				}else if(n instanceof Link && !linksPainted){
					Point2D A = null;
                			Point2D B = null;
					for(Pair p: lnks.keySet()){
					 	Link l = lnks.get(p);
					//	System.out.println("Link: " + l.getPair()[0] + " " + l.getPair()[1]);
						Router r1 = null;
						Router r2 = null;
						for(Pair rtr: routers.keySet()){
							Router next = routers.get(rtr);	
							if(next.getForestAdr(l.getZip()[0]) != null)
								r1 = next;
							else if(next.getForestAdr(l.getZip()[1]) != null)
								r2 = next;
						}
						if(r1!=null && r2 != null){
				        		A = new Point2D.Double(r1.x, r1.y);
					                B = new Point2D.Double(r2.x, r2.y);
					                l.setPoints(A, B);
						}
					//	System.out.println(ct.get(comtree).lnks);
						ArrayList ln = ct.get(comtree).lnks;
						if(ln.contains(l.getPair()[0]) || ln.contains(l.getPair()[1])){
							StdDraw.setPenRadius(PEN_RADIUS*4);
							StdDraw.setPenColor(Color.BLUE);
						}else{
							StdDraw.setPenRadius(PEN_RADIUS*2);
							StdDraw.setPenColor(Color.BLACK);
						}
						StdDraw.line(l.A.getX(), l.A.getY(), l.B.getX(), l.B.getY());
					}
					linksPainted = true;
                  		}else if(!numPainted){
					StdDraw.setPenRadius(PEN_RADIUS);
					StdDraw.setPenColor(Color.BLACK);
					String s = "comtree: " + comtree;
					StdDraw.text(maxX - 5, minY, s);
					numPainted = true;
				}
			}
			routersPainted = false;
			linksPainted = false;
			String temp =readComtree();
			if(!temp.equals("-1") && !comtree.equals(temp)){
				comtree = temp;
				numPainted = false;
				StdDraw.clear(c);
			}
			StdDraw.show(50);
		}
	}
}
