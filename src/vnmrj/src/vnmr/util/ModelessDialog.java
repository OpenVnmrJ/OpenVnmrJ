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
import java.beans.*;
import java.util.*;

import vnmr.ui.*;



/********************************************************** <pre>
 * Summary: Generic Modeless Dialog Frame with 5 buttons across the bottom.
 *
 *   What is this:
 *  This is a generic Modal Dialog Base Class which has five buttons
 *  across the bottom as standard fixtures. They are left to right
 *  History, Undo, Close, Abandon, Help.  The text for these buttons
 *  is obtained from Labels.properties (blHistory, blUndo, blClose,
 *  blAbandon, and blHelp).  Mnemonics can be assigned
 *  to these buttons also in the Labels.properties file (blmHistory,
 *  blmUndo, blmClose, blmAbandon, and blmHelp).
 *  Escape does an Abandon and Enter defaults to Close.
 *   How to use it:
 *  - Create an extended class which extends this base class.
 *    The first line of the constructor must be:
 *      super(title);
 *
 *  - Create and assign ActionListener's for the five buttons
 *    in ModelessDialog or implement ActionListener (see ImportDialog.java)
 *    For example:
 *         helpButton.addActionListener(new ActionListener() {
 *         public void actionPerformed(ActionEvent evt) {
 *             System.out.println("Help Not Implemented Yet");
 *         }
 *         });
 *
 *  - Create and assign a PopListener with a popHappened() method,
 *    for the historyButton and add with historyButton.addPopListener()
 *    This popHappened() needs to look at its arg (a String) as in
 *    ImportDialog.HistoryButtonListener.popHappened().
 *  - Create a panel and add desired items for the panel
 *  - Add the new panel to the ModalDialog's content pane, For example
 *         getContentPane().add(myPanel, BorderLayout.NORTH);
 *         pack();
 *
 *  - Since I keep using the ImportDialog as an example I will include
 *    the code from ShufflerToolBar.java which brings up the ImportDialog
 *    then the Import button is clicked.  This creates an instance of
 *    the dialog if it does not exist.  It uniconifies, shows it
 *    and positions it.  Everything else is taken care of with
 *    ActionListener's and the PopListener in your extended class.
 *
 *      importButton.addActionListener(new ActionListener() {
 *      public void actionPerformed(ActionEvent evt) {
 *          // Get the location of the button clicked.
 *          Point pt = importButton.getLocationOnScreen();
 *
 *          // We need to create one dialog and keep it around.
 *          // That is a way to be able to uniconify or bring
 *          // to the surface an existing window, just always have
 *          // an existing window after the first call.
 *          if(importDialog == null)
 *             importDialog = new ImportDialog("Import Dialog");
 *          else {
 *          // Uniconify if necessary
 *          importDialog.setState(Frame.NORMAL);
 *          }
 *          // Position over the button so that the cursor is in
 *          // the panel.
 *          importDialog.setLocation(new Point(
 *                    (int)pt.getX() -10, (int)pt.getY() -25));
 *          // Bring to top or show
 *          importDialog.setVisible(true);
 *      }
 *      });
 *
 *
 </pre> **********************************************************/
