package forest.control.console.model;

public class Route {
	private String comtree;
	private String addr;
	private String link;

	public Route(String comtree, String addr, String link) {
		this.comtree = comtree;
		this.addr = addr;
		this.link = link;
	}

	public String getComtree() {
		return comtree;
	}

	public String getAddr() {
		return addr;
	}

	public String getLink() {
		return link;
	}
}