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
import javax.swing.*;
import java.util.*;
import java.io.*;
import  vnmr.bo.*;
import  vnmr.ui.*;
import  vnmr.util.*;
import javax.swing.border.*;

/**
 * The ShufflerToolBar displays the following buttons: back, forward,
 * import, tag, and custom shuffle.
 *
 * @author Mark Cao
 */
public class ShufflerToolBar extends JComponent {
    /** back button */
    private JButton backButton;
    /** forward button */
    private JButton forwardButton;
    /** Tag button */
    private TagButton tagButton;
    /** Return Button */
    private JButton returnButton;
    /** Restore Button for Trash Mode */
    private JButton restoreButton=null;
    /** Exit Button for Trash Mode */
    private JButton exitButton;
    /** Delete Button for Trash Mode */
    private JButton deleteButton;
    /** Empty Button for Trash Mode */
    private JButton emptyButton;
    /** Panel for control buttons */
    private JPanel rightPanel;
    /** Panel for forward and back buttons */
    private JPanel leftPanel;
    /** FlowLayout for right panel */
    private FlowLayout flowLayout=null;
    /** GridLayout for right panel */
    private GridLayout gridLayout=null;
    /** Stack of ReturnInfo for return icon use. */
    private Stack returnStack;

    /**
     * constructor
     * @param sshare session share
     */
    public ShufflerToolBar(SessionShare sshare) {

	setLayout(new BorderLayout());
        returnStack = new Stack();

	leftPanel = new JPanel();
	leftPanel.setOpaque(false);

        //	backButton = new JButton(Util.getImageIcon("back.gif"));
	backButton = new JButton(Util.getImageIcon("open_arrow_left.png"));
	backButton.setDisabledIcon(Util.getImageIcon("open_arrow_left_gray.png"));
	backButton.setBorder(BorderFactory.createEmptyBorder());
	backButton.setContentAreaFilled(false);
	backButton.setToolTipText(Util.getTooltipString("Previous Statement"));
	leftPanel.add(backButton);

        //	forwardButton = new JButton(Util.getImageIcon("forward.gif"));
	forwardButton = new JButton(Util.getImageIcon("open_arrow_right.png"));
	forwardButton.setDisabledIcon(Util.getImageIcon("open_arrow_right_gray.png"));
	forwardButton.setBorder(BorderFactory.createEmptyBorder());
	forwardButton.setContentAreaFilled(false);
	forwardButton.setToolTipText(Util.getTooltipString("Next Statement"));
	leftPanel.add(forwardButton);

        // Create programmable buttons and add to panel
        createProgrammableBtns(leftPanel);

	add(leftPanel, BorderLayout.WEST);

	rightPanel = new JPanel();
	rightPanel.setOpaque(false);

	tagButton = new TagButton();

	add(rightPanel, BorderLayout.EAST);

	// Install behaviors for
	// back and forward buttons.
	LocatorHistoryList lhl = sshare.getLocatorHistoryList();
	if(lhl!=null)
		lhl.addAllStatementListeners(new StatementAdapter() {
	    public void backMovabilityChanged(boolean state) {                
                backButton.setEnabled(state);
	    }
	    public void forwardMovabilityChanged(boolean state) {
            forwardButton.setEnabled(state);
	    }
	});

	backButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		SessionShare sshare = ResultTable.getSshare();
		StatementHistory history = sshare.statementHistory();
		history.goBack();
	    }
	});
	forwardButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		SessionShare sshare = ResultTable.getSshare();
		StatementHistory history = sshare.statementHistory();
		history.goForward();
	    }
	});

        // Create the return button for use with records.
        returnButton = new JButton(Util.getImageIcon("return.gif"));
	returnButton.setBorder(BorderFactory.createEmptyBorder());
        returnButton.setContentAreaFilled(false);
        returnButton.setToolTipText(Util.getTooltipString("Return to Previous Spotter Type"));
	returnButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
                Hashtable returnStatement;
                StatementHistory his;
                // Default in case something goes wrong
		SessionShare sshare = ResultTable.getSshare();

                if(!returnStack.empty()) {
                    // Get the statement to return to
                    ReturnInfo ri = (ReturnInfo)returnStack.pop();
                    returnStatement = ri.statement;

                    // Get the objType from this statement
                    String type = (String) returnStatement.get("ObjectType");

                    // If the type we are returning to is avail_sub_types,
                    // then we need to set the correct types into the DB
                    if(type.equals(Shuf.DB_AVAIL_SUB_TYPES)) {
                        // Unfortunately, we do not know what the sub type
                        // are avail for this level because they are determined
                        // by the level one higher than this.  So, we have saved
                        // the needed information in the ReturnInfo for the
                        // level above this, and we need to get it now and
                        // use it.  The stack has already been pop'ed, so do 
                        // this, we will just peek at the stack.
                        ReturnInfo ri2 = (ReturnInfo)returnStack.peek();
                        if(ri2 != null) {
                            ArrayList subTypeList = ri2.subTypeList;

                            // Put values into the DB
                            ShufDBManager dbManager;
                            dbManager = ShufDBManager.getdbManager();
                            dbManager.setAvailSubTypes(subTypeList);
                        }
                    }

                    LocatorHistory lh = sshare.getLocatorHistory();
                    // Set History Active Object type to this type.
                    lh.setActiveObjType(type, false);

                    // We need the correct history for the new type we 
                    // are going to
                    his = sshare.statementHistory(type);

                }
                else {
                    // We need the correct history for the new type we 
                    // are going to, default to this same type.
                    his = sshare.statementHistory();
                    returnStatement = his.getCurrentStatement();
                }


                LocatorHistory lh = sshare.getLocatorHistory();
                // Update locator to the most recent statement for Record
                his.append(returnStatement);
                if(!returnStack.empty()) {
                    returnButton.setVisible(true);
                }
                else
                    hideReturnButton();
	    }
	});

        // Add the standard icon buttons to the panel.
        showStdButtons();

    } // ShufflerToolBar()

    public void showReturnButton(String returnType) {
        showReturnButton(returnType, new ArrayList());
    }

    public void showReturnButton(String returnType, ArrayList typeList) {
        returnButton.setVisible(true);

        // Put the return statement on the stack for returning to
        if(returnType != null) {
            SessionShare sshare = ResultTable.getSshare();
            LocatorHistory lh = sshare.getLocatorHistory();
            StatementHistory his = lh.getStatementHistory(returnType);
            Hashtable returnStatement = his.getCurrentStatement();

            // Without clone, we just push a pointer and the actual
            // statement is changed as we change the locator.  This way
            // we keep this statement.
            if(returnStatement != null) {
                Hashtable statement = (Hashtable) returnStatement.clone();
                ReturnInfo returnInfo;
                returnInfo = new ReturnInfo(statement, typeList);
                returnStack.push(returnInfo);
            }
        }
    }

    public void hideReturnButton() {
        returnButton.setVisible(false);
    }

    /**************************************************
     * Summary: Set visibility false for the trash mode buttons, and
     *          true for the standard locator buttons.
     *
     *
     **************************************************/
    public void showStdButtons(boolean returnStackVal) {

        // Remove any trash buttons that may have been added
        rightPanel.removeAll();
        leftPanel.setVisible(true);

        SessionShare sshare = ResultTable.getSshare();
        SpotterButton spotterButton = sshare.getSpotterButton();
        spotterButton.setVisible(true);

        if(flowLayout == null)
            flowLayout = new FlowLayout(FlowLayout.RIGHT);
        rightPanel.setLayout(flowLayout);

        rightPanel.add(returnButton);
// I believe we are going to remove the tag.  I have never heard of anyone
// using it.  Initially, we were going to move the function to the main
// panel menu, but with the locator now being available as a standalone,
// that does not make any sense.  Just in case some customer uses this
// feature and wants it, I will put in a backdoor so that it can be turned
// on for individuals without wanting for another release cycle.
// Make the flag be a file of the name displayTagIcon in the users vnmrsys dir
        String dir = FileUtil.usrdir();
        UNFile file = new UNFile(dir + File.separator + "displayTagIcon");
        if(file.exists())
            rightPanel.add(tagButton);

        if(returnStackVal) {
            returnStack.clear();
            hideReturnButton();
        }
    }

    /**************************************************
     * Summary: Set visibility false for the trash mode buttons, and
     *          true for the standard locator buttons.  Clear the
     *          returnStack.
     *
     *
     **************************************************/
    public void showStdButtons() {
        boolean returnStackVal = true;
        showStdButtons(returnStackVal);
    }


    /**************************************************
     * Summary: Set visibility false for the standard locator buttons and
     *          true for the trash mode buttons.
     *
     *
     **************************************************/
    public void showTrashButtons() {

        // remove the standard buttons
        rightPanel.removeAll();
        leftPanel.setVisible(false);
        SessionShare sshare = ResultTable.getSshare();
        SpotterButton spotterButton = sshare.getSpotterButton();
        spotterButton.setVisible(false);

        // Set layout for two rows
        if(gridLayout == null) {
            gridLayout = new GridLayout(2,2);
        }
        rightPanel.setLayout(gridLayout);

        if(restoreButton == null) {
            createTrashButtons();
        }

        // add the trash mode buttons
        rightPanel.add(restoreButton);
        rightPanel.add(exitButton);
        rightPanel.add(deleteButton);
        rightPanel.add(emptyButton);




    }

    private void createTrashButtons() {

        restoreButton = new JButton("Restore Items");
        // restoreButton.setContentAreaFilled(false);
        restoreButton.setToolTipText(Util.getTooltipString("Restore to Original Location"));
	restoreButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
                // Restore the selected items back to their original location
                // and remove from the trash.
                String hostFullpath;
                String mpath;
                int    index;
                // Delete the selected items from the trash.
                // Get the selected items.
                ResultTable resultTable = Shuffler.getresultTable();
                ArrayList selectionList = resultTable.getSelectionList();

                if(selectionList.size() == 0)
                    return;

                // Go thru the list of hostFullpath strings and delete each one.
                for(int k=0; k < selectionList.size(); k++){
                    hostFullpath = (String) selectionList.get(k);
                    // Get the mount path from the hostFullpath
                    mpath = MountPaths.getMountPath(hostFullpath);

                    // If Windows, we need a windows path for TrashItem.restore
                    if(UtilB.OSNAME.startsWith("Windows"))
                        mpath = UtilB.unixPathToWindows(mpath);

                    // Restore this item.  Need to send mpath as arg
                    if(TrashItem.restore(mpath)) {
                        // delete it and remove from locator trash
                        TrashItem.delete(hostFullpath);
                        // Update the browser
                        ExpPanel.updateBrowser();
                    }
                }
                // Cause another shuffle so that these items disappear.
                SessionShare sshare = ResultTable.getSshare();
                sshare.statementHistory().updateWithoutNewHistory();


	    }
	});

        exitButton = new JButton("Exit Trash Mode");
        // exitButton.setContentAreaFilled(false);
        exitButton.setToolTipText(Util.getTooltipString("Exit Trash Mode"));
	exitButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
                // Show the standard buttons
                showStdButtons();
                // Go back to the previous Locator type
                SessionShare sshare = ResultTable.getSshare();
                LocatorHistoryList lhl = sshare.getLocatorHistoryList();
                lhl.setLocatorHistoryToPrev();
	    }
	});

        deleteButton = new JButton("Delete Items");
        // deleteButton.setContentAreaFilled(false);
        deleteButton.setToolTipText(Util.getTooltipString("Delete Selected Items"));
	deleteButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
                String hostFullpath;
                String fullpath;
                int    index;
                // Delete the selected items from the trash.
                // Get the selected items.
                ResultTable resultTable = Shuffler.getresultTable();
                ArrayList selectionList = resultTable.getSelectionList();

                if(selectionList.size() == 0)
                    return;

                // Go thru the list of hostFullpath strings and delete each one.
                for(int k=0; k < selectionList.size(); k++){
                    hostFullpath = (String) selectionList.get(k);

                    // delete it and remove from locator
                    TrashItem.delete(hostFullpath);
                }
                // Cause another shuffle so that these items disappear.
                SessionShare sshare = ResultTable.getSshare();
                sshare.statementHistory().updateWithoutNewHistory();
	    }
	});

        emptyButton = new JButton("Empty My Trash");
        // emptyButton.setContentAreaFilled(false);
        emptyButton.setToolTipText(Util.getTooltipString("Empty My Trash Now"));
	emptyButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
                String hostFullpath;
                String fullpath;
                int    index;
                ID id;
                ResultTable resultTable = Shuffler.getresultTable();
                ResultDataModel rdm = resultTable.getdataModel();

                // Delete all of the items in the trash locator now.
                int numRows = rdm.getRowCount();

                for(int i=0; i < numRows; i++) {
                    id = (ID) rdm.getID(i);
                    hostFullpath = id.getName();

                    // delete it and remove from locator
                    TrashItem.delete(hostFullpath);
                }
                // Cause another shuffle so that these items disappear.
                SessionShare sshare = ResultTable.getSshare();
                sshare.statementHistory().updateWithoutNewHistory();
	    }
	});

    }


    // Create and setup the programmable directory buttons
    // Put them into the panel arg
    public void createProgrammableBtns(JPanel panel) {
        String strVC=null;
        String strSetVC=null;
        int numBtns = 3;
        Border  m_raisedBorder =  BorderFactory.createRaisedBevelBorder();

        for(int i=0; i < numBtns; i++) {
            // Button which can save the current statement and go back to it.
            ExpPanel exp=Util.getDefaultExp();
            SessionShare sshare = ResultTable.getSshare();
            VToolBarButton btn = new VToolBarButton(sshare, exp, "S" + i);

            // Vnmr cmd to execute when button is pressed
            strVC = "vnmrjcmd(\'LOC loadstatement loc" + i + "\')";
            // Vnmr cmd to execute when button is pressed and held 3 sec
            strSetVC = "vnmrjcmd(\'LOC addstatement loc" + i 
                + "\')";

            // Name to use for a label
            String strLabel="S" + (i +1);
            String strToolTip=Util.getLabel("_Press_and_Hold") + " " + Util.getLabel("_Statement");

            btn.setAttribute(VObjDef.ICON, strLabel);
            btn.setAttribute(VObjDef.TOOL_TIP, strToolTip);
            btn.setAttribute(VObjDef.CMD, strVC);
            btn.setAttribute(VObjDef.SET_VC, strSetVC);
            btn.setBorder(m_raisedBorder);

            // Add this button to the panel
            panel.add(btn);
        }
    }






    class ReturnInfo {
        protected Hashtable statement;
        protected ArrayList subTypeList;

        public ReturnInfo(Hashtable statement, ArrayList subTypeList) {
            this.statement = statement;
            this.subTypeList = subTypeList;
        }
    }

} // class ShufflerToolBar
