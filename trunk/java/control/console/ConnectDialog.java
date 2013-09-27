package forest.control.console;

import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;

import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

public class ConnectDialog extends JPanel {

	private static final long serialVersionUID = -6236529665506981616L;
	private JLabel nmAddr;
	private JTextField nmAddrInput;
	private JLabel ctAddr;
	private JTextField ctAddrInput;

	public ConnectDialog() {
		JPanel panel = new JPanel(new GridBagLayout());
		this.add(panel, BorderLayout.CENTER);
		GridBagConstraints c = new GridBagConstraints();

		nmAddr = new JLabel("NetMgr Address: ");
		c.gridx = 0;
		c.gridy = 1;
		panel.add(nmAddr, c);
		nmAddrInput = new JTextField(20);
		nmAddrInput.setText("forest1.arl.wustl.edu");
		// nmAddrInput.setText("localhost");
		c.gridx = 1;
		c.gridy = 1;
		panel.add(nmAddrInput, c);

		ctAddr = new JLabel("ComtCtl Address: ");
		c.gridx = 0;
		c.gridy = 2;
		panel.add(ctAddr, c);
		ctAddrInput = new JTextField(20);
		ctAddrInput.setText("forest4.arl.wustl.edu");
		// ctAddrInput.setText("localhost");
		c.gridx = 1;
		c.gridy = 2;
		panel.add(ctAddrInput, c);

	}

	public String getNmAddr() {
		return nmAddrInput.getText();
	}

	public String getCtAddr() {
		return ctAddrInput.getText();
	}
}
