/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.util.*;
import java.io.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.bo.*;
import vnmr.templates.*;


/********************************************************** <pre>
 * Summary: List of LocatorHistory objects for the purpose of being
 * able to change Spotter menus on the fly.
 *
 * Change to a new menu with the command:
 *   LocatorHistoryList lhl = sshare.getLocatorHistoryList();
 *   lhl.setLocatorHistory(LocatorHistoryList.EDIT_PANEL_LH);
 * Where the argument is the portion of the .xml filename after
 * the string "locator_statements_".
 *
 * Currently, we only deal with Hard coded file names for known
 * purposes.  The code is set up to take all files of the name
 *   "locator_statements_xxx"
 * Where the xxx becomes the key for this menu and associated 
 * LocatorHistory.  Currently there is no way for the user to
 * access these dynamically found menus.  We presumably can just
 * put the list in a menu someplace and let the user add as many
 * possibilities as desired and use them as desired.
 * 
 </pre> **********************************************************/

public class LocatorHistoryList {
    public static final String DEFAULT_LH = "default";
    public static final String PROTOCOLS_LH = "protocols";
    public static final String EDIT_PANEL_LH = "edit_panel";
    public static final String TRASH_LH = "trash";
    public static final String STATE_DEF_FILE = "locator_statements_";
    /** List of LocatorHistory objects */
    HashMap lhList;
    /** Active LocatorHistory */
    String  activeLH=DEFAULT_LH;
    /** Previous LocatorHistory to be used for restoring */
    String  prevLH=null;
    /** locator statements files to be used by the system. */
    public static String[] fileList = {
        "locator_statements_default.xml",
        "locator_statements_edit_panel.xml",
        "locator_statements_protocols.xml",
        "locator_statements_trash.xml"
    };
    
    /** Constructor
     *  parse locator_statement_xxx.xml files in fileList, and create a list
     *  of LocatorHistory's.
     */
    public LocatorHistoryList() {
	ArrayList files;
	UNFile	file;
	String	sysdir;
	String	dirPath;
	String	filepath;
	String	name;
	String	key;
	LocatorHistory lh;
	int	index1;
	int	index2;
	
	// This is the default dir if files are not found
	sysdir = new String(System.getProperty("sysdir"));
	dirPath = new String(sysdir + "/shuffler");

	file = new UNFile(dirPath);
	if (! file.exists() )
	{
    	    FillDBManager.setLocatorOff(true);
	    return;
	}

	lhList = new HashMap();

	files = new ArrayList();
	for(int i=0; i < fileList.length; i++) {
	    filepath = FileUtil.openPath("LOCATOR/"+fileList[i]);
	    if(filepath != null) {
		file = new UNFile(filepath);
		files.add(file);
	    }
            // If the file does not exist, give an error.
            else {
                Messages.postError("A file named " + fileList[i] + "\n needs" +
                           " to exist in /vnmr/shuffler to define locator" +
                           " statements for this locator type");
            }
	}

	if(files.size() == 0) {
	    filepath = dirPath + "/" + STATE_DEF_FILE + DEFAULT_LH + ".xml";
	    // The file presumably does not exist or we would not be here.
	    // Call LocatorHistory's constructor and when it cannot find
	    // the file, it will print out an error message, and create
	    // a single statement LocatorHistory, so things can continue.
	    lh = new LocatorHistory(filepath, DEFAULT_LH);
	    lhList.put(DEFAULT_LH, lh);
	}
	else {
	    // Go thru list of files and create a LocatorHistory for each.
	    // Pass in the xxx portion of the file name for use by
	    // readPersistence().
	    for(int i=0; i < files.size(); i++) {
		file = (UNFile)files.get(i);
		// Is that one of our files?
		name = file.getName();
		if(name.startsWith(STATE_DEF_FILE) && name.endsWith(".xml")) {
		    filepath = file.getAbsolutePath();

		    // For the Hash key, we need the part of the filename
		    // between STATE_DEF_FILE and .xml
		    index1 = name.indexOf(STATE_DEF_FILE);
		    index1 = index1 + STATE_DEF_FILE.length();
		    index2 = name.indexOf(".xml");
		    key = name.substring(index1, index2);

		    lh = new LocatorHistory(filepath, key);

		    //  Put this into the HashMap with the key
		    lhList.put(key, lh);
		}
	    }
	}


	// Did we find at least DEFAULT_LH and EDIT_PANEL_LH?
	// If not, give warning and create minimal statement.
	if(!lhList.containsKey(DEFAULT_LH)) {
	    filepath = dirPath + "/" + STATE_DEF_FILE + DEFAULT_LH + ".xml";
	    // The file presumably does not exist or we would not be here.
	    // Call LocatorHistory's constructor and when it cannot find
	    // the file, it will print out an error message, and create
	    // a single statement LocatorHistory, so things can continue.
	    lh = new LocatorHistory(filepath, DEFAULT_LH);
	    lhList.put(DEFAULT_LH, lh);
	}
	if(!lhList.containsKey(EDIT_PANEL_LH)) {
	    filepath = dirPath + "/" + STATE_DEF_FILE + EDIT_PANEL_LH + ".xml";
	    // The file presumably does not exist or we would not be here.
	    // Call LocatorHistory's constructor and when it cannot find
	    // the file, it will print out an error message, and create
	    // a single statement LocatorHistory, so things can continue.
	    lh = new LocatorHistory(filepath, EDIT_PANEL_LH);
	    lhList.put(EDIT_PANEL_LH, lh);
	}
	if(!lhList.containsKey(PROTOCOLS_LH)) {
	    filepath = dirPath + "/" + STATE_DEF_FILE + PROTOCOLS_LH + ".xml";
	    // The file presumably does not exist or we would not be here.
	    // Call LocatorHistory's constructor and when it cannot find
	    // the file, it will print out an error message, and create
	    // a single statement LocatorHistory, so things can continue.
	    lh = new LocatorHistory(filepath, PROTOCOLS_LH);
	    lhList.put(PROTOCOLS_LH, lh);
	}
    }

