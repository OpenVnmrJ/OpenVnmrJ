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
 * Summary: Update the Macros tables in the DB via delayed thread.
 *
 *   The delay is to allow for faster startup.
 </pre> **********************************************************/


public class FillMacrosTables extends Thread {

    public FillMacrosTables() {
    }


    public void run() {

        // Wait awhile and allow exit if vnmrj is terminated.
        try {
            sleep(180000);
        }
        catch (Exception e) {return;}


        // Catch all otherwise uncaught exceptions
        try {
            // Update the tables
            ShufDBManager dbManager = ShufDBManager.getdbManager();	
            dbManager.checkDBConnection();

            dbManager.updateMacro(Shuf.DB_COMMAND_MACRO);
            dbManager.updateMacro(Shuf.DB_PPGM_MACRO);
        }
        catch (Exception e) {
            // Error msg only if not shutting down
            if(!VNMRFrame.exiting()) {
                Messages.postError(
                	"Problem Updating DB Macros Tables in Background");
                Messages.writeStackTrace(e);
            }
        }
    }


}

