/** @file Forest.java 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

package forest.common;

/** Miscellaneous utility functions.
 *  This class defines various constants and common functions useful
 *  within a Forest router and Forest hosts.
 */
public class Forest {
	/** Forest node types.
	 *
	 *  Nodes in a Forest network are assigned specific roles.
	 *  Nodes with node type codes smaller than 100, are considered
	 *  untrusted. All packets received from such hosts are subjected
	 *  to extra checks. For example, they may only send packets with
	 *  a source address equal to their assigned address.
	 */
	public enum NodeTyp {
		UNDEF_NODE(0),
		// untrusted node types
		CLIENT(1),		///< client component
		SERVER(2),		///< server component
		// trusted node types
		TRUSTED(100),		///< numeric separator
		ROUTER(101),		///< router component
		CONTROLLER(102);	///< network control element
	
		private final int value;

		NodeTyp(int value) { this.value = value; };
		public int val() { return value; }
	}
	
	/** Forest packet types.
	 *  This enumeration lists the distinct packet types that are
	 *  currently defined. These are the types that go in the type
	 *  field of the first word of each Forest packet.
	 */
	public enum PktTyp {
		UNDEF_PKT(0),
		// client packet types
		CLIENT_DATA(1),		///< normal data packet from a host
		SUB_UNSUB(2),		///< subscribe to multicast groups
	
		CLIENT_SIG(10),		///< client signalling packet
	
		CONNECT(11),		///< connect an access link
		DISCONNECT(12),		///< disconnect an access link
	
		// internal control packet types
		NET_SIG(100),		///< network signalling packet
		RTE_REPLY(101),		///< for unicast route learning
	
		// router internal types
		RTR_CTL(200),
		VOQSTATUS(201);

		private final int value;

		PktTyp(int value) { this.value = value; };
		public int val() { return value; }
	}

	public static final byte FOREST_VERSION = 1; ///< forest proto version
        public static final int HDR_LENG = 20;       ///< header len in bytes
        public static final int OVERHEAD = 24;       ///< total overhead
        public static final short RTE_REQ = 0x01;    ///< route request flag
        public static final int ROUTER_PORT = 30123; ///< port # used by routers
        public static final int NM_PORT = 30122;     ///< port # used by netMgr

        // router implementation parameters
        public static final short MAXINTF= 20;    ///< max # of interfaces
        public static final short MAXLNK = 1000;  ///< max # of links per router
        public static final int MINBITRATE = 20;  ///< min link bit rate in Kb/s
        public static final int MAXBITRATE = 900000; ///< max bit rate in Kb/s
        public static final int MINPKTRATE = 10;     ///< min packet rate in p/s
        public static final int MAXPKTRATE = 450000; ///< max packet rate in p/s
        public static final int BUF_SIZ = 1600; ///< size of packet buffer

        // comtrees used for control
        public static final int CLIENT_CON_COMT = 1; ///< for connect packets
        public static final int CLIENT_SIG_COMT = 2; ///< comtree signaling
        public static final int NET_SIG_COMT = 100;  ///< internal signaling

	public class PktBuffer {
		private int[] buf;
		public PktBuffer() { buf = new int[BUF_SIZ/4]; }
		public int get(int i) { return buf[i]; }
		public void set(int i, int v) { buf[i] = v; }
	}
	
	/** Determine if given Forest address is a valid unicast address.
	 *  @param adr is a Forest address
	 *  @return true if it is a valid unicast address (is greater than
	 *  zero and both the zip code and local part of the address are >0)
	 */
	public static boolean validUcastAdr(int adr) {
		return adr > 0 && zipCode(adr) != 0 && localAdr(adr) != 0;
	}
	
	/** Determine if given Forest address is a valid multicast address.
	 *  @param adr is a Forest address
	 *  @return true if it is a valid multicast address (is <0)
	 */
	public static boolean mcastAdr(int adr) { return adr < 0; }
	
	/** Get the zip code of a unicast address.
	 *  Assumes that the address is valid.
	 *  @param adr is a Forest address
	 *  @return the zip code part of the address
	 */
	public static int zipCode(int adr) { return (adr >> 16) & 0x7fff; }
	
	/** Get the local address part of a unicast address.
	 *  Assumes that the address is valid.
	 *  @param adr is a Forest address
	 *  @return the local address part of the address
	 */
	public static int localAdr(int adr) { return adr & 0xffff; }
	
	/** Construct a forest address from a zip code and local address.
	 *  Assumes that both arguments are >0.
	 *  @param zip is the zip code part of the address
	 *  @param local is the local address part
	 *  @return the corresponding unicast address
	 */
	public static int forestAdr(int zip, int local ) {
		return ((zip & 0xffff) << 16) | (local & 0xffff);
	}
	
	/** Construct a forest address from a string.
	 *  
	 *  A String representing a negative number is interpreted as a
	 *  multicast address. Otherwise, we expect a unicast address
	 *  with the form zip_code.local_addr.
	 *
	 *  @param fas is the forest address String to be converted
	 *  @return the corresponding forest address, or 0 if the input
	 *  is not a valid address
	 */
	public static int forestAdr(String fas) {
                String parts[] = fas.split("[.]");
                if (parts.length != 2) return 0;
                int zip = Integer.parseInt(parts[0]);
                int loc = Integer.parseInt(parts[1]);
                return forestAdr(zip,loc);
        }

	/** Create a String representation of a forest address.
	 *  @param fAdr is a forest address
	 *  @return a String that represents fAdr
	 */
	public static String fAdr2string(int fAdr) {
		if (fAdr < 0) return "" + fAdr;
		return "" + zipCode(fAdr) + "." + localAdr(fAdr);
	}
	
	/** Compute link packet length for a given forest packet length.
	 *
	 *  @param x is the number of bytes in the Forest packet
	 *  @return the number of bytes sent on the link, including the
	 *  IP/UDP header and a presumed Ethernet header plus inter-frame gap.
	 */
	public static int truPktLeng(int x) { return 70+x; }
	
	public static String nodeTyp2string(NodeTyp nt) {
		String s = "";
		     if (nt == NodeTyp.CLIENT)     s = "client";
		else if (nt == NodeTyp.SERVER)     s = "server";
		else if (nt == NodeTyp.ROUTER)     s = "router";
		else if (nt == NodeTyp.CONTROLLER) s = "controller";
		else s = "unknown node type";
		return s;
	}
	
	public static NodeTyp string2nodeTyp(String s) {
		if (s.equals("client"))     return NodeTyp.CLIENT;
		if (s.equals("server"))     return NodeTyp.SERVER;
		if (s.equals("router"))     return NodeTyp.ROUTER;
		if (s.equals("controller")) return NodeTyp.CONTROLLER;
					    return NodeTyp.UNDEF_NODE;
	}
}
