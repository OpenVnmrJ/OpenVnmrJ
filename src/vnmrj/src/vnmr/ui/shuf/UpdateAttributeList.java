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
 * Summary: Update the TagList  from the vnmr_data table.
 *	    
 * This thread should run continuously while vnmrj is running.  We will
 * try to keep any new entries update as we go, in the code, but we cannot
 * delete attribute values very easily on the fly.  So this thread will
 * update the list from scratch every so often.
 * 
 </pre> **********************************************************/
public class UpdateAttributeList extends Thread {
    static private boolean inUse=false;
    private String type;  // all, or name of objType

    public UpdateAttributeList(String type) {
        this.type = type; 
    }
    
    public UpdateAttributeList() {
        this.type = "all";
    }

    public void run() {
	 FillDBManager dbManager;
        // Only allow one instance of this to run at a time.
        // Do not queue up requests, just abort after logging a message.
        if(inUse) {
            if(DebugOutput.isSetFor("UpdateAttributeList"))
                Messages.postDebug("UpdateAttributeList already in use, "
                                   + "Aborting.");
            return;
        }

        inUse = true;

        // Catch all otherwise uncaught exceptions
        try {
            dbManager = FillDBManager.fillDBManager;

            // Wait awhile, the update is done when vnmrj starts up
            if(type.equals("all"))
                sleep(60000);
                
            // If no Db connection, abort this thread.
            if(!dbManager.checkDBConnection()) {
                inUse = false;
                return;
            }
            if(DebugOutput.isSetFor("UpdateAttributeList"))
                Messages.postDebug("Starting UpdateAttributeList cycle for "
                                  + type);
            if(type.equals("all")) {
                TagList.updateTagNamesFromDB();
                dbManager.attrList.fillAllListsFromDB(50);
            }
            else if(!type.equals("?")) {
                dbManager.attrList.updateSingleList(type, 50);
            }

            if(DebugOutput.isSetFor("UpdateAttributeList"))
                Messages.postDebug("Finished UpdateAttributeList cycle for "
                                  + type);

            // Write out the persistence file to be sure we at least
            // have a good one when we start
                dbManager.attrList.writePersistence();
        }
        catch (Exception e) {
//            if(!VNMRFrame.exiting()) {
//                Messages.postError("UpdateAttributeList Thread Shutting Down.");
//               Messages.writeStackTrace(e);
//            }
        }

        inUse = false;
    }
}
