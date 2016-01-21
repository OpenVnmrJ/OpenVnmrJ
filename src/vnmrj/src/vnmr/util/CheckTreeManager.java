/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import javax.swing.event.*;
import java.awt.event.*;
import javax.swing.tree.*;


/********************************************************** <pre>
 * Summary: Turn a JTree into a JTree with CheckBoxs
 *
 * Check boxes are tri state.  Unselected, Selected, and gray selected
 * where gray selected means that some, but not all, items below this node
 * have been selected.  If all items below a node are selected, it is a normal
 * black check mark.
 *
 * To use, create a CheckTreeManager with your JTree as its arg
 * JTree MyTree;
 * Create the tree as desired
 * CheckTreeManager checkTreeManager = new CheckTreeManager(tree);

 * to get the paths that were checked
 * TreePath checkedPaths[] =
 *                checkTreeManager.getSelectionModel().getSelectionPaths();
 * 
 * The basic code was obtained from http://jroller.com/page/santhosh
 *
 </pre> **********************************************************/

public class CheckTreeManager extends MouseAdapter implements TreeSelectionListener{ 
    private CheckTreeSelectionModel selectionModel; 
    private JTree tree = new JTree(); 
    int hotspot = new JCheckBox().getPreferredSize().width; 
 
    public CheckTreeManager(JTree tree){ 
        this.tree = tree; 
        selectionModel = new CheckTreeSelectionModel(tree.getModel()); 
        tree.setCellRenderer(new CheckTreeCellRenderer(tree.getCellRenderer(), 
                                                       selectionModel)); 
        tree.addMouseListener(this); 
        selectionModel.addTreeSelectionListener(this); 
    } 
 
    public void mouseClicked(MouseEvent me){ 
        TreePath path = tree.getPathForLocation(me.getX(), me.getY()); 
        if(path==null) 
            return; 
        if(me.getX()>tree.getPathBounds(path).x+hotspot) 
            return; 
 
        boolean selected = selectionModel.isPathSelected(path, true); 
        selectionModel.removeTreeSelectionListener(this); 
 
        try{ 
            if(selected) 
                selectionModel.removeSelectionPath(path); 
            else 
                selectionModel.addSelectionPath(path); 
        } finally{ 
            selectionModel.addTreeSelectionListener(this); 
            tree.treeDidChange(); 
        } 
    } 
 
    public CheckTreeSelectionModel getSelectionModel(){ 
        return selectionModel; 
    } 
 
    public void valueChanged(TreeSelectionEvent e){ 
        tree.treeDidChange(); 
    } 
}
