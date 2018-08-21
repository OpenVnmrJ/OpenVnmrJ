/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

// This is primarily to be able to add a delay before starting the
// process of searching the DB.  This is called when adding a protocol
// to the DB and we need to let the protocol get added before we
// take off and search the DB
public class UpdatePBResultsViaThread extends Thread {
    ProtocolBrowser protocolBrowser=null;


    public UpdatePBResultsViaThread(ProtocolBrowser pb) {
        protocolBrowser = pb;
    }

    public void run() {
        // If the locator is not being used or the ProtocolBrowser
        // is not being used, get out of here
        if(FillDBManager.locatorOff() || protocolBrowser == null)
            return;

        try {
            // Delay long enough for the protocol to get into the DB
            sleep(500);
        }
        catch (Exception e) {return;}

        // Do the update.  This will cause a new DB search
        protocolBrowser.updateResultPanel();

    }


}
