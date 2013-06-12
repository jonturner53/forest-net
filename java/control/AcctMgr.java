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

	private boolean loggedIn;		// true when logged in

	private JTextField serverField;
	private String serverName;
	private JTextField userField;
	private String userName;
	private JPasswordField pwdField;
	private String password;
	JTextArea statusArea;

	private JTextField nameField;
	private String realName;
	private JTextField emailField;
	private String email;
	private JPasswordField nupwdField;
	private String newPassword;

	private JTextField drateField;
	private String defRates;
	private JTextField trateField;
	private String totRates;
	private JPasswordField confirmField;
	private String confirmPassword;

	private JTextField photoField;

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
		
	private Box doubleBox(Box b1, Box b2) {
		Box b = Box.createHorizontalBox();
			   b.add(Box.createHorizontalStrut(10));
		b.add(b1); b.add(Box.createHorizontalStrut(5));
		b.add(b2);
		return b;
	}

	private Box tripleBox(Box b1, Box b2, Box b3) {
		Box b = Box.createHorizontalBox();
			   b.add(Box.createHorizontalStrut(10));
		b.add(b1); b.add(Box.createHorizontalStrut(5));
		b.add(b2); b.add(Box.createHorizontalStrut(5));
		b.add(b3);
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

		// top area with server name connect button
		serverField = new JTextField(20);
		serverField.setText("forest2.arl.wustl.edu");
		serverName = "forest2.arl.wustl.edu";
		Box b1 = labeledField(serverField,"client manager");
		JButton connectBtn = new JButton("connect");
		connectBtn.addActionListener(this);
		b1.setAlignmentY(1); connectBtn.setAlignmentY(1);
		Box box0 = Box.createHorizontalBox();
		box0.add(Box.createHorizontalStrut(10));
		box0.add(b1); box0.add(connectBtn); 
		box0.add(Box.createHorizontalGlue());

		// next user name and password; login, new account buttons
		userField = new JTextField(20);
		Box b2 = labeledField(userField,"user name");
		pwdField = new JPasswordField(15);
		Box b3 = labeledField(pwdField,"password");
		Box box1 = doubleBox(b2,b3);
		JButton loginBtn = new JButton("login");
		loginBtn.addActionListener(this);
		JButton newAcctBtn = new JButton("new account");
		newAcctBtn.addActionListener(this);
		box1.setAlignmentY(1);
		loginBtn.setAlignmentY(1); newAcctBtn.setAlignmentY(1);
		Box box1a = Box.createHorizontalBox();
		box1a.add(box1); box1a.add(loginBtn); box1a.add(newAcctBtn);

		// status panel comes next
		statusArea = new JTextArea(3,60);
		statusArea.setEditable(false);
		statusArea.append("Enter server name above and connect\n");
		Box statusBox = Box.createHorizontalBox();
		statusBox.add(statusArea);
		statusBox.setBorder(BorderFactory.
				     createEmptyBorder(10,13,10,15));

		// then profile area
		nameField = new JTextField(20);
		b1 = labeledField(nameField,"real name");
		emailField = new JTextField(20);
		b2 = labeledField(emailField,"email address");
		nupwdField = new JPasswordField(15);
		b3 = labeledField(nupwdField,"new password");

		Box box2 = tripleBox(b1,b2,b3);

		drateField = new JTextField(20);
		b1 = labeledField(drateField,"default rates");
		trateField = new JTextField(20);
		b2 = labeledField(trateField,"total rates");
		confirmField = new JPasswordField(15);
		b3 = labeledField(confirmField,"confirm password");

		Box box3 = tripleBox(b1,b2,b3);

		JButton updateBtn = new JButton("update");
		updateBtn.addActionListener(this);
		JButton modPwdBtn = new JButton("change password");
		modPwdBtn.addActionListener(this);
		photoField = new JTextField(20);
		photoField.setText("not working yet");
		photoField.setMaximumSize(photoField.getPreferredSize());
		JButton uploadBtn = new JButton("upload photo");
		Box box4 = Box.createHorizontalBox();
		box4.add(Box.createHorizontalStrut(10));
		box4.add(updateBtn);
		box4.add(modPwdBtn);
		box4.add(photoField);
		box4.add(uploadBtn);

		// Finally, the sessions area
		Box box5 = showSessions();

		// Putting it together
		box0.setAlignmentX(Component.LEFT_ALIGNMENT);
		box1a.setAlignmentX(Component.LEFT_ALIGNMENT);
		statusBox.setAlignmentX(Component.LEFT_ALIGNMENT);
		box2.setAlignmentX(Component.LEFT_ALIGNMENT);
		box3.setAlignmentX(Component.LEFT_ALIGNMENT);
		box4.setAlignmentX(Component.LEFT_ALIGNMENT);

		Box boxAll = Box.createVerticalBox();
		boxAll.add(box0); boxAll.add(box1a); boxAll.add(statusBox);
		boxAll.add(box2); boxAll.add(box3); boxAll.add(box4);
		boxAll.add(box5);
		boxAll.setAlignmentX(Component.LEFT_ALIGNMENT);

		jfrm.add(boxAll);
		jfrm.setVisible(true);

		loggedIn = false; setupIo();
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
		cbuf.clear(); bbuf.clear();
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
                                "realName: \"" + realName + "\"\n" +
                                "email: " + email + "\n" +
                                "defRates: " + defRates + "\n" +
                                "totalRates: " + totRates + "\n" +
                                "over\n");
		if (!sendString("updateProfile\n" +
			   	"realName: \"" + realName + "\"\n" +
				"email: " + email + "\n" +
				"defRates: " + defRates + "\n" +
				"totalRates: " + totRates + "\n" +
				"over\n"))
			return false;
		String s1 = inBuf.readLine();
		String s2 = inBuf.readLine();
		if (!s1.equals("profile updated") || !s2.equals("over"))
		//if (!inBuf.readLine().equals("profile updated") ||
		 //   !inBuf.readLine().equals("over"))
			return false;
		return true;
	}

	private boolean changePassword(String pwd) {
		if (!sendString("changePassword\n" + "password: " + pwd +
				"\n" + "over\n"))
			return false;
		String s1 = inBuf.readLine();
		String s2 = inBuf.readLine();
		if (!s1.equals("success") || !s2.equals("over")) 
		//if (!inBuf.readLine().equals("password updated") ||
		 //   !inBuf.readLine().equals("over"))
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
				realName = inBuf.readString();
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
				defRates = rates.toString();
				drateField.setText(defRates);
				gotDefRates = true;
			} else if (s.equals("totalRates") && inBuf.verify(':')) {
				RateSpec rates = inBuf.readRspec();
				if (rates == null) return false;
				totRates = rates.toString();
				trateField.setText(totRates);
				gotTotRates = true;
			} else if (s.equals("over")) {
				inBuf.nextLine();
				return	gotName && gotEmail &&
					gotDefRates && gotTotRates;
			}
			inBuf.nextLine();
		}
	}

	private boolean connect(String server) {
		// open channel to server
		// on success, return true
		try {
			SocketAddress serverAdr = new InetSocketAddress(
                                      InetAddress.getByName(server),CM_PORT);
                	// connect to server
                	serverChan = SocketChannel.open(serverAdr);
		} catch(Exception e) {
			return false;
		}
		if (!sendString("Forest-login-v1\n")) return false;
		inBuf = new NetBuffer(serverChan,1000);
		return true;
	}

	private boolean login(String user, String pwd) {
		if (!sendString("login: " + user + "\n" +
			   	"password: " + pwd + "\nover\n"))
			return false;
		String s1 = inBuf.readLine();
		String s2 = inBuf.readLine();
		if (s1 == null || !s1.equals("login successful") ||
		    s2 == null || !s2.equals("over")) {
		//if (!inBuf.readLine().equals("login successful") ||
		    //!inBuf.readLine().equals("over")) {
			if (s1 != null) System.out.println(s1);
			if (s2 != null) System.out.println(s2);
			return false;
		}

		if (!sendString("getProfile\n")) return false;

		if (!readProfile()) return false;

		return true;
	}

	private boolean newAccount(String user, String pwd) {
		if (!sendString("newAccount: " + user + "\n" +
			   	"password: " + pwd + "\nover\n"))
			return false;

		if (!inBuf.readLine().equals("success") ||
		    !inBuf.readLine().equals("over"))
			return false;
		if (!sendString("getProfile\n")) return false;

		if (!readProfile()) return false;
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
			char[] x = pwdField.getPassword();
			password = new String(x);
		} else if (ae.getActionCommand().equals("new password")) {
			char[] x = nupwdField.getPassword();
			newPassword = new String(x);
		} else if (ae.getActionCommand().equals("confirm password")) {
			char[] x = confirmField.getPassword();
			confirmPassword = new String(x);
		} else if (ae.getActionCommand().equals("real name")) {
			realName = nameField.getText();
		} else if (ae.getActionCommand().equals("email address")) {
			email = emailField.getText();
		} else if (ae.getActionCommand().equals("default rates")) {
			defRates = drateField.getText();
		} else if (ae.getActionCommand().equals("total rates")) {
			totRates = drateField.getText();
		} else if (ae.getActionCommand().equals("connect")) {
			if (connect(serverName)) {
				showStatus("Success\nEnter user name and " +
					   "password, then login or create " +
					   "new account");
			} else {
				showStatus("Cannot connect");
			}
		} else if (ae.getActionCommand().equals("login")) {
			if (loggedIn) {
				; // do nothing
			} else if (login(userName,password)) {
				loggedIn = true;
				showStatus("Logged in");
			} else {
				showStatus("Login failed");
			}
		} else if (ae.getActionCommand().equals("new account")) {
			if (newAccount(userName,password)) {
				showStatus("New account created\n" +
					   "Update profile and change" +
				   	   "password below");
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