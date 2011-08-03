import java.util.regex.*;
import java.util.*;
import java.io.*;

public class TopoParser{
	private static final String nodes = "nodes";
	private static final String interfaces = "interfaces";
	private static final String controller = "controller";
	private static final String links = "links";
	private static final String comtrees = "comtrees";
	String pathname;
	ArrayList<NetObj> topology;
	
	public TopoParser(String path){
		pathname = path;
		topology = new ArrayList<NetObj>();
		try {
			parse();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		}

	}
	
	public void parse() throws FileNotFoundException{
		Scanner in = new Scanner(new FileInputStream(pathname));
		ArrayList<ArrayList<String>> topology_raw = new ArrayList<ArrayList<String>>();
		ArrayList<String> builder = new ArrayList<String>();
		while(in.hasNextLine()){	
			//filters comments
			String s = in.nextLine().split("#")[0];
			if(!s.isEmpty()){
				if(s.trim().equalsIgnoreCase(".")){
					topology_raw.add(builder);
					builder = new ArrayList<String>();
				}
				else
					builder.add(s.trim());
			}
		}
		
		//build routers into data structure
		int count = 0;
		String[] temp = new String[4];
		Pattern p = Pattern.compile("[\\s]+");// Split input with the pattern excluding all whitespaces
		for(int i = 0; i < topology_raw.size(); i++){
			for(int j = 1; j < topology_raw.get(i).size(); j++){
				String[] s = p.split(topology_raw.get(i).get(j));
				String header = topology_raw.get(i).get(0).trim().replace(":", "");
				
				if(header.equalsIgnoreCase(nodes))
					topology.add(new Router(s[0], s[1], s[2], s[3], s[4]));
				else if(header.equalsIgnoreCase(interfaces))
					topology.add(new Interface(s[0], s[1], s[2], s[3]));
				else if(header.equalsIgnoreCase(controller))
					topology.add(new Controller(s[0], s[1], s[2], s[3], s[4], s[5]));
				else if(header.equalsIgnoreCase(links))
					topology.add(new Link(s[0], s[1], s[2]));
				else if(header.equalsIgnoreCase(comtrees)){
					temp[count%4] = s[0];
					if(count%4 == 3){
						Comtree c = new Comtree(temp[0], temp[1], temp[2], temp[3]);
						topology.add(c);	
					}
					count++;
				}
			}
		}
	}
}
