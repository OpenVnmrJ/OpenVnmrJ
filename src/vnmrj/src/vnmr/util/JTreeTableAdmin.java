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
import javax.swing.table.*;

import java.awt.Dimension;
import java.awt.Component;
import java.awt.Graphics;

import vnmr.ui.shuf.*;
import vnmr.ui.shuf.AbstractCellEditor;

public class JTreeTableAdmin extends JTable 
{
    protected TreeTableCellRenderer tree;

    public JTreeTableAdmin(TreeTableModel treeTableModel )
    {
	super();

	// Create the tree. It will be used as a renderer and editor. 
	tree = new TreeTableCellRenderer(treeTableModel); 

	// Install a tableModel representing the visible rows in the tree. 
	super.setModel(new TreeTableModelAdapter(treeTableModel, tree));

	// Force the JTable and JTree to share their row selection models. 
	tree.setSelectionModel(new DefaultTreeSelectionModel() { 
	    // Extend the implementation of the constructor, as if: 
	 /* public this() */ {
		setSelectionModel(listSelectionModel); 
	    } 
	}); 
	// Make the tree and table row heights the same. 
	tree.setRowHeight(getRowHeight());

	// Install the tree editor renderer and editor. 
	setDefaultRenderer(TreeTableModel.class, tree); 
	setDefaultEditor(TreeTableModel.class, new TreeTableCellEditor());  

	setShowGrid(false);
	setIntercellSpacing(new Dimension(0, 0)); 	        
    }

    /* Workaround for BasicTableUI anomaly. Make sure the UI never tries to 
     * paint the editor. The UI currently uses different techniques to 
     * paint the renderers and editors and overriding setBounds() below 
     * is not the right thing to do for an editor. Returning -1 for the 
     * editing row in this case, ensures the editor is never painted. 
     */
    public int getEditingRow() 
    {
        return (getColumnClass(editingColumn) == TreeTableModel.class) ? -1 : editingRow;  
    }

    // 
    // The renderer used to display the tree nodes, a JTree.  
    //

    public class TreeTableCellRenderer extends JTree 
    implements TableCellRenderer 
    {
	protected int visibleRow;
   
	public TreeTableCellRenderer(TreeModel model) 
	{ 
	    super(model); 
	}

	public void setBounds(int x, int y, int w, int h) 
	{
	    super.setBounds(x, 0, w, JTreeTableAdmin.this.getHeight());
	}

	public void paint(Graphics g) 
	{
	    g.translate(0, -visibleRow * getRowHeight());
	    super.paint(g);
	}

	public Component getTableCellRendererComponent(JTable table,
						       Object value,
						       boolean isSelected,
						       boolean hasFocus,
						       int row, int column) 
	{
	    if(isSelected)
		setBackground(table.getSelectionBackground());
	    else
		setBackground(table.getBackground());
       
	    visibleRow = row;
	    return this;
	}
    }

    /* 
     * The editor used to interact with tree nodes, a JTree.  
     */

    public class TreeTableCellEditor extends AbstractCellEditor 
    implements TableCellEditor 
    {
	public Component getTableCellEditorComponent(JTable table, 
						     Object value,
						     boolean isSelected, 
					  	     int r, int c) 
	{
	    return tree;
	}
    }

}
