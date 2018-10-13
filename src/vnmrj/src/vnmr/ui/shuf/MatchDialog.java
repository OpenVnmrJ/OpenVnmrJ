/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.beans.*;
import java.io.*;

import vnmr.util.*;
import vnmr.ui.*;
import vnmr.bo.*;


/********************************************************** <pre>
 * Summary: Utility methods to deal with the showMatchingOnly member.
 *
 *    The java code will call getshowMatchingOnly() to determine the
 *    setting of showMatchingOnly.  The user interface for setting this
 *    member is in the Utility Menu in the System settings panel
 *    in the Display/Plot tab.  That means that it is created by an
 *    xml file and can only operate via the vq, vc and set items
 *    that control the action of the panel item.  We will keep the
 *    persistence information in the "MatchingOnly" file.  However,
 *    we need to set the vnmr variable "matchOnly" so that the panel
 *    item will come up with the correct value.  Then when the user
 *    changes the setting of the panel item, it will call a vnmrjcmd
 *    "setMatchingOnly" which will call the setshowMatchingOnly()
 *    here to keep the showMatchingOnly member up to date.
 </pre> **********************************************************/
public class MatchDialog {
    static public boolean showMatchingOnly=false;
    static public boolean persistenceRead=false;



    /******************************************************************
     * Summary: read the MatchingOnly persistence file.
     *
     *    Set the local showMatchingOnly member.
     *    Set the vnmr variable matchOnly so that the System Settings
     *    panel will have the correct setting for this item.
     *****************************************************************/

    static public void readPersistence() {
	String filepath;
        BufferedReader in;
        String line;
        String value;
        StringTokenizer tok;

	persistenceRead = true;

	filepath = FileUtil.openPath("USER/PERSISTENCE/MatchingOnly");
        if(filepath==null) {
            return;
        }
	try {
            in = new BufferedReader(new FileReader(filepath));
            line = in.readLine();
            if(!line.startsWith("matchingOnly")) {
                in.close();
                File file = new File(filepath);
                // Remove the corrupted file.
                file.delete();
                value = "false";
            }
            else {
                tok = new StringTokenizer(line, " \t\n");
                value = tok.nextToken();
                if (tok.hasMoreTokens()) {
                    value = tok.nextToken();
                }
                else
                    value = "false";
                in.close();
            }
	}
	catch (Exception e) {
	    // No error output here.
            value = "false";
	}

        if(value.startsWith("t"))
            showMatchingOnly = true;
        else
            showMatchingOnly = false;

        // We cannot sent a command to vnmr until the vnmr outPort is valid,
        // so wait for it.  Before that, we have to have a valid ExpPanel.
        int count = 0;
        ExpPanel exp=null;
        while(exp == null) {
             try {
                 Thread.sleep(200);
             }
             catch (Exception e) {}
             if(count++ > 100)
                 return;

            exp=Util.getDefaultExp();
        }

        while(!exp.outPortReady()) {
             try {
                 Thread.sleep(200);
             }
             catch (Exception e) {}
             if(count++ > 100)
                 return;
        }

    }

    static public void writePersistence() {
	String filepath;
        FileWriter fw;
        PrintWriter os;

	filepath = FileUtil.savePath("USER/PERSISTENCE/MatchingOnly");

	try {
              fw = new FileWriter(filepath);
              os = new PrintWriter(fw);
              if(showMatchingOnly)
                  os.println("matchingOnly true");
              else
                  os.println("matchingOnly false");

              os.close();
	}
	catch (Exception e) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(e, "Error caught in writePersistence");
	}

    }


    static public boolean getshowMatchingOnly() {
	if(!persistenceRead)
	    readPersistence();

	return showMatchingOnly;
    }

    static public void setshowMatchingOnly(boolean newvalue) {
        if (showMatchingOnly != newvalue) {
            showMatchingOnly = newvalue;

            // Cause another shuffle.
            SessionShare sshare = ResultTable.getSshare();
            StatementHistory history = sshare.statementHistory();
            history.updateWithoutNewHistory();

            writePersistence();
        }
    }
}
