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
import java.util.*;
import java.io.*;
import javax.swing.tree.*;

import com.sun.xml.tree.*;
import org.w3c.dom.*;

/**
 * The base class for all template ElementNode objects
 *  @author		Dean Sindorf
 */
public class VElement extends ElementNode implements MutableTreeNode
{
	protected  Template 		template=null;
	protected  boolean          init_flag=false;
    protected boolean m_bShow = true;
    private int m_indent = 0;

	// ElementNode utilities

	public String type() 					{  return getLocalName(); }
	public boolean hasAttribute(String s) 	{
		return getAttribute(s).length()==0? false:true;
	}

    public Template getTemplate(){
        return template;
    }
    public void setTemplate(Template t){
        template=t;
    }

    public boolean isVisible()
    {
        return m_bShow;
    }

    public void setVisible(boolean bShow)
    {
        m_bShow = bShow;
    }

	// member i/o routines

	public String text() 			    { return null; }
	public String toString() 			{ return type(); }
	public void setXMLAttributes() 		{ }
	public void getXMLAttributes() 		{ }

	// status methods

	public boolean isInitialized()		{ return init_flag;}
	public boolean notInitialized()		{ return !init_flag;}
	public boolean isActive()			{ return false;}
    public boolean isGroup() 			{ return !isLeaf();}

	//----------------------------------------------------------------
	/** initialization method (calls getXMLAttributes)  */
	//----------------------------------------------------------------
	public void init(Template m)		{
		template=m;
		getXMLAttributes();
		if(template.debug_init)
			System.out.println("init :"+type());
		init_flag=true;
	}

	//----------------------------------------------------------------
	/** overrides Element.writeXml (calls setXMLAttributes)  */
	//----------------------------------------------------------------
	public void writeXml(XmlWriteContext context) throws IOException{
		setXMLAttributes();
		super.writeXml(context);
	}

	//----------------------------------------------------------------
	/** Print element properties (System.out)  */
	//----------------------------------------------------------------
	public void dump()					{
		String s;
		System.out.print(type());
		NamedNodeMap attrs=getAttributes();
		for(int i=0;i<attrs.getLength();i++){
			Node a=attrs.item(i);
			System.out.print(" "+a.getNodeName()+'='+'"'+a.getNodeValue()+'"');
		}
		System.out.print("\n");
	}

    //*************  TreeNode interface implementation **************

    public TreeNode getChildAt (int n) {
    	//return (TreeNode)getChildNodes().item(n);

        int j = -1;
        TreeNode treenode = null;
        NodeList nodes = getChildNodes();
        int nSize = nodes.getLength();
        for (int i = 0; i < nSize; i++)
        {
            Node node = nodes.item(i);
            if (!(node instanceof VProtocolElement) ||
                ((VProtocolElement)node).isVisible())
               j = j+1;
           if (j == n)
           {
               treenode = (TreeNode)node;
               break;
           }
        }
        return treenode;
    }
    public int getChildCount () 		{ return getChildNodes().getLength();}
    public TreeNode getParent () 		{
 		Node parent=getParentNode();
		if((parent != null) && (parent instanceof TreeNode))
   			return (TreeNode)parent;
   		return null;
    }
    public int getIndex (TreeNode node) {
    	/*Vector v=childElements();
    	return v.indexOf(node); */

       int j = -1;
       Vector nodes=childElements();
       int pos = nodes.indexOf(node);
       int nLength = nodes.size();
       for (int i = 0; i < nLength; i++)
       {
           Node node2 = (Node)nodes.get(i);
           if (!(node2 instanceof VProtocolElement) ||
               ((VProtocolElement)node2).isVisible())
               j=j+1;
           if (node2.equals(node))
               return j;
       }
       return pos;
    }
    public boolean getAllowsChildren () { return true; }
    public boolean isLeaf() 			{ return getChildCount()==0?true:false;}
    public Enumeration children () 		{ return childElements().elements();}

    //*************  MutableTreeNode interface implementation ***********

	public void insert(MutableTreeNode child, int index){
	    Node ref=(Node)getChildAt(index);
		insertBefore((Node)child,ref);
		((VElement)child).init(template);
	}
    public void remove(int n)						{
        MutableTreeNode child=(MutableTreeNode)getChildAt(n);
        if(child !=null)
			removeChild((Node)child);
    }
    public void remove(MutableTreeNode node) 			{remove(node);}
  	public void removeFromParent() 						{remove();}
 	public void setParent(MutableTreeNode newParent) 	{
 		System.out.println("setParent not supported");
 	}
    public void setUserObject(Object object) 			{}

    //*******************************************************************
	public void insertBefore(MutableTreeNode child, MutableTreeNode ref){
		insertBefore((Node)child,(Node)ref);
		((VElement)child).init(template);
	}

	public void moveChild(MutableTreeNode child, MutableTreeNode ref){
	    if(child!=ref){
	        removeChild((Node)child);
	        insertBefore((Node)child,(Node)ref);
	    }
	}

	//----------------------------------------------------------------
	/** return a vector containing all Element child nodes  */
	//----------------------------------------------------------------
    protected Vector childElements() {
		Vector v=new Vector();
		NodeList nodes=getChildNodes();
		for(int i=0;i<nodes.getLength();i++){
			Node node=nodes.item(i);
			v.add(node);
		}
	    //System.out.println(toString()+" "+nodes.getLength()+" "+v.size());
    	return v;
    }

	//----------------------------------------------------------------
	/** return a vector containing all Element child nodes  */
	//----------------------------------------------------------------
    protected void removeChildren() {
     	NodeList nodes=getChildNodes();
    	for(int i=0;i<nodes.getLength();i++){
    		Node node=nodes.item(i);
    		removeChild(node);
    	}
    }

 	//----------------------------------------------------------------
	/** return first Element child  */
	//----------------------------------------------------------------
    public VElement getFirstElement()	{
    	Enumeration elems=children();
    	if(elems.hasMoreElements())
    		return (VElement)elems.nextElement();
    	return null;
    }

	//----------------------------------------------------------------
	/** add a child node  (overrides DOM method) */
	//----------------------------------------------------------------
	public Node appendChild(Node child){
		if(child instanceof VElement)
		    return super.appendChild(child);  // removes text nodes
		return null;
	}
	//----------------------------------------------------------------
	/** add a child node  */
	//----------------------------------------------------------------
	public VElement add(VElement child){
		VElement elem=(VElement)appendChild(child);
		child.init(template);
		return elem;
	}

	//----------------------------------------------------------------
	/** remove child node  */
	//----------------------------------------------------------------
	public VElement remove(VElement child){
		removeChild(child);
		return this;
	}

	//----------------------------------------------------------------
	/** remove self  */
	//---------------------------------------------------------------
	public VElement remove (){
		Node parent=getParentNode();
		if(parent != null)
			parent.removeChild(this);
		return this;
	}

	/**
	 * Set the indentation for this node.
	 * @param indent The indent distance (pixels).
	 */
        public void setIndent(int indent) {
            m_indent = indent;
        }

        /**
         * Get the indentation for this node.
         * @return The indent distance (pixels).
         */
        public int getIndent() {
            return m_indent ;
        }

	public int getIntAttribute(String attr, int defaultValue) {
	    int rtn = defaultValue;
	    try {
	        rtn = Integer.parseInt(getAttribute(attr));
	    } catch (NumberFormatException nfe) {
	    }
	    return rtn;
	}
}
