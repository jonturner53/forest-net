package forest.control.console;

import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;

import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JTextField;

public class LoginDialog extends JPanel {

	private static final long serialVersionUID = 2956041449726807762L;
	private JLabel username;
	private JLabel password;
	private JTextField usernameInput;
	private JPasswordField passwordInput;

	public LoginDialog() {
		JPanel panel = new JPanel(new GridBagLayout());
		this.add(panel, BorderLayout.CENTER);
		GridBagConstraints c = new GridBagConstraints();

		username = new JLabel("ID: ");
		c.gridx = 0;
		c.gridy = 1;
		panel.add(username, c);
		usernameInput = new JTextField(10);

		c.gridx = 1;
		c.gridy = 1;
		panel.add(usernameInput, c);

		password = new JLabel("Password: ");
		c.gridx = 0;
		c.gridy = 2;
		panel.add(password, c);
		passwordInput = new JPasswordField(10);

		c.gridx = 1;
		c.gridy = 2;
		panel.add(passwordInput, c);

	}

	protected char[] getPassword() {
		return passwordInput.getPassword();
	}

	protected String getUserName() {
		return usernameInput.getText();
	}

	public void clearAllTextField() {
		usernameInput.setText("");
		passwordInput.setText("");
	}
}
