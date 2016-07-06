/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */


package vnmr.apt;

import java.util.TreeSet;

import static vnmr.apt.AptDefs.*;


/**
 * This class prints debuging and error messages.
 */
public class DebugOutput {

    /** Collection of all debug categories currently turned on. */
    private static TreeSet<String> categories = new TreeSet<String>();

    /** Collection of all debug categories that get detailed messages. */
    private static TreeSet<String> detailedCats = new TreeSet<String>();

   /**
     * Add a debug category to the active list.
     * @param cat The given category (case insensitive).
     * @return True unless the category was already in the active list.
     */
    public static boolean addCategory(String cat) {
        if (cat != null && cat.trim().length() > 0) {
            cat = cat.toLowerCase();
            // If it is not already in the list, add it
            if(cat.startsWith("+")) {
                cat = cat.substring(1);
                detailedCats.add(cat);
            }
            return categories.add(cat);
        }
        return false;
    }
    
    /**
     * Add debug categories listed in the System Properties
     * under a given key.
     * Expects a "category string" in System Properties that is
     * a list of debug categories to add.
     * Elements of the list are separated by spaces and/or commas.
     * (Two commas do not indicate an empty name.)
     * @param catlist The comma/space separated names of the categories.
     * @return The number of new categories added.
     */
     public static int addCategories(String catlist) {
         int n = 0;
        if (catlist != null) {
            String[] cats = catlist.split("[ ,]+");
            for (String cat : cats) {
                if (addCategory(cat)) {
                    n++;
                }
            }
        }
        return n;
    }

    /**
     * Returns true if the given category is in the active list.
     * @param cat The given category (case insensitive).
     * @return True if the category is in the list.
     */
    public static boolean isSetFor(String cat) {
        cat = cat.toLowerCase();
        if (cat.indexOf("|") < 0) {
            return categories.contains(cat);
        } else {
            String[] cats = cat.split("\\|");
            for (String c : cats) {
                if (categories.contains(c)) {
                    return true;
                }
            }
            return false;
        }
    }

    /**
     * Returns true if the given category is to get detailed output.
     * @param cat The given category (case insensitive).
     * @return True if the category output should be detailed.
     */
    public static boolean isDetailed(String cat) {
        cat = cat.toLowerCase();
        if (cat.indexOf("|") < 0) {
            return detailedCats.contains(cat);
        } else {
            String[] cats = cat.split("\\|");
            for (String c : cats) {
                if (detailedCats.contains(c)) {
                    return true;
                }
            }
            return false;
        }
    }

    /**
     * Remove a debug category from the active list.
     * @param cat The given category (case insensitive).
     * @return True iff the category was in the active list.
     */
    public static boolean removeCategory(String cat) {
        cat = cat.toLowerCase();
        detailedCats.remove(cat);
        return categories.remove(cat);
    }

    public static String getCategories() {
        StringBuffer sb = new StringBuffer();
        for (String cat : categories) {
            sb.append(cat).append(NL);
        }
        return sb.toString();
    }
}
