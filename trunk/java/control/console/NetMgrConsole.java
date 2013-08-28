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
	
	//comtree menu
	private JPanel comtreeMenuPanel;
	private JLabel comtreeNameLabel;
	private Vector<Integer> comtreeComboBoxItems;
	private DefaultComboBoxModel comtreeComboBoxModel;
	private JComboBox<Vector<Integer>> comtreeComboBox;
	private JButton comtreeDisUpdateBtn;
	private JButton comtreeDisClearBtn;

	//comtree display
	private ComtreeDisplay comtreeDisplayPanel;
	//router info menu
	private JTable infoTable;
	private JPanel routerInfoMenuPanel;
	private JLabel routerNameLabel;
	private String[] routers = {"r1", "r2", "r3"};
	private JComboBox<String> routersComboBox;
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
	//log display
	private JPanel logDisplayPanel;
	private JTextField logTextArea;
	private JScrollPane logTextAreaScrollPane;

	private ConnectDialog connectDialog;
	private LoginDialog loginDialog;
	private boolean isLoggedin = false;
	private UpdateProfileDialog updateProfileDialog;
	private ChangePwdDialog changePwdDialog;

	private ConnectionNetMgr connectionNetMgr;
	private ConnectionComtCtl connectionComtCtl;
	private int nmPort = 30120;
	private String nmAddr = "forest1.arl.wustl.edu";

	//Comtree
	TreeSet<Integer> comtSet;
	private String routerName;
	private String table;

	private ComtreeTableModel comtreeTableModel;
	private LinkTableModel linkTableModel;
	private IfaceTableModel ifaceTableModel;
	private RouteTableModel routeTableModel;

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
						//connect to NetMgr
						connectionComtCtl.getNet();
						connectionComtCtl.getComtSet();
						comtSet = connectionComtCtl.getComtTreeSet();
						for(Integer c : comtSet)
							comtreeComboBoxModel.addElement(c);

						statusLabel.setText("Connected to \"" + nmAddr + "\"" + " and \"" + 
												ctAddr + "\"");
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
		
		comtreeDisplayPanel = new ComtreeDisplay();

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
							NetInfo netInfo = connectionComtCtl.getNetInfo();
							ComtInfo comtrees = connectionComtCtl.getComtrees();
							connectionComtCtl.getComtree(ccomt);
							comtreeDisplayPanel.updateDisplay(ccomt, netInfo, comtrees);
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
					comtreeDisplayPanel.clearLines();
					comtreeDisplayPanel.clearCircles();
					comtreeDisplayPanel.clearRects();
					comtreeDisplayPanel.updateUI();
				} else{
					showPopupStatus("login required");
				}
			}
		});
		comtreeMenuPanel.add(comtreeDisClearBtn);
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
				if(isLoggedin){
					table = tablesComboBox.getSelectedItem().toString();
					routerName = routersComboBox.getSelectedItem().toString();
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

		//Log Menu
		logMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
		logMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
		logMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH/2, MENU_HEIGHT));
		logMenuPanel.setMaximumSize(logMenuPanel.getPreferredSize());  
		JLabel tmpLabel = new JLabel("r1");
		logMenuPanel.add(tmpLabel);

		//Log Display
		logDisplayPanel = new JPanel();
//		logDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
		logTextArea = new JTextField();
		logTextArea.setPreferredSize(new Dimension(MAIN_WIDTH/2, MAIN_HEIGHT - 
													(MENU_HEIGHT*2 + 300)));
		logTextArea.setEditable(true);
		logTextAreaScrollPane = new JScrollPane(logTextArea);
		logTextAreaScrollPane.setVerticalScrollBarPolicy(ScrollPaneConstants.
															VERTICAL_SCROLLBAR_ALWAYS);
		logDisplayPanel.add(logTextAreaScrollPane);

		routerPanel.add(routerInfoMenuPanel);
		routerPanel.add(routerInfoPanel);
		routerPanel.add(logMenuPanel);
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

