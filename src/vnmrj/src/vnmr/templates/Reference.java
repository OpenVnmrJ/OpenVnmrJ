/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import vnmr.util.*;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.*;

import com.sun.xml.tree.*;
import org.w3c.dom.*;
import org.xml.sax.*;

/**
 * A utility element class to read and write embedded XML files
 *  @author		Dean Sindorf
 */
public class Reference extends VElement
{
	public boolean isLeaf () 		{ return false;}
	public boolean isActive()		{ return true;}	

    //----------------------------------------------------------------
	/** initialize element (once time after construction)  */
	//----------------------------------------------------------------
	public void init(Template m)	{
		if(notInitialized()){
			super.init(m);
			try { 
				open(); 
			}
			catch (java.io.IOException e){
		        Messages.postError("error reading reference");	
			}
		}
	}
	
    //----------------------------------------------------------------
	/** Parse an XML file and append as a child node to this element */
	//----------------------------------------------------------------
    public void open() throws IOException{
	    String 	path=FileUtil.openPath(getAttribute("file"));
	    try {
	    	if(path==null)
	    		throw new IOException();
	    	
	    	String uri="file:"+path;
			XmlDocument xdoc=template.open_xml(uri);
			VElement root=(VElement)xdoc.getDocumentElement();
			root=root.getFirstElement();
			
			if(getAttribute("keep").equals("yes")){
				template.doc.changeNodeOwner(root);
				add(root);
			}
			else{
				VElement parant=(VElement)getParentNode();
				template.doc.changeNodeOwner(root);
				parant.replaceChild(root,this);
			}
			template.init(root);			
			if(hasAttribute("loc")) // reposition branch
				root.setAttribute("loc",getAttribute("loc"));
		}
 		catch (IOException e) {
		    Messages.postError("Reference: could not open file: "+path);	
		}	
 		catch (DOMException de) {
		    Messages.postError("Reference: DOMException "+path);	
		}	
		catch (Exception ee) { 
			Messages.writeStackTrace(ee,"Reference: Exception");
		}
    }										

	//----------------------------------------------------------------
	/** write XML file component
	 *   	this method will be called only if attribute "keep" was set 
	 *   	to "yes" when the node was build. If keep="no" or was undefined
	 *      the node is expanded inline on build. 
	 * </pre>
	 */
	//----------------------------------------------------------------	
    public void writeXml(XmlWriteContext context){
		try{
			// remove child to suppress XML write of branch
			Node last=removeChild(getLastChild());
			super.writeXml(context); // temporarily childless
			appendChild(last); 		 // reattach child
		}
		catch (IOException e){
			Messages.postError("Reference IOException");
		}
		catch (DOMException de){
			Messages.postError("Reference DOMException");			
		}
    }
}
