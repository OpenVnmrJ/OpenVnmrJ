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

import vnmr.util.*;
import vnmr.ui.*;


/********************************************************** <pre>
 * Summary: Output the number of items in each DB category to the log file.
 *          Wait 5 minutes so startup is not effected.  This is to keep
 *          a log of what is in the DB for later debugging.
 * 
 </pre> **********************************************************/

public class DBStatus extends Thread {

    public DBStatus() {

    }

    public void run() {
        // Catch all otherwise uncaught exceptions
        try {
            sleep(300000);    
        
            ShufDBManager dbManager = ShufDBManager.getdbManager();
            dbManager.dbStatus();
        }
        catch (Exception e) {
            if(!VNMRFrame.exiting()) {
                // Error msg only if not shutting down
                Messages.postError("Problem getting DB status");
                Messages.writeStackTrace(e);
            }
        }


    }

}
