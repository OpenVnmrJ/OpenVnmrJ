/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;
import vnmr.util.*;

import java.awt.*;
import java.util.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.undo.*;
import javax.swing.event.*;
import java.awt.Toolkit;

public class VTextUndoMgr extends UndoManager
{
    Document doc;
    Hashtable actions;
    protected UndoAction undoAction;
    protected RedoAction redoAction;
    JTextComponent text;

    public VTextUndoMgr(JTextComponent t) {
        text=t;
        Document doc=text.getDocument();
        createActionTable();
        undoAction = new UndoAction();
        redoAction = new RedoAction();
        doc.addUndoableEditListener(new TextEditListener());
        addKeymapBindings();
    }

    public void undo(){
        try {
            super.undo();
        	undoAction.updateUndoState();
        	redoAction.updateRedoState();
        	Undo.postEdit(VTextUndoMgr.this);
        }
        catch(CannotUndoException e){ }
    }

    public void redo(){
        try {
            super.redo();
            undoAction.updateUndoState();
            redoAction.updateRedoState();
            Undo.postEdit(VTextUndoMgr.this);
        }
        catch(CannotRedoException e){ }
    }
  
    //The following two methods allow us to find an
    //action provided by the editor kit by its name.
    
    private void createActionTable() {
        actions = new Hashtable();
        Action[] actionsArray = text.getActions();
        for (int i = 0; i < actionsArray.length; i++) {
            Action a = actionsArray[i];
            actions.put(a.getValue(Action.NAME), a);
        }
    }

    private Action getActionByName(String name) {
        return (Action)(actions.get(name));
    }

    //Add a key bindings to the key map for navigation.
    protected void addKeymapBindings() {
        //Add a new key map to the keymap hierarchy.
        Keymap keymap = text.addKeymap("MyEmacsBindings",text.getKeymap());

        //Ctrl-b to go backward one character
        Action action = getActionByName(DefaultEditorKit.backwardAction);
        KeyStroke key = KeyStroke.getKeyStroke(KeyEvent.VK_B, Event.CTRL_MASK);
        keymap.addActionForKeyStroke(key, action);

        //Ctrl-f to go forward one character
        action = getActionByName(DefaultEditorKit.forwardAction);
        key = KeyStroke.getKeyStroke(KeyEvent.VK_F, Event.CTRL_MASK);
        keymap.addActionForKeyStroke(key, action);

        //Ctrl-p to go up one line
        action = getActionByName(DefaultEditorKit.upAction);
        key = KeyStroke.getKeyStroke(KeyEvent.VK_P, Event.CTRL_MASK);
        keymap.addActionForKeyStroke(key, action);

        //Ctrl-n to go down one line
        action = getActionByName(DefaultEditorKit.downAction);
        key = KeyStroke.getKeyStroke(KeyEvent.VK_N, Event.CTRL_MASK);
        keymap.addActionForKeyStroke(key, action);

        text.setKeymap(keymap);
    }

    // listener for edits that can be undone.
    
    protected class TextEditListener
        implements UndoableEditListener 
    {
        public void undoableEditHappened(UndoableEditEvent e) {
            addEdit(e.getEdit());
            undoAction.updateUndoState();
            redoAction.updateRedoState();
            Undo.postEdit(VTextUndoMgr.this);
        }
    }

    class UndoAction extends AbstractAction {
        public UndoAction() {
            super("Undo");
            setEnabled(false);
        }
          
        public void actionPerformed(ActionEvent e) {
            try {
                undo();
            } catch (CannotUndoException ex) {
                System.out.println("Unable to undo: " + ex);
                ex.printStackTrace();
            }
        }
          
        protected void updateUndoState() {
            if (canUndo()) {
                setEnabled(true);
                putValue(Action.NAME, getUndoPresentationName());
            } else {
                setEnabled(false);
                putValue(Action.NAME, "Undo");
            }
        }      
    }    

    class RedoAction extends AbstractAction {
        public RedoAction() {
            super("Redo");
            setEnabled(false);
        }

        public void actionPerformed(ActionEvent e) {
            try {
                redo();
            } catch (CannotRedoException ex) {
                System.out.println("Unable to redo: " + ex);
                ex.printStackTrace();
            }
       }

        protected void updateRedoState() {
            if (canRedo()) {
                setEnabled(true);
                putValue(Action.NAME, getRedoPresentationName());
            } else {
                setEnabled(false);
                putValue(Action.NAME, "Redo");
            }
        }
    }    
}
