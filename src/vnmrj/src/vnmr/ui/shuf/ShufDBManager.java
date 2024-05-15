/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.net.*;
import java.io.*;
import java.util.*;
import java.sql.*;

import vnmr.bo.Record;
import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

/********************************************************** <pre>
 * Summary: Utilities to start and communicate with db_manager.
 * @author  Glenn Sullivan
 </pre> **********************************************************/

public class ShufDBManager extends FillDBManager {
    /** Create dbManager at startup and keep it for returning from a static
        method. */
    private static ShufDBManager dbManager = new ShufDBManager();
    private static int warningCounter=-1;
    private static int maxLocItemsToShow = 2000;
    private static boolean maxLocItemsToShowRead = false;

    // The first 2 checks of version are before the error reporting is set up
    // in vnmrj.  We will allow checking 3 times so that there is an error
    // report in the vnmrj display
    private static int versionChecked=0;


    /************************************************** <pre>
     * Summary: Start and connect to db_manager.
     * Details:
     *   - Create a socket and get its port number
     *   - Start db_manager giving the port number as an argument.
     *     Use a thread so that we can be notified if the process dies 
     *     by calling our DBManagerExited().  Other than that, this thread
     *     does nothing but wait forever.
     *   - Wait for db_manager to connect to the socket
     *   - Start a Thread which listens to the socket and takes care of 
     *     the incoming messages.  This thread runs until the conneciton
     *     is lost.
     </pre> **************************************************/


    public ShufDBManager() {
        String persona = System.getProperty("persona");
        boolean bAdm = (persona != null && persona.equalsIgnoreCase("adm")) ? true
                            : false;

        // for admin interface, don't need the locator, nor if it is off
        if (!bAdm) {
            attrList = new LocAttrList(this);
        }

    } // End ShufDBManager()


    public static ShufDBManager getdbManager() {
        return dbManager;
    }

    
    /************************************************** <pre>
     * Summary: Get the results of a DB search when defaulting to 0 rows.
     *
     *
     </pre> **************************************************/
   private SearchResults[] getSearchResults(Hashtable searchCache,
            String objType, ResultSet rsMatch1,
            ResultSet rsMatch2,
            ResultSet rsNonMatch){
        
        int count1=0, count2=0, countNon=0;
        SearchResults[] rs;

        // Default the counts (number of rows) to 0 and call main function
        rs = getSearchResults(searchCache, objType, rsMatch1, count1,
                              rsMatch2, count2, rsNonMatch, countNon);
        return rs;
 
    }
    
   // Call to getSearchResults without specifying numAttrReturn.  This would
   // be from locator calls so default numAttrReturn to 3 since it always
   // has 3 columns.
   private SearchResults[] getSearchResults(Hashtable searchCache,
           String objType, ResultSet rsMatch1, int count1,
           ResultSet rsMatch2, int count2, 
           ResultSet rsNonMatch, int countNon) {
       
       int numAttrReturn = 3;
       return getSearchResults(searchCache, objType, rsMatch1, count1,
                               rsMatch2, count2, rsNonMatch, countNon, numAttrReturn);
   }
   
    /************************************************** <pre>
     * Summary: Get the results of a DB search.
     *
     *
     </pre> **************************************************/


