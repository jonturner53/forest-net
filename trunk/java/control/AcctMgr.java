package forest.control;

/** @file AcctMgr.java
 *
 *  @author Jon Turner
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
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
public class AcctMgr extends MouseAdapter implements ActionListener, FocusListener {
	public static void main(String[] args) {	
		SwingUtilities.invokeLater(new Runnable() {
			public void run() { new AcctMgr(); }
		});
	}

	private static int CM_PORT = 30122;	///< client manager's port
	private SocketChannel serverChan;	///< channel to client mgr
	private NetBuffer inBuf;		///< for reading from socket

	// various user interface buttons
	private JButton connectBtn;	///< connect to client mg4
	private JButton loginBtn;	///< login to specific account
	private JButton newAcctBtn;	///< create new account
	private JButton updateBtn;	///< update profile
	private JButton modPwdBtn;	///< modify password
	private JButton uploadBtn;	///< upload photo
	private JButton getSessBtn; ///< get session information
	private JButton cancelBtn;  ///< cancel session button
	private JButton cancelAllBtn; ///< cancel all open sessions

	// various text fields and the strings use to hold contents
	private JTextField serverField;	///< client mgr server name/ip
	private String serverName;
	private JTextField userField;	///< user name
	private String userName;
	private JPasswordField pwdField;///< password
	private String password;

	private JTextField nameField;	///< real name
	private String realName;
	private JTextField emailField;	///< email address
	private String email;
	private JPasswordField nupwdField; ///< new password field
	private String newPassword;
	private JPasswordField confirmField; ///< confirm password
	private String confirmPassword;

	private JTextField drateField;	///< default rate spec (as string)
	private String defRates;
	private JTextField trateField;	///< total rate spec (as string)
	private String totRates;

	private JTextField updateField;	///< used to change target user
	private String updateTarget;
	private JTextField photoField;	///< file name of photograph
	private String photoFile;

	JTextArea statusArea;	///< text area used to display status messages


	JTextArea sessionDisplay; ///< text area used to display sessions
	private JTextField cancelField;
	private String cancelAdr;  ///< string to record canceling forest address
	private String checkDisplay;

	/** Add a label to a text field and align them.
	 *  @param text is the text field
	 *  @param name is the text used to label the text feild
	 */
	private Box labeledField(JTextField text, String name) {
		JLabel label = new JLabel(name);
		label.setBorder(BorderFactory.createEmptyBorder(0,5,0,0));
		text.setMinimumSize(text.getPreferredSize());
		text.setMaximumSize(text.getPreferredSize());
		text.addFocusListener(this);
		label.setAlignmentX(Component.LEFT_ALIGNMENT);
		text.setAlignmentX(Component.LEFT_ALIGNMENT);
		Box box = Box.createVerticalBox();
		box.add(label); box.add(text);
		return box;
	}
		
	/** Create a horizontal box containing a pair of boxes.
	 *  @param b1 is the left box
	 *  @param b2 is the right box
	 */
	private Box doubleBox(Box b1, Box b2) {
		Box b = Box.createHorizontalBox();
			   b.add(Box.createHorizontalStrut(10));
		b.add(b1); b.add(Box.createHorizontalStrut(5));
		b.add(b2);
		return b;
	}

	/** Create a horizontal box containing three of boxes.
	 *  @param b1 is the left box
	 *  @param b2 is the middle box
	 *  @param b3 is the right box
	 */
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
		// assign default values to strings used for contents of
		// text fields
		serverName = userName = password = "foo";
		realName = email = defRates = totRates = "foo";
		newPassword = "foo"; confirmPassword = "bar";
		updateTarget = photoFile = "foo";

		// setup top level
		JFrame jfrm = new JFrame("Forest account manager");
		jfrm.setLayout(new FlowLayout());
		jfrm.setSize(850,400);
		jfrm.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		JPanel contentPane = new JPanel(new BorderLayout());
		jfrm.setContentPane(contentPane);

		// top area with server name connect button
		serverField = new JTextField(20);
		serverField.setText("forest2.arl.wustl.edu");
		serverName = "forest2.arl.wustl.edu";
		Box b1 = labeledField(serverField,"client manager");
		connectBtn = new JButton("connect");
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
		loginBtn = new JButton("login");
		loginBtn.addActionListener(this);
		newAcctBtn = new JButton("new account");
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

		updateField = new JTextField(13);
		updateField.addFocusListener(this);
		updateField.setMaximumSize(updateField.getPreferredSize());
		updateBtn = new JButton("update");
		updateBtn.addActionListener(this);
		photoField = new JTextField(13);
		photoField.setText("photofile.jpg");
		photoField.addFocusListener(this);
		photoField.setMaximumSize(photoField.getPreferredSize());
		uploadBtn = new JButton("upload photo");
		uploadBtn.addActionListener(this);
		modPwdBtn = new JButton("change password");
		modPwdBtn.addActionListener(this);
		Box box4 = Box.createHorizontalBox();
		box4.add(Box.createHorizontalStrut(10));
		box4.add(updateField);
		box4.add(updateBtn);
		box4.add(photoField);
		box4.add(uploadBtn);
		box4.add(Box.createHorizontalGlue());
		box4.add(modPwdBtn);
		box4.add(Box.createHorizontalStrut(25));

		// Finally, the sessions area
		//Overall box structures
		Box box5a = Box.createHorizontalBox();
		Box box5 = Box.createVerticalBox();
		box5.setBorder(BorderFactory.createEmptyBorder(10,13,10,15));

		//Create session text area and add scroll bars to it
		sessionDisplay = new JTextArea(10,60);
		sessionDisplay.setEditable(false);
		sessionDisplay.append("session information will be here");
		JScrollPane scroll = new JScrollPane(sessionDisplay);

		//Create button to retrieve user sessions
		getSessBtn = new JButton("get sessions");
		getSessBtn.addActionListener(this);
		// Create area for forest address and cancel button to 
		// cancel user session
		cancelField = new JTextField(13);
		cancelField.setText("forest ID");
		cancelField.addFocusListener(this);
		cancelBtn = new JButton("cancel");
		cancelBtn.addActionListener(this);
		cancelAllBtn = new JButton("cancel all");
		cancelAllBtn.addActionListener(this);
		getSessBtn.setAlignmentX(Component.LEFT_ALIGNMENT);
		//Finalize box area designs
		box5a.add(getSessBtn);
		box5a.add(cancelField);
		box5a.add(cancelBtn);
		box5a.add(cancelAllBtn);
		box5a.add(Box.createHorizontalStrut(250));
		box5.add(box5a);
		box5.add(scroll);
		
		// Putting it together
		box0.setAlignmentX(Component.LEFT_ALIGNMENT);
		box1a.setAlignmentX(Component.LEFT_ALIGNMENT);
		statusBox.setAlignmentX(Component.LEFT_ALIGNMENT);
		box2.setAlignmentX(Component.LEFT_ALIGNMENT);
		box3.setAlignmentX(Component.LEFT_ALIGNMENT);
		box4.setAlignmentX(Component.LEFT_ALIGNMENT);
		box5.setAlignmentX(Component.LEFT_ALIGNMENT);

		Box boxAll = Box.createVerticalBox();
		boxAll.add(box0); boxAll.add(box1a); boxAll.add(statusBox);
		boxAll.add(box2); boxAll.add(box3); boxAll.add(box4);
		boxAll.add(box5);
		boxAll.setAlignmentX(Component.LEFT_ALIGNMENT);

		JScrollPane scrollPane = new JScrollPane(boxAll);
		jfrm.add(scrollPane);

		jfrm.add(boxAll);
		jfrm.pack();
		jfrm.setVisible(true);

		setupIo();
	}

	/** Display a string in the status area.
	 *  @param s is the string to display; it replaces any old value
	 */
	public void showStatus(String s) {
		statusArea.selectAll(); statusArea.replaceSelection(s);
	}

	/** Respond to user interface events.
	 *  ae is an action event relating to one of the user interface
	 *  elements. Handle buttons only.
	 */
	public void actionPerformed(ActionEvent ae) {
		Object src = ae.getSource();
		if (src == connectBtn) {
			// connect to server
			if (connect(serverName)) {
				showStatus("Success\nEnter user name and " +
					   "password, then login or create " +
					   "new account");
			} else {
				showStatus("Cannot connect");
			}
		} else if (ae.getActionCommand().equals("login")) {
			// login to account
			String status = login(userName,password);
			if (status == null) {
				showStatus("Logged in");
			} else {
				showStatus("Login failed: " + status);
			}
		} else if (ae.getActionCommand().equals("new account")) {
			// create new account
			String status = newAccount(userName,password);
			if (status == null) {
				showStatus("New account created\n" +
					   "Update profile and change " +
				   	   "password below");
			} else {
				showStatus("Error: " + status);
			}
		} else if (ae.getActionCommand().equals("update")) {
			// update a user's profile
			String status = updateProfile(updateTarget);
			if (status == null) {
				showStatus("Profile updated");
			} else {
				showStatus("Cannot update profile: " + status);
			}
		} else if (ae.getActionCommand().equals("change password")) {
			// change a password
			if (!newPassword.equals(confirmPassword)) {
				showStatus("Mismatch. Please re-type password "+
					  "in both new password and confirm " +
					  "password boxes");
			} else {
				String status = changePassword(updateTarget,
							       newPassword);
				if (status == null) {
					showStatus("Password changed");
				} else {
					showStatus("Error: " + status);
				}
			}
		} else if (ae.getActionCommand().equals("upload photo")) {
			// upload a photograph
			String status = uploadPhoto(photoFile);
			if (status == null) {
				showStatus("Photo uploaded");
			} else {
				showStatus("Error: " + status);
			}
		} else if (ae.getActionCommand().equals("get sessions")) {
			//get session information
			String status = getSession(updateTarget);
			if (status == null) {
				showStatus("All active sessions shown below");
			} else {
				showSessions("Error: " + status);
			}
		} else if (ae.getActionCommand().equals("cancel")) {
			//cancel session
			String status = cancelSession(updateTarget, cancelAdr);
			if (status == null) {
				getSession(updateTarget);
			} else {
				showStatus("Cancel session failed: " + status);
			}
		} else if (ae.getActionCommand().equals("cancel all")) {
			//cancel all sessions
			//test for cancel all button working
			String status = cancelAllSessions(updateTarget);
			if (status == null) {
				showSessions("All sessions canceled");
			} else {
				showStatus("Cancel session failed: " + status);
			}
		}
	}

	/** Capture value of text field whenever focus leaves the field.
	 *  @param fe is a focus event; its source is the user interface
	 *  element that just lost focus
	 */
	public void focusLost(FocusEvent fe) {
		Object src = fe.getSource();
		if (src == serverField) {
			serverName = serverField.getText();
		} else if (src == userField) {
			userName = userField.getText();
		} else if (src == pwdField) {
			char[] x = pwdField.getPassword();
			password = new String(x);
		} else if (src == nupwdField) {
			char[] x = nupwdField.getPassword();
			newPassword = new String(x);
		} else if (src == confirmField) {
			char[] x = confirmField.getPassword();
			confirmPassword = new String(x);
		} else if (src == nameField) {
			realName = nameField.getText();
		} else if (src == emailField) {
			email = emailField.getText();
		} else if (src == drateField) {
			defRates = drateField.getText();
		} else if (src == trateField) {
			totRates = trateField.getText();
		} else if (src == updateField) {
			// when focus leaves updateField, retrive the profile
			// for the specified user
			updateTarget = updateField.getText();
			String status = getProfile(updateTarget);
			if (status == null) {
				showStatus("Got target profile");
			} else {
				showStatus("Error: " + status);
				updateField.setText(userName);
				updateTarget = userName;
			}
		} else if (src == photoField) {
			photoFile = photoField.getText();
		} else if (src == cancelField) {
			cancelAdr = cancelField.getText();
		}
	}

	public void focusGained(FocusEvent fe) {
	}

	/** Respond to mouse click in the panel by turning wall on/off.
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

	/** Send a string to the client manager socket.
	 *  @param s is the string to be sent; should use only ASCII characters
	 */
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

	/** Session information object. */
	private class Session {
		int clientAdr;
		int rtrAdr;
		RateSpec rates;
	};
	Session[] sessVec;	///< array of session objects

	/** Show list of sessions with key parameters and cancel button.
	 *  Complete this later. Plan is to show the sessions, one per line,
	 *  with the client's forest address, it's routers address and a
	 *  rate spec. Include a cancel button at the end of the line.
	 *  When pressed, this causes the session to be terminated.
	 */
	public void showSessions(String s) {
		sessionDisplay.selectAll();
		sessionDisplay.replaceSelection(s);
	}

	private String getSession(String userName) {
		if (!sendString("getSessions: " + userName + "\nover\n"))
			return "cannot send request to server";
System.out.println("getSession : " + userName);
		String s = inBuf.readLine();
System.out.println("reply=" + s);
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s == null ? "unexpected response" : s;
		}
		s = inBuf.readLine();
		String sessionList = "";
		while (!s.equals("over")) {
System.out.println("session=" + s);
			sessionList += s + "\n";
			s = inBuf.readLine();
		}
		showSessions(sessionList);
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response" : s;
		return null;
	}

	private String cancelSession(String userName, String cancelAdr) {
                if (!sendString("cancelSession: " + userName + " " +
				 cancelAdr + "\nover\n")) {
                        return "cannot send request to server";
		}
                String s = inBuf.readLine();
                if (s == null || !s.equals("success")) {
                        inBuf.nextLine();
                        return s == null ? "unexpected response" : s;
                }
                s = inBuf.readLine();
                if (s == null || !s.equals("over"))
                        return s == null ? "unexpected response" : s;
                return null;
        }

	private String cancelAllSessions(String userName) {
		if (!sendString("cancelAllSessions: " + userName + "\nover\n"))
                        return "cannot send request to server";
                String s = inBuf.readLine();
                if (s == null || !s.equals("success")) {
                        inBuf.nextLine();
                        return s == null ? "unexpected response" : s;
               	}
                s = inBuf.readLine();
                if (s == null || !s.equals("over"))
                        return s == null ? "unexpected response" : s;
                return null;
	}

	/** Connect to the server for the client manager.
	 *  @return true on success, else false
	 */
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
		if (!serverChan.isConnected()) return false;
