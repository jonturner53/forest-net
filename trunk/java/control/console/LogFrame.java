package forest.control.console;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.WindowConstants;

public class LogFrame extends JFrame{
	
	private static final long serialVersionUID = 2867888389068887373L;
	private final int WIDTH = 750;
	private final int HEIGHT = 300;
	
	private JPanel logDisplayPanel;
	private JTextArea logTextArea;
	private JScrollPane logTextAreaScrollPane;
	
	private JPanel menuPanel;
	private JButton clearLogBtn;
	
	public LogFrame(String title) {
		this.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
		this.setTitle(title);
		this.setPreferredSize(new Dimension(WIDTH,HEIGHT));
		
		menuPanel = new JPanel();
		menuPanel.setPreferredSize(new Dimension(WIDTH,50));
		clearLogBtn = new JButton("Clear");
		clearLogBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed (ActionEvent e) {
				
			}
		});
		menuPanel.add(clearLogBtn);
		
		logDisplayPanel = new JPanel();
		logTextArea = new JTextArea(16, 58);
		logTextArea.setLineWrap(true);
		logTextArea.setEditable(true);
		logTextAreaScrollPane = new JScrollPane(logTextArea);
		logDisplayPanel.add(logTextAreaScrollPane);
		
		this.add(menuPanel);
		this.add(logDisplayPanel);
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
	
}
