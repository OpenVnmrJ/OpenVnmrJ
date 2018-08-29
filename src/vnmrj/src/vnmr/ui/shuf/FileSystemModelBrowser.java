/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.io.File;
import java.util.Date;
import javax.swing.tree.*;
import javax.swing.event.*;

import vnmr.util.*;

/**
 * FileSystemModel is a TreeModel representing a hierarchical file 
 * system. Nodes in the FileSystemModel are FileNodes which, when they 
 * are directory nodes, cache their children to avoid repeatedly querying 
 * the real file system. 
 * 
 */

public class FileSystemModelBrowser  extends DefaultTreeModel 
                                             implements TreeModel {

    String root;
    FileNode fileNode;

    public FileSystemModelBrowser(String newRoot) { 
        super(new DefaultMutableTreeNode(newRoot));
        this.root = newRoot;

        fileNode = new FileNode(new File(root)); 
    }

    //
    // Some convenience methods. 
    //

    protected File getFile(Object node) {
        // This is written assuming one root node thus we always use
        // the fileNode saved when instantiated.

	return fileNode.getFile();       
    }

    protected Object[] getChildren(Object node) {
        // This is written assuming one root node thus we always use
        // the fileNode saved when instantiated.
	return fileNode.getChildren(); 
    }

    //
    // The TreeModel interface
    //

    public int getChildCount(Object node) { 
	Object[] children = getChildren(node); 
	return (children == null) ? 0 : children.length;
    }

    public Object getChild(Object node, int i) { 
	return getChildren(node)[i]; 
    }


    // This thing above was borrowed from a Sun Example and I am not really 
    // sure how this was supposed to work.  I also do not seem to get
    // consistent object types in the argument.  So the following seems to
    // work.  node.toString seems to either contain the full root path, or
    // the name of an item under the root path.  If we receive the root path,
    // then it is not a leaf.  Otherwise, concatonate the name received to
    // the root path and check to see if that item is a file.  If so, then
    // it must be a leaf which means that there will be no '+' for expanding
    // this item in the tree.  Also, catch if file does not exist and return
    // true.  This will stop the + is the file does not exist.  This is 
    // the case for sym links pointing to either no place, or pointing to
    // an unmounted partition on another machine
    public boolean isLeaf(Object node) { 
        boolean isFile, exists;
        String name = node.toString();
        if(name.equals(root))
            return false;

        String path = root + File.separator + name;
        UNFile file = new UNFile(path);

        isFile = file.isFile();
        exists = file.exists();
        return (isFile || !exists); 
    }

}