    /** Get the currently active LocatorHistory */
    public LocatorHistory getLocatorHistory() {
	if( ! FillDBManager.locatorOff() )
	{
           LocatorHistory lh = (LocatorHistory) lhList.get(activeLH);
	   return lh;
	}
	else
           return null;
    }

    /** Set a new active LocatorHistory.  Update the Spotter Menu accordingly */
    public void setLocatorHistory(String newLH) {
        try {
            // Save current name for restoring if not trash
            if(!activeLH.equals(TRASH_LH))
                prevLH = activeLH;  

            activeLH = newLH;

            // Make a new menu to match this LocatorHistory
            SessionShare sshare = ResultTable.getSshare();
            SpotterButton spotterButton = sshare.getSpotterButton();
            // If the locator is not open, this may not exist yet, just skip
            // updating it now.  When the locator is opened, it will get updated
            if(spotterButton != null)
                spotterButton.updateMenu();
	
            StatementHistory history = sshare.statementHistory();
            if(history != null)
                history.updateWithoutNewHistory();

            if(DebugOutput.isSetFor("LocatorHistory")) {
                Messages.postDebug("Locator set to " + newLH);
            }
        }
        catch (Exception e) {
            Messages.postError("Problem switching locator type to " +
                          newLH + ".  \n    Could the locator_statements_" +
                          newLH + ".xml file be missing from /vnmr/shuffler?");
            Messages.writeStackTrace(e);
        }
    }

    public String getLocatorHistoryName() {
	return activeLH;
    }

    /** Return the list of all strings for LocatorHistory names */
    public Iterator getAllLocatorHistoryNames() {
	Set setOfKeys = lhList.keySet();
	// Return an Iterator object with the keys.
	return setOfKeys.iterator();
    }

    /** Set the argument to be a listener for all LocatorHistory's */
    public void addAllStatementListeners (StatementListener listener) {
	LocatorHistory lh;
	Collection lhCollection = lhList.values();
	Iterator iter = lhCollection.iterator();

	while (iter.hasNext()) {
	    lh = (LocatorHistory)iter.next();
	    lh.addAllStatementListeners(listener);
	}
    }

    /** Set back to the most previous LocatorHistory unless prevLH and
	activeLH are the same.  In that case, you would be trapped.
	Thus, if equal, set to default.
    */
    public void setLocatorHistoryToPrev() {
        // If we are changing FROM trash, we need to fix the buttons
        if(activeLH.equals(TRASH_LH)) {
            // AppIF appIf = Util.getAppIF();
            Shuffler shuffler = Util.getShuffler();
            ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
            // Set locator back to standard mode
            shufflerToolBar.showStdButtons();
        }

	if(prevLH == null || prevLH.equals(activeLH))
	    setLocatorHistory(DEFAULT_LH);
	else
	    setLocatorHistory(prevLH);
    }

    public void writePersistence() {
	LocatorHistory lh;
	Collection lhCollection = lhList.values();
	Iterator iter = lhCollection.iterator();

	while (iter.hasNext()) {
	    lh = (LocatorHistory)iter.next();
	    lh.writePersistence();
	}
    }
 
}
