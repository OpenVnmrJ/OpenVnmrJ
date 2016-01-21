/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Point;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.util.*;
import java.net.*;

import javax.swing.JTextArea;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;

/********************************************************** <pre>
 * Summary: Contain info about one trash entry
 *
 * @author  Glenn Sullivan
 * Details:
 *   -
 *
 </pre> **********************************************************/



public class TrashItem implements  Serializable {

    static private TrashCan trashCan;
    static private String activeAutodir="";
    static private String sqLoaded="";
    public TrashInfo info;
    static public  boolean deleteOkay= false;



    // constructor
    /**************************************************
     * Summary: constructor, Fill all member variables from args except
     * trashpath
     *
     <**************************************************/
     public TrashItem(String fName, String ofpath, String oType, String hName,
                     String own, String ohfpath) {

         info = new TrashInfo(fName, ofpath, oType, hName, own, ohfpath, false);

    }
    public TrashItem(String fName, String ofpath, String oType, String hName,
                     String own, String ohfpath, boolean protocolBrowser) {

        info = new TrashInfo(fName, ofpath, oType, hName, own, ohfpath, protocolBrowser);
    }
    
    // Allow calling without queryResult.  If null, the popup will be created
    // as necessary.
    public boolean trashIt() {
        return trashIt(null);
    }
    

