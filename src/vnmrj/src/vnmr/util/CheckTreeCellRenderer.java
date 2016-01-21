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
import javax.swing.tree.*;
import java.awt.*;

/********************************************************** <pre>
 * Summary:  Used for a JTree with CheckBoxs
 *
 * See comments in CheckTreeManager.java
 *
 </pre> **********************************************************/

public class CheckTreeCellRenderer extends JPanel implements TreeCellRenderer{ 
    private CheckTreeSelectionModel selectionModel; 
    private TreeCellRenderer delegate; 
    private TristateCheckBox checkBox = new TristateCheckBox(); 
 
    public CheckTreeCellRenderer(TreeCellRenderer delegate, 
                                 CheckTreeSelectionModel selectionModel){ 
        this.delegate = delegate; 
        this.selectionModel = selectionModel; 
        setLayout(new BorderLayout()); 
        setOpaque(false); 
        checkBox.setOpaque(false); 
    } 
 
 
    public Component getTreeCellRendererComponent(JTree tree, Object value, 
                                     boolean selected, boolean expanded, 
                                     boolean leaf, int row, boolean hasFocus){ 
        Component renderer = delegate.getTreeCellRendererComponent(tree, value,
                                      selected, expanded, leaf, row, hasFocus); 
 
        TreePath path = tree.getPathForRow(row); 
        if(path!=null){ 
            if(selectionModel.isPathSelected(path, true)) 
                checkBox.setState(Boolean.TRUE); 
            else 
                checkBox.setState(selectionModel.isPartiallySelected(path) ? null : Boolean.FALSE); 
        } 
        removeAll(); 
        add(checkBox, BorderLayout.WEST); 
        add(renderer, BorderLayout.CENTER); 
        return this; 
    } 
} 

