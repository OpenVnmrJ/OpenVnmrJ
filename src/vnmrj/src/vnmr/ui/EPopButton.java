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
import vnmr.ui.*;
import vnmr.ui.shuf.*;

/**
 * This is a button whose action results in a menu popup.
 */
public class EPopButton extends JTextField {
    /** popup menu */
    protected JPopupMenu popup;
    /** popup action listener */
    protected EPopActionListener popActionListener;
    /** underline mode on/off */
    private boolean underline;
    /** automatically update text to reflect selection? */
    protected boolean autoUpdate;
    /** listener and statement element */
    private Vector popListeners;
    /** length of a column */
    private static final int colLength = 40;
    /** string to use for separator */
    private static final String separator = "----------";
    /** if the menu should be set to invisible */
    protected boolean m_bSetInvisible = false;

    /**
     * constructor
     */
    public EPopButton() {

	setBorder(BorderFactory.createEmptyBorder());
//	setForeground(Color.blue);
	underline = true;
	autoUpdate = false;

        // The default cursor for a text item is the one that looks like
        // a capital I.  I want the standard arrow, this gets it.
        Cursor cursor = Cursor.getDefaultCursor();
        setCursor(cursor);

	popListeners = new Vector();

	popup = new JPopupMenu();

	popActionListener = new EPopActionListener(this);

	addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    // Come here when a 'return' is hit in one of the 
                    // TextFields.

                    // Get the value of this item
                    String newVal = getText();
                    
                    // Postgres wildcards are '%' and '_'.  Translate the
                    // user entries of '*' and '?' to the postgres ones.
                    // These new characters will show on the display.
                    newVal = newVal.replace('*', '%');
                    newVal = newVal.replace('?', '_');

                    // I don't know if this is the appropriate way or not,
                    // but this is what selecting something from the menu
                    // does and it seems to work here also.
                    popNotify(newVal); 

                    // The size of the item does not change to fit the new
                    // string with just the popNotify(), so call revalidate
                    // to have the shuffler redrawn as necessary.
                    Shuffler shuffler = Util.getShuffler();
                    shuffler.revalidate();
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

        addMouseListener(new MouseAdapter() {
                public void mousePressed(MouseEvent e) {
                    int button = e.getButton();
                    if(button == MouseEvent.BUTTON3) {
                        // Button 3, bring up menu if not up, take down if up
                        if (Util.isMenuUp() ||  popup.isShowing()) {
                            // Take down menu
                            Util.setMenuUp(false);
                        }
                        else {
                            // Put up menu if down
                            popup.show(EPopButton.this, 0, getSize().height);
                        }
                    }
                    else {
                        // Buttons 1 and 2, take down menu if up
                        if (Util.isMenuUp() || popup.isShowing()) {
                            Util.setMenuUp(false);

                        }
                    }
                }
            });

    } // EPopButton()
    /**
     * constructor
     * @param values list of values, ArrayList
     */
    public EPopButton(ArrayList values) {
	this();

	String str;
	int rows;

	if(values.size() > colLength)
	    rows = colLength;
	else if(values.size() == 0)
	    rows = 1;
	else
	    rows =  values.size();

	// Use the modified GridLayout layout manager so that it will fill
	// down the columns instead of across the rows.
	GridLayoutCol lm = new GridLayoutCol(rows, 0);
	popup.setLayout(lm);

	for (int i = 0; i < values.size(); i++) {
	    str = (String)values.get(i);

	    // If separator is the first thing, skip it
	    if(str.equals(Shuf.SEPARATOR) && i > 0) {
		// The method addSeparator() looks bad here, so just
		// put a string of dashes or something.
		popup.add(separator);
	    }
	    else if (!str.equals(Shuf.SEPARATOR)) {
		JMenuItem item = popup.add(str);
		item.addActionListener(popActionListener);
	    }
	}
	// Protect against failures due to empty menu
	if(values.size() == 0)
	    popup.add(" ");

	setDefaultText();
    } // EPopButton()

