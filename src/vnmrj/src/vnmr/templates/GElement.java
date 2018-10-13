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
import javax.swing.*;
import java.awt.event.*;
import java.util.*;

/**
 * A base class for elements that have a JComponent 
 *  @author		Dean Sindorf
 */
public class GElement extends VElement implements ActionListener
{
	private JComponent 			comp=null;
	
	// VElement overrides
	
	public JComponent jcomp()		{ 
		if(!activeJcomp())
		 	comp=build();
		return comp;
	}
	public boolean isJcomp()		{ return true;}
	public boolean activeJcomp()	{ return comp==null?false:true;}
	public boolean isActive()		{ return true;}	

	//----------------------------------------------------------------
	/** handler for edit events in gui elements  */
	//----------------------------------------------------------------
	public void actionPerformed(ActionEvent ae) {
		if(template.debug_actions){
			String s=type()+" "+ae.getActionCommand();
			System.out.println(s);
		}
	}
					
	//----------------------------------------------------------------
	/** build gui component  (Abstract method) */
	//----------------------------------------------------------------
	protected JComponent build()		{  
		System.out.print("GElement ERROR: build() must be overidden");
		return null; 
	}
		
	//----------------------------------------------------------------
	/** set attributes in XML element for save */
	//----------------------------------------------------------------
	public void setXMLAttributes() {
		setJAttributes(jcomp());
	}

	//----------------------------------------------------------------
	/** set JComponent attributes in XML element */
	//----------------------------------------------------------------
	public void setJAttributes(JComponent obj) {
	}

	//----------------------------------------------------------------
	/** get JComponent attributes from XML element for build */
	//----------------------------------------------------------------
	public void getJAttributes(JComponent obj) {
	}
	
	//----------------------------------------------------------------
	/** remove self  */
	//----------------------------------------------------------------
	public VElement remove (){
		super.remove();
		if(comp != null){
			Container parent=comp.getParent();
			parent.remove(comp);
			comp.revalidate();
		}
		return this;
	}
}
