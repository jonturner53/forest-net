package remoteDisplay.TopologyGen;

/** @file MyTableModel.java */

import javax.swing.table.*;

/**
* My build of a defaultTableModel for use with a JTable
*/
class MyTableModel extends DefaultTableModel{
	Object[] columnNames;
	Object[][] data;
	public MyTableModel(Object[][] body, Object[] columns){
		super(body, columns);
		columnNames=columns;
		data = body;
	}
}

