package forest.control.console;

/** @file NetMgrConsole.java
 *
 *  @author Doowon Kim
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.util.*;
import java.awt.*;
import java.awt.event.*;

import javax.swing.*;
import javax.swing.border.BevelBorder;
import javax.swing.table.*;

import forest.control.ComtInfo;
import forest.control.NetInfo;
import forest.control.console.model.*;

public class NetMgrConsole {

	public static final int MAIN_WIDTH = 1400;
	public static final int MAIN_HEIGHT = 700;
	public static final int MENU_HEIGHT = 50;

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
	private JPanel statusPanel;
	private JLabel statusLabel;
	private JLabel statusLabel2;
	private JLabel statusLabel3;
	
	//comtree menu
	private JPanel comtreeMenuPanel;
	private JLabel comtreeNameLabel;
	private Vector<Integer> comtreeComboBoxItems;
	private DefaultComboBoxModel comtreeComboBoxModel;
	private JComboBox<Vector<Integer>> comtreeComboBox;
	private JButton comtreeDisUpdateBtn;
	private JButton comtreeDisClearBtn;
	private JCheckBox refreshCheckBox;

	//comtree display
	private ComtreeDisplay comtreeDisplayPanel;
	//router info menu
	private JTable infoTable;
	private JPanel routerInfoMenuPanel;
	private JLabel routerNameLabel;
	private Vector<String> routerItems;
	private DefaultComboBoxModel routerComboBoxModel;
	private JComboBox<Vector<String>> routerComboBox;
	private String[] tables = {"Comtree", "Link", "Iface", "Route"};
	private JComboBox<String> tablesComboBox;
	private JButton routerInfoUpdateButton;
	private JButton routerInfoClearButton;
	//Router Info Table
	private JPanel routerInfoPanel;
	private JTableHeader tableHeader;
	private DefaultTableCellRenderer centerRenderer;
	private JScrollPane infoTableScrollPane;
	//Log menu
	private JPanel logMenuPanel;
	private JPanel logMenuPanel2;
	private Vector<String> routerForLogItems;
	private DefaultComboBoxModel routerForLogComboBoxModel;
	private JComboBox<Vector<String>> routerForLogComboBox;
	private JTextField linkTextField;
	private JTextField comtreeTextField;
	private JCheckBox inCheckBox;
	private JCheckBox outCheckBox;
	private JButton onOffLogBtn;
	private JButton updateLogBtn;
	private JButton clearLogBtn;
	private String[] forestType = {"ALL", "UNDEF_PKT", "SUB_UNSUB", 
			"CLIENT_SIG", "CONNECT", "DISCONNECT", "NET_SIG", 
			"RTE_REPLY", "RTR_CTL", "VOQSTATUS"
	};
	private JComboBox<String> forestTypeComboBox;
	private String[] cpType = {"ALL", 
			"UNDEF_CPTYPE", 
			
			"CLIENT_ADD_COMTREE", "CLIENT_DROP_COMTREE", "CLIENT_GET_COMTREE", 
			"CLIENT_MOD_COMTREE",
			"CLIENT_JOIN_COMTREE", "CLIENT_LEAVE_COMTREE", "CLIENT_RESIZE_COMTREE",
			"CLIENT_GET_LEAF_RATE", "CLIENT_MOD_LEAF_RATE", 
			
			"CLIENT_NET_SIG_SEP",
			
			"ADD_IFACE", "DROP_IFACE", "GET_IFACE", "MOD_IFACE", 
			
			"ADD_LINK", "DROP_LINK", "GET_LINK", "MOD_LINK",
			"GET_LINK_SET",
			
			"ADD_COMTREE", "DROP_COMTREE", "GET_COMTREE",
			"MOD_COMTREE",
			
			"ADD_COMTREE_LINK", "DROP_COMTREE_LINK",
			"MOD_COMTREE_LINK", "GET_COMTREE_LINK",
			"GET_COMTREE_SET",
			
			"GET_IFACE_SET", "GET_ROUTE_SET",
			
			"ADD_ROUTE", "DROP_ROUTE", "GET_ROUTE", "MOD_ROUTE",
			"ADD_ROUTE_LINK", "DROP_ROUTE_LINK",
			
			"NEW_SESSION", "CANCEL_SESSION",
			"CLIENT_CONNECT", "CLIENT_DISCONNECT",
			
			"SET_LEAF_RANGE", "CONFIG_LEAF",
			
			"BOOT_ROUTER", "BOOT_COMPLETE", "BOOT_ABORT",
			"BOOT_LEAF"
	};

	private JComboBox<String> cpTypeComboBox;
	
	//log display
	private JPanel logDisplayPanel;
	private JTextField logTextArea;
	private JScrollPane logTextAreaScrollPane;

	private ConnectDialog connectDialog;
	private LoginDialog loginDialog;
	private UpdateProfileDialog updateProfileDialog;
	private ChangePwdDialog changePwdDialog;

	private ConnectionNetMgr connectionNetMgr;
	private ConnectionComtCtl connectionComtCtl;
	private int nmPort = 30120;
//	private String nmAddr = "forest1.arl.wustl.edu";

	//Comtree
	TreeSet<Integer> comtSet;
	private String routerName;
	private String table;

	private ComtreeTableModel comtreeTableModel;
	private LinkTableModel linkTableModel;
	private IfaceTableModel ifaceTableModel;
	private RouteTableModel routeTableModel;

	private boolean isLoggedin = false;
	private boolean isConnected = false;
	private boolean isLogOn = false;
	
	public NetMgrConsole(){

		mainFrame = new JFrame();
		mainFrame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
		mainFrame.setTitle("Net Manager Console");

		statusPanel = new JPanel();
		statusPanel.setBorder(new BevelBorder(BevelBorder.LOWERED));
		mainFrame.add(statusPanel, BorderLayout.SOUTH);
		statusPanel.setPreferredSize(new Dimension(mainFrame.getWidth(), 20));
		statusPanel.setLayout(new BoxLayout(statusPanel, BoxLayout.X_AXIS));

		menuBar = new JMenuBar();
		initMenuBar();
		mainFrame.setJMenuBar(menuBar);

		comtreeComboBoxItems = new Vector<Integer>();
		comtreeComboBoxModel = new DefaultComboBoxModel(comtreeComboBoxItems);
		routerItems = new Vector<String>();
		routerComboBoxModel = new DefaultComboBoxModel(routerItems);
		routerForLogItems = new Vector<String>();
		routerForLogComboBoxModel = new DefaultComboBoxModel(routerForLogItems);
		
		connectDialog = new ConnectDialog();
		loginDialog = new LoginDialog();
		updateProfileDialog = new UpdateProfileDialog();
		changePwdDialog = new ChangePwdDialog();

		connectionNetMgr = new ConnectionNetMgr();
		connectionNetMgr.setNmPort(nmPort);//set NetMgr port number
		connectionComtCtl = new ConnectionComtCtl();

		displayNetMgr();

		mainFrame.pack();
		mainFrame.setVisible(true);
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
				int n = JOptionPane.showOptionDialog(mainFrame, connectDialog, 
														"Connect to the server", 
														JOptionPane.OK_CANCEL_OPTION, 
														JOptionPane.INFORMATION_MESSAGE, 
														null, options, options[0]);
				if (n == 0){ //connect
					String nmAddr = connectDialog.getNmAddr();
					connectionNetMgr.setNmAddr(nmAddr);
					String ctAddr = connectDialog.getCtAddr();
					if(connectionNetMgr.connectToNetMgr() && connectionComtCtl.connectToComCtl()){
						isConnected = true;
						//connect to NetMgr
						connectionComtCtl.getNet();
						connectionComtCtl.getComtSet();
						comtSet = connectionComtCtl.getComtTreeSet();
						if(comtSet.size() <= 0){
							showPopupStatus("No Comtrees");
						} else{
							for(Integer c : comtSet){
								comtreeComboBoxModel.addElement(c);
							}
							statusLabel.setText("Connected to \"" + nmAddr + "\"" + " and \"" + 
													ctAddr + "\"");
							showPopupStatus("Connected");
							
							//retrieving routers' name
							NetInfo netInfo = connectionComtCtl.getNetInfo();
							for (int node = netInfo.firstNode(); node != 0; 
									node = netInfo.nextNode(node)){
								String name = netInfo.getNodeName(node);
								if(name.substring(0, 1).equals("r"))
									routerComboBoxModel.addElement(name);
								routerForLogComboBoxModel.addElement(name);
							}
						}
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
					String ret = connectionNetMgr.login(userName, new String(passwd));
					if(ret == null){
						showPopupStatus("Logged in as " + userName);
						statusLabel2.setText(" Logged in as " + userName);
						isLoggedin = true;
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
				if(isLoggedin){
					String[] options = {"Add", "Cancel"};
					int n = JOptionPane.showOptionDialog(mainFrame, loginDialog, 
															"Add a new account", 
															JOptionPane.OK_CANCEL_OPTION, 
															JOptionPane.INFORMATION_MESSAGE, 
															null, options, options[0]);
					if (n == 0){ //add
						char[] passwd = loginDialog.getPassword();
						String userName = loginDialog.getUserName();
						String ret = connectionNetMgr.newAccount(userName, new String(passwd));
						if(ret == null)
							showPopupStatus("New account created");
						else
							showPopupStatus(ret);
					}
				} else{
					showPopupStatus("login required");
				}
			}
		});
		menu.add(newAccountMenuItem);

		updateProfileMenuItem = new JMenuItem("Update Profile");
		updateProfileMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				if(isLoggedin){
					String[] options = {"Update", "Cancel"};
					AdminProfile adminProfile = connectionNetMgr.getAdminProfile();
					updateProfileDialog.setUserName(adminProfile.getUserName());
					updateProfileDialog.setRealName(adminProfile.getRealName());
					updateProfileDialog.setEmail(adminProfile.getEmail());
					
					int n = JOptionPane.showOptionDialog(mainFrame, updateProfileDialog, 
															"Update Profile", 
															JOptionPane.OK_CANCEL_OPTION, 
															JOptionPane.INFORMATION_MESSAGE, 
															null, options, options[0]);
					if (n == 0){ //update
						String userName = adminProfile.getUserName();
						String realName = updateProfileDialog.getRealName();
						String email = updateProfileDialog.getEmail();
						String ret = connectionNetMgr.updateProfile(userName, realName, email);
						if(ret == null){
							connectionNetMgr.getProfile(userName);
							showPopupStatus("The account profile updated");
						}
						else
							showPopupStatus(ret);
					}
				} else{
					showPopupStatus("login required");
				}
			}
		});
		menu.add(updateProfileMenuItem);

		changePwdMenuItem = new JMenuItem("Change Password");
		changePwdMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent evt) {
				if(isLoggedin){
					String[] options = {"Change", "Cancel"};
					AdminProfile adminProfile = connectionNetMgr.getAdminProfile();
					changePwdDialog.setUserName(adminProfile.getUserName());
					int n = JOptionPane.showOptionDialog(mainFrame, changePwdDialog, 
															"Change password", 
															JOptionPane.OK_CANCEL_OPTION, 
															JOptionPane.INFORMATION_MESSAGE, 
															null, options, options[0]);
					if (n == 0){ //change
						String userName = adminProfile.getUserName();
						char[] password = changePwdDialog.getPassword();
						char[] vPassword = changePwdDialog.getVerifyPassword();
						if(Arrays.equals(password, vPassword)){
							String ret = connectionNetMgr.changePassword(userName, 
																	new String(password));
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
					showPopupStatus("login required");
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
		
		comtreeDisplayPanel = new ComtreeDisplay(connectionComtCtl);

		mainPanel = new JPanel();
		BoxLayout boxLayout = new BoxLayout(mainPanel, BoxLayout.X_AXIS);
		mainPanel.setLayout(boxLayout);
		mainFrame.getContentPane().add(mainPanel, BorderLayout.PAGE_START);
		mainFrame.setPreferredSize(new Dimension(MAIN_WIDTH, MAIN_HEIGHT));


		//Comtree Menu
		JPanel comtreePanel = new JPanel();
		comtreePanel.setLayout(new BoxLayout(comtreePanel, BoxLayout.Y_AXIS));
		comtreePanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MAIN_HEIGHT));
		
		comtreeMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		comtreeMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		comtreeMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MENU_HEIGHT));
		comtreeMenuPanel.setMaximumSize(comtreeMenuPanel.getPreferredSize());
		comtreeNameLabel = new JLabel("ComTree:");
		comtreeMenuPanel.add(comtreeNameLabel);
		comtreeComboBox = new JComboBox<Vector<Integer>>(comtreeComboBoxModel); //combobox
		comtreeMenuPanel.add(comtreeComboBox);
		comtreeDisUpdateBtn = new JButton("Update"); //button
		comtreeDisUpdateBtn.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				if(isLoggedin){
					String c = comtreeComboBox.getSelectedItem().toString();
					if(c == null){
						showPopupStatus("Choose one of comtrees");
					} else{
						int ccomt = Integer.parseInt(c);
						if(ccomt != 0){
							comtreeDisplayPanel.clearUI();
							comtreeDisplayPanel.autoUpdateDisplay(ccomt);
						}
					}
				} else{
					showPopupStatus("login required");
				}
			}
		});
		comtreeMenuPanel.add(comtreeDisUpdateBtn);
		comtreeDisClearBtn = new JButton("Clear"); //button
		comtreeDisClearBtn.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				if(isLoggedin){
					comtreeDisplayPanel.clearUI();
				} else{
					showPopupStatus("login required");
				}
			}
		});
		comtreeMenuPanel.add(comtreeDisClearBtn);
		refreshCheckBox = new JCheckBox("refresh");//refresh button
		refreshCheckBox.addItemListener(new ItemListener() {
            @Override
            public void itemStateChanged(ItemEvent e) {
                if(e.getStateChange() == ItemEvent.SELECTED){
                	connectionComtCtl.setAutoRefresh(true);
                	if(isLoggedin && isConnected){
						String c = comtreeComboBox.getSelectedItem().toString();
						if(c == null){
							showPopupStatus("Choose one of comtrees");
						} else{
							int ccomt = Integer.parseInt(c);
							if(ccomt != 0){
								comtreeDisplayPanel.clearUI();
								comtreeDisplayPanel.autoUpdateDisplay(ccomt);
							}
						}
                	}
                }
                else
                	connectionComtCtl.setAutoRefresh(false);
            }
        });
		refreshCheckBox.setSelected(true);
		comtreeMenuPanel.add(refreshCheckBox);
		statusLabel = new JLabel(" Not connected"); //status label
		statusLabel2 = new JLabel(" Not logged in");

		statusPanel.add(statusLabel);
		statusPanel.add(new JLabel(" & "));
		statusPanel.add(statusLabel2);

		//ComTree Display
//		comtreeDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
		comtreeDisplayPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, 
															MAIN_HEIGHT - MENU_HEIGHT));
		
		comtreePanel.add(comtreeMenuPanel);
		comtreePanel.add(comtreeDisplayPanel);


		//Router Info Menu
		JPanel routerPanel = new JPanel();
		routerPanel.setLayout(new BoxLayout(routerPanel, BoxLayout.Y_AXIS));
		routerPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MAIN_HEIGHT));
		
		infoTable = new JTable(comtreeTableModel); //initially comtreeTableModel
		centerRenderer = new DefaultTableCellRenderer();
		centerRenderer.setHorizontalAlignment(JLabel.CENTER);
		setPreferredWidth(infoTable);
		routerInfoMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		routerInfoMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		routerInfoMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MENU_HEIGHT));
		routerInfoMenuPanel.setMaximumSize(routerInfoMenuPanel.getPreferredSize());
		routerNameLabel = new JLabel("Router:");
		routerInfoMenuPanel.add(routerNameLabel);
		routerComboBox = new JComboBox<Vector<String>>(routerComboBoxModel); //router combobox
		routerInfoMenuPanel.add(routerComboBox);

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
				if(isLoggedin){
					table = tablesComboBox.getSelectedItem().toString();
					routerName = routerComboBox.getSelectedItem().toString();
					String s = null;
					if (table.equals("Link")){
						s = connectionNetMgr.getTable(linkTableModel, routerName);
					} else if (table.equals("Comtree")) {
						s = connectionNetMgr.getTable(comtreeTableModel, routerName);
					} else if (table.equals("Iface")) {
						s = connectionNetMgr.getTable(ifaceTableModel, routerName);
					} else if (table.equals("Route")) {
						s = connectionNetMgr.getTable(routeTableModel, routerName);
					}
					if(s != null)
						showPopupStatus(s);
				} else{
					showPopupStatus("login required");
				}
			}
		});
		routerInfoClearButton = new JButton("Clear");
		routerInfoClearButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				linkTableModel.clear();
				ifaceTableModel.clear();
				comtreeTableModel.clear();
				routeTableModel.clear();
				linkTableModel.fireTableDataChanged();
				ifaceTableModel.fireTableDataChanged();
				comtreeTableModel.fireTableDataChanged();
				routeTableModel.fireTableDataChanged();
				
			}
		});
		routerInfoMenuPanel.add(routerInfoUpdateButton);
		routerInfoMenuPanel.add(routerInfoClearButton);


		//Link Table
		routerInfoPanel = new JPanel();
		routerInfoPanel.setBorder(BorderFactory.createTitledBorder(""));

		tableHeader = infoTable.getTableHeader();
		tableHeader.setDefaultRenderer(new HeaderRenderer(infoTable));
		infoTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF); //size
		infoTable.setFillsViewportHeight(true);

		infoTableScrollPane = new JScrollPane(infoTable);
		infoTableScrollPane.setPreferredSize(new Dimension(MAIN_WIDTH/2, 300));
		routerInfoPanel.add(infoTableScrollPane);

		//Log Display
		logDisplayPanel = new JPanel();
//		logDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
		logTextArea = new JTextField();
		logTextArea.setPreferredSize(new Dimension(MAIN_WIDTH/2, MAIN_HEIGHT - 
													(MENU_HEIGHT*3 + 300)));
		logTextArea.setEditable(true);
		logTextAreaScrollPane = new JScrollPane(logTextArea);
		logTextAreaScrollPane.setVerticalScrollBarPolicy(ScrollPaneConstants.
															VERTICAL_SCROLLBAR_ALWAYS);
		logDisplayPanel.add(logTextAreaScrollPane);
		
		//Log Menu
		logMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		logMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		logMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MENU_HEIGHT));
		logMenuPanel.setMaximumSize(logMenuPanel.getPreferredSize());  
		
		routerForLogComboBox = new JComboBox<Vector<String>>(routerForLogComboBoxModel); //router selection
		logMenuPanel.add(routerForLogComboBox);
		inCheckBox = new JCheckBox("in");
		logMenuPanel.add(inCheckBox);
		outCheckBox = new JCheckBox("out");
		logMenuPanel.add(outCheckBox);
		linkTextField = new JTextField(5);
		logMenuPanel.add(linkTextField);
		logMenuPanel.add(new JLabel("link"));
		comtreeTextField = new JTextField(5);
		logMenuPanel.add(comtreeTextField);
		logMenuPanel.add(new JLabel("comtree"));

		
		logMenuPanel2 = new JPanel(new FlowLayout(FlowLayout.LEFT));
		logMenuPanel2.setBorder(BorderFactory.createTitledBorder(""));
		logMenuPanel2.setPreferredSize(new Dimension(MAIN_WIDTH/2, MENU_HEIGHT));
		logMenuPanel2.setMaximumSize(logMenuPanel.getPreferredSize()); 
		forestTypeComboBox = new JComboBox<String>(forestType);
		logMenuPanel2.add(forestTypeComboBox);
		logMenuPanel2.add(new JLabel("type"));
		cpTypeComboBox = new JComboBox<String>(cpType);
		logMenuPanel2.add(cpTypeComboBox);
		logMenuPanel2.add(new JLabel("cptype"));
		
		statusPanel.add(new JLabel(" & "));
		statusLabel3 = new JLabel("Logging OFF");
		statusPanel.add(statusLabel3);
		
		onOffLogBtn = new JButton("OFF");
		onOffLogBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				if(!isLogOn){
					onOffLogBtn.setText("ON");
					statusLabel3.setText("Logging ON");
					isLogOn = true;
				}else{
					onOffLogBtn.setText("OFF");
					statusLabel3.setText("Logging OFF");
					isLogOn = false;
				}
				String s = connectionNetMgr.setLogOn(isLogOn);
				if(s != null){
					showPopupStatus(s);
				}
			}
		});
		logMenuPanel.add(onOffLogBtn);
		
		updateLogBtn = new JButton("Update");
		updateLogBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String inAndOut = "";
				if(inCheckBox.isSelected()){
					inAndOut += "in";
				}
				if(outCheckBox.isSelected()){
					inAndOut += "out";
				}
				String link = linkTextField.getText();
				String comtree = comtreeTextField.getText();
				String type = forestTypeComboBox.getSelectedItem().toString();
				String cpType = cpTypeComboBox.getSelectedItem().toString();
				LogFilters filter = new LogFilters(inAndOut, link,
													comtree, type,
													cpType);
				String s = connectionNetMgr.sendLogFilters(filter);
				if(s != null){
					showPopupStatus(s);
				}
				
			}
		});
		logMenuPanel.add(updateLogBtn);
		
		clearLogBtn = new JButton("Clear");
		clearLogBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				logTextArea.setText("");
			}
		});
		logMenuPanel.add(clearLogBtn);

		routerPanel.add(routerInfoMenuPanel);
		routerPanel.add(routerInfoPanel);
		routerPanel.add(logMenuPanel);
		routerPanel.add(logMenuPanel2);
		routerPanel.add(logDisplayPanel);
		
		mainPanel.add(comtreePanel);
		mainPanel.add(routerPanel);

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
			AbstractTableModel tableModel = (AbstractTableModel) infoTable.getModel();
			int j = 1;
			if(tableModel instanceof ComtreeTableModel){
				j = ((ComtreeTableModel)infoTable.getModel()).getWidth(i);
			} else if(tableModel instanceof LinkTableModel){
				j = ((LinkTableModel)infoTable.getModel()).getWidth(i);
			} else if(tableModel instanceof IfaceTableModel){
				j = ((IfaceTableModel)infoTable.getModel()).getWidth(i);
			} else if(tableModel instanceof RouteTableModel){
				j = ((RouteTableModel)infoTable.getModel()).getWidth(i);
			}
			infoTable.getColumnModel().getColumn(i).setPreferredWidth((int)(MAIN_WIDTH/2 * j * 0.1));
			infoTable.getColumnModel().getColumn(i).setCellRenderer(centerRenderer);
		}
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
		SwingUtilities.invokeLater(new Runnable() {
			public void run() { new NetMgrConsole(); }
		});
	}
}

