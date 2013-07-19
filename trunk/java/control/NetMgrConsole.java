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
        initNetMgrConnection();//initial Connection to NetMgr

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
        

        //Link Table Menu
        final LinkTableModel linkTableModel = new LinkTableModel();
        JPanel linkTableMenuPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        linkTableMenuPanel.setBorder(BorderFactory.createTitledBorder(""));
        linkTableMenuPanel.setPreferredSize(new Dimension(MAIN_WIDTH, 45));
        linkTableMenuPanel.setMaximumSize(linkTableMenuPanel.getPreferredSize());
        JLabel routerNameLabel = new JLabel("Router:");
        linkTableMenuPanel.add(routerNameLabel);
        String[] routers = {"r1", "r2"}; //comtree combobox
        final JComboBox routersComboBox = new JComboBox(routers);
        linkTableMenuPanel.add(routersComboBox);
        JButton linkTableUpdateButton = new JButton("Update");
        linkTableUpdateButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                routerName = routersComboBox.getSelectedItem().toString();
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
            }
        });
        JButton linkTableClearButton = new JButton("Clear");
        linkTableClearButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                linkTableModel.clear();
                linkTableModel.fireTableDataChanged();
            }
        });
        linkTableMenuPanel.add(linkTableUpdateButton);
        linkTableMenuPanel.add(linkTableClearButton);


        //Link Table
        JPanel linkTablePanel = new JPanel();
        linkTablePanel.setBorder(BorderFactory.createTitledBorder(""));
        JTable linkTable = new JTable(linkTableModel);
        JTableHeader linkTableHeader = linkTable.getTableHeader();
        linkTableHeader.setDefaultRenderer(new HeaderRenderer(linkTable));
        linkTable.setAutoResizeMode(JTable.AUTO_RESIZE_OFF); //size
        linkTable.getColumnModel().getColumn(0).setPreferredWidth((int)(MAIN_WIDTH * 0.1));
        linkTable.getColumnModel().getColumn(1).setPreferredWidth((int)(MAIN_WIDTH * 0.1));
        linkTable.getColumnModel().getColumn(2).setPreferredWidth((int)(MAIN_WIDTH * 0.3));
        linkTable.getColumnModel().getColumn(3).setPreferredWidth((int)(MAIN_WIDTH * 0.1));
        linkTable.getColumnModel().getColumn(4).setPreferredWidth((int)(MAIN_WIDTH * 0.1));
        linkTable.getColumnModel().getColumn(5).setPreferredWidth((int)(MAIN_WIDTH * 0.3));
        DefaultTableCellRenderer centerRenderer = new DefaultTableCellRenderer();
        centerRenderer.setHorizontalAlignment(JLabel.CENTER);
        for ( int i = 0; i < linkTableModel.getColumnCount(); ++i){
            linkTable.getColumnModel().getColumn(i).setCellRenderer(centerRenderer);
        }
        linkTable.setFillsViewportHeight(true);
        
        JScrollPane linkTableScrollPane = new JScrollPane(linkTable);
        linkTableScrollPane.setPreferredSize(new Dimension(MAIN_WIDTH, 100));
        linkTablePanel.add(linkTableScrollPane);
        
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
        mainPanel.add(linkTableMenuPanel);
        mainPanel.add(linkTablePanel);
        mainPanel.add(logMenuPanel);
        mainPanel.add(logDisplayPanel);
        
        mainFrame.add(mainPanel);
        mainFrame.pack();
        mainFrame.repaint();
    }

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
 * This class for the model for Link Table 
 */
class LinkTableModel extends AbstractTableModel {
    private String[] columnNames = {"link", "iface", "peerIp:port", "peerType", "peerAdr", "rates"};
    private ArrayList<LinkTable> data;
    // private Object[][] data = {{"1", "1", "123.123.123.123:1234", "router", "2.10", "1000,1000"}};
    public LinkTableModel(){
        data = new ArrayList<LinkTable>();
    }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        LinkTable lt = data.get(row);
        if(lt == null){
            return null;
        }
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
}

/**
 * Link Table class
 */
class LinkTable {
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