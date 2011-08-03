import princeton.StdDraw;
import java.util.regex.*;

public class Router extends NetObj{
	private int numClients;
	private String zipcode;
	//router constructor
	public Router(String nme, String type, String fAdr, String xStr, String yStr){
		name = nme;
		nodeType = type;
		forestAdr = fAdr;
		x = Integer.parseInt(xStr);
		y = Integer.parseInt(yStr);
		Pattern p = Pattern.compile("(\\d+)");
		Matcher m = p.matcher(name);
		StringBuffer sb = new StringBuffer();
		while(m.find())
			sb.append(m.group());
		zipcode = sb.toString();
	}
		
	public void setNumClients(int num){
		numClients = num;
	}	

	public int getNumClients(){
		return numClients;
	}

	public String getZip(){
		return zipcode;
	}

	public String getForestAdr(String zip){
		Pattern p = Pattern.compile("\\d+\\.");
		Matcher m = p.matcher(forestAdr);
		m.find();
		if(zip.equals(m.group()))
			return forestAdr;
		else
			return null;
	}

	@Override
	public String toString() {
		return name + " " + nodeType + " " + forestAdr;
	}

}
