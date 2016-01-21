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
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import java.awt.geom.*;
import java.util.*;
import java.io.*;
import java.beans.*;
import javax.swing.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.bo.*;
import vnmr.util.*;
import javax.swing.tree.*;
import javax.swing.event.*;
import org.w3c.dom.*;
import com.sun.xml.tree.*;
import java.lang.Integer;

import java.awt.font.*;

import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.SAXParseException;
import org.xml.sax.Attributes;
import javax.xml.parsers.SAXParserFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.FactoryConfigurationError;

public class RQTreeTableModel extends AbstractTreeTableModel {


    static public String treeTableXml = null;
    static public Hashtable cList = new Hashtable();
    static public Boolean b_no = new Boolean(false);
    static public Boolean b_yes = new Boolean(true);
    static public Boolean b_dis = new Boolean("");

    static protected ArrayList cNames = new ArrayList(); 
    static protected ArrayList cTypes = new ArrayList(); 
    static protected ArrayList cWidths = new ArrayList(); 
    static protected ArrayList cFonts = new ArrayList();
    static protected ArrayList cColors = new ArrayList();

    String rqmacro;
    int vColumnCount = 4;
    RQBuilder mgr;

    public RQTreeTableModel(VElement root, String path) {
	super(root);   

	treeTableXml = path;
	fillHashtable();
        configTreeTable();

	mgr = Util.getRQPanel().getReviewQueue().getMgr();
	mgr.setRqmacro(rqmacro);
    }

    public void setRqmacro() {
		// set macro name
		// this will be overwritten if defined in treeTableXml.
		rqmacro = Util.getLabel("mRQaction", "RQaction");
	}

    public String getRqmacro() {
	return rqmacro;
    }

    private void fillHashtable() {
        cList.put("treetable",              vnmr.ui.shuf.TreeTableModel.class);
        cList.put("string",                 String.class);
        cList.put("check",                  Boolean.class);
        cList.put("entry",                  JTextField.class);
        cList.put("entry2",                  JTextField.class);
        cList.put("entry3",                  JTextField.class);
        cList.put("entry4",                  JTextField.class);
    }

