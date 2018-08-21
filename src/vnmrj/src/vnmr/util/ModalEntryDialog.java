/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import java.awt.*;
import java.beans.*;
import java.awt.event.*;


/********************************************************** <pre>
 * Summary: Modal Entry Dialog Base Class with OK, Cancel & Help
 *
 *   What it this:
 *	This is a generic Modal Dialog Base Class which has Three buttons
 *	across the bottom as standard fixtures. They are left to right
 *	OK, Cancel and Help.  It also has a message and an input text area.
 *	This Class extends JDialog and thus is treated much like
 *	any JDialog with the exception of the standard buttons and the
 *	fact that it is fixed as modal.
 *
 *	The user can navigate from button to button using the 'tab' key
 *	and then hitting a 'return' will activate that button.
 *	The focus is set up such that typing anyplace, anytime in the
 *	dialog will direct focus to the text item.
 *	If tab has not been clicked to move the focus, 'return' will
 *	activate the 'OK' button.
 *	If the 'ESC' key is hit it activates the 'Cancel' button.
 *   How to use it:
 *	Create one of these Like the following:
 *		ModalEntryDialog med = new ModalEntryDialog(
 *					   "My Dialog Title",
 *					   "My Question for the User");
 *	Then call the following command to get the results:
 *		String results = med.showDialogAndGetValue(Button_clicked);
 *
 *	Where Button_clicked is the component which activated this
 *	entry dialog.  This will place the dialog on top of this
 *	component so the user does not have to move the mouse to find it.
 *
 *	Then dispose of this dialog when you are finished with:
 *		med.dispose();
 *	OR
 *	Create a single instance of this dialog box, and just call
 *	showDialogAndGetValue() each time you need to display it.
 *
 *	If the user clicks Cancel, results = null
 *	If the user clicks Help,   results = null
 *	If the user clicks OK,	   results = the contents of the text field
 *
 *
 </pre> **********************************************************/
public class ModalEntryDialog extends ModalDialog implements ActionListener,
                                         PropertyChangeListener,
                                         KeyListener {
    /** Text input Field */
    protected JTextField inputText;
    /** Text from User */
    protected String userText=null;
    /* Panel to hold the three standard buttons. */
    protected JPanel panelForText;
    /* Message */
    protected JLabel messageText;


    /************************************************** <pre>
     * Summary: Constructor, Add label and text fields to dialog box
     *
     </pre> **************************************************/
    public ModalEntryDialog(String title, String message)
    {
        this(title, message, "");
    }

    public ModalEntryDialog(String title, String message, String helpFile) {
        super(title);
        m_strHelpFile = helpFile;

        // Make a panel for the text items
        GridLayoutCol mgr = new GridLayoutCol(0, 1, 10, 10);
        panelForText = new JPanel(mgr);
        // It looks better with a border
        panelForText.setBorder(BorderFactory.createEmptyBorder(20,20,20,20));

        // Create the two items.
        messageText = new JLabel (message);
        inputText = new JTextField (20);

        // Add items to panel
        panelForText.add(messageText);
        panelForText.add(inputText);

        // Add the panel top of the dialog.
        getContentPane().add(panelForText, BorderLayout.NORTH);

        // Set the buttons and the text item up with Listeners
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        cancelButton.setActionCommand("cancel");
        cancelButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
        inputText.setActionCommand("text");
        inputText.addActionListener(this);
        inputText.addKeyListener(this);

        // Start OK disabled until something has been entered.
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

	panelForText.setBackground(bgColor);
	messageText.setBackground(bgColor);
	inputText.setBackground(bgColor);
	messageText.setBackground(bgColor);
    }


    /************************************************** <pre>
     * Summary: Listener for all buttons and the text field.
     *
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        // OK
        if(cmd.equals("ok")) {
            userText =inputText.getText();
            setVisible(false);
        }
        // Cancel
        else if(cmd.equals("cancel")) {
            userText = null;
            setVisible(false);
        }
        // Help
        else if(cmd.equals("help")) {
	    // Do not call setVisible(false);  That will cause
	    // the Block to release and the code which create
	    // this object will try to use userText.  This way
	    // the panel stays up and the Block stays in effect.
            userText = null;
            //Messages.postError("Help Not Implemented Yet");
            displayHelp();
        }
        // Text Field
        else if(cmd.equals("text")) {
            // Have a return do the same as clicking ok.
            okButton.doClick();
        }
    }



    /************************************************** <pre>
     * Summary: Catch any key pressed.
     *
     *    Since we extend ModalDialog and ModalDialog implements KeyListener,
     *	  ModalEntryDialog is a KeyListener.  ModalDialog has already
     *    called addKeyListener(), so we do not want to call it again,
     *    else we add the listener twice.  Here we just override ModalDialog's
     *	  keyPressed.  So, we need to call super.keyPressed() so ModalDialog
     *    can do its thing.
     </pre> **************************************************/
    public void keyPressed(KeyEvent e) {

	// default focus to text item if any key.
	if(!inputText.hasFocus())
	    inputText.grabFocus();

	// Enable OK as soon as something is typed.
	if(!okButton.isEnabled())
	    okButton.setEnabled(true);

	// ModalDialog will take care of ESC and anything else that
	// is standard, so call its keyPressed method.
	super.keyPressed(e);
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
    public String showDialogAndGetValue(Component comp) {
	// Set this dialog on top of the component passed in.
	setLocationRelativeTo(comp);

	// Show the dialog and wait for the results. (Blocking call)
	setVisible(true);

	// If I don't do this, and the user hits a 'return' on the keyboard
	// the dialog comes visible again.
	transferFocus();

	// Pass on the results.	 'null' for Cancel or Help button.
	return(userText);
    }
}
