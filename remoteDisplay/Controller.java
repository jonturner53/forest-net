package remoteDisplay;
import princeton.StdDraw;
import java.util.regex.*;

public class Controller extends NetObj{
	public Controller(String nme, String type, String ip, String fAdr, String xStr, String yStr){
		name = nme;
		nodeType = type;
		ipAdr = ip;
		forestAdr = fAdr;
		x = Integer.parseInt(xStr);
		y = Integer.parseInt(yStr);
		weight = 0;
	}
	
	@Override
	public String toString() {
		return name + " " + nodeType + " " + ipAdr + " " + forestAdr;
	}
}
