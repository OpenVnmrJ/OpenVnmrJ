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
import java.net.*;
import java.io.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.bo.*;
import vnmr.sms.*;

public class AddToLocatorViaThread extends Thread {
    String objType;
    ArrayList filename;
    ArrayList fullpath;
    String owner;
    boolean highlight;
    String attr = null;
    String attrValue = null;
    boolean locDisplayUpdate = true;
    boolean trayUpdate = false;
    ExpViewIF expIf=null;

    public AddToLocatorViaThread(String objType, ArrayList filename, 
                         ArrayList fullpath, String owner, boolean highlight) {
	this.objType = objType;
 	this.filename = filename;
 	this.fullpath = fullpath;
	this.owner = owner;
	this.highlight = highlight;
    }

    public AddToLocatorViaThread(String objType, ArrayList filename, 
                         ArrayList fullpath, String owner, boolean highlight,
                         String attr, String attrValue) {
	this.objType = objType;
 	this.filename = filename;
 	this.fullpath = fullpath;
	this.owner = owner;
	this.highlight = highlight;
        this.attr = attr;
        this.attrValue = attrValue;
    }

    public AddToLocatorViaThread(String objType, String filename, 
                         String fullpath, String owner, boolean highlight) {

        ArrayList fp = new ArrayList();
        fp.add(fullpath);
        ArrayList fn = new ArrayList();
        fn.add(filename);

	this.objType = objType;
 	this.filename = fn;
 	this.fullpath = fp;
	this.owner = owner;
	this.highlight = highlight;
 
    }

    public AddToLocatorViaThread(String objType, String filename, 
                         String fullpath, String owner, boolean highlight,
                         String attr, String attrValue) {

        ArrayList fp = new ArrayList();
        fp.add(fullpath);
        ArrayList fn = new ArrayList();
        fn.add(filename);

	this.objType = objType;
 	this.filename = fn;
 	this.fullpath = fp;
	this.owner = owner;
	this.highlight = highlight;
        this.attr = attr;
        this.attrValue = attrValue;
 
    }

    public AddToLocatorViaThread(String objType, ArrayList filename, 
                         ArrayList fullpath, String owner, boolean highlight,
                         String attr, String attrValue, 
                         boolean locDisplayUpdate, boolean trayUpdate, 
                         ExpViewIF expIf) {

	this.objType = objType;
 	this.filename = filename;
 	this.fullpath = fullpath;
	this.owner = owner;
	this.highlight = highlight;
        this.attr = attr;
        this.attrValue = attrValue;
        this.locDisplayUpdate = locDisplayUpdate;
        this.trayUpdate = trayUpdate;
        this.expIf = expIf;
    }

