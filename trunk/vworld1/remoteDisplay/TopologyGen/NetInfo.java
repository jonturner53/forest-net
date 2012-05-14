import java.io.*;
import java.util.*;

public class NetInfo{
	private RtrNodeInfo[] rtr;
	private LeafNodeInfo[] leaf;
	private LinkInfo[] link;
	private TreeMap<Integer, RtrNodeInfo> routers;
	private TreeMap<Integer, LeafNodeInfo> controllers;
	int maxRtr;		///< max node number for a router;
				///< leaf nodes all have larger node numbers
	int maxNode;		///< max node number in netTopo graph
	int maxLink;		///< max link number in netTopo graph
	int maxLeaf;		///< max number of leafs
	int maxCtl;		///< maximum number of controllers
	
	// Conprivate classor for NetInfo, allocates space and initializes private data
	NetInfo(int maxNode1, int maxLink1, int maxRtr1, int maxCtl1){
		maxNode = maxNode1;
		maxLink = maxLink1;
		maxRtr = maxRtr1;
		maxCtl = maxCtl1;

		maxLeaf = maxNode-maxRtr;
		leaf = new LeafNodeInfo[maxLeaf+1];
		rtr = new RtrNodeInfo[maxRtr+1];
		link = new LinkInfo[maxLink+1];
		
		routers = new TreeMap<Integer, RtrNodeInfo>();
		controllers = new TreeMap<Integer, LeafNodeInfo>();
	}

	private class IfInfo {
		String	ipAdr;		///< ip address of forest interface
		int	bitRate;	///< max bit rate for interface (Kb/s)
		int	pktRate;	///< max pkt rate (packets/s)
		int	firstLink;	///< number of first link assigned to interface
		int	lastLink;	///< number of last link assigned to interface
		
		public IfInfo(){}
	}

	/** private classure holding information used by leaf nodes */
	private class LeafNodeInfo implements Comparable{
		String	name;		///< leaf name
		String	nType;		///< leaf type
		String	ipAdr;		///< IP address used to initialize node
		int	fAdr;		///< leaf's forest address
		int	latitude;	///< latitude of leaf (in micro-degrees, + or -)
		int	longitude;	///< latitude of leaf (in micro-degrees, + or -)
		int	bitRate;	///< max bit rate for interface (Kb/s)
		int	pktRate;	///< max pkt rate (packets/s)
		
		public LeafNodeInfo(){}

		@Override
		public int compareTo(Object obj){
			LeafNodeInfo other = (LeafNodeInfo) obj;
			return ((Integer) fAdr).compareTo(((Integer) other.fAdr));
		}
	}
	
	private final int UNDEF_LAT = 91;	// invalid latitude
	private final int UNDEF_LONG = 361;	// invalid longitude

	/** private classure holding information used by all nodes */
	private class RtrNodeInfo implements Comparable{
		String		name;		///< node name
		String		nType;		///< node type
		int		fAdr;		///< node's forest address
		int		latitude;	///< latitude of node (in micro-degrees, + or -)
		int		longitude;	///< latitude of node (in micro-degrees, + or -)
		int		firstCliAdr;	///< router's first assignable forest address
		int 		lastCliAdr;	///< router's last assignable forest address
		int	 	numIf;		///< number of interfaces
		IfInfo[] 	iface;		///< interface information
		
		public RtrNodeInfo(){}
	
		@Override
		public int compareTo(Object obj){
			LeafNodeInfo other = (LeafNodeInfo) obj;
			return ((Integer) fAdr).compareTo(((Integer) other.fAdr));
		}

	}

	private class LinkInfo {
		int	leftLnum;	///< local link number used by "left endpoint"
		int	rightLnum;	///< local link number used by "right endpoint"
		int	bitRate;	///< max bit rate for link
		int	pktRate;	///< max packet rate for link
		
		public LinkInfo(){}
	}

	private boolean isRouter(int n){
		return (1 <= n && n <= maxRtr && rtr[n].fAdr != 0);
	}

	private boolean isLeaf(int n){
		return (maxRtr < n && n <= maxNode && leaf[n-maxRtr].fAdr != 0);
	}

	private double getNodeLat(int n){
		double x = (isLeaf(n) ? leaf[n-maxRtr].latitude :
			(isRouter(n) ? rtr[n].latitude : UNDEF_LAT));
		return x/1000000;
	}

