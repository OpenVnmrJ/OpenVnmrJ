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

import  vnmr.bo.*;
import  vnmr.ui.*;
import  vnmr.util.*;

/**
 * Tag button
 *
 */
public class TagButton extends PopButton implements Serializable {
    // ==== instance variables
    /** Contains the two previous tags for each objType.  The keys are
        the objType followed by "1" and "2" for the two values for each. */
    Hashtable previousTags;

    static final String NEW = "New";
    static final String EDIT = "Edit";
    static boolean persistenceFilesRead=false;
    ModalEntryDialog med=null;
    TagAddRemoveDialog dialog=null;
    TagEditDialog dial=null;
    /** The dialog type for the current TagEditDialog. */
    String DialogObjType=null;
    

    /**
     * constructor
     */
    public TagButton() {

	setIcon(Util.getImageIcon("tag.gif"));
	setContentAreaFilled(false);
	setUnderline(false);
	setToolTipText(Util.getLabel("_Locator_Add_Remove"));

	/* Get persistance from file */
	readPersistence();

	// Set Listener for when user clicks the tags icon.
	// This will bring up the popup menu.
	addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		 refreshMenu();
	    }
	});

	// Set Listener to internal Class for when the user selects
	// an item from the menu.
	addPopListener(new TagButtonListener());

    }





    /************************************************** <pre>
     * Summary: Clear out and refill the popup menu.
     *
     </pre> **************************************************/
    private void refreshMenu() {
	JMenuItem mitem;
	ArrayList tagList;
	ArrayList selectionList;
	String objType;
	String tag;
	ResultTable resultTable;
	int match;

	ImageIcon checkBlackIcon = Util.getImageIcon("checkblack.gif");
	ImageIcon checkGrayIcon = Util.getImageIcon("checkgray.gif");
	ImageIcon checkBlankIcon = Util.getImageIcon("checkblank.gif");

	// Need objType
	SessionShare sshare = ResultTable.getSshare();
	StatementHistory history = sshare.statementHistory();
	Hashtable statement = history.getCurrentStatement();
	objType = (String)statement.get("ObjectType");
	Color bgColor = Util.getBgColor();

 	// first, delete what's already there
	popup.removeAll();

	// Make a title of sorts
	JMenuItem objMItem = new JMenuItem("Locator Groups");
	objMItem.setBackground(bgColor);
	popup.add(objMItem);

	// Add a separator
	popup.addSeparator();

	// Get the list of currently selected locator items.
	resultTable = Shuffler.getresultTable();
	selectionList = resultTable.getSelectionList();

	// Get Full List of Tags
	tagList = TagList.getAllTagNames(objType);

	// Put last two tags accessed at top of list
	for(int i=1; i <= 2; i++) {
	    tag = (String) previousTags.get(objType + Integer.toString(i));
	    if(tag != null) {
		match = matchStatus(tag, selectionList, objType);
		mitem = new JMenuItem(tag, checkBlankIcon);
		if(match == 1) {
		    // Matched all
		    mitem = new JMenuItem((String)tag, checkBlackIcon);
		}
		else if (match == 0) {
		    // Matched none
		    mitem = new JMenuItem((String)tag, checkBlankIcon);
		}
		else
		    // Matched some
		    mitem = new JMenuItem((String)tag, checkGrayIcon);

		mitem.setActionCommand(objType + "/" + tag);
		mitem.addActionListener(popActionListener);
		mitem.setBackground(bgColor);
		popup.add(mitem);
	    }
	}

	// Add a separator
	popup.addSeparator();

	// Go thru each tag and get the list of locator entries with that tag.
	// Create the menu list
	for(int i=0; i < tagList.size(); i++) {
	    tag = (String)tagList.get(i);
	    if(tag.length() == 0)
		continue;
	    match = matchStatus(tag, selectionList, objType);
	    if(match == 1)
		// Matched all
		mitem = new JMenuItem(tag, checkBlackIcon);
	    else if (match == 0)
		// Matched none
		mitem = new JMenuItem(tag, checkBlankIcon);
	    else
		// Matched some
		mitem = new JMenuItem(tag, checkGrayIcon);

	    mitem.setActionCommand(objType+ "/" + tag);
	    mitem.addActionListener(popActionListener);
	    mitem.setBackground(bgColor);
	    popup.add(mitem);
	}


	// Add a separator
	popup.addSeparator();

	// Add New
	mitem = new JMenuItem(NEW);
	mitem.setActionCommand(objType+ "/" + NEW);
	mitem.addActionListener(popActionListener);
	mitem.setBackground(bgColor);
	popup.add(mitem);

 	// Add Edit
	mitem = new JMenuItem(EDIT);
	mitem.setActionCommand(objType+ "/" + EDIT);
	mitem.addActionListener(popActionListener);
	mitem.setBackground(bgColor);
	popup.add(mitem);
    }


    /************************************************** <pre>
     * Summary: Return status of matched between items in a tag and selected
     *          items.
     *
     *	   Return: 0 for no matches
     *		   1 for all selected items have this tag.
     *		  -1 for some matches
     </pre> **************************************************/

    private int matchStatus(String tag, ArrayList selectionList,
			    String objType) {
	ArrayList itemsWithThisTag;
	int numMatched=0;

	if(selectionList.size() == 0)
	    return 0;

	ShufDBManager dbMg = ShufDBManager.getdbManager();

	itemsWithThisTag = dbMg.getAllEntriesWithThisTag(objType, tag);
	// Check itemsWithThisTag against the selectionList of currently
	// highlighted items.
	numMatched = 0;
	for(int k=0; itemsWithThisTag != null && k < selectionList.size(); k++){
	    if(itemsWithThisTag.contains(selectionList.get(k))) {
		numMatched++;
	    }
	}
	// Matched none
	if(numMatched == 0)
	    return 0;
	// Matched all
	else if(numMatched == selectionList.size())
	    return 1;
	// Matched some
	else
	    return -1;
    }



    /************************************************** <pre>
     * Summary: Write out the persistence file.
     *
     </pre> **************************************************/

    public void writePersistence() {
	String filepath;
	ObjectOutput out;
	//String dir, perDir;
	//File file;

	//dir = System.getProperty("userdir");
	//perDir = new String(dir + "/persistence");
	//file = new File(perDir);
	// If this directory does not exist, make it.
	//if(!file.exists()) {
	//    file.mkdir();
	//}

	//filepath = new String (perDir + "/GroupMenu");

	filepath = FileUtil.savePath("USER/PERSISTENCE/GroupMenu");

	try {
	    out = new ObjectOutputStream(new FileOutputStream(filepath));
	    // Write it out.
	    out.writeObject(previousTags);
            out.close();
	}
	catch (Exception ioe) {
            Messages.postError("Problem writing filepath");
            Messages.writeStackTrace(ioe, "Error caught in writePersistence");
	}
    }


    /************************************************** <pre>
     * Summary: Read in the persistence file.
     *
     </pre> **************************************************/
    public void readPersistence() {
	String filepath;
	ObjectInputStream in;
	//String dir;

	//dir = System.getProperty("userdir");

	//filepath = new String (dir + "/persistence/GroupMenu");
	filepath = FileUtil.savePath("USER/PERSISTENCE/GroupMenu");

	try {
	    in = new ObjectInputStream(new FileInputStream(filepath));
	    // Read it in.
	    previousTags = (Hashtable) in.readObject();
            in.close();
	}
	catch (ClassNotFoundException ce) {
            Messages.postError("Problem reading filepath");
            Messages.writeStackTrace(ce, "Error caught in readPersistence");
	}
	catch (Exception e) {
	    // No error output here.
	}

	if(previousTags == null)
	    previousTags = new Hashtable();
    }

    /************************************************** <pre>
     * Summary: Set this tag in the previous tag list if not duplicate.
     *
     </pre> **************************************************/

    public void setPrevious(String objType, String tag) {
	if(tag == null || tag.length() == 0) {
	    return;
	}

	// Check for duplicate.  Get the 2 previous tags
	String tag1 = (String) previousTags.get(objType + "1");
	String tag2 = (String) previousTags.get(objType + "2");

	if((tag1 == null || !tag.equals(tag1)) &&
	   (tag2 == null || !tag.equals(tag2))) {
	    // Set 1 to the current tag
	    Object lastTag = previousTags.put(objType + "1", tag);
	    // Set 2 to the previous value of 1
	    if(lastTag != null)
		previousTags.put(objType + "2", lastTag);
	}

    }

    /********************************************************** <pre>
     * Summary: Inner Class Listener for selection of item from Tag menu
     *
     *    Called when a selection is made from the Tag menu
     </pre> **********************************************************/

    public class TagButtonListener implements PopListener {

	public void popHappened(String popStr) {
	    ShufDBManager dbManager = ShufDBManager.getdbManager();
	    String result;
            boolean status=true;

	    // put down the menu popup
	    TagButton.this.popup.setVisible(false);

	    // The locator's JTable is not updated correctly when the
	    // popup is unshown.  This seems to get around that java bug.
	    // by forcing it to be updated.
	    ResultTable resultTable = Shuffler.getresultTable();
	    resultTable.repaint();
	    refreshMenu();

	    // Update previousTags if not New nor Edit
	    if(!popStr.endsWith("/" + NEW) &&
	       !popStr.endsWith("/" + EDIT)) {
		// Get the tag name following "/"
		int index = popStr.indexOf("/");
		String tag = popStr.substring(index +1);
		// Get the objType
		String objType = popStr.substring(0, index);

		setPrevious(objType, tag);

		// Get the list of currently selected locator items.
		ArrayList selectionList = resultTable.getSelectionList();
		int match = matchStatus(tag, selectionList, objType);
		if(match == 1) {
		    // Matched all, remove this group

		    // Ask if they really want to remove this tag.
		    if(dialog == null)
			dialog = new TagAddRemoveDialog();
		    // We don't need the Add button
		    dialog.disableAdd();
		    dialog.enableRemove();
		    result = dialog.showDialog(TagButton.this);
		    if(result != null && result.equals("remove")) {
			// Remove these items from the tag group
			// Or really, remove this tag from all items.
			dbManager.deleteThisTag(objType, tag);
		    }
		    dialog.setVisible(false);
		}
		else if (match == 0) {
		    // Matched none, Add this group

		    // Ask if they really want to add these files.
		    if(dialog == null)
			dialog = new TagAddRemoveDialog();
		    // We don't need the Remove button
		    dialog.disableRemove();
		    dialog.enableAdd();
		    result = dialog.showDialog(TagButton.this);
		    if(result != null && result.equals("add")) {
			// Add these items to the tag group
			// Or really, add this tag to each item.
			status = dbManager.setTagInAllSelections(objType, tag);
                        if(status == true)
                            // Add this tag to the allTagNames list
                            TagList.addToAllTagNames(objType, tag);
		    }
		    dialog.setVisible(false);
		}
		else {

		    // Matched some, Ask what to do next.
		    if(dialog == null)
			dialog = new TagAddRemoveDialog();
		    dialog.enableAdd();
		    dialog.enableRemove();
		    result = dialog.showDialog(TagButton.this);
		    if(result != null && result.equals("add")) {
			status = dbManager.setTagInAllSelections(objType, tag);
                        if(status == true)
                            // Add this tag to the allTagNames list
                            TagList.addToAllTagNames(objType, tag);
		    }
		    else if(result != null && result.equals("remove")) {
			dbManager.deleteTagFromAllSelections(objType, tag);
		    }
		    dialog.setVisible(false);

		}
	    }
	    else {
		// Get the objType
		int index = popStr.indexOf("/");
		String objType = popStr.substring(0, index);

		if(popStr.endsWith("/" + NEW)) {
		    String tagName;

		    // Create one ModalEntryDialog and reuse it.
		    if(med == null) 
			med = new ModalEntryDialog(
		     		Util.getLabel("_Locator_New_Group"),
		     		Util.getLabel("_Locator_New_Group_Name_Entry"));

		    tagName =  med.showDialogAndGetValue(TagButton.this);

		    if(tagName != null  && tagName.length() != 0) {
			status = dbManager.setTagInAllSelections(objType, 
                                                                 tagName);
                        if(status == true) {
                            // Add this tag to the allTagNames list
                            TagList.addToAllTagNames(objType, tagName);
                            setPrevious(objType, tagName);
                            // Set this tag value into LocAttr
                            dbManager.attrList.addTagValue(objType, tagName);

                            // Update the menus
                            SessionShare sshare = ResultTable.getSshare();
                            StatementHistory his = sshare.statementHistory();
                            Hashtable statement = his.getCurrentStatement();
                            StatementDefinition curStatement;
                            curStatement = sshare.getCurrentStatementType();
                            curStatement.updateValues(statement, true);

                        }
		    }
		    // Update statement menus
		    StatementDefinition curStatement;
		    SessionShare sshare = ResultTable.getSshare();
		    StatementHistory history = sshare.statementHistory();
		    Hashtable statement = history.getCurrentStatement();
		    curStatement = sshare.getCurrentStatementType();
		    curStatement.updateValues(statement, true);


		}
		else if(popStr.endsWith("/" + EDIT)) {
                    // Get the current objType and compare with the one
                    // for the existing dialog.  If it has changed, create
                    // a new dialog.
                    SessionShare sshare = ResultTable.getSshare();
                    StatementHistory history = sshare.statementHistory();
                    Hashtable statement = history.getCurrentStatement();
                    String objT = (String)statement.get("ObjectType");
                    
                    if(dial != null) {
                        // Dispose of the old dialog.
                        dial.dispose();
                    }
                    // Create a new one
                    dial = new TagEditDialog(TagButton.this);
                    // Set DialogObjType for testing next time
                    DialogObjType = objT;

                    if(dial != null)
                        dial.showDialog(TagButton.this);

		}
	    }

            if(status == true)
                // Write out the previousTags to a persistence file
                writePersistence();
	}
    }
}
