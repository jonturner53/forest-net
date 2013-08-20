package forest.control.console;

import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;

import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JTextField;

public class UpdateProfileDialog extends JPanel{
	
	private JLabel username;
	private JLabel realName;
	private JLabel email;
	private JTextField userNameInput;
	private JTextField realNameInput;
	private JTextField emailInput;
	
	public UpdateProfileDialog(){
		JPanel panel = new JPanel(new GridBagLayout());
        this.add(panel, BorderLayout.CENTER);
        GridBagConstraints c = new GridBagConstraints();
        
        username = new JLabel("ID: ");
        c.gridx = 0;
        c.gridy = 1;
        panel.add(username,c);
        userNameInput = new JTextField(15);
        c.gridx = 1;
        c.gridy = 1;
        panel.add(userNameInput,c);

        realName = new JLabel("Name: ");
        c.gridx = 0;
        c.gridy = 2;
        panel.add(realName, c);
        realNameInput = new JTextField(15);
        c.gridx = 1;
        c.gridy = 2;
        panel.add(realNameInput, c);
        
        email = new JLabel("EMail: ");
        c.gridx = 0;
        c.gridy = 3;
        panel.add(email, c);
        emailInput = new JTextField(15);
        c.gridx = 1;
        c.gridy = 3;
        panel.add(emailInput, c);

	}
	
	protected String getRealName(){
		return realNameInput.getText();
	}
	
	protected String getEmail(){
		return emailInput.getText();
	}
	
	protected String getUserName(){
		return userNameInput.getText();
	}
	protected void setUserName(String s){
		userNameInput.setText(s);
		userNameInput.setEditable(false);
	}
	protected void setRealName(String s){
		realNameInput.setText(s);
	}
	protected void setEmail(String s){
		emailInput.setText(s);
	}
}
