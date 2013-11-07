package forest.control.console.model;

public class LogFilter {
	private int id;
	private boolean on;
	private String rtr;
	private boolean in;
	private boolean out;
	private String srcAdr;
	private String dstAdr;
	private String link;
	private String comtree;
	private String type;
	private String cpType;

	public LogFilter() {
	}
	
	public boolean getOn (){
		return on;
	}
	
	public int getId () {
		return id;
	}

	public boolean getIn () {
		return in;
	}

	public boolean getOut () {
		return out;
	}

	public String getLink () {
		return link;
	}

	public String getComtree () {
		return comtree;
	}

	public String getType () {
		return type;
	}

	public String getCpType () {
		return cpType;
	}

	public String getRtr () {
		return rtr;
	}

	public void setOn (boolean on) {
		this.on = on;
	}
	
	public void setId (int id) {
		this.id = id;
	}

	public void setRtr (String rtr) {
		this.rtr = rtr;
	}

	public void setIn (boolean in) {
		this.in = in;
	}

	public void setOut (boolean out) {
		this.out = out;
	}

	public void setLink (String link) {
		this.link = link;
	}

	public void setComtree (String comtree) {
		this.comtree = comtree;
	}

	public void setType (String type) {
		this.type = type;
	}

	public void setCpType (String cpType) {
		this.cpType = cpType;
	}

	public String getSrcAdr () {
		return srcAdr;
	}

	public void setSrcAdr (String srcAdr) {
		this.srcAdr = srcAdr;
	}

	public String getDstAdr () {
		return dstAdr;
	}

	public void setDstAdr (String dstAdr) {
		this.dstAdr = dstAdr;
	}
	
	public String toString() {
		return id + " "  + on + " " + rtr + " " + in + " " + out + " "
				+ srcAdr + " " + dstAdr + " " + link + " " + comtree + " "
				+ type + " " + cpType + "\n";
	}
}
