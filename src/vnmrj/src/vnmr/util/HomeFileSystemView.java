/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;


import java.io.File;
import java.io.IOException;

import javax.swing.Icon;
import javax.swing.filechooser.FileSystemView;


/**
 * A FileSystemView is JFileChooser's gateway to the file system.
 * This is an extension to the default FileSystemView with user-specifiable
 * home directory. The home directory is the directory that a JFileChooser
 * will go to when it's "HOME" button is clicked.
 */
public class HomeFileSystemView extends FileSystemView {
    // The implementation is done by forwarding most method calls to
    // the default FileSystemView returned by
    // m_fileSystemView (= FileSystemView.getFileSystemView()).
    // The only exception is getHomeDirectory(). If a particular home
    // directory has been specified, that is what it returns.
    //
    // Note that the protected method "createFileSystemRoot" is not
    // implemented. That should not matter, because it can never be
    // called. Any calls to it from m_fileSystemView will use the
    // method defined in that class.

    /** The home directory. If null, use the system default. */
    private File m_homeDirectory;

    /** The default FileSystemView used to implement most of our methods. */
    private static FileSystemView m_fileSystemView
        = FileSystemView.getFileSystemView();


    /**
     * Make a HomeFileSystemView with a given home directory.
     * @param homeDirectory The File to use as the home directory.
     */
    public HomeFileSystemView(File homeDirectory) {
        m_homeDirectory = homeDirectory;
    }

    /**
     * Make a HomeFileSystemView with a default home directory.
     */
    public HomeFileSystemView() {
        m_homeDirectory = null;
    }


    public File createFileObject(File dir, String filename) {
        // Returns a File object constructed in dir from the given filename.
        return m_fileSystemView.createFileObject(dir, filename);
    }

    public File createFileObject(String path) {
        // Returns a File object constructed from the given path string.
        return m_fileSystemView.createFileObject(path);
    }

    public File createNewFolder(File containingDir) throws IOException {
        // Creates a new folder with a default folder name.
        return m_fileSystemView.createNewFolder(containingDir);
    }

    public File getChild(File parent, String fileName) {
           
        return m_fileSystemView.getChild(parent, fileName);
    }

    public File getDefaultDirectory() {
        // Return the user's default starting directory for the file chooser.
        return m_fileSystemView.getDefaultDirectory();
    }

    public File[] getFiles(File dir, boolean useFileHiding) {
        // Gets the list of shown (i.e. not hidden) files.
        return m_fileSystemView.getFiles(dir, useFileHiding);
    }

    public static FileSystemView getFileSystemView() {
           
        return FileSystemView.getFileSystemView();
    }

    /**
     * Returns the home directory.
     * If a particular home directory has been specified (in the constructor),
     * that is what it returns. Otherwise, it returns whatever the
     * default FileSystemView thinks is the home directory.
     */
    public File getHomeDirectory() {
        return m_homeDirectory == null
                ? m_fileSystemView.getHomeDirectory()
                : m_homeDirectory;
    }

    public File getParentDirectory(File dir) {
        // Returns the parent directory of dir.
        return m_fileSystemView.getParentDirectory(dir);
    }

    public File[] getRoots() {
        // Returns all root partitions on this system.
        return m_fileSystemView.getRoots();
    }

    public String getSystemDisplayName(File f) {
        // Name of a file, directory, or folder as it would be displayed
        // in a system file browser.
        return m_fileSystemView.getSystemDisplayName(f);
    }

    public Icon getSystemIcon(File f) {
        // Icon for a file, directory, or folder as it would be displayed
        // in a system file browser.
        return m_fileSystemView.getSystemIcon(f);
    }

    public String getSystemTypeDescription(File f) {
        // Type description for a file, directory, or folder as it would be
        // displayed in a system file browser.
        return m_fileSystemView.getSystemTypeDescription(f);
    }

    public boolean isComputerNode(File dir) {
        // Used by UI classes to decide whether to display a special icon
        // for a computer node, e.g.
        return m_fileSystemView.isComputerNode(dir);
    }

    public boolean isDrive(File dir) {
        // Used by UI classes to decide whether to display a special icon
        // for drives or partitions, e.g.
        return m_fileSystemView.isDrive(dir);
    }

    public boolean isFileSystem(File f) {
        // Checks if f represents a real directory or file as opposed
        // to a special folder such as "Desktop".
        return m_fileSystemView.isFileSystem(f);
    }

    public boolean isFileSystemRoot(File dir) {
        // Is dir the root of a tree in the file system, such as a drive
        // or partition.
        return m_fileSystemView.isFileSystemRoot(dir);
    }

    public boolean isFloppyDrive(File dir) {
        // Used by UI classes to decide whether to display a special icon
        // for a floppy disk.
        return m_fileSystemView.isFloppyDrive(dir);
    }

    public boolean isHiddenFile(File f) {
        // Returns whether a file is hidden or not.
        return m_fileSystemView.isHiddenFile(f);
    }

    public boolean isParent(File folder, File file) {
        // On Windows, a file can appear in multiple folders,
        // other than its parent directory in the filesystem.
        return m_fileSystemView.isParent( folder,  file);
    }

    public boolean isRoot(File f) {
        // Determines if the given file is a root in the navigatable tree(s).
        return m_fileSystemView.isRoot(f);
    }

    public Boolean isTraversable(File f) {
        // Returns true if the file (directory) can be visited.
        return m_fileSystemView.isTraversable(f);
    }
}
