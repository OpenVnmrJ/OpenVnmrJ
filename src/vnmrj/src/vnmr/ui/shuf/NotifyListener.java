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


/********************************************************** <pre>
 * Summary: Mechanism for updating the locator on all computers
 *      when one computer on the network changes a table.
 *
 *      I have run into no end of problems trying to implement the postgres
 *      LISTEN/NOTIFY feature.  I will put what I learned here for future
 *      reference.  Apparently, you cannot use LISTEN/NOTIFY with Sun's
 *      JDBC, but you need to use the one supplied with postgres which is
 *      implemented with the configure "--with-java" option when
 *      configuring postgres before building.  That is in
 *      src/interfaces/jdbc.  According to the README, you have to have
 *      "ant" installed and it tells where to get it.  I downloaded "ant"
 *      and failed to get it to work.  There were no errors reported, just
 *      a stack trace into the ant code.
 *
 *      The way it is supposed to opperate is that the listener had to make
 *      a call now and then to find out if a notifications has occured.
 *      Rather crude at best.
 *
 *      So, I decided I could write my one faster than worring about "ant"
 *      and the postgres JDBC, and here it is.
 *
 *      The idea is to create a table, "notify" and put notifications in
 *      there for each host that is registered.  Then it is up to each host
 *      to query this table now and then to see what table may have been
 *      changed.  It does not seem any worse than the one postgres designed
 *      in.
 *
 </pre> **********************************************************/

public class NotifyListener extends Thread {
    ShufDBManager dbManager;
    String localHost;
    // sql command to be used for query of DB
    String queryCmd;
    // sql command to be used to clear the update status 
    String clearCmd;

    public NotifyListener() {
        boolean foundIt=false;
        String cmd, value;
        java.sql.ResultSet rs;

        dbManager = ShufDBManager.getdbManager();
        localHost = dbManager.localHost;

        // Be sure the 'notify' table exist, and if not create it.
        // This makes the code backwards compatible with already existing
        // databases.
        try {
            rs = dbManager.executeQuery("SELECT DISTINCT host FROM notify");

            // Table exists, else we would have had an exception, 
            // get list of hosts already registered
            while(rs.next()) {
                // There is only one column of results, therefore, col 1
                value = rs.getString(1);
                if(value != null) {
                    // Be sure it is not whitespace
                    value = value.trim();
                    if(value.length() > 0) {
                        if(value.equals(localHost)) {
                            foundIt = true;
                            break;
                        }
                    }
                }
            }
        }
        catch (Exception e) {
            // Probably here because the table does not exist yet, create it
            cmd = "create table notify (host text, PRIMARY KEY(host), ";
            // Now add a column for the name of each objType vnmr
            // plans to use this notification for.
            for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
                cmd = cmd.concat(Shuf.OBJTYPE_LIST[i] + " text ");

                // No trailing comma
                if(i != Shuf.OBJTYPE_LIST.length -1)
                    cmd = cmd.concat(", ");
                else
                    cmd = cmd.concat(")");
            }
            try {
                dbManager.executeUpdate(cmd);
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }
        // If not already registered, add a row to the DB for this host
        if(!foundIt) {
            cmd = "INSERT INTO notify (host) VALUES (\'" + localHost + "\')";
            try {
                dbManager.executeUpdate(cmd);
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
            
        }

        // Create the query command needed later one time and save it
        queryCmd = "SELECT ";
        for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
            queryCmd = queryCmd.concat(Shuf.OBJTYPE_LIST[i] + " ");
            if(i != Shuf.OBJTYPE_LIST.length -1) 
                queryCmd = queryCmd.concat(", ");                
        }
        queryCmd = queryCmd.concat(" FROM notify WHERE host = \'" 
                                   + localHost + "\'");

        // Create the clear command needed later one time and save it
        clearCmd = "UPDATE notify SET ";
        for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
            clearCmd = clearCmd.concat(Shuf.OBJTYPE_LIST[i] + " = \'clear\'");
            if(i != Shuf.OBJTYPE_LIST.length -1) 
                clearCmd = clearCmd.concat(", ");                
        }
        clearCmd = clearCmd.concat(" WHERE host = \'" + localHost + "\'");

    }


    public void run() {
        boolean modified;
        ArrayList list = new ArrayList();
        
        if(ShufDBManager.locatorOff())
            return;

        while(true) {
            try {
                sleep(120000);
                list.clear();

                modified = getNotificationStatus(list);

                if(modified) {
                    // Get the currently displayed objType
                    SessionShare sshare = ResultTable.getSshare();
                    LocatorHistoryList lhl = sshare.getLocatorHistoryList();
                    LocatorHistory lh = lhl.getLocatorHistory();
                    String activeObjType = lh.getActiveObjType();

                    // Is the current objType in the modified list?
                    if(list.contains(activeObjType)) {
                        // Yes, update the locator
                        StatementHistory history = sshare.statementHistory();
                        history.updateWithoutNewHistory();
                    }
                }

            }
            catch (InterruptedException ie) {
                // probably just exiting
                return;

            }
            catch (Exception e) {
                // Just continue with the loop
            }
        }
    }


    /******************************************************************
     * Summary: Clear the notifications for this localHost by setting
     *  all objType statuses to empty.
     *
     *
     *****************************************************************/
    public void clearNotification() {
        try {
            dbManager.executeUpdate(clearCmd);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        
    }


    /******************************************************************
     * Summary: Return true if any tables have been notified
     *  of being changed since the last call here.
     *
     *  Fill the users list with the list of tables which have been
     *  notified.
     *****************************************************************/
    public boolean getNotificationStatus(ArrayList list) {
        java.sql.ResultSet rs;
        String value;
        boolean somethingChanged=false;

        try {
            rs = dbManager.executeQuery(queryCmd);
            if(rs == null)
                return false;

            // Don't see why this should be needed, however, we get a null pointer exception 
            // without it on the old postgresql.  On the other hand, the new postgresql does
            // not have "first".  I have not tested on the new one to see if it
            // gives a null pointer exception there.  So, for now, just do this if
            // it is the old one and we need to test it with the new one later.
            if(dbManager.newpostgres.equals("false"));
            rs.first();

            // There should be a column for each and every item, so get the
            // value for each.
            for(int i=0; i<Shuf.OBJTYPE_LIST.length; i++) {
                value = rs.getString(i+1);  // starts at 1
                if(value != null && value.startsWith("m")) {
                    // This objType has been modified, add it to the list
                    list.add(Shuf.OBJTYPE_LIST[i]);

                    // If any table changes, set this true.
                    somethingChanged = true;
                }
            }
        }
        catch (Exception e) {
            // Bail out
            return somethingChanged;
        }

        // clear all notifications for this host if anything was set
        if(somethingChanged)
            clearNotification();

        return somethingChanged;
    }


    /******************************************************************
     * Summary: Send notification to any listening processes, that
     *  a given table has been changed.
     *
     *  Put the actual method in FillDBManager so that FillDBManager does
     *  not have to know about NotifyListener.  This is because we 
     *  cannot have managedb know about NotifyListener which updates
     *  the locator tables.
     *****************************************************************/
    public void sendNotification(String objType) {
        dbManager.sendTableModifiedNotification(objType);
    }

}
