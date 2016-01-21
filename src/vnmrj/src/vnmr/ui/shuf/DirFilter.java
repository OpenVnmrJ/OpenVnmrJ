/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.io.*;


/********************************************************** <pre>
 * Summary: File filter for listing files in a directory that match
 *    or do not match the suffix's and prefix given.
 *
 * @author  Glenn Sullivan
 </pre> **********************************************************/

public class DirFilter implements FilenameFilter {
    /** prefix to allow or disallow. */
    protected String prefix;
    /** List of suffix's to allow or disallow. */
    protected String[] suffix;
    /** Allow request for matching entries or non matching entries. */
    protected boolean matching;
    protected String objType;

    DirFilter (String prefix, String[] suffix, boolean matching) {
        this.prefix = prefix;
        this.suffix = suffix;
        this.matching = matching;
        this.objType = null;
    }
    DirFilter (String prefix, String[] suffix, boolean matching,
               String objType) {
        this.prefix = prefix;
        this.suffix = suffix;
        this.matching = matching;
        this.objType = objType;
    }

    /************************************************** <pre>
     * Summary: Test input file (fullpath) to see if it matches the
     *    suffix and prefix for this filter..
     *
     </pre> **************************************************/

    public boolean accept(File parentDir, String name) {
        File file;

        // Keep matching entries
        if(matching) {
            if(objType != null && objType.equals(Shuf.DB_STUDY)) {
                // reject if this dir does not have a studypar in it
                file = new File(parentDir,
                                 name + File.separator +"studypar");
                if(file.exists())
                    return true;
                else
                    return false;
            }
            if(objType != null && objType.equals(Shuf.DB_LCSTUDY)) {
                // reject if this dir does not have a lcpar in it
                file = new File(parentDir,
                                 name + File.separator +"lcpar");
                if(file.exists())
                    return true;
                else
                    return false;
            }
            else if(objType != null && objType.equals(Shuf.DB_AUTODIR)) {
                // reject if this dir does not have an autopar or doneQ
                file = new File(parentDir,
                                 name + File.separator + "autopar");
                if(!file.exists()) {
                    file = new File(parentDir,
                                     name + File.separator + "doneQ");
                    if(file.exists()) {
                        return true;
                    }
                    else
                        return false;
                }
                else
                    return true;
            }

            // If either test below fails, set match to false.

            // If suffix[0] is empty, then we are obviously not after
            // .fid files, so eliminate them
            if(suffix[0].length() == 0) {
                if(name.endsWith(Shuf.DB_FID_SUFFIX))
                   return false;
            }
            else {
                // This only works if there is only one suffix!!
                if(suffix[0] != null && suffix[0].length() > 0)
                    if(!name.endsWith(suffix[0]))
                        return false;
            }

            if(prefix != null && prefix.length() > 0)
                if(!name.startsWith(prefix))
                    return false;

            if(objType != null && objType.equals(Shuf.DB_PROTOCOL)){
                // Check to see if this is really a protocol
                BufferedReader in;
                String line;
                String fullpath = parentDir + File.separator + name;
                try {
                    in = new BufferedReader(new FileReader(fullpath));
                    // Only look at the top 40 lines.  don't want to spend too
                    // much time on wrong files.
                    for (int cnt=0;(line = in.readLine()) != null && cnt < 40; cnt++) {
                        if(line.indexOf("protocol title") != -1) {
                            in.close();
                            return true;
                        }
                    }
                    // Must be over 40 lines down or file did not have string, reject it
                    in.close();
                    return false;
                }
                catch(Exception ex) {
                    // Is there is a problem reading the file, we don't
                    // want it anyway.  Just skip the error
                }
            }
        }

        // Keep non-matching entries.
        else {
            if(prefix != null && prefix.length() > 0)
                if(name.startsWith(prefix))
                    return false;
            for(int i=0; i < suffix.length; i++) {
                if(suffix[i] != null && suffix[i].length() > 0)
                    if(name.endsWith(suffix[i]))
                        return false;
            }
        }

        // If neither suffix nor prefix, then everything matches.
        return true;
    }
}
