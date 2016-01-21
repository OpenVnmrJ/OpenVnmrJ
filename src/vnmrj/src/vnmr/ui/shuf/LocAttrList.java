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

import vnmr.util.*;
import vnmr.bo.*;
import vnmr.ui.*;

/********************************************************** <pre>
 * Summary: Controls the full list of attributes and the list of values
 *      for each attribute.
 *
 *      This can be used to:
 *          obtain the list of all attributes for this objTyp
 *          obtain the list of all values for a given attribute
 *
 *      In the code below the following naming will be used:
 *          allLists: Is a HashMap where keys are objType, and values are
 *              HashArrayList (attrList). The HashArrayList has keys which are
 *              attributes and values which are ArrayLists of Strings of
 *              values for that attribute (valList).
 *          attrList: Is a HashArrayList which has keys which are
 *              attributes and values which are ArrayLists of Strings of
 *              values for that attribute (valList).
 *          valList: Is an ArrayLists of Strings of values for that
 *              attribute (valList).
 *
 </pre> **********************************************************/

public class LocAttrList {
    // allLists is a HashMap where keys are objType, and values are
    // HashArrayList. The HashArrayList has keys which are attributes and
    // values which are ArrayLists of Strings of values for that attribute.
    HashMap allLists=null;
    FillDBManager dbManager;
    boolean errorReported=false;

    /******************************************************************
     * Summary: Constructor.  Fill the 'allLists' from persistence if available
     *  or from the DB if no persistence available.
     *
     *  'allLists' is a HashMap where keys are objType, and values are
     *  HashArrayList. The HashArrayList has keys which are attributes
     *  and values which are ArrayLists of Strings of values for that
     *  attribute.
     *****************************************************************/

    public LocAttrList(FillDBManager dbManager) {
        try {

            

            // Save the dbManager for future reference.  We have to do this,
            // because this code can be executed from managedb which does
            // not have ShufDBManager to use to retrieve dbManager on the fly.
            this.dbManager = dbManager;

            if(dbManager.locatorOff() && !dbManager.managedb) {
                allLists = new HashMap();
                return;
            }

            // Fill 'HashMap allLists' from persistence file if it exists.
            // Else, fill it from the DB, which is slower.

            java.util.Date starttime=null;
            if(DebugOutput.isSetFor("LocAttrList") ||
                                  DebugOutput.isSetFor("locTiming"))
                starttime = new java.util.Date();

            // fill 'allLists' either from persistence file or from DB
            if(!readPersistence()) {
                // No persistence file, so start from scratch
                if(DebugOutput.isSetFor("LocAttrList"))
                    Messages.postDebug("LocAttrList calling "
                                       + " fillAllListsFromDB");

                // If we are in managedb, we need to go ahead and fill the
                // list now.  If in vnmrj, we want to let the interface get
                // started and fill the list in a thread.
//                if (FillDBManager.managedb) {
                    fillAllListsFromDB(0);
//                }
//                else {
//                    // Create a empty set of lists so things don't have an
//                    // exception. Then we will fill the list via thread
//                    createEmptyList();
//
//                    // Start a thread to fill the lists
//                    UpdateAttributeList updateAttrThread;
//                    updateAttrThread = new UpdateAttributeList();
//                    updateAttrThread.setPriority(Thread.MIN_PRIORITY);
//                    updateAttrThread.setName("Update DB Attr List");
//                    updateAttrThread.start();
//
//                    // Tell the user it may be awhile before the attribute value
//                    // menus are up to date.
//                    Messages
//                            .postWarning("The list used for menu choices in the"
//                                    + " locator statements\n    may not be up to date yet."
//                                    + " The list is being generated,\n    but may take a few "
//                                    + " minutes to an hour to complete.");
//                }
            }

            if(DebugOutput.isSetFor("LocAttrList") ||
                                  DebugOutput.isSetFor("locTiming")) {
                java.util.Date endtime = new java.util.Date();
                long timems = endtime.getTime() - starttime.getTime();
                Messages.postDebug(
                    "LocAttrList Filling allLists in constructor: Time(ms) = "
                     + timems);
            }

        }
        catch (Exception e) {
            Messages.postError("Problem filling Attribute List, Locator will "
                               + " not function correctly.");
            Messages.writeStackTrace(e);

            // Create an empty one
            allLists = new HashMap();
        }

        if(allLists == null && !dbManager.managedb) {
            Messages.postError("Problem filling Attribute List, Locator will "
                               + " not function correctly.");
            // Create an empty one
            allLists = new HashMap();
        }

        // Check to see if any data files in the DB are from this host.
        // If not, the remote DB may have been rebuild and we may need
        // to update the DB with the files from this host.
// This does not work if creating an empty list and a thread.
// I am not so sure it was doing any good anyway.
//         ArrayList al = getAttrValueList("hostname", Shuf.DB_VNMR_DATA);

//         if(DebugOutput.isSetFor("LocAttrList")) {
//             Messages.postDebug("LocAttrList values obtained for hostname = "
//                             + al + "\n    localHost = " + dbManager.localHost);
            
//         }

//         if(!al.contains(dbManager.localHost) && !dbManager.managedb) {
//             // No msg if we are in managedb
        	
//         	// Only output this message once
//         	if(!errorReported) {
//         		Messages.postDebug("No vnmr data files were found in the\n"
//                                + "    " + dbManager.dbHost + " database, "
//                                + " originating from " + dbManager.localHost
//                                + ".  It may be that\n    the database on "
//                                + dbManager.dbHost + " has been rebuilt."
//                                + "  If that is the case,\n    you need to "
//                                + "run  \'managedb update\' on "
//                                + dbManager.localHost + " or execute\n    " 
//                                + "\'Update locator/Update all\' from "
//                                + "the \'Utilities\' menu on " 
//                                + dbManager.localHost + ".");
        		
//         		errorReported = true;
//         	}
//         }

    }

