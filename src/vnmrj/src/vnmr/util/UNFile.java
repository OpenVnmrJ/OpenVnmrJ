/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import vnmr.ui.shuf.*;


/********************************************************** <pre>
 * Summary: File Object which takes either unix or windows style paths
 *
 *     Since 'File' takes windows styles on windows systems, this
 *     will convert unix to windows if on windows before calling super
 *     to make the object.
 *
 *     Override getCanonicalPath(), getParent(), listFiles() and list()
 *     to return unix style paths.
 </pre> **********************************************************/



public class UNFile extends File {

    public UNFile(String path) {
        super(UtilB.unixPathToWindows(path));
    }

    public UNFile(String path, String name) {
        super(UtilB.unixPathToWindows(path), name);
    }



    /* Override the File.getCanonicalPath() method.
       In case we are on a windows system, we need to convert the
       path to Unix style.
    */
    public String getCanonicalPath() {
        String canPath;
        int index;

        try {
            canPath = super.getCanonicalPath();
            // In case we are on a windows system, we need to convert the
            // path to Unix style.
            if(UtilB.OSNAME.startsWith("Windows") && canPath.length() > 0) {
                // remove the ':'
                index = canPath.indexOf(':');
                String string1 = canPath.substring(0, index);
                String string2 = canPath.substring(index+1);
                canPath = string1 + string2;

                // replace '\' with '/'
                canPath = canPath.replace('\\', '/');

                if(!canPath.startsWith("/dev/fs"))
                    // insert /dev/fs if not there
                    canPath = "/dev/fs/" + canPath;
            }
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return "";
        }
        return canPath;
    }

    public String getParent() {
        String parent;
        String unixPath;

        try {
            parent = super.getParent();
            // In case we are on a windows system, we need to convert the
            // path to Unix style.
            unixPath = UtilB.windowsPathToUnix(parent);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return "";
        }
        return unixPath;

    }

    public File getParentFile() {
        String parent;
        UNFile file;

        try {
            parent = super.getParent();
            file = new UNFile(parent);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return null;
        }
        return file;

    }


    public File[] listFiles() {
        File[] files;
        UNFile[] dbfiles;

        // Get the standard list of files which may be in windows format
        try {
            files = super.listFiles();
            if(files == null)
                return null;

            dbfiles = new UNFile[files.length];

            // Convert the File objects to UNFile objects
            for(int i=0; i < files.length; i++) {
                dbfiles[i] = new UNFile(files[i].getCanonicalPath());
            }
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return null;
        }

        return dbfiles;

    }

    public File[] listFiles(DirFilter filter) {
        File[] files;
        UNFile[] dbfiles;

        // Get the standard list of files which may be in windows format
        try {
            files = super.listFiles(filter);
            if(files == null)
                return null;

            dbfiles = new UNFile[files.length];

            // Convert the File objects to UNFile objects
            for(int i=0; i < files.length; i++) {
                dbfiles[i] = new UNFile(files[i].getCanonicalPath());
            }
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
            return null;
        }

        return dbfiles;

    }

// List returns just the file/directory names, not the fullpath,
// Thus, I don't think I should be doing anything to the items in the list.

//     public String[] list() {
//         String[] list;
//         String[] unixList;

//         try {
//             if(!isDirectory())
//                 return new String[0];
//             list = super.list();

//             unixList = new String[list.length];

//             // Convert paths to unix if necessary
//             for(int i=0; i < list.length; i++) {
//                 unixList[i] =  UtilB.windowsPathToUnix(list[i]);
//             }
//         }
//         catch (Exception e) {
//             Messages.writeStackTrace(e);
//             return null;
//         }
//         return unixList;
//     }


}
