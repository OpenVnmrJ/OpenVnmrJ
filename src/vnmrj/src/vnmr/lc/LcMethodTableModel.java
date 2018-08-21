/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.LinkedList;
import java.util.List;

import javax.swing.event.TableModelListener;
import javax.swing.table.TableModel;

import vnmr.util.Messages;
import vnmr.util.Util;


public class LcMethodTableModel implements TableModel {

    private static final Class<?> sm_objectClass = new Object().getClass();
    private static final Class<?> sm_integerClass = new Integer(0).getClass();
    private static String sm_nameOfIndexColumn = "lctlLineNumber";
    private static String sm_labelOfIndexColumn
        = Util.getLabel(sm_nameOfIndexColumn);

    /**
     * The columns currently in the table, ordered as they appear
     * in the table.
     */
    private List<String> m_columnNameList;

    /** The LcMethod displayed in the table. */
    private LcMethod m_method;

    /** List of action listeners. */
    private List<ActionListener> m_actionListenerList
        = new LinkedList<ActionListener>();


    public LcMethodTableModel(LcMethod method) {
        m_method = method;

        // Initialize list of columns
        m_columnNameList = m_method.getTabledParameterNames();
        // Line # is always first column:
        m_columnNameList.add(0, sm_labelOfIndexColumn);
    }

    /**
     * Add a column to the table.
     * The position is determined automatically, ultimately based on
     * the file "lcMethodVariables", containing an ordered list of
     * parameters.
     * It will never go in the first column (index position 0), as that
     * is reserved for the row number.
     * @param param The parameter displayed in the column.
     * @return The index position of the new column.
     */
    public int addColumn(LcMethodParameter param) {
        int colIdx = -1;
        // Put it in the appropriate place
        int myPosition = param.getColumnPosition();
        int tableWidth = m_columnNameList.size();
        // NB: Never goes in the first column (i = 0)
        for (int i = 1; i < tableWidth; i++) {
            String ithName = m_columnNameList.get(i);
            LcMethodParameter ithParam = m_method.getParameter(ithName);
            if (ithParam != null) {
                int ithPosition = ithParam.getColumnPosition();
                if (ithPosition >= myPosition) { // If ==, column is already in
                    colIdx = i;
                    if (ithPosition > myPosition) {
                        m_columnNameList.add(i, param.getName());
                        param.setTabled(true);
                    }
                    break;
                }
            }
        }
        // Add it at the end if no position found
        if (colIdx < 0) {
            colIdx = tableWidth;
            m_columnNameList.add(colIdx, param.getName());
        }
        return colIdx;
    }

    /**
     * Remove a column from the table.
     * @param param The parameter displayed in the column.
     */
    public void deleteColumn(LcMethodParameter param) {
        String parname = param.getName();
        int tableWidth = m_columnNameList.size();
        // NB: Can't delete the first two columns (i <= 1)
        for (int i = 2; i < tableWidth; i++) {
            String ithName = m_columnNameList.get(i);
            if (ithName.equals(parname)) {
                m_columnNameList.remove(i);
                param.setTabled(false);
                return;
            }
        }
    }

    /**
     * Delete the data in the specified rows.
     * The first column (containing the row index) remains untouched.
     * @param rows Array of row indices to delete (from 0) in ascending order.
     */
    public void deleteRows(int[] rows) {
        int tableWidth = m_columnNameList.size();
        for (int i = 1; i < tableWidth; i++) {
            LcMethodParameter param = getParameterByIndex(i);
            if (param != null) {
                param.deleteElements(rows);
            }
        }
        // Always leave at least one row
        int ndel = Math.min(rows.length, getRowCount() - 1);
        int nrows = getRowCount() - ndel;
        m_method.setRowCount(nrows);
        m_method.getTimeParameter().setValue(0, 0); // First time is always 0
        if (ndel == 1) {
            notifyActionListeners(null, "Row deleted from table");
        } else {
            notifyActionListeners(null, ndel + " rows deleted from table");
        }
    }

    /**
     * Insert null data in the specified index position.
     * The previous data is pushed down a row.
     * Cannot be inserted at the first row (row=0).
     * @param row The row to insert; must be >0.
     */
    public void addRow(int row) {
        if (row > 0 && row <= getRowCount()) {
            int tableWidth = m_columnNameList.size();
            for (int i = 1; i < tableWidth; i++) {
                LcMethodParameter param = getParameterByIndex(i);
                if (param != null) {
                    param.insertValue(row, null);
                }
            }
            m_method.setRowCount(getRowCount() + 1);
            notifyActionListeners(null, "Row added to table");
        }
    }

