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

public class LocatorHistory {
    /** List of StatementHistory */
    private HashMap statementHistory;
    /** Active ObjType to retrieve statementHistory from HashMap */
    private String activeObjType;
    private String prevActiveObjType;
    private ShufDBManager dbManager;
    private HashMap allMenuStrings;
    private String[] allObjTypes;
    private ArrayList allStatementTypes;
    private ArrayList bufferList=null;
    private ArrayList bufPointerList;
    private ArrayList objTypeList;
    /** This is the part of the .xml file name following "locator_statements_".
	It is used here for naming the persistence file. */
    private String    key;
    // Save date stamp of locator_statement_xxx at startup.  Save it in
    // the persistence file when it is saved.  At startup, if date has
    // changed, do not read the persistence file.
    private long      locatorStatementsTimeSaved;


    /************************************************** <pre>
     * constructor.
     *
     * 
     * 
     * 
     * 
     </pre> **************************************************/
    public LocatorHistory(String filepath, String key) {
	ArrayList  statementGroupList;
	StatementHistory history;
 	ShufflerService shufServ;
	ArrayList statementDefinitionList;
	String groupType;
	Vector buffer=null;
	int    bufPointer=0;
	String objType;

	this.key = key;

	dbManager = ShufDBManager.getdbManager();
	statementHistory = new HashMap(20);

	// Connect to the DB
	dbManager.checkDBConnection();

	// Fill statementDefinitionList array of ArrayLists.
	statementDefinitionList = readStatementDefinitionFile(filepath);


	activeObjType = allObjTypes[0];
	// We need to set prevActiveObjType to something legal.
	prevActiveObjType = activeObjType;

	
	shufServ = new ShufflerService(statementDefinitionList, 
				       allObjTypes[0], allMenuStrings, 
				       allObjTypes);
	readPersistence(shufServ);


	if(bufferList != null) {
	    // Create an array of all objTypes we got from the persistence
	    // file.
	    String[] persistObjTypes = new String[bufferList.size()];

	    for(int i=0;  i < bufferList.size(); i++ ) {

		buffer = (Vector)bufferList.get(i);
		bufPointer = ((Integer)bufPointerList.get(i)).intValue();
		objType = (String)objTypeList.get(i);
		persistObjTypes[i] = objType;
		// Create a ShufflerService with this objType to be put into
		// the StatementHistory.
		shufServ = new ShufflerService(statementDefinitionList, 
					       objType, allMenuStrings, 
					       allObjTypes);

		history = new StatementHistory(shufServ, objType,
					       buffer, bufPointer);

		// Put histories in a HashMap with the ObjType as the key.
		statementHistory.put(objType, history);
	    }

	    // Now, did we get any new objTypes from readStatementDefinitionFile
	    // that were not in the persistence file.  If so, we need to add
	    // them in.
	    // Compare the two lists of objTypes.
	    boolean foundit;
	    for(int i=0; i < allObjTypes.length; i++) {
		foundit = false;
		for(int k=0; k < persistObjTypes.length; k++) {
		    if(allObjTypes[i].equals(persistObjTypes[k]))
			foundit = true;
		}
		// If this one not found, add it in.
		if(!foundit) {
		    // Create a ShufflerService with this objType to be put into
		    // the StatementHistory.
		    shufServ = new ShufflerService(statementDefinitionList, 
						allObjTypes[i], allMenuStrings, 
						allObjTypes);

		    history = new StatementHistory(shufServ, allObjTypes[i], 
						   null, bufPointer);

		    // Put histories in a HashMap with the ObjType as the key.
		    statementHistory.put(allObjTypes[i], history);
		}
	    }
	}
	else {
	    for(int i=0;  i < allObjTypes.length; i++ ) {
		// Create a ShufflerService with this objType to be put into
		// the StatementHistory.
		shufServ = new ShufflerService(statementDefinitionList, 
					       allObjTypes[i], allMenuStrings, 
					       allObjTypes);

		history = new StatementHistory(shufServ, allObjTypes[i], 
					       buffer, bufPointer);

		// Put histories in a HashMap with the ObjType as the key.
		statementHistory.put(allObjTypes[i], history);
	    }
	}	
    }

