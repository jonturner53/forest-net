package remoteDisplay;
import java.util.regex.Pattern;
import princeton.StdDraw;

public class Interface extends NetObj{

	private String[] links;
	private String ifNum;
	public Interface(String nme, String ifn, String ip, String lk){
		name = nme;
		ifNum = ifn;
		ipAdr = ip;
		Pattern p = Pattern.compile("[,\\s]+");// Split input with the pattern excluding all whitespaces
		links = p.split(lk);
	}

	@Override
	public String toString() {
		// TODO Auto-generated method stub
		StringBuilder sb = new StringBuilder();
		for(String str: links)
			sb.append(str + " ");
		return name + " " + ipAdr + " " + sb.toString().trim();
	}
}
