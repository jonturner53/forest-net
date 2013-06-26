package forest.control;

/** @file Console.java
*
 *  @author Jon Turner
 *  @date 2012
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.awt.*;
import javax.swing.*;
import java.util.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.awt.Color;
import java.awt.geom.Point2D;

import algoLib.dataStructures.basic.*;
import algoLib.dataStructures.graphs.*;
import algoLib.misc.*;
import forest.common.*;
import forest.control.*;

/** This class implements a simple network management console for
 *  a Forest network.
 */
public class Console extends MouseAdapter implements ActionListener {
	private static final int maxNode    = 10000;
	private static final int maxLink    = 20000;
	private static final int maxRtr     =  1000;
        private static final int maxComtree = 20000;

	private static final int SIZE = 700;        	///<size of gui frame

	private static InetSocketAddress nmSockAdr; ///< address for NetMgr
	private static SocketChannel nmChan = null; ///< for NetMgr connection

	private static NetInfo net;	///< contains net toppology and
					///< comtree info
	
	/** Main method launches user interface */
	public static void main(String[] args) {	
		SwingUtilities.invokeLater(new Runnable() {
			public void run() { new Console(); }
		});
	}

	JTextField inFileText, outFileText;
	EMapPanel panel;

	/** Constructor for Console class.
	 *  Setup the various user interface components.
	 */
	Console() {
		// connect to NetMgr and get NetInfo
		

		// connect to ComtCtl and get ComtInfo

		// Setup frame
		JFrame jfrm = new JFrame("Forest Console");
		jfrm.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		JPanel contentPane = new JPanel(new BorderLayout());
                jfrm.setContentPane(contentPane);

		// Setup top row controls - selecting comtree
		// Show/hide rates

		// Add console panel showing network/comtrees
		panel = new ConsolePanel(net,comtrees);
		panel.addMouseListener(this);
		jfrm.add(panel);

		// setup controls for table area
		// component selection box, table selection box,...

		// setup text area showing table contents (with scrollbar)

		// setup controls for packet log area

		// setup text area for packet log

		jfrm.pack();
		jfrm.setVisible(true);
	}

	/** Respond to user interface events.
	 *  ae is an action event relating to one of the user interface
	 *  elements.
	 */
	public void actionPerformed(ActionEvent ae) {
		// when connect button pressed,
			// connect to NetMgr and get NetInfo 
			// also connect ComtCtl and get ComtInfo
		// ...
	}

	/** Respond to mouse click in the network map.
	 *  @param e is a mouse event corresponding to a click in the panel.
	 */
	public void mouseClicked(MouseEvent e) {
	}

	Charset ascii;
        CharsetEncoder enc;
	CharBuffer cbuf;
	ByteBuffer bbuf;

	/** Setup to transfer ASCII characters on socket to client manager.
	 */
	private void setupIo() {
		ascii = Charset.forName("US-ASCII");
                enc = ascii.newEncoder();
		enc.onMalformedInput(CodingErrorAction.IGNORE);
		enc.onUnmappableCharacter(CodingErrorAction.IGNORE);
		cbuf = CharBuffer.allocate(1000);
		bbuf = ByteBuffer.allocate(1000);
	}

	/** Send a string to the network manager socket.
	 *  @param s is the string to be sent; should use only ASCII characters
	 */
	private boolean sendString(String s) {
		cbuf.clear(); bbuf.clear();
		cbuf.put(s); cbuf.flip();
		enc.encode(cbuf,bbuf,false); bbuf.flip();
		try {
			serverChan.write(bbuf);
		} catch(Exception e) {
			showStatus("cannot write to network manager");
			return false;
		}
		return true;
	}
}
