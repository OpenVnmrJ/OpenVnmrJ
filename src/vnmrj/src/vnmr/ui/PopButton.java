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

import  vnmr.util.*;

/**
 * This is a button whose action results in a menu popup.
 *
 */
public class PopButton extends JButton {
    /** popup menu */
    protected JPopupMenu popup;
    /** popup action listener */
    protected PopActionListener popActionListener;
    /** underline mode on/off */
    private boolean underline;
    /** automatically update text to reflect selection? */
    protected boolean autoUpdate;
    /** popup selection listeners */
    private Vector popListeners;
    protected boolean m_bSetInvisible = false;
    private boolean disableBtn1=false;


    public PopButton() {
        // When called with no arg, default to use of Btn 1
        this(false);
    }
    /**
     * constructor
     */
    public PopButton(boolean disableBtn1) {
        this.disableBtn1 = disableBtn1;
        setBorder(BorderFactory.createEmptyBorder());
//        setForeground(Color.blue);
        underline = true;
        autoUpdate = false;
        popListeners = new Vector();

        popup = new JPopupMenu();
        popActionListener = new PopActionListener(this);

        addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {

                    // Default for a JButton is for actionPerformed upon
                    // a Btn 1 click.  DateRangePanel needs it to respond
                    // only to Btn 3, so it will do a fireActionPerformed
                    // upon Btn3.  It will also have create this object
                    // with disableBtn1=true.  Also, since it comes from
                    // a fireActionPerformed, the modifiers have 0 bits set.
                    // Therefore, if disableBtn1 is set, and no modifier
                    // bits are set, then it must be Btn 3 fired from
                    // DateRangePanel
                    
                    int mod = evt.getModifiers();
                    if(PopButton.this.disableBtn1 && mod != 0)
                        return;
                    else {
                        if (Util.isMenuUp() ||  popup.isShowing())
                        {
                            Util.setMenuUp(false);
                        }
                        else if (!m_bSetInvisible)
                        {
                            popup.show(PopButton.this, 0, getSize().height);
                        }
                    }
                }
        });

        popup.addPopupMenuListener(new PopupMenuListener() {
                public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
                    Util.setMenuUp(true);
                }
                public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                    Util.setMenuUp(false);
                    m_bSetInvisible = true;
                }
                public void popupMenuCanceled(PopupMenuEvent e) {
                }
         });

         addMouseListener(new MouseAdapter()
         {
            public void mousePressed(MouseEvent e)
            {
                if (!Util.isMenuUp())
                    m_bSetInvisible = false;
            }
         });

    } // PopButton()

    /**
     * constructor
     * @param values list of values, String[]
     */
    public PopButton(String[] values, boolean disableBtn1) {
        this(disableBtn1);
        for (int i = 0; i < values.length; i++) {
            JMenuItem item = popup.add(values[i]);
            item.addActionListener(popActionListener);
        }
        setDefaultText();
    } // PopButton()

    /**
     * constructor
     * @param values list of values, ArrayList
     */
    public PopButton(ArrayList values, boolean disableBtn1) {
        this(disableBtn1);
        for (int i = 0; i < values.size(); i++) {
            JMenuItem item = popup.add((String)values.get(i));
            item.addActionListener(popActionListener);
        }
        setDefaultText();
    } // PopButton()

    /**
     * Set a numbered menu. The given strings are the names, while
     * their orders (indices) are the action commands.
     * @param names labels displayed
     */
    public void setNumberedMenu(String[] names) {
        popup.removeAll();
        for (int i = 0; i < names.length; i++) {
            JMenuItem item = popup.add(names[i]);
            item.addActionListener(popActionListener);
            item.setActionCommand(Integer.toString(i));
        }
        if (names.length > 0)
            setText(0);
        else
            setText("");
    } // setNumberedMenu()

    /**
     * set text to the specified item index
     * @param index index
     */
    public void setText(int index) {
        JMenuItem item = (JMenuItem)popup.getComponent(index);
        if (item != null)
            setText(item.getText());
    } // setText()

    /**
     * set default text
     */
    public void setDefaultText() {
        setText(0);
    } // setDefaultText()

    /**
     * add a popup listener
     * @param listener listener
     */
    public void addPopListener(PopListener listener) {
        popListeners.addElement(listener);
    } // addPopListener()

    /**
     * Set underline mode. By default, true.
     * @param underline true/false
     */
    public void setUnderline(boolean underline) {
        this.underline = underline;
    } // setUnderline()

    /**
     * set auto update mode. By default, false.
     * @param autoUpdate true/false
     */
    public void setAutoUpdate(boolean autoUpdate) {
        this.autoUpdate = autoUpdate;
    } // setAutoUpdate()

    /**
     * paint
     * @param g Graphics
     */
    public void paint(Graphics g) {
        super.paint(g);
        if (underline) {
            Dimension size = getSize();
            int height0 = size.height - 1;
            g.drawLine(0, height0, size.width - 1, height0);
        }
    } // paint()

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
class PopActionListener implements ActionListener {
    /** button */
    private PopButton popButton;

    /**
     * constructor
     * @param popButton PopButton
     */
    PopActionListener(PopButton popButton) {
        this.popButton = popButton;
    } // PopActionListener()

    /**
     * action performed
     * @param evt event
     */
    public void actionPerformed(ActionEvent evt) {
        if (popButton.autoUpdate)
            popButton.setText(((JMenuItem)(evt.getSource())).getText());
        popButton.popNotify(evt.getActionCommand());
        Util.setMenuUp(false);
    } // actionPerformed()
} // class PopActionListener

} // class PopButton
