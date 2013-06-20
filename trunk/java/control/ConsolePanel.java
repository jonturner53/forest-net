package forest.vworld1;

import java.awt.Color;
import java.awt.*;
import java.awt.event.*;
import java.awt.Graphics.*;
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

/** ConsolePanel is the paint panel used by Console.
 *  It displays a "network map" and supports various control operations.
 *
 *  @author Jon Turner
 */
public class ConsolePanel extends JPanel {

	Insets ins;

	/** Constructor for ConsolePanel.
	 */
	ConsolePanel(int size) {	
		setOpaque(true);
		setBorder(BorderFactory.createLineBorder(Color.BLUE,2));
		setPreferredSize(new Dimension(size,size));

		// initialize local fields
	}

	/** Initialize the walls object.
	 *  @param walls is an array of walls
	 *  @param worldSize defines the number of squares in the x and
	 *  y dimensions
	 */
	protected void setWalls(NetInfo net, ComtInfo comtrees) {
		this.net = net; this.comtrees = comtrees;
	}

	/** Paint the component.
	 *  @param g is the Graphics object for this panel
	 */
	protected void paintComponent(Graphics g) {
		super.paintComponent(g);
		ins = getInsets();
		drawWalls(g);
	}


	/** Draw walls.
	 */
	private void drawNet(Graphics g) {

// replace all with code for shwowing network

		Graphics2D g2 = (Graphics2D) g;
		g2.setStroke(new BasicStroke(1));
		g2.setPaint(Color.lightGray);

		double ss = 1.0/viewSize; // size of one square
		for (int i = 0; i <= viewSize; i++) {
			g.drawLine(xc(0),yc(ss*i),xc(1),yc(ss*i));
			g.drawLine(xc(ss*i),yc(0),xc(ss*i),yc(1));
		}

		g2.setStroke(new BasicStroke(4));
		g2.setPaint(Color.BLACK);
		for (int x = 0; x < worldSize; x++) {
			for (int y = 0; y < worldSize; y++) {
				int xy = x + y * worldSize;
				int row = y - cornerY; int col = x - cornerX;
				if (walls[xy] == 1 || walls[xy] == 3) {
					g.drawLine(
					    xc(ss*col), yc(ss*row),
					    xc(ss*col), yc(ss*(row+1)));
				} 
				if (walls[xy] == 2 || walls[xy] == 3) {
					g.drawLine(
					    xc(ss*col),yc(ss*(row+1)),
					    xc(ss*(col+1)),yc(ss*(row+1)));
				}
			}
		}
		// draw bottom walls where appropriate
		for (int x = cornerX; x < cornerX + viewSize; x++) {
			int y = cornerY - 1;
			if (y < 0) break;
			int xy = x + y * worldSize;
			int row = y - cornerY; int col = x - cornerX;
			if (walls[xy] == 2 || walls[xy] == 3) {
				g.drawLine(xc(ss*col),    yc(ss*(row+1)),
					     xc(ss*(col+1)),yc(ss*(row+1)));
			}
		}
		// draw right-side walls where appropriate
		for (int y = cornerY; y < cornerY + viewSize; y++) {
			int x = cornerX + viewSize;
			if (x >= worldSize) break;
			int xy = x + y * worldSize;
			int row = y - cornerY; int col = x - cornerX;
			if (walls[xy] == 1 || walls[xy] == 3) {
				g.drawLine(xc(ss*col), yc(ss*row),
					     xc(ss*col), yc(ss*(row+1)));
			} 
		}
		// draw boundary walls if all the way at edge of world
		if (cornerX == 0)
			g.drawLine(xc(0),yc(0),xc(0),yc(1));
		if (cornerX == worldSize-viewSize)
			g.drawLine(xc(1),yc(0),xc(1),yc(1));
		if (cornerY == 0)
			g.drawLine(xc(0),yc(0),xc(1),yc(0));
		if (cornerY == worldSize-viewSize)
			g.drawLine(xc(0),yc(1),xc(1),yc(1));

		g2.setStroke(new BasicStroke(2));
		g2.drawString("(" + cornerX + "," + cornerY + ")",
				xc(.01),yc(.01));
	}
}