    public StatementHistory getStatementHistory() {
	StatementHistory sh;
	sh = (StatementHistory)statementHistory.get(activeObjType);
	return sh;
    }

    public StatementHistory getStatementHistory(String objType) {
	StatementHistory sh;
	sh = (StatementHistory)statementHistory.get(objType);
	return sh;
    }

    /**
     * get active ShuffleService
     * @return Active ShufflerService from active StatementHistory
     */
    public ShufflerService getShufflerService() {
	StatementHistory hist = getStatementHistory();
	if(hist == null)
	    return null;
	else
	    return hist.getShufflerService();
    }

    public void setActiveObjType(String objType) {
        setActiveObjType(objType, true);
    }

    public void setActiveObjType(String objType, boolean clearReturnStack) {
        // If no change, do nothing.
        if (!objType.equals(activeObjType)) {
            // Do not set prev to Trash
            if (!activeObjType.equals(Shuf.DB_TRASH))
                prevActiveObjType = activeObjType;
            activeObjType = objType;
        }

        // Be sure standard locator buttons are showing
        Shuffler shuffler = Util.getShuffler();
        // Don't do this if the locator is not up
        if (shuffler != null) {
            ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
            shufflerToolBar.showStdButtons(clearReturnStack);
        }

    }

    public String getActiveObjType() {
        return activeObjType;
    }

    public String getPrevActiveObjType() {
	return prevActiveObjType;
    }


    public void addAllStatementListeners(StatementListener listener) {
	StatementHistory history;
	// Loop through all StatementHistory's and add this listener to them.

	// Get keys in as a Set
	Set keys = statementHistory.keySet();
	// Convert to String array for access.
	String[] keysArr = new String[keys.size()];
	keys.toArray(keysArr);
	for(int i=0; i < keysArr.length; i++) {
	    history = (StatementHistory)statementHistory.get(keysArr[i]);
	    history.addStatementListener(listener);
	}
    }


