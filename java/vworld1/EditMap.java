package forest.vworld1;

import java.awt.Color;
import java.awt.*;
import java.awt.event.*;
import java.util.HashMap;
import java.util.Map;
import java.util.HashSet;
import java.util.Set;
import java.util.Iterator;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.util.Scanner;
import javax.swing.*;

/**
 *  EditMap reads a map file and allows the user to edit the walls
 *  by clicking on them.
 *  @author Jon Turner
 */
public class EditMap extends MouseAdapter implements ActionListener {
	private static int MAX_WORLD = 5000; // max extent of virtual world
	private static int MAX_VIEW = 100; // max extent of view

	private static int worldSize;	// # of squares in full map
	private static int viewSize;	// # of map squares viewed at one time
	private static int cornerX = 0; // x coordinate of current view
	private static int cornerY = 0; // y coordinate of current view
	private static int[] walls; 	// array of walls

	/** The EditMap program implements a simple user interface for
	 *  editing map files that specify walls. The main method simply
	 *  launches the user interface.
	 */
	public static void main(String[] args) {	
		SwingUtilities.invokeLater(new Runnable() {
			public void run() { new EditMap(); }
		});
	}

	JTextField inFileText, outFileText;
	EMapPanel panel;
	boolean newInFmt = true;
	boolean newOutFmt = true;

	/** Constructor for EditMap class.
	 *  Setup the various user interface components.
	 */
	EditMap() {
		JFrame jfrm = new JFrame("EditMap");
		jfrm.getContentPane().setLayout(new FlowLayout());
		jfrm.setSize(650,720);
		jfrm.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		// check box to select new input format
		JCheckBox newInFmtBtn = new JCheckBox("new in");
		newInFmtBtn.setSelected(true);
		newInFmtBtn.addActionListener(this);

		// text field and button for opening input file
		inFileText = new JTextField(10);
		inFileText.addActionListener(this);
		inFileText.setActionCommand("inFileName");

		JButton openBtn = new JButton("open");
		openBtn.addActionListener(this);

		Box b1 = Box.createHorizontalBox();
		b1.add(newInFmtBtn); b1.add(inFileText); b1.add(openBtn);

		// check box to select new output format
		JCheckBox newOutFmtBtn = new JCheckBox("new out");
		newOutFmtBtn.setSelected(true);
		newOutFmtBtn.addActionListener(this);

		// text field for output file and save button
		outFileText = new JTextField(10);
		outFileText.addActionListener(this);
		outFileText.setActionCommand("outFileName");

		JButton saveBtn = new JButton("save");
		saveBtn.addActionListener(this);

		Box b2 = Box.createHorizontalBox();
		b2.add(newOutFmtBtn); b2.add(outFileText); b2.add(saveBtn);

		// build box containing file controls
		Box fileBox = Box.createVerticalBox();
		fileBox.add(b1); fileBox.add(b2);

		// group of buttons for zooming
		JButton zoomInBtn = new JButton("zoom in");
		JButton zoomOutBtn = new JButton("zoom out");
		zoomInBtn.addActionListener(this);
		zoomOutBtn.addActionListener(this);

		Box b3 = Box.createVerticalBox();
		b3.add(zoomInBtn); b3.add(zoomOutBtn);

		// group of buttons for panning
		JButton leftBtn = new JButton("left");
		JButton rightBtn = new JButton("right");
		leftBtn.addActionListener(this);
		rightBtn.addActionListener(this);

		Box b4 = Box.createVerticalBox();
		b4.add(leftBtn); b4.add(rightBtn);

		JButton upBtn = new JButton("up");
		JButton downBtn = new JButton("down");
		upBtn.addActionListener(this);
		downBtn.addActionListener(this);

		Box b5 = Box.createVerticalBox();
		b5.add(upBtn); b5.add(downBtn);

		Box topBar = Box.createHorizontalBox();
		topBar.add(fileBox);
		topBar.add(b3);
		topBar.add(b4);
		topBar.add(b5);

		jfrm.add(topBar);

		panel = new EMapPanel(600);
		panel.addMouseListener(this);
		jfrm.add(panel);

		jfrm.setVisible(true);
	}

