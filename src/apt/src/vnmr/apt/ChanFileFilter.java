/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.File;
import java.io.FilenameFilter;


/**
 * A FilenameFilter that selects channel files (with names like "chan#N").
 */
public class ChanFileFilter implements FilenameFilter {

    @Override
    /**
     * Select for files with names like "chan#N".
     * @param dir The directory containing the file.
     * @param name The name of the file.
     * @return True if this has the name of a channel file.
     */
    public boolean accept(File dir, String name) {
        return name.matches("chan#[0-9]+");
    }

    /**
     * Get the index number of the channel file with the specified name.
     * Parses the integer string following the "#" in the name.
     * @param name The channel name or path.
     * @return The channel number.
     */
    static public int getChanNumber(String name) {
        int idx = -1;
        try {
            idx = Integer.parseInt(name.substring(name.lastIndexOf('#') + 1));
        } catch (Exception e) {}
        return idx;
    }
}
