package forest.vworld1;

import java.awt.Color;
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
import princeton.StdDraw;
import java.util.Scanner;
/**
 *  ShowWalls displays a walls file.
 *  @author Jon Turner
 */
public class ShowWalls {
	private static int MAX_WORLD = 30000; // max extent of virtual world
	private static int MAX_VIEW = 1000; // max extent of view

	private static int worldSize;	// # of squares in full map
	private static int viewSize;	// # of map squares viewed at one time
	private static int cornerX = 0; // x coordinate of current view
	private static int cornerY = 0; // y coordinate of current view
	private static Scanner mazeFile; // filename of maze
	private static int[] walls; 	// list of walls

	/**
	 * The ShowWorldNet program displays a visualization of a very
	 * simple virtual world. Avatars moving around in the virtual
	 * world are displayed as they move.
	 *
	 *  Command line arguments
	 *   - name/address of monitor host (or localhost)
	 *   - walls file
	 */
	public static void main(String[] args) {	
		// process command line arguments	
		processArgs(args);	
		StdDraw.setCanvasSize(700,700);
		StdDraw.setPenRadius(.003);
		Color c = new Color(210,230,255);
		viewSize = worldSize;
		drawGrid(c);
		StdDraw.show(10);
	}

	/**
	 *  Process command line arguments
	 *   - walls file
	 */
	private static boolean processArgs(String[] args) {
		try {
			mazeFile = new Scanner(new File(args[0]));
			int y = 0; walls = null;
			while (mazeFile.hasNextLine() && y >= 0) {
				String temp = mazeFile.nextLine();
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
			System.out.println("usage: ShowWalls mapfile");
			System.out.println(e);
			System.exit(1);
		}
		return true;
	}

	/** Draw grid with a background color of c.
	 */
	private static void drawGrid(Color c) {
		StdDraw.clear(c);
		StdDraw.setPenRadius(.002);
		StdDraw.setPenColor(Color.GRAY);
		double frac = 1.0/viewSize;

		if (viewSize <= 40) {
			for (int i = 0; i <= viewSize; i++) {
				StdDraw.line(0,frac*i,1,frac*i);
				StdDraw.line(frac*i,0,frac*i,1);
			}
		}

		StdDraw.setPenRadius(Math.min(.006,
				     Math.max(.001,
					      .006*(4/Math.sqrt(viewSize)))));
		StdDraw.setPenColor(Color.BLACK);
		for (int x = cornerX; x < cornerX + viewSize; x++) {
			for (int y = cornerY; y < cornerY + viewSize; y++) {
				int xy = x + y * worldSize;
				int row = y - cornerY; int col = x - cornerX;
				if (walls[xy] == 1 || walls[xy] == 3) {
					StdDraw.line(frac*col, frac*row,
						     frac*col, frac*(row+1));
				} 
				if (walls[xy] == 2 || walls[xy] == 3) {
					StdDraw.line(frac*col,    frac*(row+1),
						     frac*(col+1),frac*(row+1));
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
				StdDraw.line(frac*col,    frac*(row+1),
					     frac*(col+1),frac*(row+1));
			}
		}
		// draw right-side walls where appropriate
		for (int y = cornerY; y < cornerY + viewSize; y++) {
			int x = cornerX + viewSize;
			if (x >= worldSize) break;
			int xy = x + y * worldSize;
			int row = y - cornerY; int col = x - cornerX;
			if (walls[xy] == 1 || walls[xy] == 3) {
				StdDraw.line(frac*col, frac*row,
					     frac*col, frac*(row+1));
			} 
		}
		// draw boundary walls if all the way at edge of world
		if (cornerX == 0) StdDraw.line(0,0,0,1);
		if (cornerX == worldSize-viewSize) StdDraw.line(1,0,1,1);
		if (cornerY == 0) StdDraw.line(0,0,1,0);
		if (cornerY == worldSize-viewSize) StdDraw.line(0,1,1,1);

		StdDraw.setPenRadius(.003);
		StdDraw.text(.05, -.025, "(" + cornerX + "," + cornerY + ")");
	}
}
