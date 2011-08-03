import java.awt.Dimension;
import java.awt.Color;

public class Common{
	protected static final int NUMPORTS = 17;
	//Graphics stuff
	protected static final int FIELD_WIDTH = 6;
	protected static final Dimension SIZE = new Dimension(800, 600);
	protected static final Color COLOR = new Color(210,230,255); 
	//Component IDs
	protected static final int ROUTER = 0;
	protected static final int CONTROLLER = 1;
	protected static final int CLIENT = 2;
	protected static final int LINK = 3;
	protected static final int COMTREE = 4;
	protected static final int SAVE = 5;
	protected static final int CLOSE = 6;
	protected static final int COM = 7;
	protected static final String[] types = {"router", "controller", "client", "link", "comtree", "save", "close", "com"};
}
