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
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;

import javax.swing.*;
import javax.swing.border.BevelBorder;
import javax.swing.table.*;

//import forest.control.ComtInfo;
import forest.control.NetInfo;
import forest.control.console.model.*;

public class NetMgrConsole {
	
	public static final int MAIN_WIDTH = 1400;
	public static final int MAIN_HEIGHT = 700;
	public static final int MENU_HEIGHT = 50;

	// menu bar
	private JFrame mainFrame;
	private JMenuBar menuBar;
	private JMenu menu;
	private JMenu optionMenu;
	private JMenuItem connectMenuItem;
	private JMenuItem loginMenuItem;
	private JMenuItem newAccountMenuItem;
	private JMenuItem updateProfileMenuItem;
	private JMenuItem changePwdMenuItem;
	private JMenuItem closeMenuItem;
	private JCheckBoxMenuItem saveAsFileMOption;
	
	//Save as File
	private HashMap<String, PrintWriter> hashMapWriter;

	private JPanel mainPanel;
	private JPanel statusPanel;
	private JLabel statusLabel;
	private JLabel statusLabel2;
//	private JLabel statusLabel3;s

	// comtree menu
	private JPanel comtMenuPanel;
	private JLabel comtNameLabel;
	private Vector<Integer> comtComBoxItems;
	private DefaultComboBoxModel<Integer> comtComBoxModel;
	private JComboBox<Integer> comtComBox;
	private JButton comtDisUpdateBtn;
	private JButton comtDisClearBtn;
	private JCheckBox refreshChBox;

	// comtree display
	private ComtreeDisplay comtDisplayPanel;

	// router info menu
	private JTable infoTable;
	private JPanel rtrInfoMenuPanel;
	private JLabel rtrNameLabel;
	private Vector<String> rtrItems;
	private DefaultComboBoxModel<String> rtrComBoxModel;
	private JComboBox<String> rtrComBox;
	private String[] type = { "Comtree", "Link", "Iface", "Route" };
	private JComboBox<String> tablesComBox;
	private JTextField rangeTxFld;
	private JButton rtrInfoUpdateBtn;
	private JButton rtrInfoClearBtn;

	// Router Info Table
	private JPanel rtrInfoPanel;
	private JTableHeader tableHeader;
	private DefaultTableCellRenderer centerRenderer;
	private JScrollPane infoTableScrollPane;

	// Log menu
	private JPanel logMenuPanel;
	private JPanel logMenuPanel2;
	private Vector<String> rtrLogItems;
	private DefaultComboBoxModel<String> rtrLogComBoxModel;
	private JComboBox<String> rtrLogComBox;
	private JTextField linkLogTxFld;
	private JTextField comtLogTxFld;
	private JCheckBox inChBox;
	private JCheckBox outChBox;
	private JButton addFilterBtn;
	private JButton dropFilterBtn;
	private JTextField srcTxFld;
	private JTextField dstTxFld;
	private String[] pktType = { "all", "sub_unsub", "client_sig",
			"connect", "disconnect", "net_sig", "rte_reply", "rtr_ctl",
			"voqstatus" };
	private JComboBox<String> pktTypeComBox;
	private String[] cpType = { "all", "client_add_comtree",
			"client_drop_comtree", "client_get_comtree", "client_mod_comtree",
			"client_join_comtree", "client_leave_comtree",
			"client_resize_comtree", "client_get_leaf_rate",
			"client_mod_leaf_rate",

			"client_net_sig_sep",

			"add_iface", "drop_iface", "get_iface", "mod_iface",

			"add_link", "drop_link", "get_link", "mod_link", "get_link_set",

			"add_comtree", "drop_comtree", "get_comtree", "mod_comtree",

			"add_comtree_link", "drop_comtree_link", "mod_comtree_link",
			"get_comtree_link", "get_comtree_set",

			"get_iface_set", "get_route_set",

			"add_route", "drop_route", "get_route", "mod_route",
			"add_route_link", "drop_route_link",
			
			"add_filter", "drop_filter", "get_filter", "mod_filter",
			"get_filter_set", "get_logged_packets", "enable_local_log",

			"new_session", "cancel_session", "client_connect",
			"client_disconnect",

			"set_leaf_range", "config_leaf",

			"boot_router", "boot_complete", "boot_abort", "boot_leaf" };

	private JComboBox<String> cpTypeComBox;
	private JPanel filterPanel;
	private JTable filterTable;
	private LogFilterTableModel logFilterTableModel;
	private JTableHeader filterTableHeader;
	private JScrollPane filterTableScrollPane;

