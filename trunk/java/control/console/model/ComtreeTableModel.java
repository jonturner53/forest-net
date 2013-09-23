package forest.control.console.model;

import java.util.ArrayList;
import javax.swing.table.AbstractTableModel;

public class ComtreeTableModel extends AbstractTableModel {

	private static final long serialVersionUID = 8309007097508242075L;
	private String[] columnNames = {"comtree", "inCore", "pLink", "comtLinks", "coreLinks"};
    private int[] widths = {2, 1, 1, 3, 3};
    private ArrayList<Comtree> data;
    public ComtreeTableModel(){ data = new ArrayList<Comtree>(); }
    public int getColumnCount() { return columnNames.length; }
    public int getRowCount() { return data.size(); }
    public String getColumnName(int col) { return columnNames[col];}
    public Object getValueAt(int row, int col) { 
        Comtree ct = data.get(row);
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
    public void addComtreeTable(Comtree ct) { data.add(ct); }
    public void clear(){ data.clear(); }
    public int getWidth(int i) { return widths[i];}
}
