package forest.control;

/** @file AcctMgr.java
 *
 *  @author Jon Turner
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

/*
todo
button to update profile
count of number of sessions
read sessions - forest addr, rtr addr, rate (send all, display up to 5)
terminate button for each session

in Client manager
add client type: std, admin (control over std users), master (over admins)
general handleClient method
return rateSpec as part of newSession
in future, return with comtCtl also
*/

import java.awt.Color;
import java.awt.*;
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
import java.nio.charset.*;
import java.util.Scanner;
import javax.swing.*;

import algoLib.dataStructures.basic.*;
import algoLib.dataStructures.graphs.*;
import algoLib.misc.*;
import forest.common.*;
import forest.control.*;

/** AcctMgr allows a Forest user to create and manage an account.
 *  @author Jon Turner
 *  @date 2013
 */
public class AcctMgr extends MouseAdapter implements ActionListener {
	public static void main(String[] args) {	
		SwingUtilities.invokeLater(new Runnable() {
			public void run() { new AcctMgr(); }
		});
	}

	private static int CM_PORT = 30122;
	private SocketChannel serverChan;
	private NetBuffer inBuf;

	private JTextField serverField;
	private String serverName;
	private JTextField userField;
	private String userName;
	private JPasswordField pwdField;
	private char[] password;
	JTextArea statusArea;

	private JTextField nameField;
	private String realName;
	private JTextField emailField;
	private String email;
	private JTextField nupwdField;
	private char[] newPassword;

	private JTextField drateField;
	private String defRates;
	private JTextField trateField;
	private String totRates;
	private JTextField confirmField;
	private char[] confirmPassword;

	private Box labeledField(JTextField text, String name) {
		JLabel label = new JLabel(name);
		label.setBorder(BorderFactory.createEmptyBorder(0,5,0,0));
		text.setMinimumSize(text.getPreferredSize());
		text.setMaximumSize(text.getPreferredSize());
		text.addActionListener(this);
		text.setActionCommand(name);
		label.setAlignmentX(Component.LEFT_ALIGNMENT);
		text.setAlignmentX(Component.LEFT_ALIGNMENT);
		Box box = Box.createVerticalBox();
		box.add(label); box.add(text);
		return box;
	}
		
	private Box tripleBox(Box b1, Box b2, Box b3) {
		Box b = Box.createHorizontalBox();
			   b.add(Box.createHorizontalStrut(10));
		b.add(b1); b.add(Box.createHorizontalStrut(5));
		b.add(b2); b.add(Box.createHorizontalStrut(5));
		b.add(b3); b.add(Box.createHorizontalGlue());
		return b;
	}

	/** Constructor for AcctMgr class.
	 *  Setup the various user interface components.
	 */
	AcctMgr() {
		JFrame jfrm = new JFrame("Forest account manager");
		jfrm.setLayout(new FlowLayout());
		jfrm.setSize(850,400);
		jfrm.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		// top area with server name, user name, password
		serverField = new JTextField(20);
		Box b1 = labeledField(serverField,"client manager");
		userField = new JTextField(20);
		Box b2 = labeledField(userField,"user name");
		pwdField = new JPasswordField(15);
		Box b3 = labeledField(pwdField,"password");
		Box box1 = tripleBox(b1,b2,b3);

		JButton loginBtn = new JButton("login");
		loginBtn.addActionListener(this);
		JButton newAcctBtn = new JButton("new account");
		newAcctBtn.addActionListener(this);
		Box box1a = Box.createHorizontalBox();
		box1a.add(loginBtn); box1a.add(newAcctBtn);

		// status panel comes next
		statusArea = new JTextArea(3,60);
		statusArea.setEditable(false);
		statusArea.append("Fill in fields above " +
				  "(type enter after each), " +
				  "then press login or new account button\n");
		Box statusBox = Box.createHorizontalBox();
		statusBox.add(statusArea);
		statusBox.setBorder(BorderFactory.
				     createEmptyBorder(10,13,10,15));

		// then profile area
		nameField = new JTextField(20);
		b1 = labeledField(nameField,"real name");
		emailField = new JTextField(20);
		b2 = labeledField(emailField,"email address");
		nupwdField = new JTextField(15);
		b3 = labeledField(nupwdField,"new password");

		Box box2 = tripleBox(b1,b2,b3);

		drateField = new JTextField(20);
		b1 = labeledField(drateField,"default rates");
		trateField = new JTextField(20);
		b2 = labeledField(trateField,"total rates");
		confirmField = new JTextField(15);
		b3 = labeledField(confirmField,"confirm password");

		Box box3 = tripleBox(b1,b2,b3);

		JButton updateBtn = new JButton("update");
		updateBtn.addActionListener(this);
		Box box4 = Box.createHorizontalBox();
		box4.add(Box.createHorizontalStrut(10));
		box4.add(updateBtn);

		// Finally, the sessions area
		Box box5 = showSessions();

		// Putting it together
		box1.setAlignmentX(Component.LEFT_ALIGNMENT);
		box1a.setAlignmentX(Component.LEFT_ALIGNMENT);
		statusBox.setAlignmentX(Component.LEFT_ALIGNMENT);
		box2.setAlignmentX(Component.LEFT_ALIGNMENT);
		box3.setAlignmentX(Component.LEFT_ALIGNMENT);
		box4.setAlignmentX(Component.LEFT_ALIGNMENT);

		Box boxAll = Box.createVerticalBox();
		boxAll.add(box1); boxAll.add(box1a); boxAll.add(statusBox);
		boxAll.add(box2); boxAll.add(box3); boxAll.add(box4);
		boxAll.add(box5);
		boxAll.setAlignmentX(Component.LEFT_ALIGNMENT);

		jfrm.add(boxAll);
		jfrm.setVisible(true);

		setupIo();
	}

