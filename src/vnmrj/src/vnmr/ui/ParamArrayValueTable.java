/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

import vnmr.util.*;


/********************************************************** <pre>
 * Summary:
 *
 * 
 </pre> **********************************************************/


public class ParamArrayValueTable extends JTable {
    private static Vector paramValues;
    int editRow=-1;
    int editCol=-1;

    public ParamArrayValueTable(Vector values, Vector header) {
	super(values, header);

        JTableHeader tableHeader = getTableHeader();

        // set cell renderers
        TableColumnModel colModel = tableHeader.getColumnModel();
        int numCols = colModel.getColumnCount();
        for (int i = 0; i < numCols; i++) {
            TableColumn col = colModel.getColumn(i);
            col.setCellRenderer(new MyTableCellRenderer());
            col.setHeaderRenderer(new MyTableHeaderRenderer());
        }

        DefaultCellEditor editor = (DefaultCellEditor)getDefaultEditor(TableColumn.class);
        if(editor != null) {

            ((JTextField)editor.getComponent()).addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent evt) {

                   TableCellEditor celleditor = getCellEditor();

                        if(celleditor != null) {

                            celleditor.stopCellEditing();
                            removeEditor();   //this method calls requestFocus().
                        }
                }
            });
	}

	// Save this for future use.  It is kept up to date by
	// super.setValueAt()
	paramValues = values;

	setSelectionBackground(Global.HIGHLIGHTCOLOR);
	setSelectionForeground(Color.black);
	setShowGrid(false);
	setColumnSelectionAllowed(false);
	setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

	addMouseListener(new MouseAdapter() {
	    public void mouseClicked(MouseEvent evt) {
		Point p = new Point(evt.getX(), evt.getY());
		int colIndex = columnAtPoint(p);

		// If value column
		if (colIndex == 1) {
			// If I let the normal events occur, when the user
			// clicks once, that cell is not highlighted
			// along with the rest of the row.  Keeping
			// the cell uneditable until it is double clicked,
			// keeps the highlight correct until double clicking.
			editRow = rowAtPoint(p);
			editCol = columnAtPoint(p);
			// Yes, I know, I am calling editCellAt() twice.
			// If I call it once, then the very first time I double
			// click something, it does not become editable.
			editCellAt(editRow, editCol);

		} else if(colIndex == 0) {

		    int clicks = evt.getClickCount();
		    if(clicks == 2) {
		    	String value = (String)getValueAt(rowAtPoint(p),1); 
		    	ParamArrayPanel pap;
                    	pap = ParamArrayPanel.getParamArrayPanel();
                    	pap.setCurrentValue(value);
		    }
		}
	    }
	});
    }

    /**************************************************
     * Summary: Defines which table cell is to be editable.
     *
     **************************************************/
    public boolean isCellEditable(int row, int column) {
	if(column == 0)
	    return false;
	else if(editRow == row)
	    return true;
	else
	    return false;
    }

    /**************************************************
     * Summary: Called to change the number of rows in this table.
     *
     **************************************************/
    public void setNumRows(int numRows) {
	// If no change, just return;
	if(numRows == paramValues.size())
	    return;
	// Make new Rows
	else if(numRows > paramValues.size()) {
	    for(int i=paramValues.size(); i < numRows; i++) {
		Vector row = new Vector(2);
		row.add(String.valueOf(i+1));
		row.add("");
		paramValues.add(row);
	    }
	}
	// Remove the end rows
	else {
	    for(int i=paramValues.size()-1; i > numRows-1; i--)
		paramValues.removeElementAt(i);
	}
	resizeAndRepaint();

    }

    /**************************************************
     * Summary: Called when editing is complete to set a new value.
     *
     **************************************************/
    public void setValueAt(Object value, int row, int col) {

	if(value == null) return;
	String str;

	str = (String) value;

	// Has this value changed?
	if(!str.equals(((Vector)paramValues.elementAt(row)).elementAt(col))){
	    super.setValueAt(value, row, col);

	    // ***Put code here to do something with changed values***
	    // After calling super.setValueAt(), the Vector paramValues
	    // will be up to date for all values.
	}
    }


    public void removeOneRow(int row) {
        // Trap for out of bounds
        if(row >= 0 && row < paramValues.size()) {
            paramValues.remove(row);
            resizeAndRepaint();
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
                setForeground(table.getForeground());
                if(isSelected) setBackground(Global.HIGHLIGHTCOLOR);
                else setBackground(table.getBackground());
                setFont(table.getFont());
        }

        if (hasFocus) {
            setBorder( UIManager.getBorder("Table.focusCellHighlightBorder") );
            if (table.isCellEditable(row, column)) {
                super.setForeground( UIManager.getColor("Table.focusCellForeground") );
                super.setBackground( table.getBackground() );
            }
        } else {
            setBorder(noFocusBorder);
        }

        setValue(value);
        return this;
    } // getTableCellRendererComponent()

 } // class MyTableCellRenderer

 class MyTableHeaderRenderer extends DefaultTableCellRenderer {

    /**
     * constructor
     */
    public MyTableHeaderRenderer() {
        super();
    } // MyTableHeaderRenderer()

    public Component getTableCellRendererComponent(JTable table, Object value,
                          boolean isSelected, boolean hasFocus, int row, int column) {
        if (table != null) {
            JTableHeader header = table.getTableHeader();
            if (header != null) {
                setForeground(header.getForeground());
                setBackground(header.getBackground());
                setFont(header.getFont());
            }
        }

        setValue(value);
        setBorder(BorderFactory.createMatteBorder(0, 0, 1, 1, Color.black));
        setHorizontalAlignment(JTextField.CENTER);
        return this;
    } // getTableCellRendererComponent()

 } // class MyTableHeaderRenderer

}
