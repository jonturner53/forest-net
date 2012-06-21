package forest.common;

public class PktHeader {
	public PktHeader() { setVersion(Forest.FOREST_VERSION); }
	private int ver;	// version num field
	private int lng;	// length field
	private Forest.PktTyp typ;	// packet type field
	private short flgs;	// flags
	private int comt;	// comtree field
	private int sadr;	// source address
	private int dadr;	// destination address
	private int inlnk;	// link on which packet arrived
	private int tSrcIp;	// src IP addr from substrate header
	private int tSrcPort;	// src IP port # from substrate header
	private int iob;	// num of bytes in buffer

	public void pack(Forest.PktBuffer p) {
		int x = (getVersion() << 28)
			| ((getLength() & 0xfff) << 16)
			| ((getPtype().val() & 0xff) << 8)
			| (getFlags() & 0xff);
		p.set(0,x);
		p.set(1,getComtree());
		p.set(2,getSrcAdr());
		p.set(3,getDstAdr());
	}
	
	public void unpack(Forest.PktBuffer p) {
		int x = p.get(0);
		setVersion((x >> 28) & 0xf);
		setLength((x >> 16) & 0xfff);
		int y = (x >> 8) & 0xff;
		setPtype(Forest.PktTyp.getIndexByCode(y));
		setFlags((short)(x & 0xff));
		setComtree(p.get(1));
		setSrcAdr(p.get(2));
		setDstAdr(p.get(3));
	}
	public String toString(Forest.PktBuffer p) {
		String s = "";
		s += "len=" + getLength();
		s += " typ=";
		if(getPtype() == Forest.PktTyp.CLIENT_DATA)     s+= "data      ";
		else if(getPtype() == Forest.PktTyp.SUB_UNSUB)  s+= "sub_unsub ";
		else if(getPtype() == Forest.PktTyp.CLIENT_SIG) s+= "client_sig";
		else if(getPtype() == Forest.PktTyp.CONNECT)    s+= "connect   ";
		else if(getPtype() == Forest.PktTyp.DISCONNECT) s+= "disconnect";
		else if(getPtype() == Forest.PktTyp.NET_SIG)    s+= "net_sig   ";
		else if(getPtype() == Forest.PktTyp.RTE_REPLY)  s+= "rteRep    ";
		else if(getPtype() == Forest.PktTyp.RTR_CTL)    s+= "rtr_ctl   ";
		else if(getPtype() == Forest.PktTyp.VOQSTATUS)  s+= "voq_status";
		else						s+= "--------- ";
		s += " flags=" + ((int) getFlags());
		s += " comt=" + getComtree();
		s += " sadr=" + Forest.fAdr2string(getSrcAdr());
		s += " dadr=" + Forest.fAdr2string(getDstAdr());
		int x;
		for(int i = 0; i < Math.min(8,(getLength()-Forest.HDR_LENG)/4); i++) {
			x = p.get((Forest.HDR_LENG/4)+i);
			s += " " + x;
		}
		s += "\n";
		if (getPtype() == Forest.PktTyp.CLIENT_SIG || getPtype() == Forest.PktTyp.NET_SIG) {
               		CtlPkt cp = new CtlPkt();
               		if(cp.unpack(p)) s += cp.toString();
		}
		
		return s;
	}
		//setters
	public void setVersion(int v) { ver = v; }
	public void setLength(int l) { lng = l; }
	public void setPtype(Forest.PktTyp p) { typ = p; }
	public void setFlags(short f) { flgs = f; }
	public void setComtree(int c) { comt = c; }
	public void setSrcAdr(int sa) { sadr = sa; }
	public void setDstAdr(int da) { dadr = da; }
	public void setInLink(int il) { inlnk = il; }
	public void setTunSrcIp(int tsi) { tSrcIp = tsi; }
	public void setTunSrcPort(int tsp) { tSrcPort = tsp; }
	public void setIoBytes(int b) { iob = b; }
	//getters
	public int getVersion() { return ver; }
	public int getLength() { return lng; }
	public Forest.PktTyp getPtype() { return typ; }
	public short getFlags() { return flgs; }
	public int getComtree() { return comt; }
	public int getSrcAdr() { return sadr; }
	public int getDstAdr() { return dadr; }
	public int getInLink() { return inlnk; }
	public int getTunSrcIp() { return tSrcIp; }
	public int getTunSrcPort() { return tSrcPort; }
	public int getIoBytes() { return iob; }
}