	private ConnectDialog connectDialog;
	private LoginDialog loginDialog;
	private UpdateProfileDialog updateProfileDialog;
	private ChangePwdDialog changePwdDialog;
	
	private HashMap<String, LogFrame> hashMapLogFrame; //map to Log Frame
	private HashSet<String> setUsingRtr; //A set of router to which currently filter added 

	private ConnectionNetMgr connectionNetMgr;
	private ConnectionComtCtl connectionComtCtl;

	// Comtree
	TreeSet<Integer> comtSet;
	private String rtrName;
	private String table;

	private ComtreeTableModel comtreeTableModel;
	private LinkTableModel linkTableModel;
	private IfaceTableModel ifaceTableModel;
	private RouteTableModel routeTableModel;

	private boolean isLoggedIn = false;
	private boolean isConnected = false;
	
	public NetMgrConsole() {

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

		comtComBoxItems = new Vector<Integer>();
		comtComBoxModel = new DefaultComboBoxModel<Integer>(comtComBoxItems);
		rtrItems = new Vector<String>();
		rtrComBoxModel = new DefaultComboBoxModel<String>(rtrItems);
		rtrLogItems = new Vector<String>();
		rtrLogComBoxModel = new DefaultComboBoxModel<String>(rtrLogItems);
		rtrLogComBoxModel.addElement("all");

		comtreeTableModel = new ComtreeTableModel();
		linkTableModel = new LinkTableModel();
		ifaceTableModel = new IfaceTableModel();
		routeTableModel = new RouteTableModel();
		logFilterTableModel = new LogFilterTableModel();
		
		connectDialog = new ConnectDialog();
		loginDialog = new LoginDialog();
		updateProfileDialog = new UpdateProfileDialog();
		changePwdDialog = new ChangePwdDialog();
		
		hashMapLogFrame = new HashMap<String, LogFrame>(); 
		setUsingRtr = new HashSet<String>(); 
		
		hashMapWriter = new HashMap<String, PrintWriter>();

		connectionNetMgr = new ConnectionNetMgr();
		connectionComtCtl = new ConnectionComtCtl();

		displayNetMgr();

		mainFrame.addWindowListener(new CloseEvent(connectionNetMgr,connectionComtCtl,
				hashMapWriter));
		mainFrame.pack();
		mainFrame.setVisible(true);
	}

