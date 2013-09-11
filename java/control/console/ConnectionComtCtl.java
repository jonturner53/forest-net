package forest.control.console;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PushbackReader;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.channels.SocketChannel;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CodingErrorAction;
import java.util.TreeSet;

import algoLib.misc.Util;
//import forest.common.NetBuffer;
import forest.control.ComtInfo;
import forest.control.NetInfo;

public class ConnectionComtCtl {
	
    private final int MAXNODE    = 10000;
    private final int MAXLINK    = 20000;
    private final int MAXRTR     =  1000;
    private final int MAXCOMTREE = 20000;
	
	private int cmPort = 30121;
	private String cmAddr = "forest4.arl.wustl.edu";
	
	private Charset ascii;
	private CharsetEncoder enc;
	private CharBuffer cb;
	private ByteBuffer bb;
	private SocketChannel serverChan;
	private PushbackReader inRdr;
	//private NetBuffer inBuf;
	
	private boolean autoRefresh = true;
	
	private NetInfo netInfo;
	private ComtInfo comtrees;
	private TreeSet<Integer> comtSet;
	
	public ConnectionComtCtl(){
		setupIo();
		netInfo = new NetInfo(MAXNODE, MAXLINK, MAXRTR);
		comtrees = new ComtInfo(MAXCOMTREE, netInfo);
		comtSet = new TreeSet<Integer>();
	}
	
	/**
	 * Initialize a connection to Net Manager
	 */
	private void setupIo(){
		ascii = Charset.forName("US-ASCII");
		enc = ascii.newEncoder();
		enc.onMalformedInput(CodingErrorAction.IGNORE);
		enc.onUnmappableCharacter(CodingErrorAction.IGNORE);
		cb = CharBuffer.allocate(1024);
		bb = ByteBuffer.allocate(1024);
	}
	
	/**
	 * Send a string msg to Net Manager
	 * @param  msg message
	 * @return successful
	 */
	protected boolean sendString(String msg){
		bb.clear(); cb.clear();
		cb.put(msg); cb.flip();
		enc.encode(cb,bb,false); bb.flip();
		try {
			System.out.println("SENDING: " + msg);
			serverChan.write(bb);
		} catch(Exception e){
			e.printStackTrace();
			return false;
		}
		return true;
	}
	
	protected boolean connectToComCtl() {
		try {
			SocketAddress serverAdr = new InetSocketAddress(
												InetAddress.getByName(cmAddr), cmPort);
			serverChan = SocketChannel.open(serverAdr);
		} catch(Exception e) {
			return false;
		}
		if (!serverChan.isConnected()) return false;
		//inBuf = new NetBuffer(serverChan,1000);
		return true;
	}
	
	protected String getNet(){
		if(!sendString("getNet\n")){
			return "connot send request to server";
		}
		try {
			inRdr = new PushbackReader(
						new InputStreamReader(serverChan.socket().getInputStream()));
		} catch (IOException e) {
			e.printStackTrace();
		}
		if(!netInfo.read(inRdr)){
			return "can't read network topology\n";
		}
		return null;
	}
	
	protected String getComtSet(){
		if(!sendString("getComtSet\n")) {
			return "connot send request to server";
		}
		readComtSet(inRdr);
		System.out.println(comtSet);
		return null;
	}
	
    /** Read a response to a getComtSet request.
     *  The response consists of the string "comtSet" followed by a
     *  list of comtree numbers, separated by commas and surrounded
     *  by parentheses.
     *  @return if working well, it returns null. Otherwise, error message
     */
    private String readComtSet(PushbackReader in) {
    	String word = Util.readWord(in);
        if (word == null || !word.equals("comtSet")) 
        	return "readComtSet: unexpected response " + word + "\n";
        if (!Util.verify(in,'(')) 
        	return "readComtSet: expected left paren\n";
        
        comtSet.clear();
        Util.MutableInt cnum = new Util.MutableInt();
        while (true) {
            if (!Util.readNum(in,cnum))
            	return "readComtSet: could not read " + "expected comtree number\n";
            comtSet.add(cnum.val);
            if (Util.verify(in,')')) return null;
            if (!Util.verify(in,','))
                return "readComtSet: expected comma\n";
        }
    }
    
    protected String getComtree(int ccomt){
    	if(!sendString("getComtree " + ccomt + "\n")){
    		return "connot send request to server";
    	}
    	int recvdComt = comtrees.readComtStatus(inRdr);
        if (recvdComt != ccomt) {
            System.out.println("recvdComt=" + recvdComt);
            return "received comtree info does not match request\n";
        }
        System.out.println(comtrees);
        return comtrees.toString();
    }

	public ComtInfo getComtrees() {
		return comtrees;
	}
	
	public TreeSet<Integer> getComtTreeSet(){
		return comtSet;
	}
	
	public String getCmAddr() {
		return cmAddr;
	}

	public void setCmAddr(String cmAddr) {
		this.cmAddr = cmAddr;
	}

	public NetInfo getNetInfo() {
		return netInfo;
	}
	
	public boolean isAutoRefresh(){
		return autoRefresh;
	}
	public void setAutoRefresh(boolean b){
		this.autoRefresh = b;
	}
	
	/**
	 * Close socket connection to Comtree Controller
	 */
	public void closeSocket(){
		if(serverChan != null && serverChan.isConnected()){
			try {
				serverChan.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
}
