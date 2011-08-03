import java.util.*;
import java.awt.*;
import javax.swing.*;

public class topologyGen extends JFrame{
	private int width, height;
	public topologyGen(){				
		width = 800;
		height = 600;
		setPreferredSize(new Dimension(width, height));
		getContentPane().add(new Panel());
		setTitle("topologyGen");
		setDefaultCloseOperation(EXIT_ON_CLOSE);
		pack();
		validate();
		setVisible(true);
	}
	
	public static void main(String[] args){
		topologyGen gen = new topologyGen();
	}

}
