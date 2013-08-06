package forest.control;

/** @file NetMgrConsole.java
 *
 *  @author Doowon Kim
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

import java.util.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.charset.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.table.*;
import java.awt.geom.*;

import forest.common.*;

public class NetMgrConsole {

    public static final int MAIN_WIDTH = 700;
    public static final int MAIN_HEIGHT = 900;
    public static final int LOGIN_WIDTH = 300;
    public static final int LOGIN_HEIGHT = 150; 
    public static final int COMTREE_MENU_HEIGHT = 50;
    
    private JFrame mainFrame;

    private Charset ascii;
    private CharsetEncoder enc;
    private CharBuffer cb;
    private ByteBuffer bb;
    private SocketAddress serverAddr;
    private SocketChannel serverChan;
    private NetBuffer inBuf;

    private int nmPort;
    private String nmAddr;

    private String routerName;
    private String table;

    private ComtreeTableModel comtreeTableModel;
    private LinkTableModel linkTableModel;
    private IfaceTableModel ifaceTableModel;
    private RouteTableModel routeTableModel;

    public NetMgrConsole(String nmAddr, int nmPort){
        this.nmPort = nmPort;
        this.nmAddr = nmAddr;

        mainFrame = new JFrame();
        mainFrame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        mainFrame.setTitle("Net Manager Console");
        mainFrame.setPreferredSize(new Dimension(LOGIN_WIDTH, LOGIN_HEIGHT));

        // displayLoginLabel();
        displayNetMgr();

        mainFrame.pack();
        mainFrame.setVisible(true);
    }
    
    /**
     * Display the login label (ID and Password)
     */
    private void displayLoginLabel(){
        final JPanel panel = new JPanel(new GridBagLayout());
        mainFrame.getContentPane().add(panel, BorderLayout.CENTER);
        GridBagConstraints c = new GridBagConstraints();
        
        JLabel username = new JLabel("ID:");
        c.gridx = 0;
        c.gridy = 1;
        panel.add(username,c);
        JTextField usernameInput = new JTextField(10);
        c.gridx = 1;
        c.gridy = 1;
        panel.add(usernameInput,c);

        JLabel password = new JLabel("Password:");
        c.gridx = 0;
        c.gridy = 2;
        panel.add(password,c);
        final JPasswordField passwordInput = new JPasswordField(10);
        c.gridx = 1;
        c.gridy = 2;
        panel.add(passwordInput,c);

        JButton loginInput = new JButton("Login");
        c.gridx = 1;
        c.gridy = 3;
        loginInput.addActionListener(new ActionListener(){
            public void actionPerformed(ActionEvent e) {
                char[] passwd = passwordInput.getPassword();
                if(isPasswordCorrect(passwd)){
                    JOptionPane.showMessageDialog(null, "Success");
                    mainFrame.getContentPane().removeAll();
                    displayNetMgr();
                    mainFrame.repaint();
                } else {
                    JOptionPane.showMessageDialog(null, "Failed");
                }
            }
        });
        panel.add(loginInput,c);

    }
    
    /**
     * Display Net Manager Console
     */
    private void displayNetMgr(){
        //initNetMgrConnection();//initial Connection to NetMgr
        
        comtreeTableModel = new ComtreeTableModel();
        linkTableModel = new LinkTableModel();
        ifaceTableModel = new IfaceTableModel();
        routeTableModel = new RouteTableModel();

        JPanel mainPanel = new JPanel();
        BoxLayout boxLayout = new BoxLayout(mainPanel, BoxLayout.Y_AXIS);
        mainPanel.setLayout(boxLayout);
        mainFrame.getContentPane().add(mainPanel, BorderLayout.PAGE_START);
        mainFrame.setPreferredSize(new Dimension(MAIN_WIDTH, MAIN_HEIGHT));
        

        //Comtree Menu
        JPanel comtreeMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        comtreeMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
        comtreeMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 45));
        comtreeMenuPanel.setMaximumSize(comtreeMenuPanel.getPreferredSize());
        JLabel comtreeNameLabel = new JLabel("ComTree:");
        comtreeMenuPanel.add(comtreeNameLabel);
        Integer[] comtrees = {0, 1, 1001, 1002, 1003}; //comtree combobox
        JComboBox comtreeComboBox = new JComboBox(comtrees);
        comtreeMenuPanel.add(comtreeComboBox);

        
        //ComTree Display
        JPanel comtreeDisplayPanel = new JPanel(); 
        comtreeDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
        comtreeDisplayPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 400));
        

        //Router Info Menu
        final JTable infoTable = new JTable(comtreeTableModel); //initially comtreeTableModel
        JPanel routerInfoMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        routerInfoMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
        routerInfoMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 45));
        routerInfoMenuPanel.setMaximumSize(routerInfoMenuPanel.getPreferredSize());
        JLabel routerNameLabel = new JLabel("Router:");
        routerInfoMenuPanel.add(routerNameLabel);
        String[] routers = {"r1", "r2"}; //comtree combobox
        final JComboBox routersComboBox = new JComboBox(routers); //router selection
        routerInfoMenuPanel.add(routersComboBox);
        String[] tables = {"Comtree", "Link", "Iface", "Route"};
        final JComboBox tablesComboBox = new JComboBox(tables); //info selection
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
        JButton routerInfoUpdateButton = new JButton("Update");
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
        JButton routerInfoClearButton = new JButton("Clear");
        routerInfoClearButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                linkTableModel.clear();
                linkTableModel.fireTableDataChanged();
            }
        });
        routerInfoMenuPanel.add(routerInfoUpdateButton);
        routerInfoMenuPanel.add(routerInfoClearButton);


        //Link Table
        JPanel routerInfoPanel = new JPanel();
        routerInfoPanel.setBorder(BorderFactory.createTitledBorder(""));
        
        JTableHeader linkTableHeader = infoTable.getTableHeader();
        linkTableHeader.setDefaultRenderer(new HeaderRenderer(infoTable));
        infoTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF); //size
        DefaultTableCellRenderer centerRenderer = new DefaultTableCellRenderer();
        centerRenderer.setHorizontalAlignment(JLabel.CENTER);
        for ( int i = 0; i < infoTable.getModel().getColumnCount(); ++i){
            infoTable.getColumnModel().getColumn(i).setCellRenderer(centerRenderer);
        }
        infoTable.setFillsViewportHeight(true);
        
        JScrollPane infoTableScrollPane = new JScrollPane(infoTable);
        infoTableScrollPane.setPreferredSize(new Dimension(MAIN_WIDTH, 100));
        routerInfoPanel.add(infoTableScrollPane);
        
        //Log Menu
        JPanel logMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        logMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
        logMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 45));
        logMenuPanel.setMaximumSize(logMenuPanel.getPreferredSize());  
        JLabel tmpLabel = new JLabel("r1");
        logMenuPanel.add(tmpLabel);
        
        //Log Display
        JPanel logDisplayPanel = new JPanel();
        logDisplayPanel.setBorder(BorderFactory.createTitledBorder(""));
        JTextField logTextArea = new JTextField();
        logTextArea.setPreferredSize(new Dimension(MAIN_WIDTH, 100));
        logTextArea.setEditable(true);
        JScrollPane scroll = new JScrollPane(logTextArea);
        scroll.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
        logDisplayPanel.add(scroll);

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
     * Check if given password is correct
     * @param  input password
     * @return if correct
     */
    private static boolean isPasswordCorrect(char[] input) {
        boolean isCorrect = true;
        char[] correctPassword = { 'p', 'a', 's', 's'};

        if (input.length != correctPassword.length) {
            isCorrect = false;
        } else {
            isCorrect = Arrays.equals(input, correctPassword);
        }

        //Zero out the password.
        Arrays.fill(correctPassword,'0');

        return isCorrect;
    }   

    /**
     * Initialize a connection to Net Manager
     */
    private void initNetMgrConnection(){
        ascii = Charset.forName("US-ASCII");
        enc = ascii.newEncoder();
        enc.onMalformedInput(CodingErrorAction.IGNORE);
        enc.onUnmappableCharacter(CodingErrorAction.IGNORE);
        cb = CharBuffer.allocate(1024);
        bb = ByteBuffer.allocate(1024);
    
        try {
            serverAddr = new InetSocketAddress(InetAddress.getByName(nmAddr),nmPort);
            serverChan = SocketChannel.open(serverAddr);
            inBuf = new NetBuffer(serverChan,1000);
            if (!serverChan.isConnected()) 
                System.out.println("Not Connected!");
        } catch (Exception e){
            e.printStackTrace();
        }
    }
    
    /**
     * Send a string msg to Net Manager
     * @param  msg message
     * @return succesful
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

    public static void main(String[] args) {
        final String nmAddr = args[0];
        final int nmPort = Integer.parseInt(args[1]);
        SwingUtilities.invokeLater(new Runnable() {
            public void run() { new NetMgrConsole(nmAddr, nmPort); }
        });
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
}

/**
 * This class is for the model for Link Table (JTable)
 */
