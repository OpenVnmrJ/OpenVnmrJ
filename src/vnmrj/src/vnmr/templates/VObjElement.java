/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import java.util.*;
import java.io.*;
import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;

/**
 * A base class for elements that implement the VObjIF interface 
 *  @author		Dean Sindorf
 */
public class VObjElement extends GElement implements VObjDef, VObjIF
{
	static public boolean   save_position=true;
	static public boolean   save_size=true;

 	//+++++++++ VObjIF interface implementation ++++++++++++++++++++

    public void setAttribute(int t, String s){ vcomp().setAttribute(t,s);}
    public String getAttribute(int t)		 { return vcomp().getAttribute(t);}
    public void setEditStatus(boolean s)	 { vcomp().setEditStatus(s);}
    public void setEditMode(boolean s)	 	 { vcomp().setEditMode(s);}
    public void addDefChoice(String s)	 	 { vcomp().addDefChoice(s);}
    public void addDefValue(String s)	 	 { vcomp().addDefValue(s);}
    public void setDefLabel(String s)	 	 { vcomp().setDefLabel(s);}
    public void setDefColor(String s)	 	 { vcomp().setDefColor(s);}
    public void setDefLoc(int x, int y)	 	 { vcomp().setDefLoc(x,y);}
    public Point getDefLoc()			 { return vcomp().getDefLoc(); }
    public void setSizeRatio(double w, double h) { vcomp().setSizeRatio(w, h); }
    public void updateValue()	 		 	 { vcomp().updateValue();}
    public void setValue(ParamIF p)		     { vcomp().setValue(p);}
    public void setShowValue(ParamIF p)		 { vcomp().setShowValue(p);}
    public ButtonIF getVnmrIF() 		     { return vcomp().getVnmrIF(); }
    public void setVnmrIF(ButtonIF vif)	     {  vcomp().setVnmrIF(vif); }
    public void refresh()	 	 			 { 
    	//vcomp().setEditStatus(mgr().isEditing());
    	vcomp().refresh();
    }
    public void changeFont() 				 { vcomp().changeFont();}
    public void destroy()					 { vcomp().destroy();} 		
    
 	//+++++++++ TreeNode interface override methods +++++++++++++++++
 	
	public String toString () 	{ 
		if(hasAttribute("value"))
			return super.toString()+"  ("+getAttribute("value")+")";
		else
			return super.toString();			
	}

 	//+++++++++ VElement interface override methods +++++++++++++++++

    public void changeFocus(boolean s) {
	}

	public boolean isLeaf() { 
		return (this instanceof VGroupIF) ? false: true; 
	}

	public String getValue() { 
		String s=vcomp().getAttribute(LABEL);
		return s==null?"unknown value":s;
	} 

	public void setValue(String s)  { 
		vcomp().setAttribute(LABEL,s);} 	

 	//+++++++++ LayoutTemplate access methods ++++++++++++++++++++++++
	//----------------------------------------------------------------
	/** Return template cast as a PanelTemplate. */
	//----------------------------------------------------------------
	public LayoutBuilder builder()  	{ 
		return (LayoutBuilder)template;
	}

 	//+++++++++ other public methods +++++++++++++++++++++++++++++++++
			
	//----------------------------------------------------------------
	/** Set and return VObjIF component. */
	//----------------------------------------------------------------
	public VObjIF vcomp() { 
		return (VObjIF)jcomp(); 
	} 
		
	//----------------------------------------------------------------
	/** Build swing component.  */
	//----------------------------------------------------------------
	protected JComponent build()		{ 	
		VObjIF obj=builder().getVObj(type());
	   	getJAttributes((JComponent)obj);
   		getVAttributes(obj);
	   	return (JComponent)obj;
	}	

	//----------------------------------------------------------------
	/** Set attributes in XML element for save. */
	//----------------------------------------------------------------
	public void setXMLAttributes() {
		setJAttributes(jcomp());
		setVAttributes(vcomp());
	}

	//----------------------------------------------------------------
	/** Return true if attibute set in VObjIF component. */
	//----------------------------------------------------------------
	public boolean hasVAttribute(int s) {
		return vcomp().getAttribute(s)==null? false:true;		 
	}

	//----------------------------------------------------------------
	/** Get JComponent attributes from XML element for build. */
	//----------------------------------------------------------------
	public void getJAttributes(JComponent obj) {
		super.getJAttributes(obj);
		if(hasAttribute("loc")){
			String s=getAttribute("loc");
			StringTokenizer st = new StringTokenizer(s," ");
			if(st.countTokens() != 2){
				System.out.println("invalid position attribute: "+s);
				return;
			}
			int x=Integer.parseInt(st.nextToken());
			int y=Integer.parseInt(st.nextToken());
			obj.setLocation(x,y);
		}
		if(hasAttribute("size")){
			String s=getAttribute("size");
			StringTokenizer st = new StringTokenizer(s," ");
			if(st.countTokens() != 2){
				System.out.println("invalid size attribute: "+s);
				return;
			}
			int w=Integer.parseInt(st.nextToken());
			int h=Integer.parseInt(st.nextToken());
			obj.setSize(w, h);
			obj.setPreferredSize(new Dimension(w,h));
		}
		/*
		 * For some reason, the following is needed to make the
		 * sliding thingy appear in the VSlider widget.
		 */
		if (obj instanceof VSlider) {
		    obj.setBounds(obj.getX(), obj.getY(),
				  obj.getWidth()+1, obj.getHeight());
		}
	}

	//----------------------------------------------------------------
	/** Set JComponent attributes in XML element. */
	//----------------------------------------------------------------
	public void setJAttributes(JComponent obj) {
		if(obj==null){
			System.out.println("VObjElement ERROR: jcomp not defined "+type());
			return;
		}
		super.setJAttributes(obj);
		if(save_position){
			JComponent jc=(JComponent)obj;		
			int x=jc.getX();
			int y=jc.getY();
			if(x>0 || y>0)
				setAttribute("loc",""+x+" "+y);
		}
		if(save_size){
			JComponent jc=(JComponent)obj;
			int w=jc.getWidth();
			int h=jc.getHeight();
			if(w>0 && h>0)
				setAttribute("size",""+w+" "+h);
		}
	}

	//----------------------------------------------------------------
	/** Get VObjIF attributes from XML element.  */
	//----------------------------------------------------------------
	public void getVAttributes(VObjIF obj) {
		Enumeration attrs=LayoutBuilder.keys();
		boolean newfont=false;
		while(attrs.hasMoreElements()){
			String  key=(String)attrs.nextElement();
			if(hasAttribute(key)){
				int at=LayoutBuilder.getAttribute(key);
				obj.setAttribute(at,getAttribute(key));
				if(at==FONT_STYLE || at==FONT_SIZE || at==FONT_NAME)
				 	newfont=true;
			}
		}
		if(newfont)
			obj.changeFont();
	}

	//----------------------------------------------------------------
	/** Set VObjIF attributes in XML element. */
	//----------------------------------------------------------------
	public void setVAttributes(VObjIF obj) {
		if(obj==null){
			System.out.println("VObjElement ERROR: vcomp not defined");
			return;
		}
		Enumeration attrs=LayoutBuilder.keys();
		while(attrs.hasMoreElements()){
			String  key=(String)attrs.nextElement();
			int at=LayoutBuilder.getAttribute(key);
			if(hasVAttribute(at))
				setAttribute(key,obj.getAttribute(at));
		}
	}	
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
}
