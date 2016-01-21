/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.dnd.*;
import javax.swing.*;
import java.awt.datatransfer.*;
import java.io.IOException;
import java.util.Vector;


public class DroppableList extends JList implements DropTargetListener {
    DropTarget dropTarget;
    Vector v = new Vector();

    public DroppableList() {
        dropTarget = new DropTarget(this,this);
    }

    public void dragEnter (DropTargetDragEvent e) {
        e.acceptDrag ( DnDConstants.ACTION_COPY);
    }

    public void dragExit (DropTargetEvent e) {
    }

    public void dragOver (DropTargetDragEvent e) {
    }

    public void drop (DropTargetDropEvent e) {
        try {
            Transferable tr = e.getTransferable();
            if ( tr.isDataFlavorSupported (DataFlavor.stringFlavor) ) {
                e.acceptDrop (DnDConstants.ACTION_COPY_OR_MOVE);
		        String s = (String)tr.getTransferData (DataFlavor.stringFlavor);
		        v.add(s);
		        setListData(v);
		        paintImmediately(getVisibleRect());
		        e.getDropTargetContext().dropComplete(true);
            } else {
		        System.err.println("Rejected");
		        e.rejectDrop();
            }
        } catch (IOException io) {
	        io.printStackTrace();
	        e.rejectDrop();
        } catch (UnsupportedFlavorException ufe) {
	        ufe.printStackTrace();
	        e.rejectDrop();
        }
    }

    public void dropActionChanged (DropTargetDragEvent e) {
    }
}
