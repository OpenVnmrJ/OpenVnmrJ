/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.part11;

import java.io.*;
import java.awt.*;
import java.util.*;
import javax.swing.*;
import java.awt.event.*;
import javax.swing.table.*;

import vnmr.bo.*;
import vnmr.util.*;

public class FileTable extends JTable implements VTooltipIF
{
    private FileTableModel m_tableModel;
    private String m_type = "";
    private Vector m_flags = null;

    public FileTable() {
    super();
        setOpaque(false);
    m_tableModel = new FileTableModel();
    }

    public FileTable(String path, String type) {
    super();
        setOpaque(false);

    Vector paths = new Vector();
    paths.addElement(path);
    makeFileTable(paths, type);
    }

    public FileTable(Vector paths, String type) {
    super();
        setOpaque(false);

    makeFileTable(paths, type);
    }

    public boolean makeFileTable(Vector paths, String type) {

    m_type = type;

    int selectedRow = getSelectedRow();
    if(selectedRow < 0) selectedRow = 0;

        if (m_tableModel == null)
    {
        m_tableModel = new FileTableModel(paths, type);
    }
    else
        m_tableModel.updateModel(paths, type);
        TableSorter sorter = new TableSorter(m_tableModel);
        setModel(sorter);
        sorter.addMouseListenerToHeaderInTable(this);

    m_flags = m_tableModel.getFlags();
	updateFlags();

        JTableHeader tableHeader = getTableHeader();

        TableColumnModel colModel = tableHeader.getColumnModel();
        int numCols = colModel.getColumnCount();
        for (int i = 0; i < numCols; i++) {
            TableColumn col = colModel.getColumn(i);
            col.setCellRenderer(new MyTableCellRenderer());
        }

        //setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        setShowGrid(false);
        setColumnSelectionAllowed(false);

    setTableColumnWidths();
	if(getRowCount() > selectedRow)
	setRowSelectionInterval(selectedRow,selectedRow);

        boolean b = true;

        if(m_tableModel.getRowCount() <= 0) {
            b = false;
        }

    if (Util.isNativeGraphics()) {
            ToolTipManager.sharedInstance().unregisterComponent(this);
            VTipManager.sharedInstance().registerComponent(this);
        }

        return(b);
    }

    public boolean updateFileTable(Vector paths, String type) {

    m_type = type;

    int selectedRow = getSelectedRow();
    if(selectedRow < 0) selectedRow = 0;

    if (m_tableModel == null)
    {
        m_tableModel = new FileTableModel(paths, type);
    }
    else
        m_tableModel.updateModel(paths, type);

        TableSorter sorter = new TableSorter(m_tableModel);
        setModel(sorter);
        sorter.addMouseListenerToHeaderInTable(this);

    m_flags = m_tableModel.getFlags();
	updateFlags();

        JTableHeader tableHeader = getTableHeader();

        TableColumnModel colModel = tableHeader.getColumnModel();
        int numCols = colModel.getColumnCount();
        for (int i = 0; i < numCols; i++) {
            TableColumn col = colModel.getColumn(i);
            col.setCellRenderer(new MyTableCellRenderer());
        }

        //setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        setShowGrid(false);
        setColumnSelectionAllowed(false);

    setTableColumnWidths();
	if(getRowCount() > selectedRow)
	setRowSelectionInterval(selectedRow,selectedRow);

        boolean b = true;

        if(m_tableModel.getRowCount() <= 0) {
            b = false;
        }

    if (Util.isNativeGraphics()) {
            ToolTipManager.sharedInstance().unregisterComponent(this);
            VTipManager.sharedInstance().registerComponent(this);
        }

        return(b);
    }

    private String getHostName(int row) {

	String console = "";
	int consoleCol = -1;
	for(int i=0; i<getColumnCount(); i++)
        if(getColumnName(i).equals("Console")) consoleCol = i;
	
	if(consoleCol != -1) {
            console = (String)getValueAt(row, consoleCol);

	    if(console.equals("?") || console.equals("-") || 
		console.equals("null") || console.equals("none"))
	    console = "";

	    if(console.length() > 0 && console.indexOf(":") != -1) {
	        StringTokenizer tok = new StringTokenizer(console, ":", false);
		if(tok.hasMoreTokens()) console = tok.nextToken();
		else console = "";
	    }
	}

	if(console.length() == 0) console = Util.getHostName();

	return console;
    }

    public void setTooltip(String str) { }