	Charset ascii;
        CharsetEncoder enc;
	CharBuffer cbuf;
	ByteBuffer bbuf;

	private void setupIo() {
		ascii = Charset.forName("US-ASCII");
                enc = ascii.newEncoder();
		enc.onMalformedInput(CodingErrorAction.IGNORE);
		enc.onUnmappableCharacter(CodingErrorAction.IGNORE);
		cbuf = CharBuffer.allocate(1000);
		bbuf = ByteBuffer.allocate(1000);
	}

	private boolean sendString(String s) {
		cbuf.put(s); cbuf.flip();
		enc.encode(cbuf,bbuf,false); bbuf.flip();
		try {
			serverChan.write(bbuf);
		} catch(Exception e) {
			showStatus("cannot write to client manager");
			return false;
		}
		return true;
	}

	private class Session {
		int clientAdr;
		int rtrAdr;
		RateSpec rates;
	};
	Session[] sessVec;

	private Box showSessions() {
		JLabel sessLabel = new JLabel("sessions");
		Box b = Box.createVerticalBox();
		b.add(sessLabel);
		return b;
	}

	private boolean updateProfile() {
		if (!sendString("updateProfile\n" +
			   	"realName: " + realName + "\n" +
				"email: " + email + "\n" +
				"defRates: " + defRates + "\n" +
				"totRates: " + totRates + "\n" +
				"over"))
			return false;
		if (!inBuf.readLine().equals("profile updated") ||
		    !inBuf.readLine().equals("over"))
			return false;
		return true;
	}

	private boolean changePassword(String pwd) {
		if (!sendString("changePassword\n" + "password: " + pwd +
				"\n" + "over"))
			return false;
		if (!inBuf.readLine().equals("password updated") ||
		    !inBuf.readLine().equals("over"))
			return false;
		return true;
	}

	private boolean readProfile() {
		String s;
		boolean gotName, gotEmail, gotDefRates, gotTotRates; 
		gotName = gotEmail = gotDefRates = gotTotRates = false; 
		while (true) {
			s = inBuf.readAlphas();
			if (s.equals("realName") && inBuf.verify(':')) {
				realName = inBuf.readName();
				if (realName == null) return false;
				nameField.setText(realName);
				gotName = true;
			} else if (s.equals("email") && inBuf.verify(':')) {
				email = inBuf.readWord();
				if (email == null) return false;
				emailField.setText(email);
				gotEmail = true;
			} else if (s.equals("defRates") && inBuf.verify(':')) {
				RateSpec rates = inBuf.readRspec();
				if (rates == null) return false;
				drateField.setText(rates.toString());
				gotDefRates = true;
			} else if (s.equals("totRates") && inBuf.verify(':')) {
				RateSpec rates = inBuf.readRspec();
				if (rates == null) return false;
				trateField.setText(rates.toString());
				gotTotRates = true;
			} else {
				return	gotName && gotEmail &&
					gotDefRates && gotTotRates;
			}
			inBuf.nextLine();
		}
	}

	private boolean login(String server, String user, char[] pwd) {
		// open channel to server, send login string and
		// process reply
		// on success, return true
		try {
			SocketAddress serverAdr = new InetSocketAddress(
                                      InetAddress.getByName(server),CM_PORT);
                	// connect to server
                	serverChan = SocketChannel.open(serverAdr);
		} catch(Exception e) {
			showStatus("cannot connect to client manager" + "\n");
			return false;
		}

		if (!sendString("Forest-login-v1\nlogin: " + user + "\n" +
			   	"password: " + (new String(pwd)) + "\nover\n"))
			return false;

		inBuf = new NetBuffer(serverChan,1000);
		if (!inBuf.readLine().equals("login successful") ||
		    !inBuf.readLine().equals("over"))
			return false;

		if (!sendString("getProfile\n")) return false;

		if (!readProfile()) return false;

		return true;
	}