    /**
     * constructor
     * @param values list of values, String[]
     */
    public EPopButton(String[] values) {
	this();

	String str;
	int rows;

	if(values.length > colLength)
	    rows = colLength;
	else if(values.length == 0)
	    rows = 1;
	else
	    rows =  values.length;

	// Use the modified GridLayout layout manager so that it will fill
	// down the columns instead of across the rows.
	GridLayoutCol lm = new GridLayoutCol(rows, 0);
	popup.setLayout(lm);

	for (int i = 0; i < values.length; i++) {
	    str = (String)values[i];
	    // If separator is the first thing, skip it
	    if(str.equals(Shuf.SEPARATOR) && i > 0) {
		// The method addSeparator() looks bad here, so just
		// put a string of dashes or something.
		popup.add(separator);
	    }
	    else  if (!str.equals(Shuf.SEPARATOR)){
		JMenuItem item = popup.add(values[i]);
		item.addActionListener(popActionListener);
	    }
	}

	// Protect against failures due to empty menu
	if(values.length == 0)
	    popup.add(" ");

	setDefaultText();
    } // EPopButton()

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

    /**
     * add a popup listener
     * @param listener listener
     */
    public void addPopListener(EPopListener listener, StatementElement elem) {
	// Put these two args into a single class and add that to the Vector.
	PopActionInfo popActionInfo = new PopActionInfo(listener, elem);

	popListeners.addElement(popActionInfo);
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
	    PopActionInfo info = (PopActionInfo)en.nextElement();
	    info.listener.popHappened(popStr, info.statementElement);
	}
    } // popNotify()

    public void resetMenuChoices(ArrayList newlist) {
	String str;
	int rows;

	// Get current list of JMenuItem items
	Component[] items = (Component[]) popup.getComponents();

	// Remove them all
	for(int i = 0; i < items.length; i++)
	    popup.remove(items[i]);


	if(newlist.size() > colLength)
	    rows = colLength;
	else if(newlist.size() == 0)
	    rows = 1;
	else
	    rows =  newlist.size();

	// Use the modified GridLayout layout manager so that it will fill
	// down the columns instead of across the rows.
	GridLayoutCol lm = new GridLayoutCol(rows, 0);
	popup.setLayout(lm);

	// Add each of the new ones from the list
	for (int i = 0; i < newlist.size(); i++) {
	    str = (String)newlist.get(i);
	    // If separator is the first thing, skip it
	    if(str.equals(Shuf.SEPARATOR) && i > 0) {
		// The method addSeparator() looks bad here, so just
		// put a string of dashes or something.
		popup.add(separator);
	    }
	    else if (!str.equals(Shuf.SEPARATOR)) {
		JMenuItem item = popup.add(str);
		item.addActionListener(popActionListener);
	    }
	}
	setDefaultText();
    }

} // class EPopButton

/**
 * listens to menu selections
 */
class EPopActionListener implements ActionListener {
    /** button */
    private EPopButton popButton;

    /**
     * constructor
     * @param popButton EPopButton
     */
    EPopActionListener(EPopButton popButton) {
	this.popButton = popButton;
    } // EPopActionListener()

    /**
     * action performed
     * @param evt event
     */
    public void actionPerformed(ActionEvent evt) {
	if (popButton.autoUpdate)
	    popButton.setText(((JMenuItem)(evt.getSource())).getText());

	popButton.popNotify(evt.getActionCommand());
        // The size of the item does not always change to fit the new
        // string with just the popNotify(), so call revalidate
        // to have the shuffler redrawn as necessary.
        Shuffler shuffler = Util.getShuffler();
        shuffler.revalidate();

    } // actionPerformed()
} // class EPopActionListener

class PopActionInfo {
    /** popup selection listeners */
    protected EPopListener listener;
    /* StatementElement which goes with popListeners */
    protected StatementElement statementElement;

    PopActionInfo(EPopListener listener, StatementElement element) {
	this.listener = listener;
	statementElement = element;
    }
}
