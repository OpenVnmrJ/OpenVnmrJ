/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.lc;

import java.io.File;
import java.io.FileFilter;
import java.util.TreeSet;

import javax.swing.JComboBox;

import vnmr.util.FileUtil;
import vnmr.util.Messages;

public class LcTableMenu extends JComboBox {

    public LcTableMenu() {
        refreshMenu();
    }

    /**
     * Refresh the choice menu.
     * Uses the names of files in the directory named by the getPath method.
     * Filters the result with the FileFilter from getFileFilter. Unless it's
     * overridden in a subclass, this will be a null filter.
     */
    protected void refreshMenu() {
        File dir = null;
        String dirpath = FileUtil.openPath(getPath());
        if (dirpath != null) {
            dir = new File(dirpath);
        }
        if (dir == null || !dir.isDirectory() || !dir.canRead()) {
            if (dir != null) {
                dir.delete();
            }
            dirpath = FileUtil.savePath(getPath());
            if (dirpath != null) {
                new File(dirpath).mkdirs();
            }
        }
        addItem(""); // Include a null choice
        if (dir.canRead()) {
            FileFilter filter = getFileFilter();
            File[] paths = null;
            if (filter == null) {
                paths = dir.listFiles();
            } else {
                paths = dir.listFiles(filter);
            }
            TreeSet<String> sortedNames = new TreeSet<String>();
            for (File path : paths) {
                sortedNames.add(path.getName());
            }
            for (String name : sortedNames) {
                addItem(name);
            }
        }
    }

    public void setSelectedItem(String value) {
        if (value.equals(getSelectedItem())) {
            return;
        }
        int len = getItemCount();
        for (int i = 0; i < len; i++) {
            if (value.equals(getItemAt(i))) {
                setSelectedIndex(i);
                return;
            }
        }
        setSelectedIndex(0);
    }

    /** 
     * Get the abstract path to the directory containing files to be listed.
     * Suitable as input to FileUtil.openPath(path). 
     * @return The abstract path.
     */
    protected String getPath() {
        return ".";
    }

    /**
     * Get a FileFilter used to filter the results of the directory listing.
     * This base implementation returns null, which the refreshMenu method
     * interprets as no filter.
     * 
     * @return The FileFilter or null.
     */
    protected FileFilter getFileFilter() {
        return null;
    }
}
