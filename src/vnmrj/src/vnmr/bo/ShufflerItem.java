/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.ui.shuf.*;

/********************************************************** <pre>
 * Summary: Contain info about one shuffler entry
 *
 * @author  Glenn Sullivan
 * Details:
 *   -
 *
 </pre> **********************************************************/


public class ShufflerItem implements  Serializable {

    public boolean lockStatus;
    public String[] value;    // Attribute values for columns in shuffler
    public String[] attrname; // Name of attributes
    public String filename;
    public String dpath;  // Full path of file including filename (direct)
    public String objType;   // Spotter type
    public String dhost;  // Host name where this file exists
    public String hostFullpath; // hostname:dpath
    public String owner;
    public String source;    // source of drag, LOCATOR or HOLDING
    public String bykeywords;  // Base name of protocol to be used.
                               // locaction is expected to take this and
                               // determine the actual protocol to use.
    public int    sourceRow;// the row where this item was in the source.

    // as static, there are only one of these for all ShufflerItem objects.
    // Thus, this is used as the way to save the hostFullpath from when
    // something is double clicked that takes us to DB_AVAIL_SUB_TYPES
    // so that when the DB_AVAIL_SUB_TYPES is double clicked, it will know
    // the hostFullpath to use.
    static private String hostFullpathForAvailTypes=":";
    static private String filenameForAvailTypes="";

    static private RightsList rightsList=null;



    // constructor
    public ShufflerItem() {
        value = new String[3];
        attrname = new String[3];
    }

    public ShufflerItem(String fullpath, String source, String byKeywords) {
        this(fullpath, source);
        
        bykeywords = byKeywords;
    }

    
    // Constructor to allow starting from fullpath string
    // Fill in minimal things.
    public ShufflerItem(String fullpath, String source) {
        value = new String[3];
        attrname = new String[3];

            if(DebugOutput.isSetFor("ShufflerItem")) {
                Messages.postDebug("ShufflerItem showing Return btn if locator is available");
            }

        try {
            String dhost1, dpath1;
            Vector mp = MountPaths.getCanonicalPathAndHost(fullpath);
            dhost1  = (String) mp.get(Shuf.HOST);
            dpath1 = (String) mp.get(Shuf.PATH);

            String hostFullpath = dhost1 + ":" + dpath1;
            
            File file = new File(fullpath);

            String objType = FillDBManager.getType(fullpath);
            if(objType.equals("?")) {
                if(file.isDirectory())
                    objType = "directory";
                else if(file.isFile())
                    objType = "file";
                else
                    objType = "unknown";
            }

            String filename = file.getName();
            // strip suffix off of the filename.
            // Not for automation nor study
            if(!objType.equals(Shuf.DB_AUTODIR) && 
                      !objType.equals(Shuf.DB_STUDY) &&
                      !objType.equals(Shuf.DB_LCSTUDY)) {

                int index = filename.lastIndexOf('.');
                if(index > 0)
                    filename = filename.substring(0, index);
            }

            String owner = FillDBManager.getUnixOwner(fullpath);

            setValue("hostFullpath", hostFullpath);
            setValue("fullpath", dpath1);
            setValue("filename", filename);
            setValue("source", source);
            setValue("dhost", dhost1);
            // If dragged to holding area, we need something set
            // so set the filename.
            setValue("attrname0", "");
            setValue("value0", "");
            setValue("attrname1", "filename");
            setValue("value1", filename);
            setValue("attrname2", "");
            setValue("value2", "");
            setValue("objType", objType);
            setValue("owner", owner);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return;
        }

    }


    public String toString() {
        return("lockStatus " +lockStatus + "\n" +
               "attrname0 " + attrname[0] + "\n" +
               "attrname1 " + attrname[1] + "\n" +
               "attrname2 " + attrname[2] + "\n" +
               "value0 " + value[0] + "\n" +
               "value1 " + value[1] + "\n" +
               "value2 " + value[2] + "\n" +
               "filename " + filename + "\n" +
               "fullpath " + dpath + "\n" +
               "objType " + objType  + "\n" +
               "hostName " + dhost + "\n" +
               "hostFullpath " + hostFullpath + "\n" +
               "owner " + owner + "\n" +
               "source " + source + "\n" +
               "sourceRow " + sourceRow + "\n");
    }

