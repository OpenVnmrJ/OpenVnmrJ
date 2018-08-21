/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;


import javax.swing.*;
import javax.swing.undo.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import vnmr.util.*;
import vnmr.ui.*;

/**
 * Title:       VUndoButton
 * Description: Button object that appear in the toolbar (undo).
 */

public class VUndoButton extends VToolBarButton implements UndoableEditListener
{
    public VUndoButton(SessionShare sshare, ButtonIF vif, String typ)
    {
	    super(sshare, vif, typ);
		addFocusListener(new FocusAdapter() {
		    public void focusGained(FocusEvent evt) {
		        Undo.restoreLastUndoMgr();
		    }
		});
	    Undo.addUndoListener(this);
    }

    public void undoableEditHappened(UndoableEditEvent e){
   	    UndoManager mgr=Undo.getCurrentUndoMgr();
   	    if(mgr != null && mgr.canUndo()){
   	        setToolTipText(mgr.getUndoPresentationName());
   	        setEnabled(true);
   	    }
   	    else{
    	    setToolTipText(tipStr);
   	        setEnabled(false);
   	    }
    }
    public void actionPerformed(ActionEvent  e) {
   	    UndoManager mgr=Undo.getLastUndoMgr();
		if(mgr != null && mgr.canUndo())
		    mgr.undo();
    }
 }
