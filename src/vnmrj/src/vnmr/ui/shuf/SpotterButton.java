/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui.shuf;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.*;
import java.beans.*;

import  vnmr.bo.*;
import  vnmr.ui.*;
import  vnmr.util.*;

/**
 * spotter button
 *
 */
public class SpotterButton extends PopButton implements  Serializable, PropertyChangeListener {
    // ==== instance variables
    /** statement history */
    private SessionShare sshare;
    /** string to use for separator */
    private static final String separator = "-----------------------";

    static private boolean menuAlreadyFilled=false;

    /**
     * constructor
     * @param sshare session share
     */
    public SpotterButton(SessionShare sshare) {
	JMenuItem item;

	this.sshare = sshare;

//	setBackground(Global.BGCOLOR);
	setContentAreaFilled(false);
	// setBorder(BorderFactory.createEmptyBorder(0, 0, 0, 5));
	setMargin(new Insets(0,0,0,0));
	setBorder(new VButtonBorder());

	setUnderline(false);
	setIcon(Util.getImageIcon("search_20.png"));
	setToolTipText(Util.getLabel("_Locator_Statements"));

	addMouseListener(new MouseAdapter() {
                public void mousePressed(MouseEvent evt) {
                        // Save startup time by not filling this menu until
                        // it is first clicked.
                        fillPopupMenu();
                    }
            });

	addPopListener(new PopListener() {
	    public void popHappened(String popStr) {

		SessionShare sshare = ResultTable.getSshare();
		if (popStr.startsWith("save:")) {
		    String saveName = popStr.substring(5);
		    StatementHistory history = sshare.statementHistory();
		    history.readNamedStatement(saveName);
		}
		else if (popStr.startsWith("command:")) {
		    String commandName = popStr.substring(8);
		    int index = commandName.indexOf('/');
		    String objType = commandName.substring(0, index);
		    LocatorHistory lh = sshare.getLocatorHistory();
		    // Set History Active Object type to this type.
		    lh.setActiveObjType(objType);

		    // Now get history for this type.
		    StatementHistory history = sshare.statementHistory();
		    history.appendLastOfType(commandName);
		}
		else if (popStr.startsWith("title:")) {
		    String objType = popStr.substring(6);
		    LocatorHistory lh = sshare.getLocatorHistory();
                    // Update locator to the most recent statement for this type
		    lh.setHistoryToThisType(objType);
		}

	    }
	});
	DisplayOptions.addChangeListener(this);
    } // SpotterButton()

    public void propertyChange(PropertyChangeEvent evt)
    {
	int nCount = popup.getComponentCount();

	for (int i = 0; i < nCount; i++)
	{
	    JComponent objMItem = (JComponent)popup.getComponent(i);
//	    objMItem.setBackground(Util.getBgColor());
	}
    }

    /**
     * When a new set of saved statements come in, refresh the menu
     * of saved statements.
     */
    private void refreshSaveMenu() {
	ArrayList list;
	// first, delete what's already there
	for (;;) {
	    Component comp = popup.getComponent(1);
	    if (!(comp instanceof JMenuItem))
		break;

	    JMenuItem item = (JMenuItem)comp;
	    if (item.getActionCommand().startsWith("save:"))
		popup.remove(1);
	    else
		break;
	}
	StatementHistory history;
	history = sshare.statementHistory();
	// now insert the new list of saved statements
	list = history.getNamedStatementList();
	Color bgColor = Util.getBgColor();
	for(int i=0; i < list.size(); i++) {
            ArrayList nameNlabel = (ArrayList)list.get(i);
	    JMenuItem item = new JMenuItem("  " + (String)nameNlabel.get(1));
	    item.setActionCommand("save:" + (String)nameNlabel.get(0));
	    popup.add(item, 1);
//	    item.setBackground(bgColor);
	    item.addActionListener(popActionListener);
	}

    } // refreshSaveMenu()

    public void updateMenu() {

	// Remove all current items from popup
	int count = popup.getComponentCount();
	for (int i=0; i < count; i++) {
	    popup.remove(0);
	}

        menuAlreadyFilled = false;

	// Refill based on current StatementHistory/ShufflerService
	fillPopupMenu();

    }