    /************************************************** <pre>
     * Summary: Set one member of this object as per args.
     *
     *
     </pre> **************************************************/
    public void setValue(String var, String val) {
        if(var.equals("lockStatus")) {
            if(value.equals("true"))
                lockStatus = true;
            else
                lockStatus = false;
        }
        else if(var.equals("sourceRow"))
            sourceRow = Integer.parseInt(val);
        else if(var.equals("attrname0"))
            attrname[0] = new String(val);
        else if(var.equals("attrname1"))
            attrname[1] = new String(val);
        else if(var.equals("attrname2"))
            attrname[2] = new String(val);
        else if(var.equals("value0"))
            value[0] = new String(val);
        else if(var.equals("value1"))
            value[1] = new String(val);
        else if(var.equals("value2"))
            value[2] = new String(val);
        else if(var.equals("filename"))
            filename = new String(val);
        else if(var.equals("fullpath"))
            dpath= new String(val);
        else if(var.equals("objType"))
            objType = new String(val);
        else if(var.equals("hostName"))
            dhost = new String(val);
        else if(var.equals("dhost"))
            dhost = new String(val);
        else if(var.equals("hostFullpath"))
            hostFullpath = new String(val);
        else if(var.equals("source"))
            source = new String(val);
         else if(var.equals("owner"))
            owner = new String(val);
      }


    /************************************************** <pre>
     * Summary: Act on this item which must have been D&D or doubled clicked.
     *
     * Details: When this is item is Dragged and Dropped on someplace
     *          or double clicked on, call the action macro or command
     *          which will determine what to do with it.
     *
     *
     </pre> **************************************************/

