package forest.control.console;

/** @file NetMgrConsole.java
 *
 *  @author Doowon Kim
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.util.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.charset.*;
import java.awt.*;
import java.awt.event.*;

import javax.swing.*;
import javax.swing.table.*;

import forest.common.*;
import forest.control.console.model.*;

public class NetMgrConsole {

	public static final int MAIN_WIDTH = 700;
	public static final int MAIN_HEIGHT = 900;
	public static final int LOGIN_WIDTH = 300;
	public static final int LOGIN_HEIGHT = 150; 
	public static final int COMTREE_MENU_HEIGHT = 50;

	private JFrame mainFrame;
	private JMenuBar menuBar;
	private JMenu menu;
	private JMenuItem connectMenuItem;
	private JMenuItem loginMenuItem;
	private JMenuItem newAccountMenuItem;
	private JMenuItem updateProfileMenuItem;
	private JMenuItem changePwdMenuItem;
	private JMenuItem closeMenuItem;
	
	private JPanel mainPanel;
	//comtree menu
	private JPanel comtreeMenuPanel;
	private JLabel comtreeNameLabel;
	private Integer[] comtrees = {0, 1, 1001, 1002, 1003};;
	private JComboBox<Integer> comtreeComboBox;
	private JLabel isConnectLabel;
	private JLabel loggedInAsLabel;
	//comtree display
	private JPanel comtreeDisplayPanel;
	//router info menu
	private JTable infoTable;
	private JPanel routerInfoMenuPanel;
	private JLabel routerNameLabel;
	private String[] routers = {"r1", "r2"};
	private JComboBox<String> routersComboBox;
	private String[] tables = {"Comtree", "Link", "Iface", "Route"};
	private JComboBox<String> tablesComboBox;
	private JButton routerInfoUpdateButton;
	private JButton routerInfoClearButton;
	//Router Info Table
	private JPanel routerInfoPanel;
	private JTableHeader linkTableHeader;
	private JScrollPane infoTableScrollPane;
	//Log menu
	private JPanel logMenuPanel;
	//log display
	private JPanel logDisplayPanel;
	private JTextField logTextArea;
	private JScrollPane logTextAreaScrollPane;

	private ConnectDialog connectDialog;
	private LoginDialog loginDialog;
	private AdminProfile adminProfile;
	private boolean loggedin = false;
	private UpdateProfileDialog updateProfileDialog;
	private ChangePwdDialog changePwdDialog;
	
	private Charset ascii;
	private CharsetEncoder enc;
	private CharBuffer cb;
	private ByteBuffer bb;
	private SocketAddress serverAddr;
	private SocketChannel serverChan;
	private NetBuffer inBuf;

	private int nmPort = 30120;
	private String nmAddr = "forest1.arl.wustl.edu";

	private String routerName;
	private String table;

	private ComtreeTableModel comtreeTableModel;
	private LinkTableModel linkTableModel;
	private IfaceTableModel ifaceTableModel;
	private RouteTableModel routeTableModel;

	public NetMgrConsole(){
		//this.nmPort = nmPort;
		//this.nmAddr = nmAddr;

		mainFrame = new JFrame();
		mainFrame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
		mainFrame.setTitle("Net Manager Console");
		mainFrame.setPreferredSize(new Dimension(LOGIN_WIDTH, LOGIN_HEIGHT));

		menuBar = new JMenuBar();
		initMenuBar();
		mainFrame.setJMenuBar(menuBar);

		connectDialog = new ConnectDialog(nmAddr);
		loginDialog = new LoginDialog();
		updateProfileDialog = new UpdateProfileDialog();
		changePwdDialog = new ChangePwdDialog();
		adminProfile = new AdminProfile();

		setupIo(); //initial Connection to NetMgr

		displayNetMgr();

		mainFrame.pack();
		mainFrame.setVisible(true);
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

	private boolean connect(String server) {
		// open channel to server
		// on success, return true
		try {
			SocketAddress serverAdr = new InetSocketAddress(InetAddress.getByName(server), nmPort);
			// connect to server
			serverChan = SocketChannel.open(serverAdr);
		} catch(Exception e) {
			return false;
		}
		if (!serverChan.isConnected()) return false;
		// add initial greeting message from NetMgr so that we can
		// detect when we're really talking to server and not just tunnel
		if (!sendString("Forest-Console-v1\n")) return false;
		inBuf = new NetBuffer(serverChan,1000);
		return true;
	}

	/** Login to a specific account.
	 *  @param user is the name of the user whose account we're logging into
	 *  @param pwd is the password
	 *  @return null if the operation succeeds, otherwise a string
	 *  containing an error message from the client manager
	 */
	private String login(String user, String pwd){
		if (user.length() == 0 || pwd.length() == 0)
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
		System.out.println(adminProfile);
		loggedInAsLabel.setText(" Logged in as " + user);
		return null;
	}

	/** Get a admin's profile.
	 *  This method sends a request to the net manager and processes
	 *  the response. The text fields in the displayed profile are
	 *  updated based on the response.
	 *  @param userName is the name of the admin whose profile is
	 *  to be modified
	 *  @return null if the operation succeeded, otherwise,
	 *  a string containing an error message from the net manager
	 */
	private String getProfile(String userName) {
		String s;
		boolean gotName, gotEmail;
		adminProfile.setUserName(userName);

		if (userName.length() == 0) return "empty user name";
		if (!sendString("getProfile: " + userName + "\nover\n"))
			return "cannot send request to server";
		gotName = gotEmail = false;

		while (true) {
			s = inBuf.readAlphas();
			if (s == null) {
				// skip
			} else if (s.equals("realName") && inBuf.verify(':')) {
				String realName = inBuf.readString();
				if (realName != null)
					adminProfile.setRealName(realName);
				else
					adminProfile.setRealName("noname");
				gotName = true;
			} else if (s.equals("email") && inBuf.verify(':')) {
				String email = inBuf.readWord();
				if (email != null) 
					adminProfile.setEmail(email);
				else
					adminProfile.setEmail("nomail");
				gotEmail = true;
			}  else if (s.equals("over")) {
				inBuf.nextLine();
				if (gotName && gotEmail)
					return null;
				else
					return "incomplete response";
			} else {
				return s == null ? "unexpected response " : s;
			}
			inBuf.nextLine();
		}
	}

	/** Create a new account.
	 *  @param user is the user name for the account being created
	 *  @param pwd is the password to assign to the account
	 *  @return null if the operation succeeds, otherwise a string
	 *  containing an error message from the net manager
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
//		s = getProfile(user);
//		if (s != null) return s;
		return null;
	}

	/** Update a admin's profile.
	 *  This method sends a request to the net manager and checks
	 *  the response.
	 *  @param userName is the name of the admin whose profile is
	 *  to be modified.
	 *  @return null if the operation succeeded, otherwise,
	 *  a string containing an error message from the net manager
	 */
	private String updateProfile(String userName, String realName, String email) {
		if (userName.length() == 0) return "empty user name";
		if (!sendString("updateProfile: " + userName + "\n" +
			   	"realName: \"" + realName + "\"\n" +
				"email: " + email + "\n" + "over\n"))
			return "cannot send request";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine(); return s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over")) return s;
		return null;
	}
	
	/** Change an admin's password.
	 *  This method sends a request to the net manager and checks
	 *  the response.
	 *  @param userName is the name of the admin whose profile is
	 *  to be modified.
	 *  @param pwd is the new password to be assigned
	 *  @return null if the operation succeeded, otherwise,
	 *  a string containing an error message from the net manager
	 */
	private String changePassword(String userName, String pwd) {
		if (userName.length() == 0 || pwd.length() == 0)
			return "empty user name or password";
		if (!sendString("changePassword: " + userName + " "
				+ pwd + "\nover\n"))
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

	/**
	 * Initialize menu bar 
	 */
	private void initMenuBar(){
		menu = new JMenu("Menu");
		menuBar.add(menu);

		connectMenuItem = new JMenuItem("Connect");
		connectMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				String[] options = {"Connect", "Cancel"};
				int n = JOptionPane.showOptionDialog(mainFrame, connectDialog, "Connect to the server", 
														JOptionPane.OK_CANCEL_OPTION, 
														JOptionPane.INFORMATION_MESSAGE, 
														null, options, options[0]);
				if (n == 0){ //connect
					if(connect(nmAddr)){
						String addr = connectDialog.getAddr();
						isConnectLabel.setText("Connected to \"" + addr + "\"");
						showPopupStatus("Connected");
					} else{
						showPopupStatus("Connection is failed.");
					}
				}
			}
		});
		menu.add(connectMenuItem);
		
		menu.addSeparator();
		loginMenuItem = new JMenuItem("Login");
		loginMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				String[] options = {"Login", "Cancel"};
				int n = JOptionPane.showOptionDialog(mainFrame, loginDialog, "Login", 
														JOptionPane.OK_CANCEL_OPTION, 
														JOptionPane.INFORMATION_MESSAGE, 
														null, options, options[0]);
				if (n == 0){ //login
					char[] passwd = loginDialog.getPassword();
					String userName = loginDialog.getUserName();
					String ret = login(userName, new String(passwd));
					if(ret == null){
						showPopupStatus("Logged in as " + userName);
						loggedin = true;
						loginMenuItem.setText("Log out");
					}
					else
						showPopupStatus(ret);
				}
			}
		});
		menu.add(loginMenuItem);

		menu.addSeparator();
		newAccountMenuItem = new JMenuItem("New Account");
		newAccountMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				if(loggedin){
					String[] options = {"Add", "Cancel"};
					int n = JOptionPane.showOptionDialog(mainFrame, loginDialog, "Add a new account", 
															JOptionPane.OK_CANCEL_OPTION, 
															JOptionPane.INFORMATION_MESSAGE, 
															null, options, options[0]);
					if (n == 0){ //add
						char[] passwd = loginDialog.getPassword();
						String userName = loginDialog.getUserName();
						String ret = newAccount(userName, new String(passwd));
						if(ret == null)
							showPopupStatus("New account created");
						else
							showPopupStatus(ret);
					}
				} else{
					showPopupStatus("Please login first");
				}
			}
		});
		menu.add(newAccountMenuItem);

		updateProfileMenuItem = new JMenuItem("Update Profile");
		updateProfileMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				if(loggedin){
					String[] options = {"Update", "Cancel"};
					updateProfileDialog.setUserName(adminProfile.getUserName());
					updateProfileDialog.setRealName(adminProfile.getRealName());
					updateProfileDialog.setEmail(adminProfile.getEmail());
					
					int n = JOptionPane.showOptionDialog(mainFrame, updateProfileDialog, "Update Profile", 
															JOptionPane.OK_CANCEL_OPTION, 
															JOptionPane.INFORMATION_MESSAGE, 
															null, options, options[0]);
					if (n == 0){ //update
						String userName = adminProfile.getUserName();
						String realName = updateProfileDialog.getRealName();
						String email = updateProfileDialog.getEmail();
						String ret = updateProfile(userName, realName, email);
						if(ret == null){
							getProfile(userName);
							showPopupStatus("The account profile updated");
						}
						else
							showPopupStatus(ret);
					}
				} else{
					showPopupStatus("Please login first");
				}
			}
		});
		menu.add(updateProfileMenuItem);

		changePwdMenuItem = new JMenuItem("Change Password");
		changePwdMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				if(loggedin){
					String[] options = {"Change", "Cancel"};
					changePwdDialog.setUserName(adminProfile.getUserName());
					int n = JOptionPane.showOptionDialog(mainFrame, changePwdDialog, "Change password", 
															JOptionPane.OK_CANCEL_OPTION, 
															JOptionPane.INFORMATION_MESSAGE, 
															null, options, options[0]);
					if (n == 0){ //change
						String userName = adminProfile.getUserName();
						char[] password = changePwdDialog.getPassword();
						char[] vPassword = changePwdDialog.getVerifyPassword();
						if(Arrays.equals(password, vPassword)){
							String ret = changePassword(userName, new String(password));
							if(ret == null){
								showPopupStatus("The password changed");
							}
							else
								showPopupStatus(ret);
						} else{
							showPopupStatus("Passwords are not matched");
						}
					}
				} else{
					showPopupStatus("Please login first");
				}
			}
		});
		menu.add(changePwdMenuItem);

		menu.addSeparator();
		closeMenuItem = new JMenuItem("Close");
		closeMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				System.exit(0);
			}
		});
		menu.add(closeMenuItem);
	}

	/**
	 * Display Net Manager Console
	 */
	private void displayNetMgr(){
		comtreeTableModel = new ComtreeTableModel();
		linkTableModel = new LinkTableModel();
		ifaceTableModel = new IfaceTableModel();
		routeTableModel = new RouteTableModel();

		mainPanel = new JPanel();
		BoxLayout boxLayout = new BoxLayout(mainPanel, BoxLayout.Y_AXIS);
		mainPanel.setLayout(boxLayout);
		mainFrame.getContentPane().add(mainPanel, BorderLayout.PAGE_START);
		mainFrame.setPreferredSize(new Dimension(MAIN_WIDTH, MAIN_HEIGHT));


		//Comtree Menu
		comtreeMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		comtreeMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		comtreeMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 45));
		comtreeMenuPanel.setMaximumSize(comtreeMenuPanel.getPreferredSize());
		comtreeNameLabel = new JLabel("ComTree:");
		comtreeMenuPanel.add(comtreeNameLabel);
		comtreeComboBox = new JComboBox<Integer>(comtrees);
		comtreeMenuPanel.add(comtreeComboBox);
		isConnectLabel = new JLabel(" Not connected to \"" + nmAddr + "\"");
		comtreeMenuPanel.add(isConnectLabel);
		comtreeMenuPanel.add(new JLabel(" & "));
		loggedInAsLabel = new JLabel(" Not logged in");
		comtreeMenuPanel.add(loggedInAsLabel);


		//ComTree Display
		comtreeDisplayPanel = new JPanel(); 
		comtreeDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
		comtreeDisplayPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 400));


		//Router Info Menu
		infoTable = new JTable(comtreeTableModel); //initially comtreeTableModel
		routerInfoMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		routerInfoMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		routerInfoMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 45));
		routerInfoMenuPanel.setMaximumSize(routerInfoMenuPanel.getPreferredSize());
		routerNameLabel = new JLabel("Router:");
		routerInfoMenuPanel.add(routerNameLabel);
		routersComboBox = new JComboBox<String>(routers); //router selection
		routerInfoMenuPanel.add(routersComboBox);

		tablesComboBox = new JComboBox<String>(tables); //info selection
		tablesComboBox.addItemListener(new ItemListener(){
			public void itemStateChanged(ItemEvent e){
				if (e.getStateChange() == 1){
					if(e.getItem().equals("Comtree")) {
						infoTable.setModel(comtreeTableModel);
					}
					else if(e.getItem().equals("Link")) {
						infoTable.setModel(linkTableModel);
					}
					else if(e.getItem().equals("Iface")) {
						infoTable.setModel(ifaceTableModel);
					}
					else if(e.getItem().equals("Route")) {
						infoTable.setModel(routeTableModel);
					}
					setPreferredWidth(infoTable);
				}
			}
		});
		routerInfoMenuPanel.add(tablesComboBox);  
		routerInfoUpdateButton = new JButton("Update");
		routerInfoUpdateButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				table = tablesComboBox.getSelectedItem().toString();
				routerName = routersComboBox.getSelectedItem().toString();
				if (table.equals("Link")){
					if(!sendString("getLinkTable: " + routerName +"\n")){
						JOptionPane.showMessageDialog(null, "Error at sending a message to NetMgr");
					}
					linkTableModel.clear();
					while(true){
						String s = inBuf.readLine();
						System.out.println(s);
						if(s.equals("over")){
							System.out.println("Over!");
							break;
						} else {
							String[] tokens = s.split(" ");
							linkTableModel.addLinkTable(new LinkTable(tokens[0], tokens[1], 
									new String(tokens[2] + ":" + tokens[3]), tokens[4], tokens[5], tokens[6]));
							linkTableModel.fireTableDataChanged();
						}
					}
				} else if (table.equals("Comtree")) {

				} else if (table.equals("Iface")) {

				} else if (table.equals("Route")) {

				}
			}
		});
		routerInfoClearButton = new JButton("Clear");
		routerInfoClearButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				linkTableModel.clear();
				linkTableModel.fireTableDataChanged();
			}
		});
		routerInfoMenuPanel.add(routerInfoUpdateButton);
		routerInfoMenuPanel.add(routerInfoClearButton);


		//Link Table
		routerInfoPanel = new JPanel();
		routerInfoPanel.setBorder(BorderFactory.createTitledBorder(""));

		linkTableHeader = infoTable.getTableHeader();
		linkTableHeader.setDefaultRenderer(new HeaderRenderer(infoTable));
		infoTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF); //size
		DefaultTableCellRenderer centerRenderer = new DefaultTableCellRenderer();
		centerRenderer.setHorizontalAlignment(JLabel.CENTER);
		for ( int i = 0; i < infoTable.getModel().getColumnCount(); ++i){
			infoTable.getColumnModel().getColumn(i).setCellRenderer(centerRenderer);
		}
		infoTable.setFillsViewportHeight(true);

		infoTableScrollPane = new JScrollPane(infoTable);
		infoTableScrollPane.setPreferredSize(new Dimension(MAIN_WIDTH, 100));
		routerInfoPanel.add(infoTableScrollPane);

		//Log Menu
		logMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		logMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		logMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 45));
		logMenuPanel.setMaximumSize(logMenuPanel.getPreferredSize());  
		JLabel tmpLabel = new JLabel("r1");
		logMenuPanel.add(tmpLabel);

		//Log Display
		logDisplayPanel = new JPanel();
		logDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
		logTextArea = new JTextField();
		logTextArea.setPreferredSize(new Dimension(MAIN_WIDTH, 100));
		logTextArea.setEditable(true);
		logTextAreaScrollPane = new JScrollPane(logTextArea);
		logTextAreaScrollPane.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
		logDisplayPanel.add(logTextAreaScrollPane);

		mainPanel.add(comtreeMenuPanel);
		mainPanel.add(comtreeDisplayPanel);
		mainPanel.add(routerInfoMenuPanel);
		mainPanel.add(routerInfoPanel);
		mainPanel.add(logMenuPanel);
		mainPanel.add(logDisplayPanel);

		mainFrame.add(mainPanel);
		mainFrame.pack();
		mainFrame.repaint();
	}

	/**
	 * Set preferred column header's width 
	 * @param infoTable information table
	 */
	private void setPreferredWidth(JTable infoTable){
		for(int i = 0 ; i < infoTable.getModel().getColumnCount() ; ++i){
			// int j = (infoTable.getModel().getWidth(i);
			// infoTable.getColumnModel().getColumn(i).setPreferredWidth((int)(MAIN_WIDTH * j * 0.1));
		}
	}

	/**
	 * Send a string msg to Net Manager
	 * @param  msg message
	 * @return successful
	 */
	private boolean sendString(String msg){
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

	/**
	 * This class is for centering table's header value.
	 * Reference: http://stackoverflow.com/questions/7493369/jtable-right-align-header
	 */
	private static class HeaderRenderer implements TableCellRenderer {
		DefaultTableCellRenderer renderer;
		public HeaderRenderer(JTable table) {
			renderer = (DefaultTableCellRenderer)
					table.getTableHeader().getDefaultRenderer();
			renderer.setHorizontalAlignment(JLabel.CENTER);
		}
		@Override
		public Component getTableCellRendererComponent(
				JTable table, Object value, boolean isSelected,
				boolean hasFocus, int row, int col) {
			return renderer.getTableCellRendererComponent(
					table, value, isSelected, hasFocus, row, col);
		}
	}

	public void showPopupStatus(String status){
		JOptionPane.showMessageDialog(mainFrame, status);
	}

	public static void main(String[] args) {
		final String nmAddr = args[0];
		final int nmPort = Integer.parseInt(args[1]);
		SwingUtilities.invokeLater(new Runnable() {
			public void run() { new NetMgrConsole(); }
		});
	}
}