	/**
	 * Initialize menu bar
	 */
	private void initMenuBar () {
		menu = new JMenu("Menu");
		menuBar.add(menu);

		connectMenuItem = new JMenuItem("Connect");
		connectMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent evt) {
				if (!isConnected) {
					connectNetMgrComtCtl();
				}
			}
		});
		menu.add(connectMenuItem);

		menu.addSeparator();
		loginMenuItem = new JMenuItem("Login");
		loginMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent evt) {
				if (isConnected) {
					logIn();
				} else {
					showPopupStatus("Connection required");
				}		
			}
		});
		menu.add(loginMenuItem);

		menu.addSeparator();
		newAccountMenuItem = new JMenuItem("New Account");
		newAccountMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent evt) {
				if (isConnected && isLoggedIn) {
					String[] options = { "Add", "Cancel" };
					int n = JOptionPane.showOptionDialog(mainFrame,
							loginDialog, "Add a new account",
							JOptionPane.OK_CANCEL_OPTION,
							JOptionPane.INFORMATION_MESSAGE, null, options,
							options[0]);
					if (n == 0) { // add
						char[] passwd = loginDialog.getPassword();
						String userName = loginDialog.getUserName();
						String ret = connectionNetMgr.newAccount(userName, new String(passwd));
						if (ret == null)
							showPopupStatus("New account created");
						else
							showPopupStatus(ret);
					}
				} else {
					showPopupStatus("login required");
				}
			}
		});
		menu.add(newAccountMenuItem);

		updateProfileMenuItem = new JMenuItem("Update Profile");
		updateProfileMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent evt) {
				if (isConnected && isLoggedIn) {
					String[] options = { "Update", "Cancel" };
					AdminProfile adminProfile = connectionNetMgr.getAdminProfile();
					updateProfileDialog.setUserName(adminProfile.getUserName());
					updateProfileDialog.setRealName(adminProfile.getRealName());
					updateProfileDialog.setEmail(adminProfile.getEmail());

					int n = JOptionPane.showOptionDialog(mainFrame,
							updateProfileDialog, "Update Profile",
							JOptionPane.OK_CANCEL_OPTION,
							JOptionPane.INFORMATION_MESSAGE, null, options,
							options[0]);
					if (n == 0) { // update
						String userName = adminProfile.getUserName();
						String realName = updateProfileDialog.getRealName();
						String email = updateProfileDialog.getEmail();
						String ret = connectionNetMgr.updateProfile(userName,
								realName, email);
						if (ret == null) {
							connectionNetMgr.getProfile(userName);
							showPopupStatus("The account profile updated");
						} else
							showPopupStatus(ret);
					}
				} else {
					showPopupStatus("login required");
				}
			}
		});
		menu.add(updateProfileMenuItem);

		changePwdMenuItem = new JMenuItem("Change Password");
		changePwdMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent evt) {
				if (isConnected && isLoggedIn) {
					String[] options = { "Change", "Cancel" };
					AdminProfile adminProfile = connectionNetMgr
							.getAdminProfile();
					changePwdDialog.setUserName(adminProfile.getUserName());
					int n = JOptionPane.showOptionDialog(mainFrame,
							changePwdDialog, "Change password",
							JOptionPane.OK_CANCEL_OPTION,
							JOptionPane.INFORMATION_MESSAGE, null, options,
							options[0]);
					if (n == 0) { // change
						String userName = adminProfile.getUserName();
						char[] password = changePwdDialog.getPassword();
						char[] vPassword = changePwdDialog.getVerifyPassword();
						if (Arrays.equals(password, vPassword)) {
							String ret = connectionNetMgr.changePassword(
									userName, new String(password));
							if (ret == null) {
								showPopupStatus("The password changed");
							} else
								showPopupStatus(ret);
						} else {
							showPopupStatus("Passwords are not matched");
						}
					}
				} else {
					showPopupStatus("login required");
				}
			}
		});
		menu.add(changePwdMenuItem);

		menu.addSeparator();
		closeMenuItem = new JMenuItem("Close");
		closeMenuItem.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent evt) {
				if (connectionNetMgr != null && connectionComtCtl != null) {
					connectionNetMgr.closeSocket();
					connectionComtCtl.closeSocket();
					System.out.println("Connections are closing...");
				}
				System.exit(0);
			}
		});
		menu.add(closeMenuItem);

		optionMenu = new JMenu("Option"); // optionMenu
		menuBar.add(optionMenu);

		saveAsFileMOption = new JCheckBoxMenuItem("SaveAsFile");
		saveAsFileMOption.setSelected(true);
		optionMenu.add(saveAsFileMOption);
	}

	/**
	 * Display Net Manager Console
	 */
	private void displayNetMgr () {
		comtDisplayPanel = new ComtreeDisplay(connectionComtCtl);

		mainPanel = new JPanel();
		BoxLayout boxLayout = new BoxLayout(mainPanel, BoxLayout.X_AXIS);
		mainPanel.setLayout(boxLayout);
		mainFrame.getContentPane().add(mainPanel, BorderLayout.PAGE_START);
		mainFrame.setPreferredSize(new Dimension(MAIN_WIDTH, MAIN_HEIGHT));

		// Comtree Menu
		JPanel comtreePanel = new JPanel();
		comtreePanel.setLayout(new BoxLayout(comtreePanel, BoxLayout.Y_AXIS));
		comtreePanel.setPreferredSize(new Dimension(MAIN_WIDTH/2,MAIN_HEIGHT));

		comtMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		comtMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		comtMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2,MENU_HEIGHT));
		comtMenuPanel.setMaximumSize(comtMenuPanel.getPreferredSize());
		comtNameLabel = new JLabel("ComTree:");
		comtMenuPanel.add(comtNameLabel);
		comtComBox = new JComboBox<Integer>(comtComBoxModel); // combobox
		comtMenuPanel.add(comtComBox);
		comtDisUpdateBtn = new JButton("Update"); // button
		comtDisUpdateBtn.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent e) {
				if (isConnected && isLoggedIn) {
					String c = comtComBox.getSelectedItem().toString();
					if (c == null) {
						showPopupStatus("Choose one of comtrees");
					} else {
						int ccomt = Integer.parseInt(c);
						if (ccomt != 0) {
							comtDisplayPanel.clearUI();
							comtDisplayPanel.autoUpdateDisplay(ccomt);
						}
					}
				} else {
					showPopupStatus("login required");
				}
			}
		});
		comtMenuPanel.add(comtDisUpdateBtn);
		comtDisClearBtn = new JButton("Clear"); // button
		comtDisClearBtn.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent e) {
				if (isConnected && isLoggedIn) {
					comtDisplayPanel.clearUI();
				} else {
					showPopupStatus("login required");
				}
			}
		});
		comtMenuPanel.add(comtDisClearBtn);
		refreshChBox = new JCheckBox("refresh");// refresh button
		refreshChBox.addItemListener(new ItemListener() {
			@Override
			public void itemStateChanged (ItemEvent e) {
				if (e.getStateChange() == ItemEvent.SELECTED) {
					connectionComtCtl.setAutoRefresh(true);
					if (isLoggedIn && isConnected) {
						String c = comtComBox.getSelectedItem().toString();
						if (c == null) {
							showPopupStatus("Choose one of comtrees");
						} else {
							int ccomt = Integer.parseInt(c);
							if (ccomt != 0) {
								comtDisplayPanel.clearUI();
								comtDisplayPanel.autoUpdateDisplay(ccomt);
							}
						}
					}
				} else
					connectionComtCtl.setAutoRefresh(false);
			}
		});
		refreshChBox.setSelected(true);
		comtMenuPanel.add(refreshChBox);
		statusLabel = new JLabel(" Not connected"); // status label
		statusLabel2 = new JLabel(" Not logged in");

		statusPanel.add(statusLabel);
		statusPanel.add(new JLabel(" || "));
		statusPanel.add(statusLabel2);

		// ComTree Display
		// comtreeDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
		comtDisplayPanel.setPreferredSize(new Dimension(MAIN_WIDTH / 2,
				MAIN_HEIGHT - MENU_HEIGHT));

		comtreePanel.add(comtMenuPanel);
		comtreePanel.add(comtDisplayPanel);

		/******************** ROUTER INFOMATION ********************/
		// Router Info Menu
		JPanel routerPanel = new JPanel();
		routerPanel.setLayout(new BoxLayout(routerPanel, BoxLayout.Y_AXIS));
		routerPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MAIN_HEIGHT));

		infoTable = new JTable(comtreeTableModel); //comtreeTableModel
		centerRenderer = new DefaultTableCellRenderer();
		centerRenderer.setHorizontalAlignment(JLabel.CENTER);
		setPreferredWidth(infoTable);
		rtrInfoMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		rtrInfoMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		rtrInfoMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MENU_HEIGHT));
		rtrInfoMenuPanel.setMaximumSize(rtrInfoMenuPanel.getPreferredSize());
		rtrNameLabel = new JLabel("Router:");
		rtrInfoMenuPanel.add(rtrNameLabel);
		rtrComBox = new JComboBox<String>(rtrComBoxModel); // router
		rtrInfoMenuPanel.add(rtrComBox);

		tablesComBox = new JComboBox<String>(type); // info selection
		tablesComBox.addItemListener(new ItemListener() {
			public void itemStateChanged (ItemEvent e) {
				if (e.getStateChange() == 1) {
					if (e.getItem().equals("Comtree")) {
						infoTable.setModel(comtreeTableModel);
					} else if (e.getItem().equals("Link")) {
						infoTable.setModel(linkTableModel);
					} else if (e.getItem().equals("Iface")) {
						infoTable.setModel(ifaceTableModel);
					} else if (e.getItem().equals("Route")) {
						infoTable.setModel(routeTableModel);
					}
					setPreferredWidth(infoTable);
				}
			}
		});
		
		rtrInfoMenuPanel.add(tablesComBox);
		
		rangeTxFld = new JTextField(5); //range Text Field
		rtrInfoMenuPanel.add(rangeTxFld);
		rtrInfoMenuPanel.add(new JLabel("range"));
		
		rtrInfoUpdateBtn = new JButton("Update");
		rtrInfoUpdateBtn.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent e) {
				if (isConnected && isLoggedIn) {
					table = tablesComBox.getSelectedItem().toString();
					rtrName = rtrComBox.getSelectedItem().toString();
					String range = rangeTxFld.getText();
					int start = 0;
					int end = 0;
					
					if (range.contains(",")) {
						String[] tokens = range.split(",");
						if (tokens.length == 2) {
							start = Integer.valueOf(tokens[0]);
							end = Integer.valueOf(tokens[1]);
						} else {
							showPopupStatus("invalid input, e.g. 0, 100");
							return;
						}
					}
					
					/* TODO : Add the exception for invalid input like colon */
				
					String s = null;
					if (table.equals("Link")) {
						s = connectionNetMgr.getTable(linkTableModel,rtrName,start,end);
					} else if (table.equals("Comtree")) {
						s = connectionNetMgr.getTable(comtreeTableModel,rtrName,start,end);
					} else if (table.equals("Iface")) {
						s = connectionNetMgr.getTable(ifaceTableModel,rtrName,start,end);
					} else if (table.equals("Route")) {
						s = connectionNetMgr.getTable(routeTableModel,rtrName,start,end);
					}
					if (s != null)
						showPopupStatus(s);
					
				} else {
					showPopupStatus("login required");
				}
			}
		});
		rtrInfoMenuPanel.add(rtrInfoUpdateBtn);
		
		rtrInfoClearBtn = new JButton("Clear");
		rtrInfoClearBtn.addActionListener(new ActionListener() {
			public void actionPerformed (ActionEvent e) {
				linkTableModel.clear();
				ifaceTableModel.clear();
				comtreeTableModel.clear();
				routeTableModel.clear();
				linkTableModel.fireTableDataChanged();
				ifaceTableModel.fireTableDataChanged();
				comtreeTableModel.fireTableDataChanged();
				routeTableModel.fireTableDataChanged();
				
				new LogFrame("r1", connectionNetMgr).setVisible(true);
			}
		});
		rtrInfoMenuPanel.add(rtrInfoClearBtn);

		// Router Info Table
		rtrInfoPanel = new JPanel();
		rtrInfoPanel.setBorder(BorderFactory.createTitledBorder(""));

		tableHeader = infoTable.getTableHeader();
		tableHeader.setDefaultRenderer(new HeaderRenderer(infoTable));
		infoTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF); // size
		infoTable.setFillsViewportHeight(true);

		infoTableScrollPane = new JScrollPane(infoTable);
		infoTableScrollPane.setPreferredSize(new Dimension(MAIN_WIDTH/2, 300));
		rtrInfoPanel.add(infoTableScrollPane);

		
		/******************** LOG ********************/
		// Log Menu
		logMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		logMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		logMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2,MENU_HEIGHT));
		logMenuPanel.setMaximumSize(logMenuPanel.getPreferredSize());

		rtrLogComBox = new JComboBox<String>(rtrLogComBoxModel);
				
		logMenuPanel.add(rtrLogComBox);
		inChBox = new JCheckBox("in");
		logMenuPanel.add(inChBox);
		outChBox = new JCheckBox("out");
		logMenuPanel.add(outChBox);
		linkLogTxFld = new JTextField(4);
		logMenuPanel.add(linkLogTxFld);
		logMenuPanel.add(new JLabel("link"));
		comtLogTxFld = new JTextField(4);
		logMenuPanel.add(comtLogTxFld);
		logMenuPanel.add(new JLabel("comt"));
		srcTxFld = new JTextField(4);
		logMenuPanel.add(srcTxFld);
		logMenuPanel.add(new JLabel("src"));
		dstTxFld = new JTextField(4);
		logMenuPanel.add(dstTxFld);
		logMenuPanel.add(new JLabel("dst"));

		logMenuPanel2 = new JPanel(new FlowLayout(FlowLayout.LEFT));
		logMenuPanel2.setBorder(BorderFactory.createTitledBorder(""));
		logMenuPanel2.setPreferredSize(new Dimension(MAIN_WIDTH/2,MENU_HEIGHT));
		logMenuPanel2.setMaximumSize(logMenuPanel.getPreferredSize());
		pktTypeComBox = new JComboBox<String>(pktType); //pkt Type
		logMenuPanel2.add(pktTypeComBox);
