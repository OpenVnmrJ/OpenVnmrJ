/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;


/********************************************************** <pre>
 * Summary: Control debug output categories
 *
 * This is for use by programmers who want to allow the turning on/off
 * of debug output.  Debug output is segregated into categories where
 * a programmer can simply create a category by setting it and checking
 * for it.  That is, no hard coded categories exist as part of the
 * DebugOutput class.  If a programmer wants to create a new debug category
 * called locatorHistory he/she can simply add to his/her code the line:
 *     if(DebugOutput.isSetFor("locatorHistory")
 *          Messages.postDebug("My message");
 * Then if locatorHistory is set in the startup script (see below) or with
 * the vnmrjcmd (see below), this debug output will happen.
 *
 * Category names should be  single words.  They are case sensitive.
 *
 * Since the categories are not coded as part of DebugOutput, you need to
 * be sure to avoid collision with others who may choose the same name
 * for their category.  I would suggest grep'ing for "isSetFor" and checking
 * to see if anyone has used your category yet.
 *
 * All methods are static and can be called from anyplace at anytime.
 *
 * - Set debug categories from startup defines.
 *     For example to turn on sql and panel debuging use the following
 *     entry in the vnmrj.sh or managedb.sh startup script:
 *       -Ddebug="sql, panel"
 * - Adding and Removing active categories in real time.
 *   - on line 3 use the command
 *       vnmrjcmd('DEBUG add/remove category')
 *     For Example to turn on sql debug output on the fly use the command:
 *       vnmrjcmd('DEBUG add sql')
 * - Check to see if a category is currently active
 *   For example:
 *      if(DebugOutput.isSetFor("sqlCmd")
 *           Messages.postDebug("My message");
 </pre> **********************************************************/


public class DebugOutput {

    /** List of all debug categories currently turned on. */
    private static TreeSet<String> categories = new TreeSet<String>();
    /** Specify full verbose mode, ie approve all queries in isSetFor() */
    private static boolean fullVerbose = false;

    /** Who wants to know when the debug list changes. */
    private static ArrayList<Listener> listenerList = null;
    private static TreeSet<String> addedByFile = new TreeSet<String>();


    /************************************************** <pre>
     * Summary: Fill the 'categories' list from the defined value for 'debug'.
     *
     * In the vnmrj.sh and managedb.sh startup scripts, debug categories
     * can be set with a comma separated list as [-Ddebug="sql, panel"].
     * These categories will be put into the 'categories' member.
     </pre> **************************************************/
    static {
        String debugVal = System.getProperty("debug");
        StringTokenizer tok;

        try {
            if(debugVal != null) {
                tok = new StringTokenizer(debugVal, ",;\" \t");
                while(tok.hasMoreElements()) {
                    addCategory(tok.nextToken());
                }
            }
        } catch (Exception e) {
           Messages.postError("Problem setting debug output category. ");
           Messages.writeStackTrace(e);
        }

        // If requested, output the list we found
        if(isSetFor("debugCategoryList"))
            Messages.postDebug("Debug Category List: " + categories);
    }

    /************************************************** <pre>
     * Summary: Return true if the category specified by the argument
     *          is a currently active debug category.
     *
     *
     </pre> **************************************************/
    static int count=1000;

    public static boolean isSetFor(String cat) {
        String dir;



        if(fullVerbose)
            return true;
        else {

            // every n calls, check to see if any files have been added
            // or removed to the system to request us to add or remove
            // debug categories.
            if(count <= 0) {
                count = 1000;  // reset
                dir = FileUtil.usrdir();
                // List of files that can dynamically change the categories
                // list if one or more of these files appears in the
                // users vnmrsys directory, that category will be turned
                // on.  If it disappears, it will be turned off.  This
                // is being created for customer problems that can not be
                // reproduced, and where turning on some of the
                // categories outputs too much output to be feasible to
                // have on all the time.  Also to be able to turn on
                // debug stuff during an install when we cannot just add a
                // debug arg.  Note: the file names begin with 'db_' to
                // help find these for removal.  It is important to only
                // turn off a category IF it was turned on by a file.
                // Else, no files means debug flags are given at startup
                // are cleared when this point is reached.
                String[] fileList = { "sqlcmd", "updatedb", "loctiming",
                                      "fillatable", "biglogfile" ,"vacuumdb",
                                      "updateappdirs", "filelist", "locTiming"};

                UNFile file;
                for(int i=0; i < fileList.length; i++) {
                    file = new UNFile(dir + "/db_" + fileList[i]);
                    if(file.exists()) {
                        addCategory(fileList[i]);
                        if(!addedByFile.contains(fileList[i])) {
                           addedByFile.add(fileList[i]);
                        }
                    }
                    else {
                        // Only clear a category if it was set via file

                        if(addedByFile.contains(fileList[i])) {
                            removeCategory(fileList[i]);
                            addedByFile.remove(fileList[i]);
                        }
                    }
                }
            }
            count--;

            String lcat = cat.toLowerCase();
            return categories.contains(lcat);
        }
    }

