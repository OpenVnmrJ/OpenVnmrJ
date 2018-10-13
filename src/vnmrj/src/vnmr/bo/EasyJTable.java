/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;


import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Rectangle;
import java.awt.datatransfer.Clipboard;
import java.awt.dnd.DnDConstants;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.util.Vector;

import javax.swing.JTable;
import javax.swing.JViewport;
import javax.swing.ListSelectionModel;
import javax.swing.SwingUtilities;
import javax.swing.TransferHandler;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ListSelectionEvent;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableCellRenderer;
import javax.swing.table.TableColumn;
import javax.swing.table.TableColumnModel;
import javax.swing.table.TableModel;

import vnmr.util.Messages;


/**
 * An extension of JTable intended to simplify managing the table.
 * EasyJTable has all the same constructors as JTable; in most cases
 * it can be substituted for JTable without other changes.
 * (But see "Limitations", below.)
 * <p><b>Scrolling:</b><br>
 * These methods assume that the Table is managed by a JScrollPane.
 * <br>
 * The {@link #scrollToRow(int rowIndex) scrollToRow} method can be
 * called to make a given row visible in the scrolling window.
 * <br>
 * The {@link #setScrollPolicy(int[]) setScrollPolicy} method sets the
 * scrolling behavior of the table in various circumstances.
 * This is where you can specify that, for example, when a row is
 * added to the bottom of the table, and the bottom of the table was
 * previously visible, scroll the table down to keep the bottom
 * in view.
 * <br>
 * The {@link #addRow(Object) addRow} methods append a row to the
 * bottom of the table and scroll the table if appropriate.
 * <br>
 * The {@link #positionScroll(int reason) positionScroll}
 * method is called to make the table scroll to the position dictated
 * by a previously set ScrollPolicy.
 * <p><b>Copy/Paste:</b><br>
 * EasyJTable adds to JTable's valueChanged method by copying all
 * the selected table cells to the "SystemSelection" clipboard.
 * <p><b>Limitations:</b><br>
 * To use addRow, the TableModel must be (or extend) DefaultTableModel.
 */
public class EasyJTable extends JTable {

    /** Show the last row of the table. */
    public static final int ACTION_SCROLL_TO_BOTTOM = 0x1;

    /** Show the first row of the table. */
    public static final int ACTION_SCROLL_TO_TOP = 0x2;

    /** Scroll so first visible row is same as before. */
    public static final int ACTION_KEEP_TOP_OF_VIEW = 0x4;

    /** Scroll so last visible row is same as before. */
    public static final int ACTION_KEEP_BOTTOM_OF_VIEW = 0x8;

    /** Don't change the scroll position. */
    public static final int ACTION_DONT_SCROLL = 0x10;

    /** The bit mask for the ACTION */
    private static final int ACTION_MASK = 0xff;


    /** The bit mask for the REASON */
    private static final int REASON_MASK = 0xff00;

    /** A row has been appended to the table. */
    public static final int REASON_ROW_APPENDED = 0x100;

    /** A row may have been resized. */
    public static final int REASON_ROWS_RESIZED = 0x200;

    /** The viewport has been resized. */
    public static final int REASON_VIEW_RESIZED = 0x400;

    /** The table is being displayed for the first time. */
    public static final int REASON_INITIAL_VIEW = 0x800;

    /** The table is being displayed for the first time. */
    public static final int REASON_ANY = REASON_MASK;


    /** The bit mask for the STATE */
    // private static final int STATE_MASK = 0xff0000;

    /** The bottom row was in view before. */
    public static final int STATE_BOTTOM_SHOWS = 0x100000;

    /** This matches any state. */
    public static final int STATE_ANY = 0x200000;

    private TableSizeListener m_tableSizeListener = new TableSizeListener();

    private int[] m_scrollPolicy = null;


    /**
     * Constructor provided for compatibility with JTable.
     */
    public EasyJTable() {
        init();
    }

    /**
     * Constructor provided for compatibility with JTable.
     */
    public EasyJTable(int numRows, int numColumns) {
        super(numRows, numColumns);
        init();
    }

    /**
     * Constructor provided for compatibility with JTable.
     */
    public EasyJTable(Object[][] rowData, Object[] columnNames) {
        super(rowData, columnNames);
        init();
    }

    /**
     * Constructor provided for compatibility with JTable.
     */
    public EasyJTable(TableModel dm) {
        super(dm);
        init();
    }

    /**
     * Constructor provided for compatibility with JTable.
     */
    public EasyJTable(TableModel dm, TableColumnModel cm) {
        super(dm, cm);
        init();
    }

