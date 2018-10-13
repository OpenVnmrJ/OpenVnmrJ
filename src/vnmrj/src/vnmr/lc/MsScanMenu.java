/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.lc;


/**
 * This is a combo box with the selection of all the MS scans defined in
 * the user's "lc/msmethods" directory.
 */
public class MsScanMenu extends LcTableMenu {

    /** 
     * Get the abstract path to the directory containing files to be listed.
     * Suitable as input to FileUtil.openPath(path). 
     * @return The abstract path.
     */
    protected String getPath() {
        return "USER/lc/msmethods";
    }
}
