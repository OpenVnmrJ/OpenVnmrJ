/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.util.*;
import java.io.*;
import java.text.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;

/**
 * <pre>
 * Utilities for Shuffler.
 *
 * For details on the shuffler statement definition XML file, see
 * {@link vnmr.bo.ShufflerService#readStatementDefinitionFile() 
 * ShufflerService.readStatementDefinitionFile()}
 * </pre>
 * @author  Glenn Sullivan
 **/

public class ShufflerService {
    private ArrayList	  statementDefinitionList;
    /** all Menu Strings for all Object Types keyed by ObjType. Each item 
        in the HashArray is an ArrayList of menuStrings for that ObjType. */
    private HashArrayList allMenuStrings;
    private ShufDBManager dbManager;
    private String objectType;



    /************************************************** <pre>
     * Summary: constructor.
     *
     * Details:
     *   - Create a dbManager which will start the db_manager process.
     *   - Read the Shuffler Statement Config file and fill the
     *     statementDefinitionList with the entries.
     </pre> **************************************************/
    public ShufflerService(ArrayList statDefList, String objType,
			   HashMap allStatementsNames, String[] allObjTypes) {
        String type;
        String[] origNames;
        ArrayList newNames;
        String name;

        allMenuStrings = new HashArrayList();        

	dbManager = ShufDBManager.getdbManager();
	statementDefinitionList = statDefList;
	objectType = objType;

        // allObjTypes will contain all types even though some of them may
        // only contain 'internal use' statements and thus not belong in
        // the spotter menu.  Also, allMenuStrings is a HashArrayList where the
        // keys are objtypes and the values are String[] containing all
        // menuString names including 'internal use' name.  We want to go
        // through the lists and eliminate objTypes which only have
        // 'internal use' menuString, and eliminate menuStrings which only
        // have 'internal use' statements.

        // So, lets first eliminate menuStrings with 'internal use', then
        // we can eliminate objTypes with empty menuStrings lists.
        // Convert to ArrayList of menuStrings just to make it easier to
        // have a list of unknown size.
        for(int i = 0; i < allObjTypes.length; i++) {
            type = allObjTypes[i];
            newNames = new ArrayList();
            origNames = (String[]) allStatementsNames.get(type);
            for(int k = 0; k < origNames.length; k++) {
                name = origNames[k];
                if(!name.endsWith("internal use")) {
                    newNames.add(name);
                }
            }
            if(newNames.size() > 0) 
                allMenuStrings.put(type, newNames);                
        }


    } // ShufflerService()

    /**
     * Get possible values of the given category. Return null
     * if category is invalid.
     * @return array of possible values
     */
    public ArrayList queryCategoryValues(String objType, String category) {
	ArrayList list;


	list = dbManager.attrList.getAttrValueList(category, objType);
        if(list == null) {
            Messages.postError("Problem getting attribute list for "
                                           + category);
        }

	return list;
 
    } // queryCategoryValues()


    public ShufflerItem getShufflerItem(String host_fullpath) {
        ShufflerItem item = new ShufflerItem();
        try {
            SessionShare sshare = ResultTable.getSshare();
            Hashtable searchCache = sshare.searchCache();

            Object[] itemlist = (Object[])searchCache.get(host_fullpath);

            item.lockStatus = ((LockStatus)itemlist[0]).isLocked();
            item.value[0] = new String((String)itemlist[1]);
            item.value[1] = new String((String)itemlist[2]);
            item.value[2] = new String((String)itemlist[3]);
            item.filename = new String((String)itemlist[4]);
            item.dhost =  new String((String)itemlist[7]);
            // setFullpath() uses dhost and sets fullpath and hostFullpath
            item.setFullpath(new String((String)itemlist[5]));
            item.objType = new String((String)itemlist[6]);
            item.owner = new String((String)itemlist[8]);



            // Get the names of the three columns attributes from the current
            // statement.
            StatementHistory history = sshare.statementHistory();
            Hashtable statement = history.getCurrentStatement();
            if(statement == null)
                return null;
            String[] headers = (String[])statement.get("headers");
            item.attrname[0] = headers[0];
            item.attrname[1] = headers[1];
            item.attrname[2] = headers[2];
        }
        catch (Exception e) {
            Messages.postError("Problem creating ShufflerItem");
            Messages.writeStackTrace(e);
        }
	return(item);
       
    }



    /**
     * return all "object types"
     * @return all object types
     */
    public ArrayList getAllMenuObjectTypes() {

        return allMenuStrings.getKeyList();

    } // getAllMenuObjectTypes()