	private boolean newAccount(String server, String user, char[] pwd) {
		// open channel to server, send login string and
		// process reply
		// on success, return true
		try {
			SocketAddress serverAdr = new InetSocketAddress(
                                      InetAddress.getByName(server),CM_PORT);
                	// connect to server
                	serverChan = SocketChannel.open(serverAdr);
		} catch(Exception e) {
			showStatus("cannot connect to client manager" + "\n");
			return false;
		}

		if (!sendString("Forest-login-v1\nnewAccount: " + user + "\n" +
			   	"password: " + (new String(pwd)) + "\nover\n"))
			return false;

		NetBuffer inBuf = new NetBuffer(serverChan,1000);
		if (!inBuf.readLine().equals("new account created") ||
		    !inBuf.readLine().equals("over"))
			return false;
		return true;
	}

	public void showStatus(String s) {
		statusArea.selectAll(); statusArea.replaceSelection(s);
	}

	/** Respond to user interface events.
	 *  ae is an action event relating to one of the user interface
	 *  elements.
	 */
	public void actionPerformed(ActionEvent ae) {
		if (ae.getActionCommand().equals("client manager")) {
			serverName = serverField.getText();
		} else if (ae.getActionCommand().equals("user name")) {
			userName = userField.getText();
		} else if (ae.getActionCommand().equals("password")) {
			password = pwdField.getPassword();
		} else if (ae.getActionCommand().equals("new password")) {
			newPassword = pwdField.getPassword();
		} else if (ae.getActionCommand().equals("confirm password")) {
			confirmPassword = pwdField.getPassword();
		} else if (ae.getActionCommand().equals("real name")) {
			realName = nameField.getText();
		} else if (ae.getActionCommand().equals("email address")) {
			email = emailField.getText();
		} else if (ae.getActionCommand().equals("default rates")) {
			defRates = drateField.getText();
		} else if (ae.getActionCommand().equals("total rates")) {
			totRates = drateField.getText();
		} else if (ae.getActionCommand().equals("login")) {
			if (login(serverName,userName,password)) {
				showStatus("Success");
			} else {
				showStatus("Login failed");
			}
		} else if (ae.getActionCommand().equals("newAccount")) {
			showStatus("Complete profile info below," +
				   "typing password twice to confirm");
			if (newAccount(serverName,userName,password)) {
				showStatus("New account created");
			} else {
				showStatus("Cannot create account");
			}
		} else if (ae.getActionCommand().equals("update")) {
			if (updateProfile()) {
				showStatus("Profile updated");
			} else {
				showStatus("Cannot update profile");
			}
		} else if (ae.getActionCommand().equals("change password")) {
			if (!newPassword.equals(confirmPassword)) {
				showStatus("Mismatch. Please re-type password " +
					   "in both new password and confirm " +
					   "password boxes");
			} else if (changePassword(new String(newPassword))) {
				showStatus("Password changed");
			} else {
				showStatus("Cannot change password");
			}
		}
	}

	/** Respond to mouse click in the panel by turning wall on/off.
	 *  @param e is a mouse event corresponding to a click in the panel.
	 */
	public void mouseClicked(MouseEvent e) {
	}


/*
	public static void main(String[] args) {
		process command line arguments
		 - cliMgrName/ip
		 - connect
		 - setup input and output buffers
		 - send initial string
		 - prompt user for login and password
		 - send to client manager and check response
		while (true) {
			prompt
			read an input line
			process it
			display the response
		}
		Or, use gui to provide access to parameters.
		Collection of text boxes.
		For administrative users, add box for target user.
		Upload button to transfer photos.

		Reproduce NetBuffer for interaction with ClientMgr?
	}

	
	
		cmSock.sendall("Forest-login-v1\nlogin: " + uname + \
			       "\npassword: " + pword + "\nover\n")


		buf = ""
		line,buf = self.readLine(cmSock,buf)
		if line != "login successful" :
			return False
		line,buf = self.readLine(cmSock,buf)
		if line != "over" :
			return False

		cmSock.sendall("newSession\nover\n")

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "yourAddress" or chunks[1] != ":" :
			return False
		self.myFadr = string2fadr(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "yourRouter" or chunks[1] != ":" :
			return False
		triple = chunks[2].strip(); triple = triple[1:-1].strip()
		chunks = triple.split(",")
		if len(chunks) != 3 : return False
		self.rtrIp = string2ip(chunks[0].strip())
		self.rtrPort = int(chunks[1].strip())
		self.rtrFadr = string2fadr(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "comtCtlAddress" or chunks[1] != ":" :
			return False
		self.comtCtlFadr = string2fadr(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "connectNonce" or chunks[1] != ":" :
			return False
		self.nonce = int(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf) 
		if line != "overAndOut" : return False

		cmSock.close()

		if self.debug >= 1 :
			print "avatar address =", fadr2string(self.myFadr)
			print "router info = (", ip2string(self.rtrIp), \
					     str(self.rtrPort), \
					     fadr2string(self.rtrFadr), ")"
			print "comtCtl address = ",fadr2string(self.comtCtlFadr)
			print "nonce = ", self.nonce

		return True
*/

}