    public void actOnThisItem(String target, String action, String command) {
        SessionShare sshare;
        StatementDefinition curStatement;
        String actionMacro=null;
        String actionCommand=null;
        String cmd;
        String mpath;

        if(DebugOutput.isSetFor("ShufflerItem")) {
            Messages.postDebug("ShufflerItem action: " + action);
        }

        if (objType == null)
            return;

        if(!fileExists()) {
            Messages.postWarning(hostFullpath 
                                 + " Does Not Exist or is not Mounted.");
            return;
        }

        sshare = ResultTable.getSshare();

        // If double click on a record, change objtype to rec_data.
        if(objType.equals(Shuf.DB_VNMR_RECORD) &&
                     action.equals("DoubleClick")) {
            // If Record, change objType to REC_DATA to a specific
            // statement and params to show only data for this record.

            LocatorHistory lh = sshare.getLocatorHistory();
            // Set History Active Object type to this type.
            lh.setActiveObjType(Shuf.DB_VNMR_REC_DATA, false);

            // Get the statement for changing
            StatementHistory his;
            his = sshare.statementHistory(Shuf.DB_VNMR_REC_DATA);
            if(his == null) {
                Messages.postError("The locator type \'Shuf.DB_VNMR_REC_DATA\'"
                                   + " does not exist.\n    Is the correct "
                                   + "locator_statements_default.xml file "
                                   + "being used?");
                return;
            }
            Hashtable statement;
            statement = his.getLastOfType(Shuf.DB_VNMR_REC_DATA 
                                          + "/by record internal use");

            // Change AttrValue-0 to hostFullpath of the current record without
            // the .REC or .rec
            int index = hostFullpath.indexOf(Shuf.DB_REC_SUFFIX);
            if(index == -1)
                index = hostFullpath.indexOf(Shuf.DB_REC_SUFFIX.toLowerCase());
            if(index == -1) {
                Messages.postError("Cannot find record name in " +
                                   hostFullpath);
            }
            else {
                String record = hostFullpath.substring(0, index);

                statement.put("AttrValue-0", record);
                
                // Change Label to include this filename
                statement.put("Label", "Show data in " + filename);
                // Change the locator to this new modified statement.
                his.append(statement);

            }

            // Show the return icon.
            Shuffler shuffler = Util.getShuffler();
            if(shuffler != null) {
                ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
                shufflerToolBar.showReturnButton(objType);
            }

            return;
        }


        // If double click on an automation, change objtype to study or vnmr_data.
        if(objType.equals(Shuf.DB_AUTODIR) &&
           action.equals("DoubleClick")) {
            // If Autodir, change objType to study or vnmr_data (depending
            // on the subtype attribute, to a specific
            // statement and params to show only data for this automation.

            // Get subtype attribute
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            mpath = MountPaths.getMountPath(dhost, dpath);

            // Bail out if no name
            if(mpath.length() == 0)
                return;

            String subType = dbManager.getAttributeValue(Shuf.DB_AUTODIR,
                                                 mpath, dhost, "subtype");

            LocatorHistory lh = sshare.getLocatorHistory();

            // sub data type is study
            if(subType.equals(Shuf.DB_STUDY) || 
                         subType.equals(Shuf.DB_LCSTUDY)) {
                // Set History Active Object type to this type.
                lh.setActiveObjType(subType, false);
                // Get the statement for changing
                StatementHistory his;
                his = sshare.statementHistory(subType);
                if(his == null) {
                    Messages.postError("The locator type \'" + subType
                                    + "\' does not exist.\n    Is the correct "
                                    + "locator_statements_default.xml file "
                                    + "being used?");
                    return;
                }
                Hashtable statement;
                statement = his.getLastOfType(subType + "/by "
                                              + Shuf.DB_AUTODIR
                                              + " internal use");

                // Change AttrValue-0 to hostFullpath of the current automation
                statement.put("AttrValue-0", hostFullpath);
                // Change Label to include this filename
                statement.put("Label", "Show " + subType + " in " + filename);

                // Change the locator to this new modified statement.
                his.append(statement);
            }
            // Must be vnmr_data
            else {
                // Set History Active Object type to this type.
                lh.setActiveObjType(Shuf.DB_VNMR_DATA, false);
                // Get the statement for changing
                StatementHistory his;
                his = sshare.statementHistory(Shuf.DB_VNMR_DATA);
                if(his == null) {
                    Messages.postError("The locator type \'Shuf.DB_VNMR_DATA\'"
                                       + " does not exist.\n    Is the correct "
                                       + "locator_statements_default.xml file "
                                       + "being used?");
                    return;
                }
                Hashtable statement;
                statement = his.getLastOfType(Shuf.DB_VNMR_DATA + "/by "
                                              + Shuf.DB_AUTODIR
                                              + " internal use");

                // Change AttrValue-0 to hostFullpath of the current automation
                statement.put("AttrValue-0", hostFullpath);
                // Change Label to include this filename
                statement.put("Label", "Show data in " + filename);

                // Change the locator to this new modified statement.
                his.append(statement);
            }
            // Show the return icon.
            Shuffler shuffler = Util.getShuffler();
            ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
            shufflerToolBar.showReturnButton(objType);

            return;
        }
        // I did have LCSTUDY dealt with just like STUDY, but it was 
        // requested to make double click on LCSTUDY call locaction to
        // load it into the study queue.

        // If double click on an study, change objtype to DB_AVAIL_SUB_TYPES.
        if(objType.equals(Shuf.DB_STUDY) && action.equals("DoubleClick") &&
                !target.equals("ReviewViewport") ) {
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            ArrayList typeList = dbManager.getAvailSubTypeList(objType,
                                                               hostFullpath);


            // If there are more than one type, put the types up for
            // the user to choose from.
            if(typeList.size() > 1) {
                // Fill the available subtypes DB table
                dbManager.setAvailSubTypes(typeList);

                // Set History Active Object type to DB_AVAIL_SUB_TYPES.
                LocatorHistory lh = sshare.getLocatorHistory();

                lh.setHistoryToThisType(Shuf.DB_AVAIL_SUB_TYPES, false);

                // Show the return icon.
                Shuffler shuffler = Util.getShuffler();
                ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
                shufflerToolBar.showReturnButton(objType, typeList);

                // We need to save the current hostFullpath.  When an item in
                // the DB_AVAIL_SUB_TYPES list is double clicked on, we need to
                // change to the type clicked on AND we need the hostFullpath
                // from here.
                hostFullpathForAvailTypes = new String(hostFullpath);
                filenameForAvailTypes = new String(filename);
            }
            // If only one type, just switch to that objType
            // If none, switch to vnmr_data
            else {
                // Show the return icon.  Do this before modifying the
                // statement because showReturnButton will save this statement
                // as it is now.
                Shuffler shuffler = Util.getShuffler();
                ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
                shufflerToolBar.showReturnButton(objType);

                LocatorHistory lh = sshare.getLocatorHistory();
                String type;
                // If only one type, just switch to that objType
                if(typeList.size() == 1)
                    type = (String) typeList.get(0);
                // If none, switch to vnmr_data
                else
                    type = Shuf.DB_VNMR_DATA;

                // Set History Active Object type to this type.
                lh.setActiveObjType(type, false);
                // Get the statement for changing
                StatementHistory his;
                his = sshare.statementHistory(type);
                if(his == null) {
                    Messages.postError("The locator type \'" + type + "\'"
                                       + " does not exist.\n    Is the correct "
                                       + "locator_statements_default.xml file "
                                       + "being used?");
                    return;
                }
                Hashtable statement;
                statement = his.getLastOfType(type +"/by " + objType 
                                              + " internal use");

                // Change AttrValue-0 to hostFullpath of the current study
                statement.put("AttrValue-0", hostFullpath);
                // Change Label to include this filename
//                 if(type.equals(Shuf.DB_VNMR_DATA))
//                     statement.put("Label", "Show data in " + filename);
//                 else
//                     statement.put("Label", "Show studies in " + filename);
                statement.put("Label", "Show " + type + " in " + filename);

                // Change the locator to this new modified statement.
                his.append(statement);

            }

            return;
        }

        // If double click on an avail_sub_types, change objtype to type
        // clicked on.
        if(objType.equals(Shuf.DB_AVAIL_SUB_TYPES) &&
                       action.equals("DoubleClick")) {
            // What objType did they click on?
            // Be sure the statement has attribute1 correct
            if(!attrname[1].equals("types")) {
                // The statement should have the middle column = types
                // Something is wrong with the statement.
                Messages.postDebug("Something is wrong with the locator "
                                   + "statement for \'avail_sub_types\'.\n"
                                   + "The Middle column must by \'types\'.");
                return;
            }

            // Get the value for types
            String typesValue = value[1];

            // Set History Active Object type to this type.
            LocatorHistory lh = sshare.getLocatorHistory();
            lh.setActiveObjType(typesValue, false);
            // Get the statement for changing
            StatementHistory his;
            his = sshare.statementHistory(typesValue);
            if(his == null) {
                Messages.postError("The locator type \'" + typesValue + "\'"
                                   + " does not exist.\n    Is the correct "
                                   + "locator_statements_default.xml file "
                                   + "being used?");
                return;
            }
            Hashtable statement;
            statement = his.getLastOfType(typesValue +"/by study internal use");

            // Change AttrValue-0 to hostFullpath of the study we came from
            statement.put("AttrValue-0", hostFullpathForAvailTypes);
            // Change Label to include this filename
            if(typesValue.equals(Shuf.DB_VNMR_DATA))
                statement.put("Label", "Show data in " 
                              + filenameForAvailTypes);
            else
                statement.put("Label", "Show studies in " 
                              + filenameForAvailTypes);

            // Change the locator to this new modified statement.
            his.append(statement);
            // Show the return icon.
            Shuffler shuffler = Util.getShuffler();
            ShufflerToolBar shufflerToolBar = shuffler.shufflerToolBar;
            shufflerToolBar.showReturnButton(objType);

            return;
        }

/*
 *   Protocol should not be available if it is not approved
 *
        // If protocol, do not open unapproved items
        if(objType.equals(Shuf.DB_PROTOCOL)) {
            // Initilize the list once
            if(rightsList == null) {
                boolean masterList = true;
                rightsList = new RightsList(masterList);

            }
            
            // Is this protocol disapproved?  Use name without .xml
            String name;
            if(filename.endsWith(".xml"))
                name = filename.substring(0, filename.length() -4);
            else
                name = filename;
            // For protocols, the keyword in rightsList is equal to name,
            // so use name here for the keyword
            boolean skip = rightsList.approveIsFalse(name);

            if(skip) {
                // yes, skip the action
                Messages.postWarning(filename + " is not approved for this "
                                     + "operator, aborting!");
                return;
            }
        }
 */



        // Get the action macro and action commands
        curStatement = sshare.getCurrentStatementType();
        if(curStatement != null) {
            actionMacro = curStatement.getactionMacro();
            actionCommand = curStatement.getactionCommand();
        }
        else {
            // If not statement, use the default macro cmd
            actionMacro = "locaction";
        }

        if(actionCommand == null || actionCommand.length() == 0) {
            // Get the local mount path for use below.
            mpath = MountPaths.getMountPath(dhost, dpath);

            // Bail out if no name
            if(mpath.length() == 0)
                return;

            // workspaces need just the filename, not the fullpath
            if(objType.equals(Shuf.DB_WORKSPACE)) {
                cmd = actionMacro + "(\'" + action + "\', \'" + objType +
                    "\', \'" + dhost + "\', \'" + filename +
                    "\', \'" + target + "\', \'" + command + "\')";

            }
            // Trash types, need to be transulated so that the macro
            // is given the type of the item inside the trash and the
            // fullpath to the item inside the trash.
            else if(objType.equals(Shuf.DB_TRASH)) {
                TrashInfo tinfo;
                int index;
                String name;


                // We need to use the TrashInfo.objType here and
                // we need to get the full filename including the extension
                // from the TrashInfo item.
                tinfo = TrashInfo.readTrashInfoFile(mpath);
                // null means file was not found
                if(tinfo == null) {
                    Messages.postError(mpath + " not a valid trash file");
                    return;
                }

                index = tinfo.origFullpath.lastIndexOf("/");
                name = tinfo.origFullpath.substring(index +1);

                mpath = UtilB.windowsPathToUnix(mpath);
                cmd = actionMacro + "(\'" + action + "\', \'" + tinfo.objType +
                       "\', \'" + dhost + "\', \'" +
                       mpath  + File.separator + name +
                       "\', \'" + target + "\', \'" + command + "\')";

            }
            else if(source != null && source.equals("PROTOCOLBROWSER") && 
                    target.equals("ReviewViewport")) {
                // For source PROTOCOLBROWSER, we need to run the RQaction command.
                String loadCmd="";
                mpath = UtilB.windowsPathToUnix(mpath);
                if(objType.equals(Shuf.DB_IMAGE_DIR))
                    loadCmd = "loadData";
                else if(objType.equals(Shuf.DB_STUDY))
                    loadCmd = "loadStudyData";
                cmd = "$cmd=`RQaction(\'" + loadCmd + "\', \'" + mpath + "\', "  
                + "\'DoubleClick\') vnmrjcmd(\'LOC protocolBrowserClose\')"
                + "` vnmrjcmd(\'VP 3\', $cmd)";
            }
            else if(source != null && source.equals("PROTOCOLBROWSER") && 
                    target.equals("Default") && objType.equals(Shuf.DB_VNMR_DATA))  {
                
                cmd = actionMacro + "(\'" + action + "\', \'" + objType
                + "\', \'" + dhost + "\', \'" + mpath
                + "\', \'" + target + "\', \'" + command + "\') "
                + "vnmrjcmd(\'LOC protocolBrowserClose\')";

            }
            else { 
                mpath = UtilB.windowsPathToUnix(mpath);
                
                if(bykeywords != null && bykeywords.length() > 0) {
                    cmd = actionMacro + "(\'" + action + "\', \'" + objType +
                            "\', \'" + dhost + "\', \'" + bykeywords +
                                "\', \'" + target + "\', \'" + command + "\'" +
                                  ", \'" + source + "\')";
                }
                else {
                    cmd = actionMacro + "(\'" + action + "\', \'" + objType +
                             "\', \'" + dhost + "\', \'" + mpath +
                             "\', \'" + target + "\', \'" + command + "\'" +
                               ", \'" + source + "\')";
                }
            }
        }
        else {
            cmd = actionCommand;
        }
        if(DebugOutput.isSetFor("ShufflerItem")) {
            Messages.postDebug("actOnThisItem command being sent: " + cmd);
        }

        Util.sendToVnmr(cmd);
    }

