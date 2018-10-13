/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.beans.*;

import vnmr.util.*;

/********************************************************** <pre>
 * Summary: Modal dialog with two additional buttons for add and remove.
 *
 *
 </pre> **********************************************************/

public class TagAddRemoveDialog extends ModalDialog
    			      implements ActionListener, PropertyChangeListener {
    /** Add Button */
    protected JButton addButton;
    /** Remove Button */
    protected JButton removeButton;
    /** Result */
    protected String result=null;
    /* Panel to hold the buttons. */
    protected JPanel panelForBtns;



    /************************************************** <pre>
     * Summary: Constructor, Add buttons to dialog box
     *
     </pre> **************************************************/
    public TagAddRemoveDialog() {
	super(Util.getLabel("_Locator_Add_Remove"));

	// Make a panel for the buttons
	panelForBtns = new JPanel();
	// It looks better with a border
	panelForBtns.setBorder(BorderFactory.createEmptyBorder(20,35,20,35));

	// Create the two items.
	addButton = new  JButton("Add to Group");
	removeButton = new  JButton("Remove From Group");

	// Add items to panel
	panelForBtns.add(addButton);
	panelForBtns.add(removeButton);

	// Add the panel top of the dialog.
	getContentPane().add(panelForBtns, BorderLayout.NORTH);

	// Set the buttons and the text item up with Listeners
	cancelButton.setActionCommand("cancel");
	cancelButton.addActionListener(this);
	helpButton.setActionCommand("help");
	helpButton.addActionListener(this);
	addButton.setActionCommand("add");
	addButton.addActionListener(this);
	addButton.setMnemonic('a');
	removeButton.setActionCommand("remove");
	removeButton.addActionListener(this);
	removeButton.setMnemonic('r');



	// OK disabled.
	okButton.setEnabled(false);

	setBgColor(Util.getBgColor());
	DisplayOptions.addChangeListener(this);

	// Make the frame fit its contents.
	pack();
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
	setBgColor(Util.getBgColor());
    }

    protected void setBgColor(Color bgColor)
    {
	setBackgroundColor(bgColor);

	panelForBtns.setBackground(Util.getBgColor());
	addButton.setBackground(Util.getBgColor());
	removeButton.setBackground(Util.getBgColor());
    }

    public void disableAdd() {
	addButton.setEnabled(false);
    }

    public void disableRemove() {
	removeButton.setEnabled(false);
    }

    public void enableAdd() {
	addButton.setEnabled(true);
    }

    public void enableRemove() {
	removeButton.setEnabled(true);
    }


    /************************************************** <pre>
     * Summary: Listener for all buttons.
     *
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e) {
	String cmd = e.getActionCommand();
	// Cancel
	if(cmd.equals("cancel")) {
	    result = null;
	    setVisible(false);
	}
	// Help
	else if(cmd.equals("help")) {
	    // Do not call setVisible(false);  That will cause
	    // the Block to release and the code which create
	    // this object will try to use userText.  This way
	    // the panel stays up and the Block stays in effect.
            displayHelp();
	}
	else if(cmd.equals("add")) {
	    result = "add";
	    setVisible(false);
	}
	else if(cmd.equals("remove")) {
	    result = "remove";
	    setVisible(false);
	}

    }


    /************************************************** <pre>
     * Summary: Show the dialog, wait for results, get and return results.
     *
     *	    The Dialog will be positioned over the component passed in.
     *	    This way, the cursor will not need to be moved.
     *	    If null is passed in, the dialog will be positioned in the
     *	    center of the screen.
     *
     </pre> **************************************************/
    public String showDialog(Component comp) {
	// Set this dialog on top of the component passed in.
	setLocationRelativeTo(comp);

	// Show the dialog and wait for the results. (Blocking call)
	setVisible(true);

	// If I don't do this, and the user hits a 'return' on the keyboard
	// the dialog comes visible again.
	transferFocus();

	return result;
    }
}
