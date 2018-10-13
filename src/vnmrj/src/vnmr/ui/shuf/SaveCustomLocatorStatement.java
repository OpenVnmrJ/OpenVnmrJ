/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui.shuf;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.*;


/********************************************************** <pre>
 * Summary: Popup to save custom locator statements.
 *
 *
 </pre> **********************************************************/
public class SaveCustomLocatorStatement {

    static private ModalEntryDialog med=null;


    public SaveCustomLocatorStatement() {

    }

    static public void showPopup(String helpFile) {
        String shuffleName;
        SessionShare sshare = ResultTable.getSshare();
        StatementHistory history = sshare.statementHistory();

        // Create one ModalEntryDialog and reuse it.
        if(med == null) {
            String title = Util.getLabel("_Custom_Locator_Statement");
            String entryLabel = Util.getLabel("_Custom_Locator_Statement_Name") + " "
                                + Util.getLabel("_Custom_Locator_Statement");
            med = new ModalEntryDialog(title, entryLabel, helpFile);
        }

        shuffleName =  med.showDialogAndGetValue(null);

        if(shuffleName != null  && shuffleName.length() != 0) {
            // Write out the file
            history.writeCurStatement(shuffleName);

            // Update the Spotter menu to show the new file.
            SpotterButton spotterButton = sshare.getSpotterButton();
            spotterButton.updateMenu();
        }
    }
}
