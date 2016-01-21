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
 * This is for cut and/or copy targets. If a target is meant to be used
 * only for copy operations, implement an empty cutCompletionRequested()
 * method.
 *
 * @author Mark Cao
 */
public interface CutCopyTarget extends CutCopyPasteTarget {
    /**
     * get transfer data
     * @return transfer data
     */
    public Transferable getTransferable();

    /**
     * cut completion requested
     */
    public void cutCompletionRequested();

} // interface CutCopyTarget
