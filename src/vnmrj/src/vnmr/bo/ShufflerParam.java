/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.util.*;
import java.io.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;


/* When Vnmr sends 'pnew' data to ExpPanel, this class will be called
   to update shuffler parameters.
   After request, Vnmr will send back new value with 'eval', but without
   parameter name. So this class needs new class 'ShufObj' which is based
   on VObjIF to take care of this situation.
   Because the ShufObj was dynamically created, it is not necessary to 
   keep them forever, so they can be recycle by using Vector to keep
   the track of them.
*/
   

public class ShufflerParam {
    static private ArrayList shufList = null;
    private ButtonIF vnmrIf;
    private ShufDBManager dbManager;


	
    public ShufflerParam(ButtonIF vif) {
	this.vnmrIf = vif;
	dbManager = ShufDBManager.getdbManager();
    }

    /** Update values in DB as per params and values in parValList.
        parValList is a Vector of Strings in the form
           [attr1, attr2, attr3, val1, val2, val3] where the number
        of attr is variable.
        Take each attr and its value and update the workspace.
    */
    public void updateValue(String expname, Vector parValList) {
        int size;

	// Just get list from DB once.
	if(shufList == null) {
            if (dbManager.attrList == null)
               return;
	    shufList = dbManager.attrList.getAllAttributeNames(
                                                          Shuf.DB_WORKSPACE);
            if (shufList == null)
               return;
        }
	size = parValList.size();
        // Loop thru the attribute name in the first half of the Vector
	for (int i = 0; i < size/2; i++) {
            String attr = (String)parValList.elementAt(i);
            for (int k = 0; k < shufList.size(); k++) {
		String p2 = (String) shufList.get(k);
                // Is this attr in the shufList?
		if (attr.equals(p2)) {
                    // Get the value from the last half of the Vector
                    String val = (String)parValList.elementAt(size/2 +i);

                    // Test value for escape (033).  If it is, then there
                    // was no value.  Send command to vnmrbg to have it
                    // send us the value for this cmd.  Then this method
                    // will be called again when the new command arrives.
                    if(val.equals("\033")) {
                        String cmd = "jFunc(41, \'" + expname + " " 
                            + attr + "\', \'" + attr + "\')";
                        // The result of this command will be a 'vloc'
                        // command coming back with three args, expname,
                        // attr and value.
                        vnmrIf.sendToVnmr(cmd);
                    }
                    // E@@ERR is a code for error of course.  Do not try
                    // to set the value when we get this string.
                    else if(!val.equals("E@@ERR")) {
                        // Only set value if it is changed.
                        // This is because querying for the current value is 
                        // about 8 times faster than setting a value in the DB
                        // Get users directory with workspaces
                        String usrdir = FileUtil.openPath("USER");
                        String curval;
                        
                        curval = dbManager.getAttributeValueNoError(
                                Shuf.DB_WORKSPACE, usrdir + File.separator + 
                                expname, dbManager.localHost, attr);
                        if(!val.equals(curval)) {
                            dbManager.updateWorkspaceAttribute(attr, null, 
                                    val, expname);
                        }
                    }
		}
            }
	}
    }
}