// add initial greeting message from ClientMgr so that we can
// detect when we're really talking to server and not just tunnel
		if (!sendString("Forest-login-v1\n")) return false;
		inBuf = new NetBuffer(serverChan,1000);
		return true;
	}

	/** Login to a specific account.
	 *  @param user is the name of the user whose account we're logging into
	 *  @param pwd is the password
	 *  @return null if the operation succeeds, otherwise a string
	 *  containing an error message from the client manager
	 */
	private String login(String user, String pwd) {
		if (userName.length() == 0 || pwd.length() == 0)
			return "missing user name or password";
		if (!sendString("login: " + user + "\n" +
			   	"password: " + pwd + "\nover\n"))
			return "cannot send request to server";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s == null ? "unexpected response" : s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response" : s;

		s = getProfile(user);
		if (s != null) return s;
		updateField.setText(user);
		updateTarget = user;
		return null;
	}

	/** Create a new account.
	 *  @param user is the user name for the account being created
	 *  @param pwd is the password to assign to the account
	 *  @return null if the operation succeeds, otherwise a string
	 *  containing an error message from the client manager
	 */
	private String newAccount(String user, String pwd) {
		if (!sendString("newAccount: " + user + "\n" +
			   	"password: " + pwd + "\nover\n"))
			return "cannot send request to server";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s == null ? "unexpected response" : s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over")) 
			return s == null ? "unexpected response" : s;
		s = getProfile(user);
		if (s != null) return s;
		updateField.setText(user);
		updateTarget = user;
		return null;
	}

	/** Update a client's profile.
	 *  This method sends a request to the client manager and checks
	 *  the response.
	 *  @param userName is the name of the client whose profile is
	 *  to be modified.
	 *  @return null if the operation succeeded, otherwise,
	 *  a string containing an error message from the client manager
	 */
	private String updateProfile(String userName) {
		if (userName.length() == 0) return "empty user name";
		if (!sendString("updateProfile: " + userName + "\n" +
			   	"realName: \"" + realName + "\"\n" +
				"email: " + email + "\n" +
				"defRates: " + defRates + "\n" +
				"totalRates: " + totRates + "\n" +
				"over\n"))
			return "cannot send request";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine(); return s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over")) return s;
		return null;
	}

	/** Change a client's password.
	 *  This method sends a request to the client manager and checks
	 *  the response.
	 *  @param userName is the name of the client whose profile is
	 *  to be modified.
	 *  @param pwd is the new password to be assigned
	 *  @return null if the operation succeeded, otherwise,
	 *  a string containing an error message from the client manager
	 */
	private String changePassword(String userName, String pwd) {
		if (userName.length() == 0 || pwd.length() == 0)
			return "empty user name or password";
		if (!sendString("changePassword: " + userName +
				"\npassword: " + pwd + "\nover\n"))
			return "cannot send request to server";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine(); 
			return s == null ? "unexpected response " : s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response " : s;
		return null;
	}

	/** Get a client's profile.
	 *  This method sends a request to the client manager and processes
	 *  the response. The text fields in the displayed profile are
	 *  updated based on the response.
	 *  @param userName is the name of the client whose profile is
	 *  to be modified
	 *  @return null if the operation succeeded, otherwise,
	 *  a string containing an error message from the client manager
	 */
	private String getProfile(String userName) {
		String s;
		boolean gotName, gotEmail, gotDefRates, gotTotRates; 

		if (userName.length() == 0) return "empty user name";
		if (!sendString("getProfile: " + userName + "\nover\n"))
			return "cannot send request to server";
		gotName = gotEmail = gotDefRates = gotTotRates = false; 
		while (true) {
			s = inBuf.readAlphas();
			if (s == null) {
				// skip
			} else if (s.equals("realName") && inBuf.verify(':')) {
				realName = inBuf.readString();
				if (realName != null)
					nameField.setText(realName);
				else
					nameField.setText("noname");
				gotName = true;
			} else if (s.equals("email") && inBuf.verify(':')) {
				email = inBuf.readWord();
				if (email != null) 
					emailField.setText(email);
				else
					emailField.setText("nomail");
				gotEmail = true;
			} else if (s.equals("defRates") && inBuf.verify(':')) {
				RateSpec rates = inBuf.readRspec();
				if (rates != null) {
					defRates = rates.toString();
					drateField.setText(defRates);
				} else
					drateField.setText("(0,0,0,0)");
				gotDefRates = true;
			} else if (s.equals("totalRates") && inBuf.verify(':')){
				RateSpec rates = inBuf.readRspec();
				if (rates != null) {
					totRates = rates.toString();
					trateField.setText(defRates);
				} else
					trateField.setText("(0,0,0,0)");
				gotTotRates = true;
			} else if (s.equals("over")) {
				inBuf.nextLine();
				if (gotName && gotEmail &&
				    gotDefRates && gotTotRates)
					return null;
				else
					return "incomplete response";
			} else {
				return s == null ? "unexpected response " : s;
			}
			inBuf.nextLine();
		}
	}

	/** Upload a photo to the client manager.
	 *  @param photoFileName is the name of a local file containing
	 *  the photo to be transferred
	 *  @return null on success, otherwise a reference to a String
	 *  object containing an error message
	 */
	public String uploadPhoto(String photoFileName) {
		// check for .jpg extension
		if (!photoFileName.endsWith(".jpg")) {
			return "photo file name must have .jpg suffix";
		}
		// open photo file
		File photoFile;
		FileInputStream photoStream;
		FileChannel photoChan;
		long length;
		try {
			photoFile = new File(photoFileName);
			photoStream = new FileInputStream(photoFile);
			photoChan = photoStream.getChannel();
			length = photoChan.size();
		} catch(Exception e) {
			return "cannot open photo file";
		}

		if (!sendString("uploadPhoto: " + length + "\n"))
			return "could not start transfer";
		String s = inBuf.readLine();
		if (s == null || !s.equals("proceed"))
			return s == null ? "unexpected response " : s;

		ByteBuffer buf = ByteBuffer.allocate(1000);
		int numSent = 0;
		try {
			while (true) {
				int n = photoChan.read(buf);
				if (n <= 0) break;
				buf.flip();
				int m = serverChan.write(buf);
				if (m != n) break;
				numSent += m;
				if (numSent == length) break;
				buf.clear();
			}
			photoChan.close();
		} catch(Exception e) {
			return "failure while transferring photo";
		}
		if (numSent != length) {
			return "could not upload complete file";
		}
		if (!sendString("photo finished\nover\n"))
			return "could not upload complete file";

		s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s == null ? "unexpected response " : s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response " : s;
		return null;
	}
}