    /******************************************************************
     * Summary: Return the key list which is the list of all attr names.
     *
     *
     *****************************************************************/
    public ArrayList getAllAttributeNames(String objType) {
        if(FillDBManager.locatorOff())
            return new ArrayList();

        HashArrayList attrList = (HashArrayList)allLists.get(objType);
        if(attrList == null)
            attrList = new HashArrayList();

        ArrayList attrNames = attrList.getKeyList();

        return (ArrayList)attrNames.clone();
    }

    /******************************************************************
     * Summary: Return the key list which is the list of all attr names.
     *
     *
     *****************************************************************/
    public ArrayList getAllAttributeNamesSort(String objType) {

        if(FillDBManager.locatorOff())
            return new ArrayList();

        // Get a clone of the list
        ArrayList sortedNames = getAllAttributeNames(objType);

        Collections.sort(sortedNames,  new NumericStringComparator());

        return sortedNames;
    }


    /******************************************************************
     * Summary: Return the sorted list of attributes from the limited
     *  list which are contained in the shufxxxParamsLimited list.  
     *
     *
     *****************************************************************/
    public ArrayList getAttrNamesLimited(String objType) {
        ArrayList limitingList;
        
        if(FillDBManager.locatorOff())
            return new ArrayList();

        // Get the shufxxxParamsLimited list for this objType
        if(objType.equals(Shuf.DB_VNMR_DATA))
            limitingList = dbManager.shufDataParamsLimited.getKeyList();
        else if(objType.equals(Shuf.DB_STUDY) || 
                objType.equals(Shuf.DB_LCSTUDY))
            limitingList = dbManager.shufStudyParamsLimited.getKeyList();
        else if(objType.equals(Shuf.DB_PROTOCOL))
            limitingList = dbManager.shufProtocolParamsLimited.getKeyList();
        else if(objType.equals(Shuf.DB_IMAGE_DIR) ||
                objType.equals(Shuf.DB_COMPUTED_DIR) ||
                objType.equals(Shuf.DB_IMAGE_FILE))
            limitingList = dbManager.shufImageParamsLimited.getKeyList();
        else
            // Not one of the above, return the full list from the DB
            limitingList = getAllAttributeNamesSort(objType);

        ArrayList sortedVals = (ArrayList)limitingList.clone();
        Collections.sort(sortedVals,  new NumericStringComparator());

        if(DebugOutput.isSetFor("getAttrNamesLimited"))
            Messages.postDebug("getAttrNamesLimited attribute list for " +
                               objType + ": " + sortedVals);

        // If I don't return a clone here, then StatementDefinition can modify 
        // my real copy of it.
        return sortedVals;
    }



