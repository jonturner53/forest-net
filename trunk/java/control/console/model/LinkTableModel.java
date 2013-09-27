package forest.control.console.model;

import java.util.ArrayList;

import javax.swing.table.AbstractTableModel;

public class LinkTableModel extends AbstractTableModel {

	private static final long serialVersionUID = 5896172706741952062L;
	private String[] columnNames = { "link", "iface", "peerIp:port",
			"peerType", "peerAdr", "rates" };
	private int[] widths = { 1, 1, 3, 1, 1, 3 };
	private ArrayList<Link> data;

	public LinkTableModel() {
		data = new ArrayList<Link>();
	}

	public int getColumnCount () {
		return columnNames.length;
	}

	public int getRowCount () {
		return data.size();
	}

	public String getColumnName (int col) {
		return columnNames[col];
	}

	public Object getValueAt (int row, int col) {
		Link lt = data.get(row);
		if (lt == null)
			return null;
		switch (col) {
		case 0:
			return lt.getLink();
		case 1:
			return lt.getIface();
		case 2:
			return lt.getPeerIpAndPort();
		case 3:
			return lt.getPeerType();
		case 4:
			return lt.getPeerAdr();
		case 5:
			return lt.getRates();
		default:
			return null;
		}
	}

	public void addLinkTable (Link lt) {
		data.add(lt);
	}

	public void clear () {
		data.clear();
	}

	public int getWidth (int i) {
		return widths[i];
	}
}