    private SearchResults[] getSearchResults(Hashtable searchCache,
                                     String objType, ResultSet rsMatch1, int count1,
                                     ResultSet rsMatch2, int count2, 
                                     ResultSet rsNonMatch, int countNon, int numAttrReturn){

        int     match_val=1;
        String  filename;
        String  fullpath;
        String  attr1;
        String  attr2;
        String  attr3;  
        String  hostname;
        String  host_fullpath;
        String  owner;
        Object  newrow[];
        SearchResults[] result;
        ResultSetMetaData md;
        int totRows=0;
        ResultSet rs;
        int type;
        boolean [] dates = new boolean[numAttrReturn];
        String lock;

        if(objType == null)
            objType = "?";

        // Get the total number of rows coming.
        totRows = count1 + count2 + countNon;
        result = new SearchResults[totRows];

        // If too many things are displayed in the locator, it is too slow.
        // This causes bug reports etc.  Tell the user how to remedy this.
        // Check to see if there are greater than 10000 fids in the DB and the
        // user is getting non-matching items.  Just do it a couple of times.
        if(countNon > 100) {
            warningCounter++;

            if(warningCounter == 10 || warningCounter == 100) {
                int numFids = getNumFidsInDB();

                if(numFids > 10000) {

                    Messages.postMessage(Messages.OTHER|Messages.WARN|
                                         Messages.MBOX| Messages.LOG,
                        " Probable slow locator update.\n"
                        + "    With moderate numbers of items in the locator "
                        + "database, the system \n    will operate faster if "
                        + "the display of items in the locator is set to\n"
                        + "    'Display only matching items'.  This can be set "
                        + "by going to the \n    'System settings' panel from "
                        + "the Utilities menu, then to the \n"
                        + "    'Display/Plot' tab.");
                }
            }
        }


        // Set rs to the first bunch.
        if(rsMatch1 != null) 
            rs = rsMatch1;
        else if(rsMatch2 != null) 
            rs = rsMatch2;
        else if (rsNonMatch != null) 
            rs = rsNonMatch;
        else {
            // If there is no DB connection, or some similar problem,
            // Fill the columns with Error and continue.
            // If debug set to locatorOff, then put "Off" in the panel
            String err = "Error";
            if(locatorOff())
                err = "Off";

            result = new SearchResults[1];
            newrow =  new Object[] 
                       {LockStatus.unlocked, err, err, err, err,
                        err, Shuf.DB_VNMR_DATA, err, err};
            result[0] = new SearchResults(1, newrow, 
                                      new Exp(err + ":" + err), err, err);
            searchCache.put(err + ":" + err, newrow);

            // Remove the locAttrList persistence file in case it
            // is screwed up
//            LocAttrList.removePersistenceFile();

            return result;      
        }
                
        // We need to know if any of the  displayed columns is a Date.  
        try {
            
            md = rs.getMetaData();
            for(int k=0; k < numAttrReturn; k++) {
                type = md.getColumnType(k+1);

                if(type == Types.TIMESTAMP)
                    dates[k] = true;
                else
                    dates[k] = false;
            }
        }
        catch (Exception e) {
            Messages.postError("Problem getting locator results from DB");
            Messages.writeStackTrace(e, "Error caught in getSearchResults()" +
                                     " while checking for Date for " + objType);

            // Remove the locAttrList persistence file in case it
            // is screwed up
            LocAttrList.removePersistenceFile();

            return null;
        }

        for(int i=0; i < totRows; i++) {
            // After we finish with rsMatch1, go for the ones in rsMatch2 if any
            if(i == count1 && count2 > 0) {
                rs = rsMatch2;
            }
            // change the flag and rs when we hit the non matching rows.
            if(i == count1 + count2) {
                rs = rsNonMatch;
                // change the flag when we hit the non matching rows.
                match_val = 0;
            }
            
            // Advance to the next row, (also needed to get to the first row)
            try {
                // Get the  displayed columns.
                // If we are returned a null, set to empty string.
                // If we have a date, limit to 19 characters.
                // 19 will keep date and time, but cut off the time zone number

                String[] attrs = new String[numAttrReturn];
                for(int k=0; k < numAttrReturn; k++) {
                    attrs[k] = rs.getString(k+1);
                    if(attrs[k] == null)
                        attrs[k] = "";
                    if(dates[k] && attrs[k].length() > 19) {
                        attrs[k] = attrs[k].substring(0, 19);
                    }
                }

// ********   Check that these are correct for locator and Protocol Browser ****
                filename = rs.getString(numAttrReturn +1);
                if(filename == null)
                    filename = "";
                fullpath = rs.getString(numAttrReturn +2);         
                if(fullpath == null)
                    fullpath = "";
                hostname = rs.getString(numAttrReturn +6);
                if(hostname == null)
                    hostname = "";
                lock = rs.getString(numAttrReturn +4);
                if(lock == null)
                    lock = "0";
                owner = rs.getString(numAttrReturn +5);
                if(owner == null)
                    owner = "";

                newrow = new Object[numAttrReturn +6];
                newrow[0] = LockStatus.unlocked;
                // Fill the columns attrs being returned
                for(int k=0; k < numAttrReturn; k++) {
                    newrow[k+1] = attrs[k];
                }
                newrow[numAttrReturn +1] = filename;
                newrow[numAttrReturn +2] = fullpath;
                newrow[numAttrReturn +3] = objType;
                newrow[numAttrReturn +4] = hostname;
                newrow[numAttrReturn +5] = owner;

                // ********** Add other types as they are implemented ********
                // Create the correct type of object based on object_type
                host_fullpath = new String(hostname + ":" + fullpath);
                if(objType.equals(Shuf.DB_VNMR_DATA))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new Exp(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_VNMR_PAR))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new Param(host_fullpath), 
                                                  hostname, objType);
                
                else if(objType.equals(Shuf.DB_VNMR_RECORD) ||
                        objType.equals(Shuf.DB_VNMR_REC_DATA)) {
                    // If corrupt, override match_val with number for red
                    UNFile file = new UNFile(fullpath + File.separator + 
                                         "checksum.flag");
                    int match_val_override;
                    if(file.exists())
                        match_val_override = 3;
                    else
                        match_val_override = match_val;


                    // If the DB is out of date with respect to the corrupt
                    // attribute, we need to take care of it immediately.
                    String corrupt = rs.getString(9);
                    if((corrupt.equals("valid") && file.exists()) ||
                        corrupt.equals("corrupt") && !file.exists()) {
                        // Update this file in the DB
                        addVnmrFileToDB(filename, fullpath, owner, objType, 0, null);

                        // Now the DB is update, but the results from the
                        // search are still out of date.  Look at the three
                        // column values and see if any of them are for the
                        // corrupt attr and if so, change its value.
                        // Since we don't seem to have the column header 
                        // names, just look for valid or corrupt as a value.

                        if(attrs[1].equals("valid"))
                            attrs[1] = "corrupt";
                        else if(attrs[1].equals("corrupt"))
                            attrs[1] = "valid";
                        if(attrs[2].equals("valid"))
                            attrs[2] = "corrupt";
                        else if(attrs[2].equals("corrupt"))
                            attrs[2] = "valid";
                        if(attrs[3].equals("valid"))
                            attrs[3] = "corrupt";
                        else if(attrs[3].equals("corrupt"))
                            attrs[3] = "valid";

                        // Now we need to create a new newrow with the new
                        // attr value we changed.
                        if(lock.equals("0"))
                            newrow =  new Object[] 
                                {LockStatus.unlocked, attrs[1], attrs[2], attrs[3], 
                                 filename, fullpath, objType, hostname, owner};
                        else
                            newrow =  new Object[] 
                                {LockStatus.locked, attrs[1], attrs[2], attrs[3], 
                                 filename, fullpath, objType, hostname, owner};

                    }

                    if(objType.equals(Shuf.DB_VNMR_RECORD)) {
                        result[i] = new SearchResults(match_val_override, 
                                                    newrow, 
                                                    new Record(host_fullpath), 
                                                    hostname, objType);
                    }
                    else if(objType.equals(Shuf.DB_VNMR_REC_DATA)) {
                        result[i] = new SearchResults(match_val_override, 
                                                 newrow, 
                                                 new RecordData(host_fullpath), 
                                                 hostname, objType);
                    }
                }
                else if(objType.equals(Shuf.DB_PANELSNCOMPONENTS))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new PanelNComp(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_WORKSPACE))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new Workspace(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_COMMAND_MACRO))
                    // filename for command_n_macro is really 'name'
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new Macro(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_PPGM_MACRO))
                    // filename for DB_PPGM_MACRO is really 'name'
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new PpgmMacro(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_SHIMS))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new ShimSet(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_PROTOCOL))
                    result[i] = new SearchResults(match_val, newrow, 
                                                new ProtocolType(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_STUDY))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new StudyType(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_LCSTUDY))
                    result[i] = new SearchResults(match_val, newrow, 
                                                new LcStudyType(host_fullpath), 
                                                hostname, objType);
                else if(objType.equals(Shuf.DB_AVAIL_SUB_TYPES))
                    result[i] = new SearchResults(match_val, newrow, 
                                       new AvailSubTypes(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_AUTODIR))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new Autodir(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_IMAGE_DIR))
                    result[i] = new SearchResults(match_val, newrow, 
                                              new ImageDirType(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_COMPUTED_DIR))
                    result[i] = new SearchResults(match_val, newrow, 
                                              new ImageDirType(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_IMAGE_FILE))
                    result[i] = new SearchResults(match_val, newrow, 
                                              new ImageFileType(host_fullpath), 
                                                  hostname, objType);
                else if(objType.equals(Shuf.DB_TRASH))
                    result[i] = new SearchResults(match_val, newrow, 
                                                  new TrashType(host_fullpath), 
                                                  hostname, objType);
                else
                    Messages.postError("Unsupported object type, " + objType);



                // If we had totRows equal maxLocItemsToShow, then we probably 
                // truncated the results.  Put the word Truncated as the 
                // last item 
                int rowLimit = getMaxLocItemsToShow();
                if(i == (totRows -1) && totRows >= rowLimit ) {
                    newrow =  new Object[] 
                       {LockStatus.unlocked, "Truncated", "Truncated", 
                        "Truncated", "Truncated", "Truncated", 
                        Shuf.DB_VNMR_DATA, "Truncated", "Truncated"};
                    result[i] = new SearchResults(match_val, newrow, 
                                                new Exp("Truncated:Truncated"), 
                                                "Truncated", "Truncated");
                    
                    host_fullpath = "Truncated:Truncated";
                }

                searchCache.put(host_fullpath, newrow);
            }
            catch (Exception e) {
                Messages.postError("Problem getting locator results from DB");
                Messages.writeStackTrace(e, "Error caught in getSearchResults" +
                                " while getting rows of data for " + objType);

                // Remove the locAttrList persistence file in case it
                // is screwed up
                LocAttrList.removePersistenceFile();

                return null;
            }

            // Go to the next row.  NOTE: if we had not done the "last"
            // above to get the number of rows, and then gone to the
            // first, we would have needed this command at the top of
            // this loop.
            try {
                rs.next();
            } 
            catch (Exception e) {
            Messages.postError("Problem getting locator results from DB");
            Messages.writeStackTrace(e, "Error caught in getSearchResults()" +
                                 " while getting the next row for " + objType);
            }

        }
        return result;
    }

 

    /************************************************** <pre>
     * Summary:  Send search command to DB and get results.
     </pre> **************************************************/
    public synchronized SearchResults[] startSearchNGetResults(
                     Hashtable statement, Hashtable searchCache, 
                     StatementDefinition curStatement, String matchFlag) {
        SearchInfo info;
        SearchResults[] result=null;

        // Check the version two times, versionChecked is static.
        // The first two checks of version are before the error reporting is 
        // set up in vnmrj.  We will allow checking 4 times so that there 
        // is an error report in the vnmrj display
        if(versionChecked < 4) {
            // Check DB version
            String vresult = checkDBversion();
            if(vresult.equals("bad")) {
                // If there is a connection, then the bad version is probably 
                // valid. 
                if(checkDBConnection() == true)
                    Messages.postError( 
                        "DataBase contents version is not correct. \n"
                        + "      Run 'dbsetup' to create a new version." );
/* Already get a no DB message */
/*
                else
                    Messages.postError(
                                "No DB Connection, Locator will not function.");
 */
                result = getSearchResults(searchCache, null,
                                          null, null, null);
                return result;
            }
            versionChecked++;
        }


        if(curStatement == null) {
            // Fill the shuffler comumns with Error
            result = getSearchResults(searchCache, null,
                                      null, null, null);
            return result;
        }

        // Get all of the info required for the search.
        info = curStatement.getSearchInfo(statement);
        if(info == null){
            // Fill the shuffler comumns with Error
            result = getSearchResults(searchCache, null,
                                      null, null, null);
            return result;
        }

        // Do the real work
        result = doSearch(info, searchCache, matchFlag);
        return result;
    }

    // Do a DB search and return the results.
    // searchInfo can be filled as appropriate and passed in
    // matchFlag can be set to "match", "all" or "nonmatch".
    // searchCache can be obtained with the following call to use to pass in here
    //     SessionShare searchCache = Util.getSessionShare();
    // Returns an array of SearchResults items.
    public synchronized SearchResults[] doSearch(SearchInfo searchInfo, 
                                          Hashtable searchCache, String matchFlag) {
        String sqlCmd=null;
        ResultSet rsMatch1=null, rsMatch2=null, rsNonMatch=null;
        int count1=0, count2=0, countNon=0;
        SearchResults[] result=null;
        boolean matching=true, matchAnyAttr;
        int numRows=-1;

        // Do the search
        try { 
            if(matchFlag.equals("match") ||  matchFlag.equals("all")){
                // Get matching items if requested
                matching=true;
                matchAnyAttr=false;

                // For the ProtocolBrowser, we have the ability to combine the
                // results from two tables.  This is done by using the sql command
                // UNION ALL in between two or our normal sql commands.
                // So, if secondObjType is not null, then do this.
                if(searchInfo.secondObjType != null) {
                    // usingUnion will trigger createSqlCmd to leave off
                    // the sorting and limiting for this first part of the
                    // sql cmd.
                    searchInfo.usingUnion = true;
                    sqlCmd = createSqlCmd(searchInfo, matching, matchAnyAttr);

                    // This is a bit of a kludge, but it looks like the easiest way
                    // to get the sqlcmd for the second objType is to just set
                    // the new type in searchInfo and call createSqlCmd()
                    String savObjType = searchInfo.objectType;
                    searchInfo.objectType = searchInfo.secondObjType;
                    
                    // Allow the sort and limit to be added for the 2nd half of the cmd
                    searchInfo.usingUnion = false;

                    String sqlCmd2 = createSqlCmd(searchInfo, matching, matchAnyAttr);
                    
                    // Now combine the two sqlcmds with UNION ALL and continue below.
                    sqlCmd = sqlCmd + " UNION ALL " + sqlCmd2;
                    
                    // Restore just in case it is needed.  I don't think it is.
                    searchInfo.objectType = savObjType;
                }
                else {
                    // Create an SQL command from all the searchInfo
                    sqlCmd = createSqlCmd(searchInfo, matching, matchAnyAttr);
                }

                // Do the Search
                if(sqlCmd != null) {
                    count1= executeQueryCount(sqlCmd);
                    rsMatch1 = executeQuery(sqlCmd);
                    rsMatch1.next();
                }
                // if more than one bool attribute, then call again with
                // matchAnyAttr = true and matching still true
                if(searchInfo.numBAttributes > 1) {
                    matchAnyAttr=true;
                    // Create an SQL command from all the searchInfo
                    sqlCmd = createSqlCmd(searchInfo, matching, matchAnyAttr);
                    // Do another Search
                    if(sqlCmd != null)  {
                        count2= executeQueryCount(sqlCmd);
                        rsMatch2 = executeQuery(sqlCmd);
                        rsMatch2.next();
                    }
                }
            }
            if (matchFlag.equals("nonmatch") ||  matchFlag.equals("all")){
                /* Get Non matching items if requested */
                matching = false;
                matchAnyAttr=false;
                // Create an SQL command from all the searchInfo
                sqlCmd = createSqlCmd(searchInfo, matching, matchAnyAttr);        
                // Do another Search
                if(sqlCmd != null) {
                    countNon= executeQueryCount(sqlCmd);
                    rsNonMatch = executeQuery(sqlCmd);
                    rsNonMatch.next();
                }
            }
        } 
        catch (Exception e) { 
            // We need to assemble an error string depending on what
            // happens below.
            String errStr = "Problem doing locator search. \n    " + 
                               e.getMessage();
            
            // If error was that an attribute name was not in the DB,
            // then the locator statement must have an attribute in
            // it that is not in the DB.  Tell the user this.
            // OR!!! the DB may be empty and never filled for this type
            // and thus not have this attribute.
            String str = e.getMessage();
            if(str != null) {
                // Check the error message to see if it is 
                //"attribute xxx not fount"
                if(str.indexOf("attribute") != -1 && 
                                str.indexOf("not found") != -1) {

                    // Check to see if there are any entries in the DB
                    // for this objType.  If not, then just continue
                    // with an empty set.
                    try { 
                        String cmd = "select * from " + searchInfo.objectType;
                        numRows = executeQueryCount(cmd);
                    }
                    catch (Exception se) { }

                    if(numRows == 0) {
                        Messages.postError("Database is empty for " +
                                           searchInfo.objectType);
                    }
                    else {
                        // Determine the name of the persistence file we
                        // need to remove.  Else, we propogate this problem.
                        SessionShare sshare = ResultTable.getSshare();
                        LocatorHistoryList lhl = sshare.getLocatorHistoryList();
                        LocatorHistory lh = lhl.getLocatorHistory();
                        String key = lh.getKey();
                        String filepath;
                        filepath = FileUtil.openPath("USER/PERSISTENCE/" +
                                                     "LocatorHistory_" +key);
                        // Delete this persistence file.
                        File file = null;
                        if(filepath != null)
                            file = new File(filepath);
                        if(file != null)
                            file.delete();

                        // Add to error string for output below.
                        errStr = errStr.concat("Probable cause is that the " +
                            " attribute named above, is being" +
                            " specified in the\n" + "'" +
                            FileUtil.openPath(
                                 "LOCATOR/" +LocatorHistoryList.STATE_DEF_FILE +
                                  key + ".xml") + 
                            "' file, and is not" +
                            " in the DataBase for this type.\n" +
                            "Fix this file and run vnmrj again.\n");
                    }
                }
            }
            Messages.postError(errStr);
            Messages.writeStackTrace(e, 
                                "Error caught in startSearchNGetResults()\n" +
                                "while getting the number of rows.\n" +
                                "sqlCmd: " + sqlCmd);

            // If there is no DB connection, or some similar problem,
            // Fill the columns with Error and continue.
            result = getSearchResults(searchCache, searchInfo.objectType,
                                      null, null, null);
            return result;          
        }
        
        int numAttrReturn = searchInfo.attrToReturn.length;
        // Put all of the results into a single SearchResults[] for return
        result = getSearchResults(searchCache, searchInfo.objectType, 
                                  rsMatch1, count1, rsMatch2, count2, 
                                  rsNonMatch, countNon, numAttrReturn);
        return result;
    }



    /************************************************** <pre>
     * Summary: Set the value for a given attribute for a given filepath.
     *
     * Details:
     *    Example of calling this method:
     *
     *    ShufflerService shufflerService = ShufflerService.getDefault();
     *    ShufDBManager dbManager = shufflerService.getdbManager();
     *    boolean value = dbManager.setAttributeValue(objType, fullpath, 
     *                                          host, attr, attrType, attrVal);
     *        where:
     *           objType = Shuf.DB_VNMR_DATA, Shuf.DB_PANELSNCOMPONENTS,
     *                      etc.
     *           fullpath = The filename and path starting at /
     *           host =     The host name that added the file to the DB
     *           attr =     Which attribute to set the value for
     *           attrType = DB data type: text, int, float8 or date
     *           attrVal =  Text string with new value
     *
     *        Return true for success and false for failure
     *
     *    If this needs to be called from FillDBManager in managedb, then
     *    call FillDBManager.setNonTagAttributeValue().
     *
     </pre> **************************************************/

    public boolean setAttributeValue(String objType, String fullpath, 
                                     String host, String attr, String attrType,
                                     String attrVal) {
        String str;
        StringBuffer cmd;
        String tagName;
        int    numTags;
        ResultSet rs;
        ResultSetMetaData md;
        int   emptyTagCol;
        int   numColumns;
        String canonPath;
        UNFile file;
        String dhost;

        if(DebugOutput.isSetFor("setAttributeValue")) {
            Messages.postDebug("setAttributeValue: objType = " + objType
                               + " attr = " + attr + " attrVal = " + attrVal
                               + "\nfullpath = " + fullpath);
        }

        try {
            file = new UNFile(fullpath);

            // If the file does not exist, bail out
            if(!file.exists()) {
                return false;
            }
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost = (String) mp.get(Shuf.HOST);
            canonPath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            Messages.postError("Problem updating attribute value for\n    " 
                               + fullpath);
            Messages.writeStackTrace(e);
            return false;
        }


        if(!attr.equals("tag")) {
            // All but tag.  If tag, find actual tag column name to
            // use below, then call again with attr = tag0, tag1, etc.
            // Then we will hit this section and set it.
            // See if this attribute exists.  If not, it will be added.
            if(isAttributeInDB(objType, attr, attrType, canonPath)) {
                if(objType.endsWith(Shuf.DB_MACRO)) {
                    cmd = new StringBuffer("UPDATE ");
                    cmd.append(objType).append(" SET \"").append(attr);
                    cmd.append("\" = \'").append(attrVal);
                    cmd.append("\' WHERE name = \'").append(canonPath);
                    cmd.append("\'");
                }
                else {
                    cmd = new StringBuffer("UPDATE ");
                    cmd.append(objType).append(" SET \"").append(attr);
                    cmd.append("\" = \'").append(attrVal);
                    cmd.append("\' WHERE host_fullpath = \'");
                    cmd.append(dhost).append(":").append(canonPath);
                    cmd.append("\'");
                }
                try {
                    executeUpdate(cmd.toString());
                    return true;
                }
                catch (Exception e) { 
                    Messages.postError("Problem setting value for \n    " +
                                       attr + " in " + canonPath);
                    Messages.writeStackTrace(e,"setAttributeValue: objType = "+
                             objType + "\fullpath = " + canonPath + "\nattr = " +
                             attr + " attrType = " + attrType + " attrVal = " +
                             attrVal);
                    return false;
                }
            }
        }
        // attr == tag
        else {
            String user = System.getProperty("user.name");
            // Save tags as user:tag
            tagName = new String(user + ":" + attrVal);


            try { 
                // Set value for a tag.  Add new tag columns as necessary.
                numTags = numTagColumns(objType);

                /* See if this host_fullpath already has this tag set on some 
                   column. */
                if(numTags > 0) {
                    if(objType.endsWith(Shuf.DB_MACRO)) {
                        cmd = new StringBuffer("SELECT name FROM ");
                        cmd.append(objType).append(" WHERE name = \'");
                        cmd.append(canonPath).append("\' AND (");
                    }
                    else {
                        cmd = new StringBuffer("SELECT host_fullpath FROM ");
                        cmd.append(objType).append(" WHERE host_fullpath = \'");
                        cmd.append(dhost).append(":").append(canonPath);
                        cmd.append("\' AND (");
                    }
                    for(int i=0; i < numTags; i++) {
                        cmd.append("tag").append(i).append(" = \'");
                        cmd.append(tagName).append("\' ");
                        if(i < numTags -1)
                            cmd.append("OR ");
                    }
                    cmd.append(")");
                    rs = executeQuery(cmd.toString());
                    if(rs == null)
                        return false;

                    if(rs.next()) {
                        /* It already has that tag, so just return true. */
                        return true;
                    }

                    // Else we need to set a new tag value. 
                    /* Is their an empty column to use, or do we need to 
                       create one. */
                    int i;
                    cmd = new StringBuffer("SELECT tag0");
                    for(i=1; i < numTags; i++) {
                        cmd.append(", tag").append(i);
                    }
                    if(objType.endsWith(Shuf.DB_MACRO)) {
                        cmd.append(" FROM ").append(objType);
                        cmd.append(" WHERE name = \'").append(canonPath);
                        cmd.append("\'");
                    }
                    else {
                        cmd.append(" FROM ").append(objType);
                        cmd.append(" WHERE host_fullpath = \'");
                        cmd.append(dhost).append(":").append(canonPath);
                        cmd.append("\'");
                    }               
                    rs = executeQuery(cmd.toString());
                    if(rs == null)
                        return false;

                    md = rs.getMetaData();

                    boolean exists = rs.next();  // Go to the first and only row
                    if(!exists)
                        return false;

                    numColumns = md.getColumnCount();

                    // Go thru the columns to see if any are empty.
                    for(i=0; i < numColumns; i++) {
                        // i+1 because the columns start at 1
                        str = rs.getString(i+1);
                        if(str == null || str.length() == 0 || 
                                          str.equals("null") || 
                                          str.equals(user + ":" + "null")) {
                            break;
                        }
                    }
                    if(i < numTags)
                        emptyTagCol = i;
                    else
                        emptyTagCol = -1;
                }
                else
                    emptyTagCol = -1;

                if(emptyTagCol == -1) {  /* Create a new column */
                    str = "tag" + String.valueOf(numTags);
                    isAttributeInDB(objType, str, attrType, canonPath);
                    emptyTagCol = numTags;
                }
                
                str = "tag" + emptyTagCol;
                // Make a recursive call to this function.  Now we have
                // replaced attr with str which will be tag0, tag1 etc
                // so this time the upper part of this if/else will execute
                // and set this tag value.
                setAttributeValue(objType, fullpath, host, str, 
                                  attrType, tagName);

                TagList.addToAllTagNames(objType, attrVal);

            }
            catch (Exception e) { 
                Messages.postError("Problem setting value for \n    " + attr +
                                       " in " + canonPath);
                Messages.writeStackTrace(e,"setAttributeValue: objType = "+
                             objType + "\fullpath = " + canonPath + "\nattr = " +
                             attr + " attrType = " + attrType + " attrVal = " +
                             attrVal);
                return false;
            }

        }
        return true;
    }



    /************************************************** <pre>
     * Summary: Update a single attribute and reshuffle.
     *
     * Call this method when the value of an attribute/parameter
     * in a workspace changes and that attribute is in the DB
     *
     * Arguments:
     *  attr => the attribute name being updated
     *  attrType =>  DB data type: text, int, float8 or date
     *  attrVal => String with new value for this attribute
     *  filename => exp1, exp2 etc.
     *  
     * To get access to this method, you can do the following:
     *    ShufflerService shufflerService = ShufflerService.getDefault();
     *    ShufDBManager dbManager = shufflerService.getdbManager();
     *    dbManager.updateWorkspaceAttribute(attr, attrType, attrVal, filename);
     *
     *  
     * To get a list of the params that need to be updated if
     * they are changed do the following:
     *    ShufflerService shufflerService = ShufflerService.getDefault();
     *    ShufDBManager dbManager = shufflerService.getdbManager();
     *    ArrayList attrList;
     *    attrList = dbManager.getAllAttributeNames(Shuf.DB_WORKSPACE);
     * This gives a list of all attributes in the DB for workspaces
     *    
     </pre> **************************************************/

    public boolean updateWorkspaceAttribute(String attr, String attrType,
                                               String attrVal, String filename){

        SessionShare sshare = ResultTable.getSshare();
        StatementHistory history = sshare.statementHistory();
        Hashtable statement = history.getCurrentStatement();
        String objType = (String)statement.get("ObjectType");
        HoldingDataModel holdingDataModel;
        
        // If attr is a time, and the value is empty, skip it.
        if(attr.startsWith("time_")) {
            String trimmed = attrVal.trim();
            if(trimmed.length() == 0) {
                return false;
            }
        }

        // Get the host name.  It should be safe to just get the local
        // host, because we are ONLY working with workspaces here.
        String host;
        try {
            InetAddress inetAddress = InetAddress.getLocalHost();
            host = inetAddress.getHostName();
        }
        catch(Exception e) {
            Messages.postError("Problem getting HostName");
            host = new String("");
        }
        

        // Create fullpath for this exp
        //String dir = System.getProperty("userdir");
        String dir = FileUtil.usrdir();
        String fullpath = new String(dir + "/" + filename);


        boolean status = setAttributeValue (Shuf.DB_WORKSPACE, fullpath, 
                                            host, attr, attrType, attrVal);
        
        // Only reshuffle if setting was successful 
        if(status == false)
            return false;
    
    // If the shuffler panel is not up, don't try to work with it.
    if(!Shuffler.shufflerInUse()) 
        return false;
        

        // Do the shuffler and set the results in the panel only if workspace.
        if(objType.equals(Shuf.DB_WORKSPACE)) {
            // Initiate the actual database search via thread
            StartSearch searchThread = new StartSearch(statement);
            searchThread.setName("Shuffler Search");
            searchThread.start();
        }
        else {
            // The workspace table is not displayed, however, we may have
            // workspace items in the holding area.  Go thru them and
            // see if any are workspace and need to be updated.
            holdingDataModel = sshare.holdingDataModel();
            holdingDataModel.updateSingleParam(host +":" + fullpath, attr, 
                                               attrVal);

        }

        return true;

    }

    static String prevSortAttr="";
    static boolean charType=false;
    /************************************************** <pre>
     * Create an SQL command from the information in the SearchInfo arg.
     *
     *
     </pre> **************************************************/
    private String createSqlCmd(SearchInfo info, boolean matching, 
                                boolean matchAnyAttr) {
        String cmd;
        ResultSet rs;
        ResultSetMetaData md;
        String attrName, value, string=null, string2, name;
        int type=0;
        boolean battr=false, tag=false;
        boolean dirLimit=false;
        boolean any_attr=false, arr_attr=false;
        boolean battr2=false, date=false;
        /* utype_e => exclusive, utype_n => nonexclusive */
        boolean utype_e=false, utype_n=false;
        int numTags;
        boolean foundit=false;
        boolean useAccess=false;

        if(info == null)
            return "";


        // Use the user access information for the following objTypes
        if(info.objectType.equals(Shuf.DB_VNMR_DATA) || 
                   info.objectType.equals(Shuf.DB_VNMR_PAR)  ||
                   info.objectType.equals(Shuf.DB_STUDY)  ||
                   info.objectType.equals(Shuf.DB_LCSTUDY)  ||
                   info.objectType.equals(Shuf.DB_AUTODIR)  ||
                   info.objectType.equals(Shuf.DB_IMAGE_DIR)  ||
                   info.objectType.equals(Shuf.DB_COMPUTED_DIR)  ||
                   info.objectType.equals(Shuf.DB_IMAGE_FILE)) {
            useAccess = true;
            
            // If first accessibleUsers is all, then don't use the access
            if(info.accessibleUsers.size() == 0 || info.accessibleUsers.get(0).equals("all")) 
                useAccess = false;
        }
        if(info.sortAttr.equals("tag")) {
            Messages.postError("Cannot sort on tag!");
            return null;
        }
        /* We need to determine the data type for sort_attribute for use below.
           I don't know how to find it until a "select" has been done. So,
           do a dummy select which retrieves sort_attribute, then find out
           what its type is.  Since this does take some time, only do it
           if the attribute has changed. 
        */
        if(!info.sortAttr.equals(prevSortAttr)) {
            prevSortAttr = info.sortAttr;

            try {
                /* This select does not search the database, so it is quick.
                   It returns no rows, but we can access the field type. 
                */
                rs = executeQuery("SELECT \"" + info.sortAttr + "\" FROM " +
                                  info.objectType + " WHERE \'false\'::bool;"); 
                if(rs == null)
                    return "";
                // What we have now is all the columns, with no rows.
                // Get the column information.
                md = rs.getMetaData();
                type = md.getColumnType(1);
            }
            catch (Exception e) {
                /* If there was a problem, set type to non text an go on. */
                charType = false;
            }

            /* If it is a text type, note it as such. */
            if(type == Types.VARCHAR)
                charType = true;
            else
                charType = false;
        }

        /** Now create the search string and do the search. **/

        if(info.objectType.endsWith(Shuf.DB_MACRO)) {
            if(info.attrToReturn[0] != null)
                cmd = new String("SELECT \"" + info.attrToReturn[0] + 
                                 "\", \"" + info.attrToReturn[1] + "\", \"" + 
                                 info.attrToReturn[2] + "\", \"name\"");
            else
                cmd = new String("SELECT \"name\"");
        }
        else {
            // Add the attributes to be returned as comma separate list.
            cmd = new String("SELECT \"" + info.attrToReturn[0] + "\"");
            for(int i=1; i < info.attrToReturn.length; i++) {
                cmd = cmd.concat(", \"" + info.attrToReturn[i] + "\"");  
            }

            cmd =  cmd.concat(", \"filename\", \"fullpath\", \"hostdest\", arch_lock" +
                     ", \"owner\", \"hostname\"");
        }

        // for Records we will need the corrupt attribute.
        if(info.objectType.equals(Shuf.DB_VNMR_RECORD) ||
                 info.objectType.equals(Shuf.DB_VNMR_REC_DATA)) {
            cmd = cmd.concat(", \"corrupt\"");
        }

        
        /* To sort we must return the sorted attribute also.  To get the
           sort to be case-insensitive, we will return the attribute
           cast to upper case for text strings, then sort on that new item. 
           If item is not a char, we still need to specify that it be 
           returned, but do not specify upper.
        */
        if(charType)
            cmd = cmd.concat(", UPPER (\"" + info.sortAttr + "\") AS sortkey");
        else
            cmd = cmd.concat(", \"" + info.sortAttr + "\" AS sortkey");

        
        /* Specify which table */
        cmd = cmd.concat(" FROM " + info.objectType);

        /* To create the sql where statement correctly with parenthesis etc,
           we need to know which items we have to put in.  Create short
           easy local variables for this.
        */

        if(info.numAttributes > 0)
            any_attr = true;
        if(info.numBAttributes > 0)
            battr = true;
        if(info.numBAttributes > 1)
            battr2 = true;
        if(info.numArrAttributes > 0)
            arr_attr = true;
        if(info.dirLimitAttribute != null)
            dirLimit = true;

        if(!info.dateRange.equals(Shuf.DB_DATE_ALL))
            date = true;

        /* Do we have a tag? */
        if(info.tag.length() > 0)
            tag = true;

        if(info.userTypeNames.size() > 0 && info.userTypeMode.startsWith("e"))
            utype_e = true;
        if(info.userTypeNames.size() > 0 && info.userTypeMode.startsWith("n"))
            utype_n = true;

        if(useAccess) {
            /* There will always at least be the accessable owners list for
               some types, so always have a where command. */
            cmd = cmd.concat(" WHERE ");
        }
        else {
            /* else we only have a where if there is something */
            if(any_attr || arr_attr || battr || date || tag || utype_n || 
                        utype_e || dirLimit) {
                cmd = cmd.concat(" WHERE ");
            }

        }

        if(useAccess) {
            /* If anything, put paren around it for accessable_owner list */
            if(any_attr || arr_attr || battr || date || tag || utype_n || 
                        utype_e) {
                cmd = cmd.concat("(");
            }
        }

        /* if userType is exclusive, put paren around everything else */
        if(utype_e && (any_attr || arr_attr || battr || date || tag ))
            cmd = cmd.concat("(");

        /* if non matching, just one paren. */
        if(!matching && 
           (any_attr || arr_attr || battr || battr2 || date || tag || utype_n))
            cmd = cmd.concat("(");

        /* If nothing given, Tell the caller we found nothing. */
        /* utype_e not in this list. */
        if(!matching && !any_attr && !arr_attr && !battr && !date && !tag && 
                !utype_n) {
            return (null);
        }

        /* Go thru the attributeNames attributes and add the ones that need
           to be added.   Use UPPER on these so that we will search
           in a 'case insensitive' mode. */
        for(int i=0; i < info.numAttributes; i++) {
            /* If anything is to follow add AND or OR.  This includes
               search_attr beyond this one. */
            attrName = (String)info.attributeNames.get(i);
            value = (String)info.attributeValues.get(i);
            if(i < info.numAttributes -1 || arr_attr || battr || date || tag ||
                                utype_n) {
                if(matching) {
                    // We cannot do a search on numeric values using LIKE, else
                    // things like "7" and "7.0" fail to match.  This is a
                    // particular problem with the ProtocolBrowser and the
                    // "field" attribute.  Currently, the information as to what
                    // type of attribute this is (numeric versus text) is not
                    // known here.  So, catch "field" and use "=" instead of
                    // LIKE.
                    if(attrName.equals("field")) {
                        cmd = cmd.concat(attrName + " = \'" + value + "\' AND ");
                    }
                    else {
                        cmd = cmd.concat("UPPER (" + attrName + ") LIKE UPPER (\'" 
                                     + value + "\') AND ");
                    }
                }
                else  {  /* NON_MATCHING */
                    cmd = cmd.concat("UPPER (" + attrName 
                                     + ") NOT LIKE UPPER (\'" + value 
                                     + "\') OR " + attrName + " IS NULL OR ");
                }
            }
            else {
                if(matching) {
                    if(attrName.equals("field")) {
                        cmd = cmd.concat(attrName + " = \'" + value + "\' ");
                    }
                    else {
                        cmd = cmd.concat("UPPER (" + attrName + ") LIKE UPPER (\'" 
                                     + value + "\') ");
                    }
                }
                else  {  /* NON_MATCHING */
                    cmd = cmd.concat("UPPER (" + attrName 
                                     + ") NOT LIKE UPPER (\'" + value 
                                     + "\') OR " + attrName + " IS NULL "); 
                }
            }
        }

        /* Go thru the arrAttributeNames attributes and add the ones that need
           to be added.    */
        for(int i=0; i < info.numArrAttributes; i++) {
            /* If anything is to follow add AND or OR.  This includes
               search_attr beyond this one. */
            attrName = (String)info.arrAttributeNames.get(i);
            value = (String)info.arrAttributeValues.get(i);
            if(i < info.numArrAttributes -1 || battr || date || tag || utype_n){
                if(matching) 
                    cmd = cmd.concat("\'" + value + "\' = ANY (" + 
                                     attrName + ") AND ");
                else  {  /* NON_MATCHING   '**<>' is array not equal */
                    cmd = cmd.concat("\'" + value + "\' != ANY (" + 
                              attrName + ") OR " + attrName + " IS NULL OR ");
                }
            }
            else {
                if(matching)
                    cmd = cmd.concat("\'" + value + "\' = ANY (" + 
                                     attrName + ") ");
                else  {  /* NON_MATCHING */
                    cmd = cmd.concat("\'" + value + "\' != ANY (" + 
                              attrName + ") OR " + attrName + " IS NULL ");
                }
            }
        }

        /* bool attribute */
        if(battr) {     
            /* If only one attribute, go here. */
            if(!battr2) {
                attrName = (String)info.bAttributeNames.get(0);
                value = (String)info.bAttributeValues.get(0);
                if(matching) 
                    cmd = cmd.concat("UPPER (\"" + attrName + 
                                     "\") = UPPER (\'" + value + "\')"); 
                else  {  /* NON_MATCHING */
                    cmd = cmd.concat("UPPER (\"" + attrName + 
                                     "\") != UPPER (\'" + value + "\') OR " +
                                     attrName + " IS NULL ");
                }
            }
            /* If more than one attribute, come here.   The caller will specify
               whether we are requiring a match of ALL attributes or ANY of 
               them. To get all items, the caller will need to call 3 times.  
               Once with matchAnyAttr = TRUE, once with FALSE and once with
               return_which_items = DB_NON_MATCHING.
            */
            else {
                if(!matching) {
                    for(int i=0; i < info.numBAttributes; i++) {
                        if(i == 0) 
                            cmd = cmd.concat(" NOT (");
                        if(i > 0)
                            cmd = cmd.concat(" OR ");
                        attrName = (String)info.bAttributeNames.get(i);
                        value = (String)info.bAttributeValues.get(i);
                        cmd = cmd.concat("UPPER (\"" + attrName + 
                                         "\") = UPPER (\'" + value + "\')");

                        // If there are not any further BAttributes
                        if(i == info.numBAttributes -1) 
                            cmd = cmd.concat(")");
                    }
                }
                else { /* MATCHING */
                    for(int i=0; i < info.numBAttributes; i++) {
                        if(matchAnyAttr) { /* match any but not all attr. */
                            if(i == 0) {
                                cmd = cmd.concat("(");
                                string = " AND NOT (";
                            }
                            if(i > 0) {
                                cmd = cmd.concat(" OR ");
                                /* String will collect the AND NOT part of the
                                   command to eliminate the full match items.
                                */
                                string = string.concat(" AND ");
                            }
                            attrName = (String)info.bAttributeNames.get(i);
                            value = (String)info.bAttributeValues.get(i);
                            string2 = ("UPPER (\"" + attrName + 
                                       "\") = UPPER (\'" + value + "\')");
                            cmd = cmd.concat(string2);
                            string = string.concat(string2);
                            /* If this was the last attribute. */
                            if(i == info.numBAttributes -1) {
                                cmd = cmd.concat(")");
                                string = string.concat(")");
                                /* append the AND NOT part of the command. */
                                cmd = cmd.concat(string);
                            }
                        }
                        else { /* Must match all attributes. */
                            if(i > 0) 
                                cmd = cmd.concat(" AND ");
                            attrName = (String)info.bAttributeNames.get(i);
                            value = (String)info.bAttributeValues.get(i);
                            cmd = cmd.concat("UPPER (\"" + attrName + 
                                             "\") = UPPER (\'" + value + "\')");
                        }
                    }
                }
            }
            if(date || tag || utype_n) {
                if(matching) 
                    cmd = cmd.concat(" AND ");
                else
                    cmd = cmd.concat(" OR ");
            }
        }

        /* Specify date range */
        if(date) {
            if(matching) {
                if(info.dateRange.equals(Shuf.DB_DATE_SINCE)){
                   cmd = cmd.concat(" " + info.whichDate + " >= \'" + 
                                    info.date1 + "\' ");
                }
                else if(info.dateRange.equals(Shuf.DB_DATE_BEFORE)){
                    cmd = cmd.concat(" " + info.whichDate + " < \'" + 
                                     info.date1 + "\' ");
                }
                else if(info.dateRange.equals(Shuf.DB_DATE_BETWEEN)){
                    cmd = cmd.concat(" " + info.whichDate + " between \'" + 
                                     info.date1 + "\' and \'" + 
                                     info.date2 + "\' ");
                }
            }
            else {       /* NON_MATCHING */
                if(info.dateRange.equals(Shuf.DB_DATE_SINCE)) {
                    cmd = cmd.concat(" " + info.whichDate + " < \'" + 
                                     info.date1 + "\' ");
                }
                else if(info.dateRange.equals(Shuf.DB_DATE_BEFORE)) {
                    cmd = cmd.concat(" " + info.whichDate + " >= \'" +
                                     info.date1 + "\' ");
                }
                else if(info.dateRange.equals(Shuf.DB_DATE_BETWEEN)) {
                    cmd = cmd.concat(" " + info.whichDate + " not between \'" +
                                     info.date1 + "\' and \'" + 
                                     info.date2 + "\' ");
                }
            }
            if(tag || utype_n) {
                if(matching) 
                    cmd = cmd.concat(" AND ");
                else
                    cmd = cmd.concat(" OR ");
            }

        }

        /* specify tag search.  This is complicated because we need to know
           if the tag matches ANY of the tag columns. */
        if(tag) {
            String user = System.getProperty("user.name");
            cmd = cmd.concat("(");
            /* How many tag columns are there. */
            try {
                numTags = numTagColumns(info.objectType);
            }
            catch (Exception e) {
                Messages.writeStackTrace(e);
                numTags = 0;
            }
            for(int i=0; i < numTags; i++) {
                if(matching) {
                    cmd = cmd.concat("tag" + String.valueOf(i) + 
                                     " = \'" + user + ":" + info.tag + "\' ");
                    if(i < numTags -1)
                        cmd = cmd.concat("OR ");
                }
                else {
                    cmd = cmd.concat("(tag" + String.valueOf(i) + 
                                     " != \'" +  user + ":" + info.tag + 
                                     "\' OR  " + 
                                     "tag" + String.valueOf(i) + " IS NULL) ");
                    if(i < numTags -1)
                        cmd = cmd.concat("AND ");
                }
            }
            cmd = cmd.concat(")");
            if(utype_n) {
                if(matching) 
                    cmd = cmd.concat(" AND ");
                else
                    cmd = cmd.concat(" OR  ");
            }
        }

        /* Specify user_type nonexclusive list. */
        if(utype_n) {
            if(matching)
                cmd = cmd.concat(info.userType + " in (");
            else 
                cmd = cmd.concat(info.userType + " not in (");

            /* List out the names. */
            for(int i=0; i < info.userTypeNames.size(); i++) {
                name = (String)info.userTypeNames.get(i);
                cmd = cmd.concat("\'" + name + "\' ");

                /* If we have more to follow, add ",". */
                if(i < info.userTypeNames.size() -1)
                    cmd = cmd.concat(",");
                else
                    cmd = cmd.concat(") ");
            }
        }



        if(!matching && (any_attr || arr_attr || battr || battr2 || date || 
                                tag || utype_n)) 
            cmd = cmd.concat(")");
    

        /* Exclusive user_type.  AND this test with all of the other WHERE
           statement for both matching and nonmatching */
        if(utype_e) {
            /* if anything else, need paren and AND */
            if(any_attr || arr_attr || battr || date || tag || utype_n) {
                cmd = cmd.concat(") AND ");
            }
            cmd = cmd.concat(" " + info.userType + " in (");
        
            /* List out the names. */
            for(int i=0; i < info.userTypeNames.size(); i++) {
                name = (String)info.userTypeNames.get(i);
                cmd = cmd.concat("\'" + name + "\' ");

                /* If we have more to follow, add ",". */
                if(i < info.userTypeNames.size() -1)
                    cmd = cmd.concat(",");
                else
                    cmd = cmd.concat(") ");
            }
        }

        /* Force owners test for some types */
        if(useAccess) {
            /* Accessible owners as an exclusive match. */
            /* Always have this item. */
            if(any_attr || arr_attr || battr || date || tag || utype_n || 
                                utype_e) {
                cmd = cmd.concat(") AND ");
            }
            cmd = cmd.concat("owner in (");
            /* List out the names. */
            for(int i=0; i < info.accessibleUsers.size(); i++) {
                name = (String)info.accessibleUsers.get(i);
                cmd = cmd.concat("\'" + name + "\' ");
                /* If we have more to follow, add ",". */
                if(i < info.accessibleUsers.size() -1)
                    cmd = cmd.concat(",");
            }
            cmd = cmd.concat(") ");
        }

        // Look for any exclusive attributes and add SQL cmds to
        // to limit results (both matching and non matching) 
        String mode;
        for(int i=0; i < info.numAttributes; i++) {
            mode = (String)info.attributeMode.get(i);
            if(mode.equals("exclusive")) {
                attrName = (String)info.attributeNames.get(i);
                value = (String)info.attributeValues.get(i);
                cmd = cmd.concat(" AND UPPER (" + attrName 
                                 + ") LIKE UPPER (\'" + value + "\')");
            }
        }

        // dirLimit
        if(dirLimit) {
            int numLimits=0;

            attrName = info.dirLimitAttribute;
            value = info.dirLimitValues;

            // value here will be a space separated list of directories.
            // We need to create an OR series of these in the command looking
            // something like:
            //   AND ((fullpath LIKE '/path1%') OR (fullpath LIKE '/path2%') 
            //   OR ...)
            // Where  LIKE allows wildcards, and % means any string of char.
            // That is match any host_fullpath which begins with this string.
            // Note:  I tried using "~" in place of "LIKE" and it takes
            //        substantially longer to execute.
            StringTokenizer tok = new StringTokenizer(value);
            if(any_attr || arr_attr || battr || date || tag || utype_n || 
                                utype_e || useAccess ) {
                cmd = cmd.concat(" AND ");
            }
            cmd = cmd.concat(" (");
            while(tok.hasMoreTokens()) {
                if(numLimits > 0)
                    cmd = cmd.concat(" OR ");                    
                numLimits++;
                cmd = cmd.concat(" (" + attrName + " LIKE \'" 
                                 + tok.nextToken() + "%\') ");
                                 
            }
            cmd = cmd.concat(") ");

            
        }



        /* specify attribute to sort by.  Use the upper case item, sortkey,
           we created above. */
        // If usingUnion, then don't sort and don't limit.
        // usingUnion is set by the ProtocolBrowser to get the first of
        // two sql cmds that will be joined with the sql UNION command.
        if(info.usingUnion == false) {
            String direction = info.sortDirection;
            if(!info.sortDirection.equals(Shuf.DB_SORT_DESCENDING) 
                    &&  !info.sortDirection.equals(Shuf.DB_SORT_ASCENDING)) {
                direction = Shuf.DB_SORT_DESCENDING;
            }
            cmd = cmd.concat(" ORDER BY sortkey " + direction);


            /* specify a limit on the number of rows to be sent back.  This
               is to speed up searches on large DB. */
            int rowLimit = getMaxLocItemsToShow();
            cmd = cmd.concat(" LIMIT " + rowLimit);
        }

        return cmd;
    }  // createSqlCmd()




    /************************************************** <pre>
     * Summary: Get the list of files with this tag.
     *
     * Since tags are in numerous columns (like tag0, tag1, tag2 ...)
     * we have to check for a given tag name in all tag columns.
     * When we specify info.tag below, it causes createSqlCmd() to look for
     * the tag name given this way.
     *
     </pre> **************************************************/
    public ArrayList getAllEntriesWithThisTag(String objType, String tagName) {
        SearchInfo info;
        ResultSet  rs;
        String     sqlCmd;
        ArrayList  list;
        boolean    useAccess=false;

        // Create and fill a SearchInfo object and use it to do the search
        info = new SearchInfo();
        
        info.tag = tagName;
        info.objectType = objType;

        // Get the access list for the current user
        LoginService loginService = LoginService.getDefault();
        Hashtable accessHash = loginService.getaccessHash();
        String curuser = System.getProperty("user.name");
        Access access = (Access)accessHash.get(curuser);
        SessionShare sshare = ResultTable.getSshare();
        ShufflerService shufflerService = sshare.shufflerService();

        if(info.objectType.equals(Shuf.DB_VNMR_DATA) || 
                   info.objectType.equals(Shuf.DB_VNMR_PAR)  ||
                   info.objectType.equals(Shuf.DB_STUDY)  ||
                   info.objectType.equals(Shuf.DB_LCSTUDY)  ||
                   info.objectType.equals(Shuf.DB_AUTODIR)  ||
                   info.objectType.equals(Shuf.DB_IMAGE_DIR)  ||
                   info.objectType.equals(Shuf.DB_COMPUTED_DIR)  ||
                   info.objectType.equals(Shuf.DB_IMAGE_FILE)  ||
                   info.objectType.equals(Shuf.DB_PROTOCOL)) {
            useAccess = true;
        }

        if(access == null) {
            // This should not be posible, but just in case, just give this
            // user access to himself.
            info.accessibleUsers.add(curuser);
        }
        else {
            info.accessibleUsers = access.getlist();
        }
        if(useAccess) {
            // If the first item in the access list is 'all' then ignore
            // the list and use instead, all of the users found in the DB.
            if(info.accessibleUsers.get(0).equals("all")) {
                ArrayList strarr = shufflerService.
                    queryCategoryValues(info.objectType, "owner");
                if(strarr.size() == 0) {
                    Messages.postWarning("Database empty for this type or " +
                                         "Problem communicating with database");

                    // Remove the locAttrList persistence file in case it
                    // is screwed up
                    LocAttrList.removePersistenceFile();

                    return null;
                }
                // Reset info.accessibleUsers with this new list
                info.accessibleUsers = strarr;
            }
        }
        // If not DB_VNMR_DATA, set to empty list
        else {
            info.accessibleUsers = new ArrayList();
        }
        // Fill header with columns to return.
        // We don't really need these, so set them to something valid.  
        if(objType.endsWith(Shuf.DB_MACRO)) {
            info.attrToReturn[0] = null;
            info.sortAttr = "name";
        }
        else {
            // We want host_fullpath for the resulting list.
            // Putting it in the first header column, puts it in the first
            // column of the result.
            info.attrToReturn[0] = "host_fullpath";
            info.attrToReturn[1] = "hostdest";
            info.attrToReturn[2] = "hostdest";
            // Sort on filename so our list comes out aphabetical
            info.sortAttr = "filename";
        }
        info.sortDirection =Shuf.DB_SORT_ASCENDING;



        // Create an SQL command from all the info
        sqlCmd = createSqlCmd(info, true, true);
        if(sqlCmd == null) {
            return null;
        }
        // Do the Search
        try {
            rs = executeQuery(sqlCmd);
            if(rs == null)
                return null;
            list = new ArrayList();
            // Get all of the resulting rows.
            while(rs.next()) {
                // host_fullpath is in column 1 since it is given as
                // as table column 1.
                list.add(rs.getString(1));
            }
        }
        catch (Exception e) {
            Messages.postError("Problem getting tag entries from DB");
            Messages.writeStackTrace(e, 
                   "Error caught in getAllEntriesWithThisTag" +
                   " with objType = " + objType + " and tagName = " + tagName);

            return null;
        }

        return list;    
    } // getAllEntriesWithThisTag()


    /************************************************** <pre>
     * Summary: Delete this tag entry from all files which contain it.
     *
     * Get the list of files with this tag.  We need to go thru the list
     * of files and set any  tag column which equals the tag name to "".
     *
     </pre> **************************************************/
    public void deleteThisTag(String objType, String tagName) {
        ArrayList list;
        String    host_fullpath;
        int       numTags;
        String    cmd;

        // Get the list of entries with this tag and go thru that list.
        try {
            list = getAllEntriesWithThisTag(objType, tagName);
            numTags = numTagColumns(objType);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return;
        }
        String user = System.getProperty("user.name");
        for(int i=0; list != null && i < list.size(); i++) {
            host_fullpath = (String) list.get(i);
            // Go thru the list of all tag columns (tag0, tag1, ...)
            for(int k=0; k < numTags; k++) {
                if(objType.endsWith(Shuf.DB_MACRO)) {
                    cmd = "UPDATE " + objType + " SET tag" + String.valueOf(k) +
                          " = \'\' WHERE name = \'" + host_fullpath +
                          "\' AND  tag" + String.valueOf(k) + " = \'" + 
                          user + ":" + tagName + "\'";
                }
                else {
                    cmd = "UPDATE " + objType + " SET tag" + String.valueOf(k) +
                          " = \'\' WHERE host_fullpath = \'" + host_fullpath +
                          "\' AND  tag" + String.valueOf(k) + " = \'" + 
                          user + ":" + tagName + "\'";
                }
                try {
                    executeUpdate(cmd);
                }
                catch (Exception e) {
                    Messages.postError("Problem deleting tag (" + tagName +
                                       ") from DB");
                    Messages.writeStackTrace(e, "Error caught in deleteThisTag"+
                                             " with objType = " + objType + 
                                             " and tagName = " + tagName); 
                }
            }
        }

        // Since this tag will no longer exist, we need to update
        // the statement menus.
        SessionShare sshare = ResultTable.getSshare();
        StatementHistory history = sshare.statementHistory();
        Hashtable statement = history.getCurrentStatement();
        StatementDefinition curStatement = sshare.getCurrentStatementType();
        // Set force=true to force an update.
        curStatement.updateValues(statement, true);
    }

    /************************************************** <pre>
     * Summary: Delete this tag entry from highlighted files.
     *
     * Get the list of files highlighted.  We need to go thru the list
     * of files and set any  tag column which equals the tag name to "".
     *
     </pre> **************************************************/
    public void deleteTagFromAllSelections(String objType, String tagName) {
        ArrayList list;
        String    host_fullpath;
        int       numTags;
        String    cmd;

        // Get the list of highlighted items and go thru that list.
        ResultTable resultTable = Shuffler.getresultTable();
        list = resultTable.getSelectionList();
        if(list == null)
            return;

        String user = System.getProperty("user.name");
        try {
            numTags = numTagColumns(objType);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return;
        }
        for(int i=0; i < list.size(); i++) {
            host_fullpath = (String) list.get(i);
            // Go thru the list of all tag columns (tag0, tag1, ...)
            for(int k=0; k < numTags; k++) {
                cmd = "UPDATE " + objType + " SET tag" + String.valueOf(k) +
                      " = \'\' WHERE host_fullpath = \'" + host_fullpath +
                      "\' AND  tag" + String.valueOf(k) + " = \'" + 
                      user + ":" + tagName + 
                      "\'";
                try {
                    executeUpdate(cmd);
                }
                catch (Exception e) {
                    Messages.postError("Problem deleting tag (" + tagName +
                                       ") from all selections");
                    Messages.writeStackTrace(e, 
                               "Error caught in deleteTagFromAllSelections: " +
                               "\nsqlCmd: " + cmd);
                }
            }
        }

        // Since this tag may no longer exist, we need to update
        // the statement menus.
        SessionShare sshare = ResultTable.getSshare();
        StatementHistory history = sshare.statementHistory();
        Hashtable statement = history.getCurrentStatement();
        StatementDefinition curStatement = sshare.getCurrentStatementType();
        // Set force=true to force an update.
        curStatement.updateValues(statement, true);
    }

    /************************************************** <pre>
     * Summary: Set this tag name to all of the entries in the list.
     *
     * The arg 'entries' is an array list of host_fullpath entries in the DB
     *
     * New tags are created as necessary.  It is not an error to try
     * to set a tag to a file that already has that tag.  Thus, you can
     * just call this with the full list of files even if only some really
     * need setting.
     *
     </pre> **************************************************/
    public void setTag(String objType, String tagName, ArrayList entries,
                       boolean update) {
        String string;
        String host;
        String fullpath;
        StringTokenizer tok;

        if(tagName == null  || tagName.length() == 0)
            return;

        for(int i=0; i < entries.size(); i++) {
            string = (String) entries.get(i);
            // If macro, no host exists and we just get the name here.
            if(objType.endsWith(Shuf.DB_MACRO)) {
                host = null;
                fullpath = string;
            }
            else {
                // Break apart hostFullpath into host and fullpath
                tok = new StringTokenizer(string, ":");
                host =  tok.nextToken();
                fullpath =  tok.nextToken();
            }
            // Add/set this tagName for this DB entry
            // The fullpath and host here are direct and we need mounted
            // fullpath and local host for setAttributeValue()
            String mpath = MountPaths.getMountPath(host, fullpath);

            setAttributeValue(objType, mpath, localHost, "tag", 
                              DB_ATTR_TEXT, tagName);
        }
        if(update) {
            // Since this may be a new tag, we may want to update
            // the statement menus.
            SessionShare sshare = ResultTable.getSshare();
            StatementHistory history = sshare.statementHistory();
            Hashtable statement = history.getCurrentStatement();
            StatementDefinition curStatement = sshare.getCurrentStatementType();

            curStatement.updateValues(statement, update);
        }
    }

    /************************************************** <pre>
     * Summary: Set this tag name to all of the currently selected
     * items in the shuffler.
     *
     *
     </pre> **************************************************/
    public boolean setTagInAllSelections(String objType, String tagName) {
        ArrayList list;

        ResultTable resultTable = Shuffler.getresultTable();
        list = resultTable.getSelectionList();
        if(list == null)
            return false;
        
        setTag(objType, tagName, list, true);
        return true;
        
    }

    public ArrayList getAttrValueList(String attr, String objType) {

        ArrayList output;
        try {
            if(attr == null) {
                Messages.postError("Attribute value missing.");
                return null;
            }
            if(attr.equals("tag")) {
                output = TagList.getAllTagNames(objType);
            }
            // Take care of tags here, and take care of everything else
            // in FillDBManager.getAttrValueList()
            else {
                output = super.attrList.getAttrValueList(attr, objType);
            }
        }
        catch(Exception e) {
            Messages.postError("Problem getting Attribute values");
            Messages.writeStackTrace(e);
            return null;
        }
        return output;
    }


    /******************************************************************
     * Summary: Get and return the number of trash items in the trash
     *          table for this user.
     *
     *
     *****************************************************************/

    public int getNumTrashRows() {
        int numRows=0;
        String cmd=null;
        String user = System.getProperty("user.name");

        try {
            cmd = new String("select filename from " + Shuf.DB_TRASH + 
                             " where owner=\'" + user + "\'");
            numRows = executeQueryCount(cmd);
        }
        catch (Exception e) {
            Messages.postError("Problem getting number of rows from trash");
            Messages.writeStackTrace(e, "SQL command: " + cmd);
        }

        return numRows;
    }

    public static int getMaxLocItemsToShow() {
        // If the persistence file has not been read yet, read it.
        if(!maxLocItemsToShowRead)
            readMaxLocItemsToShow();

        return maxLocItemsToShow;
    }

    public static void setMaxLocItemsToShow(String value) {
        int intValue=0;

        try {
            intValue = Integer.parseInt(value);
        }
        catch (Exception e) {
            intValue = 2000;
        }
        
        if(intValue < 1)
            intValue = 2000;

        setMaxLocItemsToShow(intValue);

    }

    public static void setMaxLocItemsToShow(int value) {
        // Set the member value
        maxLocItemsToShow = value;

        if(value > 5000 && value <= 20000)
            Messages.postWarning("Setting Max # of items to show in Locator\n"
                                 + "    to greater than 5000 will slow down "
                                 + "the responsiveness of the Locator.");
        else if(value > 20000)
            Messages.postWarning("Setting Max # of items to show in Locator\n"
                           + "    to greater than 20000 will slow down "
                           + "the responsiveness of the Locator and\n"
                           + "    can cause Java to run out of memory."
                           + "  Raise the `-mx` value in the vnmrj\n"
                           + "    startup script if this is really necessary.");

        // Write out to a persistence file
        writeMaxLocItemsToShow();
    }

     static public void writeMaxLocItemsToShow() {
        String filepath;
        FileWriter fw;
        PrintWriter os;

        filepath = FileUtil.savePath("USER/PERSISTENCE/MaxLocItemsToShow");

        try {
              fw = new FileWriter(filepath);
              os = new PrintWriter(fw);
              os.println("MaxLocItemsToShow  " + maxLocItemsToShow);
              os.close();
        }
        catch (Exception e) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(e);
        }
     }


    static public void readMaxLocItemsToShow() {
        String filepath;
        BufferedReader in;
        String line;
        String value;
        StringTokenizer tok;
        int newValue;

        maxLocItemsToShowRead = true;

        filepath = FileUtil.openPath("USER/PERSISTENCE/MaxLocItemsToShow");
        if(filepath==null) {
            return;
        }
        try {
            in = new BufferedReader(new FileReader(filepath));
            line = in.readLine();
            if(!line.startsWith("MaxLocItemsToShow")) {
                in.close();
                File file = new File(filepath);
                // Remove the corrupted file.
                file.delete();
                value = "2000";
            }
            else {
                tok = new StringTokenizer(line, " \t\n");
                value = tok.nextToken();
                if (tok.hasMoreTokens()) {
                    value = tok.nextToken();
                    in.close();
                }
                else {
                    in.close();
                    File file = new File(filepath);
                    // Remove the corrupted file.
                    file.delete();
                    value = "2000";
                }
            }
        }
        catch (Exception e) {
            // No error output here.
            value = "2000";
        }

        try {
            newValue = Integer.parseInt(value);
        }
        catch (Exception e) {
            // No error output
            newValue = 2000;

            File file = new File(filepath);
            // Remove the corrupted file.
            file.delete();
        }

        setMaxLocItemsToShow(newValue);
    }

}  // class ShufDBManager


