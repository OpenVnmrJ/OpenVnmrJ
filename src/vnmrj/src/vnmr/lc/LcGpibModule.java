/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


public class LcGpibModule implements Comparable {

    /**
     * List of known module ID strings (names) and their corresponding labels.
     * The name is what comes back from the module as its ID.
     * The label is what the user sees.
     * The refName is the base name of the CORBA ref string file.
     * The type of module (Detector or Pump).
     * <name>, <label>, <refName>, <type>
     */
    protected static final String[][] idList = {
        { "9150", "310 / 9050", "9050", "Detector"},
        { "9112", "230 / 9012", "9012", "Pump"},
    };

    public String name = "";
    public String label = "";
    public String refName = "";
    public String type = "";
    public int address = 0;

    public LcGpibModule() {}

    public LcGpibModule(String name, int address) {
        this.name = name;
        this.address = address;
        for (int i = 0; i < idList.length; i++) {
            if (idList[i][0].equals(name)) {
                label = idList[i][1];
                refName = idList[i][2];
                type = idList[i][3];
                break;
            }
        }
        label = label + " at " + address;
    }

    /**
     * Sort alphabetically by label.
     */
    public int compareTo(Object obj) {
        return label.compareTo(((LcGpibModule)obj).label);
    }
}
