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

	private static int CM_PORT = 30122;
	private SocketChannel serverChan;
	private NetBuffer inBuf;

	private boolean loggedIn;		// true when logged in

	// various user interface buttons
	private JButton connectBtn;
	private JButton loginBtn;
	private JButton newAcctBtn;
	private JButton updateBtn;
	private JButton modPwdBtn;
	private JButton uploadBtn;

	// various text fields and the strings use to hold contents
	private JTextField serverField;
	private String serverName;
	private JTextField userField;
	private String userName;
	private JPasswordField pwdField;
	private String password;

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

	private JTextField updateField;
	private String updateTarget;
	private JTextField photoField;
	private String photoFile;

	// text area used to display status messages
	JTextArea statusArea;

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
		serverName = userName = password = "foo";
		realName = email = defRates = totRates = "foo";
		newPassword = "foo"; confirmPassword = "bar";
		updateTarget = photoFile = "foo";

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

		JScrollPane scrollPane = new JScrollPane(boxAll);
		jfrm.add(scrollPane);

		jfrm.add(boxAll);
		jfrm.pack();
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
		if (s == null || !s.equals("profile updated")) {
			inBuf.nextLine(); return s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over")) return s;
		return null;
	}

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

	private String getProfile(String userName) {
		String s;
		boolean gotName, gotEmail, gotDefRates, gotTotRates; 

		if (userName.length() == 0) return "empty user name";
		if (!sendString("getProfile: " + userName + "\n"))
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

	private String login(String user, String pwd) {
		if (userName.length() == 0 || pwd.length() == 0)
			return "missing user name or password";
		if (!sendString("login: " + user + "\n" +
			   	"password: " + pwd + "\nover\n"))
			return "cannot send request to server";
		String s = inBuf.readLine();
		if (s == null || !s.equals("login successful")) {
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
			} else {
				String status = login(userName,password);
				if (status == null) {
					loggedIn = true;
					showStatus("Logged in");
				} else {
					showStatus("Login failed: " + status);
				}
			}
		} else if (ae.getActionCommand().equals("new account")) {
			String status = newAccount(userName,password);
			if (status == null) {
				showStatus("New account created\n" +
					   "Update profile and change " +
				   	   "password below");
			} else {
				showStatus("Error: " + status);
			}
		} else if (ae.getActionCommand().equals("update")) {
			String status = updateProfile(updateTarget);
			if (status == null) {
				showStatus("Profile updated");
			} else {
				showStatus("Cannot update profile: " + status);
			}
		} else if (ae.getActionCommand().equals("change password")) {
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
			String status = uploadPhoto(photoFile);
			if (status == null) {
				showStatus("Photo uploaded");
			} else {
				showStatus("Error: " + status);
			}
		}
	}

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
		}
	}

	public void focusGained(FocusEvent fe) {
	}

	/** Respond to mouse click in the panel by turning wall on/off.
	 *  @param e is a mouse event corresponding to a click in the panel.
	 */
	public void mouseClicked(MouseEvent e) {
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
		if (s == null || !s.equals("photo received"))
			return s == null ? "unexpected response " : s;
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response " : s;
		return null;
	}
}