    /**
     * Add a debug category to the active list.
     * @param cat The category to add
     */
    public static void addCategory(String cat) {
        String lcat = cat.toLowerCase();
        // If it is not already in the list, add it
        if(!categories.contains(cat))
            categories.add(lcat);

        if(lcat.equals("all")) {
            fullVerbose = true; // Turn on all categories (blanket enable)
        } else if ("logwarnings".equals(lcat)) {
            Messages.setLogWarnings(true); // Put warnings in debug log
        } else if ("loginfos".equals(lcat)) {
            Messages.setLogInfos(true); // Put infos in debug log
        }

        notifyListeners();
    }

    /**
     * Remove a debug category from the active list.
     * @param cat The category to remove
     */
    public static void removeCategory(String cat) {
        String lcat = cat.toLowerCase();
        categories.remove(lcat);

        if(lcat.equals("all")) {
            fullVerbose = false; // Turn off blanket enable of all messages
        } else if ("logwarnings".equals(lcat)) {
            Messages.setLogWarnings(false); // Don't put warnings in debug log
        } else if ("loginfos".equals(lcat)) {
            Messages.setLogInfos(false); // Don't put infos in debug log
        }

        notifyListeners();
    }

    /**
     * Set the time precision printed in debug messages
     * @param cat String representation of the time precision in ms.
     */
    public static void setTimePrecision(String cat) {
        try {
            int precision = Integer.parseInt(cat);
            Messages.setTimePrecision(precision);
        } catch (NumberFormatException nfe) {
            Messages.postError("DebugOutput.setTimePrecision(): \""
                               + cat + "\" is not a valid integer");
        }
    }

    /**
     * Add someone to be notified of changes to the debug list.
     * @param l The listener to add.
     * @see #removeListener(DebugOutput.Listener)
     */
    public static void addListener(Listener l) {
        if (listenerList == null) {
            listenerList = new ArrayList<Listener>(1);
        }
        listenerList.add(l);
        notifyListeners();
    }

    /**
     * Remove someone from the notify list.
     * @param l The listener to remove.
     * @see #addListener(DebugOutput.Listener)
     */
    public static void removeListener(Listener l) {
        if (listenerList != null) {
            listenerList.remove(l);
        }
    }

    /**
     * Send the current debug string to all the listeners.
     * This is a comma-delimited-list of strings -- including leading
     * and trailing commas.  It is all lower-case.
     */
    private static void notifyListeners() {
        if (listenerList != null && categories != null) {
            StringBuffer sbuf = new StringBuffer(",");
            Iterator<String> itr = categories.iterator();
            while (itr.hasNext()) {
                sbuf.append(itr.next()).append(",");
            }
            String str = sbuf.toString();
            for (int i = 0; i < listenerList.size(); i++) {
                listenerList.get(i).setDebugString(str);
            }
        }
    }


    /**
     * A DebugOutput.Listener gets notified any time the list of active
     * debug categories changes.
     * @see DebugOutput#addListener(DebugOutput.Listener)
     */
    public interface Listener {
        /**
         * Informs the listener of the list of active debug categories.
         * The category names are case-insensitive and they are sent in
         * all lower-case.
         * @param str The comma-sepatated-list of active categories.
         * The string begins and ends with commas, so the string
         * will contain the search string ",category name," iff the
         * category named "category name" is active.
         */
        public void setDebugString(String str);
    }

}