    /**
     * Constructor provided for compatibility with JTable.
     */
    public EasyJTable(TableModel dm,
                      TableColumnModel cm, ListSelectionModel sm) {
        super(dm, cm, sm);
        init();
    }

    /**
     * Constructor provided for compatibility with JTable.
     */
    public EasyJTable(Vector rowData, Vector columnNames) {
        super(rowData, columnNames);
        init();
    }

    /**
     * Do default initializations of the table properties.
     */
    private void init() {
        // Fix for losing edits on losing focus: Java Bug #4709394
        putClientProperty("terminateEditOnFocusLost", Boolean.TRUE);
    }

    /**
     * Sets the policy for how to adjust the scroll in various situations.
     * Convenience method equivalent to
     * <pre>
     * int[] policy = new int[] {reason | state | action};
     * setScrollPolicy(policy);
     * </pre>
     * @see #setScrollPolicy(int[])
     */
    public boolean setScrollPolicy(int reason, int state, int action) {
        int[] policy = new int[] {reason | state | action};
        return setScrollPolicy(policy);
    }

    /**
     * Set the background color for the table header.
     * Convenience routine for <code>getTableHeader().setBackground(bg);</code>.
     */
    public void setHeaderBackground(Color bg) {
        getTableHeader().setBackground(bg);
    }

    /**
     * Sets the policy for how to adjust the scroll in various situations.
     * Setting "<tt>policy = null</tt>" means never automatically scroll.
     * The "policy" is an array of <i>n</i> reason/state/action codes.
     * <p>
     * Each code is a bit-mapped integer that is the
     * arithmetic OR of one or more "reasons", one or more "states",
     * and one "action".
     * The "reason" is the arithmetic OR of the reason(s) that the scroll may
     * need adjusting. Possible reasons are REASON_ROW_APPENDED,
     * REASON_ROWS_RESIZED, REASON_VIEW_RESIZED, REASON_INITIAL_VIEW,
     * and REASON_ANY.
     * The "state" represents the current state of the scrolling.
     * Possible states are STATE_BOTTOM_SHOWS and STATE_ANY.  The
     * STATE_ANY bit forces a match regardless of the current state.
     * If any of the "reasons" AND any of the "states" match the
     * current situation then the scrolling is adjusted according
     * to "action".
     * The possible actions are ACTION_SCROLL_TO_BOTTOM, ACTION_SCROLL_TO_TOP,
     * ACTION_KEEP_TOP_OF_VIEW (not implemented),
     * ACTION_KEEP_BOTTOM_OF_VIEW (not implemented).
     * If more than one code in the policy array matches the current
     * reason and state, the action for the first one that matches is
     * used.
     * <p>
     * Example:
     * <pre><tt>
     * policy = {REASON_INITIAL_VIEW | STATE_ANY | ACTION_SCROLL_TO_BOTTOM,
     *           REASON_ANY | STATE_BOTTOM_SHOWS | ACTION_SCROLL_TO_BOTTOM};
     * </pre></tt>
     * When the table is first displayed, it always scrolls to show the
     * last line.
     * For any other table change, it scrolls to the bottom only
     * if the bottom was showing before, otherwise the scroll doesn't change.
     * @param policy Array of reason/state/action codes.
     * @return Returns false if the policy array was invalid; the policy
     * is not changed in this case.
     */
    public boolean setScrollPolicy(int[] policy) {
        boolean ok = true;
        // Check that only one action is set per code.
        for (int i = 0; ok && policy != null && i < policy.length; i++) {
            int tstBits = policy[i] & ACTION_MASK;
            int nBitsSet = 0;
            while (tstBits != 0) {
                nBitsSet += tstBits & 1;
                tstBits >>>= 1;
            }
            if (nBitsSet != 1) {
                Messages.postError("EasyJTable.setScrollPolicy: "
                                   + "Bad action in policy[" + i
                                   + "]: action=" + policy[i]);
                ok = false;
            }
        }
        if (ok) {
            m_scrollPolicy = policy;
        }
        return ok;
    }

    /**
     * Gets the current scroll policy, in case you forgot what you set.
     * @see #setScrollPolicy(int[])
     * @return The current scroll policy.
     */
    public int[] getScrollPolicy() {
        return m_scrollPolicy;
    }

    /**
     * Convenience method equivalent to "<tt>addRow(rowData, true)</tt>".
     * @see #addRow(Object, boolean)
     */
    public void addRow(Object rowData) {
        addRow(rowData, true);
    }