    public String getTooltip(MouseEvent evt) {
        Point p = new Point(evt.getX(), evt.getY());
        int colIndex = columnAtPoint(p);
        int rowIndex = rowAtPoint(p);

        if(colIndex < 0 || colIndex >= getColumnCount()) return("");
        if(rowIndex < 0 || rowIndex >= getRowCount()) return("");

        String header = getColumnName(colIndex);
        if(header.equals("none")) return("");
        else if(header.equals("User")) {
        String uname = (String)getValueAt(rowIndex, colIndex);
	String host = getHostName(rowIndex);
        return host+":"+VPropertyBundles.getValue("userResources", uname);
        } else if(header.equals("Records")) {
        return getRowValue(rowIndex);
        } else return getValueAt(rowIndex, colIndex).toString();
    }

    public FileTableModel getFileTableModel() {
        return(this.m_tableModel);
    }

    public String getRowValue(int row) {
    if(row >= 0 && row < this.m_tableModel.getRowCount()) {
        int[] ind = ((TableSorter) getModel()).getIndexes();
        return((String)this.m_tableModel.getRowValue(ind[row]));
    } else return(null);
    }

    public void updateFlags() {

// when directly access m_flags, ind[i] is used.
// when access through isRowFlaged, i is used.
// when access rowValue through FileTable's getRowValue, used i;
// when access rowValue through FileTableModel's getRowValue, used ind[i].

	Hashtable table = Audit.flagtable;

	if(table == null || m_type == null || 
		!m_type.equals("records") || table.size() <= 0) return;

        int[] ind = ((TableSorter) getModel()).getIndexes();
	for(int i=0; i<getRowCount(); i++) {
	      String key = getRowValue(i) +"/";
	      String value = (String)table.get(key);
	      if(value != null && value.equals("1") && !isRowFlaged(i)) { 
		m_flags.remove(ind[i]);
		m_flags.add(ind[i], new Boolean(true));
	      }
        }

        return;
    }

    public boolean isRowFlaged(int row) {
    if(row < m_flags.size() && row >= 0) {
        int[] ind = ((TableSorter) getModel()).getIndexes();
        return(((Boolean)m_flags.elementAt(ind[row])).booleanValue());
    } else return(false);
    }

    public Vector getOutputTypes() {
        return(this.m_tableModel.getOutputTypes());
    }

    public boolean isCellEditable(int row, int column) {
            return false;
    }

    private void setTableColumnWidths() {

        TableColumn column = null;
        if(getColumnCount() == 4 && m_type != null && (m_type.equals("records") ||
        m_type.equals("s_auditTrailFiles"))) {
            column = getColumnModel().getColumn(0);
            column.setPreferredWidth(50);
            column = getColumnModel().getColumn(1);
            column.setPreferredWidth(50);
            column = getColumnModel().getColumn(2);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(3);
            column.setPreferredWidth(100);
        }
        else if(getColumnCount() == 3 && m_type != null && m_type.equals("cmdHistory")) {
            column = getColumnModel().getColumn(0);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(1);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(2);
            column.setPreferredWidth(100);
        }
        else if(getColumnCount() == 4 && m_type != null && m_type.equals("s_auditTrail")) {
            column = getColumnModel().getColumn(0);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(1);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(2);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(3);
            column.setPreferredWidth(100);
        }
        else if(getColumnCount() == 6 && m_type != null && m_type.equals("d_auditTrail")) {
            column = getColumnModel().getColumn(0);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(1);
            column.setPreferredWidth(50);
            column = getColumnModel().getColumn(2);
            column.setPreferredWidth(50);
            column = getColumnModel().getColumn(3);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(4);
            column.setPreferredWidth(100);
            column = getColumnModel().getColumn(5);
            column.setPreferredWidth(100);
        }
    else if(getColumnCount() == 1) {
            column = getColumnModel().getColumn(0);
            column.setPreferredWidth(400);
    }
    }

 class MyTableCellRenderer extends DefaultTableCellRenderer {

    /**
     * constructor
     */
    public MyTableCellRenderer() {
        super();
    } // MyTableCellRenderer()

    public Component getTableCellRendererComponent(JTable table, Object value,
                          boolean isSelected, boolean hasFocus, int row, int column) {

        if (table != null) {
                setBackground(table.getBackground());
        int[] ind = ((TableSorter) table.getModel()).getIndexes();
                if(((Boolean)m_flags.elementAt(ind[row])).booleanValue())
            setForeground(Color.red);
                else setForeground(table.getForeground());

                setFont(table.getFont());
        }

        setBorder(noFocusBorder);
        if (isSelected) {
            super.setBackground( Color.yellow );
        }

        setValue(value);
    setHorizontalAlignment(JTextField.CENTER);
        return this;
    } // getTableCellRendererComponent()

 } // class MyTableCellRenderer

}

