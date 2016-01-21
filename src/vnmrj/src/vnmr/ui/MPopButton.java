/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;

import vnmr.util.*;

/**
 * This is a button whose action results in a menu popup.
 *
 */
public class MPopButton extends JButton {
    /** popup menu */
    protected JPopupMenu popup;
    /** popup action listener */
    protected MPopActionListener popActionListener;
    /** popup selection listeners */
    private Vector popListeners;

    protected ArrayList m_aListValues = new ArrayList();

    /**
     * constructor
     */
    public MPopButton() {
    popListeners = new Vector();

    popup = new JPopupMenu();
    popActionListener = new MPopActionListener(this);

    addActionListener(new ActionListener() {
        public void actionPerformed(ActionEvent evt) {
        popup.show(MPopButton.this, 6, getSize().height -6);

        }
    });

    popup.addPopupMenuListener(new PopupMenuListener() {
                public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
                    Util.setMenuUp(true);
                }
                public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                    Util.setMenuUp(false);
                }
                public void popupMenuCanceled(PopupMenuEvent e) {
                }
         });

    } // MPopButton()

    /**
     * constructor
     * @param values list of values, String[]
     */
    public MPopButton(String[] values) {
    this();
    for (int i = 0; i < values.length; i++) {
        JMenuItem item = popup.add(values[i]);
        item.addActionListener(popActionListener);
        popup.add(item);
        m_aListValues.add(values[i]);
    }
    setDefaultText();
    } // MPopButton()

    /**
     * constructor
     * @param values list of values, ArrayList
     */
    public MPopButton(ArrayList values) {
    this();
    for (int i = 0; i < values.size(); i++) {
        JMenuItem item = popup.add((String)values.get(i));
        item.addActionListener(popActionListener);
        popup.add(item);
    }
    m_aListValues = values;
    setDefaultText();
    } // MPopButton()


    /**
     * set text to the specified item index
     * @param index index
     */
    public void setText(int index) {
    JMenuItem item = (JMenuItem)popup.getComponent(index);
    setText(item.getText());
    } // setText()

    /**
     * set default text
     */
    public void setDefaultText() {
    JMenuItem item = (JMenuItem)popup.getComponent(0);
    if (item != null)
        setText(item.getText());
    } // setDefaultText()

    public void setEnabled(String strTxt, boolean bEnabled)
    {
        if (m_aListValues != null && m_aListValues.contains(strTxt))
        {
            setEnabled(m_aListValues.indexOf(strTxt), bEnabled);
        }
    }

    public void setEnabled(int index, boolean bEnabled)
    {
        JMenuItem item = (JMenuItem)popup.getComponent(index);
        if (item != null)
            item.setEnabled(bEnabled);
    }

    /**
     * add a popup listener
     * @param listener listener
     */
    public void addPopListener(PopListener listener) {
    popListeners.addElement(listener);
    } // addPopListener()


    /**
     * notify listeners of a popup selection
     * @param popStr popup selection string
     */
    void popNotify(String popStr) {
    Enumeration en = popListeners.elements();
    for ( ; en.hasMoreElements(); ) {
        PopListener listener = (PopListener)en.nextElement();
        listener.popHappened(popStr);
    }
    } // popNotify()


/**
 * listens to menu selections
 */
class MPopActionListener implements ActionListener {
    /** button */
    private MPopButton popButton;

    /**
     * constructor
     * @param popButton MPopButton
     */
    MPopActionListener(MPopButton popButton) {
    this.popButton = popButton;
    } // MPopActionListener()

    /**
     * action performed
     * @param evt event
     */
    public void actionPerformed(ActionEvent evt) {
    popButton.popNotify(evt.getActionCommand());
    } // actionPerformed()
} // class MPopActionListener
} // class MPopButton
