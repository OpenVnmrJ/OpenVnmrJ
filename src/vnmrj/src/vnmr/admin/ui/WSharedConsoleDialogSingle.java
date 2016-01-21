/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import vnmr.util.*;

import javax.swing.*;
import java.awt.*;
import java.beans.*;
import java.awt.event.*;





public class WSharedConsoleDialogSingle extends ModalDialog implements ActionListener,
                                         PropertyChangeListener {
    /** Text input Field */
    protected JPasswordField pwInput1=null;
    /** Text from User */
    protected String userText1=null;
    /* Panel to hold the three standard buttons. */
    protected JPanel panelForText;
    /* Message */
    protected JLabel messageText1=null;
    protected JLabel statusLabel;
    protected final String ENABLED= Util.getLabel("_Enabled");
    protected final String DISABLED=Util.getLabel("_Disabled");
    protected final String ENABLE=Util.getLabel("_Enable");
    protected final String DISABLE=Util.getLabel("_Disable");
    protected final String STATUS=Util.getLabel("_Status");
    // Return strings from dtsharcntrl.sh
    protected final String SHARING_ENABLED="Sharing Enabled";
    protected final String SHARING_DISABLED="Sharing Disabled";
    

    public WSharedConsoleDialogSingle(String strHelpFile) {
        super(Util.getLabel("_Console_Display_Sharing"));

        // Make a panel for the text items
        GridLayoutCol mgr = new GridLayoutCol(0, 1, 10, 10);
        panelForText = new JPanel(mgr);
        // It looks better with a border
        panelForText.setBorder(BorderFactory.createEmptyBorder(20,20,20,20));

        // Create the two items.
        messageText1 = new JLabel (Util.getLabel("_Enter_Sharing_Password"));
        pwInput1 = new  JPasswordField(20);

        // *Warning, working around a Java problem*
        // When we went to the T3500 running Redhat 5.3, the JPasswordField
        // fields sometimes does not allow ANY entry of characters.  Setting
        // the enableInputMethods() to true fixed this problem.  There are
        // comments that indicate that this could cause the typed characters
        // to be visible.  I have not found that to be a problem.
        // This may not be required in the future, or could cause characters
        // to become visible in the future if Java changes it's code.
        pwInput1.enableInputMethods(true);

        statusLabel = new JLabel("Status: "
               + "                                                                 ");

        // Add items to panel
        panelForText.add(messageText1);
        panelForText.add(pwInput1);
        panelForText.add(statusLabel);

        // Add the panel top of the dialog.
        getContentPane().add(panelForText, BorderLayout.NORTH);

        // Set the buttons and the text item up with Listeners
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        okButton.setText(ENABLE);
        cancelButton.setActionCommand("cancel");
        cancelButton.addActionListener(this);
        cancelButton.setText("Close");
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
        pwInput1.setActionCommand("text");
        pwInput1.addActionListener(this);
        pwInput1.addKeyListener(this);

        // Start OK disabled until something has been entered.
        okButton.setEnabled(false);

        DisplayOptions.addChangeListener(this);

        // Make the frame fit its contents.
        pack();

        setDisplayMode();
        setBgColor(Util.getBgColor());
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        setBgColor(Util.getBgColor());
    }

    protected void setBgColor(Color bgColor)
    {
        panelForText.setBackground(bgColor);
        messageText1.setBackground(bgColor);
        pwInput1.setBackground(bgColor);
        setBackgroundColor(bgColor);
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
            // Which Button did they click (enable or disable)?
            String button = getButtonStatus();
            if(button.equals(ENABLE)) {
                userText1 = new String(pwInput1.getPassword());
                // Clear the password field after use
                pwInput1.setText("");
                if(userText1.length() < 6 && userText1.length() > 0) {
                    statusLabel.setText(Util.getLabel("_Password_Length") + " 6");
                    // Clear the field
                    pwInput1.setText("");
                    // Set focus to the password field
                    resetFocus();
                    return;
                }
                // Be sure we are in a disable status before trying to enable
                String status = getStatus();
                String output;
                if(status.equals(ENABLED)) {
                    // Something is screwed up.  We should not have had an enable
                    // button if we are already enabled.  If it is already enabled, then we
                    // might as well just fix the panel and log the event.
                    Messages.postLog("WSharedConsoleDialog found mismatch between status and button"); 
                    // Set proper display mode
                    setDisplayMode();
                    return;
                }
                else {
                    // Execute the command to enable.
                    output = WSharedConsoleCommands.enable(userText1);
                }

                // If the script is successful, it should send back a status
                // starting with "Sharing".  If there is a problem, it will
                // sent a description of the problem, so we need to post an error
                if(!output.startsWith("Sharing")) {
                    Messages.postError("Problem Enabling Console Sharing: "
                        + output);
                }
                if(output.startsWith(SHARING_ENABLED))
                    statusLabel.setText(STATUS + ": "  + ENABLED);
                else
                    statusLabel.setText(STATUS + ": "  + DISABLED);
            }
            else {  // DISABLE button
                // Execute the command to disable
                String output = WSharedConsoleCommands.disable();
                // If the script is successful, it should send back a status
                // starting with "Sharing".  If there is a problem, it will
                // sent a description of the problem, so we need to post an error
                // If the X server needs to be restarted, the msg with start with
                // "Log".  Catch that one also
                if(output.startsWith("Log")) {
                    Messages.postError(Util.getLabel("_Important") + ": "
                                       + output);
                    // Put it into the status field
                    statusLabel.setText(Util.getLabel("_Important") + ": " + output);
                }
                else if(!output.startsWith("Sharing")) {
                    Messages.postError("Problem Disabling Console Sharing: "
                        + output);
                    // Put it into the status field
                    statusLabel.setText("Problem: " + output);
                }
                else {
                    // Put it into the status field
                    if(output.startsWith(SHARING_ENABLED))
                        statusLabel.setText(STATUS + ": "  + ENABLED);
                    else
                        statusLabel.setText(STATUS + ": "  + DISABLED);
                }
            }
            // Recheck the status and set the panel up accordingly
            setDisplayMode();
        }
        // Cancel
        else if(cmd.equals("cancel")) {
            userText1 = null;
            pwInput1.setText("");
            setVisible(false);
        }
        // Help
        else if(cmd.equals("help")) {
            // Do not call setVisible(false);  That will cause
            // the Block to release and the code which create
            // this object will try to use userText.  This way
            // the panel stays up and the Block stays in effect.
            userText1 = null;
            pwInput1.setText("");
            displayHelp();
        }
        // Text Field
        else if(cmd.equals("text")) {
            // Have a return do the same as clicking ok.
            okButton.doClick();
        }
    }

    public void setDisplayMode() {
        String status;
        
        status = getStatus();
        if(status.equals(ENABLED))
            setDisableMode();
        else
            setEnableMode();
    }



    public String getStatus() {
        String status;

        // Determine current status
        String output = WSharedConsoleCommands.status();
        if(output.contains("active"))
            status = ENABLED;
        else
            status = DISABLED;

        return status;

    }


    public void setDisableMode() {
        okButton.setText(DISABLE);
        okButton.setEnabled(true);
        messageText1.setVisible(false);
        pwInput1.setVisible(false);
        statusLabel.setText(STATUS + ": "  + ENABLED);
        
    
    }
    
    public void setEnableMode() {
        okButton.setText(ENABLE);
        okButton.setEnabled(true);

        messageText1.setVisible(true);
        pwInput1.setVisible(true);
        pwInput1.setEnabled(true);
        statusLabel.setText(STATUS + ": "  + DISABLED);

   }

    public String getButtonStatus() {
        String text = okButton.getText();
        
        return text;
    }
    
    
    //Must be called from the event dispatch thread.
    protected void resetFocus() {
        pwInput1.requestFocusInWindow();
    }

}
