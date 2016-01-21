/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

/**
 * A cut-copy-paste object aids in cut, copy, and paste by (1) being
 * a central object where objects that could be cut, copied, or pasted
 * onto are registered.
 *
 * @author Mark Cao
 */
public class CutCopyPaste {
    /** current target of cut, copy, or paste */
    private CutCopyPasteTarget target;
    /** current cut or copy target */
    private CutCopyTarget cutCopyTarget;
    /** is current cutCopyTarget for copying or cutting too? */
    private boolean cutFlag;

    /**
     * Set target.  This is used to indicate that an object has been
     * highlighted and is a candidate for cut, copy, or paste; the
     * actual cut, copy, or paste action has not been initiated.
     * @param target target
     */
    public void setTarget(CutCopyPasteTarget target) {
	this.target = target;
    } // setTarget()

    /**
     * Start cutting the target by transferring its data to system clipboard.
     * At this point, paste has yet to be invoked.
     */
    public void startCut() {
	if (target instanceof CutCopyTarget) {
	    cutCopyTarget = (CutCopyTarget)target;
	    cutFlag = true;
	} else {
	    cutCopyTarget = null;
	}
    } // startCut()

    /**
     * start copying the target
     */
    public void startCopy() {
	if (target instanceof CutCopyTarget) {
	    cutCopyTarget = (CutCopyTarget)target;
	    cutFlag = false;
	} else {
	    cutCopyTarget = null;
	}
    } // startCopy()

    /**
     * paste the cut/copy object
     */
    public void paste() {
	if (target instanceof PasteTarget) {
	    PasteTarget pasteTarget = (PasteTarget)target;
	    if (cutCopyTarget != null) {
		boolean pasteSucceeded =
		   pasteTarget.pasteRequested(cutCopyTarget.getTransferable());
		if (pasteSucceeded && cutFlag)
		    cutCopyTarget.cutCompletionRequested();
	    }
	}
    } // paste()

} // class CutCopyPaste