    public void fillPopupMenu() {
	JMenuItem item;
	TextImageIcon textIcon;
	StatementHistory history;
        int rowCount=0;
        int numRows=0;
        Insets margin = new Insets(0,0,0,0);

        // Get the font defined in the displayOptions panel for menus
        Font ft = DisplayOptions.getFont("Menu1");
        // We need a fairly small font, so make it 2 smaller.
        int size = ft.getSize();
        // If larger than 12, subtract 2
        if(size > 12)
            size -= 2;
        Font font = DisplayOptions.getFont(ft.getName(), ft.getStyle(), size);

        // This flag is used so that we only fill the menu when needed
        if(menuAlreadyFilled)
            return;

        menuAlreadyFilled = true;

	history = sshare.statementHistory();

	ArrayList list = history.getNamedStatementList();
	Color bgColor = Util.getBgColor();
	// Only show the Saved Statements section if there are some.
	if(list != null && list.size() != 0) {
	    item = popup.add("Saved Statements");
            rowCount++;
//	    item.setForeground(Color.blue);
//	    item.setBackground(bgColor);
            item.setFont(font);
            item.setMargin(margin);
	    popup.add(item);

	    for(int i=0; i < list.size(); i++) {
                ArrayList nameNlabel = (ArrayList)list.get(i);
                // first item in nameNlabel is name and second is label
		item = popup.add("  " + (String)nameNlabel.get(1));
                rowCount++;
		item.setActionCommand("save:" + (String)nameNlabel.get(0));
//		item.setBackground(bgColor);
		item.addActionListener(popActionListener);
                item.setFont(font);
                item.setMargin(margin);

	    }
	}
	// the rest of menu (return object types, statement types, etc.)
	ShufflerService shufflerService = sshare.shufflerService();
	ArrayList objTypes = shufflerService.getAllMenuObjectTypes();

	for (int i = 0; i < objTypes.size(); i++) {
	    String objType = (String) objTypes.get(i);

            // Do not display menu for DB_AVAIL_SUB_TYPES
            if(objType.equals(Shuf.DB_AVAIL_SUB_TYPES))
                continue;

            // addSeparator looks bad using GridLayout because it creates rows
            // and columns which are all equal in size.  It cannot have a row
            // with a separator which is a different height than the other
            // rectangles it creates. So just use dashes.
	    item = popup.add(separator);
            item.setFont(font);
            item.setMargin(margin);
            rowCount++;
	    item = popup.add(shufflerService.getCategoryLabel(objType));
            rowCount++;
//	    item.setForeground(Color.blue);
	    item.setActionCommand("title:" + objType);
//	    item.setBackground(bgColor);
	    item.addActionListener(popActionListener);
            item.setFont(font);
            item.setMargin(margin);

	    ArrayList menuStrings =
		shufflerService.getmenuStringsThisObj(objType);

            // If current rowCount plus the next section size is too big,
            // specify the numRows to the current value of rowCount -1.
            // That is, put this next section in a new column.
            // 47 is emperical number of rows to fit 90% full screen.
            if(numRows == 0 && rowCount - 1 + menuStrings.size() > 44) {

                numRows = rowCount -2;
            }

	    for (int j = 0; j < menuStrings.size(); j++) {
		String menuString = (String) menuStrings.get(j);
		item = popup.add("  " + menuString);
                rowCount++;
		item.setActionCommand("command:" + objType + "/" + menuString);
//		item.setBackground(bgColor);
		item.addActionListener(popActionListener);
                item.setFont(font);
                item.setMargin(margin);
	    }
	    // The spotter menu changes dynamically when
	    // the list of saved statements changes.
	    history.addStatementListener(new StatementAdapter() {
		public void saveListChanged() {
		    refreshSaveMenu();
		}
	    });
	}

        if(numRows == 0)
            numRows = rowCount;

	GridLayoutCol lm = new GridLayoutCol(numRows, 0);
	popup.setLayout(lm);
    }

} // class SpotterButton
