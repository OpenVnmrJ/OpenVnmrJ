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


public class ParamArrayTable extends JTable implements VTooltipIF {
    protected static Vector arrayedParams;
    private static String curParam;
    private static int    curParamRow=-1;
    public static final int PARAMCOL=0;
    public static final int DESCCOL=1;
    public static final int SIZECOL=2;
    public static final int ORDERCOL=3;
    public static final int ONOFFCOL=4;

    public ParamArrayTable(Vector values, Vector header) {
	super(values, header);

	JTableHeader tableHeader = getTableHeader();

	// set cell renderers
        TableColumnModel colModel = tableHeader.getColumnModel();
        int numCols = colModel.getColumnCount();
        for (int i = 0; i < numCols; i++) {
            TableColumn col = colModel.getColumn(i);
            col.setCellRenderer(new MyTableCellRenderer());
            col.setHeaderRenderer(new MyTableHeaderRenderer());
	    if(i == 1) col.setPreferredWidth(260);
	    else col.setPreferredWidth(80);
        }

	DefaultCellEditor editor = (DefaultCellEditor)getDefaultEditor(TableColumn.class);
	if(editor != null) {

        // when editing a table cell, stopCellEditing and removeEditor are 
	// invoked when clicking on other cell or hitting return key.
        // the change is saved, editor object is discarded, and the cell
        // is rendered once again. Without clicking other cell or hitting
        // return, the last value is not saved.

	// the whole purpose here is to capture the last change in the table
	// without hitting return. To do that, I need to know who got the focus
	// while the cell is being editing, then have that object (intuitively 
	// it should be the cell editor) listen to focus and call stopCellEditing 
	// and removeEditor methods to save the last change when it lost the focus.

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

	setSelectionBackground(Global.HIGHLIGHTCOLOR);
	setSelectionForeground(Color.black);
	setShowGrid(false);
	setColumnSelectionAllowed(false);
	setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

	// Force the order of rows by assending 'order'
	sortByOrder(values);

	resizeAndRepaint();

	// Save this for future use.  It is kept up to date by
	// super.setValueAt()
	arrayedParams = values;

	if (Util.isNativeGraphics()) {
            ToolTipManager.sharedInstance().unregisterComponent(this);
            VTipManager.sharedInstance().registerComponent(this);
        }

	addMouseListener(new MouseAdapter() {
	    public void mouseClicked(MouseEvent evt) {
		Point p = new Point(evt.getX(), evt.getY());
		int colIndex = columnAtPoint(p);
		int rowIndex = rowAtPoint(p);

		ParamArrayPanel pap;
		pap = ParamArrayPanel.getParamArrayPanel();
		if(pap != null && pap.paramScrollpane != null) {
                    Dimension dim = pap.paramScrollpane.getSize();
                    pap.paramScrollpane.setPreferredSize(dim);
		}

		if(rowIndex != -1 && colIndex != -1) {

		    // Single Click, Update values table for this param
		    // No matter which column they click in, get param
		    // name from column PARAMCOL.
	
		    curParamRow = rowIndex;
		    setRowSelectionInterval(curParamRow, curParamRow);

		    if(isCellEditable(rowIndex, colIndex)) {
		    	editCellAt(rowIndex, colIndex);
		    }

		    String param = (String)getValueAt(rowAtPoint(p), PARAMCOL);

		    // Is this a new param name?
		    if(param.length() > 0) {
		    	// Save the new param
		    	curParam = param;

		    	// Request that the values table be updated.

		    	//pap.requestUpdateValueTable(param);
		    	if(pap != null) pap.updateCenterPane(param);
		    }

	            int clicks = evt.getClickCount();
	            if(clicks == 2 && colIndex == ONOFFCOL) {

 		      String val = (String)getValueAt(rowIndex, colIndex);
 		      if(val.equals("On"))
 			setValueAt("Off", rowIndex, colIndex);
 		      else
 			setValueAt("On", rowIndex, colIndex);

 		      if(curParamRow == -1)
 			curParamRow = rowIndex;
	            } 
	 	}
	    }

	    public void mousePressed(MouseEvent evt) {
		// Find out where the mouse is right now.
		Point p = new Point(evt.getX(), evt.getY());
		int row = rowAtPoint(p);
		int col = columnAtPoint(p);

		// Do not allow clicking in col ONOFFCOL to change the
		// selection.
		if(row != -1) curParamRow = row;
		if(curParamRow > getRowCount()) curParamRow = 0;
		if(curParamRow >= 0) {
		    setRowSelectionInterval(curParamRow, curParamRow);
		}

	    }

	});
    }

    public void setTooltip(String str) { }

    public String getTooltip(MouseEvent evt) {
        Point p = new Point(evt.getX(), evt.getY());
        int colIndex = columnAtPoint(p);
        int rowIndex = rowAtPoint(p);

        if(colIndex < 0 || colIndex >= getColumnCount()) return("");
        if(rowIndex < 0 || rowIndex >= getRowCount()) return("");

        return (String)getValueAt(rowIndex, colIndex);
    }

    /**************************************************
     * Summary: Defines which table cell is to be editable.
     *
     **************************************************/
    public boolean isCellEditable(int row, int column) {
	if(column == SIZECOL || column == ONOFFCOL || column == DESCCOL)
	    return false;
/*
        else if(column == ORDERCOL &&
            ((String)getValueAt(row, column)).equals("0")) 
            return false;
*/
	else
	    return true;
    }

    public boolean IsImplicitArray(String paramName) {
        if(paramName.equals("ni") ||
           paramName.equals("ni2") ||
           paramName.equals("ni3")) return true;

        return false;
    }

    /**************************************************
     * Summary: Called when editing is complete to set a new value.
     *
     **************************************************/
    public void setValueAt(Object value, int row, int col) {

	if(value == null) return;

	//Vector rowVector = new Vector();
	String str = (String) value;
	if(str.length() == 0) return;

	String name = (String)(((Vector)arrayedParams.elementAt(row)).elementAt(PARAMCOL));

	// Has this value changed?
	if(!str.equals(((Vector)arrayedParams.elementAt(row)).elementAt(col))){
	    if(col != ORDERCOL || (IsImplicitArray(name) && str.equals("0"))) 
		super.setValueAt(value, row, col);
	    else if(!str.equals("0") && isOrderValid(arrayedParams,value,row)) 
		super.setValueAt(value, row, col);
	    else if(!str.equals("0") && isSwabValid(arrayedParams,value,row)) { 
		swabOrder(arrayedParams,value,row);
		super.setValueAt(value, row, col);
	    }

	    if(col == PARAMCOL) curParam = str;

	    // After calling super.setValueAt(), the Vector arrayedParams
	    // will be up to date for all values.

	}
    }

    /**************************************************
     * Summary: Remove the specified row from the table.
     *
     **************************************************/
    public void removeOneRow(int row) {
	// Trap for out of bounds
	if(row >= 0 && row < arrayedParams.size()) {
	    arrayedParams.remove(row);
	    resizeAndRepaint();
	}
    }

    /**************************************************
     * Summary: Add a blank row to the table.
     *
     **************************************************/
    public void addBlankRow() {

	Vector newRow = new Vector(5);
	newRow.add("");
	newRow.add("");
	newRow.add("");
	newRow.add("");
	newRow.add("On");
	
	arrayedParams.add(0, newRow);
	// Select the new row.
	setRowSelectionInterval(0,0);
	resizeAndRepaint();
    }


    /**************************************************
     * Summary: Sort the rows by the 'order' entry.
     *
     *	Since values is a Vector of Vectors, I have to do this
     *  myself instead of using some existing sort routine.
     **************************************************/
    public static void sortByOrder(Vector values) {

	for (int i=0; i < values.size(); i++) {
	    for (int j=i; j > 0 && 
		     getOrder(values, j-1) > getOrder(values, j); j--) {

		// Swap rows j and j-1
		Vector a = (Vector)values.elementAt(j);
		values.remove(j);
		values.add(j, values.elementAt(j-1));
		values.remove(j-1);
		values.add(j-1, a);
	    }
	}

    }

    /**************************************************
     * Summary: Get the order of the given row.
     *
     *
     **************************************************/
    public static int getOrder(Vector values, int row) {
	Vector rowVector = (Vector)values.elementAt(row);
	String orderString = (String)rowVector.elementAt(ORDERCOL);
	int order = values.size();
	if(ParamArrayPanel.isDigits(orderString)) 
	order = Integer.valueOf(orderString).intValue();
	return order;
    }

    public void resizeAndRepaint() {
	super.resizeAndRepaint();

    }

    private boolean isOrderValid(Vector values, Object newOrder, int row) {

	//test the newOrder for a given row. 
	//the newOrder is not applied to values yet.
	//valid only if all arrayed parameters of the same order
	//has the same size.
	
	String onoff = (String)(((Vector) values.elementAt(row)).elementAt(ONOFFCOL));
	if(onoff.equals("Off")) return false;

	ParamArrayPanel pap = ParamArrayPanel.getParamArrayPanel();

	int thisSize = pap.getActualArraySize(row);

	for(int i=0; i<values.size(); i++) {
	    int size = pap.getActualArraySize(i);
	    Object order = ((Vector) values.elementAt(i)).elementAt(ORDERCOL);
	    onoff = (String)(((Vector) values.elementAt(i)).elementAt(ONOFFCOL));
	    if(onoff.equals("On") && 
		order.equals(newOrder) && size != thisSize) return false;
	} 
	return true;
    }

    private boolean isSwabValid(Vector values, Object newOrder, int row) {

	//make a copy of values and call swabOrder method, i.e.,
	//set the given row to newOrder, swab all parameters that has this
	//order but has different size to the order the given row had (prevOrder).

	//test the swabbed Vector.
	//valid only if after swabbing all arrayed parameters of the same order
	//has the same size .

	String onoff = (String)(((Vector) values.elementAt(row)).elementAt(ONOFFCOL));
	if(onoff.equals("Off")) return false;

	Object prevOrder = ((Vector) values.elementAt(row)).elementAt(ORDERCOL);
	if(((String) prevOrder).length() == 0) return false; 

	Vector swabbed = values;

	//the swabbed rows indices will be returned as a Vector.
	Vector swabbedRows = swabOrder(swabbed, newOrder, row);

	//empty means either swab is not needed (true for isOrderValid),
	//or swab is not valid.
	if(swabbedRows == null || swabbedRows.size() == 0) return false; // nothing is swabbed.

	int newRow = ((Integer) swabbedRows.elementAt(0)).intValue();
	return isOrderValid(swabbed, prevOrder, newRow);

    }

    private Vector swabOrder(Vector values, Object newOrder, int row) {

	//save prevOrder and set newOrder for the row.

	Object prevOrder = ((Vector) values.elementAt(row)).elementAt(ORDERCOL);
	if(((String) prevOrder).length() == 0) return null; 
        if(prevOrder.equals("0")) {
	  Integer ord = new Integer(values.size());
	  prevOrder = ord.toString();
	}
	
	Vector v = (Vector) values.elementAt(row);
	v.setElementAt(newOrder, ORDERCOL);
	values.setElementAt(v, row);

	ParamArrayPanel pap = ParamArrayPanel.getParamArrayPanel();

	int thisSize = pap.getActualArraySize(row);

	Vector swabbedRows = new Vector();
	for(int i=0; i<values.size(); i++) {
	    int size = pap.getActualArraySize(i);
	    Object order = ((Vector) values.elementAt(i)).elementAt(ORDERCOL);
	    String onoff = (String)(((Vector) values.elementAt(i)).elementAt(ONOFFCOL));
	    if(onoff.equals("On") && order.equals(newOrder) && size != thisSize) {
		swabbedRows.add(new Integer(i));
		Vector r = (Vector) values.elementAt(i);
		r.setElementAt(prevOrder, ORDERCOL);
		values.setElementAt(r, i);
	    }
	}
	    
	return swabbedRows;
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
	setHorizontalAlignment(JTextField.CENTER);
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
