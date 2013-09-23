package forest.control.console.model;

import java.util.ArrayList;
import javax.swing.table.AbstractTableModel;

public class RouteTableModel extends AbstractTableModel {
	private static final long serialVersionUID = 1609850163080109126L;
	private String[] columnNames = {"Comtree", "Addr", "Link"};
    private int[] widths = {2, 6, 2};
    private ArrayList<Route> data;
    public RouteTableModel(){ data = new ArrayList<Route>(); }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        Route rt = data.get(row);
        if(rt == null) return null;
        switch (col){
            case 0: return rt.getComtree(); 
            case 1: return rt.getAddr();
            case 2: return rt.getLink();
            default: return null;
        }
    }
    public void addRouteTable(Route rt) { data.add(rt); }
    public void clear(){ data.clear(); }
    public int getWidth(int i) { return widths[i];}
}