    /***
     * <pre>
     * Read and parse the shuffler statement definition <b>XML</b>file.
     *
     * <b>XML file syntax for </b>
     *
     * &lt ?xml version="1.0" encoding="UTF-8" standalone="yes"?&gt
     * &lt shuffler &gt 
     *   &lt Statement &gt 
     *      &lt <i>element</i> /&gt 
     *      ....
     *   &lt /Statement&gt
     *   &lt Statement &gt
     *      &lt <i>element</i> /&gt 
     *      ....
     *   &lt /Statement&gt
     * &lt /shuffler&gt
     *
     *  where: 
     * <i>element</i>          {@link vnmr.ui.shuf.StatementElement StatementElement} expression
     *                     <i>type value="string" [display="false"]</i>
     *			 
     * <i>type</i>             {@link vnmr.ui.shuf.StatementElement StatementElement} type
     *     ObjectType      basic statement type (required) with value = 
     *                     vnmr_data, workspace, panels_n_components or macro
     *     MenuString      string to display in spotter menu
     *     Label           string item in statement (plain text)
     *     Attribute-1     pulldown menu in statement with attribute to use
     *                     for search.  If the item is displayed, this
     *                     value is only a default.
     *     AttrValue-1     pulldown menu in statement with value of attribute
     *                     to use for search.  If the item is displayed, this
     *                     value is only a default.
     *     Attribute-2     pulldown menu in statement with attribute to use
     *                     for search.  If the item is displayed, this
     *                     value is only a default.
     *     AttrValue-2     pulldown menu in statement with value of attribute
     *                     to use for search. If the item is displayed, this
     *                     value is only a default.
     *     UserType        pulldown menu.  There are three values required
     *                     with $ separating values.
     *                     first - which user type this is 
     *                             (owner, operator, operator_)
     *                     second - default value
     *                     third - exclusive or nonexclusive
     *                             Set to exclusive to see ONLY the files for
     *                             the person or group chosen.
     *     Date            date item with value = "time_run", "time_started",
     *                     "time_imported", "time_saved", "time_archived", or
     *                     "time_published"
     *     Calendar        calendar type with value = "on all dates", "since",
     *                     "before" or "between"
     *     Columns         column header list where value = 3 "$" separated
     *                     attributes (eg. "solvent$owner$tn")
     *     Sort            sorting attribute where the first value is the
     *                     attribute to sort on and the second is the direction
     *                     of the sort (DESC or ASC).  Thus the value entry
     *                     might look like "exptype$DESC".
     *			 
     *			 
     * <i>value</i>               string value assigned to StatementElement
     *                     e.g. UserType value="owner$everyone$nonexclusive"
     *			 
     * <i>display</i>             Optional display string. 
     *                     if display="false", StatementElement value does not
     *                     appear in the statement. If absent or display="true"
     *                     text is displayed.
     *                             
     * <b>XML file syntax (example)</b>
     *
     * &lt ?xml version="1.0" encoding="UTF-8" standalone="yes"?&gt
     * 
     * &lt shuffler &gt
     *  &lt Statement &gt
     *    &lt ObjectType value="vnmr_data" display="false" /&gt 
     *    &lt MenuString value="of this nucleus and solvent" display="false" /&gt 
     *    &lt Label value="Show" /&gt 
     *    &lt Attribute-1 value="tn" display="false" /&gt 
     *    &lt Label value="nucleus"/&gt
     *    &lt AttrValue-1 value="H1" /&gt 
     *    &lt Label value="and" /&gt 
     *    &lt Attribute-2 value="solvent" /&gt 
     *    &lt AttrValue-2 value="CDCl3" /&gt 
     *    &lt Label value="Run By" /&gt 
     *    &lt UserType value="owner$everyone$nonexclusive" /&gt 
     *    &lt Label value="with" /&gt 
     *    &lt Date value="time_run" /&gt 
     *    &lt Calendar value="on all dates" /&gt 
     *    &lt Columns display="false" value="tn$owner$solvent" /&gt 
     *    &lt Sort display="false" value="tn$DESC" /&gt 
     *  &lt /Statement&gt
     *
     *</pre>
     */
    public ArrayList readStatementDefinitionFile(String filepath) {
	StatementDefinition statementDefinition;
	StatementElement stElem, stElem2;
	ArrayList eList;
	ArrayList  statementDefinitionList;
	
	statementDefinitionList = new ArrayList();
	allStatementTypes = new ArrayList();



	try {
	     ShufflerStatementBuilder.build(statementDefinitionList, filepath);
	}
	catch(java.lang.Exception e) {
	    // Why did we fail?  That is, does the file not exist,
	    // or is it incorrect.
	    File file = new File(filepath);
	    if(file.exists()) {
		Messages.postMessage(Messages.OTHER|Messages.WARN|Messages.MBOX|Messages.LOG,
                                     "The file \n'" + filepath +
                                     "  seems to have some problem." +
                                     "  Continuing with a single statement.");
	    }
	    else {
		Messages.postMessage(Messages.OTHER|Messages.WARN|Messages.MBOX|Messages.LOG,
                                     "You should have a file \n    '" + 
                                     filepath + "  to specify a variety of" +
                                     "\n    shuffler statements." +
                                     "  Continuing with a single statement.");
	    }
	    // Hard code in one generic statement so vnmrj will function
	    // even though the file is missing.

	    // Init eList for the Statement
	    eList  = new ArrayList();

	    // Forget the previous statementDefinitionList in case it
	    // has partial info from an aborted read.
	    statementDefinitionList = new ArrayList();

	    stElem = new StatementElement("ObjectType",
					  Shuf.DB_VNMR_DATA,false);
	    eList.add(stElem);
	    // Be sure to set MenuString's display to false
	    stElem = new StatementElement("MenuString",
					  "Generic Statement",false);
	    eList.add(stElem);
	    stElem = new StatementElement("Label","Show",true);
	    eList.add(stElem);
	    stElem = new StatementElement("Attribute-1","pslabel",true);
	    eList.add(stElem);
	    stElem = new StatementElement("AttrValue-1","all",true);
	    eList.add(stElem);
	    stElem = new StatementElement("Label","and",true);
	    eList.add(stElem);
	    stElem = new StatementElement("Attribute-2","solvent",true);
	    eList.add(stElem);
	    stElem = new StatementElement("AttrValue-2","all",true);
	    eList.add(stElem);
	    stElem = new StatementElement("Label","Run By",true);
	    eList.add(stElem);
	    stElem = new StatementElement("UserType",
					  "owner$everyone$nonexclusive",true);
	    eList.add(stElem);
	    stElem = new StatementElement("Label","with",true);
	    eList.add(stElem);
	    stElem = new StatementElement("Date","time_run",true);
	    eList.add(stElem);
	    stElem = new StatementElement("Calendar", DateRangePanel.ALL,true);
	    eList.add(stElem);

	    stElem = new StatementElement("Columns",
					  "pslabel$owner$solvent",false);
	    eList.add(stElem);
	    stElem = new StatementElement("Sort","solvent$DESC",false);
	    eList.add(stElem);
	

	    // Create the StatementDefinition for this statement
 	    try { 
		statementDefinition = new StatementDefinition(eList);
	    }
	    catch (Exception ex) {
                // StatementDefinition outputs its own errors, so don't
                // output anything here.
		return statementDefinitionList;
	    }
	    statementDefinitionList.add(statementDefinition);
	}

	// Be sure each StatementDefinition has necessary elements.


	// Fill in the member variables, allObjTypes and allMenuStrings
	StatementDefinition sd;
	HashArrayList objtypeList = new HashArrayList();
	ArrayList al;
	String objType;
	String menuString;
	// We need to go through the StatementDefinition's and accumulate
	// the menuStrings for each objType.  We need N arrayLists of
	// menuStrings, one for each objType.  Keep the array Lists in
	// a Hashtable for easy access by objType.
	for(int i=0; i < statementDefinitionList.size(); i++) {
	    sd = (StatementDefinition) statementDefinitionList.get(i);
	    // We need all of the ObjTypes, without duplicating.  Is this
	    // ObjType already in the hashtable?
	    objType = sd.getobjType();

            // If this user does not have the right to open protocols in
            // the locator, do not put them in the spotter menu.  This can
            // be used as a way to limit some users to only the protocols
            // listed in the experiment selector.
//             if(objType.equals("protocol")) {
//                 // Does this operator have the right to see all protocols?
//                 // ***** TBD  How to get the real right??? ******/
//                 String right = "false";

//                 if(right.equals("false")) {
//                     statementDefinitionList.remove(i);
//                     // The i-- is because removing an element from an
//                     // ArrayList, drops all higher elements down in number.
//                     // Thus the next element is at the same value of i that
//                     // we just retrieved.
//                     i--;
//                     continue;
//                 }
//             }

	    if(!objtypeList.containsKey(objType)){
		// No, so add it along with an empty arrayList
		al = new ArrayList();
		objtypeList.put(objType, al);
	    }
	    // Now add the menuString to the arrayList for this objType
	    al = (ArrayList) objtypeList.get(objType);
	    menuString = sd.getmenuString();

            al.add(menuString);
            // Keep list of statementType's in the form objType/menu_string
            allStatementTypes.add(objType + "/" + menuString);
	}

	allMenuStrings = new HashMap();
	allObjTypes = new String[objtypeList.size()];

	for(int i=0; i < objtypeList.size() ; i++) {
	    // Convert entries in objtypeList to String[]
	    allObjTypes[i] = (String)objtypeList.getKey(i);

	    // Each item in the allMenuStrings Hashtable is a String[] 
	    // for that ObjType.  Get the ArrayList for this objType.
	    al = (ArrayList) objtypeList.get(i);
	    // Convert to String[] and fill hashtable.
	    String[] sa = new String[1]; // Dummy variable to specify type
	    String[] strarr = (String[]) al.toArray(sa);
	    // allMenuStrings is a hashtable with objType as key and
	    // an array of menu strings as the value.
	    allMenuStrings.put(allObjTypes[i], strarr);

	    
	}

	return statementDefinitionList;
    }