public class ModelessDialog extends JDialog implements KeyListener,
        PropertyChangeListener, VPopupIF {
    protected MPopButton historyButton;
    protected JButton undoButton;
    protected JButton closeButton;
    protected JButton abandonButton;
    protected JButton helpButton;
    protected JPanel buttonPane;
    protected String dialogTitle;
    protected boolean bOnTopSet = true;
    
    // This help method is obsolete.
    protected String m_strHelpFile=null;

    public ModelessDialog(Frame owner, String title) {
        super(owner, title, false);
        dialogTitle = title;
        initUi();
    }

    public ModelessDialog(Frame owner, String title, String helpFile) {
        super(owner, title, false);
        dialogTitle = title;
        initUi();
        m_strHelpFile = helpFile;
    }

    public ModelessDialog(String title) {
        super(VNMRFrame.getVNMRFrame(), title, false);
        dialogTitle = title;
        initUi();
    }

    public ModelessDialog(String title, String helpFile) {
        super(VNMRFrame.getVNMRFrame(), title, false);
        dialogTitle = title;
        initUi();
        m_strHelpFile = helpFile;
    }

    private void initUi() {
        String history;
        String undo;
        String close;
        String abandon;
        String help;
        String string;
        char helpMnemonic;
        char historyMnemonic;
        char undoMnemonic;
        char closeMnemonic;
        char abandonMnemonic;

        DisplayOptions.addChangeListener(this);

        // setAlwaysOnTop(true);

        // Get text for buttons from properties/resource file
        history = Util.getLabel("blHistory", "Edit...");
        undo = Util.getLabel("blUndo", "Undo");
        close = Util.getLabel("blClose", "Close");
        abandon = Util.getLabel("blAbandon", "Abandon");
        help = Util.getLabel("blHelp", "Help");

        // buttons
        undoButton = new JButton(undo);
        closeButton = new JButton(close);
        abandonButton = new JButton(abandon);
        helpButton = new JButton(help);

        // Create an ArrayList of menu items from properties file

        ArrayList<String> historyList = new ArrayList<String>();
        historyList.add(Util.getLabel("mlHistReturnInitState",
                "Return to initial state"));
        historyList.add(Util.getLabel("mlHistMakeSnapshot", "Make a snapshot"));
        historyList.add(Util.getLabel("mlHistReturnToSnapshot",
                "Return to snapshot"));
        historyList.add(Util.getLabel("mlHistReturnToDefault",
                "Return to system defaults"));

        // Pop Button for history menu
        historyButton = new MPopButton(historyList);
        historyButton.setText(history);

        // Only set mnemonics if found.

        if(Util.labelExists("blmHelp")) {
            string = Util.getLabel("blmHelp");
            helpMnemonic = string.charAt(0);
            helpButton.setMnemonic(helpMnemonic);
        }

        if(Util.labelExists("blmUndo")) {
            string = Util.getLabel("blmUndo");
            undoMnemonic = string.charAt(0);
            undoButton.setMnemonic(undoMnemonic);
        }

        if(Util.labelExists("blmAbandon")) {
            string = Util.getLabel("blmAbandon");
            abandonMnemonic = string.charAt(0);
            abandonButton.setMnemonic(abandonMnemonic);
        }

        if(Util.labelExists("blmClose")) {
            string = Util.getLabel("blmClose");
            closeMnemonic = string.charAt(0);
            closeButton.setMnemonic(closeMnemonic);
        }

        if(Util.labelExists("blmHistory")) {
            string = Util.getLabel("blmHistory");
            historyMnemonic = string.charAt(0);
            historyButton.setMnemonic(historyMnemonic);
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
        buttonPane.setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER));

        // Add the buttons to the panel with space between buttons.
        buttonPane.add(historyButton);
        // buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(undoButton);
        // buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(closeButton);
        // buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(abandonButton);
        // buttonPane.add(Box.createRigidArea(new Dimension(5, 0)));
        buttonPane.add(helpButton);

        //Put everything together, using the content pane's BorderLayout.
        Container contentPane = getContentPane();
        contentPane.add(buttonPane, BorderLayout.SOUTH);

        setHistoryEnabled(false);
        // setCloseEnabled(false);
        setAbandonEnabled(false);
        setUndoEnabled(false);
        
        if(!CSH_Util.haveTopic(dialogTitle))
            setHelpEnabled(false);

        // buttonPane.setVisible(false);
        // Add key listener to the whole dialog
        addKeyListener(this);
        // Make the frame fit its contents.
        //pack(); // nothing to pack.
    }

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);
    }

    public void setVisible(boolean bShow) {
        if (bShow) {
            if(CSH_Util.haveTopic(dialogTitle))
                setHelpEnabled(true);
            else
                setHelpEnabled(false);
            
            enableControlPanel();
        }
        VPopupManager.addRemovePopup(this, bShow);
        super.setVisible(bShow);
    }

    protected void setBgColor(Color colorbg) {
        //super.setBackground(colorbg);
        //setBgColor(this.getComponents(), colorbg);
    }

    public int getState() {
        // for backward compatilibility
        // since it is extended from JDialog, it can't be iconified so return
        // normal.
        return Frame.NORMAL;
    }

    public void setState(int i) {
        // for backward compatibility, used to be extended from JFrame.
    }

    public void setIconImage(Image image) {
        // for backward compatibility, used to be extended from JFrame.
    }

    protected JPanel getButtonPane() {
        return buttonPane;
    }

    protected JButton getAbndnBtn() {
        return abandonButton;
    }

    protected JButton getCloseBtn() {
        return closeButton;
    }

    protected JButton getHelpBtn() {
        return helpButton;
    }

    protected JButton getHistoryBtn() {
        return historyButton;
    }

    protected JButton getUndoBtn() {
        return undoButton;
    }

    public void setCloseEnabled(boolean b) {
        closeButton.setEnabled(b);
        closeButton.setVisible(b);
        enableControlPanel();
    }

    public void setHelpEnabled(boolean b) {
        helpButton.setEnabled(b);
        helpButton.setVisible(b);
        enableControlPanel();
    }

    public void setAbandonEnabled(boolean b) {
        abandonButton.setEnabled(b);
        abandonButton.setVisible(b);
        enableControlPanel();
    }

    public void setHistoryEnabled(boolean b) {
        historyButton.setEnabled(b);
        historyButton.setVisible(b);
        enableControlPanel();
    }

    public void setUndoEnabled(boolean b) {
        undoButton.setEnabled(b);
        undoButton.setVisible(b);
        enableControlPanel();
    }

    public void setCloseAction(String cmd, ActionListener listener) {
        closeButton.setActionCommand(cmd);
        closeButton.addActionListener(listener);
    }

    public void setHelpAction(String cmd, ActionListener listener) {
        helpButton.setActionCommand(cmd);
        helpButton.addActionListener(listener);
    }

    public void setAbandonAction(String cmd, ActionListener listener) {
        abandonButton.setActionCommand(cmd);
        abandonButton.addActionListener(listener);
    }

    public void setHistoryAction(String cmd, ActionListener listener) {
        historyButton.setActionCommand(cmd);
        historyButton.addActionListener(listener);
    }

    public void setUndoAction(String cmd, ActionListener listener) {
        undoButton.setActionCommand(cmd);
        undoButton.addActionListener(listener);
    }

    /************************************************** <pre>
     * Summary: not used here but required for interface.
     *
     *
     </pre> **************************************************/
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
     </pre> **************************************************/
    public void keyPressed(KeyEvent e) {
        // Have an escape do the same as clicking cancel.
        if (e.getKeyCode() == KeyEvent.VK_ESCAPE)
            abandonButton.doClick();
        // Return
        else if (e.getKeyCode() == KeyEvent.VK_ENTER) {
            if (closeButton.isEnabled()) {
                closeButton.doClick();
            }
        }
    }

    public void enableControlPanel() {
         boolean bVisible = false;

         int nmembers = buttonPane.getComponentCount();
         for (int k = 0; k < nmembers; k++) {
             Component comp = buttonPane.getComponent(k);
             if (comp != null) {
                  if (comp.isVisible() || comp.isEnabled()) {
                     bVisible = true;
                     break;
                  }
             }
         }

         if (bVisible && !buttonPane.isVisible()) {
             Dimension dim = getSize();
             Dimension dim1 = buttonPane.getPreferredSize();
             int w = dim.width;
             int h = dim.height + dim1.height;
             if (dim1.width > w)
                 w = dim1.width;
             if (w < 300)
                 w = 300;
             if (h < 200)
                 h = 200;
             setSize(w, h);
         }
         buttonPane.setVisible(bVisible);
    }
    
    public void displayHelp() {
        CSH_Util.displayCSHelp(dialogTitle);
    }

    public boolean isOnTopSet() {
        return bOnTopSet;
    }

    public void setOnTop(boolean b) {
        bOnTopSet = b;
    }
}
