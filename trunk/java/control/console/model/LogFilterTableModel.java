package forest.control.console.model;

import java.util.ArrayList;

import javax.swing.table.AbstractTableModel;

public class LogFilterTableModel extends AbstractTableModel {

	private static final long serialVersionUID = -8601706496073175363L;
	private String[] columnNames = { "id", "rtn", "in", "out", "link",
			"comtree", "type", "cpType" };
	private int[] widths = { 1, 1, 1, 1, 1, 1, 2, 2 };
	private ArrayList<LogFilter> data;

	public LogFilterTableModel() {
		data = new ArrayList<LogFilter>();
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
		LogFilter lf = data.get(row);
		if (lf == null)
			return null;
		switch (col) {
		case 0:
			return lf.getId();
		case 1:
			return lf.getRtn();
		case 2:
			return lf.getIn();
		case 3:
			return lf.getOut();
		case 4:
			return lf.getLink();
		case 5:
			return lf.getComtree();
		case 6:
			return lf.getType();
		case 7:
			return lf.getCpType();
		default:
			return null;
		}
	}

	public void addLogFilterTable(LogFilter ct) {
		data.add(ct);
	}

	public void clear() {
		data.clear();
	}

	public ArrayList<LogFilter> getLogFilterList() {
		return data;
	}

	public int getWidth(int i) {
		return widths[i];
	}

	public void removeLogFilterTable(int i) {
		data.remove(i);
	}

}
