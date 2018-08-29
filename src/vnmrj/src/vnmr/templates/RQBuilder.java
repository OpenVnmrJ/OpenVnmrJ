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
import javax.swing.table.TableColumn;
import javax.swing.table.DefaultTableCellRenderer;
import org.w3c.dom.*;
import com.sun.xml.tree.*;
import java.lang.Integer;
import javax.swing.DefaultCellEditor;
import javax.swing.table.TableCellRenderer;


import java.awt.font.*;

//import javax.swing.event.TreeSelectionEvent;
//import javax.swing.event.TreeSelectionListener;

public class RQBuilder extends Template
     implements PropertyChangeListener
{
    PBTreeTable         treeTable=null;
    ProtocolRenderer    treerenderer = null;
    RQTreeTableModel    treemodel=null;
    VElement            root=null;
    ReviewQueue          queue=null;
    ProtocolEditor      editor=null;
    boolean             expand_new=true;
    String              selected_id="0";
    int                 node_id=0;
    boolean             allow_nesting=true;
    Hashtable           excluded=null;
    int 		frows = 0;
    int 		fcols = 0;
    int 		autoDis = 0;
    ArrayList		groups = new ArrayList();
    ArrayList		gids = new ArrayList();
    ArrayList grplist = new ArrayList();
    ArrayList imglist = new ArrayList();
    ArrayList frmlist = new ArrayList();
    VElement            selectedNode=null;
    String rqSelection = "-1";
    String rqUpdate = "update";
    int rqSort = 0;
    int rqMode = 0;
    int rqLayout = 0;
    int rqbatchs = 0;
    int rqbatch = 0;
    boolean adjustWidth = true;
    int ilist[];
    int flist[];
    int selectedRow = -1;
    String              rtGroupPath="";
    String              selectedPath="";
    private int m_modifier=0;

    static public String ATTR_NAME      = "name";
    static public String ATTR_TYPE      = "type";
    static public String ATTR_TITLE     = "title";
    static public String ATTR_GROUP     = "group";
    static public String ATTR_TIME      = "time";
    static public String ATTR_OWNER     = "owner";
    static public String ATTR_SIZE      = "size";
    static public String ATTR_ID        = "id";
    static public String ATTR_TOOLTEXT  = "tooltext";
    static public String ATTR_ELEMENT   = "element";
    static public String ATTR_TRASH     = "trash";
    static public String ATTR_EXPAND    = "expand";
    static public String ATTR_SLICES    = "slices";
    static public String ATTR_ECHOES    = "echoes";
    static public String ATTR_ARRAY     = "array";
    static public String ATTR_FRAMES    = "frames";
    static public String ATTR_IMAGES    = "images";
    static public String ATTR_DISPLAY   = "display";
    static public String ATTR_SORT      = "sort";
    static public String ATTR_DIR       = "dir";
    static public String ATTR_NSLICES   = "ns";
    static public String ATTR_NECHOES   = "ne";
    static public String ATTR_NARRAY    = "na";
    static public String ATTR_SLICE     = "slice";
    static public String ATTR_ECHO      = "echo";
    static public String ATTR_IMAGE     = "image";
    static public String ATTR_EXT       = "ext";
    static public String ATTR_COPIES    = "copy";

    static public String STUDY          = "study";
    static public String SCAN           = "scan";
    static public String IMG            = "img";

    static public String SEA            = "sea";
    static public String ESA            = "esa";

    static public String REQ            = "REQ";
    static public String READY          = "Ready";
    static public String EXECUTING      = "Executing";
    static public String SKIPPED        = "Skipped";
    static public String COMPLETED      = "Completed";
    static public String QUEUED         = "Queued";

    static public final int FILENODES   = 2;
    static public final int ANY         = 3;
    static public final int NEW         = 4;

    static public final int EQ          = 1;
    static public final int GT          = 2;
    static public final int GE          = 3;
    static public final int LT          = 4;
    static public final int LE          = 5;
    static public final int BEFORE      = 6;
    static public final int AFTER       = 7;
    static public final int FIRST       = 8;
    static public final int LAST        = 9;
    static public final int ALL         = 10;
    static public final int ONE         = 11;
    static public final int INTO        = 12;

    static public final int CLICK       = 0;
    static public final int COPY        = 1;
    static public final int MOVE        = 2;

    String treeTableXml0 = "INTERFACE/DefaultRQTreeTable_";
    String treeTableXml = null;
    int prevW = 0;

    static public final String imgListFile = "INTERFACE/imageList";
    static public final String rqimgListFile = "PERSISTENCE/rqSelectedImgs";

    //----------------------------------------------------------------
    /** Constructor. */
    //----------------------------------------------------------------

    public RQBuilder(Object obj)
    {
        super();
        if(obj instanceof ReviewQueue)
            queue=(ReviewQueue)obj;
        else if(obj instanceof ProtocolEditor)
            editor=(ProtocolEditor)obj;

        treerenderer = new ProtocolRenderer();
        treerenderer.setLeafIcon(null);
        treerenderer.setClosedIcon(null);
        treerenderer.setOpenIcon(null);
        Color c=Color.yellow;
        if(DisplayOptions.isOption(DisplayOptions.COLOR,"Selected"))
            c=DisplayOptions.getColor("Selected");
	treerenderer.setBackgroundSelectionColor(c);

        DisplayOptions.addChangeListener(this);
        excluded=new Hashtable();
        //excluded.put(ATTR_GROUP,ATTR_GROUP);
    }

    public void setRTGroup(String groupPath) {
	rtGroupPath = groupPath;
        treeTable.validate();
        treeTable.repaint();
    }

    public void setAdjustWidth(boolean b) {
	adjustWidth = b;
    }

    public void initColumnSizes(int width) {

	if(treemodel == null) return;

	ArrayList widths = new ArrayList();
        TableColumn column = null;
        Component comp = null;
	int vColumnCount = treemodel.getVisibleColumnCount();
	if (vColumnCount == 0) vColumnCount = 4;

        int cwidth;

        for (int i = 0; i <treemodel.getColumnCount(); i++) {
	    cwidth = treemodel.getColumnWidth(i);
	    if(cwidth == 0) cwidth = width/(vColumnCount+1);
	    widths.add(new Integer(cwidth));
        }

	int cwidth0 = width;
	int col = Math.min(treemodel.getColumnCount(),vColumnCount);
        for (int i = 1; i <col; i++) {
	    cwidth0 -= ((Integer)widths.get(i)).intValue();
        }

	if(cwidth0 < 0) cwidth0 = 0;

        for (int i = 0; i < treemodel.getColumnCount(); i++) {
	    if(i == 0) cwidth = cwidth0;
	    else {
		cwidth = ((Integer)widths.get(i)).intValue();
	    }

	    if(cwidth > 0) {
                column = treeTable.getColumnModel().getColumn(i);
                column.setPreferredWidth(cwidth);
	    }
        }
    }

    public ArrayList getColumnWidths() {

	ArrayList widths = new ArrayList();

	if(treeTable == null) return widths;

	int count = Math.min(treemodel.getVisibleColumnCount(),treemodel.getColumnCount());
	for(int i=0; i<count; i++)
	   widths.add(new Integer(treemodel.getColumnWidth(i)));

	return widths;
    }

    public void setRqmacro(String str) {
	if(queue != null) queue.setRqmacro(str);
    }

    public String getRqmacro() {
	if(queue != null) return queue.getRqmacro();
	else return "";
    }

    public RQTreeTableModel getTreeTableModel() {
	    return treemodel;
    }

    private void debugDnD(String msg){
        if(DebugOutput.isSetFor("dnd"))
            Messages.postDebug("PE: " + msg);
    }

    //----------------------------------------------------------------
    /** Set default class bindings for xml keywords. */
    //----------------------------------------------------------------
    protected void setDefaultKeys(){
        setKey("filenode",  vnmr.templates.VFileElement.class);
        //setKey("protocol",  vnmr.templates.VProtocolElement.class);
        //setKey("action",    vnmr.templates.VActionElement.class);
        setKey("template",  vnmr.templates.VElement.class);
        setKey("ref",       vnmr.templates.Reference.class);
        setKey("*Element",  vnmr.templates.VElement.class);
    }

    //----------------------------------------------------------------
    /** remove non-key attributes Attributes node  */
    //----------------------------------------------------------------
     public void trimAttributes (VElement node) {
        if(node==null)
            node=rootElement();

        ElementTree treewalker=new ElementTree(node);

        while(node!=null){
            VElement elem=(VElement)node;
            if(isFilenode(elem)){
                NamedNodeMap attrs=elem.getAttributes();
                for(int i=0;i<attrs.getLength();i++){
                    Node a=attrs.item(i);
                    String s=a.getNodeName();
                    if(excluded.containsKey(s)){
                        elem.removeAttribute(s);
                        //invalidate(elem);
                    }
                }
            }
            node=(VElement)treewalker.getNext();
        }
    }

    //----------------------------------------------------------------
    /** Sole PropertyChangeListener interface method. */
    //----------------------------------------------------------------
    public void propertyChange(PropertyChangeEvent evt){
        setPanelColor();
        invalidateTree();
        if(treeTable != null) treeTable.repaint();
    }

    //----------------------------------------------------------------
    /** Set nesting flag. */
    //----------------------------------------------------------------
    public void setAllowNesting(boolean f){
        allow_nesting=f;
    }

    //----------------------------------------------------------------
    /** Set executing status. */
    //----------------------------------------------------------------
    public boolean allowNesting(){
        return allow_nesting;
    }

    //----------------------------------------------------------------
    /** Set expand_new flag. */
    //----------------------------------------------------------------
    public void setExpandNew(boolean f){
        expand_new=f;
        if(treeTable !=null)
            treeTable.invalidate();
       // invalidateTree();
    }

    //----------------------------------------------------------------
    /** Get expand_new flag. */
    //----------------------------------------------------------------
    public boolean getExpandNew(){
        return expand_new;
    }

    //----------------------------------------------------------------
    /** Return true if treeTable contains only a root element. */
    //----------------------------------------------------------------
    public boolean isEmpty(){
        root=rootElement();
        if(treemodel.getChildCount(root)==0)
            return true;
        else
            return false;
    }

    //----------------------------------------------------------------
    /** Return name attribute of root element. */
    //----------------------------------------------------------------
    public String rootName(){
        return rootElement().getAttribute(ATTR_NAME);
    }

    //----------------------------------------------------------------
    /** Set panel background color. */
    //----------------------------------------------------------------
    public void setPanelColor(){
        if(treeTable==null)
            return;
        Color color=null;
            color=Util.getBgColor();
/*
            color=DisplayOptions.getColor("Background");
*/
        if(color==null)
            color=Global.BGCOLOR;
        treeTable.setBackground(color);
	if(treerenderer != null) {
	treerenderer.setBackgroundNonSelectionColor(color);
        if(DisplayOptions.isOption(DisplayOptions.COLOR,"Selected"))
            color=DisplayOptions.getColor("Selected");
	treerenderer.setBackgroundSelectionColor(color);
	}
        //invalidateTree();
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void setSelected(VElement elem){
/*
        if(elem != null){
        	String s=elem.getAttribute(ATTR_ID);
        	if(s != null && selected_id!=null && selected_id.equals(s))
        	    return;
        }
*/
        setSelected(elem,CLICK);
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void setSelected(VElement elem, int mode){
        if(elem==null){
            selected_id="null";
            super.setSelected(null);
            treeTable.clearSelection();
        }
        else{
            String s=elem.getAttribute(ATTR_ID);
            //if(s != null && s.length()>0){
                selected_id=s;
                doSingleClick(mode);
                super.setSelected(elem);
            //}
        }
    }

    //----------------------------------------------------------------
    /** return path to Element node. */
    //----------------------------------------------------------------
    public TreePath getPath(VElement elem){
        if(elem==null)
            return null;
        return new TreePath(treemodel.getPathToRoot(elem));
    }

    //----------------------------------------------------------------
    /** return true Element is collapsed. */
    //----------------------------------------------------------------
    public boolean isCollapsed(VElement elem){
        if(elem==root)
            return false;
        TreePath path=getPath(elem);
        if(elem!=null)
            return treeTable.getTree().isCollapsed(path);
        return true;
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void selectElement(VElement elem){
        selectElement(elem,CLICK);
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void selectElement(VElement elem,int mode){
        if(elem !=null){
            TreePath path=new TreePath(treemodel.getPathToRoot(elem));
            treeTable.getTree().makeVisible(path);
            treeTable.getTree().setSelectionPath(path);
            setSelected(elem,mode);
        }
    }

    //================= Tree Display Methods =========================

    //----------------------------------------------------------------
    /** Invalidate and redraw the tree.  */
    //----------------------------------------------------------------
    public void invalidate(TreeNode elem) {
        //treemodel.nodeStructureChanged(elem);
        treemodel.nodeChanged(elem);
    }

    //----------------------------------------------------------------
    /** Invalidate and redraw the tree.  */
    //----------------------------------------------------------------
    public void invalidateTree() {
        if(rootElement()==null || treeTable==null)
            return;

        ElementTree treewalker=new ElementTree(rootElement());
        Node elem=(Node)treewalker.getNext();
        while(elem!=null){
            if(elem instanceof VTreeNodeElement)
                invalidate((TreeNode)elem);
            elem=(Node)treewalker.getNext();
        }
    }

    //----------------------------------------------------------------
    /** Expand an Element node. */
    //----------------------------------------------------------------
    public void expandElement(VElement elem){
        if(elem==null || treeTable==null)
            return;

        TreePath path=new TreePath(treemodel.getPathToRoot(elem));
	String expand = elem.getAttribute(ATTR_EXPAND);
        if(expand != null && expand.equals("yes") && treeTable.getTree().isCollapsed(path)) {
            treeTable.getTree().makeVisible(path);
            treeTable.getTree().expandPath(path);
	}

        expandElement(getElementAfter(elem,FILENODES));
   }

    //----------------------------------------------------------------
    /** Expand all Protocol nodes  */
    //----------------------------------------------------------------
    public void expandTree() {
        expandElement(firstFilenode());
    }

    //----------------------------------------------------------------
    /** Compress an Element node. */
    //----------------------------------------------------------------
    public void collapseElement(VElement elem){
        if(elem !=null){
            TreePath path=new TreePath(treemodel.getPathToRoot(elem));
            treeTable.getTree().collapsePath(path);
        }
    }

    //----------------------------------------------------------------
    /** Collapse the tree  */
    //----------------------------------------------------------------
    public void collapseTree() {
        collapseElement(firstFilenode());
    }

    //----------------------------------------------------------------
    /** Build the tree panel. */
    //----------------------------------------------------------------
    public JTreeTable buildTreePanel(){
        treeTable = new PBTreeTable (treemodel);
        ToolTipManager.sharedInstance().registerComponent(treeTable);
        setPanelColor();
        //treemodel.nodeStructureChanged(root);
        return treeTable;
    }

    //----------------------------------------------------------------
    /** Return the tree panel. */
    //----------------------------------------------------------------
    public JTreeTable getTreePanel(){
        return treeTable;
    }

    public TreePath getPathForRow(int row){
        return treeTable.getTree().getPathForRow(row);
    }

    //================= Element  methods =======================
    //----------------------------------------------------------------
    /** Clone an Element node. */
    //----------------------------------------------------------------
    public VElement cloneElement(VElement src){
        VElement elem=(VElement)src.cloneNode(true);
        setAll(elem,ANY,ATTR_ID,"");
        return elem;
    }

    //----------------------------------------------------------------
    /** Assign Ids to all Elements.  */
    //----------------------------------------------------------------
    public void setIds() {
        Node node=rootElement();
        ElementTree treewalker=new ElementTree(node);
        node_id=0;
        node=treewalker.getNext();
        trimAttributes((VElement)node);
        while(node!=null){
            VElement obj=(VElement)node;
            if(isFilenode(obj))
                obj.setAttribute(ATTR_ID,"n"+ ++node_id);
            node=treewalker.getNext();
        }
    }

    //----------------------------------------------------------------
    /** Return all Element attributes.  */
    //----------------------------------------------------------------
    public ArrayList getAttributes(VElement obj) {
        ArrayList list=new ArrayList();
        list.add(ATTR_TYPE);
        list.add(obj.getAttribute(ATTR_TYPE));

        list.add(ATTR_ID);
        list.add(obj.getAttribute(ATTR_ID));

        list.add(ATTR_GROUP);
        list.add(obj.getAttribute(ATTR_GROUP));

        NamedNodeMap attrs=obj.getAttributes();
        for(int i=0;i<attrs.getLength();i++){
            Node a=attrs.item(i);
            String s=a.getNodeName();
            if(s.equals(ATTR_TYPE))   continue;
            if(s.equals(ATTR_ID))     continue;
            if(s.equals(ATTR_GROUP)) continue;
            list.add(s);
            list.add(a.getNodeValue());
        }

        list.add(ATTR_ELEMENT);
        list.add(obj.getNodeName());

        return list;
    }

    //----------------------------------------------------------------
    /** Get attribute for an Element.  */
    //----------------------------------------------------------------
    public String getAttribute(VElement obj,String s) {
        return obj.getAttribute(s);
    }

    //----------------------------------------------------------------
    /** Set attribute for an Element.  */
    //----------------------------------------------------------------
    public void setAttribute(VElement obj, String s, String v) {
        if(s.equals(ATTR_ELEMENT))
            return;
        obj.setAttribute(s,v);
        invalidate(obj);
    }

    //----------------------------------------------------------------
    /** Set attribute for an Element.  */
    //----------------------------------------------------------------
    public void setAttribute(VElement obj, int type, String attr, String value) {
        if(isType(obj,type))
            obj.setAttribute(attr,value);
    }

    //----------------------------------------------------------------
    /** Set attribute for all Elements before target . */
    //----------------------------------------------------------------
    public void setBefore(VElement elem, int type, String attr, String value) {
        ArrayList list=getElementsBefore(elem,type);
        for(int i=0;i<list.size();i++){
            VElement obj=(VElement)list.get(i);
            setAttribute(obj,attr,value);
        }
    }

    //----------------------------------------------------------------
    /** Set attribute for all Elements after target . */
    //----------------------------------------------------------------
    public void setAfter(VElement elem, int type, String attr, String value) {
        ArrayList list=getElementsAfter(elem,type);
        for(int i=0;i<list.size();i++){
            VElement obj=(VElement)list.get(i);
            setAttribute(obj,attr,value);
        }
    }

    //----------------------------------------------------------------
    /** Set attribute for all Elements . */
    //----------------------------------------------------------------
    public void setAll(int type, String attr, String value) {
        setAll(rootElement(),type,attr,value);
    }

    //----------------------------------------------------------------
    /** Set attribute for all Elements . */
    //----------------------------------------------------------------
    public void setAll(VElement elem, int type, String attr, String value) {
        ArrayList list=getElements(elem,type);
        for(int i=0;i<list.size();i++){
            VElement obj=(VElement)list.get(i);
            obj.setAttribute(attr,value);
        }
    }

    //================= Element access methods =======================

    //----------------------------------------------------------------
    /** Return the root element. */
    //----------------------------------------------------------------
    public VElement rootTreeNode(){
        return root;
    }

    //----------------------------------------------------------------
    /** Get element with specified Id.  */
    //----------------------------------------------------------------
    public VElement lastElement() {
        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getNext();
        VElement last=null;
        while(node!=null){
            if(node instanceof VTreeNodeElement)
                last=(VElement)node;
            node=treewalker.getNext();
        }
        if(last==null)
            return rootElement();
        return last;
    }

    //----------------------------------------------------------------
    /** Get element with specified Id.  */
    //----------------------------------------------------------------
    public VElement getElement(String id) {
        if(id==null || id.equals("0") || id.equals("null")){
            return rootElement();
        }

        ElementTree treewalker=new ElementTree(rootElement());
        Node node=(Node)treewalker.getNext();
        VElement elem=null;
        while(node!=null){
            elem=(VElement)node;
	    if(id.compareToIgnoreCase(elem.getAttribute(ATTR_ID)) == 0) {
               return elem;
	    }
            node=treewalker.getNext();
        }

        return null;
    }

    //----------------------------------------------------------------
    /** Get typed elements in subtree elem. */
    //----------------------------------------------------------------
    public ArrayList getElements(VElement obj, int cond, int type) {
        ArrayList list=new ArrayList();
        if(obj==null)
            obj=rootElement();
        switch(cond){
        case LE:
        case LT:
            list=getElementsBefore(obj,type);
            if(cond==LE && isType(obj,type))
                list.add(obj);
            break;
        case GE:
        case GT:
            list=getElementsAfter(obj,type);
            if(cond==GE && isType(obj,type))
                list.add(obj);
            break;
        case EQ:
            list=getElements(obj,type);
            break;
        case ALL:
            list=getElements(null,type);
            break;
        case AFTER:
            obj=getElementAfter(obj,type);
            if(obj !=null)
                list.add(obj);
            break;
        case BEFORE:
            obj=getElementBefore(obj,type);
            if(obj !=null)
                list.add(obj);
            break;
        case FIRST:
            obj=getElementAfter(obj,type);
            if(obj !=null)
                list.add(obj);
            break;
        case ONE:
            if(obj==null)
                break;
            if(isType(obj,type))
                list.add(obj);
            else
                Messages.postWarning("SQ node is not specified type");
            break;
        }
        return list;
    }

    //----------------------------------------------------------------
    /** Get typed elements in subtree elem. */
    //----------------------------------------------------------------
    public ArrayList getElements(VElement elem, int type) {
        ArrayList list=new ArrayList();
        if(elem==null)
            elem=rootElement();
        ElementTree treewalker=new ElementTree(elem);
        Node node=treewalker.getCurrent();

        while(node!=null){
            VElement obj=(VElement)node;
            if(isType(obj,type))
                list.add(obj);
            node=treewalker.getNext();
        }
        return list;
    }

    //----------------------------------------------------------------
    /** Get typed elements before node with specified Id.  */
    //----------------------------------------------------------------
    public ArrayList getElementsBefore(VElement elem, int type) {
        ArrayList list=new ArrayList();
        if(elem==null)
            return list;
        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getCurrent();
        while(node!=null){
            VElement obj=(VElement)node;
            if(node==(Node)elem)
                return list;
            else if(isType(obj,type))
                list.add(obj);
            node=treewalker.getNext();
        }
        return list;
    }

    //----------------------------------------------------------------
    /** Get typed element before node with specified Id.  */
    //----------------------------------------------------------------
    public VElement getElementBefore(VElement elem, int type) {
        ArrayList list=getElementsBefore(elem,type);
        if(list.size()==0)
            return null;
        return (VElement)list.get(list.size()-1); // last element
    }

    //----------------------------------------------------------------
    /** Get typed elements after node with specified Id . */
    //----------------------------------------------------------------
    public ArrayList getElementsAfter(VElement elem, int type) {
        ArrayList list=new ArrayList();
        if(elem==null)
            elem=rootElement();
        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getCurrent();

        boolean found=false;
        while(node!=null){
            if(node==(Node)elem)
                found=true;
            else if(found){
                VElement obj=(VElement)node;
                if(isType(obj,type))
                    list.add(obj);
            }
            node=treewalker.getNext();
        }
        return list;
    }

    //----------------------------------------------------------------
    /** Get typed element after node with specified Id.  */
    //----------------------------------------------------------------
    public VElement getElementAfter(VElement elem, int type) {
        ArrayList list=getElementsAfter(elem,type);
        if(list.size()==0)
            return null;
        return (VElement)list.get(0);  // first element
    }

    //----------------------------------------------------------------
    /** Test if an Element is a decendent of another Element. */
    //----------------------------------------------------------------
    public boolean isDecendent(TreeNode parent, TreeNode child) {
        ElementTree treewalker=new ElementTree((Node)parent);
        Node elem=(Node)treewalker.getNext();
        while(elem!=null){
            if(elem==child)
                return true;
            elem=(Node)treewalker.getNext();
        }
        return false;
    }

    //----------------------------------------------------------------
    /** Test if an Element preceeds another Element in the tree. */
    //----------------------------------------------------------------
    public boolean isBefore(TreeNode e1, TreeNode e2) {
        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getNext();

        boolean found1=false;

        while(node!=null){
            if(node==(Node)e1)
                found1=true;
            else if(node==(Node)e2){
                if(found1)
                    return false;
                return true;
            }
            node=treewalker.getNext();
        }
        return true;
    }


    //----------------------------------------------------------------
    /** Does element with this path exist?  If so, return the element,
        else return null.  */
    //----------------------------------------------------------------
    public VElement getElementWithPath(String path) {

        if(path==null || path.equals("0") || path.equals("null")){
            return null;
        }

        ElementTree treewalker=new ElementTree(rootElement());
        Node node=(Node)treewalker.getNext();
        if(node == null)
            return null;

        VElement elem=null;
        // Do through all of the tree nodes of the RQ panel
        while(node!=null){
            String dir=null;
            String name=null;
            elem=(VElement)node;

	    String nodePath = elem.getAttribute(ATTR_DIR) +"/"+
		  elem.getAttribute(ATTR_NAME) +
		  elem.getAttribute(ATTR_EXT);

            if(path.equals(nodePath)) {
                // We have the arg path already in the tree,  return
                // this node element
                return elem;
            }
            node=treewalker.getNext();
        }

        // Not in the tree, return null
        return null;
    }

    //================= [VTreeNode Element] ===========================

    //----------------------------------------------------------------
    /** Return the first VTreeNode Element. */
    //----------------------------------------------------------------
    public VElement firstElement(){
        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getCurrent();
        while(node!=null){
            if(node instanceof VTreeNodeElement)
                return (VElement)node;
            node=treewalker.getNext();
        }
        return null;
    }

    //----------------------------------------------------------------
    /** return true if Element is of the specified type.  */
    //----------------------------------------------------------------
    public boolean isType(VElement obj, int type) {
        if((type&FILENODES)>0 && isFilenode(obj))
            return true;
        return false;
    }

    //================= [Protocol Element] ===========================

    //----------------------------------------------------------------
    /** Return the first Protocol Element. */
    //----------------------------------------------------------------
    public VElement firstFilenode(){
         return getElementAfter(rootElement(),FILENODES);
    }
    //----------------------------------------------------------------
    /** Test if an Element is a filenode. */
    //----------------------------------------------------------------
    public static boolean isFilenode (Node elem) {
        if(elem!=null && (elem instanceof VFileElement))
            return true;
        return false;
    }

    //=================== StudyQueue Methods =========================

    //----------------------------------------------------------------
    /** Return true tree can be rebuilt. */
    //----------------------------------------------------------------
    public boolean canLoadStudy(String fn){
        if(queue !=null && !queue.testRead(fn))
            return false;
        return true;
    }

    //----------------------------------------------------------------
    /** Return name of study in study dir. */
    //----------------------------------------------------------------
    public String getReviewName(String dir){
        if(queue !=null)
            return queue.reviewName(dir);
        return dir+"/study.xml";
    }

    //----------------------------------------------------------------
    /** Flag single-click event. */
    //----------------------------------------------------------------
    public void doSingleClick(int mode){
        if(queue!=null)
            queue.wasClicked(selected_id,mode);
    }

    //----------------------------------------------------------------
    /** Flag double-click event. */
    //----------------------------------------------------------------
    public void doDoubleClick(VElement elem, String action){
	selectedNode = elem;
	String key = getKey(elem);
	if(queue != null) queue.doDoubleClicked(key, action);
    }

    //----------------------------------------------------------------
    /** Flag tree modification event. */
    //----------------------------------------------------------------
    public void setDragAndDrop(){
        if(queue!=null)
            queue.setDragAndDrop(selected_id);
    }

    //=================== Tree Modification Methods ==================

    //----------------------------------------------------------------
    /** Open a new Protocol XML file. */
    //----------------------------------------------------------------
    public void newTree(){
        clearTree();
        newDocument();
        root=rootElement();
        if(treemodel==null) {
            treemodel=new RQTreeTableModel(root, null);
        } else{
            treemodel.setRoot(root);
	}
        if(editor!=null) {
            editor.setTitle(rootElement());
	}
    }

    //----------------------------------------------------------------
    /** Open a new Protocol XML file. */
    //----------------------------------------------------------------
    public void newTree(String path){

        if(path == null) return;
        clearTree();
        try {
            open(path);
        }
        catch(Exception e) {
           Messages.postError("could not open study "+path);
           return;
        }
        root=rootElement();
	String tpath = treeTableXml0 + root.getAttribute("type") + ".xml";
        if(treemodel==null) {
	    treeTableXml = tpath;
            treemodel=new RQTreeTableModel(root, treeTableXml);
	} else if(!tpath.equals(treeTableXml)) {
	    treeTableXml = tpath;
            treemodel=new RQTreeTableModel(root, treeTableXml);
	    Util.getRQPanel().buildNewRQ();
        } else
            treemodel.setRoot(root);
        if(queue==null)
            setIds();
        if(editor!=null)
            editor.setTitle(root);
        if(expand_new)
            expandTree();
        if(selectedRow != -1) {
            treeTable.getTree().setSelectionRow(selectedRow);
            treeTable.setRowSelectionInterval(selectedRow,selectedRow);
            selectedRow = -1;
        }
    }

    //----------------------------------------------------------------
    /** Clear all tree items. */
    //----------------------------------------------------------------
    public void clearTree(){
        if(root !=null)
            removeChildren(root);
        if(editor!=null)
            editor.setTitle(rootElement());
    }

    //----------------------------------------------------------------
    /** Remove all child nodes. */
    //----------------------------------------------------------------
    public void removeChildren(VElement parent){
        if(parent==null)
            return;
        Enumeration nodes=parent.children();
        while(nodes.hasMoreElements()){
            MutableTreeNode node=(MutableTreeNode)nodes.nextElement();
            if(treemodel!=null)
                treemodel.removeNodeFromParent(node);
        }
        parent.removeChildren();
     }

    //----------------------------------------------------------------
    /** Get a child Element. */
    //----------------------------------------------------------------
    public VElement getElement(VElement parent, int index){
        return (VElement)treemodel.getChild(parent,index);
    }

    //----------------------------------------------------------------
    /** Build a new Element by parsing a file. */
    //----------------------------------------------------------------
    public VElement buildElement(String path){
        RQBuilder builder=new RQBuilder(null);
        try {
            builder.open(path);
        }
        catch(Exception e) {
           return null;
        }
        VElement elem=builder.rootElement();
        if(elem!=null)
            doc.changeNodeOwner(elem);
        return elem;
    }

    public void stopTableCellEditing() {
	treeTable.stopTableCellEditing();
    }

    //******************* Local Classes *****************************

    public class PBTreeTable extends JTreeTable implements DragGestureListener,
                                                 DragSourceListener,
                                                 DropTargetListener,
                                                 MouseListener {
        DragSource dragSource;
        DropTarget dropTarget;
        Cursor  oldCursor = null;
	RQTreeTableModel model = null;
	int editingRow = -1;
	int editingCol = -1;
	String keyText = "";

        public PBTreeTable(RQTreeTableModel model) {
            super(model);
	    this.model = model;
            putClientProperty("JTree.lineStyle", "Angled");
            getTree().setShowsRootHandles(true);
            getTree().setRootVisible(false);
	    getTree().setCellRenderer(treerenderer);
	    setSelectionBackground(treerenderer.getBackgroundSelectionColor());
	    setSelectionForeground(treerenderer.getForeground());

            getSelectionModel().setSelectionMode(
                             ListSelectionModel.SINGLE_SELECTION);
            getTree().getSelectionModel().setSelectionMode(
                             ListSelectionModel.SINGLE_SELECTION);

            getTree().setExpandsSelectedPaths(true);

	    setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

            addMouseListener(PBTreeTable.this);
            dropTarget = new DropTarget(PBTreeTable.this,PBTreeTable.this);
            dragSource = new DragSource();
            dragSource.createDefaultDragGestureRecognizer(PBTreeTable.this,
                                   DnDConstants.ACTION_COPY_OR_MOVE,
                                   PBTreeTable.this
                                   );

	    for(int i=0; i<getColumnCount(); i++) {
		Class type = model.getColumnClass(i);
		if(type.getName().indexOf("Boolean") != -1) {
		    getColumnModel().getColumn(i).setCellRenderer(new MyTableCellBooleanRenderer());
		} else if(type.getName().indexOf("String") != -1) {
		    getColumnModel().getColumn(i).setCellRenderer(new MyTableCellRenderer());
		    MyTableCellEditor edt = new MyTableCellEditor(new JTextField());
                    edt.setClickCountToStart(1);
                    getColumnModel().getColumn(i).setCellEditor(edt);
	        }
	    }

	    addFocusListener(new FocusAdapter() {

		public void focusGained(FocusEvent evt) {

	           if(!keyText.equals("Down") && !keyText.equals("Up")) return;

		   if(editingRow != -1 && editingCol != -1) {
			editCellAt(editingRow, editingCol);
			setRowSelectionInterval(editingRow,editingRow);
			transferFocus();
		   }
		   keyText = "";
		}
	    });


	    addKeyListener(new KeyAdapter() {
        	public void keyPressed(KeyEvent e) {
	          int row = getSelectedRow();
	          if(row == -1) return;

	          String key = e.getKeyText(e.getKeyCode());
	          if(key.equals("Down") && row+1 <getRowCount()) {
		      setRowSelectionInterval(row+1,row+1);
	          } else if(key.equals("Up") && row > 0) {
		      setRowSelectionInterval(row-1,row-1);
	          }
                }
	    });
        }

	public void stopTableCellEditing() {
	   if(!isEditing()) return;

	   DefaultCellEditor edt = (DefaultCellEditor)
		getCellEditor(getEditingRow(), getEditingColumn());
	   if(edt != null) edt.stopCellEditing();
	}

	public void processKeyPressed(KeyEvent e) {
	//Right and Left can directly set next editing field.
	//But for Up and Down, focus will be transferred from
	//the editing textfiled to the table.
	//so save keyText and next editingRow, editingCol
	//for table's focusGained.

	    editingRow = getEditingRow();
	    editingCol = getEditingColumn();

	    if(editingRow == -1 || editingCol == -1) return;

	    keyText = e.getKeyText(e.getKeyCode());
	    if(keyText.equals("Down")) {
		for(int i=editingRow+1; i<getRowCount(); i++) {
		    if(isCellEditable(i,editingCol)) {
			editingRow = i;
			//System.out.println("down "+editingRow+" "+editingCol);
			return;
		    }
		}
	    } else if(keyText.equals("Up")) {
		for(int i=editingRow-1; i>=0; i--) {
		    if(isCellEditable(i,editingCol)) {
			editingRow = i;
			//System.out.println("up "+editingRow+" "+editingCol);
			return;
		    }
		}
	    } else if(keyText.equals("Right")) {
		for(int j=editingCol+1; j<getColumnCount(); j++) {
		    if(model.getColumnClass(j).getName().indexOf("String")
		       != -1 && isCellEditable(editingRow,j)) {
		       editingCol = j;
			editCellAt(editingRow, editingCol);
			transferFocus();
			return;
		    }
		}
	    } else if(keyText.equals("Left")) {
	    	if(editingRow != -1 && editingCol != -1)
		for(int j=editingCol-1; j>=0; j--) {
		    if(model.getColumnClass(j).getName().indexOf("String")
		       != -1 && isCellEditable(editingRow,j)) {
		       editingCol = j;
			editCellAt(editingRow, editingCol);
			transferFocus();
			return;
		    }
		}
	    }
 	}

// overwrite to do nothing, so selection won't change when dragging.

        public void changeSelection(int rowIndex,
                            int columnIndex,
                            boolean toggle,
                            boolean extend) { }

        public void paint(Graphics g) {
	    int w = this.getParent().getParent().getWidth();
	    //System.out.println("paint " + w);
            if(adjustWidth && w != prevW) {
	      //initColumnSizes(w);
	      prevW = w;
	    }
            super.paint(g);
        }

        public boolean isCellEditable(int row, int col) {

	    if(col >= getColumnCount() ||
		row >= getRowCount()) return false;

	    String header = model.getColumnName(col);
            String ctype = model.getColumnClass(col).getName();
	    TreePath path = getTree().getPathForRow(row);
	    if(path == null) return false;

	    if(ctype.indexOf("TreeTable") != -1) {
                return true;
            } else {
	        VElement obj=(VElement)path.getLastPathComponent();
	        //if(obj.getAttribute(ATTR_TYPE).equals(SCAN) &&
	        if(ctype.indexOf("Boolean") != -1) {
		   // IMG check boxes are uneditable
		   //if(obj.getAttribute(ATTR_TYPE).equals(IMG)) return false;
		   // STUDY and IMG check boxes are uneditable
		   if(obj.getAttribute(ATTR_TYPE).equals(STUDY) ||
			obj.getAttribute(ATTR_TYPE).equals(IMG)) return false;
		   else return true;
		} else if(obj.getAttribute(ATTR_TYPE).equals(SCAN) &&
                  (header.equals(ATTR_SLICES) ||
                   header.equals(ATTR_ECHOES) ||
                   header.equals(ATTR_ARRAY) ||
                   header.equals(ATTR_FRAMES) ||
                   header.equals(ATTR_IMAGES))) {
                   return true;

                } else return false;
	    }
	}

        public TreePath getElementPath(int x, int y) {
            Point p=new Point(x,y);
            TreePath path=getTree().getClosestPathForLocation(p.x,p.y);
            Rectangle rect=getTree().getPathBounds(path);
            if (rect == null)
                return null;
            int y1=(int)rect.getY();
            int y2=(int)(y1+rect.getHeight());
            if (p.y>=y1 && p.y<=y2)
                return path;
            return null;
        }

        // MouseListener interface

        public void mouseClicked(MouseEvent e) {
            int clicks = e.getClickCount();
            int modifier = e.getModifiers();
	    if ((modifier & InputEvent.BUTTON1_MASK) != 0){
            if (clicks >= 2){
		Point p = new Point(e.getX(), e.getY());
		int row = rowAtPoint(p);
		int col = columnAtPoint(p);
	        String name = model.getColumnName(col);
		if(!name.equals(ATTR_NAME) && isCellEditable(row,col)) return;
                TreePath tpath=getElementPath(e.getX(), e.getY());
                if (tpath !=null) {
		    VElement elem=(VElement)tpath.getLastPathComponent();
		    if(elem.getAttribute(ATTR_TYPE).equals(STUDY)) return;
                    if ((modifier & InputEvent.SHIFT_MASK) != 0){
                        doDoubleClick(elem, "shiftdoubleclick");
                    } else if ((modifier & InputEvent.CTRL_MASK) != 0){
                        doDoubleClick(elem, "ctrldoubleclick");
                    } else if ((modifier & InputEvent.ALT_MASK) != 0){
                        doDoubleClick(elem, "altdoubleclick");
		    } else if(name.equals(ATTR_GROUP)) {
                        doDoubleClick(elem, "doubleclickgroup");
		    } else {
                        doDoubleClick(elem, "doubleclick");
		    }
		    if(elem.getAttribute(ATTR_TYPE).equals(IMG)) 
	              selectedPath=getFilePath(elem);
/*
		    if(elem.getAttribute(ATTR_TYPE).equals(SCAN)) {
		       uncheckdisplay("-1");
		       checkdisplay(getGroupID(elem));
		    } 
*/
                }
            } 
            }
        }

        public void mouseReleased(MouseEvent e){

	    Point p = new Point(e.getX(), e.getY());

	    TreePath path=getElementPath(e.getX(), e.getY());
	    if(path == null) return;
            VElement obj=(VElement)path.getLastPathComponent();

            boolean treeExp = obj.getAttribute(ATTR_EXPAND).equals("yes");
	    boolean b = treeTable.getTree().isExpanded(path);
            if(b != treeExp && b) {
                obj.setAttribute(ATTR_EXPAND,"yes");
		doSetValue(obj, ATTR_EXPAND, "yes");
	    } else if(b != treeExp) {
                obj.setAttribute(ATTR_EXPAND,"no");
		doSetValue(obj, ATTR_EXPAND, "no");
	    }
        }
        public void mouseEntered(MouseEvent e){
	}

        public void mouseExited(MouseEvent e){
        }
        public void mousePressed(MouseEvent e){
/*
	path bonds didn't update without this.
*/
	    getTree().updateUI();
            TreePath path=getElementPath(e.getX(), e.getY());
            if (path == null)
                setSelected(null);
            else {
                getTree().setSelectionPath(path);
                VElement sel=(VElement)path.getLastPathComponent();
                setSelected(sel);
            }
        }

        // DragGestureListener interface

        public void dragGestureRecognized(DragGestureEvent evt) {
            Point p=evt.getDragOrigin();
            int col = columnAtPoint(p);
            int row = rowAtPoint(p);
	//System.out.println("getDragAction "+" "+row+" "+col);
	    String name = model.getColumnClass(col).getName();
	    if(name.indexOf("TreeTable") == -1
		&& isCellEditable(row,col)) return;
            if (getSelected()==null)
                return;
            int action=evt.getDragAction();
            VElement elem=null;
            elem=getSelected();
            if (elem==null)
                return;

            LocalRefSelection ref = new LocalRefSelection(elem);
            dragSource.startDrag(evt, null, ref, PBTreeTable.this);
        }

        // DragSourceListener interface

        public void dragDropEnd (DragSourceDropEvent evt)       {
	 }

        public void dragEnter (DragSourceDragEvent evt)         {
	   m_modifier = evt.getGestureModifiers();
            //debugDnD("dragEnter");
        }
        public void dragExit (DragSourceEvent evt)              {
            //debugDnD("dragExit");
        }
        public void dragOver (DragSourceDragEvent evt)          { }
        public void dropActionChanged (DragSourceDragEvent evt) { }

        // DropTargetListener interface

        public void dragEnter(DropTargetDragEvent e) {
            debugDnD("dragEnter");
            // can add code here to reject drop
        }
        public void dragExit(DropTargetEvent e) {
            debugDnD("dragExit");
        }
        public void dragOver(DropTargetDragEvent e) {}
        public void dropActionChanged (DropTargetDragEvent e) {}
        public void drop(DropTargetDropEvent e) {
    	     processDrop(e, this);
        }

	public Color getColumnColor(int column) {
	    return model.getColumnColor(column);
   	}

	public Font getColumnFont(int column) {
	    return model.getColumnFont(column);
   	}

        class MyTableCellEditor extends DefaultCellEditor {
           public MyTableCellEditor(JTextField t) {
	    super(t);

	    getComponent().addKeyListener(new KeyAdapter() {
        	public void keyPressed(KeyEvent e) {
	   	   processKeyPressed(e);
                }
	    });
	   }
	}

    class MyTableCellBooleanRenderer extends JCheckBox implements TableCellRenderer
    {

	MyTableCellRenderer textRenderer = new MyTableCellRenderer();
	
        public MyTableCellBooleanRenderer() {
            super();
            setHorizontalAlignment(JLabel.CENTER);
        }

        public Component getTableCellRendererComponent(JTable table, Object value,
                boolean isSelected, boolean hasFocus, int row, int column) {
            if (!isCellEditable(row, column)) {
		return textRenderer.getTableCellRendererComponent(table, "", 
		isSelected, hasFocus, row, column);
            } else if (isSelected) {
                setForeground(table.getSelectionForeground());
                super.setBackground(table.getSelectionBackground());
            }
            else {
                //setForeground(table.getForeground());
                setForeground(((PBTreeTable)table).getColumnColor(column));
                setBackground(table.getBackground());
            }
            setSelected((value != null && ((Boolean)value).booleanValue()));
            return this;
        }
    }

        class MyTableCellRenderer extends DefaultTableCellRenderer {

           public MyTableCellRenderer() {
               super();
           }

           public Component getTableCellRendererComponent(JTable table, Object value,
                          boolean isSelected, boolean hasFocus, int row, int column) {

              if (table != null) {
                //setForeground(table.getForeground());
		setForeground(((PBTreeTable)table).getColumnColor(column));

	 	String str = model.getColumnClass(column).getName();
		boolean b = isCellEditable(row, column);
                if(b && str.indexOf("String") != -1) {
		    setBackground(Color.white);
		    setBorder( UIManager.getBorder("Table.focusCellHighlightBorder") );
		} else { 
		    setBackground(table.getBackground());
		    setBorder(noFocusBorder);
		}
                //setFont(table.getFont());
		setFont(((PBTreeTable)table).getColumnFont(column));

		if(isSelected) {
                    setBackground(Global.HIGHLIGHTCOLOR);
                }
	        String header = model.getColumnName(column);
                if(header.equals(ATTR_GROUP)) {
	           TreePath path = getTree().getPathForRow(row);
	           if(path != null) {
	             VElement obj=(VElement)path.getLastPathComponent();
                     String gpath = getFilePath(obj);
		     if(gpath.equals(rtGroupPath)) setForeground(Color.cyan);
                   }
	        }
              }

              setValue(value);
              //setHorizontalAlignment(JTextField.CENTER);
              return this;
           }

        } // class MyTableCellRenderer

    } // class PBTreeTable

    public void processDrop(DropTargetDropEvent e, JTreeTable tree) {

            try {
                Transferable tr = e.getTransferable();
                if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)
                    || tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                    Point p = e.getLocation();
                    TreePath path=((PBTreeTable)tree).getElementPath(p.x,p.y);
                    Object obj;

                    // Catch drag of String (probably from JFileChooser)
                    // Create a ShufflerItem and continue with code below this.
                    if(tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                        obj = tr.getTransferData(DataFlavor.stringFlavor);

                        // The object, being a String, is the fullpath of the
                        // dragged item.
                        String fullpath = (String)obj;
                        File file = new File(fullpath);
                        if(!file.exists()) {
                            Messages.postError("File not found " + fullpath);
                            return;
                        }
                        // This is not a locator recognized objType.
                        String objType = FillDBManager.getType(fullpath);
                        if(objType.equals("?")) {
                          if(file.isDirectory()) { 
                            // allow to drop any directory.
			    if(queue != null) {
			      queue.doLoadData(fullpath,"");
			    }
                            e.getDropTargetContext().dropComplete(true);
                            return;
			  } else {
                            Messages.postError("Unrecognized drop item " + 
                                               fullpath);
                            return;
			  }
                        }

                        // This assumes that the browser is the only place that
                        // a string is dragged from.  If more places come into
                        // being, we will have to figure out what to do
                        ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");
                        // Replace the obj with the ShufflerItem, and continue
                        // below.
                        obj = item;
                    }
                    // Not string, get the dragged object
                    else {
                        obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                    }

                    VElement dst=null;
                    String key ="";

                    if (path != null) {
                        dst=(VElement)path.getLastPathComponent();
                        key=getKey(dst);
                    }
	            // Allow a single ShufflerItem, or an ArrayList of ShufflerItem's
	            // If it is a single ShufflerItem, put it into an ArrayList of one
	            // for continuing with the code
	            ArrayList<ShufflerItem> newObj = new ArrayList<ShufflerItem>();
	            if (obj instanceof ShufflerItem) {
	                newObj.add((ShufflerItem)obj);
	                obj = newObj;
	            }

	            if(obj instanceof ArrayList<?>) {
	                ArrayList<?> list = (ArrayList<?>)obj;
	                // We currently only allow an ArrayList of ShufflerItems
	                // If anything else, bail out
	                if(!(list.get(0) instanceof ShufflerItem)) {
	                    //e.rejectDrop();
	                    e.getDropTargetContext().dropComplete(true);
	                    return;
	                }

	                // Go through the list and do the following operation
	                // for each item in the list
	                for(int i=0; i<list.size(); i++) {
	                    ShufflerItem item = (ShufflerItem)list.get(i);

	                    if (item.objType.equals(Shuf.DB_TRASH)) {
	                        if (queue != null) {
	                            item.actOnThisItem("ReviewQueue", "DragNDrop", "");
	                        }
	                        e.getDropTargetContext().dropComplete(true);
	                        return;
	                    } else {

	                        String fullpath = fixPath(item.getFullpath());

	                        //if(queue != null && getElementWithPath(fullpath) == null) {
	                        if (queue != null) {
	                            queue.doLoadData(fullpath, getKey(dst));
	                        }
	                    }
	                }
	                e.getDropTargetContext().dropComplete(true);
	                return;

	            }
		    else if (obj instanceof VFileElement) {

                        VElement src=(VTreeNodeElement)obj;
                        int action=e.getDropAction();
                        Document sdoc=src.getOwnerDocument();
                        if (sdoc != doc) {
			    return;
                        }
                        if(action==DnDConstants.ACTION_MOVE){
			    
                            if (queue != null) {
                                queue.doMoveNode(getKey(src), getKey(dst));
			    }
                            e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                            e.getDropTargetContext().dropComplete(true);
                            return;
                        }
                        else if(action==DnDConstants.ACTION_COPY){
                            if (queue != null) {
                                queue.doCopyNode(getKey(src), getKey(dst));
                            }
                            e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                            e.getDropTargetContext().dropComplete(true);
                            return;
                        }
                    }
                    else if (obj instanceof VTreeNodeElement) {

			VTreeNodeElement elem=(VTreeNodeElement)obj;
			if(elem.getAttribute(ProtocolBuilder.ATTR_DATA).length() == 0)
			return;

			String spath = elem.getAttribute(ProtocolBuilder.ATTR_DATA);
			if(!spath.startsWith("/")) {
			   spath =  ((ProtocolBuilder)elem.getTemplate()).getStudyPath() + "/" +spath;
			}			
			spath = fixPath(spath);
			    
			//if(queue != null && getElementWithPath(spath) == null) {
			if(queue != null) {
			    queue.doLoadData(spath,getKey(dst));
			}
                        e.getDropTargetContext().dropComplete(true);
                        return;
		    }
                }
            } catch (IOException io) {
                io.printStackTrace();
            } catch (UnsupportedFlavorException ufe) {
                ufe.printStackTrace();
            }
            e.rejectDrop();
    }

    private class ProtocolRenderer extends DefaultTreeCellRenderer
    {
        Font oldfont=null;
        Font font=null;
        int iwidth=0;

        public ProtocolRenderer() {}
        public Component getTreeCellRendererComponent(
                            JTree tree,
                            Object value,
                            boolean sel,
                            boolean expanded,
                            boolean leaf,
                            int row,
                            boolean hasFocus) {

             super.getTreeCellRendererComponent(
                            tree, value, sel,
                            expanded, leaf, row,
                            hasFocus);

            boolean active_flag=false;
            VElement obj=(VElement)value;

            String type=obj.getAttribute(ATTR_TYPE);
            String group=obj.getAttribute(ATTR_GROUP);
            String title=obj.getAttribute(ATTR_NAME).trim();
            String var="PlainText";
            if (obj instanceof VFileElement && type.equals(STUDY))
                var="Parent";
	    else if (obj instanceof VFileElement && type.equals(SCAN))
                var="Group";
	    else if (obj instanceof VFileElement && type.equals(IMG))
                var="Children";
	    else
                var="Children";
            
            String path = getFilePath(obj);
            Color color;
	    if(path.equals(selectedPath)) color=Global.HIGHLIGHTCOLOR;
            else color=DisplayOptions.getColor(var);

            oldfont=getFont();
            font=DisplayOptions.getFont(var,var,var);
            if(oldfont != null && !oldfont.equals(font)){
                setFont(font);
                oldfont=font;
            }

            if(active_flag){
                setOpaque(true);
                setBackground(color);
            }
            else{
                setOpaque(false);
                setForeground(color);
            }

	    group = Util.getLabelString(group);
	
	    StringTokenizer tok = new StringTokenizer(title, " \t\n");
            title = "";
            while(tok.hasMoreTokens())
                title += Util.getLabelString(tok.nextToken().trim())+" ";

            String ttext=obj.getAttribute(ATTR_TOOLTEXT);
            if(ttext==null || ttext.length()==0){
                ttext=obj.getNodeName()+" "+title+" "+obj.getAttribute(ATTR_ID);
                ttext=ttext.trim();
                if(group.length()>0)
                        ttext+=" ["+group+"]";
            } else {
		tok = new StringTokenizer(ttext, " \t\n");
                ttext = "";
                while(tok.hasMoreTokens())
                    ttext += Util.getLabelString(tok.nextToken().trim())+" ";
	    }
            if(!ttext.equals("null"))
                setToolTipText(ttext);
            String oldtitle=getText();
            title=title.trim();
            if(!title.equals(oldtitle)){
                setText(title);
            }
            iwidth=0;
            return this;
        }
        public Dimension getPreferredSize(){
            String text=getText();
            if(text==null || text.length()==0)
                return super.getPreferredSize();
            text=text.trim();
            FontMetrics metrics=getFontMetrics(font);
            int w=metrics.stringWidth(text);
            int h=metrics.getHeight();

            // cheap solution for wrong metrics.stringWidth() problem:
            // use twice the width returned. Downside is that sometimes
            // unnecessary scroll bars are drawn

            Dimension dim=new Dimension(2*w+iwidth,h);
            return dim;
        }

    } // class ProtocolRenderer

    public static String fixImgstr(String imgs, int size) {
        int step = 1;
        int n = imgs.indexOf("-:");
        if(n != -1) {
           try {
             step = Integer.valueOf(imgs.substring(n+2)).intValue();
           } catch(NumberFormatException e) {}
           imgs = imgs.substring(0,n+1);
        }
        if(step <= 0) step = 1;

        if(imgs.endsWith("-")) imgs += size;
	else if(imgs.indexOf("all") != -1) imgs = "1-"+size;
	if(step > 1) imgs += ":"+step;

	return imgs;
    }

    public static String fixFrmstr(String frms, int[] ilst) {
        int step = 1;
        int n = frms.indexOf("-:");
        if(n != -1) {
           try {
             step = Integer.valueOf(frms.substring(n+2)).intValue();
           } catch(NumberFormatException e) {}
           frms = frms.substring(0,n+1);
        }
        if(step <= 0) step = 1;

        if(frms.endsWith("-")) {
	   int fst;
           try {
	      fst = Integer.valueOf(frms.substring(0,frms.length()-1)).intValue();
           } catch(NumberFormatException e) {fst = 1;}
	   frms += fst - ilst[0] + ilst.length;
	} else if(frms.indexOf("all") != -1) frms = "1-"+ilst.length;
	if(step > 1) frms += ":"+step;

	return frms;
    }

    public boolean checkID(String gid) {
	if(gid.startsWith("g") || gid.startsWith("G")
	  || gid.equals("-1")) return true;
	else return false;
    }

    public void dropToVnmrXCanvas(VElement elem, Point p, String mod) {
        if(queue == null) return;

	selectedNode = elem;
	String key = getKey(elem);
	
        if(mod.length()>0) queue.doModDnd(key, p.x, p.y,mod);
	else queue.doRQdnd(key, p.x, p.y);
    }

    public String fixPath(String path) {
	File f = new File(path);
	if(!f.exists() && !path.endsWith(".fid") && !path.endsWith(".img")) { 
	    return path + ".img";
        } else if(path.endsWith(".fid")) { 
	    return path.substring(0,path.length()-4) +".img";
	} else return path;
    }

/*
    public void dropToVnmrXCanvas(String path) {
        if(queue == null) return;

	//path = fixPath(path);

	queue.doLoadData(path, "");
    }
*/

    public String getMod() {
	if ((m_modifier & InputEvent.CTRL_MASK) != 0 ) return "ctrl";
        else if ((m_modifier & InputEvent.SHIFT_MASK) != 0 ) return "shift";
	else return "";
    }

    public void dropToVnmrXCanvas(String path, Point p, String mod) {
        if(queue == null) return;

	//path = fixPath(path);

        if(mod.length()>0) queue.doModDnd(path, p.x, p.y,mod);
	else queue.doDnd(path, p.x, p.y);
    }

    public void doSetValue(VElement elem, String name, String value) {
        if(queue == null) return;
	selectedRow = treeTable.getSelectedRow();
	queue.doSetValue(getKey(elem), name, value);
    }

    public String getFilePath(VElement elem) {
        if(elem == null || elem == root) return "";

	return elem.getAttribute(ATTR_DIR) +"/"+
		  elem.getAttribute(ATTR_NAME) +
		  elem.getAttribute(ATTR_EXT);
    }

    public String getKey(VElement elem) {
        if(elem == null || elem == root) return "";

	return elem.getAttribute(ATTR_DIR) +" "+
		  elem.getAttribute(ATTR_NAME) +
		  elem.getAttribute(ATTR_EXT) +
		  elem.getAttribute(ATTR_COPIES);
	
    }

    public boolean removeElement(VElement elem){
        if(elem==null) return false;

        if(queue != null) queue.doRemoveNode(getKey(elem));

        return true;
    }

    public void requestRemoveImg(Point p, int but) {
        if(queue != null) queue.doRemoveImg(p.x, p.y, but);
    }

    public void uncheckdisplay(String gid) {
        if(rootElement() == null) return;

        int dcol = -1;
        for(int i=0; i<treeTable.getColumnCount(); i++) {
           if(treeTable.getColumnName(i).equals(ATTR_DISPLAY))
           dcol = i;
        }

        if(dcol == -1) return;

        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getNext();
        VElement elem=null;
        while(node!=null){
            elem=(VElement)node;
            if(gid.equals("-1") &&
                elem.getAttribute(ATTR_DISPLAY).equals("yes")) {
                treemodel.setValueAt(new Boolean(false), elem, dcol);
            } else if(gid.equals(getGroupID(elem)) &&
                elem.getAttribute(ATTR_DISPLAY).equals("yes")) {
                treemodel.setValueAt(new Boolean(false), elem, dcol);
                break;
            }
            node=treewalker.getNext();
        }

        treeTable.validate();
        treeTable.repaint();
    }

    public void checkdisplay(String gid) {

        if(rootElement() == null) return;

        int dcol = -1;
        for(int i=0; i<treeTable.getColumnCount(); i++) {
           if(treeTable.getColumnName(i).equals(ATTR_DISPLAY))
           dcol = i;
        }

        if(dcol == -1) return;

        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getNext();
        VElement elem=null;
        while(node!=null){
            elem=(VElement)node;
            if(gid.equals("-1") &&
                elem.getAttribute(ATTR_DISPLAY).equals("no")) {
                treemodel.setValueAt(new Boolean(true), elem, dcol);
            } else if(gid.equals(getGroupID(elem)) &&
                elem.getAttribute(ATTR_DISPLAY).equals("no")) {
                treemodel.setValueAt(new Boolean(true), elem, dcol);
                break;
            }
            node=treewalker.getNext();
        }

        treeTable.validate();
        treeTable.repaint();
    }

    public void drop2frame(String path, String frms) {
       VElement elem = getElementWithPath(path);
	if(elem == null) return;
	
	int fcol = -1;
        for(int i=0; i<treeTable.getColumnCount(); i++) {
           if(treeTable.getColumnName(i).equals(ATTR_FRAMES))
           fcol = i;
        }

        if(fcol == -1) return;

	treemodel.setValueAt(frms, elem, fcol);
        treeTable.validate();
        treeTable.repaint();
    } 

    public String getGroupID(VElement elem) {
        String group = elem.getAttribute(ATTR_GROUP);
        if(group.indexOf("(") != -1)
           return group.substring(0,group.indexOf("("));
        else {
           group = elem.getAttribute(ATTR_GROUP);
           if(group.indexOf("(") != -1)
              return group.substring(0,group.indexOf("("));
           else return "";
        }
    }

}

