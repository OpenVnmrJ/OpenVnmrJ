/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui.shuf;

import  vnmr.bo.*;
import  vnmr.ui.*;
import  vnmr.util.*;

import java.util.*;
import java.io.*;
import java.awt.peer.*;
import java.awt.image.*;
import java.awt.event.*;
import java.lang.reflect.*;
import java.awt.*;
import sun.awt.SunToolkit;
import java.text.*;


public class UpdateDatabase extends Thread {
    String  table;
    boolean updateNow;

    // If no args, assume all table, in a loop
    public UpdateDatabase() {
        this.table = "all";
        this.updateNow = false;
    }  
    
    public UpdateDatabase(String table, boolean updateNow) {
        this.table = table;
        this.updateNow = updateNow;
    }

    public void run() {
        ShufflerService shufflerService;
        ShufDBManager dbManager;
        Hashtable userHash;
        LoginService loginService;
        String objType;
        SessionShare sshare;
        StatementHistory history;
        ResultTable resultTable;
        Hashtable statement;
        String TimeToStart;
        User      user;
        boolean needIt;
        LocatorHistoryList lhl;
        LocatorHistory lh;
        String activeObjType;

        if(updateNow) {
            try {
                // If the locator is not being used, get out of here
                if(FillDBManager.locatorOff())
                    return;

                // If no Db connection, abort this thread.
                dbManager = ShufDBManager.getdbManager();
                if(!dbManager.checkDBConnection()) {
                    return;
                }

                if(table.equals("all")) {
                    // Do not update the workspace table.  The DB is kept
                    // up to date dynamically, without the exp# file being
                    // updated.  Thus, reading the file will cause the
                    // attr values to be wrong.
                    boolean workspace = false;
                    loginService = LoginService.getDefault();
                    Hashtable users = loginService.getuserHash();

                    // Just call the normal updateDB routine and let it go
                    dbManager.updateDB(users, 0);

                    // Update the display when finished.
                    sshare = ResultTable.getSshare();
                    history = sshare.statementHistory();
                    history.updateWithoutNewHistory();
                }
                else if(table.equals("imaging")) {
                    // Do both types of image directories and the fdf files
                    dbManager.updateThisTable(Shuf.DB_IMAGE_DIR);
                    sshare = ResultTable.getSshare();
                    lhl = sshare.getLocatorHistoryList();
                    lh = lhl.getLocatorHistory();
                    activeObjType = lh.getActiveObjType();
                    if(activeObjType.equals(Shuf.DB_IMAGE_DIR)) {
                        // Update the display when finished if displaying 
                        // one of these tables
                        history = sshare.statementHistory();
                        history.updateWithoutNewHistory();
                    }
                }
                else {
                    dbManager.updateThisTable(table);
                    sshare = ResultTable.getSshare();
                    lhl = sshare.getLocatorHistoryList();
                    lh = lhl.getLocatorHistory();
                    activeObjType = lh.getActiveObjType();
                    if(activeObjType.equals(table)) {
                        // Update the display when finished if displaying 
                        // this table
                        history = sshare.statementHistory();
                        history.updateWithoutNewHistory();
                    }
                }
                Util.sendToVnmr("write('line3', 'Locator Update Complete')");

                return;
            }
            catch (Exception e) {return;}
        }
        // If it was updateNow, we will have returned and will not continue.

        // Must not be updateNow, so do it in a loop later.

        // Give vnmrj some time to get its panels etc up
        // before we start this.  Otherwise, this can slow down
        // the startup too much even with this run as a low priority thread.

        try {
            sleep(200000);
        }
        catch (InterruptedException e) {return;}


        // Catch all otherwise uncaught exceptions
        try {
            loginService = LoginService.getDefault();
            sshare = ResultTable.getSshare();
            shufflerService = sshare.shufflerService();
            history = sshare.statementHistory();

            // Get the list of users
            userHash = loginService.getuserHash();
            dbManager = ShufDBManager.getdbManager();

            // If no Db connection, abort this thread.
            if(!dbManager.checkDBConnection()) {
                return;
            }

            // First off, check when the DB was last vaccum'ed to see
            // if we need to force it.
            needIt = dbManager.vacuumOverdue();
            if(needIt) {
                // If it needs to be done, wait a bit and then go
                // ahead and start an updateDB.  Then the loop below
                // will kick in for nightly builds.
                sleep(900000);
                dbManager.vacuumDB();

            }

            // Allow a way to bail out in case this causes a problem
            // for some sites in the field.
            if(DebugOutput.isSetFor("noCrawler")) {
                Messages.postDebug("Debug flag 'noCrawler' causing crawler "
                                   + "to abort.");
                return;
            }

            // Have a db server start its update at a different time from
            // the clients.
            String dbnet_server = System.getProperty("dbnet_server");
            if(dbnet_server == null ||
               (!dbnet_server.equals("yes") &&  !dbnet_server.equals("true"))) {
                TimeToStart = "21";
            }
            else
                TimeToStart = "19";

            // Now the loop to update nightly while vnmrj is running.
            while(true) {
                // If no Db connection, abort this thread.
                if(!dbManager.checkDBConnection()) {
                    return;
                }

                // Get the current time
                Date date = new Date();
                SimpleDateFormat formatter = new SimpleDateFormat ("HH");
                String hourString = formatter.format(date);

                // If 8PM, start an update
                if(hourString.equals(TimeToStart)) {
                    try {
                        if(DebugOutput.isSetFor("updatedb") ||
                           DebugOutput.isSetFor("updatedbStart")) {
                            Messages.postDebug("Starting update cycle");
                        }
                        // Do not update the workspace table.  The DB is kept
                        // up to date dynamically, without the exp# file being
                        // updated.  Thus, reading the file will cause the
                        // attr values to be wrong.
                        boolean workspace = false;
                        dbManager.updateDB(userHash, 300, workspace);

                        // Check holding area for entries which have no reality.
                        HoldingDataModel holdingDataModel;
                        holdingDataModel = sshare.holdingDataModel();
                        holdingDataModel.cleanupHoldingArea();

                        if(DebugOutput.isSetFor("updatedb") ||
                           DebugOutput.isSetFor("updatedbStart")) {
                            Messages.postDebug("Finished update cycle");
                        }

                        // Sleep an hour to be sure we do not hit
                        // twice during the starting hour.
                        sleep(3600000);
                    }
                    catch (InterruptedException e) {
                        return;
                    }
                }
                else {
                    // It is not time, sleep awhile, then check again
                    sleep(900000);

                    // Do we need to vacuum the DB?
                    // Adding files causes the DB to need vacuuming.
                    // This vac is to catch the times that people just
                    // add a few at a time all day in an autosampler
                    // or imager.

                    needIt = dbManager.vacuumOverdue();
                    if(needIt) {
                        dbManager.vacuumDB();
                    }

                }
            }
        }
        catch (Exception e) {
            // This should run until vnmrj is shutdown.  If we are exiting
            // prematurely, log it
            if(!VNMRFrame.exiting()) {
                Messages.postError("Problem Updating Database in Background.");
                Messages.writeStackTrace(e);
            }
        }
    }