    /**
     * Add a row to the bottom of the table.
     * Updates the cell renderers and optionally scrolls according the the
     * current policy.
     * The data is, in general, an array of Objects (instanceof Object[]);
     * one Object for each cell in the row.
     * For single-column tables, the Object need not be an array.
     * When adding many rows, consider setting "<tt>scrollFlag = false</tt>"
     * for all but the last row.
     * <p><b>Limitations:</b><br>
     * To use addRow, the TableModel must be (or extend) DefaultTableModel.
     * @param rowData The data for the last row. Either an Array
     * (instanceof Object[]) with length equal to the number of columns,
     * or (for single-column tables only) a non-Arrayed Object.
     * @param scrollFlag If true, will adjust the scroll position after
     * adding the row.
     */
    public void addRow(Object rowData, boolean scrollFlag) {
        if (!(getModel() instanceof DefaultTableModel)) {
            Messages.postError("EasyJTable.addRow method cannot be used: "
                               + "the TableModel is not a DefaultTableModel");
            return;
        }
            
        // Add data to the model
        DefaultTableModel tableModel = (DefaultTableModel)getModel();
        Object[] arrayData = {rowData};
        if (rowData instanceof Object[]) {
            arrayData = (Object[])rowData;
        }
        tableModel.addRow(arrayData);

        // Update the row height to the height of the tallest cell.
        // First we have to make sure that the component that is
        // rendering each cell is updated with the new content.
        // Normally, this does not happen when you add the row to
        // the table model, but when the table is drawn.
        // This makes it happen now.
        int lastRow = tableModel.getRowCount() - 1;
        int height = 0;
        for (int i = 0; i < arrayData.length; i++) {
            // Get the "renderer" for the cell
            TableCellRenderer renderer = getCellRenderer(lastRow, i);
            // Ask the renderer to make a Component that displays the content
            Component comp
                    = renderer.getTableCellRendererComponent(this,
                                                             arrayData[i],
                                                             false, // ~selected
                                                             false, // ~focus
                                                             lastRow,
                                                             i);
            // Ask the component how big it is
            height = Math.max(height, comp.getPreferredSize().height);
        }
        // Now we can set the correct height for the row.
        setRowHeight(lastRow, height);

        // Finally, set the scrolling
        if (scrollFlag) {
            positionScroll(REASON_ROW_APPENDED);
        }
    }

    /**
     * Make a row visible in the scrolling window.
     * Never scrolls horizontally.
     * @param row The index of the row to show.
     */
    public void scrollToRow(int row) {
        Rectangle cellRect = getCellRect(row, 0, true);
        int x = 0;              // Set rect x to something already in view
        Container parent = getParent();
        if (parent instanceof JViewport) {
            x = ((JViewport)parent).getX();
        }
        cellRect.x = x;
        cellRect.width = 1;
        scrollRectToVisible(cellRect);
    }

    /**
     * Make a row visible in the scrolling window.
     * Execution is deferred until all the tasks currently in the
     * EventQueue are done.
     * Never scrolls horizontally.
     * @param row The index of the row to show.
     */
    public void scrollToRowLater(int row) {
        Runnable scroll = new ScrollToRowTask(row);
        SwingUtilities.invokeLater(scroll);
    }

    /**
     * Position the scrolling according to the current state and
     * scrolling policy.
     * The caller specifies why the method is being called, i.e.,
     * how the table has been changed.
     * The choices are REASON_ROW_APPENDED, REASON_ROWS_RESIZED,
     * REASON_VIEW_RESIZED, REASON_INITIAL_VIEW, and REASON_ANY.
     *
     * @param reason Why we're calling this method. May be the arithmetic
     * OR of multiple reasons.
     * @return The "action" code that was performed, e.g.,
     * ACTION_SCROLL_TO_BOTTOM.
     */
    public int positionScroll(int reason) {
        // Always look for this:
        int state = STATE_ANY;

        // Find out where we are.
        int portHeight = getViewportHeight();
        int portY = getViewportY();
        int textHeight = getTextPaneHeight();
        if (portY + portHeight >= textHeight) {
            // Bottom of text is showing
            state |= STATE_BOTTOM_SHOWS;
        }

        // Find out what we should do.
        int action = ACTION_DONT_SCROLL;
        if (m_scrollPolicy != null) {
            for (int i = 0; i < m_scrollPolicy.length; i++) {
                boolean bReason = ((reason & m_scrollPolicy[i]) != 0);
                boolean bState = (state & m_scrollPolicy[i]) != 0;
                if (bReason && bState) {
                    action = m_scrollPolicy[i] & ACTION_MASK;
                    break;
                }
            }
        }

        // Do the scrolling.
        switch (action) {

        case ACTION_KEEP_TOP_OF_VIEW:
            break;              // This often works if we do nothing.

        case ACTION_SCROLL_TO_BOTTOM:
            scrollToRow(getModel().getRowCount() - 1);
            break;

        case ACTION_SCROLL_TO_TOP:
            scrollToRow(0);
            break;

        case ACTION_KEEP_BOTTOM_OF_VIEW:
            Messages.postMessage
                    (Messages.OTHER | Messages.DEBUG | Messages.LOG,
                     "EasyJTable.positionScroll: "
                     + "ACTION_KEEP_BOTTOM_OF_VIEW is not implemented");
            break;

        case ACTION_DONT_SCROLL:
            break;

        default:
            Messages.postMessage
                    (Messages.OTHER | Messages.DEBUG | Messages.LOG,
                     "EasyJTable.positionScroll: " + action
                     + " is not a valid scrolling action");
            break;
        }
        return action;
    }

