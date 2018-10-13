/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import java.io.*;
import javax.swing.*;
import java.awt.*;
import java.util.*;

import vnmr.ui.*;
import vnmr.bo.*;

/**
 * A Base Template class for a generic JComponent panel 
 *  @author		Dean Sindorf
 */
public class PanelTemplate extends Template
{
	public JComponent 			panel=null;
	public Container            parent=null;
	public TreePanel			treepnl=null;   
	
	//+++++++++++++ public methods +++++++++++++++++++++++++++++++++++

	//----------------------------------------------------------------
	/** Constructor. */
	//----------------------------------------------------------------
	public PanelTemplate(JComponent pnl)	{  
		super();
		panel=pnl;
		parent=panel.getParent();
	}
	
	//----------------------------------------------------------------
	/** Open a template file and build the panel.*/
	//----------------------------------------------------------------
	public void open(String f)	throws Exception {
		try {
			super.open(f);
	   	}
	   	catch (Exception e) {
			return;
	   	}
		build();
		if(visTreePanel())
			showTreePanel();
		panel.validate();
		repaint();
	}
	
	protected JComponent   last_jcomp=null;		

	//----------------------------------------------------------------
	/** Build the swing panel. */ 
	//----------------------------------------------------------------
	public void build(){ 
		if(testdoc(true))
		    return;
		last_jcomp=panel;
		build(rootElement());
	}

	//----------------------------------------------------------------
	/** Recursive call to build JComponents in Swing panel. */ 
	//----------------------------------------------------------------
	protected void build(VElement elem){	
		Enumeration     elems=elem.children();
		JComponent 		jcomp=null;
		VElement		child;		
		
		if(elem instanceof GElement && elem.isGroup()){
			jcomp=last_jcomp;
			last_jcomp=((GElement)elem).jcomp();
		}
		
		while(elems.hasMoreElements()){
			child=(VElement)elems.nextElement();
			build(child);
		}

		if(elem instanceof GElement){
			if(elem.isGroup())
				last_jcomp=jcomp;
			JComponent test=((GElement)elem).jcomp();
			if(last_jcomp !=null && test!=null)
				add(last_jcomp,test);
		}
	}

	//----------------------------------------------------------------
	/** Add to a panel component group. */
	//----------------------------------------------------------------
	public void add(JComponent grp, JComponent obj){
		grp.add(obj);
	}

	//----------------------------------------------------------------
	/** Get the currently active panel. */
	//----------------------------------------------------------------
	public JComponent getPanel(){
		return panel;	
	}

	//----------------------------------------------------------------
	/** Set the currently active panel */
	//----------------------------------------------------------------
	public void setPanel(JComponent p){
		panel=p;	
	}

	//----------------------------------------------------------------
	/** Set the active panel's container. */ 
	//----------------------------------------------------------------
	public void setParent(Container c){ 
		parent=c;
	}

	//----------------------------------------------------------------
	/** Add panel to container. */ 
	//----------------------------------------------------------------
	public void addPanel(Container c){ 
		parent=c;		
		parent.add(panel);   
		parent.invalidate();
	}

	//----------------------------------------------------------------
	/** Initialize a new panel. */ 
	//----------------------------------------------------------------
	public void newPanel(){
		Container content=panel.getParent();
		panel.removeAll();
		panel.validate();
		newDocument();
		if(visTreePanel())
			showTreePanel();
        repaint();
	}

	//----------------------------------------------------------------
	/** Repaint the panel. */ 
	//----------------------------------------------------------------
	public void repaint(){
		if(parent!=null)
			parent.repaint();
	}

	//----------------------------------------------------------------
	/** Add JTree+panel to container. */ 
	//----------------------------------------------------------------
	private boolean validParent(){
		if(parent==null){
    		System.out.println("PanelTemplate error: no parent");
    		return false;
		}
		return true;
	}
	
	//~~~~~~~~~~~~~~~~ TreePanel methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
	//----------------------------------------------------------------
	/** Test for a treePanel instance. */ 
	//----------------------------------------------------------------
	public boolean visTreePanel(){ 
		return treepnl==null?false:true;
	}

	//----------------------------------------------------------------
	/** Show the tree panel */ 
	//----------------------------------------------------------------
	public void showTreePanel(){
		if(!validParent())
			return;
		Container content;
		if(visTreePanel()){
			content=treepnl.getParent();
			content.remove(treepnl);
		}
		else {
			content=panel.getParent();
			content.remove(panel);
		}
		treepnl=new TreePanel(this,panel);
		content.add(treepnl);
		content.validate();
		treepnl.validate();
	}

	//----------------------------------------------------------------
	/** Hide the tree panel. */ 
	//----------------------------------------------------------------
	public void hideTreePanel(){
		if(!validParent())
			return;
		if(treepnl==null)
			return;
		Container content=treepnl.getParent();
		content.remove(treepnl);
		treepnl=null;
		panel.validate();
		content.add(panel);   
		content.validate();
	}

	//~~~~~~~~~~~~~~~~ selection methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	//----------------------------------------------------------------
	/** Set selected element. */ 
	//----------------------------------------------------------------
	public void setSelected(VElement e){
		super.setSelected(e);
		if(debug_select && e instanceof GElement)
		   hilightElement((GElement)e);
	}

	//----------------------------------------------------------------
	/** Remove selected element. */ 
	//----------------------------------------------------------------
	public void removeSelected(){
		if(getSelected()==null)
			return;
		if(treepnl!=null)
			treepnl.removeSelected(getSelected());
		super.removeSelected();
		repaint();
	}

	private Color lastc=null;
	private JComponent lastj=null;
	//----------------------------------------------------------------
	/** Hilight jcomp of an element. */ 
	//----------------------------------------------------------------
	public void hilightElement(GElement e){
		if(!e.isJcomp())
			return;
		JComponent jc=e.jcomp();
		jc.requestFocus();
		
		if(lastj!=null){
			lastj.setForeground(lastc);
			lastj.invalidate();
		}
		lastj=jc;
		lastc=jc.getForeground();
		jc.setForeground(Color.red);
		jc.invalidate();
	}
}
