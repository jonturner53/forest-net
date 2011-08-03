import javax.swing.table.*;

class MyTableModel extends DefaultTableModel{
	Object[] columnNames;
	Object[][] data;
	public MyTableModel(Object[][] body, Object[] columns){
		super(body, columns);
		columnNames=columns;
		data = body;
	}
}