class LinkTableModel extends AbstractTableModel {
    private String[] columnNames = {"link", "iface", "peerIp:port", "peerType", "peerAdr", "rates"};
    private int[] widths = {1, 1, 3, 1, 1, 3};
    private ArrayList<LinkTable> data;
    public LinkTableModel(){ data = new ArrayList<LinkTable>(); }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        LinkTable lt = data.get(row);
        if(lt == null) return null;
        switch (col){
            case 0: return lt.getLink(); 
            case 1: return lt.getIface();
            case 2: return lt.getPeerIpAndPort();
            case 3: return lt.getPeerType();
            case 4: return lt.getPeerAdr();
            case 5: return lt.getRates();
            default: return null;
        }
    }
    public void addLinkTable(LinkTable lt) { data.add(lt); }
    public void clear(){ data.clear(); }
    public int getWidth(int i) { return widths[i];}
}

/**
 * This class is for the model for Comtree Table (JTable)
 */
class ComtreeTableModel extends AbstractTableModel {
    private String[] columnNames = {"comtree", "inCore", "pLink", "link"};
    private int[] widths = {1, 1, 3, 1};
    private ArrayList<ComtreeTable> data;
    public ComtreeTableModel(){ data = new ArrayList<ComtreeTable>(); }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        ComtreeTable ct = data.get(row);
        if(ct == null) return null;
        switch (col){
            case 0: return ct.getComtree(); 
            case 1: return ct.getInCore();
            case 2: return ct.getpLink();
            case 3: return ct.getLink();
            default: return null;
        }
    }
    public void addComtreeTable(ComtreeTable ct) { data.add(ct); }
    public void clear(){ data.clear(); }
    public int getWidth(int i) { return widths[i];}
}