   /*************************************************
     * Summary: Add one TrashItem to the DB and move actual file to trash dir.
     * Allow passing in of query popup result so that if we are to be called
     * multiple times, the caller can ask the question only once.
     <**************************************************/
    public boolean trashIt(String queryResult) {
        String trashDir;
        String infoName;
        String destFilename;
        String destFullpath;
        UNFile trashFile;
        UNFile sourceFile;
        UNFile destFile;
        ObjectOutput out;
        String localhost;
        TrashInfo restoreInfo;
        boolean status;
        String parlib=null;
        Vector mp;
        String dhost=null;
        String dpath=null;
        
        try{
            InetAddress inetAddress = InetAddress.getLocalHost();
            localhost = inetAddress.getHostName();
        }
        catch(Exception e) {
            Messages.postError("Problem getting HostName");
            return false;
        }

        // If the item dragged is an AutoDir and is the active AutoDir,
        // then disallow the trashing of it.  activeAutodir is the mpath
        // for the active one.
        if(info.objType.equals(Shuf.DB_AUTODIR)) {
            try {
                // info will have the dpath and dhost, convert the mpath in
                // activeAutodir to dhost and dpath for the comparison
                if(activeAutodir.length() > 1) {
                    mp = MountPaths.getCanonicalPathAndHost(activeAutodir);
                    dhost = (String) mp.get(Shuf.HOST);
                    dpath = (String) mp.get(Shuf.PATH);
                    if(info.origHostFullpath.equals(dhost + ":" + dpath)) {
                        Messages.postWarning("Cannot remove active autodir, " 
                                             + activeAutodir);
                        return false;
                    }
                }

                // If the autodir is currently loaded in the sq, clear the
                // sq along with removing the autodir
                if(sqLoaded.length() > 1) {
                    mp = MountPaths.getCanonicalPathAndHost(sqLoaded);
                    dhost = (String) mp.get(Shuf.HOST);
                    dpath = (String) mp.get(Shuf.PATH);
                    if(info.origHostFullpath.equals(dhost + ":" + dpath)) {
                        Util.sendToVnmr("vnmrjcmd(\'SQ delete all\')");
                    }
                }
            }
            catch (Exception e) {
                Messages.postError("Problem removing " + dhost + ":" + dpath);
                Messages.writeStackTrace(e);
                return false;
            }
        }

        if(isTrashItem(info.origFullpath)){
        	info.objType=new String(Shuf.DB_TRASH);        	
        }
        
        // If item dragged to trash is already a 'trash' item, then
        // just delete it.
        if(info.objType.equals(Shuf.DB_TRASH)) {
            // When the objtype is trash, the origFullpath will be
            // actual fullpath for the trash directory.
            status = delete(info.origHostFullpath);

            // Cause another shuffle so that this item disappears.
            SessionShare sshare = ResultTable.getSshare();
            sshare.statementHistory().updateWithoutNewHistory();

            return status;
        }

        // Be sure this file belongs to the current user.
        String user = System.getProperty("user.name");
        if(!info.owner.equals(user) && !info.objType.equals(Shuf.DB_VNMR_RECORD)) {
            Messages.postError("Cannot delete another user's files: " +
                               info.origFullpath);
            return false;
        }
        
        // Check to see that we have permission to delete this/
        UNFile pathFile = new UNFile(info.origFullpath);
        if(!pathFile.canWrite()) {
            Messages.postError("Permission Denied deleting: " +
                    info.origFullpath);
            return false;
        }


        if(info.objType.equals(Shuf.DB_WORKSPACE)) {
            // Deletion of exp1 is not allowed.
            if(info.filename.equals("exp1")) {
                Messages.postError("Cannot delete exp1");
                return false;
            }

            // Get number of exp
            String num = info.filename.substring(3);


            // Delete it.  Do not put it into the trash DB.
            // If
            Util.sendToVnmr("delexp(" + num + ")");
            return true;
        }

        // For Protocols, there may also be a parlib file.  Get its name
        // if one exist for use below.  If type is composite, then there
        // will be none.
        if(info.objType.equals(Shuf.DB_PROTOCOL)) {
            // Get the type from the DB.
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            // The fullpath here needs to be the mountpath and 
            // info.origFullpath is dpath
            String mopath = MountPaths.getMountPath(info.dhost, info.origFullpath);
            String type = dbManager.getAttributeValue(info.objType,
                                         mopath, localhost, "type");
            if(type == null || !type.equals("composite")) {
                String mpath;
                // File access required the local mounted path
                // Get the host from info.origHostFullpath
                int index = info.origHostFullpath.indexOf(":");
                dhost = info.origHostFullpath.substring(0, index);
                mpath = MountPaths.getMountPath(dhost, info.origFullpath);
                parlib = getProtocolParlib(mpath);
            }
            else {
                parlib = null;
            }
            
            // Don't allow dragging to trash of system protocols, this is only
            // for user protocols.
            if(!info.origFullpath.startsWith(FileUtil.usrdir())) {
                Messages.postError("Can only drag this users protocols to trash.\n"
                    + "Cannot discard " + info.origFullpath);
                return false;
            }
            
            // Were we called from the protocol browser?
            if(info.protocolBrowser) {
                
                if(type.equals("study card")) {
                    new DeleteQuery("   Delete the study card and all its components?    ", null);
                }
                else {
                // Confirmer popup
                    new DeleteQuery("   Delete Protocol?   ", null);
                }

                // The result of the popup will be in deleteOkay
                if(!deleteOkay)
                    return false;

                ExpSelector expSelector = ExpSelector.expSelector;
                if(expSelector != null)
                    // Clean up the ExpSel if it exists
                    ExpSelector.removeProtocolFromExpSelFile(info.filename);
            }
            else {
                // Must have be called from the Exp Sel
                // Remove the protocol, or just the entry in the Experiment Selector?
                if(queryResult == null) {
                    new DeleteQuery("   Delete File?   ", trashCan);
                    // The result of the popup will be in deleteOkay
                    if(TrashItem.deleteOkay)
                        queryResult = "delete";
                    else
                        queryResult = "cancel";
                }
                // The result of the popup will be in queryResult
                if(queryResult.equals("cancel"))
                    return false;
                else if(queryResult.equals("entry")) {
                    ExpSelector.removeProtocolEntryFromExpSelFile(info.filename);
                    return false;
                }
                else if(queryResult.equals("protocol") || queryResult.equals("delete")) {
                    // for protocols, remove references to this protocol from
                    // the ExperimentSelector_user.xml file
                    ExpSelector.removeProtocolFromExpSelFile(info.filename);
                }
            }
        }
        if(info.objType.equals(Shuf.DB_VNMR_RECORD)) {
            // Don't put records in the locator trash.
            // Just have vnmrbg delete them and have it log the event
            String mpath = MountPaths.getMountPath(info.dhost, info.origFullpath);
            Util.sendToVnmr("deleteREC(\'" + mpath + "\')");
            
            // remove the item from the DB
            DBCommunInfo comminfo = new DBCommunInfo();
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            dbManager.removeEntryFromDB(info.objType,
                                  info.origHostFullpath, comminfo);

            // Cause another shuffle so that this item disappears.
            SessionShare sshare = ResultTable.getSshare();
            sshare.statementHistory().updateWithoutNewHistory();
            
            return true;  
        }
        
        // Get a unique name in which to save this item.
        trashDir = getUniqueName();
        try {
            mp = MountPaths.getCanonicalPathAndHost(trashDir);
            dhost = (String) mp.get(Shuf.HOST);
            dpath = (String) mp.get(Shuf.PATH);
        }
        catch (Exception e) {
            dhost = localhost;
            dpath = trashDir;
        }

        info.fullpath = dpath;
        info.hostFullpath = dhost + ":" + dpath;
        trashFile = new UNFile(trashDir);

        // Name of file for trash info
        infoName = trashDir + File.separator + "trashinfo";

        // Create the directory to hold this trash item
        trashFile.mkdirs();


        // Get a File for the file being trashed
        String mpath;
        // Must use mount paths
        mpath = MountPaths.getMountPath(info.dhost, info.origFullpath);

        sourceFile = new UNFile(mpath);

        destFilename = sourceFile.getName();
        destFullpath = trashDir + File.separator + destFilename;

        // If Windows, we need a windows path for File
        if(UtilB.OSNAME.startsWith("Windows"))
            destFullpath = UtilB.unixPathToWindows(destFullpath);
        destFile = new UNFile(destFullpath);
        try {

            if(sourceFile.canWrite() && trashFile.canWrite()) {
                // Move to the trash directory (Java rename fails if going
                // across file systems)
                String[] cmd = {UtilB.SHTOOLCMD, UtilB.SHTOOLOPTION,
                        "mv " + mpath + " " + destFullpath};
                Process proc = null;
                try {
                    Runtime rt = Runtime.getRuntime();
                    proc = rt.exec(cmd);
                    if(proc != null) {
                        String strg = "";
                        InputStream istrm = proc.getInputStream();
                        if (istrm == null) {
                            if(DebugOutput.isSetFor("trash")) 
                                Messages.postDebug("Moved " + mpath + " To " + destFullpath);

                        }
                        else {
                            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));
                            strg = bfr.readLine();
                            if(DebugOutput.isSetFor("trash")) 
                                Messages.postDebug("Move cmd returned: " + strg);
                        }

                        proc.waitFor();
                    }
                }
                finally {
                    // It is my understanding that these streams are left
                    // open sometimes depending on the garbage collector.
                    // So, close them.
                    if(proc != null) {
                        OutputStream os = proc.getOutputStream();
                        if(os != null)
                            os.close();
                        InputStream is = proc.getInputStream();
                        if(is != null)
                            is.close();
                        is = proc.getErrorStream();
                        if(is != null)
                            is.close();
                    }
                }
                

                if(DebugOutput.isSetFor("trash")) {
                    Messages.postDebug("Trash moving "
                                       + sourceFile.getCanonicalPath()
                                       + " to\n" + destFile.getCanonicalPath());
                }
            }
            else {
                Messages.postError("Permission Denied moving: " +
                                   mpath + " to " +
                                   destFullpath);
                // Remove the file and dir we started
                delete(info.hostFullpath);
                return false;
            }
        }
        catch (Exception e) {
            Messages.postError("Problem moving " + mpath + " to " +
                               destFullpath);
            Messages.writeStackTrace(e);
            // Remove the file and dir we started
            delete(info.hostFullpath);
            return false;
        }

        // Remove from standard locator tables
        // First, be sure it is in the DB
        ShufDBManager dbManager = ShufDBManager.getdbManager();
        status = dbManager.isDBKeyUnique(info.objType,
                                                 info.origHostFullpath);
        boolean success = false;
        DBCommunInfo comminfo = new DBCommunInfo();

        if(status == false) {
            // The entry exists, remove it.
            success = dbManager.removeEntryFromDB(info.objType,
                                          info.origHostFullpath, comminfo);
            if(!success) {
                Messages.postError("Problem removing " + info.origFullpath +
                                     " from database.");
                return false;
            }
            else {
                // Cause another shuffle so that this item disappears.
                SessionShare sshare = ResultTable.getSshare();
                sshare.statementHistory().updateWithoutNewHistory();
            }
        }
        
        // Now that it is out of the DB, update the protocolBrowser if we
        // were called from there
        if(info.protocolBrowser) {
            ProtocolBrowser protocolBrowser = ExpPanel.getProtocolBrowser();
            if(protocolBrowser != null) {
                protocolBrowser.updateResultPanel();
            }
        }

        // For Protocols, there may also be a parlib file.
        if(info.objType.equals(Shuf.DB_PROTOCOL) && parlib != null) {
            // The parlib file will be in either a users dir, or the
            // system dir.  This is determined by the location of the
            // protocol.xml file itself.  So, look at the path for the
            // xml file and take the part before 'template'.
            int index = info.origFullpath.indexOf("templates");
            String dparpath;
            dpath = info.origFullpath.substring(0, index);
            // Add 'parlib' to the path and use this to look for the
            // parlib file
            dparpath = dpath.concat("parlib" + File.separator);

            // File access requires the local mounted path
            mpath = MountPaths.getMountPath(info.dhost, dparpath);

            UNFile file = new UNFile(mpath + parlib);
            if(file.canWrite()) {
                String dest = trashDir + File.separator + parlib;
                destFile = new UNFile(dest);
                // Move the parlib file to the same directory with the
                // .xml file.
                file.renameTo(destFile);

                if(DebugOutput.isSetFor("trash")) {
                    Messages.postDebug("Trash moving " + mpath + parlib
                                       + " to\n" + dest);
                }

                // Save info in TrashInfo
                info.parFile = parlib;
                info.dparPath = dparpath + parlib;

                // Remove from standard locator tables
                // First, be sure it is in the DB
                status = dbManager.isDBKeyUnique(Shuf.DB_VNMR_PAR,
                                            info.dhost + ":" + info.dparPath);
                success = false;
                comminfo = new DBCommunInfo();

                if(status == false) {
                    // The entry exists, remove it.
                    success = dbManager.removeEntryFromDB(Shuf.DB_VNMR_PAR,
                                           info.dhost + ":" + info.dparPath,
                                           comminfo);

                    if(!success) {
                        Messages.postError("Problem removing "  + info.dparPath
                                           + " from database.");
                        return false;
                    }
                }
            }
        }
        // For Protocols, there may also be a studycardlib directory.
        if(info.objType.equals(Shuf.DB_PROTOCOL)) {
            // The studycardlib file location is determined by the location of the
            // protocol.xml file itself.  So, look at the path for the
            // xml file and take the part before 'template'.
            int index = info.origFullpath.indexOf("templates");
            String dscpath;
            dpath = info.origFullpath.substring(0, index);
            // Add 'studycardlib' to the path and use this to look for the
            // studycardlib file
            dscpath = dpath.concat("studycardlib" + File.separator);

            // File access requires the local mounted path
            mpath = MountPaths.getMountPath(info.dhost, dscpath);
            index = destFilename.indexOf(".xml");
            String baseName = destFilename.substring(0, index);

            UNFile file = new UNFile(mpath + baseName);
            if(file.canWrite()) {
                String dest = trashDir + File.separator + baseName;
                destFile = new UNFile(dest);
                // Move the studycardlib file to the same directory with the
                // .xml file.
                file.renameTo(destFile);

                if(DebugOutput.isSetFor("trash")) {
                    Messages.postDebug("Trash moving " + mpath + parlib
                                       + " to\n" + dest);
                }                
            }
        }


        // Write the TrashInfo to a file in the new directory.
        info.writeTrashInfoFile(infoName);

        // Add to trash locator table
        success = dbManager.addFileToDB(Shuf.DB_TRASH, info.filename,
                                          trashDir, info.owner);

        // Cause another shuffle so that these items disappear.
        SessionShare sshare = ResultTable.getSshare();
        sshare.statementHistory().updateWithoutNewHistory();

        // Update the trash can icon as necessary.
        updateTrashCanIcon();

        return success;

    }

    /**************************************************
     * Summary: Generate a unique name based on time of day in msec.
     *
     *
     **************************************************/
    private String getUniqueName() {   
        UNFile file;
        String usrdir=FileUtil.usrdir();
        String dir;
        
        if(FillDBManager.locatoroff){ // need to use the Browser for trash IO
        	int index=1;
        	// use the filename name of the deleted object as the trash dir name
            dir = usrdir + File.separator + "trash" + File.separator + info.filename;
            file = new UNFile(dir);
            // If the directory already exists, loop here incrementing an index count
            // until we find a unique name.
            while(file.exists()) {
                dir = usrdir + File.separator + "trash" + File.separator +
                info.filename+"["+String.valueOf(++index)+"]";
                file = new UNFile(dir);
            }
        }
        else{
            // Get the time trashed in msec as a long.
            long time = info.dateTrashed.getTime();
            // Create a file using the fullpath ending with the time string.
            dir = usrdir + File.separator + "trash" + File.separator +
                		String.valueOf(time);
            // If the directory already exists, loop here incrementing the
            // time until we find a unique name.
            file = new UNFile(dir);
            while(file.exists()) {
                dir = usrdir + File.separator + "trash" + File.separator
                +String.valueOf(++time);
                file = new UNFile(dir);
            }
        }
        return dir;
    }

    /**************************************************
     * Summary: return path to trash directory.
     *
     **************************************************/
    public static String trashDir(){
	    String trashdir =  FileUtil.usrdir()+ File.separator + "trash";
	    if(UtilB.iswindows())
	    	trashdir=UtilB.windowsPathToUnix(trashdir);
	    return trashdir;
    } 

    /**************************************************
     * Summary: return true if path points to a trash directory.
     *
     **************************************************/
    public static boolean isTrashItem(String path){
	    UNFile tfile= new UNFile(path);
	    if(trashDir().equals(tfile.getParent())){
	    	return true;
	    }
	    return false;
    }

    /**************************************************
     * Summary: Recursively delete a trash directory from the disk.
     *
     **************************************************/
    public static void deleteAll(File dir){
    	if(dir.isDirectory()){
    		File[] files=dir.listFiles();
    		for(int i=0;i<files.length;i++)
    			deleteAll(files[i]);
    		if(!dir.delete())
     			Messages.postError("Error deleting directory: " +dir.getPath());
    		else if(DebugOutput.isSetFor("trash"))
    			System.out.println("deleting directory: " +dir.getPath());
    	}
    	else{
    		if(!dir.delete())
    			Messages.postError("Error deleting file: " +dir.getPath());  
    		else if(DebugOutput.isSetFor("trash"))
    			System.out.println("deleting file: " +dir.getPath());
    	}
    }
    /**************************************************
     * Summary: Recursively delete a trash entry from the disk.
     *
     *
     **************************************************/
    static public boolean delete(String dhostFullpath) {
        UNFile infoFile;
        Runtime rt;
        Process chkit;
        UNFile pathFile;
        String mpath;
        int index;
        

        mpath = MountPaths.getMountPath(dhostFullpath);

        // Be sure this is a trash directory.  Check for the file
        // mpath/trashinfo exist as proof that it is a trash directory.
        infoFile = new UNFile(mpath + "/trashinfo");

        if(infoFile.exists()) {
            try {
                TrashInfo  trashInfo = TrashInfo.readTrashInfoFile(mpath);

                // null means file was not found
                if(trashInfo == null)
                    return false;

                // If obytype is vnmr_record, then call deleteREC so that
                // an audit trail is kept.
                if(trashInfo.objType.equals(Shuf.DB_VNMR_RECORD) ||
                	trashInfo.objType.equals(Shuf.DB_VNMR_REC_DATA)) {
                    // We need the name of the record down inside of the
                    // trash directory so that deleteREC can work on it.
                    // get that from origFullpath.
                    index = trashInfo.origFullpath.lastIndexOf(File.separator);
                    String name = trashInfo.origFullpath.substring(index +1);

                    String fullpath = trashInfo.fullpath + File.separator + name;
                    Util.sendToVnmr("deleteREC(\'" + fullpath + "\')");

                    // remove the item from the DB
                    DBCommunInfo comminfo = new DBCommunInfo();
                    ShufDBManager dbManager = ShufDBManager.getdbManager();
                    dbManager.removeEntryFromDB(trashInfo.objType,
                                          trashInfo.origHostFullpath, comminfo);

                    // Cause another shuffle so that this item disappears.
                    SessionShare sshare = ResultTable.getSshare();
                    sshare.statementHistory().updateWithoutNewHistory();

                    // We have removed the .rec/.REC file, now continue
                    // below and remove the trash directory.

                }

                // Be sure this file belongs to the current user.
                String user = System.getProperty("user.name");
                // If objtype is REC, then don't go by this user test
                if(!trashInfo.owner.equals(user) && !trashInfo.objType.equals(Shuf.DB_VNMR_RECORD)) {
                    Messages.postError("Cannot delete another user's " +
                                           "files: " + mpath);
                    return false;
                }
                // Check to see that we have permission to delete this/
                boolean success = false;
                pathFile = new UNFile(mpath);
                if(pathFile.canWrite()) {
                    //infoFile.delete();
                    //pathFile.delete();
                	deleteAll(pathFile);
                }
                else {
                    Messages.postError("Permission Denied deleting: " +
                                       mpath);
                    return false;
                }

                InetAddress inetAddress = InetAddress.getLocalHost();
                String localhost = inetAddress.getHostName();

                // Remove this entry from the DB
                ShufDBManager dbManager = ShufDBManager.getdbManager();
                boolean status = dbManager.isDBKeyUnique(Shuf.DB_TRASH,
                                                       dhostFullpath);
                
                DBCommunInfo comminfo = new DBCommunInfo();

                if(status == false) {
                    success = dbManager.removeEntryFromDB(Shuf.DB_TRASH,
                                           dhostFullpath, comminfo);
                    if(!success) {
                        Messages.postError("Problem removing " + mpath +
                                           " from database.");
                        return false;
                    }
                    else {

                    }
                }

                // Update the trash can icon as necessary;
                TrashItem.updateTrashCanIcon();

                return true;
            }
            catch (Exception e) {
                Messages.postError("Problem deleting " + mpath);
                Messages.writeStackTrace(e);
                return false;
            }
        }
        return false;
    }

    // The arg here should be the mount path for the current host
    static public boolean restore(String filepath) {
        TrashInfo restoreInfo;
        UNFile trashFile;
        UNFile destFile;
        UNFile destDir;
        String mpath;
        String localhost;
        String mTrashPath;


        // Get the trash info for this filepath
        restoreInfo = TrashInfo.readTrashInfoFile(filepath);
        // null means file was not found
        if(restoreInfo == null)
            return false;

        try{
            InetAddress inetAddress = InetAddress.getLocalHost();
            localhost = inetAddress.getHostName();
        }
        catch(Exception e) {
            Messages.postError("Problem getting HostName");
            return false;
        }

        // Get the actual filename or directory name from the end of
        // origFullpath
        int index = restoreInfo.origFullpath.lastIndexOf('/');
        String name = restoreInfo.origFullpath.substring(index+1);

        // Get the directory where the file will be restored to for checking
        // write permission.  In case it is not on this host, we need
        // to get the mounted directory path.
        String dir = restoreInfo.origFullpath.substring(0, index);
        String mdir = MountPaths.getMountPath(restoreInfo.dhost, dir);

        mpath = MountPaths.getMountPath(restoreInfo.dhost,
                                        restoreInfo.origFullpath);
        destDir = new UNFile(mdir);

        // Now get the mounted path for the trash file
        mTrashPath = MountPaths.getMountPath(restoreInfo.hostFullpath);

        trashFile = new UNFile(mTrashPath + File.separator + name);
        destFile = new UNFile(mdir + File.separator + name);
        try {
            // Does this destination file already exist?
            if(destFile.exists()) {
                Messages.postError("The file " + restoreInfo.origFullpath +
                                   " already exists.  Aborting restoration");
                return false;
            }

            if(trashFile.canWrite() && destDir.canWrite()) {
                // Move to original directory
                trashFile.renameTo(destFile);

                if(DebugOutput.isSetFor("trash")) {
                    Messages.postDebug("Moving "
                                +  mTrashPath + File.separator + name
                                + " to\n" + mpath);
                }

                
            }
            else {
                Messages.postError("Permission Denied moving: " +
                                   mTrashPath  +
                                   File.separator + name + " to " +
                                   mpath);
               return false;
            }
            
            // Update the Exp Selector by having it re-create the ES_user.xml file
            // so that it included the restored file.  The timer update will find
            // the updated ES_user.xml file and cause an update.
            // This takes a few seconds.  It seems common for the ES timer to
            // find the ES_user.xml file update at least twice and thus it does
            // multiple updates after this call.
            Util.getAppIF().sendToVnmr("updateExpSelector");

            if(restoreInfo.objType.equals(Shuf.DB_PROTOCOL) &&
               (restoreInfo.parFile != null) &&
               (restoreInfo.dparPath != null)) {

                // If there is there a .par file that needs to be restored.
                String filePath = mTrashPath + File.separator + restoreInfo.parFile;
                UNFile pFile = new UNFile(filePath);

                String mparPath = restoreInfo.dparPath; // in case of error
                try {
                    mparPath = MountPaths.getMountPath(restoreInfo.dhost,
                                                       restoreInfo.dparPath);
                    // Move this file back to where it came from
                    destFile = new UNFile(mparPath);
                    pFile.renameTo(destFile);

                    if(DebugOutput.isSetFor("trash")) {
                        Messages.postDebug("Moving " +  filePath
                                           + " to\n" + mparPath);
                    }
                    // Add to standard DB table
                    ShufDBManager dbManager = ShufDBManager.getdbManager();
                    dbManager.addFileToDB(Shuf.DB_VNMR_PAR,
                                     restoreInfo.parFile, restoreInfo.dparPath,
                                     restoreInfo.owner);

                }
                catch (Exception e) {
                    Messages.postError("Problem moving " +  filePath
                                       + " to\n" + mparPath);
                    Messages.writeStackTrace(e);
                    return false;
                }
            }
            // If there is a directory, it should be the studycardlib directory
            // restore it.
            if(restoreInfo.objType.equals(Shuf.DB_PROTOCOL)) {
                // Get the name of the protocol minus the .xml
                index = name.indexOf(".xml");
                String scName = name.substring(0, index);

                // Does this directory exist in restoreInfo.fullpath?
                String srcPath = restoreInfo.fullpath + File.separator + scName;
                UNFile scrfile = new UNFile(srcPath);
                
                // If this studycardlib directory exists
                if(scrfile.exists()) {
                    // The studycardlib file location is determined by the location of the
                    // protocol.xml file itself.  So, look at the path for the
                    // xml file and take the part before 'template'.
                    index = restoreInfo.origFullpath.indexOf("templates");
                    String dscpath;
                    String dpath;
                    dpath = restoreInfo.origFullpath.substring(0, index);
                    // Add 'studycardlib' to the path 
                    dscpath = dpath.concat("studycardlib" + File.separator);

                    // File access requires the local mounted path
                    String dmpath = MountPaths.getMountPath(restoreInfo.dhost, dscpath);

                    UNFile destfile = new UNFile(dmpath + scName);
                    
                    scrfile.renameTo(destfile);
                    if(DebugOutput.isSetFor("trash")) {
                        Messages.postDebug("Trash moving " + srcPath
                                           + " to\n" + dmpath + scName);
                    }
                }


            }
            // Add to standard DB table
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            String objType = restoreInfo.objType;
            // If not a real locator type, don't try to add it.
            if(!objType.equals("unknown") && !objType.equals("directory") &&
                    !objType.equals("file")){
                dbManager.addFileToDB(restoreInfo.objType, name,
                                      mpath, restoreInfo.owner);
            }

        }
        catch (Exception e) {
            Messages.postError("Problem moving "
                               + restoreInfo.fullpath + File.separator + name +
                               " to\n" + mTrashPath);
            Messages.writeStackTrace(e);
            return false;
        }
        // Remove the trash file and dir
        delete(restoreInfo.hostFullpath);

        // Update the trash can icon as necessary.
        TrashItem.updateTrashCanIcon();
        
        // Update the ProtocolBrowser if it exists
        ProtocolBrowser protocolBrowser = ExpPanel.getProtocolBrowser();
        if(protocolBrowser != null) {
            protocolBrowser.updateResultPanel();
        }

        return true;

    }
    

     /******************************************************************
     * Summary: Save the TrashCan item in a static so that we can
     *          change the icon when needed.
     *
     *
     *****************************************************************/
    static public void setTrashCan(TrashCan tc) {
        trashCan = tc;
    }


    /******************************************************************
     * Summary: Change the TrashCan icon to the full one.
     *
     *
     *****************************************************************/
    static public void fullTrashCanIcon() {
        trashCan.setIcon(Util.getImageIcon("trashcanFull.gif"));
    }

    /******************************************************************
     * Summary: Change the TrashCan icon to the empty one.
     *
     *
     *****************************************************************/
    static public void emptyTrashCanIcon() {
        trashCan.setIcon(Util.getImageIcon("trashcan.gif"));
    }

    static public void updateTrashCanIcon() {
        int numRows;

        ShufDBManager dbManager = ShufDBManager.getdbManager();
        numRows = dbManager.getNumTrashRows();
        if(numRows == 0)
            emptyTrashCanIcon();
        else
            fullTrashCanIcon();
    }
    /******************************************************************
     * Summary: Get parfile name from protocol.
     *
     *  Get name in the protocol xml file from the entry as follows:
     *       'macro="sqexp('sems1a_05')"
     *  where sems1s_05 would be the parfile name.
     *****************************************************************/

    public String getProtocolParlib (String fullpath) {
        String          inLine=null;
        BufferedReader  in;
        int             index;
        String          string=null;
        String          path;

        // If Windows, we need a windows path
        if(UtilB.OSNAME.startsWith("Windows"))
            path = UtilB.unixPathToWindows(fullpath);
        else
            path = new String(fullpath);

        // Open the protocol xml file
        try {
            UNFile file = new UNFile(path);
            in = new BufferedReader(new FileReader(file));
        } catch(Exception e) {
            Messages.postError("Problem opening " + path);
            Messages.writeStackTrace(e, "Error caught in getProtocolParlib");
            return null;
        }

        // Read until '<action' is found
        // Get the line/s to parse from the first occurence of a '<action'
        // continuing to the next '>'.
        try {
            // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
               inLine = inLine.trim();
               if (string == null && inLine.length() > 1 &&
                                         inLine.startsWith("<action")) {
                   // We are at the start of our string, save it.
                   string = new String(inLine);

                   // If this line contains the closing '>', then
                   // stop reading now.
                   if ((index = inLine.indexOf('>')) != -1) {
                       // Only take the string up to the '>'
                       string = inLine.substring(0, index);
                       break;
                   }
               }
               // concatonate to string until '>' is found.
               else if (string != null && inLine.indexOf('>') == -1) {
                   string = string.concat(" " + inLine);
               }
               // Include the line up to the '>'
               else if (string != null && (index = inLine.indexOf('>')) != -1) {
                   // Concatonate up to the '>' and include a space.
                   string = string.concat(" " + inLine.substring(0, index));
                   break;
               }

            }
            in.close();

            if(string == null)
                throw (new Exception("Badly formated xml file"));
        }
        catch (Exception e) {
            if(!ExitStatus.exiting()) {
                Messages.postError("Problem reading " + path);
                Messages.postError("addXmlFileToDB failed on line: '" +
                                   inLine + "'");
                Messages.writeStackTrace(e);
            }

            try {
                in.close();
            }
            catch(Exception  eio) {}
            return null;
        }
        // We should now have something like
        //    <action exp="epidw" time="4 sec" macro="sqexp('epidw')
        // Take string starting at the 'macro=' following <action.
        index = string.indexOf("macro=");
        if(index == -1)
            return null;

        string = string.substring(index + "macro=".length());

        // Now look for exp(\'
        index = string.indexOf("exp(\'");
        if(index == -1)
            return null;

        string = string.substring(index + "exp(\'".length());

        // Now the string up to the next single qoute will be the parlib
        index = string.indexOf("\'");
        if(index == -1)
            return null;

        string = string.substring(0, index);


        return string + ".par";
    }

    static public void setActiveAutodir(String value) {
        activeAutodir = new String(value);
        if(DebugOutput.isSetFor("TrashItem")) 
            Messages.postDebug("TrashItem.setActiveAutodir() setting "
                               + "activeAutodir to\n    " + value);
    }

    static public void setSqLoaded(String value) {
        sqLoaded = new String(value);
        if(DebugOutput.isSetFor("TrashItem")) 
            Messages.postDebug("TrashItem.setSqLoaded() setting "
                               + "sqLoaded to\n    " + value);
    }

    public String toString() {
        return info.toString();
    }

    // This is only used if we were called from the ProtocolBrowser, so is not generic
    static public class DeleteQuery extends ModalDialog implements ActionListener
    {

        public DeleteQuery(String message, Component parent) {
            super("Delete?");

            okButton.addActionListener(this);
            cancelButton.addActionListener(this);

            JTextArea msg = new JTextArea(message);
            
            getContentPane().add(msg, BorderLayout.NORTH);
            pack();
            Point loc;
            if(parent == null) {
                // Set the location of this popup to that of the main ProtocolBrowser panel
                loc = ProtocolBrowserPanel.getParentLocation();
            }
            else {
                loc = parent.getLocationOnScreen();
            }
            if(loc != null)
                setLocation(loc);

            setVisible(true);

        }


        public void actionPerformed(ActionEvent e) {

            String cmd = e.getActionCommand();

            if (cmd.equalsIgnoreCase("ok")) {
                deleteOkay = true;
            }
            else if (cmd.equalsIgnoreCase("cancel"))
            {
                deleteOkay = false;
            }

            setVisible(false);
        }
    }



}