    /**
     * Return the category label for the given object type. 
     * The category label is to be put on the spotter menu.
     * @return category label
     */
    public String getCategoryLabel(String objType) {
        String title= Util.getLabel(objType,objType+" ...");
	    return ("Sort " + title);
    } // getCategoryLabel()

    /**
     * get all menuStrings for a given object type
     * @return all statement types
     */
    public ArrayList getmenuStringsThisObj(String objType) {
	return ((ArrayList) allMenuStrings.get(objType));
    }


    public String getFirstMenuStringThisObj(String objType) {
	ArrayList strings = (ArrayList) allMenuStrings.get(objType);
	if(strings.size() > 0)
	    return (String) strings.get(0);
	else
	    return null;
    }

    /**
     * Return the default statement type.
     *
     * Return the first objType and its first menuString with
     * a "/" between the two.
     * @return statement type (string representation)
     */
    public String getDefaultStatementType() {
	ArrayList strarray = (ArrayList) allMenuStrings.get(objectType);
	return (objectType + "/" + (String) strarray.get(0));
    } // getDefaultStatementType()

    /**
     * Build a default statement, in the form of a hashtable.
     * @return default statement
     */
    public Hashtable getDefaultStatement(String statementType) {
	Hashtable statement = new Hashtable();
	ArrayList  list;
	String[]  headers;

	statement.put("Statement_type", statementType);
	
	// Set a default for all items in this statementType.
	// First get the StatementDefinition for this statementType
	StatementDefinition sd=null;
	boolean foundit=false;
	for(int i=0; i < statementDefinitionList.size(); i++) {
	    sd = (StatementDefinition) statementDefinitionList.get(i);

	    if(statementType.equals(sd.getobjType() + "/" +sd.getmenuString())){
		foundit = true;
		break;
	    }
	}

	if(!foundit || sd == null) {
	   Messages.postError("Statement Type " + statementType + 
                              " Not Found.");
	    return null;
	}

	// Loop thru the StatementDefinition's elementList to get each item.
	ArrayList elemList = sd.getelementList();
	StatementElement elem;
	String[] elemValues;
	String   elemType;
	String[] columns = null;
	String[] sort = null;

	for(int i=0; i < elemList.size(); i++) {
	    elem = (StatementElement) elemList.get(i);
	    // Put all items in the Hashtable.  I don't think there is a 
	    // problem with putting too much in the hashtable.  
	    // Unused items like Label's will just remain unused.
	    elemValues = elem.getelementValues();
	    elemType = elem.getelementType();
	    // UserType has the actual attribute name in elemValues[0] and
	    // the default in elemValues[1].
	    // Put both in the hashtable.
	    if(elemType.equals("UserType")) {
		statement.put(elemType, elemValues[0]);
		statement.put(elemValues[0], elemValues[1]);
	    }
	    else if(elemType.equals("Calendar")) {
		// With no args, the calander will be created with todays date
		statement.put("date-0", new GregorianCalendar());
		statement.put("date-1", new GregorianCalendar());
		statement.put("DateRange", elemValues[0]);
	    }
	    else if(elemType.equals("Columns")) {
		columns = elemValues;  // Save for use below
	    }
	    else if(elemType.equals("Sort"))
		sort = elemValues;  // Save for use below
	    else
		statement.put(elemType, elemValues[0]);
	}

	headers = ResultDataModel.getDefaultHeaderInfo(statementType, 
						       columns, sort);
	statement.put("headers", headers);
	

	return statement;
    } // getDefaultStatement()


    /************************************************** <pre>
     * Summary: Get the StatementDefinition for this statementType.
     *
     </pre> **************************************************/

    public StatementDefinition getAStatement(String statementType) {
	StatementDefinition sd;
	for(int i=0; i < statementDefinitionList.size(); i++) {
	    sd = (StatementDefinition) statementDefinitionList.get(i);

	    if(statementType.equals(sd.getobjType() + "/" +sd.getmenuString())){
		return sd;
	    }
	}
	return null;
    }


    /**
     * Perform a search. Return only the ids of matches, but put actual
     * data in the given search cache.
     *
     * <p>A fully implemented search function would take more arguments,
     * such as the attributes to be returned and the criterion by which
     * search results are ordered.
     * @param searchCache search cache
     * @param returnObjType return object type
     * @return array of SearchResults indicating matches and nonmatches
     */
    public SearchResults[] searchDB(Hashtable searchCache, String returnObjTyp, 
                       Hashtable statement, StatementDefinition curStatement,
                       String match) {

	SearchResults[] result;

	// Start the DB search and wait for the results.
	result = dbManager.startSearchNGetResults(statement, searchCache,
						  curStatement, match);

	return result;
    } // search()


} // class ShufflerService