    /**
     * Gets the parameter name for the columnIndex'th column of the table.
     */
    public String getColumnParameterName(int columnIndex) {
        if (columnIndex < m_columnNameList.size()) {
            return m_columnNameList.get(columnIndex);
        } else {
            return "?";
        }
    }

    /**
     * Gets the parameter in the columnIndex'th column of the table.
     * Returns null in columnIndex is 0 (the first column).
     */
    private LcMethodParameter getParameterByIndex(int columnIndex) {
        String name = getColumnParameterName(columnIndex);
        return m_method.getParameter(name);
    }        

    /**
     * Adds a listener to the list that is notified each time a change
     * to the data model occurs.
     */
    public void addTableModelListener(TableModelListener l) {
    }

    /**
     * Returns the most specific superclass for all the
     * cell values in the column.
     */
    public Class<?> getColumnClass(int columnIndex) {
        if (columnIndex == 0) {
            return sm_integerClass;
        } else {
            LcMethodParameter par = getParameterByIndex(columnIndex);
            return par == null
                    ? sm_objectClass
                    : par.getParameterClass();
        }
    }

    /**
     * Returns the number of columns in the model.
     */
    public int getColumnCount() {
        return m_columnNameList.size();
    }

    /**
     * Returns the (localized) label for the column at columnIndex.
     * @param columnIndex The column index (from 0).
     * @return The label of the column.
     */
    public String getColumnName(int columnIndex) {
        if (columnIndex == 0) {
            return sm_labelOfIndexColumn;
        } else {
            LcMethodParameter par = getParameterByIndex(columnIndex);
            if (par != null) {
                return par.getLabel();
            }
        }
        return "?";
    }

    /**
     * Returns the number of rows in the model.
     * @return The number of rows.
     */
    public int getRowCount() {
        return m_method.getRowCount();
    }

    /**
     * Returns the value for a given cell.
     * @param rowIndex The row index (from 0).
     * @param columnIndex The column index (from 0).
     * @return An Object representing the value.
     */
    public Object getValueAt(int rowIndex, int columnIndex) {
        if (columnIndex == 0) {
            return rowIndex;
        } else {
            LcMethodParameter par = getParameterByIndex(columnIndex);
            if (par != null) {
                return par.getValue(rowIndex);
            } else {
                return null;
            }
        }
    }

    /**
     * Returns true if the cell at rowIndex and columnIndex is editable.
     * The first column and the first row of the second column are not editable.
     * @param rowIndex The row index (from 0).
     * @param columnIndex The column index (from 0).
     */
    public boolean isCellEditable(int rowIndex, int columnIndex) {
        return !(columnIndex == 0 || (rowIndex == 0 && columnIndex == 1));
    }

    /**
     * Removes a listener from the list that is notified each time
     * a change to the data model occurs.
     */
    public void removeTableModelListener(TableModelListener l) {
    }

    /**
     * Sets the value in a given cell to a given value.
     * The value is stored in the LcMethod attached to this model.
     * @param aValue An Object expressing the new value.
     * @param rowIndex The row index of the cell (from 0).
     * @param columnIndex The column index of the cell (from 0).
     */
    public void setValueAt(Object aValue, int rowIndex, int columnIndex) {
        Messages.postDebug("LcMethodTableModel",
                           "LcMethodTableModel.setValueAt(value=" + aValue
                           + ", row=" + rowIndex
                           + ", col=" + columnIndex + ")");
        if (columnIndex > 0) {
            LcMethodParameter par = getParameterByIndex(columnIndex);
            if (par.setValue(rowIndex, aValue)) {
                notifyActionListeners(par.getName(),
                                      "Parameter modified in table");
            }
        }
    }

    /**
     * Notify all action listeners of a state change.
     * The ActionCommand that is sent is 'TableChanged "<parname>" <msg>'.
     * <p>
     * NB: No notification is sent for adding or deleting columns,
     * that is currently dealt with in LcMethodEditor.
     * @param parlabel The the (localized) name of the changed parameter.
     * Should be null if the change does not pertain to a particular column.
     * @param msg A user readable message specifying the change.
     */
    public void notifyActionListeners(String parname, String msg) {
        String command = "TableChanged " + parname + " " + msg;
        ActionEvent actionEvent = new ActionEvent(this, hashCode(), command);
        for (ActionListener listener : m_actionListenerList) {
            listener.actionPerformed(actionEvent);
        }
    }

    /**
     * Add an action listener to be notified of table state changes.
     */
    public void addActionListener(ActionListener listener) {
        m_actionListenerList.add(listener);
    }

    /**
     * Remove an action listener from the list of those to be notified.
     */
    public void removeActionListener(ActionListener listener) {
        m_actionListenerList.remove(listener);
    }

}
