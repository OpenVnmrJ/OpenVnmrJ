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
import java.awt.datatransfer.*;
import java.awt.dnd.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;
import java.net.*;


import  vnmr.bo.*;
import  vnmr.util.*;
import  vnmr.ui.shuf.*;

/**
 * DropTargetListener for holding area.
 *
 * @author Mark Cao
 */
class HoldingDropTargetListener implements DropTargetListener {
    /** session share */
    private SessionShare sshare;

    /**
     * constructor
     * @param sshare session share
     */
    public HoldingDropTargetListener(SessionShare sshare) {
	this.sshare = sshare;
    } // HoldingDropTargetListener()

    public void dragEnter(DropTargetDragEvent evt) {}
    public void dragExit(DropTargetEvent evt) {}
    public void dragOver(DropTargetDragEvent evt) {}

    public void drop(DropTargetDropEvent evt) {

	try {
	    Transferable tr = evt.getTransferable();

            // Catch drag of String (probably from JFileChooser)
            if (tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                Object obj = tr.getTransferData(DataFlavor.stringFlavor);
                evt.acceptDrop(DnDConstants.ACTION_MOVE);

                // The object, being a String, is the fullpath of the
                // dragged item.
                String fullpath = (String) obj;
                File file = new File(fullpath);
                if (!file.exists()) {
                    Messages.postError("File not found " + fullpath);
                    return;
                }


                InetAddress inetAddress = InetAddress.getLocalHost();
                String localHost = inetAddress.getHostName();
                String hostFullpath = localHost + ":" + fullpath;

                // Create a ShufflerItem
                // Since the code is accustom to dealing with ShufflerItem's
                // being D&D, I will just use that type and fill it in.
                // This allows ShufflerItem.actOnThisItem() and the macro
                // locaction to operate normally and take care of this item.
                ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");

                // Finish as we would if a ShufflerItem had been dropped
                Point pt = evt.getLocation();
                HoldingArea hArea = Util.getHoldingArea();
                int row = hArea.m_holdingTable.rowAtPoint(pt);

                if(row == -1) {
                    // if -1, append to end, get the current last row #
                    row = hArea.m_holdingTable.getRowCount();
                }

                // add at cursor location
                sshare.holdingDataModel().add(row, item);
 
                evt.getDropTargetContext().dropComplete(true);

                return;
            }
            // If not a String, then should be here
	    if (tr.isDataFlavorSupported(LocalRefSelection.
					 LOCALREF_FLAVOR)) {
		Object obj =
		    tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
		if (obj instanceof ShufflerItem) {
		    evt.acceptDrop(DnDConstants.ACTION_MOVE);
		    ShufflerItem item = (ShufflerItem) obj;
			    	
                    Point pt = evt.getLocation();
                    HoldingArea hArea = Util.getHoldingArea();
                    int row = hArea.m_holdingTable.rowAtPoint(pt);

                    if(row == -1) {
                        // if -1, append to end, get the current last row #
                        row = hArea.m_holdingTable.getRowCount();
                    }

                    // add at cursor location
                    sshare.holdingDataModel().add(row, item);

		    evt.getDropTargetContext().dropComplete(true);

		    // If objType is DB_VNMR_DATA, then tell set the
		    // arch lock on this item and in the DB.
		    if(item.objType.equals(Shuf.DB_VNMR_DATA)) {
			ResultTable resultTable = Shuffler.getresultTable();

			ResultDataModel dataModel = resultTable.getdataModel();
			// Force archive lock to locked (value of 1)
			// set the Lock column icon and update the DB
			dataModel.setLockStatus(item.sourceRow, 
						new LockStatus(true));
		    }
		    return; 
		}
	    }
	} catch (IOException io) {
	    Messages.writeStackTrace(io);
	} catch (UnsupportedFlavorException ufe) {
	    Messages.writeStackTrace(ufe);
	}
	evt.rejectDrop();
    }

    public void dropActionChanged (DropTargetDragEvent evt) {}

} // class HoldingDropTargetListener
