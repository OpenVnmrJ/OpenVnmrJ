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

/**
 * paste target
 *
 * @author Mark Cao
 */
public interface PasteTarget extends CutCopyPasteTarget {
    /**
     * Requested a paste of the given transferable data.
     * @return true if paste succeeded
     */
    public boolean pasteRequested(Transferable transferable);

} // interface PasteTarget
