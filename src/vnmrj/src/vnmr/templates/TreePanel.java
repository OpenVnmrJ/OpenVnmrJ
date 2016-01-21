/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import java.awt.*;
import java.awt.event.*;

import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;
    
/**
 * A JTree representation of a template document.
 * @author		Dean Sindorf
 */
public class TreePanel extends JSplitPane implements TreeSelectionListener
{
    private JTree              tree=null;
    private PanelTemplate      template=null;
    private VElement           root=null;
    protected DefaultTreeModel treeModel=null;
      
    //----------------------------------------------------------------
	/** constructor.  */ 
	//----------------------------------------------------------------
    public TreePanel (PanelTemplate t, JComponent panel) {        
		super (JSplitPane.HORIZONTAL_SPLIT);
    	template=t;
    	                
		setBorder (BorderFactory.createEmptyBorder (5, 5, 5, 5));

		root = t.rootElement();
		root.normalize ();
		treeModel = new DefaultTreeModel(root);
		
		tree = new JTree (treeModel);
		tree.getSelectionModel()
	        .setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
		tree.addTreeSelectionListener(this);
        tree.setEditable(true);
	
		// The right pane is used to render each scene
		   
		JScrollPane         treeView, panelView;
	
		setLeftComponent (treeView = new JScrollPane (tree));
		setPreferredSize (new Dimension (780, 580));
	    if(panel !=null){
			setRightComponent (panelView = new JScrollPane (panel));
			Dimension minimumSize = new Dimension (100, 50);
			panelView.setMinimumSize (minimumSize);
			panelView.setMinimumSize (minimumSize);			
			setDividerLocation (200);
		}
    }

    //----------------------------------------------------------------
	/** tree listener  */ 
	//----------------------------------------------------------------
	public void valueChanged (TreeSelectionEvent e) {
		TreePath    path;
 		VElement    selected;

		path = tree.getSelectionPath();

		if (path == null)
			return;
                
		selected = (VElement)path.getLastPathComponent ();
		if (selected instanceof VElement)
			template.setSelected(selected);
	 }

    //----------------------------------------------------------------
    /** Remove the currently selected node. */
    //----------------------------------------------------------------
    public void removeSelected(VElement selected) {
		if(selected == null)
			return;
		if(selected.getParent()==null){
    		System.out.println("TreePanel error: cannot remove root element");
			return;
		}
        treeModel.removeNodeFromParent(selected);
    }
 }