    public void run() {
        boolean success=false;
        boolean status;
        String name, path=null;
        ShufDBManager dbManager=null;


        // If the locator is not being used, get out of here
        if(FillDBManager.locatorOff())
            return;

        // Catch all otherwise uncaught exceptions
        try {
            dbManager = ShufDBManager.getdbManager();
            java.util.Date starttime=null;
            if(DebugOutput.isSetFor("AddToLocatorViaThread") || 
                       DebugOutput.isSetFor("locTiming"))
                starttime = new java.util.Date();

            for(int i=0; i < fullpath.size(); i++) {
                try {
                    name = (String) filename.get(i);
                    path = (String) fullpath.get(i);
                    boolean notify=false;
                    status = dbManager.addFileToDB(objType, name, path, 
                                                   owner, notify);
                    if(DebugOutput.isSetFor("AddToLocatorViaThread")) {
                        if(status)
                            Messages.postDebug("AddToLocatorViaThread added "
                                               + path);
                        else
                            Messages.postDebug("AddToLocatorViaThread failed "
                                               + "to add " + path);
                    }

                    // If requested, set an attr to attrValue
                    if(status == true && attr != null && attrValue != null) {
                        dbManager.setNonTagAttributeValue(objType, path, 
                                  dbManager.localHost, attr, "text", attrValue);

                        // If the attribute is 'automation', then set the
                        // attribute autodir also.  
                        if(attr.equals(Shuf.DB_AUTODIR)) {
                            // The value for automation will be the 
                            // dhost:dpath.  For autodir, we need to
                            // translate to mhost:mpath.  Be sure it at least
                            // has a ":".
                            if(attrValue.indexOf(":") > 0) {
                                String mpath;
                                mpath = MountPaths.getMountPath(attrValue);
                                dbManager.setNonTagAttributeValue(objType, 
                                           path, dbManager.localHost, 
                                           "autodir", "text", mpath);
                            }
                        }
                    }

                    // If any files were added successfully, then success=true.
                    if(status || success)
                        success = true;
                }
                catch (Exception e) {
                    Messages.postError("Problem Adding " + path + " to DB.");
                    Messages.writeStackTrace(e);
                }
            }
            if(success) {
                SessionShare sshare = ResultTable.getSshare();
                // Only set newlySavedFile if the objType currently in
                // the shuffler is the same as the thing added (objType)
                StatementHistory history = sshare.statementHistory();
                Hashtable statement = history.getCurrentStatement();
                String objectType = (String)statement.get("ObjectType");
                if(objectType.equals(objType) && highlight)
                    // Save these filenames so that they can be put at the 
                    // top of the shuffler list and highlighted as a newly 
                    // saved files.  Use Canonical path, because that is
                    // what will be in the locator.
                    for(int i=0; i < fullpath.size(); i++) {
                        String cpath =  (String)fullpath.get(i);
                        Vector mp = MountPaths.getCanonicalPathAndHost(cpath);
                        String mhost  = (String) mp.get(Shuf.HOST);
                        cpath = (String) mp.get(Shuf.PATH);
                        String hfpath =  new String(mhost + ":" + cpath);
                        Shuf.newlySavedFile.add(hfpath);
                    }
                // Cause another shuffle so that this new item shows up, 
                // if requested.
                if(locDisplayUpdate)
                    history.updateWithoutNewHistory();

                // Update the sample trays if requested
                // This should be a duplicate of "vnmrjcmd('tray update')"
                if(expIf != null && trayUpdate) {
                    SmsPanel sPan = ((ExpViewArea)expIf).smsPan;
                    if (sPan != null && sPan.isVisible())
                        sPan.setVisible(true);
                }

                // Update the menus
                statement = history.getCurrentStatement();
                StatementDefinition curStatement;
                curStatement = sshare.getCurrentStatementType();
                if(curStatement != null)
                	curStatement.updateValues(statement, true);

                // Update the AttrList
                // This thread should ignore attempts to start it
                // if it is already running.  Therefore, if we call this
                // multiple times, we should not cause any problems.
                UpdateAttributeList updateAttrThread;
                updateAttrThread = new UpdateAttributeList(objType);
                updateAttrThread.setPriority(Thread.MIN_PRIORITY);
                updateAttrThread.setName("Update DB Attr List");
                updateAttrThread.start();
            }
            if(DebugOutput.isSetFor("AddToLocatorViaThread") || 
                       DebugOutput.isSetFor("locTiming")) {
                java.util.Date endtime = new java.util.Date();
                long timems = endtime.getTime() - starttime.getTime();
                Messages.postDebug("AddToLocatorViaThread: Time(ms) for adding "
                              + fullpath.size()  + " items and reshuffling = " 
                              + timems);
            }
            if (objType != null)
                Util.dbAction("add", objType);

        }
        catch (Exception e) {
            Messages.postError("Problem Adding " + fullpath.get(0) + " to DB "
                + "and reshuffleing.");
            Messages.writeStackTrace(e);
        }
        // Send notification that this table of the DB has been modified
        // We did not let addFileToDB() do it above, because it was in
        // a loop.
        dbManager.sendTableModifiedNotification(objType);

    }
}
