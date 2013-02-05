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

/** EMapPanel is the paint panel used by edit map.
 *  It displays a "walls map" and supports simple editing operations.
 *  By clicking on a wall segment (or place where a wall segment could be
 *  be placed) a wall segment is either added or removed.
 *
 *  @author Jon Turner
 */
public class EMapPanel extends JPanel {
	private static int MAX_WORLD = 5000; // max extent of virtual world
	private static int MAX_VIEW = 100; // max extent of view

	private int[] walls; 		// array of walls
	private int worldSize;		// # of squares in each dimension
	private int cornerX, cornerY;	// defines bottom left corner of view
	private int viewSize;		// defines # of squares in view

	Insets ins;
	Boolean flip;

	/** Constructor for EMapPanel.
	 */
	EMapPanel(int size) {	
		setOpaque(true);
		setBorder(BorderFactory.createLineBorder(Color.RED,2));
		setPreferredSize(new Dimension(size,size));
		flip = false;
		walls = null;
		worldSize = viewSize = 10;
		cornerX = cornerY = 0;
	}

	/** Initialize the walls object.
	 *  @param walls is an array of walls
	 *  @param worldSize defines the number of squares in the x and
	 *  y dimensions
	 */
	protected void setWalls(int[] walls, int worldSize) {
		this.walls = walls;
		this.worldSize = worldSize;
	}

	/** Set the position of the current view in the world.
	 *  @param x is the new x coordinate of the lower left corner
	 *  @param y is the new y coordinate of the lower left corner
	 */
	protected void setCorner(int x, int y) {
		cornerX = x; cornerY = y;
	}

	/** Set the size of the current view.
	 *  @param vs is the new view size (number of squares in each dimension)
	 */
	protected void setViewSize(int vs) {
		viewSize = vs;
	}

	/** Paint the component.
	 *  @param g is the Graphics object for this panel
	 */
	protected void paintComponent(Graphics g) {
		super.paintComponent(g);
		ins = getInsets();
		drawWalls(g);
	}

	/** Convert normalized coordinates to user-space coordinates.
	 *  The drawing routine uses unit coordinate space with the origin
	 *  at the bottom left.
	 *  @param xc is an x coordinate in the unit coordinate space
	 *  @return the corresponding user-space coordinate
	 */
	private int xc(double x) {
		int w = getWidth() - (ins.left + ins.right + 10);
		return ins.left+5 + ((int) (w*x));
	}

	/** Convert normalized coordinates to user-space coordinates.
	 *  The drawing routine uses unit coordinate space with the origin
	 *  at the bottom left.
	 *  @param yc is a y coordinate in the unit coordinate space
	 *  @return the corresponding user-space coordinate
	 */
	private int yc(double y) {
		int h = getHeight() - (ins.top + ins.bottom + 10);
		return getHeight() - (ins.bottom + 5 + ((int) (h*y)));
	}

	/** Add or remove a wall at a specified location, or block/unblock cell.
	 *  @param x is the x-coordinate of a point in user-space
	 *  @param y is the y-coordinate of a point in user-space
	 */
	public void flipWall(int x, int y) {
		double w = getWidth() - (ins.left+ins.right);
		double xd = (x - ins.left) / w;
		double h = getHeight() - (ins.top+ins.bottom);
		double yd = 1.0 - (y - ins.top) / h;

		double ss = 1.0/viewSize; // size of one square
		int col = cornerX + (int) (xd/ss);
		int row = cornerY + (int) (yd/ss);

		if (row < 0 || col < 0 || row >= worldSize || col >= worldSize)
			return;

		double x0 = (col-cornerX)*ss;
		double y0 = (row-cornerY)*ss;


		if (.25*ss < (xd-x0) && (xd-x0) < .75*ss &&
		    .25*ss < (yd-y0) && (yd-y0) < .75*ss) {
			walls[col+row*worldSize] ^= 4;
		} else if ((xd-x0) < (yd-y0) && (xd-x0) < (y0+ss-yd)) {
			walls[col+row*worldSize] ^= 1;
		} else if ((xd-x0) < (yd-y0) && (xd-x0) > (y0+ss-yd)) {
			walls[col+row*worldSize] ^= 2;
		} else if ((xd-x0) > (yd-y0) && (xd-x0) > (y0+ss-yd)) {
			if (col < worldSize-1)
				walls[(col+1)+row*worldSize] ^= 1;
		} else {
			if (row > 0)
				walls[col+(row-1)*worldSize] ^= 2;
		}
	}

	/** Draw walls.
	 */
	private void drawWalls(Graphics g) {
		if (walls == null) return;

		Graphics2D g2 = (Graphics2D) g;

		double ss = 1.0/viewSize; // size of one square

		int w = getWidth() - (ins.left + ins.right + 10);
		int ssc = (int) (ss * w);

		// draw squares
		for (int x = cornerX; x < cornerX+viewSize; x++) {
			for (int y = cornerY; y < cornerY+viewSize; y++) {
				int xy = x + y * worldSize;
				if ((walls[xy]&4) != 0)
					g2.setPaint(Color.gray);
				else
					g2.setPaint(Color.white);
				int row = y - cornerY; int col = x - cornerX;
				g.fillRect(xc(ss*col),yc(ss*(row+1)),ssc,ssc);
			}
		}

		// draw grid lines
		g2.setStroke(new BasicStroke(1));
		g2.setPaint(Color.lightGray);
		for (int i = 0; i <= viewSize; i++) {
			g.drawLine(xc(0),yc(ss*i),xc(1),yc(ss*i));
			g.drawLine(xc(ss*i),yc(0),xc(ss*i),yc(1));
		}

		// draw walls
		g2.setStroke(new BasicStroke(4));
		g2.setPaint(Color.BLACK);
		for (int x = cornerX; x < cornerX+viewSize; x++) {
			for (int y = cornerY; y < cornerY+viewSize; y++) {
				int xy = x + y * worldSize;
				int row = y - cornerY; int col = x - cornerX;
				if ((walls[xy]&1) != 0) {
					g.drawLine(
					    xc(ss*col), yc(ss*row),
					    xc(ss*col), yc(ss*(row+1)));
				} 
				if ((walls[xy]&2) != 0) {
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