//		logMenuPanel2.add(new JLabel("type"));
		cpTypeComBox = new JComboBox<String>(cpType); //ControlPkt Type
		logMenuPanel2.add(cpTypeComBox);
//		logMenuPanel2.add(new JLabel("cptype"));

		// status
//		statusPanel.add(new JLabel(" || "));
//		statusLabel3 = new JLabel("Logging OFF");
//		statusPanel.add(statusLabel3);

		// onOffLogBtn = new JButton("OFF");
		// onOffLogBtn.addActionListener(new ActionListener() {
		// @Override
		// public void actionPerformed(ActionEvent e) {
		// }
		// });
		// logMenuPanel.add(onOffLogBtn);

		addFilterBtn = new JButton("Add");
		addFilterBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed (ActionEvent e) {
				if (isConnected && isLoggedIn) {
					String rtr = rtrLogComBox.getSelectedItem().toString();
					String s = null;
					if (rtr.equals("all")) { //repeat all routers
						for (int i = 1 ; i < rtrLogComBoxModel.getSize() ; i++){
							rtr = rtrLogComBoxModel.getElementAt(i);
							s = addFilter(rtr);
							if (s != null) {
								showPopupStatus(s);
								return;
							}
							//create a logFrame
							if (!hashMapLogFrame.containsKey(rtr)) { //check if exists
								LogFrame logFrame = new LogFrame(rtr, connectionNetMgr);
								logFrame.setVisible(true);
								hashMapLogFrame.put(rtr, logFrame);
								setUsingRtr.add(rtr);
							}
							
							//FIXME: temporally retreiving filters
							ArrayList<LogFilter> filters = new ArrayList<LogFilter>();
							connectionNetMgr.getFilterSet(filters, rtr);
							for (LogFilter f : filters) {
								writeLog(f.toString(), rtr);
							}
						}
					} else {
						s = addFilter(rtr);
						if (s != null) {
							showPopupStatus(s);
							return;
						}
						//create a logFrame
						if (!hashMapLogFrame.containsKey(rtr)) { //check if exists
							LogFrame logFrame = new LogFrame(rtr, connectionNetMgr);
							logFrame.setVisible(true);
							hashMapLogFrame.put(rtr, logFrame);
							setUsingRtr.add(rtr);
						}
						
						//FIXME: temporally retreiving filters
						ArrayList<LogFilter> filters = new ArrayList<LogFilter>();
						connectionNetMgr.getFilterSet(filters, rtr);
						for (LogFilter f : filters) {
							writeLog(f.toString(), rtr);
						}
					}
				} else {
					showPopupStatus("connection or login required");
				}
			}
		});
		logMenuPanel2.add(addFilterBtn);

		// drop filter
		dropFilterBtn = new JButton("Drop");
		dropFilterBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed (ActionEvent e) {
				if (isConnected && isLoggedIn) {
					int i = filterTable.getSelectedRow();
					if (i >= 0) {
						int fx = (Integer) logFilterTableModel.getValueAt(i, 0);
						String rtrName = (String) logFilterTableModel.getValueAt(i, 1);
						String s = connectionNetMgr.dropFilter(rtrName, fx);
						if (s == null) {
							logFilterTableModel.removeLogFilterTable(i);
							logFilterTableModel.fireTableDataChanged();
							showPopupStatus("filter: " + fx + " was dropped");
							if (setUsingRtr.contains(rtrName)) {
								setUsingRtr.remove(rtrName);
							}
						} else {
							showPopupStatus(s);
						}

					} else { // i == -1
						showPopupStatus("select one of rows");
					}
				} else {
					showPopupStatus("connection or login required");
				}
			}
		});
		logMenuPanel2.add(dropFilterBtn);
		
		// filter table
		filterTable = new JTable(logFilterTableModel);
		setPreferredWidth(filterTable);
		filterTableHeader = filterTable.getTableHeader();
		filterTableHeader.setDefaultRenderer(new HeaderRenderer(filterTable));
		filterTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF); // size
		filterTable.setFillsViewportHeight(true);
		filterTableScrollPane = new JScrollPane(filterTable);
		filterTableScrollPane.setPreferredSize(new Dimension(MAIN_WIDTH/2,200));

		filterPanel = new JPanel();
		filterPanel.setBorder(BorderFactory.createTitledBorder(""));
		filterPanel.add(filterTableScrollPane);

		routerPanel.add(rtrInfoMenuPanel);
		routerPanel.add(rtrInfoPanel);
		routerPanel.add(logMenuPanel);
		routerPanel.add(logMenuPanel2);
		routerPanel.add(filterPanel);

		mainPanel.add(comtreePanel);
		mainPanel.add(routerPanel);

		mainFrame.add(mainPanel);
		mainFrame.pack();
		mainFrame.repaint();
	}
	
	/**
	 * Add a log filter 
	 * @param rtrName router name
	 * @return null if successful otherwise error message
	 */
	private String addFilter (String rtrName) {
		LogFilter logFilter = new LogFilter();
		boolean in = false;
		boolean out = false;
		if (inChBox.isSelected()) {
			in = true;
		}
		if (outChBox.isSelected()) {
			out = true;
		}
		logFilter.setRtr(rtrName); // router name

		logFilter.setIn(in); // in
		logFilter.setOut(out); // out

		String link = linkLogTxFld.getText();
		try {
			Integer.parseInt(link);
		} catch (NumberFormatException ne) {
			showPopupStatus("Put an interger in link field");
			return "Put an interger in link field";
		}
		logFilter.setLink(link); // link

//		NetInfo net = connectionComtCtl.getNetInfo();
//		int srcAdr = net.getNodeAdr(net.getNodeNum("netMgr"));
//		logFilter.setSrcAdr(Forest.fAdr2string(srcAdr));
		
		logFilter.setSrcAdr(srcTxFld.getText()); // src addr
		
//		int dstAdr = net.getNodeAdr(net.getNodeNum(rtrName));
//		logFilter.setDstAdr(Forest.fAdr2string(dstAdr));
		
		logFilter.setDstAdr(dstTxFld.getText()); // dst addr
		
		String comtree = comtLogTxFld.getText();
		if (comtree.equals("0")) {
			comtree = "0";
		}
		logFilter.setComtree(comtree); // comtree

		String type = pktTypeComBox.getSelectedItem().toString();
		if (type.equals("all")) {
			type = "undef";
		}
		logFilter.setType(type); // type
		
		String cpType = cpTypeComBox.getSelectedItem().toString();
		if (cpType.equals("all")) {
			cpType = "undef";
		}
		logFilter.setCpType(cpType); // cpType

		String s = connectionNetMgr.addFilter(logFilter);
		if (s != null) {
			return s;
		} else {
			logFilterTableModel.addLogFilterTable(logFilter);
			logFilterTableModel.fireTableDataChanged();
		}
	
		return null;
	}
	

	
	/**
	 * Log in NetMgr
	 */
	private void logIn () {
		String[] options = { "Login", "Cancel" };
		int n = JOptionPane.showOptionDialog(mainFrame, loginDialog, "Login", 
											JOptionPane.OK_CANCEL_OPTION, 
											JOptionPane.INFORMATION_MESSAGE, 
											null, options, options[0]);
		
		if (n == 0) { // login option
			char[] passwd = loginDialog.getPassword();
			String userName = loginDialog.getUserName();
			String ret = connectionNetMgr.login(userName, new String(passwd));
			if (ret == null) {
				isLoggedIn = true;
				loginMenuItem.setText("Log out");
				showPopupStatus("Logged in as " + userName);
				statusLabel2.setText(" Logged in as " + userName);
				
				// getting filter sets from routers
				String rtr = ""; String str = "";
				for (int i = 1 ; i < rtrLogComBoxModel.getSize() ; i++) {
					ArrayList<LogFilter> filters = new ArrayList<LogFilter>();
					rtr = rtrLogComBoxModel.getElementAt(i);
					str = connectionNetMgr.getFilterSet(filters, rtr);
					if (str == null) {
						for (LogFilter f : filters) {
							//logFilterTableModel.addLogFilterTable(f);
							String str2 = connectionNetMgr.dropFilter(rtr, f.getId());
							if ( str2 != null) {
								showPopupStatus(str2);
							}
						}
						enablePacketLog(rtr, false, false); // all logs off
					}
				}
			} else {
				showPopupStatus(ret);
			}
		}
	}
	
	/**
	 * Connect to NetMgr and ComtCtl
	 */
	private void connectNetMgrComtCtl() {
		String[] options = { "Connect", "Cancel" };
		int n = JOptionPane.showOptionDialog(mainFrame, connectDialog,
				"Connect to the server", JOptionPane.OK_CANCEL_OPTION,
				JOptionPane.INFORMATION_MESSAGE, null, options, options[0]);
		
		if (n == 0) { //connect
			String nmAddr = connectDialog.getNmAddr();
			connectionNetMgr.setNmAddr(nmAddr);
			String ctAddr = connectDialog.getCtAddr();
			connectionComtCtl.setCmAddr(ctAddr);
			if (connectionNetMgr.connectToNetMgr() && connectionComtCtl.connectToComCtl()) {
				connectionComtCtl.getNet();
				connectionComtCtl.getComtSet();
				comtSet = connectionComtCtl.getComtTreeSet();
				if (comtSet.size() <= 0) {
					showPopupStatus("No Comtrees");
					if (connectionNetMgr != null && connectionComtCtl != null){
						connectionNetMgr.closeSocket();
						connectionComtCtl.closeSocket();
					}
				} else {
					for (Integer c : comtSet) {
						comtComBoxModel.addElement(c);
					}
					statusLabel.setText("Connected to \""+nmAddr+"\""+" and \""+ctAddr+"\"");
					// retrieving routers' name
					NetInfo netInfo = connectionComtCtl.getNetInfo();
					for (int node = netInfo.firstNode(); node != 0; 
							node = netInfo.nextNode(node)) {
						String name = netInfo.getNodeName(node);
						if (name.substring(0, 1).equals("r")) {
							rtrComBoxModel.addElement(name);
							rtrLogComBoxModel.addElement(name);
						}
					}
					isConnected = true;
					showPopupStatus("Connected");
				}
			} else {
				showPopupStatus("Connection is failed.");
				if (connectionNetMgr != null && connectionComtCtl != null){
					connectionNetMgr.closeSocket();
					connectionComtCtl.closeSocket();
				}
			}
		}
	}
	
	/**
	 * Set preferred column header's width
	 * @param infoTable information table
	 */
	private void setPreferredWidth (JTable infoTable) {
		for (int i = 0; i < infoTable.getModel().getColumnCount(); ++i) {
			AbstractTableModel tableModel = (AbstractTableModel)infoTable.getModel();
			float j = 1;
			if (tableModel instanceof ComtreeTableModel) {
				j = ((ComtreeTableModel) infoTable.getModel()).getWidth(i);
			} else if (tableModel instanceof LinkTableModel) {
				j = ((LinkTableModel) infoTable.getModel()).getWidth(i);
			} else if (tableModel instanceof IfaceTableModel) {
				j = ((IfaceTableModel) infoTable.getModel()).getWidth(i);
			} else if (tableModel instanceof RouteTableModel) {
				j = ((RouteTableModel) infoTable.getModel()).getWidth(i);
			} else if (tableModel instanceof LogFilterTableModel) {
				j = ((LogFilterTableModel) infoTable.getModel()).getWidth(i);
			}
			infoTable.getColumnModel().getColumn(i)
					.setPreferredWidth((int) (MAIN_WIDTH / 2 * j * 0.1));
			infoTable.getColumnModel().getColumn(i)
					.setCellRenderer(centerRenderer);
		}
	}

	/**
	 * This class is for centering table's header value. Reference:
	 * http://stackoverflow.com/questions/7493369/jtable-right-align-header
	 */
	private static class HeaderRenderer implements TableCellRenderer {
		DefaultTableCellRenderer renderer;

		public HeaderRenderer(JTable table) {
			renderer = (DefaultTableCellRenderer) table.getTableHeader()
					.getDefaultRenderer();
			renderer.setHorizontalAlignment(JLabel.CENTER);
		}

		@Override
		public Component getTableCellRendererComponent (JTable table,
				Object value, boolean isSelected, boolean hasFocus, int row,
				int col) {
			return renderer.getTableCellRendererComponent(table, value,
					isSelected, hasFocus, row, col);
		}
	}

	public void showPopupStatus (String status) {
		JOptionPane.showMessageDialog(mainFrame, status);
	}

	/**
	 * Write log to separate log frame and as a file if the option is selected 
	 * @param str A string of logs
	 * @param rtr router name to write log to separate log frame  
	 */
	public void writeLog (String str, String rtr) {
		if (saveAsFileMOption.isSelected()) { //save logs as files
			PrintWriter writer = hashMapWriter.get(rtr);
			if (writer == null && !hashMapWriter.containsKey(rtr)) {
				try {
					String fileName = "log_" + rtr + ".txt";
					writer = new PrintWriter(fileName, "UTF-8");
					hashMapWriter.put(rtr, writer);
				} catch (FileNotFoundException e) {
					e.printStackTrace();
					return;
				} catch (UnsupportedEncodingException e) {
					e.printStackTrace();
					return;
				}
			}
			writer.println(str);
			writer.flush();
		}
		
		if (hashMapLogFrame.containsKey(rtr) && (hashMapLogFrame.get(rtr) != null)) {
			LogFrame logFrame = hashMapLogFrame.get(rtr);
			logFrame.write(str);
		}
	}

	/**
	 * Enable packet log in remote router
	 * @param rtr router name
	 * @param on controls whether logging is to be turned on (true) or off (false)
	 * @param local controls local logging in the same way
	 * @return null if success, otherwise error message
	 */
	private String enablePacketLog (String rtr, boolean on, boolean local) {
		String ret = connectionNetMgr.enablePacketLog(rtr, on, local);
		return ret;
	}
	
	public static void main (String[] args) {
		SwingUtilities.invokeLater(new Runnable() {
			public void run () {
				new NetMgrConsole();
			}
		});
	}
}

class CloseEvent extends WindowAdapter {
	private ConnectionNetMgr netMgr;
	private ConnectionComtCtl comtCtl;
	private HashMap<String, PrintWriter> hashMapWriter;

	public CloseEvent(ConnectionNetMgr netMgr, ConnectionComtCtl comtCtl,
						HashMap<String, PrintWriter> hashMapWriter) {
		this.netMgr = netMgr;
		this.comtCtl = comtCtl;
		this.hashMapWriter = hashMapWriter;
	}

	public void windowClosing (WindowEvent e) {
		if (!hashMapWriter.isEmpty()) {
			for (PrintWriter writer : hashMapWriter.values()) {
				writer.close();
			}
		}
		if (netMgr != null && comtCtl != null) { //socket 
			netMgr.closeSocket();
			comtCtl.closeSocket();
			System.out.println("Connections are closing...");
		}
	}
}
