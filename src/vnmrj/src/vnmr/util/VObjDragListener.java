/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.util.*;
import java.awt.event.*;
import java.awt.dnd.*;
import vnmr.ui.*;
import vnmr.bo.*;


public class VObjDragListener implements DragGestureListener {
    private DragSource dragSource;

    public VObjDragListener(DragSource dragSource) {
	this.dragSource = dragSource;
    }

    public void dragGestureRecognized(DragGestureEvent evt) {
	Component comp = evt.getComponent();
	if (comp == null || !(comp instanceof VObjIF)) {
	    return;
	}
	/* for draging VTextWin, should drag its container, not itself */
	if (comp instanceof VTextWin) {
	    VTextWin twin = (VTextWin) comp;
            comp = twin.getContainer();
	}
        LocalRefSelection ref = new LocalRefSelection(comp);
	dragSource.startDrag(evt, DragSource.DefaultMoveDrop, ref,
                                 new VObjDragSourceListener());
    }

    class VObjDragSourceListener implements DragSourceListener {
        public void dragDropEnd (DragSourceDropEvent evt) {
        }

        public void dragEnter (DragSourceDragEvent evt) { }
        public void dragExit (DragSourceEvent evt) { }
        public void dragOver (DragSourceDragEvent evt) { }
        public void dropActionChanged (DragSourceDragEvent evt) { }
    }
}