    /**
     * Get the height of the virtual text area.
     * Some of this may not be visible because it's not in the viewport.
     * @return The height in pixels.
     */
    private int getTextPaneHeight() {
        return getHeight();
    }

    /**
     * Get the height of the viewport.
     * This is the height of the visible area of the text pane.
     * @return The height in pixels.
     */
    private int getViewportHeight() {
        int h;
        Container parent = getParent();
        if (parent instanceof JViewport) {
            h = ((JViewport)parent).getHeight();
        } else {
            h = getHeight();
        }
        return h;
    }

    /**
     * Get the Y posistion of the top of the viewport on the text pane.
     * Measured from the top of the text pane to the first visible
     * pixel in the pane.
     * @return The position in pixels.
     */
    private int getViewportY() {
        return -getY();
    }

    /**
     * Invoked when the row selection changes.
     * Override automatically copies the selected stuff to the
     * SystemSelection clipboard.
     * <p>
     * Note: For this to work, the component for the cell
     * (returned by TableCellRenderer.getTableCellRendererComponent())
     * must have a reasonable toString() method.
     */
    public void valueChanged(ListSelectionEvent e) {
        super.valueChanged(e);  // Repaints to show the new selection
        if (getSelectedRowCount() > 0 || getSelectedColumnCount() > 0) {
            copySelectedToSelectionClipboard();
        }
    }

    /**
     * Copy the selected cells to the SystemSelection clipboard.
     * (If only rows/columns are selected, the set of selected cells
     * is all the cells in the selected rows/columns.
     * If both columns and rows are selected, the set of selected cells
     * is the intersection of the set of cells in selected rows
     * with the set of cells in selected columns.)
     * <p>
     * To get the selection into the SystemClipboard as well use
     * the "Copy" key (on the Sun).
     * We won't change the SystemClipboard without specific user instruction.
     */
    private void copySelectedToSelectionClipboard() {
        TransferHandler th = getTransferHandler();
        Clipboard clipboard = getToolkit().getSystemSelection();
        if ((clipboard != null) && (th != null)) {
            th.exportToClipboard(this, clipboard, DnDConstants.ACTION_COPY);
        }
    }

    /**
     * This method override is a workaround to Java bug #4330950
     * (lost newly entered data in the cell when resizing column width).
     * This workaround contributed by "adubon" Oct 23, 2000.  (And the
     * bug is still "in progress" May 8, 2006!)
     */
    public void columnMarginChanged(ChangeEvent e) {
        editCellAt(0, 0);
        super.columnMarginChanged(e);
    }

    /**
     * Turn on/off automatic column width optimization on table resizing.
     * @see #optimizeColumnWidths()
     * @param isAuto True to enable auto optimization, false to disable.
     */
    public void setAutoColumnWidths(boolean isAuto) {
        Container parent = getParent();
        if (parent instanceof JViewport) {
            if (isAuto) {
                parent.addComponentListener(m_tableSizeListener);
            } else {
                parent.removeComponentListener(m_tableSizeListener);
            }
        }
    }

