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

/********************************************************** <pre>
 * Summary: Update the Workspace table in the DB via delayed thread.
 *
 *   The delay is to allow for faster startup.
 </pre> **********************************************************/


public class FillWorkspaceTable extends Thread {

    public FillWorkspaceTable() {
    }



    public void run() {
        if(ShufDBManager.locatorOff())
            return;

        // Wait awhile to start
        try {
            sleep(120000);
        }
        catch (Exception e) {return;}

        // Catch all otherwise uncaught exceptions
        try {
            // Update the table
            DBCommunInfo info = new DBCommunInfo();
            String dir = FileUtil.usrdir();
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            String user = System.getProperty("user.name");

            if(DebugOutput.isSetFor("fillworkspace")) {
                Messages.postDebug("FillWorkspaceTable: dir = " + dir
                                   + "  user = " + user);
            }

            dbManager.fillATable(Shuf.DB_WORKSPACE, dir, user, false, info);
        }
        catch (Exception e) {
            // No error if shutting down
            if(!VNMRFrame.exiting()) {
                Messages.postError(
                    "Problem Updating Workspaces in DB in Background");
                Messages.writeStackTrace(e);
            }
        }
    }
}
