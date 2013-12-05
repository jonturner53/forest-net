package forest.control.console;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.ArrayList;
import java.util.HashMap;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.Timer;
import javax.swing.WindowConstants;

public class LogFrame extends JFrame{

	private static final long serialVersionUID = 2867888389068887373L;

	private static final int PERIOD_TIME = 1; //second
	private final int WIDTH = 750;
	private final int HEIGHT = 340;

	private JPanel logDisplayPanel;
	private JTextArea logTextArea;
	private JScrollPane logTextAreaScrollPane;

	private JPanel menuPanel;
	private JButton clearLogBtn;
	private JCheckBox localLogChBox;
	private JCheckBox onChBox;

	private ConnectionNetMgr connectionNetMgr;
	private HashMap<String,LogFrame> hashMapLogFrame;

	private String rtr; //router name

	private Timer timer;

	public LogFrame(String rtrName, ConnectionNetMgr connectionNetMgr, 
			HashMap<String,LogFrame> hashMapLogFrame) {
		this.connectionNetMgr = connectionNetMgr;
		this.hashMapLogFrame = hashMapLogFrame;
		this.rtr = rtrName;

		this.setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
		this.setTitle(this.rtr);
		this.setPreferredSize(new Dimension(WIDTH,HEIGHT));
		this.setLayout(new BorderLayout());

		menuPanel = new JPanel();
		menuPanel.setPreferredSize(new Dimension(WIDTH,40));

		onChBox = new JCheckBox("Log On");
		onChBox.setSelected(false);
		onChBox.addItemListener(new ItemListener() {

			@Override
			public void itemStateChanged (ItemEvent e) {
				boolean on = false;
				boolean local = localLogChBox.isSelected();
				if (e.getStateChange() == ItemEvent.SELECTED) {
					on = true;
				} else {
					on = false;
				}

				//FIXME: check if logged in or connected
//				if (isLoggedIn && isConnected) {
					String ret = enablePacketLog(rtr, on, local);
					if (ret == null) {
						if (on) {
							autoRefreshUpdateLog();
						}
					} else {
						showPopupStatus(ret);
					}
//				}
			}
		});
		menuPanel.add(onChBox);

		localLogChBox = new JCheckBox("Local"); //local checkbox
		localLogChBox.setSelected(false);
		localLogChBox.addItemListener(new ItemListener() {
			@Override
			public void itemStateChanged (ItemEvent e) {
				boolean on = onChBox.isSelected();
				boolean local = false;
				if (e.getStateChange() == ItemEvent.SELECTED)
					local = true;
				else
					local = false;

				//FIXME: check if logged in or connected
//				if (isLoggedIn && isConnected) {
					enablePacketLog(rtr, on, local);
//				}
			}
		});
		menuPanel.add(localLogChBox);


		clearLogBtn = new JButton("Clear");
		clearLogBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed (ActionEvent e) {
				clearLogs();
			}
		});
		menuPanel.add(clearLogBtn);

		logDisplayPanel = new JPanel();
		logTextArea = new JTextArea(16, 58);
		logTextArea.setLineWrap(true);
		logTextArea.setEditable(true);
		logTextAreaScrollPane = new JScrollPane(logTextArea);
		logDisplayPanel.add(logTextAreaScrollPane);

		this.add(menuPanel,BorderLayout.PAGE_START);
		this.add(logDisplayPanel, BorderLayout.PAGE_END);
		
		//FIXME: Should make the timer stopped before close
		this.addWindowListener(new CloseEventInLogFrame(this));
		this.pack();
	}

	/**
	 * Write string to Log Text Area
	 * @param str String
	 */
	public void write(String str) {
		logTextArea.append(str);
	}

	/**
	 * Clear Logs in text area
	 */
	public void clearLogs() {
		logTextArea.setText("");
	}
	
	/**
	 * Retreiving packeted log from remote log periodically say 1s
	 */
	public void autoRefreshUpdateLog(){
		final ArrayList<String> logs = new ArrayList<String>();
		timer = new Timer(PERIOD_TIME * 1000, new ActionListener() {
			public void actionPerformed (ActionEvent evt) {
				if (!onChBox.isSelected()) {
					timer.stop();
				} else {
					
					//FIXME: check if logged in or connected
//					if (isLoggedIn && isConnected) {
						logs.clear(); //remove all log strings in arrayList
						StringBuilder sb = new StringBuilder();
						String s = connectionNetMgr.getLoggedPackets(logs, rtr);
						if (s != null) {
							showPopupStatus(s);
						} else {
							for (String l : logs) {
								if (l.equals("")) break;
								sb.append(l); sb.append("\n");
							}
						}
						if (sb.length() > 0) {
							write(sb.toString());
						}
//					}
				}
			}
		});
		timer.start();
	}

	/**
	 * Enable packet log in remote router
	 * @param rtr router name
	 * @param on controls whether logging is to be turned on (true) or off (false)
	 * @param local controls local logging in the same way
	 * @return null if success, otherwise error message
	 */
	private String enablePacketLog(String rtr, boolean on, boolean local) {
		String ret = connectionNetMgr.enablePacketLog(rtr, on, local);
		return ret;
	}
	
	/**
	 * Popup message
	 * @param status
	 */
	public void showPopupStatus(String status) {
		JOptionPane.showMessageDialog(this, status);
	}
	
	/**
	 * Get Timer
	 * @return 
	 */
	public Timer getTimer() {
		return timer;
	}
	
	public HashMap<String, LogFrame> getHashMap() {
		return hashMapLogFrame;
	}
	
	public String getRouterName() {
		return rtr;
	}
}

/**
 * Close Event, stopping timer
 * @author doowon
 *
 */
class CloseEventInLogFrame extends WindowAdapter {
	private LogFrame logFrame;
	public CloseEventInLogFrame(LogFrame logFrame) {
		this.logFrame = logFrame;
	}
	
	@Override
	public void windowClosing(WindowEvent e) {
		if (logFrame.getTimer() != null) {
			logFrame.getTimer() .stop();
			System.out.println("timer stopped");
		}
		logFrame.getHashMap().remove(logFrame.getRouterName());
	}
}