    /************************************************** <pre>
     * Summary: Wait until Event Queue is Idle.
     *
     *  This was taken from Robot.java .  It does not seem to work
     *  for the purpose here.  That is, if this thread is started
     *  early in main this waitForIdle completes many seconds before
     *  the display is complete.  I will leave it here for future
     *  trials.
     *
     *  Even if this did work, it does not take into account
     *  other processes like vnmrbg running and needing time.
     </pre> **************************************************/

    public synchronized void waitForIdle() {

        // ** Perhaps move this to a higher level so it is not called so much
        SunToolkit toolkit = (SunToolkit)Toolkit.getDefaultToolkit();

        checkNotDispatchThread();
        // post a dummy event to the queue so we know when
        // all the events before it have been processed
        try {
            toolkit.flushPendingEvents();
            EventQueue.invokeAndWait( new Runnable() { 
                    public void run() {
                        // dummy implementation
                    }
                } );
        } catch(InterruptedException ite) {
            ite.printStackTrace();
        } catch(InvocationTargetException ine) {
            ine.printStackTrace();
        }
    }

    private void checkNotDispatchThread() {         
        if (EventQueue.isDispatchThread()) {
            throw new IllegalThreadStateException("Cannot call method from " +
                                                "the event dispatcher thread");
        }
    }

}