	/** Respond to user interface events.
	 *  ae is an action event relating to one of the user interface
	 *  elements.
	 */
	public void actionPerformed(ActionEvent ae) {
		if (ae.getActionCommand().equals("new in")) {
			newInFmt = ! newInFmt;
		} else if (ae.getActionCommand().equals("new out")) {
			newOutFmt = ! newOutFmt;
		} else if (ae.getActionCommand().equals("left")) {
			if (cornerX > 0) {
				cornerX--; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("right")) {
			if (cornerX + viewSize < worldSize) {
				cornerX++; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("down")) {
			if (cornerY > 0) {
				cornerY--; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("up")) {
			if (cornerY + viewSize < worldSize) {
				cornerY++; panel.setCorner(cornerX,cornerY);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("zoom in")) {
			if (viewSize > 0) {
				viewSize--; panel.setViewSize(viewSize);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("zoom out")) {
			if (cornerX + viewSize < worldSize &&
			    cornerY + viewSize < worldSize) {
				viewSize++; panel.setViewSize(viewSize);
				panel.repaint();
			}
		} else if (ae.getActionCommand().equals("open") ||
		    ae.getActionCommand().equals("inFileName")) {
			if (newInFmt)
				readNewFile(inFileText.getText());
			else
				readOldFile(inFileText.getText());
			outFileText.setText(inFileText.getText());
			panel.repaint();
		} else if (ae.getActionCommand().equals("save") || 
		    	   ae.getActionCommand().equals("outFileName")) {
			if (newOutFmt)
				writeNewFile(outFileText.getText());
			else
				writeOldFile(outFileText.getText());
		}
	}

	/** Respond to mouse click in the panel by turning wall on/off.
	 *  @param e is a mouse event corresponding to a click in the panel.
	 */
	public void mouseClicked(MouseEvent e) {
		panel.flipWall(e.getX(), e.getY());
		panel.repaint();
	}

	/** Read an input file.
	 *  @param fileName is the name of the input file to be read
	 */
	private void readOldFile(String fileName) {
		try {
			Scanner imap = new Scanner(new File(fileName));
			int y = 0; walls = null;
			while (imap.hasNextLine() && y >= 0) {
				String temp = imap.nextLine();
				if (walls == null) {
					worldSize = temp.length();
					y = worldSize-1;
					viewSize = Math.min(10,worldSize);
					walls = new int[worldSize*worldSize];
				} else if (temp.length() != worldSize) {
					System.out.println("Map file has "
						 + "unequal line lengths");
					System.exit(1);
				}
				for(int x = 0; x < worldSize; x++) {
					if(temp.charAt(x) == '+') {
						walls[y * worldSize + x] = 3;
					} else if(temp.charAt(x) == '-') {
						walls[y * worldSize + x] = 2;
					} else if(temp.charAt(x) == '|') {
						walls[y * worldSize + x] = 1;
					} else if(temp.charAt(x) == ' ') {
						walls[y * worldSize + x] = 0;
					} else {
						System.out.println(
						  "Unrecognized symbol in map "
					  	  + "file!");
					}
				}
				y--;
			}
			if (y >= 0) {
				System.out.println("Map file incomplete");
				System.exit(1);
			}
		} catch (Exception e) {
			System.out.println("EditMap cannot read " + fileName);
			System.out.println(e);
			System.exit(1);
		}
		panel.setWalls(walls,worldSize);
	}

	/** Read a new format input file.
	 *  @param fileName is the name of the input file to be read
	 */
	private void readNewFile(String fileName) {
		try {
			Scanner imap = new Scanner(new File(fileName));
			int y = 0; walls = null;
			boolean horizRow = true;
			while (imap.hasNextLine() && y >= 0) {
				String temp = imap.nextLine();
				if (walls == null) {
					worldSize = temp.length()/2;
					y = worldSize-1;
					viewSize = Math.min(10,worldSize);
					walls = new int[worldSize*worldSize];
				} else if (temp.length()/2 != worldSize) {
					System.out.println("Map file has "
						 + "mismatched line lengths");
					System.exit(1);
				}
				for(int xx = 0; xx < 2*worldSize; xx++) {
					int pos = y * worldSize + xx/2;
					if (horizRow) {
						if ((xx&1) == 0) continue;
						if (temp.charAt(xx) == '-')
							walls[pos] |= 2;
						continue;
					}
					if ((xx&1) != 0) {
						if (temp.charAt(xx) == 'x')
							walls[pos] |= 4;
					} else if (temp.charAt(xx) == '|') 
						walls[pos] |= 1;
				}
				horizRow = !horizRow;
				if (horizRow) y--;
			}
			if (y >= 0) {
				System.out.println("Map file incomplete");
				System.exit(1);
			}
		} catch (Exception e) {
			System.out.println("EditMap cannot read " + fileName);
			System.out.println(e);
			System.exit(1);
		}
		panel.setWalls(walls,worldSize);
	}

	/** Write an old format output file.
	 *  @param fileName is the name of the output file to be written
	 */
	private void writeOldFile(String fileName) {
		try {
			FileWriter omap = new FileWriter(new File(fileName));
			for (int y = worldSize-1; y >= 0; y--) {
				for (int x = 0; x < worldSize; x++) {
					switch (walls[x+y*worldSize]) {
					case 0:
						omap.write(' '); break;
					case 1:
						omap.write('|'); break;
					case 2:
						omap.write('-'); break;
					case 3:
						omap.write('+'); break;
					}
				}
				omap.write('\n');
			}
			omap.close();
		} catch (Exception e) {
			System.out.println("EditMap cannot write " + fileName);
			System.out.println(e);
			System.exit(1);
		}
	}

	/** Write an old format output file.
	 *  @param fileName is the name of the output file to be written
	 */
	private void writeNewFile(String fileName) {
		try {
			FileWriter omap = new FileWriter(new File(fileName));
			for (int y = worldSize-1; y >= 0; y--) {
				for (int x = 0; x < worldSize; x++) {
					omap.write(' ');
					if ((walls[x+y*worldSize]&2) != 0)
						omap.write('-');
					else
						omap.write(' ');
				}
				omap.write('\n');
				for (int x = 0; x < worldSize; x++) {
					if ((walls[x+y*worldSize]&1) != 0)
						omap.write('|');
					else
						omap.write(' ');
					if ((walls[x+y*worldSize]&4) != 0)
						omap.write('x');
					else
						omap.write(' ');
				}
				omap.write('\n');
			}
			omap.close();
		} catch (Exception e) {
			System.out.println("EditMap cannot write " + fileName);
			System.out.println(e);
			System.exit(1);
		}
	}
}
