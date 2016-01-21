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
import java.awt.event.*;
import vnmr.util.*;

public class VUndoableText extends JTextField 
{
    VTextUndoMgr mgr=null;
    public VUndoableText() {
        setManager();
    }
    public VUndoableText(String s, int l) {
        super(s,l);
        setManager();
    }
    private void setManager(){
	    addFocusListener(new FocusAdapter() {
	        public void focusLost(FocusEvent evt) {
		        if(mgr!=null)
	                 Undo.removeUndoMgr(mgr,VUndoableText.this);
	        }
	        public void focusGained(FocusEvent evt) {
		        if(mgr==null)
	                mgr=new VTextUndoMgr(VUndoableText.this);
	            Undo.setUndoMgr(mgr,VUndoableText.this);
	        }
	    });
    }
}
