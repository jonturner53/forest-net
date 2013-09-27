package forest.control.console.model;

import java.util.ArrayList;
import javax.swing.table.AbstractTableModel;

public class IfaceTableModel extends AbstractTableModel {

	private static final long serialVersionUID = 5816215241745495815L;
	private String[] columnNames = { "iface", "ip", "rates" };
	private int[] widths = { 2, 2, 6 };
	private ArrayList<Iface> data;

	public IfaceTableModel() {
		data = new ArrayList<Iface>();
	}

	public int getColumnCount() {
		return columnNames.length;
	}

	public int getRowCount() {
		return data.size();
	}

	public String getColumnName(int col) {
		return columnNames[col];
	}

	public Object getValueAt(int row, int col) {
		Iface it = data.get(row);
		if (it == null)
			return null;
		switch (col) {
		case 0:
			return it.getIface();
		case 1:
			return it.getIp();
		case 2:
			return it.getRates();
		default:
			return null;
		}
	}

	public void addIfaceTable(Iface it) {
		data.add(it);
	}

	public void clear() {
		data.clear();
	}

	public int getWidth(int i) {
		return widths[i];
	}
}