    private void configTreeTable() {

        setRqmacro();

      	cNames.clear();
	cTypes.clear();
	cWidths.clear();
	cFonts.clear();
	cColors.clear();

        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false);  // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            SAXParser parser = spf.newSAXParser();
	    String file = null;
	    if(treeTableXml != null)
             file = FileUtil.openPath(treeTableXml);
            if(file != null) {
                parser.parse( new File(file), new MySaxHandler() );
            } else {
		//cNames.add("name");
		//cTypes.add(cList.get("treetable"));
            }

        } catch (ParserConfigurationException pce) {
            System.out.println("The underlying parser does not support the " +
                               "requested feature(s).");
        } catch (FactoryConfigurationError fce) {
            System.out.println("Error occurred obtaining SAX Parser Factory.");
        } catch (Exception e) {
                e.printStackTrace();
        }

    }


    // The superclass's implementation would work, but this is more efficient.
       // public boolean isLeaf(Object node) { return getFile(node).isFile(); }

    //
    //  The TreeTableNode interface.
    //

    public int getColumnCount() {
        return cNames.size();
    }

    public int getVisibleColumnCount() {
	if(vColumnCount <= 0) vColumnCount = Math.min(getColumnCount(), 4);
	else if(vColumnCount > getColumnCount()) vColumnCount = getColumnCount();
        return vColumnCount;
    }

    public String getColumnName(int column) {
        return (String)cNames.get(column);
    }

    public Class getColumnClass(int column) {
        return (Class)cTypes.get(column);
    }

    public int getColumnWidth(int column) {
	Object obj = cWidths.get(column);
	if(obj == null) return 0;
	else {
	   return (new Integer((String)obj)).intValue();
	}
    }

    public Font getColumnFont(int column) {
        return (Font)cFonts.get(column);
    }

    public Color getColumnColor(int column) {
        return (Color)cColors.get(column);
    }

    public Object getValueAt(Object node, int column) {
	if(node == null) return null;

   	VElement elm =(VElement)node;
	String header = getColumnName(column);
	Class type = getColumnClass(column);
	if(header.equals("group")) {
	    int count = elm.getChildCount();
	    String group = elm.getAttribute(RQBuilder.ATTR_GROUP);
	    if(group == null || group.equals("")) 
	        return "";
	    else {
 		return group;
	    }
        } else if(type.getName().indexOf("Boolean") != -1) {
	    String value = elm.getAttribute(header);
	    if(value != null && value.equals("yes")) {
	        return b_yes;
	    } else if(value != null && value.equals("no")) {
		return b_no;
	    } else return b_dis; 
/*
	} else if(type.getName().indexOf("JComboBox") != -1) {
*/
	} else
	    return elm.getAttribute(header);
    }

    public void setValueAt(Object aValue, Object node, int column) {

	Object old = getValueAt(node, column);

	if(old.toString().equals(aValue.toString())) return;

	VElement elm =(VElement)node;
        String key = mgr.getKey(elm);
	if(key.indexOf(".fid") != -1) return;
	String name = getColumnName(column);
        if(aValue instanceof Boolean ) {
	    Boolean b = (Boolean)aValue;
	    String type = elm.getAttribute(RQBuilder.ATTR_TYPE);
	    if(b.booleanValue() == true) {
            	elm.setAttribute(name, "yes"); 
	        mgr.doSetValue(elm, name, "yes");  
	    } else { 
            	elm.setAttribute(name, "no"); 
	        mgr.doSetValue(elm, name, "no");  
	    }
/*
	    // set the same value for children
	    if(type.equals("study") && elm.hasChildNodes()) {
	        Object child;
		for(int i=0; i<elm.getChildCount(); i++) {
		    child = elm.getChildAt(i);
		    if(child != null) setValueAt(aValue, child, column);
		}    
		JTreeTable treeTable = 
		 mgr.getTreePanel();
		
		if(treeTable != null &&
		   treeTable.getTree().isExpanded(new TreePath(getPathToRoot(elm)))) {
		    treeTable.validate();
		    treeTable.repaint();
		}
	    }
*/
/*
	} else if(aValue instanceof JComboBox) {
*/
	} else if(name.equals(RQBuilder.ATTR_SLICES)) {
	    String vmax = elm.getAttribute(RQBuilder.ATTR_NSLICES);
	    String str = (String)aValue;
	    try {
		int imax = Integer.valueOf(vmax).intValue();
	        str = RQBuilder.fixImgstr(str,imax);
	    } catch(NumberFormatException e) {}
	    
            elm.setAttribute(name, str); 

	    mgr.doSetValue(elm, name, str);  

	} else if(name.equals(RQBuilder.ATTR_ECHOES)) {
	    String vmax = elm.getAttribute(RQBuilder.ATTR_NECHOES);
	    String str = (String)aValue;
	    try {
		int imax = Integer.valueOf(vmax).intValue();
	        str = RQBuilder.fixImgstr(str,imax);
	    } catch(NumberFormatException e) {}
	    
            elm.setAttribute(name, str); 

	    mgr.doSetValue(elm, name, str);  

	} else if(name.equals(RQBuilder.ATTR_ARRAY)) {
	    String vmax = elm.getAttribute(RQBuilder.ATTR_NARRAY);
	    String str = (String)aValue;
	    try {
		int imax = Integer.valueOf(vmax).intValue();
	        str = RQBuilder.fixImgstr(str,imax);
	    } catch(NumberFormatException e) {}
	    
            elm.setAttribute(name, str); 

	    mgr.doSetValue(elm, name, str);  

	} else if(name.equals(RQBuilder.ATTR_IMAGES)) {
            int ns = 1;
	    try {
                String vmax = elm.getAttribute(RQBuilder.ATTR_NSLICES);
		ns  = Integer.valueOf(vmax).intValue();
	        if(ns<1) ns=1;
	    } catch(NumberFormatException e) {}

            int ne = 1;
	    try {
                String vmax = elm.getAttribute(RQBuilder.ATTR_NECHOES);
		ne  = Integer.valueOf(vmax).intValue();
	        if(ne<1) ne=1;
	    } catch(NumberFormatException e) {}

            int na = 1;
	    try {
                String vmax = elm.getAttribute(RQBuilder.ATTR_NARRAY);
		na  = Integer.valueOf(vmax).intValue();
	        if(na<1) na=1;
	    } catch(NumberFormatException e) {}

	    int imax = ns*ne*na;
	    String str = (String)aValue;
	    try {
	        str = RQBuilder.fixImgstr(str,imax);
	    } catch(NumberFormatException e) {}

            elm.setAttribute(name, str); 

	    mgr.doSetValue(elm, name, str);  

	} else if(name.equals(RQBuilder.ATTR_FRAMES)) {
            int ns = 1;
	    try {
                String vmax = elm.getAttribute(RQBuilder.ATTR_NSLICES);
		ns  = Integer.valueOf(vmax).intValue();
	        if(ns<1) ns=1;
	    } catch(NumberFormatException e) {}

            int ne = 1;
	    try {
                String vmax = elm.getAttribute(RQBuilder.ATTR_NECHOES);
		ne  = Integer.valueOf(vmax).intValue();
	        if(ne<1) ne=1;
	    } catch(NumberFormatException e) {}

            int na = 1;
	    try {
                String vmax = elm.getAttribute(RQBuilder.ATTR_NARRAY);
		na  = Integer.valueOf(vmax).intValue();
	        if(na<1) na=1;
	    } catch(NumberFormatException e) {}

	    int imax = ns*ne*na;
	    String str = (String)aValue;
	    try {
		int istr = 1;
		if(str.indexOf("-") != -1) {
		    istr = Integer.valueOf(str.
			substring(0,str.indexOf("-"))).intValue();
		}
	        str = RQBuilder.fixImgstr(str,imax+istr-1);
	    } catch(NumberFormatException e) {}

            elm.setAttribute(name, str); 

	    mgr.doSetValue(elm, name, str);  
/*
	} else {
            elm.setAttribute(name, aValue.toString()); 
	    mgr.doSetValue(elm, name, aValue.toString());  
*/
	}  
    }

    public void setRoot(Object root) {
        Object oldRoot = this.root;
        this.root = root;
        if (root == null && oldRoot != null) {
            fireTreeStructureChanged(this, null);
        }
        else {
            nodeStructureChanged((TreeNode)root);
        }
    }

    public int getIndexOfChild(Object parent, Object child) {
        if(parent == null || child == null)
            return -1;
        return ((TreeNode)parent).getIndex((TreeNode)child);
    }

    public Object getChild(Object parent, int index) {
        return ((TreeNode)parent).getChildAt(index);
    }

    public int getChildCount(Object parent) {
        return ((TreeNode)parent).getChildCount();
    }

    public boolean isLeaf(Object node) {
        return ((TreeNode)node).isLeaf();
    }

    public void reload(TreeNode node) {
        if(node != null) {
            fireTreeStructureChanged(this, getPathToRoot(node), null, null);
        }
    }

    public void reload() {
        reload((TreeNode)this.root);
    }

    public void insertNodeInto(MutableTreeNode newChild,
                               MutableTreeNode parent, int index){
        parent.insert(newChild, index);

        int[]           newIndexs = new int[1];

        newIndexs[0] = index;
        nodesWereInserted(parent, newIndexs);
    }

    public void valueForPathChanged(TreePath path, Object newValue) {
        MutableTreeNode   aNode = (MutableTreeNode)path.getLastPathComponent();

        aNode.setUserObject(newValue);
        nodeChanged(aNode);
    }

    public void removeNodeFromParent(MutableTreeNode node) {
        MutableTreeNode         parent = (MutableTreeNode)node.getParent();

        if(parent == null)
            throw new IllegalArgumentException("node does not have a parent.");

        int[]            childIndex = new int[1];
        Object[]         removedArray = new Object[1];

        childIndex[0] = parent.getIndex(node);
        parent.remove(childIndex[0]);
        removedArray[0] = node;
        nodesWereRemoved(parent, childIndex, removedArray);
    }

    public void nodeChanged(TreeNode node) {
        if(listenerList != null && node != null) {
            TreeNode         parent = node.getParent();

            if(parent != null) {
                int        anIndex = parent.getIndex(node);
                if(anIndex != -1) {
                    int[]        cIndexs = new int[1];

                    cIndexs[0] = anIndex;
                    nodesChanged(parent, cIndexs);
                }
            }
            else if (node == getRoot()) {
                nodesChanged(node, null);
            }
        }
    }

    public void nodesWereInserted(TreeNode node, int[] childIndices) {
        if(listenerList != null && node != null && childIndices != null
           && childIndices.length > 0) {
            int               cCount = childIndices.length;
            Object[]          newChildren = new Object[cCount];

            for(int counter = 0; counter < cCount; counter++)
                newChildren[counter] = node.getChildAt(childIndices[counter]);
            fireTreeNodesInserted(this, getPathToRoot(node), childIndices,
                                  newChildren);
        }
    }

    public void nodesWereRemoved(TreeNode node, int[] childIndices,
                                 Object[] removedChildren) {
        if(node != null && childIndices != null) {
            fireTreeNodesRemoved(this, getPathToRoot(node), childIndices,
                                 removedChildren);
        }
    }

    public void nodesChanged(TreeNode node, int[] childIndices) {
        if(node != null) {
            if (childIndices != null) {
                int            cCount = childIndices.length;

                if(cCount > 0) {
                    Object[]       cChildren = new Object[cCount];

                    for(int counter = 0; counter < cCount; counter++)
                        cChildren[counter] = node.getChildAt
                            (childIndices[counter]);
                    fireTreeNodesChanged(this, getPathToRoot(node),
                                         childIndices, cChildren);
                }
            }
            else if (node == getRoot()) {
                fireTreeNodesChanged(this, getPathToRoot(node), null, null);
            }
        }
    }

    public void nodeStructureChanged(TreeNode node) {
        if(node != null) {
           fireTreeStructureChanged(this, getPathToRoot(node), null, null);
        }
    }

    public TreeNode[] getPathToRoot(TreeNode aNode) {
        return getPathToRoot(aNode, 0);
    }

    protected TreeNode[] getPathToRoot(TreeNode aNode, int depth) {
        TreeNode[]              retNodes;
        // This method recurses, traversing towards the root in order
        // size the array. On the way back, it fills in the nodes,
        // starting from the root and working back to the original node.

        /* Check for null, in case someone passed in a null node, or
           they passed in an element that isn't rooted at root. */
        if(aNode == null) {
            if(depth == 0)
                return null;
            else
                retNodes = new TreeNode[depth];
        }
        else {
            depth++;
            if(aNode == root)
                retNodes = new TreeNode[depth];
            else
                retNodes = getPathToRoot(aNode.getParent(), depth);
            retNodes[retNodes.length - depth] = aNode;
        }
        return retNodes;
    }

    public TreeModelListener[] getTreeModelListeners() {
        return (TreeModelListener[])listenerList.getListeners(
                TreeModelListener.class);
    }

    private void fireTreeStructureChanged(Object source, TreePath path) {
        // Guaranteed to return a non-null array
        Object[] listeners = listenerList.getListenerList();
        TreeModelEvent e = null;
        // Process the listeners last to first, notifying
        // those that are interested in this event
        for (int i = listeners.length-2; i>=0; i-=2) {
            if (listeners[i]==TreeModelListener.class) {
                // Lazily create the event:
                if (e == null)
                    e = new TreeModelEvent(source, path);
                ((TreeModelListener)listeners[i+1]).treeStructureChanged(e);
            }
        }
    }

    public EventListener[] getListeners(Class listenerType) {
        return listenerList.getListeners(listenerType);
    }

    private class MySaxHandler extends DefaultHandler {

        public void endDocument() {
        }

        public void endElement(String uri, String localName, String qName) {
        }
        public void error(SAXParseException spe) {
            System.out.println("Error at line "+spe.getLineNumber()+
                                   ", column "+spe.getColumnNumber());
        }
        public void fatalError(SAXParseException spe) {
            System.out.println("Fatal error at line "+spe.getLineNumber()+
                                         ", column "+spe.getColumnNumber());
        }
        public void startDocument() {
        }

        public void startElement(String uri,   String localName,
                                 String qName, Attributes attr) {
	    
	    String str = null;
	    String header = null;
	    String type = null;
            if (qName.equals("template")) {
		str = attr.getValue("macro");
		if(str != null && str.length() > 0) rqmacro = str;
		str = attr.getValue("visiblecolumns");
		if(str != null) {
		    vColumnCount = (new Integer(str)).intValue();
		}
	    } else if (qName.equals("column")) {
	      header = attr.getValue("header");
	      type = attr.getValue("class");

	      if(header != null && header.length() > 0 && type != null && type.length() > 0)
	      {   
		cNames.add(header);
		cTypes.add(cList.get(type));
		cWidths.add(attr.getValue("width"));

		String font = attr.getValue("font");
		String style = attr.getValue("style");
		String size = attr.getValue("size");
		if(font == null || font == "") font = "Dialog";
		if(style == null || style == "") style = "Plain";
		if(size == null || size == "") size = "12";
		cFonts.add(DisplayOptions.getFont(font, style, size));
		
	 	str = attr.getValue("color");	
		if(str == null || str == "") str = "black";
		cColors.add(DisplayOptions.getColor(str));
	      }
            }
        }
        public void warning(SAXParseException spe) {
            System.out.println("Warning at line "+spe.getLineNumber()+
                                     ", column "+spe.getColumnNumber());
        }
    }

} // class RQTreeTableModel 
