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
 * Title:        VRedoButton
 * Description:  Button object that appear in the toolbar (redo).
 */

public class VRedoButton extends VToolBarButton implements UndoableEditListener
{
    public VRedoButton(SessionShare sshare, ButtonIF vif, String typ)
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
   	    if(mgr != null && mgr.canRedo()){
   	        setToolTipText(mgr.getRedoPresentationName());
   	        setEnabled(true);
   	    }
   	    else{
    	    setToolTipText(tipStr);
  	        setEnabled(false);
   	    }
    }
    public void actionPerformed(ActionEvent  e) {
   	    UndoManager mgr=Undo.getLastUndoMgr();
		if(mgr != null && mgr.canRedo())
		    mgr.redo();
    }
 }
