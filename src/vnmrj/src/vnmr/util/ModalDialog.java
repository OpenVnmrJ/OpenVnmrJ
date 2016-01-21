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
import java.awt.event.*;
import java.util.*;
import java.beans.*;

import vnmr.ui.*;


/********************************************************** <pre>
 * Summary: Generic Modal Dialog Frame with Three buttons at bottom.
 *
 *   What it this:
 *  This is a generic Modal Dialog Base Class which has Three buttons
 *  across the bottom as standard fixtures. They are left to right
 *  OK, Cancel and Help. The text for these buttons is obtained from
 *      Labels.properties (blOk, blCancel and blHelp).  Mnemonics can be
 *  assigned to these buttons also in the Labels.properties file
 *  (blmOk, blmCancel and blmHelp).
 *  This Class extends JDialog and thus is treated much like any
 *  JDialog with the exception of the standard buttons and the fact
 *  that it is fixed as modal.
 *      Escape does a Cancel and Enter does an OK.
 *   How to use it:
 *  - Create an extended class which extends this base class.
 *    The first line of the constructor must be:
 *      super(title);
 *
 *  - Create and assign ActionListener's for the three buttons
 *    in ModalDialog or implement ActionListener (see ModalEntryDialog.java)
 *    For example:
 *         helpButton.addActionListener(new ActionListener() {
 *         public void actionPerformed(ActionEvent evt) {
 *             System.out.println("Help Not Implemented Yet");
 *         }
 *         });
 *
 *  - Create a panel and add desired items for the panel
 *  - Add the new panel to the ModalDialog's content pane, For example
 *         getContentPane().add(myPanel, BorderLayout.NORTH);
 *         pack();
 *  - Now you have one create and filled, but not displayed yet.
 *  - You may want a method in your class to fill in values for the items
 *    in your panel and then to show it.
 *    This method would need at least the following:
 *        setLocationRelativeTo(component);
 *        showDialogWithThread();
 *    Where component is the button clicked to bring up this dialog.
 *    This will position the new dialog over that location so that
 *    the cursor is in the new dialog.
 *    showDialogWithThread() will start a separate thread to bring up
 *    the dialog. All activity from the dialog should be taken care of
 *    in the ActionListener().
 *
 *    If you want it to block and NOT use a separate thread, you
 *    can call setVisible(true) instead of showDialogWithThread();
 *
 *   See Also: ModalEntryDialog for a Modal Dialog which has a
 *         label and one text item for user entry added to this ModalDialog
 *
 </pre> **********************************************************/
