/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;

import java.awt.Container;
import java.io.File;

import vnmr.bo.VGroup;
import vnmr.ui.RoboHelp_CSH;

/** Utility functions for Context Sensitive Help (CSH) */
public class CSH_Util   {
    static VResourceBundle bssIDs = null; 
    static VResourceBundle helpResIDs = null; 

    // Given a topic, get the ID and display the help content via RoboHelp
    // If, the topic is actually a fullpath, then just call vnmr_open to
    // open a .pdf, .htm, .html, .doc, .txt
    static public void displayCSHelp(String topic) {
        if (topic == null)
            return;
        
        if(topic.startsWith(File.separator)) {
            // Must be a fullpath, send it to  vnmr_open
            String cmd = "vnmr_open " + topic;
            UNFile file = new UNFile(topic);
            if(file.canRead()) {
                try {
                    Runtime rt = Runtime.getRuntime();
                    rt.exec(cmd);
                    if(DebugOutput.isSetFor("help"))
                        Messages.postDebug("Help Cmd: " + cmd);
                }
                catch (Exception ex) {
                    Messages.postError("Problem opening " + topic);
                }
            }
            else {
                Messages.postError("Problem opening " + topic);
            }
        }
        else {
            topic = topic.replace(" ", "_");

            // Must be a topic, get the ID
            int id = getHelpID(topic);
            
            String mainFile = "/vnmr/help/WebHelp/VnmrJ.htm";
            UNFile file = new UNFile(mainFile);
            if(!file.exists()) {
                mainFile = "/vnmr/help/WebHelp/index.htm";
                file = new UNFile(mainFile);
                if(!file.exists()) {
                    mainFile = "/vnmr/help/WebHelp/Index.htm";
                }
            }
            // Make it a URL
            mainFile = "file://" + mainFile;

            int hParent = 0;
            RoboHelp_CSH.RH_ShowHelp(hParent, mainFile, RoboHelp_CSH.HH_HELP_CONTEXT, id);
            if(DebugOutput.isSetFor("help"))
                Messages.postDebug("Help file: " + mainFile + " ID: " + id);
        }

    }

    // Is this topic found in properties file?
    static public boolean haveTopic(String topic) {
        if(topic == null)
            return false;
        
        topic = topic.replace(" ", "_");
        
        int id = getHelpID(topic);

        if(id > 0)
            // Numeric ID was found.  This topic exists
            return true;
        else
            return false;
    }

    static public int getHelpID(String str) {
        if(str == null || str.length() == 0) 
            return 0;
        
        int idint;
        int id;
        String idStr;
        
        if(str == null)
            return 0;
        
        // Only open and read the properties files once, then
        // just use the list obtained from the files
        if(bssIDs == null)
            bssIDs = new VResourceBundle("BSSCDefault");
        if(helpResIDs == null)
            helpResIDs = new VResourceBundle("helpResources");
        
        // Replace all spaces with underscores
        String topic = str.replace(" ", "_");

        // First try helpResources
        idStr = helpResIDs.getString(topic);

        if(idStr.equals(topic)) {
           // try BSSCDefault
           idStr = bssIDs.getString(topic);
        }

        // See if we can convert to int
        try {
            id = Integer.parseInt(idStr);
        }
        catch (Exception ex) {
            // did not end up with an int.  return 0
            if(DebugOutput.isSetFor("help"))
                Messages.postDebug("Help: For topic, \"" + str + "\", Not Found");
            return 0;
        }
        if(DebugOutput.isSetFor("help"))
            Messages.postDebug("Help: For topic, \"" + str + "\", ID = " + id);

        return id;
        
    }

    // Try to find help for an item based on it's parent and up two more levels.
    // First see if there is a helplink set at any level.  If not, use the first
    // level with a "title" for that group and use the "title".
    static public String getHelpFromGroupParent(Container parent, String label, 
                                                int LABEL, int HELPLINK) {
        String helpstr=null;
        boolean foundit=false;

        if(parent instanceof VGroup) {
            // Look for helplink set in some parent group
            helpstr=((VGroup)parent).getAttribute(HELPLINK);
            if(helpstr==null){
                Container group2 = parent.getParent();
                if(group2 instanceof VGroup)
                    helpstr=((VGroup)group2).getAttribute(HELPLINK);
                if(helpstr==null){
                    Container group3 = group2.getParent();
                    if(group3 instanceof VGroup)
                        helpstr=((VGroup)group3).getAttribute(HELPLINK);
                }
            }
        }

        // If no helplink found, try parent group's titles
        if(helpstr == null) {
            // No help is found.  Try using the value of the label
            // If there are spaces, replace them with underscores
            // The label passed in is from the item, not a group.  See if it
            // is a good one.
            if(label != null && label.length() > 0) {
                helpstr = new String(label);
                helpstr = helpstr.replace(" ", "_");
                foundit = CSH_Util.haveTopic(helpstr);
            }
            if(foundit) {
                // There was no helplink, but the items label has help, so
                // use that.
                helpstr = label;
            }
            else {
                // Try the parent's title then it's parent and it's parent
                if(parent instanceof VGroup) {
                    String title = ((VGroup)parent).getAttribute(LABEL);
                    if(title != null && title.length() > 0) {
                        title = title.replace(" ", "_");
                        foundit = CSH_Util.haveTopic(title);
                        if(foundit)
                            helpstr = title;
                    }
                    if(title == null || title.length() == 0 || !foundit) {
                        Container group2 = parent.getParent();
                        if(group2 instanceof VGroup) {
                            title = ((VGroup)group2).getAttribute(LABEL);
                            if(title != null && title.length() > 0) {
                                title = title.replace(" ", "_");
                                foundit = CSH_Util.haveTopic(title);
                                if(foundit)
                                    helpstr = title;
                            }
                            if(title == null || title.length() == 0 || !foundit) {
                                Container group3 = group2.getParent();
                                if(group3 instanceof VGroup) {
                                    title = ((VGroup)group3).getAttribute(LABEL);
                                    if(title != null && title.length() > 0) {
                                        title = title.replace(" ", "_");
                                        foundit = CSH_Util.haveTopic(title);
                                        if(foundit)
                                            helpstr = title;
                                    }
                                    if(title == null || title.length() == 0 || !foundit) {
                                        Container group4 = group3.getParent();
                                        if(group4 instanceof VGroup) {
                                            title = ((VGroup)group4).getAttribute(LABEL);
                                            if(title != null && title.length() > 0) {
                                                title = title.replace(" ", "_");
                                                foundit = CSH_Util.haveTopic(title);
                                                if(foundit)
                                                    helpstr = title;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return helpstr;
    }
}

