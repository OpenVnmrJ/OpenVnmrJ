/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.datatransfer.*;

import vnmr.util.Messages;

/**
 * A class which implements the capability required to transfer an
 * object reference.
 *
 * @author Mark Cao
 */
public class LocalRefSelection implements Transferable {
    // ==== static variables
    /** local reference flavor */
    public static DataFlavor LOCALREF_FLAVOR;
    /** index of local reference flavor */
    private static final int LOCALREF = 0;
    /** supported flavors */
    private static DataFlavor[] flavors;

    // ==== instance variables
    /** data */
    private Object data;
						   
    static {
	try {
	    LOCALREF_FLAVOR =
                    new DataFlavor(DataFlavor.javaJVMLocalObjectMimeType,
                                   "local reference");
	} catch (Exception e) {
            Messages.writeStackTrace(e);
        }
	flavors = new DataFlavor[] {LOCALREF_FLAVOR};
    }

    /**
     * Creates a transferable object capable of transferring the
     * specified object.
     * @param data data
     */
    public LocalRefSelection(Object data) {
        this.data = data;
    } // LocalRefSelection()

    /**
     * Returns the array of flavors in which it can provide the data.
     */
    public DataFlavor[] getTransferDataFlavors() {
	return flavors;
    } // getTransferDataFlavors()

    /**
     * Returns whether the requested flavor is supported by this object.
     * @param flavor the requested flavor for the data
     */
    public boolean isDataFlavorSupported(DataFlavor flavor) {
	return flavor.equals(flavors[LOCALREF]);
    } // isDataFlavorSupported()

    /**
     * If the data was requested in the supported flavor, return data.
     *
     * @param flavor the requested flavor for the data
     * @exception UnsupportedFlavorException if requested flavor unsupported
     */
    public Object getTransferData(DataFlavor flavor) 
	throws UnsupportedFlavorException {
	if (flavor.equals(flavors[LOCALREF])) {
	    return data;
	} else {
	    throw new UnsupportedFlavorException(flavor);
	}
    } // getTransferData()

} // class LocalRefSelection