    /******************************************************************
     * Summary: Return the fullpath after translating to the mount path
     *  if necessary.
     *
     *
     *****************************************************************/

    public String getFullpath() {
        String mpath;

        mpath = MountPaths.getMountPath(dhost, dpath);
        return mpath;

    }

    /******************************************************************
     * Summary: Set this items fullpath to dpath.
     *
     *
     *****************************************************************/

    public void setFullpath(String newFullpath) {

        dpath = newFullpath;
        hostFullpath = dhost + ":" + dpath;
    }


    // Does the file/directory this item contains, exists?
    // It could have been deleted without the DB knowing about it,
    // or it could be an invalid sym link. It is allowable to have
    // objType as a value for dpath here for the avail_sub_types
    public boolean fileExists() {
        String mpath;
        UNFile file;

        // Get the mount path
        mpath = MountPaths.getMountPath(dhost, dpath);
        
        file = new UNFile(mpath);
        if(file.exists())
            return true;
        else {
            // See if dpath is just the name of an objType
            for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
                if(dpath.equals(Shuf.OBJTYPE_LIST[i])) {
                    // yes, it is a legal objType name here
                    return true;
                }
            }

            // Must not be an objType name
            return false;
        }
    }
    
    // When operators are changed, we set this to null so at next use
    // a new one is created.
    static public void setRightsListNull() {
        rightsList = null;
    }
}