    /************************************************** <pre>
     * Summary: Write out the persistence file for All StatementHistory's
     *
     </pre> **************************************************/

    public void writePersistence() {
	String filepath;
	ObjectOutput out;
	ArrayList bufferList;
	ArrayList bufPointerList;
	ArrayList objTypeList;
	HistoryPersistence historyPersistence;
	StatementHistory history;

	// If locator is off via debug, then do not write out this file.
        // If we do, then running vnmrj with the locator back on will not work.
        if(ShufDBManager.locatorOff())
            return;
    
	// Assemble the histories and bufPointers for each objType into 
	// a single object for writing out to a persistence file.
	// We need ArrayList's for the buffer's and the bufPointer's
	bufferList = new ArrayList(20);
	bufPointerList = new ArrayList(20);
	objTypeList = new ArrayList(20);

	Set keys = statementHistory.keySet();
	// Convert to String array for access.
	String[] keysArr = new String[keys.size()];
	keys.toArray(keysArr);
	for(int i=0; i < keysArr.length; i++) {
	    history = (StatementHistory)statementHistory.get(keysArr[i]);
	    bufferList.add(history.getBuffer());
	    // Convert int to Integer to put into ArrayList
	    bufPointerList.add(new Integer(history.getBufPointer()));
	    objTypeList.add(keysArr[i]);
	}

	historyPersistence = new HistoryPersistence(bufferList, bufPointerList,
						    objTypeList, activeObjType,
                                                    locatorStatementsTimeSaved);

	// Create the filepath and prepare to write.

	filepath = FileUtil.savePath("USER/PERSISTENCE/"+"LocatorHistory_"+key);

	try {
            File file = new File(filepath);
	    out = new ObjectOutputStream(new FileOutputStream(file));
	    // Write it out.
	    out.writeObject(historyPersistence);
            out.close();
	}
	catch (Exception e) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(e);
	}
	
    }

    /************************************************** <pre>
     * Summary: Read in the persistence file.
     *
     </pre> **************************************************/
    public void readPersistence(ShufflerService shufflerService) {
	String filepath;
	ObjectInputStream in;
	String dir;
	Integer bufP;
	HistoryPersistence historyPersistence;
	Vector buffer;
	int bufPointer;
	File file;
	
        // If the locator_statement_key file has changed since we read it
        // last, then do not use the persistence file.
        // Get the date stamp.  Use it now, and also save it for use
        // in writePersistence later.
        filepath = FileUtil.openPath("LOCATOR/locator_statements_" + key + 
                                     ".xml");
        if(filepath != null) {
            try {
                file = new File(filepath);
                locatorStatementsTimeSaved = file.lastModified();
            }
            catch (Exception e) {  }
        }


	filepath = FileUtil.openPath("USER/PERSISTENCE/"+"LocatorHistory_"+key);
        if(filepath==null)
            return;
            
	try {
	    in = new ObjectInputStream(new FileInputStream(filepath));
	    // Read it in.
	    historyPersistence = (HistoryPersistence) in.readObject();
            in.close();
	}
	catch (Exception e) {
	    // No error output here.  The file was probably not found
	    // or DashO stuff causes this sometimes, so just continue 
	    // with no history.
	    return;
	}
	
        // If locator_statements_xxx file has changed, bail out
        if(historyPersistence.locatorStatementsTimeSaved != 
           locatorStatementsTimeSaved) {
            // File has changed, bail out
            return;
        }

	// Fill LocatorHistory members with info from historyPersistence
	bufferList = historyPersistence.bufferList;
	bufPointerList = historyPersistence.bufPointerList;
	objTypeList = historyPersistence.objTypeList;
	activeObjType = historyPersistence.activeObjectType;

	if(activeObjType == null && objTypeList != null) {
	    activeObjType = (String)objTypeList.get(0);
        }
        // If the user had double clicked on a record and was display
        // rec_data with the return icon, we want to return him to
        // rec_data at startup.  Else, the icon will be missing.
        if(activeObjType.equals(Shuf.DB_VNMR_REC_DATA))
            activeObjType = Shuf.DB_VNMR_RECORD;

	// Go thru the statements and check to see that the statementType
	// is valid.  If not, remove it.
	// While in here,  check to see if a statement of type activeObjType
	// exists.  If not, reset activeObjType
	Hashtable statement;
	String statementType;
	StatementDefinition sd;
	String objType;
	String firstObjType=null;
	boolean foundObjType=false;

	for(int k=0; k < bufferList.size(); k++) {
	    buffer = (Vector)bufferList.get(k);
	    bufPointer = ((Integer)bufPointerList.get(k)).intValue();

	    for(int i=0; buffer != null && i < buffer.size(); i++) {
		statement = (Hashtable)buffer.elementAt(i);
		// Get the statementType from this statement
		statementType = (String)statement.get("Statement_type");
		String objtype = (String)statement.get("ObjectType");
		if(firstObjType == null)
		    firstObjType = objtype;
		if(objtype.equals(activeObjType)) {
		    // We found the active object type, set the flag
		    foundObjType = true;
		}
		sd = shufflerService.getAStatement(statementType);

		if(sd == null) {
		    buffer.removeElementAt(i);

		    // In case this objType is now gone, we need to be sure
		    // activeObjType is not set to it.
		    activeObjType = allObjTypes[0];
		    prevActiveObjType = activeObjType;
		    bufPointer = 0;
		    
		    // Since we removed one from the Vector, we need to 
		    // decrement i so that we don't miss one.
		    i--;
		}
	    }

	    // Try to at least keep it valid.
	    if(buffer != null && bufPointer > buffer.size() -1)
		bufPointer = buffer.size() -1;
            if(buffer == null || buffer.size() == 0)
                bufPointer = -1;
	    if(bufPointer < -1)
		bufPointer = -1;

	}
	if(!foundObjType) {
	    // The active object type was not in the statements.  Set
	    // activeObjType to the first one we encountered.  At least
	    // it should be valid.
	    activeObjType = allObjTypes[0];
	}

    }

    public ArrayList getallObjTypes() {
	ArrayList outList = new ArrayList();
	for(int i=0; i < allObjTypes.length; i++) {
	    outList.add(allObjTypes[i]);
	}
	return outList;
    }

    public ArrayList getallStatementTypes() {
	return allStatementTypes;
    }


    public String getKey() {
	return key;
    }

    public void setHistoryToThisType(String objType) {
        // call setHistoryToThisType() with clearReturnStack true.
        setHistoryToThisType(objType, true);
    }

    public void setHistoryToThisType(String objType, boolean clearReturnStack) {
        // Set History Active Object type to this type.
        setActiveObjType(objType, clearReturnStack);

        SessionShare sshare = ResultTable.getSshare();
        // Now get history for this type.
        StatementHistory history = sshare.statementHistory();

        if(history == null) {
            Messages.postError("Category " + objType +
                           ", does not exist in locator_statements_xxx file");
            // Replace the previous type
            String type = getPrevActiveObjType();
            setActiveObjType(type, clearReturnStack);
        }
        else {
            // Update locator to the most recent statement for this type.
            // Disallow statements ending in 'internal use'.  They are
            // for temp internal use and should not be gone to as a default
            // history. This does not apply to DB_AVAIL_SUB_TYPES
            if(objType.equals(Shuf.DB_AVAIL_SUB_TYPES))
                history.updateWithoutNewHistory();
            else
                history.updateWithoutNewHistoryNotInternal();
        }
    }

}


/** This is a container to hold histories of all object types in one place
    for the purpose of saving in a persistence file. */
class HistoryPersistence implements Serializable {
    public ArrayList bufferList;      // List of Vector objects
    public ArrayList bufPointerList;  // List of Integer objects
    public ArrayList objTypeList;     // List of String objects
    public String activeObjectType;
    // date stamp of locator_statements file
    public long locatorStatementsTimeSaved=0;     

    public HistoryPersistence(ArrayList bufferList, ArrayList bufPointerList,
			      ArrayList objTypeList, String activeObjectType,
                              long timeSaved) {
	this.bufferList = bufferList;
	this.bufPointerList = bufPointerList;
	this.activeObjectType = activeObjectType;
	this.objTypeList = objTypeList;
        this.locatorStatementsTimeSaved = timeSaved;
        
    }

}
