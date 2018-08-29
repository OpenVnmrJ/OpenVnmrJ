/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;
import vnmr.bo.*;

import java.awt.*;
import java.util.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;
import javax.swing.undo.*;
import javax.swing.event.*;
import java.awt.Toolkit;

/**
 * Title:        Undo
 * Description:  Utilty class that processes global undo events.
 *               Does not need to be instantiated and contains only static methods.
 */
public class Undo
{
    static UndoableEditSupport support=new UndoableEditSupport();
    static UndoManager current=null;
    static UndoManager last=null;
    static KeyAdapter ka = new KeyAdapter() 
    {
        public void keyPressed(KeyEvent e) {
            int modifiers = e.getModifiers();
            int code=e.getKeyCode();
            if(code==KeyEvent.VK_UNDO || (code==KeyEvent.VK_Z && e.isControlDown())){
   	            if(current !=null){
                    if(e.isShiftDown()){
                        if(current.canRedo())
                            current.redo();
                    }
                    else{
                        if(current.canUndo())
                            current.undo();
                    }
                }
            }
        }
    };
        
    /** called by UndoableEditListeners  */   
    public static synchronized void addUndoListener(UndoableEditListener m){
        support.addUndoableEditListener(m);
    }
    
    /** called by UndoableEditListeners  */   
    public static synchronized void removeUndoListener(UndoableEditListener m){
        support.removeUndoableEditListener(m);
    }
    
    /** called by Undoable when a Undoable event is generated */   
    public static void postEdit(UndoableEdit anEdit){
        support.postEdit(anEdit);
    }
    
    /** called by Undoable component on gained focus*/   
    public static void setUndoMgr(UndoManager m,Component c){
        c.addKeyListener(ka);
        setUndoMgr(m);
    } 
       
    /** called by Undoable on gained context */   
    public static void setUndoMgr(UndoManager m){
        current=m;
	    postEdit(m);
    } 
       
    /** called by Undoable component on lost focus*/   
    public static void removeUndoMgr(UndoManager m,Component c){
        if(m!=null){
            c.removeKeyListener(ka);
            removeUndoMgr(m);
        }
    } 
       
    /** called by Undoable on lost context */   
    public static void removeUndoMgr(UndoManager m){
        if(m!=null){
			support.removeUndoableEditListener(m);
			if(m==current){
		        last=current;
		        current=null;
		    }
		    support.postEdit(new UndoManager());
        }
    }
        
    /** called by UndoableEditListeners on undoableEditHappened */   
    public static UndoManager getCurrentUndoMgr(){
        return current;
    }
       
    /** called by UndoableEditListeners on actionPerformed */   
    public static UndoManager getLastUndoMgr(){
        return last;
    }
    
    /** called by UndoableEditListeners on focusGained */   
    public static void restoreLastUndoMgr(){
        // note: A focus a gain for the Listener (e.g. undobutton) results in
        //       a focus lost for the current Undoable.
        //       This routine restores the Undo context to the last Undoable.
        if(current==null && last !=null){ 
            current=last;
            support.postEdit(current);
        }
    }   
 }

