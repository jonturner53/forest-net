package forest.control.console.model;

public class Comtree {
	private String comtree;
	private String inCore;
	private String pLink;
	private String comtLinks;
	private String coreLinks;

	public Comtree(String comtree, String inCore, String pLink, String link,
			String coreLinks) {
		this.comtree = comtree;
		this.inCore = inCore;
		this.pLink = pLink;
		this.comtLinks = link;
		this.coreLinks = coreLinks;
	}

	public String getComtree() {
		return comtree;
	}

	public String getInCore() {
		return inCore;
	}

	public String getpLink() {
		return pLink;
	}

	public String getComtLinks() {
		return comtLinks;
	}

	public String getCoreLink() {
		return coreLinks;
	}
}
