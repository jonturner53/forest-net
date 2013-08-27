package forest.control.console.model;

import java.util.ArrayList;
import javax.swing.table.AbstractTableModel;

public class ComtreeTableModel extends AbstractTableModel {
	private String[] columnNames = {"comtree", "inCore", "pLink", "comtLinks", "coreLinks"};
    private int[] widths = {1, 1, 3, 1};
    private ArrayList<ComtreeTable> data;
    public ComtreeTableModel(){ data = new ArrayList<ComtreeTable>(); }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        ComtreeTable ct = data.get(row);
        if(ct == null) return null;
        switch (col){
            case 0: return ct.getComtree(); 
            case 1: return ct.getInCore();
            case 2: return ct.getpLink();
            case 3: return ct.getComtLinks();
            case 4: return ct.getCoreLink();
            default: return null;
        }
    }
    public void addComtreeTable(ComtreeTable ct) { data.add(ct); }
    public void clear(){ data.clear(); }
    public int getWidth(int i) { return widths[i];}
}
