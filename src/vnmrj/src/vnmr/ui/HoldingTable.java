/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;

import  vnmr.util.*;
import  vnmr.bo.*;
import  vnmr.ui.shuf.*;

/**
 * Table of current items in the holding area.
 *
 * @author Mark Cao
 */
public class HoldingTable extends JTable implements VTooltipIF {
    /** data model */
    private HoldingDataModel dataModel;
    /** drop target listener */
    private DropTarget dropTarget;
    /** drag source */
    private DragSource dragSource;
    /** Row where mouse is pressed down */
    private int mouseDownRow;

    /**
     * constructor
     * @param sshare session share
     */
    public HoldingTable(SessionShare sshare) {
	dataModel = sshare.holdingDataModel();
	setModel(dataModel);
	JTableHeader header = getTableHeader();

	// set various attributes
	setShowGrid(false);
	setIntercellSpacing(new Dimension(0, 0));
	setSelectionBackground(Global.HIGHLIGHTCOLOR);
	setCellSelectionEnabled(false);
	header.setForeground(Color.blue);
	header.setResizingAllowed(true);
        // Do not allow rearranging of header columns.
        header.setReorderingAllowed(false);
	// Allow only a single selection
	setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

	// set cell renderers
	TableColumnModel colModel = header.getColumnModel();
	int numCols = colModel.getColumnCount();
	for (int i = 0; i < numCols; i++) {
	    TableColumn col = colModel.getColumn(i);
	    col.setCellRenderer(new HoldingCellRenderer(this));
	    col.setHeaderRenderer(new HeaderValRenderer(header));
	    col.setHeaderValue(dataModel.getHeaderValue(i));
	}

	// set width of lock column
// 	int lockWidth = HeaderValRenderer.getLockWidth();
// 	TableColumn col0 = colModel.getColumn(0);
// 	col0.setPreferredWidth(lockWidth);
// 	col0.setMinWidth(lockWidth);
// 	col0.setMaxWidth(lockWidth);

	// make the holding area a drop target
	dropTarget =
	    new DropTarget(this, new HoldingDropTargetListener(sshare));

	addMouseListener(new MouseAdapter() {
	    public void mousePressed(MouseEvent evt) {
		// Find out where the mouse is right now.
		Point p = new Point(evt.getX(), evt.getY());
		int row = rowAtPoint(p);

		// Save this row for use in dragGestureRecognized
		// Else the selected row ends up changing beyond my
		// control as the mouse is dragged.  I think this is
		// a Java bug.
		mouseDownRow = row; 
	    }
	});



	addMouseListener(new MouseAdapter() {
	    public void mouseClicked(MouseEvent evt) {
		int clicks = evt.getClickCount();
		if(clicks == 2) {
		    // We have a double click, What now?
		    
		    ShufflerItem item = dataModel.getRowItem(mouseDownRow);

		    if(item == null)
			return;

		    // Decide what is to be done with this item.
                    // Default target to Default, and action is double click.
                    item.actOnThisItem("Default", "DoubleClick", "");
		}
	    }
	});
	header.setPreferredSize(new Dimension(8, 4));

	// make this object a drag source
	dragSource = new DragSource();
	dragSource.createDefaultDragGestureRecognizer(this,
				    DnDConstants.ACTION_COPY,
				    new HoldingDragGestureListener());
	if (Util.isNativeGraphics()) {
            ToolTipManager.sharedInstance().unregisterComponent(this);
            VTipManager.sharedInstance().registerComponent(this);
        }

    } // HoldingTable()

    public void setTooltip(String str) { }

    public String getTooltip(MouseEvent ev) {
        return getToolTipText(ev);
    }

    class HoldingDragSourceListener implements DragSourceListener {
	public void dragDropEnd (DragSourceDropEvent evt) {}
	public void dragEnter (DragSourceDragEvent evt) {}
	public void dragExit (DragSourceEvent evt) {}
	public void dragOver (DragSourceDragEvent evt) {}
	public void dropActionChanged (DragSourceDragEvent evt) {}
    } // class HoldingDragSourceListener

    /************************************************** <pre>
     * Summary: Send dragged object to whomever it was dragged to.
     *
     </pre> **************************************************/

     class HoldingDragGestureListener implements DragGestureListener {
	public void dragGestureRecognized(DragGestureEvent evt) {
	    // Don't use the selected row.  Instead use the row we
	    // saved when the mouse was pressed down.  Because the selected 
	    // row ends up changing beyond my control as the mouse is 
	    // dragged.  I think this is a Java bug.
	    // We used to do a getSelectedRow() call here.
	    int row = mouseDownRow;
 
	    // Since the selection keeps changing on me, reset it to
	    // the value when the drag started.  I am not using this here
	    // but it keeps the selected item highlighted so it
	    // looks normal to the user.
 	    getSelectionModel().setSelectionInterval(row, row);


	    // Get the ShufflerItem to be passed.
	    ShufflerItem item = dataModel.getRowItem(row);

	    // Let the receiver know where this came from.
	    item.source = "HOLDING";
	    LocalRefSelection ref = new LocalRefSelection(item);
	    // We pass the ShufflerItem to be caughted where this
	    // is dropped.
	    dragSource.startDrag(evt, DragSource.DefaultMoveDrop, ref,
				 new HoldingDragSourceListener());
	}
    } // class HoldingDragGestureListener

} // class HoldingTable
