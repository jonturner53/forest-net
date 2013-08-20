package forest.control.console;

import java.awt.BorderLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;

import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

public class ConnectDialog extends JPanel{
	
	private JLabel addr;
	private JTextField addrInput;
	
	public ConnectDialog(String s){
		JPanel panel = new JPanel(new GridBagLayout());
        this.add(panel, BorderLayout.CENTER);
        GridBagConstraints c = new GridBagConstraints();
        
        addr = new JLabel("Address: ");
        c.gridx = 0;
        c.gridy = 1;
        panel.add(addr,c);
        addrInput = new JTextField(20);
        addrInput.setText(s);
        c.gridx = 1;
        c.gridy = 1;
        panel.add(addrInput,c);

	}
	protected String getAddr(){
		return addrInput.getText();
	}
}
