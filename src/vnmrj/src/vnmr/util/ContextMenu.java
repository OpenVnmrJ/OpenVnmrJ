/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import vnmr.templates.*;
import vnmr.util.*;
import vnmr.ui.*;

import javax.swing.*;
import java.awt.*;


/********************************************************** <pre>
 * Summary: Create and Pop up a context sensitive menu
 *
 *      - Let LayoutBuilder do the work of parsing the menu xml file
 *      - Let ExpPanel do the work of dealing with the commands in the menu file
 *      - Transfer the input arg to the vnmr variable $contextmenuarg
 *        for use by the vc command.
 *      - At this point, the menus needs to be written by hand and must be
 *        located in the interface directory of vnmr or the user.
 *        A simple example is as follows:
 *
 *    <mainmenu>
 *	<mchoice label = "Open"
 *	      vc = "locaction(contextmenuarg[1], contextmenuarg[2], 
 *		 contextmenuarg[3], contextmenuarg[4],
 *		 contextmenuarg[5], contextmenuarg[6])"
 *	      style="Menu1"
 *        />
 *	<mchoice label = "Delete"
 *	      vc = "vnmrjcmd('LOC trashfile ' + contextmenuarg[4])"
 *	      style="Menu1"
 *        />
 *    </mainmenu>
 *
 *      To use this utility, you need to:
 *      - Catch the  MouseEvent.BUTTON3  mouseClicked event.  
 *      - Get the position of the click
 *      - Get the parent of the item clicked if necessary
 *      - Call the static  ContextMenu.popupMenu() with
 *        - The item type name.  After adding '.xml', this will be the
 *          name of the xml file creating the menu to be popped up.
 *          For example 'locator_item' and 'locator_field'
 *        - A string, arg, which is the value to be passed to the
 *          menu xml code.  If more that one arg is to be passed, create
 *          the string 'arg' in the form "'value1', 'value2', 'value3'...
 *          This will cause a temp variable ($contextmenuarg) to be created
 *          as an array.  Use the value of $contextmenuarg in the vc command
 *          in the menu file.  See the example menu file above.
 *        - The parent of this item
 *        - The position of the click for locating the menu
 *
 </pre> **********************************************************/
public class ContextMenu  {

    static public void popupMenu(String fileinfo, String arg, 
                                    JComponent parent, Point position) {

        // Create the contextmenuarg as a global parameter if it does not
        // already exist.  It has to be global in case the menu cmd using
        // it is being run in a different viewport.
        Util.sendToVnmr("create('contextmenuarg','string','global','')");

        // Put the arg in vnmr variables contextmenuarg
        Util.sendToVnmr("contextmenuarg=" + arg );

        // Create filename for context sensitive menu .xml file
        String dir = FileUtil.openPath("INTERFACE/" + fileinfo + ".xml");

        // Create a JPopupMenu to sent to LayoutBuilder to put the menu into
        JPopupMenu popup = new JPopupMenu();

        // Get the ButtonIF which knows how to deal with commands
        ExpPanel exp=Util.getDefaultExp();

        // Use LayoutBuilder to create a menu from the xml file
        try {
            LayoutBuilder.build(popup, exp, dir);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return;
        }

        int x = (int)position.getX();
        int y = (int)position.getY();

        // Show the menu
        popup.show(parent, x, y);
        
        return;
    }
}