    /**
     * Optimizes the column widths for the current displayed table width.
     * Columns are always made at least wide enough to show all their
     * contents without "..."s showing. If it table width is then too
     * narrow to show all the columns at once, the horizonal scrollbar
     * is turned on.
     */
    public void optimizeColumnWidths() {
        int w;
        int sumMinWidths = 0;
        int nCols = getModel().getColumnCount();
        String errMsg = "Problem setting column widths";

        // First, see if we need a horizontal scrollbar
        for (int iCol = 0; iCol <= nCols; iCol++) {
            int tCol = convertColumnIndexToView(iCol);
            if (tCol >= 0 && tCol < nCols) {
                try {
                    w = getPreferredColumnWidth(iCol);
                    sumMinWidths += w;
                } catch (Exception e) {
                    Messages.writeStackTrace(e, errMsg);
                }
            }
        }

        boolean needScrollbar;
        if (getTableWidth() < sumMinWidths) {
            needScrollbar = true;
            setAutoResizeMode(JTable.AUTO_RESIZE_OFF); // Enables scrollbar
        } else {
            needScrollbar = false;
            setAutoResizeMode(JTable.AUTO_RESIZE_SUBSEQUENT_COLUMNS);
        }

        // Now set the widths appropriate for scrollbar or non-scrollbar
        for (int iCol = 0; iCol <= nCols; iCol++) {
            int tCol = convertColumnIndexToView(iCol);
            if (tCol >= 0 && tCol < nCols) {
                try {
                    TableColumn col = getColumnModel().getColumn(tCol);
                    w = getPreferredColumnWidth(iCol);
                    int preferredWidth = needScrollbar ? w : w + 20;
                    col.setMinWidth(w);
                    col.setPreferredWidth(preferredWidth);
                    col.setMaxWidth(4 * (w + 20));
                } catch (Exception e) {
                    Messages.writeStackTrace(e, errMsg);
                }
            }
        }
    }

    /**
     * Get the visible width of the table in pixels.
     */
    public int getTableWidth() {
        int width = getWidth();
        Container parent = getParent();
        if (parent instanceof JViewport) {
            width = parent.getWidth();
        }
        return width;
    }

    /**
     * Get the preferred width of a given column in pixels.
     * @param colIndex The index of the column in the table model (from 0).
     * @return The preferred width in pixels.
     */
    protected int getPreferredColumnWidth(int colIndex) {
        final int MIN_COLUMN_WIDTH = 22;
        final int COLUMN_PAD = 3;
        final int CELL_PAD = 1;

        if (colIndex < 0 || colIndex >= getModel().getColumnCount()) {
            Messages.postError("Bad column number passed to "
                               + "EasyJTable.getPreferredColumnWidth: "
                               + colIndex);
            return -1;
        }
        int width = 0;
        try {
            // The table column # for model column colIndex:
            int tCol = convertColumnIndexToView(colIndex);

            // Check width of header cell
            TableColumn col = getColumnModel().getColumn(tCol);
            TableCellRenderer hr = col.getHeaderRenderer();
            if (hr == null) {
                hr = getTableHeader().getDefaultRenderer();
            }
            if (hr != null) {
                String label = getColumnName(tCol);
                Component rc = hr.getTableCellRendererComponent
                        (this, label, false, false, 0, tCol);
                width = rc.getPreferredSize().width;
            }

            // Look at widths of column cells
            TableCellRenderer cr = getCellRenderer(0, tCol);
            int nRows = getRowCount();
            for (int iRow = 0; iRow < nRows; iRow++) {
                Object value = getValueAt(iRow, tCol);
                if (value == null) {
                    continue;
                }
                Component rc = cr.getTableCellRendererComponent
                        (this, value, false, false, iRow, tCol);
                width = Math.max(width, rc.getPreferredSize().width + CELL_PAD);
                if (value instanceof Boolean) {
                    break;      // Don't check other rows (size is constant)
                }
            }
            width += COLUMN_PAD;         // Minimum good-looking width
        } catch (Exception e) {
            Messages.writeStackTrace(e, "Problem calculating column widths");
        }
        width = Math.max(width, MIN_COLUMN_WIDTH);
        return width;
    }


    /**
     * This inner class listens for table resize events, so that the
     * table layout can be readjusted as necessary.
     */
    class TableSizeListener extends ComponentAdapter {

        public void componentResized(ComponentEvent ce) {
            optimizeColumnWidths();
        }
    }


    /**
     * This inner class is a Runnable used to queue up scroll requests
     * in the Event Thread.
     * Calls {@link #scrollToRow(int) scrollToRow} on the given row.
     */
    class ScrollToRowTask implements Runnable {
        /** The row this task scrolls to. */
        int mm_row;

        /**
         * Construct a ScrollTask object that will scroll to make
         * a given row visible.
         * @param row The index of the row to make visible.
         */
        public ScrollToRowTask(int row) {
            mm_row = row;
        }

        /**
         * Executed by the Event Thread when it is time.
         * Calls {@link #scrollToRow(int) scrollToRow} on the given row.
         */
        public void run() {
            scrollToRow(mm_row);
        }
    };
}
