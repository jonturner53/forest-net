package forest.control.console.model;

public class LogFilters {
	private String inAndOut;
	private String link;
	private String comtree;
	private String type;
	private String cpType;
	
	public LogFilters(String inAndOut, String link,
						String comtree, String type,
						String cpType){
		this.inAndOut = inAndOut;
		this.link = link; this.comtree = comtree;
		this.type = type; this.cpType = cpType;
	}

	public String getInAndOut() {
		return inAndOut;
	}

	public String getLink() {
		return link;
	}

	public String getComtree() {
		return comtree;
	}

	public String getType() {
		return type;
	}

	public String getCpType() {
		return cpType;
	}
	
	
}