     /******************************************************************
     * Summary: Return the list of values for this attribute.
     *
     *
     *****************************************************************/
    public ArrayList getAttrValueListSort(String attr, String objType) {


        // Get a clone of the list
        ArrayList sortedVals = getAttrValueList(attr, objType);

        Collections.sort(sortedVals,  new NumericStringComparator());

        // If I don't return a clone here, then StatementDefinition modify my
        // real copy of it.
        return sortedVals;
    }

    /******************************************************************
     * Summary: Update the List from info in a single row of the DB.
     *
     *
     *****************************************************************/
    public void updateListFromSingleRow(String fullpath, String host,
                                        String objType, boolean tags) {
        ArrayList attrNames;
        ArrayList valList;
        String attr;
        String fileValue;

        if(FillDBManager.locatorOff())
            return;

        java.util.Date starttime=null;
        if(DebugOutput.isSetFor("LocAttrList") ||
                       DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        HashArrayList attrList = (HashArrayList)allLists.get(objType);
        if(attrList == null)
            attrList = new HashArrayList();

        //Get the list of all attributes.
        attrNames = getAllAttributeNames(objType);

        for(int i=0; i < attrNames.size(); i++) {
            //Go through that list, getting the 'value' for this file
            attr = (String)attrNames.get(i);

            // If tags is false, then we want to skip tags
            if(!tags && attr.startsWith("tag"))
                continue;

            fileValue = dbManager.getAttributeValue(objType, fullpath,
                                          host, attr);

            // If this file does not have a value set, then skip it
            if(fileValue == null || (fileValue.trim()).length() == 0)
                continue;

            //See if this value is already in attributelist for this attr.
            valList = getAttrValueList(attr, objType);

            if(valList != null && !valList.contains(fileValue)) {
                // Does this attribute exists yet?
                if(attrList.containsKey(attr)) {
                    // The attr exists, just add this value to its value list.
                    valList.add(fileValue);  // add the new value

                    // put will replace the prev entry with the new valList
                    attrList.put(attr, valList);
                }
                else {
                    // The attr does not exist, create it and add this as
                    // its only value.
                    ArrayList al = new ArrayList();
                    al.add(fileValue);
                    attrList.put(attr, al);
                }
            }
        }

        if(DebugOutput.isSetFor("LocAttrList") ||
                       DebugOutput.isSetFor("locTiming")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("updateListFromSingleRow: Time(ms) = "
                               + timems);
        }

    }
    /******************************************************************
     * Summary: Return the list of values for this attribute.
     *
     *
     *****************************************************************/
    public ArrayList getAttrValueList(String attr, String objType) {
    
        if(FillDBManager.locatorOff())
            return new ArrayList();
    
        HashArrayList attrList = (HashArrayList)allLists.get(objType);
        if(attrList == null)
            attrList = new HashArrayList();
    
        ArrayList valList = (ArrayList)attrList.get(attr);
        if(valList == null)
            valList = new ArrayList();
    
        ArrayList sortedVals = (ArrayList)valList.clone();
    
        // If I don't return a clone here, then StatementDefinition modifies my
        // real copy of it.
        return sortedVals;
    }

    public void addTagValue(String objType, String tag) {
        ArrayList valList;

        if(FillDBManager.locatorOff())
            return;

        //See if this value is already in attributelist for this attr.
        valList = getAttrValueList("tag", objType);

        if(valList != null && !valList.contains(tag)) {
            HashArrayList attrList = (HashArrayList)allLists.get(objType);
            if(attrList == null)
                attrList = new HashArrayList();

            // Does this attribute exists yet?
            if(attrList.containsKey("tag")) {
                // The attr exists, just add this value to its value list.
                valList.add(tag);  // add the new value

                // put will replace the prev entry with the new valList
                attrList.put("tag", valList);
            }
            else {
                // The attr does not exist, create it and add this as
                // its only value.
                ArrayList al = new ArrayList();
                al.add(tag);
                attrList.put("tag", al);
            }
        }
    }

    /******************************************************************
     * Summary: Update the LocAttrList in its entirety based on the DB.
     *
     *
     *****************************************************************/

    public HashArrayList createListFromDB(long sleepTimeMs, String objType)
                        throws Exception {
        ArrayList valList, attrListFromFullDB;
        ArrayList attrListFromAttributeList;
        String string, cmd, attr;
        HashArrayList attrList;
        java.sql.ResultSet rs;

        java.util.Date starttime=null;
        if(DebugOutput.isSetFor("LocAttrList") ||
                       DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        // Get all of the attr names, except tag
        try {
            attrListFromFullDB = dbManager.getAllAttributeNamesFromDB(objType);
        }
        catch (Exception e) {
            throw e;
        }
        attrList = new HashArrayList();

        for(int i=0; i < attrListFromFullDB.size(); i++) {
            attr = (String) attrListFromFullDB.get(i);
            try {
                // Get the current list of values for this attr from the
                // DB itself
                valList = dbManager.getAttrValueListFromDB(attr, objType,
                                                           sleepTimeMs);
            }
            catch (Exception e) {
                throw  e;
            }

            if(valList == null)
                valList = new ArrayList();  // Create an empty list if necessary

            // Put this attr and value list combo into the HashArrayList
            attrList.put(attr, valList);

            // If we are in a thread and we want to cause mimimal interruption
            // sleep between  attributes.
            if(sleepTimeMs > 0) {
                try {
                    Thread.sleep(sleepTimeMs);
                }
                catch(InterruptedException ie) {
                    // This normally means that we are closing vnmrj and
                    // we need to forward this information up the line.
                    throw ie;
                }
                catch(Exception e) {
                }
            }
        }
        if(DebugOutput.isSetFor("LocAttrList") ||
           DebugOutput.isSetFor("locTiming")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("createListFromDB: Time(ms) = "
                               + timems + " for " + objType);
        }
        return attrList;
    }



    /******************************************************************
     * Summary: Add one or more values to the LocAttrList for this attribute.
     *
     *
     *****************************************************************/

    public void updateValues(String attr, ArrayList Values, String objType) {
        ArrayList valList;

        if(FillDBManager.locatorOff())
            return;

        // Do not put tags or long_time in here.
        if(attr.startsWith("long_time") || attr.startsWith("tag"))
            return;

        valList =  getAttrValueList(attr, objType);
        if(valList == null)
            return;

        for(int i=0; i < Values.size(); i++) {
            if (!valList.contains(Values.get(i))) {
                // We need to add this value to the list of values for this attr
                valList.add(Values.get(i));
                HashArrayList attrList = (HashArrayList)allLists.get(objType);
                if(attrList == null)
                    attrList = new HashArrayList();
                // Put this new list in place for this attr.
                attrList.put(attr, valList);

            }
        }
    }

    /******************************************************************
     * Summary: Fill the full set of allLists from the DB.
     *
     *  Fill 'HashMap allLists' where allLists is a HashMap where keys are
     *  objType, and values are HashArrayList.
     *  The HashArrayList has keys which are attributes and values which are
     *  ArrayLists of Strings of values for that attribute.
     *****************************************************************/
    public void fillAllListsFromDB(long sleepTimeMs)
                                               throws Exception {
        String objType;
        HashArrayList attrList;



        if(FillDBManager.locatorOff())
            return;

        java.util.Date starttime=null;
        if(DebugOutput.isSetFor("LocAttrList") ||
                       DebugOutput.isSetFor("fillAllListsFromDB") ||
                       DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        if(allLists == null)
            allLists = new HashMap();

        // Create an entry to the hashMap (allLists) which contains
        // the dbhost which this list was created from.
        Hashtable ht = new Hashtable();
        ht.put("dbHost", dbManager.dbHost);
        allLists.put("dbHost", ht);
        
        // Go through the objType list and create a HashArrayList for each
        // as described in the header.
        for(int i=0; i < Shuf.OBJTYPE_LIST.length; i++) {
            objType = Shuf.OBJTYPE_LIST[i];
            try {
                attrList = createListFromDB(sleepTimeMs, objType);
            }
            catch (Exception e) {
                // This normally means that we are closing vnmrj and
                // we need to forward this information up the line.
                throw e;
            }

            // Add this list to the full 'allLists' with objType as the key
            // and the list as the value.
            // We need to do this in a way that keeps the 'allLists' as a valid
            // object for other simultaneous threads.  Based on the api for
            // HashMap, this single command should be okay, because this
            // key already exists, and only its value is being changed.
            // If it does not exist, we must be at startup time.
            allLists.put(objType, attrList);

            // If vnmrj, write persistence and update the statement in
            // the locator.
            if(!FillDBManager.managedb) {
                // Write the persistence file after each objtype in case vnmrj
                // is stopped.  Then we will at least have the objtypes that 
                // have been done.
                writePersistence();
            
                // Update the statement so that it shows the info we have
                // thus far.
                SessionShare sshare = ResultTable.getSshare();
                if(sshare != null) {
                    StatementHistory history = sshare.statementHistory();
                    history.updateWithoutNewHistory();
                }
            }

        }

        if(DebugOutput.isSetFor("LocAttrList") ||
                       DebugOutput.isSetFor("fillAllListsFromDB") ||
                       DebugOutput.isSetFor("locTiming")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("LocAttrList.fillAllListsFromDB: Time(ms) = "
                               + timems + " with sleepTimeMs = " + sleepTimeMs);
        }
    }


    // Create an empty list to be filled in later
    public void createEmptyList() {
        String objType;
        HashArrayList attrList;


        java.util.Date starttime=null;

        if(allLists == null)
            allLists = new HashMap();

        // Create an entry to the hashMap (allLists) which contains
        // the dbhost which this list was created from.
        Hashtable ht = new Hashtable();
        ht.put("dbHost", dbManager.dbHost);
        allLists.put("dbHost", ht);
        
        // Go through the objType list and create an empty HashArrayList for each
        // as described in the header.
        for(int i=0; i < Shuf.OBJTYPE_LIST.length; i++) {
            objType = Shuf.OBJTYPE_LIST[i];
            attrList = new HashArrayList();        

            // Add this list to the full 'allLists' with objType as the key
            // and the list as the value.
            // We need to do this in a way that keeps the 'allLists' as a valid
            // object for other simultaneous threads.  Based on the api for
            // HashMap, this single command should be okay, because this
            // key already exists, and only its value is being changed.
            // If it does not exist, we must be at startup time.
            allLists.put(objType, attrList);

        }
    }

        

    /******************************************************************
     * Summary: Update a single objType list within allLists.
     *
     *
     *****************************************************************/
    public void updateSingleList(String objType, long sleepTimeMs) {
        HashArrayList attrList=null;

        if(FillDBManager.locatorOff())
            return;

        try {
            // Get the up to date contents for this objType
            attrList = createListFromDB(sleepTimeMs, objType);
        }
        catch (InterruptedIOException ioe) {
            // This interrupt should mean vnmrj is exiting and we would
            // need to forward this up the line.  However, this method
            // is not called by the thread which needs to be exited.
            // So, do nothing.
        }
        catch (InterruptedException ie) {
            // This interrupt should mean vnmrj is exiting and we would
            // need to forward this up the line.  However, this method
            // is not called by the thread which needs to be exited.
            // So, do nothing.
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return;
        }

        // Put these contents into the main list.
        allLists.put(objType, attrList);

    }


    /******************************************************************
     * Summary: Read the persistence file and fill 'HashMap allLists'if it
     *  exists.  Return false if no persistence file, or problem reading it.
     *
     *****************************************************************/
    private boolean readPersistence() {
        String filepath;
        ObjectInputStream in;

        java.util.Date starttime=null;
        if(DebugOutput.isSetFor("LocAttrList") ||
           DebugOutput.isSetFor("locTiming"))
            starttime = new java.util.Date();

        filepath = FileUtil.openPath(FileUtil.sysdir()
                                     + "/pgsql/persistence/LocAttrList");
        if(filepath != null) {
            File file = new File(filepath);
            if(!file.canRead()) {
                if(DebugOutput.isSetFor("LocAttrList"))
                    Messages.postDebug(filepath +
                                "\nNot readable, creating LocAttrList from DB");
                return false;
            }
        }
        // File or dir does not exist, just exit.
        else {
            if(DebugOutput.isSetFor("LocAttrList"))
                Messages.postDebug(FileUtil.sysdir()
                                 + "/pgsql/persistence/LocAttrList"
                                 +"\nNot found, creating LocAttrList from DB");
            return false;
        }

        try {
            in = new ObjectInputStream(new FileInputStream(filepath));
            // Read it in.
            allLists = (HashMap) in.readObject();
            in.close();
        }
        catch (ClassNotFoundException ce) {
            Messages.postError("Problem reading filepath");
            Messages.writeStackTrace(ce,
                               "Error caught in LocAttrList.readPersistence()");
            removePersistenceFile();
            return false;
        }
        catch (Exception e) {
            if(DebugOutput.isSetFor("LocAttrList")) {
               Messages.writeStackTrace(e, "LocAttrList.readPersistence()");
            }
            removePersistenceFile();
            return false;
        }

        // HashArrayList is not serializable.  That is, it does not get
        // written out with the two ArrayLists it has.  However, all of the
        // information is in the Hashtable.  We need to recreate all of
        // the HashArrayLists from the Hashtables which were saved properly.
        Set set = allLists.entrySet();
        Iterator iter = set.iterator();
        while(iter.hasNext()) {

            Map.Entry entry = (Map.Entry)iter.next();

            // Catch the entry with a key of dbHost
            String key = (String) entry.getKey();
            if(key.equals("dbHost")) {
                Hashtable ht = (Hashtable)  entry.getValue();
                String dbHost = (String) ht.get("dbHost");
                if(!dbHost.equals(dbManager.dbHost)) {
                    // This persistence file does not go with the current
                    // dbHost.  Return false, and a new list will be
                    // created from the DB
                    Messages.postDebug("LocAttrList persistence file is "
                                       + "for " + dbHost + "\n    not "
                                       + dbManager.dbHost + ".  "
                                       + "A new list be being build.");
                    return false;
                }
                // Continue to the next entry
                continue;
            }

           // The Value here should be a Hashtable written without
           // the two ArrayLists.  Treat it like a Hashtable and create

           HashArrayList hal = new HashArrayList((Hashtable)entry.getValue());

           // Put it back using the same key (objType) and the new HashArrayList
           allLists.put(entry.getKey(), hal);

        }

        if(DebugOutput.isSetFor("LocAttrList") ||
                       DebugOutput.isSetFor("locTiming")) {
            java.util.Date endtime = new java.util.Date();
            long timems = endtime.getTime() - starttime.getTime();
            Messages.postDebug("LocAttrList.readPersistence: Time(ms) = "
                               + timems);
        }
        if(DebugOutput.isSetFor("LocAttrList"))
            Messages.postDebug("LocAttrList finished reading " + filepath);

        return true;

    }

    public void writePersistence() {
        String filepath;
        ObjectOutput out;
        HashMap newAllLists = new HashMap();

        if(allLists == null)
            return;
        
        if(FillDBManager.locatorOff())
            return;
        
        try {
            // If dir does not exist or is not writable, just exit.
            File file = new File(FileUtil.sysdir() + "/pgsql/persistence");
            File dir = new File(FileUtil.sysdir() + "/pgsql");
            if(!file.canWrite()) {
                if(!dir.canWrite()) {
                    if(DebugOutput.isSetFor("LocAttrList")) {
                        Messages.postDebug(FileUtil.sysdir()
                                       + "/pgsql/persistence\n"
                                       + "does not exist or is not writable."
                                       + "\nSkipping writing of LocAttrList.");
                    }
                    return;
                }
            }

            filepath = FileUtil.savePath(FileUtil.sysdir()
                                     + "/pgsql/persistence/LocAttrList");
        }
        catch (Exception e) {
            Messages.postError(" Problem writing "
                        + FileUtil.sysdir() + "/pgsql/persistence/LocAttrList\n"
                        + "        Skipping writing of LocAttrList.");
            Messages.writeStackTrace(e);
            return;
        }

        if(filepath == null) {
            Messages.postDebug(" Problem opening "
                               + FileUtil.sysdir() + "/pgsql/persistence\n"
                               + "        Skipping writing of LocAttrList.");
            return;
        }

        // Create an entry to the hashMap (newAllLists) which contains
        // the dbhost which this list was created from.
        Hashtable htHost = new Hashtable();
        htHost.put("dbHost", dbManager.dbHost);
        newAllLists.put("dbHost", htHost);

        // put the HashArrayLists in newAllLists with the equilivent
        // Hashtables.
        Set set = allLists.entrySet();
        Iterator iter = set.iterator();
        while(iter.hasNext()) {
            // Skip the dbHost entry, a correct values was put in above.
            Map.Entry entry = (Map.Entry)iter.next();
            String key = (String) entry.getKey();
            if(key.equals("dbHost")) {
                continue;
            }

            // The Value here should be a Hashtable. Use it to create
            // a HashArrayList with the info from this Hashtable.
            HashArrayList hal = (HashArrayList) entry.getValue();

            Hashtable ht = hal.getHashtable();

            // Put in using the same key and the new Hashtable
            newAllLists.put(entry.getKey(), ht);
        }

        try {
            out = new ObjectOutputStream(new FileOutputStream(filepath));
            // Write it out.
            out.writeObject(newAllLists);
            out.close();
        }
        catch (Exception ioe) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(ioe);
        }

        if(DebugOutput.isSetFor("LocAttrList")) {
            Messages.postDebug("LocAttrList.writePersistence() wrote " +
                               filepath);
        }

        // The file should now exist, be sure it has world write access
        // so that future users can access it.
        // If Windows, we need a windows path
        String[] cmd = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION, 
                        "chmod 666 " + filepath};
        Process prcs = null;
        try {
            Runtime rt = Runtime.getRuntime();
            prcs = rt.exec(cmd);

            // Be sure this completes before continuing.
            prcs.waitFor();
        }
        catch (Exception e) {
            Messages.postDebug(" Problem with cmd: " + cmd
                               + "\nSkipping writing of LocAttrList.");
            Messages.writeStackTrace(e);
            return;
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }

        }

    }

    /******************************************************************
     * Summary: Remove the LocAttrList persistence file.
     *
     *  When the DB is destroyed and rebuilt, the persistence file
     *  will be out of date, use this to get rid of it.
     *****************************************************************/

    static public void removePersistenceFile() {
        try {
            // If dir does not exist or is not writable, just exit.
            String filepath = FileUtil.openPath(FileUtil.sysdir()
                                            + "/pgsql/persistence/LocAttrList");
            if (filepath != null) {
                File file = new File(filepath);
                if (file != null) {
                    if (file.canWrite()) {
                        file.delete();
                    }
                }
            }
        }
        // No errors, just bail out
        catch (Exception e) {
        }
    }

}