	private double getNodeLong(int n){
		double x = (isLeaf(n) ? leaf[n-maxRtr].longitude :
		    (isRouter(n) ? rtr[n].longitude : UNDEF_LONG));
		return x/1000000;
	}

	
	public void write(String path) {
		try{
			FileWriter fs = new FileWriter(path);
			BufferedWriter out = new BufferedWriter(fs);
			out.write( "Routers\n\n");
			Iterator<RtrNodeInfo> itr = routers.values().iterator();
			while (itr.hasNext()) {
				RtrNodeInfo rni = itr.next();
				String rName = rni.name;	///< node name
				String nType = rni.nType;	///< node type
				int fAdr = rni.fAdr;		///< node's forest address
				
				out.write( "name=" + rName + " "+ "nodeType=" + nType + " fAdr="+fAdr);
				out.write( "\n\tlocation=(" + rni.latitude + ","
						     + rni.longitude + ") "
				    + "CliAdrRange=(" + rni.firstCliAdr+"-"+rni.lastCliAdr+")\n");
				out.write( "interfaces\n"
				    + "# iface#   ipAdr  linkRange  bitRate  pktRate\n");
				for (int i = 1; i <= rni.iface.length; i++) {
					if(rni.iface[i] != null){
						out.write( "   " + i + "  ");
						if (rni.iface[i].firstLink == rni.iface[i].lastLink) 
							out.write(" " + rni.iface[i].firstLink + " ");
						else
							out.write(" " + rni.iface[i].firstLink + "-"
							    + rni.iface[i].lastLink + "  ");
						out.write(rni.iface[i].bitRate + "  "
						    + rni.iface[i].pktRate + ";\n");
					}
				}
				out.write( "end\n;\n");
			}
			out.write( ";\n\n");
			out.write( "LeafNodes\n\n");
	/*
			list<int>::iterator cp = controllers->begin();
			// print controllers first
			while (cp != controllers->end()) {
				int c = *cp; cp++;
				ntyp_t nt = getNodeType(c);
				string ntString; Forest::addNodeType2string(ntString, nt);
				out.write( "name=" + getNodeName(c) + " "
				    + "nodeType=" + ntString + " "
				    + "ipAdr=";  Np4d::writeIpAdr(out,getLeafIpAdr(c));
				out  + " fAdr="; Forest::writeForestAdr(out,getNodeAdr(c));
				out.write( "\n\tlocation=(" + getNodeLat(c) + ","
						     + getNodeLong(c) + ");\n";
			}
			// then any other leaf nodes
			for (int c = maxRtr+1; c <= netTopo->n(); c++) {
				if (!isLeaf(c) || getNodeType(c) == CONTROLLER)
				ntyp_t nt = getNodeType(c);
				string ntString; Forest::addNodeType2string(ntString, nt);
				out.write( "name=" + getNodeName(c) + " "
				    + "nodeType=" + ntString + " "
				    + "ipAdr=";  Np4d::writeIpAdr(out,getLeafIpAdr(c));
				out.write("fAdr="; Forest::writeForestAdr(out,getNodeAdr(c)));
				out.write("\n\tlocation=(" + getNodeLat(c) + ","
						     + getNodeLong(c) + ");\n");
			}
			out.write(";\n\n");
			out.write("Links\n\n");
			for (int lnk = 1; lnk <= netTopo->m(); lnk++) {
				if (!validLink(lnk)) continue;
				int ln = getLnkL(lnk); int rn = getLnkR(lnk);
				out.write( "link="+ getNodeName(ln));
				if (isRouter(ln)) out.write( "." + getLocLnkL(ln);
				out.write( "," + getNodeName(rn));
				if (isRouter(rn)) out.write( "." + getLocLnkR(rn);
				out.write(" "
				    + "bitRate=" + getLinkBitRate(lnk) + " "
				    + "pktRate=" + getLinkPktRate(lnk) + " "
				    + "length=" + getLinkLength(lnk) + ";\n");
			}
			out.write(";\n\n");
			out.write("Comtrees\n\n");
			for (int comt = 1; comt <= maxComtrees; comt++) {
				if (!validComtIndex(comt)) continue;

				out.write( "comtree=" + getComtree(comt)
				    + " core=" + getComtCore(comt)
				    + "\nbitRateDown=" + getComtBrDown(comt)
				    + " bitRateUp=" + getComtBrUp(comt)
				    + " pktRateDown=" + getComtPrDown(comt)
				    + " pktRateUp=" + getComtPrUp(comt)
				    + "\nleafBitRateDown=" + getComtLeafBrDown(comt)
				    + " leafBitRateUp=" + getComtLeafBrUp(comt)
				    + " leafPktRateDown=" + getComtLeafPrDown(comt)
				    + " leafPktRateUp=" + getComtLeafPrUp(comt));

				// iterate through core nodes and print
				out.write("\n");
				set<int>::iterater cp;
				for (cp = firstCore(comt); cp != lastCore(comt); cp++) {
					out.write( "core=" + getNodeName(*cp) + " ";
				}
				out.write( "\n";
				// iterate through links and print
				
				for (lp = firstComtLink(comt); lp != lastComtLink(comt); lp++) {
					int left = getLinkL(*lp); int right = getLinkR(*lp);
					out.write( "link(=" + getNodeName(left);
					if (isRouter(left)) out.write( "." + getLocLnkL(*lp);
					out.write("," + getNodeName(right);
					if (isRouter(right)) out.write("." + getLocLnkR(*lp);
					out.write(") ";
				}
			}
*/
			out.write( ";\n");
		}catch(Exception e){
				System.err.println("IO ERROR");
		}
	}
}

