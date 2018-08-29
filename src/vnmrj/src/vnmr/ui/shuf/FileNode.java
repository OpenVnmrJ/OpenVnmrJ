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
import java.util.Date;

import vnmr.util.*;



/* A FileNode is a derivative of the File class - though we delegate to
 * the File object rather than subclassing it. It is used to maintain a
 * cache of a directory's children and therefore avoid repeated access
 * to the underlying file system during rendering.
 */
public class FileNode {
    File     file;
    public boolean isNew = true;
    public boolean isExist = false;
    Object[] children;

    public FileNode(File file) {
        this.file = file;
    }

    // Used to sort the file names.
    static private MergeSort  fileMS = new MergeSort() {
        public int compareElementsAt(int a, int b) {
            return ((String)toSort[a]).compareTo((String)toSort[b]);
        }
    };

    /**
     * Returns the the string to be used to display this leaf in the JTree.
     */
    public String toString() {
        return file.getName();
    }

    public File getFile() {
        return file;
    }

    // Since vnmr "files" are often actually directories, we need to only
    // specify something is a leaf if it is not a vnmr file/directory
    public boolean isLeaf() {

        if(file.isFile())
            return true;
        String name = file.getName();
        if(name.endsWith(Shuf.DB_FID_SUFFIX) ||
           name.endsWith(Shuf.DB_PAR_SUFFIX) ||
           name.endsWith(Shuf.DB_REC_SUFFIX) ||
           name.endsWith(Shuf.DB_IMG_DIR_SUFFIX) ||
           name.endsWith(Shuf.DB_IMG_FILE_SUFFIX)) {

            return true;
        }
        else
            return false;
    }

    public String getPath() {
        String path;
        try {
            // This is used for comparisons with DB items, and thus must
            // be the CanonicalPath
            path = file.getCanonicalPath();
        }
        catch (IOException e) {
            Messages.writeStackTrace(e);
            path = "";
        }
        return path;
    }

    public String getName() {
            return file.getName();
    }

    /**
     * Loads the children, caching the results in the children ivar.
     */
    protected Object[] getChildren() {

        if (children != null) {
            return children;
        }
        try {
            // Get file list excluding hidden ('.') files
            HiddenFileFilter hiddenFF = new HiddenFileFilter();
            String[] files = file.list(hiddenFF);
            if(files != null) {
                fileMS.sort(files);
                children = new FileNode[files.length];
                String path = file.getPath();
                for(int i = 0; i < files.length; i++) {
                    File childFile = new File(path, files[i]);
                    children[i] = new FileNode(childFile);
                }
            }
        } catch (SecurityException se) {}
        return children;
    }

    // FilenameFilter to exclude hidden files, ie., starting with '.'
    class HiddenFileFilter implements FilenameFilter {

        public boolean accept(File parentDir, String name) {
            if(name.startsWith("."))
                return false;
            else
                return true;
            
        }
    }
}