public class ModalDialog extends JDialog implements Runnable, KeyListener, PropertyChangeListener, VPopupIF
    {
    public JButton okButton;
    public JButton cancelButton;
    public JButton helpButton;
    protected Thread modalThread;
    protected JPanel buttonPane;
    protected String dialogTitle;
    protected boolean bOnTopSet = false;
    
    // This help method is obsolete.
    protected String m_strHelpFile;

    public ModalDialog(String title) {
        this(VNMRFrame.getVNMRFrame(), title);
        dialogTitle = title;
    }

    public ModalDialog(Frame owner, String title) {
        this(owner, title, "");
        dialogTitle = title;
    }

    public ModalDialog(String title, String helpFile) {
        this(VNMRFrame.getVNMRFrame(), title, helpFile);
        dialogTitle = title;
    }

    public ModalDialog(Frame owner, String title, String helpFile) {
        super(owner, title, true);
        m_strHelpFile = helpFile.trim();
        dialogTitle = title;
        setAlwaysOnTop(true);
        makeDialog();
    }

        private void makeDialog() {

            String ok;
            String cancel;
            String help;
            String string;
            char okMnemonic;
            char cancelMnemonic;
            char helpMnemonic;
        
            DisplayOptions.addChangeListener(this);

            // Get text for buttons from properties/resource file

            ok = Util.getLabel("blOk", "OK");
            cancel = Util.getLabel("blCancel", "Cancel");
            help = Util.getLabel("blHelp", "Help");

            // buttons
            okButton = new JButton(ok);
            cancelButton = new JButton(cancel);
            helpButton = new JButton(help);

            // Only set mnemonics if found.
            if (Util.labelExists("blmOk")) {
                string = Util.getLabel("blmOk");
                okMnemonic = string.charAt(0);
                okButton.setMnemonic(okMnemonic);
            }

            if (Util.labelExists("blmCancel")) {
                string = Util.getLabel("blmCancel");
                cancelMnemonic = string.charAt(0);
                cancelButton.setMnemonic(cancelMnemonic);
            }

            if (Util.labelExists("blmHelp")) {
                string = Util.getLabel("blmHelp");
                helpMnemonic = string.charAt(0);
                helpButton.setMnemonic(helpMnemonic);
            }
        
            helpButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent evt) {
                        // Display Help if help file exists
                        CSH_Util.displayCSHelp(dialogTitle);
                    }
                });

        

        // Make a panel to hold the buttons
        buttonPane = new JPanel();

        // Put an empty border around the inside of the panel.
        buttonPane.setBorder(BorderFactory.createEmptyBorder(0, 5, 10, 5));

        // Add the buttons to the panel with space between buttons.
        buttonPane.add(okButton);
        buttonPane.add(Box.createRigidArea(new Dimension(10, 0)));
        buttonPane.add(cancelButton);
        buttonPane.add(Box.createRigidArea(new Dimension(10, 0)));
        buttonPane.add(helpButton);

        // Put everything together, using the content pane's BorderLayout.
        Container contentPane = getContentPane();
        contentPane.add(buttonPane, BorderLayout.SOUTH);

        // Add key listener to the whole dialog
        addKeyListener(this);

        // Make the frame fit its contents.
        pack();
    }
    
    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);
    }

    public void setVisible(boolean bShow, boolean showHelp) {
        if(showHelp)
            super.setVisible(bShow);

    }
    
    public void setVisible(boolean bShow) {
        if (bShow) {
            boolean helpExists = false;
            String title = this.getTitle();
            // See if help exists for this title
            helpExists = CSH_Util.haveTopic(title);
            
            if (helpExists)
                helpButton.setVisible(true);
            else
                helpButton.setVisible(false);
            

            // Restarting the thread fails with Java 5. I don't believe you are
            // supposed to restart threads anyway. The super.setVisible seems to
            // take
            // care of everything, such that removing these lines allows it to
            // work
            // I will leave this here for now to see what happens.
            // if (modalThread == null)
            // showDialogWithThread();
            // else if (!modalThread.isAlive())
            // modalThread.start();
        }
        VPopupManager.addRemovePopup(this, bShow);
        super.setVisible(bShow);
    }
    
    protected void setBackgroundColor(Color bgColor) {
        /*
        setBackground(bgColor);
        int nCompCount = getComponentCount();
        for (int i = 0; i < nCompCount; i++) {
            Component comp = getComponent(i);
            comp.setBackground(bgColor);
            if (comp instanceof JComponent)
                setBackgroundColor((JComponent) comp, bgColor);
        }
        */
    }
    

    /***************************************************************************
     * ************************************************
     * 
     * <pre>
     *  Summary: not used here but required for interface.
     * 
     * 
     * </pre>
     **************************************************************************/
    public void keyTyped(KeyEvent e) {

    }

    /************************************************** <pre>
     * Summary: not used here but required for interface.
     *
     *
     </pre> **************************************************/
    public void keyReleased(KeyEvent e) {
    }

    /************************************************** <pre>
     * Summary: Catch any key pressed.
     *
     *
     </pre> **************************************************/
    public void keyPressed(KeyEvent e) {
        int keyCode;

        keyCode = e.getKeyCode();

        // Have an escape do the same as clicking cancel.
        if (keyCode == KeyEvent.VK_ESCAPE)
            cancelButton.doClick();
        // If OK button is active, then have return execute OK
        else if (keyCode == KeyEvent.VK_ENTER) {
            if (okButton.isEnabled()) {
                okButton.doClick();
            }
        }
    }
    
    public void displayHelp() {
        CSH_Util.displayCSHelp(dialogTitle);
    }

    /** create modalThread and start the run method.
    Call this method to show the dialog after you have set it up.
    This is not a blocking call.
    If you do not want the dialog to have its own thread,
    then use setVisible(true).  That is a blocking call for this
    type of dialog.
    */
    public void showDialogWithThread() {
        modalThread = new Thread(this);
        modalThread.setName("Modal Dialog");
        modalThread.start();
        if(!CSH_Util.haveTopic(dialogTitle))
            helpButton.setVisible(false);
    }

    /** run method for modalThread. */
    public void run() {
        setVisible(true);
    }

    public boolean isOnTopSet() {
        return bOnTopSet;
    }

    public void setOnTop(boolean b) {
        bOnTopSet = b;
    }

        
}