/**
 * This class is for the model for Iface Table (JTable)
 */
class IfaceTableModel extends AbstractTableModel {
    private String[] columnNames = {"iface", "ip", "rates"};
    private int[] widths = {1, 1, 3};
    private ArrayList<IfaceTable> data;
    public IfaceTableModel(){ data = new ArrayList<IfaceTable>(); }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        IfaceTable it = data.get(row);
        if(it == null) return null;
        switch (col){
            case 0: return it.getIface(); 
            case 1: return it.getIp();
            case 2: return it.getRates();
            default: return null;
        }
    }
    public void addIfaceTable(IfaceTable it) { data.add(it); }
    public void clear(){ data.clear(); }
    public int getWidth(int i) { return widths[i];}
}

/**
 * This class is for the model for Route Table (JTable)
 */
class RouteTableModel extends AbstractTableModel {
    private String[] columnNames = {"comtree", "Addr", "test", "test", "test", "test"};
    private int[] widths = {1, 1, 3, 1, 1, 3};
    private ArrayList<RouteTable> data;
    public RouteTableModel(){ data = new ArrayList<RouteTable>(); }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        RouteTable rt = data.get(row);
        if(rt == null) return null;
        switch (col){
            case 0: return rt.getComtree(); 
            case 1: return rt.getAddr();
            default: return null;
        }
    }
    public void addIfaceTable(RouteTable rt) { data.add(rt); }
    public void clear(){ data.clear(); }
    public int getWidth(int i) { return widths[i];}
}

class LinkTable{
    private String link;
    private String iface;
    private String peerIpAndPort;
    private String peerType;
    private String peerAdr;
    private String rates;
    public LinkTable(String link, String iface, String peerIpAndPort, String peerType, String peerAdr, String rates) {
        this.link = link; this.iface = iface; this.peerIpAndPort = peerIpAndPort;
        this.peerType = peerType; this.peerAdr = peerAdr; this.rates = rates;
    }
    public String getLink() { return link; }
    public String getIface() { return iface; }
    public String getPeerIpAndPort() { return peerIpAndPort; }
    public String getPeerType() { return peerType; }
    public String getPeerAdr() { return peerAdr; }
    public String getRates() { return rates; }
}

class ComtreeTable{
    private String comtree;
    private String inCore;
    private String pLink;
    private String link;
    public ComtreeTable(String comtree, String inCore, String pLink, String link){
        this.comtree = comtree; this.inCore = inCore;
        this.pLink = pLink; this.link = link;
    }
    public String getComtree() {return comtree;}
    public String getInCore() {return inCore;}
    public String getpLink() {return pLink;}
    public String getLink() {return link;}
}

class IfaceTable{
    private String iface;
    private String ip;
    private String rates;
    public IfaceTable(String iface, String ip, String rates){
        this.iface = iface; this.ip = ip; this.rates = rates;
    }
    public String getIface() {return iface;}
    public String getIp() {return ip;}
    public String getRates() {return rates;}
}

class RouteTable{
    private String comtree;
    private String addr;
    public RouteTable(String comtree, String addr){
        this.comtree = comtree; this.addr = addr;
    }
    public String getComtree() {return comtree;}
    public String getAddr() {return addr;}
}