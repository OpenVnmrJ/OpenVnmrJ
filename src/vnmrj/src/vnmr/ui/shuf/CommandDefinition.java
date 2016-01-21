/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import vnmr.util.*;

import java.awt.*;
import java.util.*;
import javax.swing.*;


/********************************************************** <pre>
 * Summary: Holds one command/macro for DB
 *
 * @author  Glenn Sullivan
 * 
 </pre> **********************************************************/

public class CommandDefinition {
    String commandName=null;
    Hashtable attrList; // List of attributes besides the two above.

    
    /************************************************** <pre>
     * Summary: Constructor, Save info for one command/macro.
     *
     * Details:
     *	Retrieve the name from the attr list and save in CommandName.
     *	Remove name from the attr list, then save whats left in
     *  the list in attrList.
     *
     </pre> **************************************************/

    public CommandDefinition(Hashtable list, String type) {
	attrList = list;

	if(!attrList.containsKey("name")) {
	    Messages.postError("All command/macro definitions " +
                               "MUST have a 'name' attribute!");
	    return;
	}
	
	commandName = (String)attrList.get("name");
	
	// We needed name separate, so now get it out of the attrList.
	attrList.remove("name");

	// Now put the type into the hashtable as a normal attribute
	attrList.put("exectype", type);
    }

    public String getcommandName() {
	return commandName;
    }

    public Hashtable getattrList() {
	return attrList;
    }

    public String toString(){
    	return commandName+" "+attrList.toString();
    }
}
