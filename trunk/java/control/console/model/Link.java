package forest.control.console.model;

/**
 * @file LinkTable.java
 * 
 * @author Doowon Kim
 * @date 2013 This is open source software licensed under the Apache 2.0
 *       license. See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

public class Link {
	private String link;
	private String iface;
	private String peerIpAndPort;
	private String peerType;
	private String peerAdr;
	private String rates;

	public Link(String link, String iface, String peerIpAndPort,
			String peerType, String peerAdr, String rates) {
		this.link = link;
		this.iface = iface;
		this.peerIpAndPort = peerIpAndPort;
		this.peerType = peerType;
		this.peerAdr = peerAdr;
		this.rates = rates;
	}

	public String getLink() {
		return link;
	}

	public String getIface() {
		return iface;
	}

	public String getPeerIpAndPort() {
		return peerIpAndPort;
	}

	public String getPeerType() {
		return peerType;
	}

	public String getPeerAdr() {
		return peerAdr;
	}

	public String getRates() {
		return rates;
	}
}
