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
import java.util.*;
import java.util.List;
import java.io.*;
import java.beans.*;
import javax.swing.*;
import javax.swing.event.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.util.DisplayOptions.DisplayStyle;

import javax.swing.plaf.basic.BasicTreeUI;
import javax.swing.tree.*;
import org.w3c.dom.*;

import static javax.swing.tree.TreeSelectionModel.*;

//import javax.swing.event.TreeSelectionEvent;
//import javax.swing.event.TreeSelectionListener;

/**
 * @author      Dean Sindorf
 */
public class ProtocolBuilder extends Template
     implements PropertyChangeListener {

    /** Right margin of Action nodes in the SQ viewport. */
    private static final int TREE_MARGIN = 2;

    PBTree              tree=null;
    ProtocolRenderer    renderer;
    DefaultTreeModel    treemodel=null;
    protected ProtocolUpdate protocolupdate;
    VElement            root=null;
    boolean             executing=false;
    boolean             paused=false;
    private String      panelMode = "";
    StudyQueue          queue=null;
    ProtocolEditor      editor=null;
    static boolean      showtimes=true;
    static boolean      show_icons=true;  // as per applab un-request
    static boolean      expand_new=true;
    String              selected_id="0";
    int                 node_id=0;
    int                 insert_mode;
    boolean             allow_nesting=true;
    boolean             validate_move=false;
    boolean             validate_copy=false;
    Hashtable           excluded=null;
    String				studyPath="";
    protected ArrayList aListProtocol = new ArrayList();
//    private static LayoutManager layout = null;
    //private SQActionRenderer m_executingAction = new SQActionRenderer();
    //protected Rectangle m_executingNodeRect = new Rectangle();
    private int m_count = 0; /*TMP*/
    public boolean m_isExecuting = false;
    private JPanel selectpanel;

    /**
     * How much each tree sub-node is indented from the previous level.
     * The top level is indented this amount.
     * Set the desired indent here, but set for real when the
     * tree is instantiated.
     */
    private int m_treeIndent = 10;

    private boolean m_treeDirty = false;

    static private SQActionRenderer m_actionRenderer = new SQActionRenderer();

    //static public final String ATTR_ACTION_TITLE     = "actiontitle";
    //static public final String ATTR_ACTION_TYPE      = "actiontype";
    static public final String ATTR_DATA      = "data";
    static public final String ATTR_ELEMENT   = "element";
    static public final String ATTR_EXP       = "exp";
    static public final String ATTR_EXPANDED  = "expanded";
    static public final String ATTR_SQUISHED  = "squished";
    static public final String ATTR_SQUISH_COUNT = "_squishcount";
    static public final String ATTR_ID        = "id";
    static public final String ATTR_LOCK      = "lock";
    static public final String ATTR_MACRO     = "macro";
    static public final String ATTR_NAME      = "name";
    static public final String ATTR_PROGRESS  = "progress";
    static public final String ATTR_PSLABEL   = "pslabel";
    static public final String ATTR_QUEUE     = "queue";
    static public final String ATTR_STATUS    = "status";
    static public final String ATTR_NIGHT     = "night";
    static public final String ATTR_TIME      = "time";
    static public final String ATTR_TIMEDAY   = "timeday";
    static public final String ATTR_TIMENIGHT = "timenight";
    static public final String ATTR_TITLE     = "title";
    static public final String ATTR_TOOLTEXT  = "tooltext";
    static public final String ATTR_TYPE      = "type";
    static public final String ATTR_WHEN      = "when";
    static public final String ATTR_STYLE     = "style";
    static public final String ATTR_FONTNAME  = "fontname";
    static public final String ATTR_FONTSTYLE = "fontstyle";
    static public final String ATTR_FONTSIZE  = "fontsize";
    static public final String ATTR_FONTCOLOR = "fontcolor";
    static public final String ATTR_BGCOLOR   = "bgcolor";

    static public final String REQ            = "REQ";

    static public final int MAX_SQUISH = 4; // The number of squished nodes to display
    static public final String SQ_READY          = "Ready";
    static public final String SQ_EXECUTING      = "Executing";
    static public final String SQ_SKIPPED        = "Skipped";
    static public final String SQ_COMPLETED      = "Completed";
    static public final String SQ_QUEUED         = "Queued";
    static public final String SQ_ERROR          = "Error";
    static public final String SQ_ACTIVE         = "Active";
    static public final String SQ_CUSTOMIZED     = "Customized";

    static public final int ACTIONS     = 1;
    static public final int PROTOCOLS   = 2;
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
    private int m_modifier=0;

    public String getMod() {
        if ((m_modifier & InputEvent.CTRL_MASK) != 0 ) return "ctrl";
        else if ((m_modifier & InputEvent.SHIFT_MASK) != 0 ) return "shift";
        else return "";
    }

    //----------------------------------------------------------------
    /** Constructor. */
    //----------------------------------------------------------------
    public ProtocolBuilder(Object obj)
    {
        super();
        if(obj instanceof StudyQueue)
            queue=(StudyQueue)obj;
        else if(obj instanceof ProtocolEditor)
            editor=(ProtocolEditor)obj;

        renderer = new ProtocolRenderer();
        renderer.setLeafIcon(null);
        renderer.setClosedIcon(null);
        renderer.setOpenIcon(null);
        Color c=Color.yellow;
        if(DisplayOptions.isOption(DisplayOptions.COLOR,"Selected"))
            c=DisplayOptions.getColor("Selected");
        renderer.setBackgroundSelectionColor(c);
        DisplayOptions.addChangeListener(this);
        excluded=new Hashtable();
        excluded.put(ATTR_TIME,ATTR_TIME);
        excluded.put(ATTR_DATA,ATTR_DATA);
        excluded.put(ATTR_STATUS,ATTR_STATUS);
        excluded.put(ATTR_LOCK,ATTR_LOCK);
    }

    private void debugDnD(String msg){
        if(DebugOutput.isSetFor("dnd"))
            Messages.postDebug(new StringBuffer().append("PE: " ).append( msg).toString());

    }

    //----------------------------------------------------------------
    /** Set default class bindings for xml keywords. */
    //----------------------------------------------------------------
    protected void setDefaultKeys(){
        setKey("protocol",  vnmr.templates.VProtocolElement.class);
        setKey("action",    vnmr.templates.VActionElement.class);
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
            if(isActionOrProtocol(elem)){
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
        if(tree != null) {
            tree.repaint();
        }
    }

    //----------------------------------------------------------------
    /** Set insert mode. */
    //----------------------------------------------------------------
    public void setInsertMode(int i){
        insert_mode=i;
    }

    //----------------------------------------------------------------
    /** Set the SQ panel mode. */
    //----------------------------------------------------------------
    public void setMode(String mode){
        panelMode = mode;
        setPanelColor();
    }

    //----------------------------------------------------------------
    /** Set executing status. */
    //----------------------------------------------------------------
    public void setExecuting(boolean f){
        executing=f;
        setPanelColor();
    }

    //----------------------------------------------------------------
    /** Set executing status. */
    //----------------------------------------------------------------
    public void setPaused(boolean f){
        paused=f;
        setPanelColor();
    }

    //----------------------------------------------------------------
    /** Set executing status. */
    //----------------------------------------------------------------
    public boolean isExecuting(){
        return executing;
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
    /** Set move validation bit. */
    //----------------------------------------------------------------
    public void setValidateMove(boolean f){
        validate_move=f;
    }

    //----------------------------------------------------------------
    /** Get move validation bit. */
    //----------------------------------------------------------------
    public boolean validateMove(){
        return validate_move;
    }

    //----------------------------------------------------------------
    /** Set copy validation bit. */
    //----------------------------------------------------------------
    public void setValidateCopy(boolean f){
        validate_copy=f;
    }

    //----------------------------------------------------------------
    /** Get copy validation bit. */
    //----------------------------------------------------------------
    public boolean validateCopy(){
        return validate_copy;
    }

    //----------------------------------------------------------------
    /** Set show times flag. */
    //----------------------------------------------------------------
    public void setTimeShown(boolean f){
        showtimes=f;
        invalidateTree();
    }

    /**
     * Merge any font style attributes that have been set on this node
     * into the normal display style.
     * The normal display style is keyed off the "type" variable.
     * If the display style, ATTR_STYLE, attribute has been set to a valid
     * value, it replaces the given style.
     * Then, any of the individual attributes ATTR_FONTNAME, ATTR_FONTSTYLE,
     * ATTR_FONTSIZE, or ATTR_FONTCOLOR that have been set are used to modify
     * the display style.
     * The resulting style is returned.
     * @param node The node that this style is for.
     * @param type The type of node this is for.
     *
     * @return The new style.
     */
    public static DisplayStyle getStyleForNode(VElement node, String type) {
        if (node == null) {
            return getStyleForNodeType(type);
        }
//        String nodeType = node instanceof VActionElement ? "act" : "pro";
//        String status = node.getAttribute(ATTR_STATUS);
        String dispStyle = node.getAttribute(ATTR_STYLE);
        String fontName = node.getAttribute(ATTR_FONTNAME);
        String fontStyle = node.getAttribute(ATTR_FONTSTYLE);
        String fontSize = node.getAttribute(ATTR_FONTSIZE);
        String fontColor = node.getAttribute(ATTR_FONTCOLOR);
//        String key = new StringBuffer(nodeType)
//                    .append(":").append(type)
//                    .append(":").append(status)
//                    .append(":").append(dispStyle)
//                    .append(":").append(fontName)
//                    .append(":").append(fontStyle)
//                    .append(":").append(fontSize)
//                    .append(":").append(fontColor).toString();
//        DisplayStyle style = DisplayOptions.getCachedStyle(key);
        DisplayStyle style = null;
        if (style == null) {
            if (dispStyle.trim().length() > 0) {
                style = DisplayOptions.getDisplayStyle(dispStyle);
            }
            if (style == null) {
                style = getStyleForNodeType(type);
            }
            Font newFont = getFontForNode(style.getFont(), fontName,
                                          fontStyle, fontSize);
            Color newColor = getFgForNode(style.getFontColor(), fontColor);
            if (!newFont.equals(style.getFont())) {
                style = style.deriveStyle(newFont);
            }
            if (!newColor.equals(style.getFontColor())) {
                style = style.deriveStyle(newColor);
            }
//            DisplayOptions.putCachedStyle(key, style);
        }
        return style;
    }

    /**
     * @param var
     * @return
     */
    private static DisplayStyle getStyleForNodeType(String var) {
        DisplayStyle style;
        Font font=DisplayOptions.getFont(var,var,var);
        Color color=DisplayOptions.getColor(var);
        style = new DisplayStyle(font, color);
        return style;
    }

    public static Font getFontForNode(Font font, String name,
                                      String style, String size) {
        if (style != null && style.trim().length() > 0) {
            int newStyle = DisplayOptions.getFontStyle(style);
            font = font.deriveFont(newStyle);
        }
        if (size != null && size.trim().length() > 0) {
            float newSize = DisplayOptions.getFontSize(size);
            font = font.deriveFont(newSize);
        }
        if (name != null && name.trim().length() > 0) {
            int oldStyle = font.getStyle();
            float oldSize = font.getSize2D();
            name = DisplayOptions.getFontName(name);
            font = new Font(name, oldStyle, (int)oldSize);
            if ((int)oldSize != oldSize) {
                font = font.deriveFont(oldSize);
            }
        }
        return font;
    }

    public static Color getFgForNode(Color color, String fg) {
        if (fg != null && fg.trim().length() > 0) {
            color = DisplayOptions.getColor(fg);
        }
        return color;
    }

    public static Color getBgForNode(Color color, VElement node) {
        String bg = node.getAttribute(ATTR_BGCOLOR);
        if (bg != null && bg.trim().length() > 0) {
            color = DisplayOptions.getColor(bg);
        }
        return color;
    }

    /** Get show times flag.
     * @return True if the time attribute should be displayed
     * in the nodes.
     */
    public static boolean isTimeShown(){
        return showtimes;
    }

    //----------------------------------------------------------------
    /** Set show locks flag. */
    //----------------------------------------------------------------
    public void setShowIcons(boolean f){
        show_icons=f;
        invalidateTree();
    }

    //----------------------------------------------------------------
    /** Get show locks flag. */
    //----------------------------------------------------------------
    public boolean getShowIcons(){
        return show_icons;
    }

    //----------------------------------------------------------------
    /** Set expand_new flag. */
    //----------------------------------------------------------------
    public void setExpandNew(boolean f){
        expand_new=f;
        if(tree !=null)
            tree.invalidate();
       // invalidateTree();
    }

    //----------------------------------------------------------------
    /** Get expand_new flag. */
    //----------------------------------------------------------------
    public boolean getExpandNew(){
        return expand_new;
    }

    //----------------------------------------------------------------
    /** Return true if tree contains only a root element. */
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
        if(tree==null)
            return;
        Color color=null;
        if ("NormalMode".equalsIgnoreCase(panelMode)) {
            color = DisplayOptions.getColor("SQNormalModeBg");
        } else if ("SubmitMode".equalsIgnoreCase(panelMode)) {
            color = DisplayOptions.getColor("SQSubmitModeBg");
        } else if(paused)
            color=DisplayOptions.getColor("PausedBackground");
        else if(executing)
            color=DisplayOptions.getColor("ActiveBackground");
        else
            color=Util.getBgColor();

        if(color==null)
            color=Global.BGCOLOR;
        tree.setBackground(color);
        // Commented out so that color of SQ selector stays constant
        //if (selectpanel != null) {
        //    selectpanel.setBackground(color);
        //}
        renderer.setBackgroundNonSelectionColor(color);
        if(DisplayOptions.isOption(DisplayOptions.COLOR,"Selected"))
            color=DisplayOptions.getColor("Selected");
        renderer.setBackgroundSelectionColor(color);
        //invalidateTree();
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void setSelected(VElement elem){
        if(elem != null){
        	String s=elem.getAttribute(ATTR_ID);
        	if(s != null && selected_id!=null && selected_id.equals(s))
        	    return;
        }
        setSelected(elem,null,CLICK);
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void setSelected(VElement elem, VElement dst, int mode){
        if(elem==null){
            selected_id="null";
            super.setSelected(null);
            tree.clearSelection();
        }
        else{
            String s=elem.getAttribute(ATTR_ID);
            if(s != null && s.length()>0){
                selected_id=s;
                if(dst==null)
                    doSingleClick(selected_id,null, mode);
                else{
                    String dstr=dst.getAttribute(ATTR_ID);
                    if(dstr.length()==0)
                    	dstr="null";
                    doSingleClick(selected_id,dstr, mode);
                }
                super.setSelected(elem);
            }
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
            return tree.isCollapsed(path);
        return true;
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void selectElement(VElement elem){
        selectElement(elem,null,CLICK);
    }

    //----------------------------------------------------------------
    /** Select an Element node. */
    //----------------------------------------------------------------
    public void selectElement(VElement elem,VElement dst,int mode){
        if(elem !=null){
            TreePath path=new TreePath(treemodel.getPathToRoot(elem));
            tree.makeVisible(path);
            tree.setSelectionPath(path);
            setSelected(elem,dst,mode);
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
        if(rootElement()==null || tree==null)
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
        expandElement(elem, true);
   }

   public void expandElement(VElement elem, boolean bElem)
   {
       if(elem==null || tree==null)
            return;

        TreePath path=new TreePath(treemodel.getPathToRoot(elem));
        tree.makeVisible(path);
        if(tree.isCollapsed(path))
            tree.expandPath(path);

        if (bElem)
            expandElement(getElementAfter(elem,PROTOCOLS));
   }

    //----------------------------------------------------------------
    /** Expand all Protocol nodes  */
    //----------------------------------------------------------------
    public void expandTree() {
        expandElement(firstProtocol());
    }

    public void setTreeExpanded(VElement elem)
    {
        if (elem == null || tree == null)
            return;

        String value = elem.getAttribute(ATTR_EXPANDED);
        /*if (value != null && (value.equals("false") || value.equals("no")))
            collapseElement(elem);*/
        if (value == null || value.equals("true"))
        {
            TreePath path = new TreePath(treemodel.getPathToRoot(elem));
            tree.makeVisible(path);
            tree.expandPath(path);
        }
        setTreeExpanded(getElementAfter(elem, PROTOCOLS));
    }

    //----------------------------------------------------------------
    /** Compress an Element node. */
    //----------------------------------------------------------------
    public void collapseElement(VElement elem){
        if(elem !=null){
            TreePath path=new TreePath(treemodel.getPathToRoot(elem));
            tree.collapsePath(path);
        }
    }

    //----------------------------------------------------------------
    /** Collapse the tree  */
    //----------------------------------------------------------------
    public void collapseTree() {
        collapseElement(firstProtocol());
    }

    //----------------------------------------------------------------
    /** Build the tree panel. */
    //----------------------------------------------------------------
    public JTree buildTreePanel(){
        tree = new PBTree(treemodel);
        //tree.addComponentListener(tree);
//        if (layout == null) {
//            layout = tree.getLayout();
//        }
        //tree.setLayout(new TreeOverlayLayout());
        protocolupdate = new ProtocolUpdate(tree);
        ToolTipManager.sharedInstance().registerComponent(tree);
        setPanelColor();
        //treemodel.nodeStructureChanged(root);
        return tree;
    }

//    //----------------------------------------------------------------
//    /** Return the tree panel. */
//    //----------------------------------------------------------------
//    public JTree getTreePanel(){
//        return tree;
//    }

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
    /** Copy an Element node. */
    //----------------------------------------------------------------
    public VElement copyElement(VElement src, VElement dst) {
        VElement elem=cloneElement(src);
        addElement(dst,elem);
        selectElement(elem,dst,COPY);
        setInsertMode(0);
        return elem;
    }

    //----------------------------------------------------------------
    /** Read an Element node from a file. */
    //----------------------------------------------------------------
    public VElement readElement(String fn, VElement dst) {
        VElement src=addElement(dst,fn);
        if(src != null)
            selectElement(src);
        setInsertMode(0);
        return src;
    }

    //================= Element Attribute Methods ===================

    //----------------------------------------------------------------
    /** Assign Ids to all Elements.  */
    //----------------------------------------------------------------
    public void assignIds() {
        if(queue==null){
           setIds();
           return;
        }
        ElementTree treewalker=new ElementTree(rootElement());

        Node node=treewalker.getNext();
        while(node!=null){
            VElement obj=(VElement)node;
            if(isActionOrProtocol(obj)){
                String id=obj.getAttribute(ATTR_ID);
                if(id == null || id.length()==0){
                    obj.setAttribute(ATTR_ID,"n"+ ++node_id);
                }
            }
            node=treewalker.getNext();
        }
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
            if(isActionOrProtocol(obj))
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

        list.add(ATTR_STATUS);
        list.add(obj.getAttribute(ATTR_STATUS));

        NamedNodeMap attrs=obj.getAttributes();
        for(int i=0;i<attrs.getLength();i++){
            Node a=attrs.item(i);
            String s=a.getNodeName();
            if(s.equals(ATTR_TYPE))   continue;
            if(s.equals(ATTR_ID))     continue;
            if(s.equals(ATTR_STATUS)) continue;
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
        //treemodel.nodeChanged(obj);
        if (s.equals(ATTR_SQUISHED)) {
            m_treeDirty = true;
        }
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
    /** return true if element is in the tree.  */
    //----------------------------------------------------------------
    public boolean inTree(VElement obj) {
        ElementTree treewalker=new ElementTree(rootElement());
        Node node=treewalker.getNext();
        while(node!=null){
            if(node instanceof VTreeNodeElement){
                 if((VElement)node==obj)
                    return true;
            }
            node=treewalker.getNext();
        }
        return false;
    }

    //----------------------------------------------------------------
    /** Get element with specified Id.  */
    //----------------------------------------------------------------
    public VElement getElement(String id) {
        if(id==null || id.equals("0") || id.equals("null")){
            return rootElement();
        }
        VElement rtn = rootElement();
        if(rtn == null)
           return rtn;
        ElementTree treewalker=new ElementTree(rtn);
        rtn = null;
        Node node=treewalker.getNext();
        while(node!=null){
            if(node instanceof VTreeNodeElement){
                VElement obj=(VElement)node;
                String attr=obj.getAttribute(ATTR_ID).trim();
                if(attr != null && attr.equals(id)) {
                    rtn = obj;
                    break;
                }
            }
            node=treewalker.getNext();
        }
        return rtn;
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

    public ArrayList<VElement> getContiguousSquishedNodes(VElement elem) {
        ArrayList<VElement> nodes = new ArrayList<VElement>();
        ArrayList<VElement> before = getElementsBefore(elem, ANY);
        for (int i = before.size() -1; i >= 0; --i) {
            VElement ielem = before.get(i);
            if ("true".equals(ielem.getAttribute(ATTR_SQUISHED))) {
                nodes.add(ielem);
            } else {
                break;
            }
        }
        ArrayList<VElement> after = getElementsAfter(elem, ANY);
        for (int i = 0; i < after.size(); i++) {
            VElement ielem = after.get(i);
            if ("true".equals(ielem.getAttribute(ATTR_SQUISHED))) {
                nodes.add(ielem);
            } else {
                break;
            }
        }
        return nodes;
    }

    //----------------------------------------------------------------
    /** Get typed elements in subtree elem. */
    //----------------------------------------------------------------
    public ArrayList<VElement> getElements(VElement elem, int type) {
        ArrayList<VElement> list=new ArrayList<VElement>();
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
    public ArrayList<VElement> getElementsBefore(VElement elem, int type) {
        ArrayList<VElement> list=new ArrayList<VElement>();
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
    public ArrayList<VElement> getElementsAfter(VElement elem, int type) {
        ArrayList<VElement> list=new ArrayList<VElement>();
        if(elem==null)
            elem=rootElement();
        if(elem==null)
            return list;
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
    /** Test if an Element is an Action or protocol. */
    //----------------------------------------------------------------
    public static boolean isActionOrProtocol (Node elem) {
        if(elem!=null && (elem instanceof VTreeNodeElement))
            return true;
        return false;
    }

    //----------------------------------------------------------------
    /** return true if Element is of the specified type.  */
    //----------------------------------------------------------------
    public boolean isType(VElement obj, int type) {
        if((type&ACTIONS)>0 && isAction(obj))
            return true;
        else if((type&PROTOCOLS)>0 && isProtocol(obj))
            return true;
        return false;
    }

    //================= [Protocol Element] ===========================

    //----------------------------------------------------------------
    /** Return the first Protocol Element. */
    //----------------------------------------------------------------
    public VElement firstProtocol(){
         return getElementAfter(rootElement(),PROTOCOLS);
    }
    //----------------------------------------------------------------
    /** Test if an Element is a Protocol. */
    //----------------------------------------------------------------
    public static boolean isProtocol (Node elem) {
        if(elem!=null && (elem instanceof VProtocolElement))
            return true;
        return false;
    }

    //=================== [Action Element] ===========================

    //----------------------------------------------------------------
    /** Return the first Action Element. */
    //----------------------------------------------------------------
    public VElement firstAction(){
         return getElementAfter(rootElement(),ACTIONS);
    }

    //----------------------------------------------------------------
    /** Test if an Element is an Action. */
    //----------------------------------------------------------------
    public static boolean isAction (Node elem) {
        if(elem!=null && (elem instanceof VActionElement))
            return true;
        return false;
    }

    //=================== StudyQueue Methods =========================

    //----------------------------------------------------------------
    /** Return true if element can be deleted. */
    //----------------------------------------------------------------
    public boolean canDelete(VElement src){
        if(queue !=null && !queue.testDelete(src))
            return false;
        return true;
    }

    //----------------------------------------------------------------
    /** Return true if element can be added here. */
    //----------------------------------------------------------------
    public boolean canAdd(VElement elem){
        if(queue !=null && !queue.testAdd(elem)){
            return false;
        }
        return true;
    }

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
    public String getStudyName(String dir){
        if(queue !=null)
            return queue.studyName(dir);
        return (new StringBuffer().append(dir).append("/study.xml").toString());

    }

    public ArrayList getHiddenNodes()
    {
        return aListProtocol;
    }

    public void setHiddenNodes(ArrayList alist)
    {
        aListProtocol = alist;
    }

    //----------------------------------------------------------------
    /** Flag single-click event. */
    //----------------------------------------------------------------
    public void doSingleClick(String src, String dst, int mode){
        if(queue!=null)
            queue.wasClicked(src,dst,mode);
    }

    //----------------------------------------------------------------
    /** Flag double-click event. */
    //----------------------------------------------------------------
    public void doDoubleClick(){
        if(queue!=null)
            queue.wasDoubleClicked(selected_id);
    }

    //----------------------------------------------------------------
    /** Flag tree modification event. */
    //----------------------------------------------------------------
    public void setDragAndDrop(){
        if(queue!=null) {
            queue.setDragAndDrop(selected_id, getMod());
	}
    }

    //=================== Tree Modification Methods ==================

    //----------------------------------------------------------------
    /** Open a new Protocol XML file. */
    //----------------------------------------------------------------
    public void newTree(){
        clearTree();
        newDocument();
        root=rootElement();
        if(treemodel==null)
            treemodel=new ProtocolModel(root);
        else
            treemodel.setRoot(root);
        if(editor!=null)
            editor.setTitle(rootElement());
    }

    //----------------------------------------------------------------
    /** Open a new Protocol XML file. */
    //----------------------------------------------------------------
    public void newTree(String path){

        clearTree();

	// strip of "/study.xml"
	studyPath = path.substring(0,path.length()-10);

        try {
            open(path);
        }
        catch(Exception e) {
           Messages.postError(new StringBuffer().append("could not open study ").
                                append(path).toString());

           return;
        }
        root=rootElement();
        if(treemodel==null)
            treemodel=new ProtocolModel(root);
        else
            treemodel.setRoot(root);
        if(queue==null)
            setIds();
        if(editor!=null)
            editor.setTitle(root);
        if(expand_new)
           setTreeExpanded(firstProtocol());
    }

    public String getStudyPath(){
	return studyPath;
    }

    //----------------------------------------------------------------
    /** Clear all tree items. */
    //----------------------------------------------------------------
    public void clearTree(){
        Messages.postDebug("PBuilder", "+++***+++ clearTree");
        if(root !=null)
            removeChildren(root);
        if(editor!=null)
            editor.setTitle(rootElement());

        studyPath = "";
        aListProtocol.clear();
        if (protocolupdate != null)
            protocolupdate.clearPath();
    }

    //----------------------------------------------------------------
    /** Remove all child nodes. */
    //----------------------------------------------------------------
    public void removeChildren(VElement parent){
        if(parent==null)
            return;
        int nSize = treemodel.getChildCount(parent);
        MutableTreeNode[] nodes = new MutableTreeNode[nSize];
        for (int i = 0; i < nSize; i++) {
            MutableTreeNode node=(MutableTreeNode)treemodel.getChild(parent, i);
            nodes[i] = node;
        }
        for (int i = 0; i < nSize; i++)
        {
            MutableTreeNode node = nodes[i];
            parent.removeChild((Node)node);
        }

        //parent.removeChildren();
        ((ProtocolModel)treemodel).showElement();
     }

    //----------------------------------------------------------------
    /** Get a child Element. */
    //----------------------------------------------------------------
    public VElement getElement(VElement parent, int index){
        return (VElement)treemodel.getChild(parent,index);
    }

    //----------------------------------------------------------------
    /** Add a child Element. */
    //----------------------------------------------------------------
    public VElement addElement(VElement target, VElement elem)
    {
        return addElement(target, elem, false);
    }

    public VElement addElement(VElement target, VElement elem, boolean bignorelock){
        if(elem==null){
            Messages.postWarning("ProtocolBuilder ERROR could not add null element");
            return null;
        }
        if (target==null) {
            target=root;
        }
        int index=0;
        VElement parent=target;

        // source == action

        if(isAction(elem)){
            if(target==root)
                target=lastElement();

            // target == action

            if(isAction(target)){
                parent=(VElement)target.getParent();
                index=treemodel.getIndexOfChild(parent,target)+1;
            }
            if(!canAdd(target) && !bignorelock)
                return null;
            treemodel.insertNodeInto(elem, parent, index);
        }

        // source == protocol

        else{
            if(isAction(target))
                parent=(VElement)target.getParent();

            boolean append=parent.hasChildNodes() && isCollapsed(parent);
            if(parent==root)
                append=false;
            else if(!allow_nesting || insert_mode==AFTER)
                append=true;
            else if(insert_mode==INTO)
                append=false;

            if(append){
                VElement sibling=parent;
                parent=(VElement)parent.getParent();
                index=parent.getIndex(sibling)+1;
            }
            else if(parent==root)
                index=treemodel.getChildCount(root);
            else if(isAction(target))
                index=parent.getIndex(target)+1;
            else
                index=0;
            if(target !=root && !canAdd(target) && !bignorelock)
                return null;
            treemodel.insertNodeInto(elem, parent, index);
        }

        if(tree==null)
            return null;
        //tree.scrollPathToVisible(new TreePath(treemodel.getPathToRoot(elem)));
        assignIds();
        return elem;
    }

    //----------------------------------------------------------------
    /** Build a new Element by parsing a file. */
    //----------------------------------------------------------------
    public VElement buildElement(String path){
        ProtocolBuilder builder=new ProtocolBuilder(null);
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

    //----------------------------------------------------------------
    /** Add an Element parsed from an XML file. */
    //----------------------------------------------------------------
    public VElement addElement(VElement parent, String file){
        boolean empty=isEmpty();
        String path=FileUtil.openPath(file);
        VElement item=buildElement(path);
        VElement itemroot=item;
        if(item==null){
            Messages.postWarning(new StringBuffer().append("could not build ").
                                   append(file).toString());

            return null;
        }

        if(isAction(item) && (parent==root || parent==null))
            return null;
        Enumeration elems=item.children();
        VElement elem=null;
        int i = 0;
        while(elems.hasMoreElements()){
            elem=(VElement)elems.nextElement();
            if (i > 0)
                parent = lastElement();
            if(elem instanceof VTreeNodeElement)
            {
                elem = addElement(parent,elem);
                i=i+1;
                if (elem != null && expand_new)
                    expandElement(elem);
            }
        }
        ElementTree treewalker=new ElementTree((Node)elem);
        Node node=treewalker.getNext();
        while(node!=null){
            if(node instanceof VElement)
                ((VElement)node).setTemplate(this);
            node=treewalker.getNext();
        }
        if(empty && editor!=null)
            editor.setTitle(itemroot);
        return elem; // return last child
    }

    public void writeElement(VElement parent, String path)
    {
        ProtocolBuilder builder = new ProtocolBuilder(null);
        if (path == null)
            return;

        try
        {
            builder.newDocument();
            VElement elem = (VElement)parent.cloneNode(true);
            builder.doc.changeNodeOwner(elem);
            ((VElement)builder.doc.getDocumentElement()).add(elem);
            builder.save(path);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    public void showElementAll(String value)
    {
        boolean bShow = true;
        if (value != null && (value.equals("false") || value.equals("no")))
            bShow = false;

       //showElementAll(firstProtocol(), bShow);
       int nLength = aListProtocol.size();
       for (int i = 0; i < nLength; i++)
       {
           VElement elem = (VElement)aListProtocol.get(i);
           elem.setVisible(true);
       }
       aListProtocol.clear();

       ((ProtocolModel)treemodel).showElement();
       //protocolupdate.update();
    }

    public void showElementAll(VElement elem, boolean bShow)
    {
        if(elem==null || tree==null)
            return;

        TreePath path=new TreePath(treemodel.getPathToRoot(elem));
        if (aListProtocol.contains(elem))
        {
            if (bShow)
                aListProtocol.remove(elem);
        }
        else if (!bShow)
            aListProtocol.add(elem);

        elem.setVisible(bShow);

        showElementAll(getElementAfter(elem,PROTOCOLS), bShow);
    }

    public void hideElements()
    {
        int nLength = aListProtocol.size();
        for (int i = 0; i < nLength; i++)
        {
            VElement elem = (VElement)aListProtocol.get(i);
            elem.setVisible(false);
        }

        ((ProtocolModel)treemodel).showElement();
        //protocolupdate.update();
    }

    public void showElement(ArrayList aListElem, String value)
    {
        if (value == null || value.equals(""))
            return;

        boolean bShow = true;
        if (value.equals("false") || value.equals("no"))
            bShow = false;

        int nLength = aListElem.size();
        for (int i = 0; i < nLength; i++)
        {
            VElement elem = (VElement)aListElem.get(i);
            if (aListProtocol.contains(elem))
            {
                if (bShow)
                    aListProtocol.remove(elem);
            }
            else if (!bShow)
                aListProtocol.add(elem);

            elem.setVisible(bShow);
        }
        ((ProtocolModel)treemodel).showElement();
        //protocolupdate.update();
    }

    //----------------------------------------------------------------
    /** set local ownership of new node. */
    //----------------------------------------------------------------
    public void setOwner(VElement elem){
        Node node=(Node)elem;
        ElementTree treewalker=new ElementTree(node);
        doc.changeNodeOwner(elem);
        while(node!=null){
            if(node instanceof VElement){
                elem=(VElement)node;
                elem.setTemplate(this);
            }
            node=treewalker.getNext();
        }
    }

    //----------------------------------------------------------------
    /** Add an Element parsed from an XML file to current selection. */
    //----------------------------------------------------------------
    public VElement addToSelected(String file){
        VElement elem=getSelected();
        if(elem !=null)
            return addElement(elem,file);
        return null;
    }

    //----------------------------------------------------------------
    /** Delete an Element node. */
    //----------------------------------------------------------------
    public boolean deleteElement(VElement elem) {
        if(treemodel==null){
            Messages.postError("ProtocolBuilder ERROR: invalid tree model");
            return false;
        }

        if(elem == null)
            return false;
        if(elem.getParent()==null){
            Messages.postError("ProtocolBuilder ERROR: cannot remove root element");
            return false;
        }
        if (aListProtocol.contains(elem))
        {
            Messages.postDebug("Node must be visible to delete it");
            return false;
        }
        treemodel.removeNodeFromParent(elem);
        aListProtocol.remove(elem);
        if(elem == getSelected())
            setSelected(null);
        if((queue==null || isEmpty()) && aListProtocol.isEmpty())
            setIds();
        if(editor !=null && isEmpty())
            editor.setTitle(rootElement());

        return true;
    }

    //----------------------------------------------------------------
    /** Delete element (called from trashcan). */
    //----------------------------------------------------------------
    public boolean removeElement(VElement elem){
        if(!canDelete(elem))
            return false;
        return deleteElement(elem);
    }

    //----------------------------------------------------------------
    /** Remove the currently selected Element node. */
    //----------------------------------------------------------------
    public void removeSelected(){
        if(getSelected()==null)
            return;
        if(removeElement(getSelected()))
            super.removeSelected();
    }

   /************************************************** <pre>
    * Test permission to move an Element node.
    * 
    *  - Called in response to a user's mouse action in the tree.   
    *    If move validation is enabled a "testmove" sqaction
    *    will be sent to vnmrbg. Vnmrbg then will validate 
    *    whether or not the move is allowed and if so send a
    *    "SQ move" command back to do the actual move
    *    (via moveElement).
    * -  If move validation is disabled moveElement is called
    *    immediately.
    * -  In all cases, moveElement further checks whether the
    *    move request will violate other criteria (lock permissions
    *    etc) and if so returns "false" without carrying out
    *    the move.
    * -  vnmrbg can override normal locking behavior by first
    *    enabling move validation. In response to a "testmove",
    *    if the move request is to a locked part of the tree
    *    an "SQ move" command can be sent with an "ignore lock"
    *    argument (new syntax - see StudyQueue.process).  
    <pre>**************************************************/
    private void testMoveElement(VElement src, VElement dst) {
        if(queue !=null){
	        String sid=src.getAttribute(ProtocolBuilder.ATTR_ID);
	        String did=dst.getAttribute(ProtocolBuilder.ATTR_ID);
	        if(sid==null || did==null)
	        	return;
	        if(did.length()==0)
	        	did="null";
	        if(validateMove()){
	        	queue.requestMove(sid,did);
	            return;       	
	        }
        }
        moveElement(src, dst, false);
    }

    //----------------------------------------------------------------
    /** Move an Element node. (called for SQ move) */
    //----------------------------------------------------------------
    public boolean moveElement(VElement src, VElement dst, boolean ignorelock) {

        if(dst==null || src==null || src==dst)
            return false;

        if(isDecendent(src,dst)){
        	Messages.postError("ProtocolBuilder: can't move parent node to decendent");
            return false;
        }

        if(!ignorelock && queue !=null && !queue.testMove(src,dst))
            return false;

        boolean was_collapsed=isCollapsed(src);
        treemodel.removeNodeFromParent(src);
        addElement(dst,src, ignorelock);
        selectElement(src,dst,MOVE);
        if(!was_collapsed)
            expandElement(src, false);
        return true;
    }

    /************************************************** <pre>
     * Summary: Append a Protocol to the End of the Study Queue.
     *
     </pre> **************************************************/
    public void appendProtocol(String fullpath) {
        VElement dst;
        if(queue==null)
            dst=rootElement();
        else
            dst=lastElement();
        insertProtocol(dst, fullpath);
    }

    public void appendProtocol(ShufflerItem shufflerItem)
    {
        VElement dst = null;
        String nid = "null";

        if (queue == null)
            dst = rootElement();
        else
            dst = lastElement();

        if (shufflerItem.objType.equals(Shuf.DB_PROTOCOL)) {
            if (queue != null)
                shufflerItem.actOnThisItem("StudyQueue", "DragNDrop", nid);
            else if (editor != null)
                insertProtocol(dst, shufflerItem.getFullpath());
        }

    }

    /************************************************** <pre>
     * Summary: Insert a Protocol into the the Study Queue.
     *
     </pre> **************************************************/
    public boolean insertProtocol(VElement dst, String fullpath) {
        VElement elem=addElement(dst, fullpath);
        if(elem !=null){
            if(expand_new)
                expandElement(elem);
            selectElement(elem);
            return true;
        }
        return false;
    }


    /************************************************** <pre>
     * Summary: Insert a Study into the the Study Queue.
     *
     *
     </pre> **************************************************/
    public boolean insertStudy(String fullpath) {
        if(canLoadStudy(fullpath)){
            newTree(getStudyName(fullpath));
            return true;
        }
        return false;
    }

    public void setSelectPanel(JPanel panel) {
        selectpanel = panel;
    }

    /**
     * Test method: adds elements from given study directory to the SQ
     * @param queueDir e.g., vnmrsys/studies/exp1
     */
    public void addQueue(String queueDir) {
        SQBuilder builder = SQBuilder.getSQBuilder();
        List<VActionElement> elements = builder.getQueuedActions(queueDir);
        VElement parent = null;
        for (VElement element : elements) {
            addElement(parent, element);
            parent = element;
        }
    }

//    @Override
//    public void addLayoutComponent(String name, Component comp) {
//        layout.addLayoutComponent(name, comp);
//    }
//
//    @Override
//    public void layoutContainer(Container parent) {
////        Messages.postDebug("***--- PBTree.layoutContainer");
////        SQActionRenderer.resetSize();
//        layout.layoutContainer(parent);
//    }
//
//    @Override
//    public Dimension minimumLayoutSize(Container parent) {
//        return layout.minimumLayoutSize(parent);
//    }
//
//    @Override
//    public Dimension preferredLayoutSize(Container parent) {
//        return layout.preferredLayoutSize(parent);
//    }
//
//    @Override
//    public void removeLayoutComponent(Component comp) {
//        layout.removeLayoutComponent(comp);
//    }


    //******************* Local Classes *****************************

    public class PBTree extends JTree implements DragGestureListener,
                                                 DragSourceListener,
                                                 DropTargetListener,
                                                 MouseListener,
                                                 StatusListenerIF,
                                                 ComponentListener    {

        DragSource dragSource;
        DropTarget dropTarget;
        Cursor  oldCursor = null;
        private Dimension actionWidgetSize;

        public PBTree(DefaultTreeModel model) {
            super(model);
            putClientProperty("JTree.lineStyle", "None");
            setShowsRootHandles(true);
            setRootVisible(false);
            getSelectionModel().setSelectionMode(DISCONTIGUOUS_TREE_SELECTION);
            setEditable(false);
            setCellRenderer(renderer);
            setExpandsSelectedPaths(true);
            addMouseListener(PBTree.this);
            ExpPanel.addStatusListener(this);

            setIndents();
            //
            // Set icons
            //ui.setCollapsedIcon(null);
            //ui.setExpandedIcon(null);

            dropTarget = new DropTarget(PBTree.this,PBTree.this);
            dragSource = new DragSource();
            dragSource.createDefaultDragGestureRecognizer(PBTree.this,
                                   DnDConstants.ACTION_COPY_OR_MOVE,
                                   PBTree.this
                                   );
        }

        /**
         * Set the amount of indentation for the tree.
         */
        private void setIndents() {
            // NB: Control tree indentation and expansion control icons
            // If lines are drawn for the node's children, the
            // "leftChildIndent" is the part left of the vertical line, and the
            // "rightChildIndent" is the part right of the line.
            // Their sum is the total indent per tree level.
            //
            // Read default indents this way:
            //Messages.postDebug("Left=" + UIManager.getInt("Tree.leftChildIndent"));
            //Messages.postDebug("Right=" + UIManager.getInt("Tree.rightChildIndent"));
            //
            // Set indents:
            BasicTreeUI tui = (BasicTreeUI)this.getUI();
            if (tui.getLeftChildIndent() + tui.getRightChildIndent() != m_treeIndent) {
                tui.setLeftChildIndent(3);
                tui.setRightChildIndent(7);
                m_treeIndent = tui.getLeftChildIndent() + tui.getRightChildIndent();
            }
        }

        public TreePath getElementPath(int x, int y) {
            Point p = new Point(x, y);
            TreePath path = getClosestPathForLocation(p.x, p.y);
            Rectangle rect = getPathBounds(path);
            if (rect == null) {
                return null;
            }
            int y1 = (int)rect.getY();
            int y2 = (int)(y1 + rect.getHeight());
            if (p.y >= y1 && p.y <= y2) {
                return path;
            }
            return null;
        }

        // MouseListener interface

        public void mouseClicked(MouseEvent e) {
            Messages.postDebug("SQMouse", "Mouse clicked in tree");/*TMP*/
            int clicks = e.getClickCount();
            int modifier = e.getModifiers();
            if ((modifier & InputEvent.BUTTON1_MASK) != 0){
                if (clicks == 2){
                    TreePath path=getElementPath(e.getX(), e.getY());
                    Messages.postDebug("SQMouse", "Path=" + path);/*TMP*/
                    if (path != null) {
                        VElement sel=(VElement)path.getLastPathComponent();/*TMP*/
                        Messages.postDebug("SQMouse", "sel=" + sel);/*TMP*/
                        doDoubleClick();
                    }
                }
            }
        }

        public void mouseReleased(MouseEvent e) {}

        public void mouseEntered(MouseEvent e) {}

        public void mouseExited(MouseEvent e) {}

        public void mousePressed(MouseEvent e) {
            TreePath path=getElementPath(e.getX(), e.getY());
            if (path == null) {
                setSelected(null);
            } else {
                //tree.setSelectionPath(path);
                VElement sel=(VElement)path.getLastPathComponent();
                setSelected(sel);
            }
        }

        // DragGestureListener interface

        public void dragGestureRecognized(DragGestureEvent evt) {
            if (getSelected()==null)
                return;
            int action=evt.getDragAction();
            VElement elem=null;
            elem=getSelected();
            if (elem != null) {
                String id = elem.getAttribute(ATTR_ID);
                if (!"tmpstudy".equals(id)) {
                    // OK to drag this node
                    LocalRefSelection ref = new LocalRefSelection(elem);
                    dragSource.startDrag(evt, null, ref, PBTree.this);
                }
            }
        }

        // DragSourceListener interface

        public void dragDropEnd (DragSourceDropEvent evt)       { }
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
            maybeAcceptDrop(e);
        }

        public void dragExit(DropTargetEvent e) {
            debugDnD("dragExit");
        }

        public void dragOver(DropTargetDragEvent e) {
            debugDnD("dragOver");
            maybeAcceptDrop(e);
        }

        public void dropActionChanged (DropTargetDragEvent e) {
            debugDnD("dragActionChanged");
            maybeAcceptDrop(e);
        }

        /**
         * Accept or reject a drop event based on location and what is
         * being dragged.
         * @param e The drop event to evaluate.
         */
        private void maybeAcceptDrop(DropTargetDragEvent e) {
            Transferable tr = e.getTransferable();
            Object data = null;
            try {
                data = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
            } catch (Exception ex) {}
            boolean ok = true;
            if (data != null && (data instanceof VActionElement)) {
                VActionElement elem = (VActionElement)data;
                // Check if a node can be dropped here
                ok = false;
                boolean srcLocked = "on".equals(elem.getAttribute(ATTR_LOCK));
                // Locked nodes cannot be moved around within the panel
                if (!srcLocked) {
                    Point p = e.getLocation();
                    TreePath path=getElementPath(p.x,p.y);
                    Messages.postDebug("DnD", "path=" + path);
                    if (path != null) {
                        Messages.postDebug("DnD", "path=" + path);
                        VElement dst = (VElement)path.getLastPathComponent();
                        Messages.postDebug("DnD", "dst=" + dst);
                        VElement dst2 = (VElement)dst.getNextSibling();
                        if (dst2 == null) {
                            ok = true;
                        } else {
                            String lock = dst2.getAttribute(ATTR_LOCK);
                            Messages.postDebug("DnD", "lock=" + lock);
                            ok = (!"on".equals(lock));
                        }
                    }
                }
            }
            int dropAction = e.getDropAction();
            if (ok) {
                ok =  (dropAction == TransferHandler.COPY
                       || dropAction == TransferHandler.MOVE);
            }

            if (ok) {
                e.acceptDrag(dropAction);
            } else {
                e.rejectDrag();
            }
        }

	private String GetImgPath(String path) {
	   if(path.endsWith("procpar")) {
	      String dir = path.substring(0,path.length()-8);
	      if(dir.endsWith(".img")) return dir;
	   } 
	   return null;
	}

	public void drop(DropTargetDropEvent e) {
        boolean success = false;

	    try {
	        Transferable tr = e.getTransferable();
	        if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)
	                ||  tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
	            Point p = e.getLocation();
	            TreePath path=getElementPath(p.x,p.y);
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
	                // If this is not a locator recognized objType, then
	                // disallow the drag.
	                String objType = FillDBManager.getType(fullpath);
	                if(objType.equals("?")) {
	                    String imgPath = GetImgPath(fullpath);
	                    if(imgPath != null) { 
	                        //fullpath = imgPath;
	                    } else {
	                        Messages.postError("Unrecognized drop item " + 
	                                fullpath);
	                        return;
	                    }
	                }

	                // This assumes that the browser is the only place that
	                // a string is dragged from.  If more places come into
	                // being, we will have to figure out what to do
	                ShufflerItem item = new ShufflerItem(fullpath, 
	                "BROWSER");
	                // Replace the obj with the ShufflerItem, and continue
	                // below.
	                obj = item;
	            }
	            // Not string, get the dragged object
	            else {
	                obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
	            }

	            VElement dst=null;
	            String nid="null";

	            if (path != null) {
	                dst=(VElement)path.getLastPathComponent();
	                nid=dst.getAttribute(ATTR_ID);
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
	                    e.rejectDrop();
	                    return;
	                }

	                // Go through the list and do the following operation
	                // for each item in the list
	                for(int i=0; i<list.size(); i++) {
	                    ShufflerItem item = (ShufflerItem)list.get(i);

	                    if(dst==null)
	                        dst=rootElement();
	                    if (item.objType.equals(Shuf.DB_PROTOCOL)) {
	                        if (queue != null) {
	                            String id = queue.getSqId();
	                            if(id.equals("SQ")) {
	                                item.actOnThisItem("StudyQueue", "DragNDrop", nid);
	                            }
	                            else
	                                item.actOnThisItem(id, "DragNDrop", nid);
	                        }
	                        else if (editor != null) {
	                            if (insertProtocol(dst,item.getFullpath())) {
	                                e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
	                            }
	                        }
	                        e.getDropTargetContext().dropComplete(true);
	                        success = true;
	                    }
	                    else if (item.objType.equals(Shuf.DB_STUDY) ||
	                            item.objType.equals(Shuf.DB_AUTODIR)  ||
	                            item.objType.equals(Shuf.DB_LCSTUDY)) {
	                        if (queue != null) {
	                            String id = queue.getSqId();
	                            if(id.equals("SQ"))
	                                item.actOnThisItem("StudyQueue", "DragNDrop", "");
	                            else
	                                item.actOnThisItem(id, "DragNDrop", "");
	                        }
	                        else if (editor != null) {
	                            if (insertStudy(item.getFullpath())) {
	                                e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
	                            }
	                        }
	                        e.getDropTargetContext().dropComplete(true);
	                        success = true;
	                    }
	                    else if (item.objType.equals(Shuf.DB_IMAGE_DIR)) {
	                        if (queue != null) {
	                            String id = queue.getSqId();
	                            if(id.equals("SQ"))
	                                item.actOnThisItem("StudyQueue", "DragNDrop", "");
	                            else
	                                item.actOnThisItem(id, "DragNDrop", "");

	                            e.getDropTargetContext().dropComplete(true);
	                            success = true;
	                        }
	                    }
	                    else if (item.objType.equals(Shuf.DB_VNMR_DATA) || 
	                            item.objType.equals(Shuf.DB_VNMR_PAR) ||
	                            item.objType.equals(Shuf.DB_FILE)) {
	                        if (queue != null) {
	                            String id = queue.getSqId();
	                            if(id.equals("SQ"))
	                                item.actOnThisItem("StudyQueue", "DragNDrop", "");
	                            else
	                                item.actOnThisItem(id, "DragNDrop", "");

	                            e.getDropTargetContext().dropComplete(true);
	                            success = true;
	                        }

	                    }
	                    else if (item.objType.equals(Shuf.DB_TRASH)) {
	                        if (queue != null) {
	                            String id = queue.getSqId();
	                            if(id.equals("SQ"))
	                                item.actOnThisItem("StudyQueue", "DragNDrop", "");
	                            else
	                                item.actOnThisItem(id, "DragNDrop", "");
	                        }
	                        e.getDropTargetContext().dropComplete(true);
	                        success = true;
	                    }
	                }
	            }
	            else if (obj instanceof VTreeNodeElement) {
	                VElement src=(VTreeNodeElement)obj;
	                int action=e.getDropAction();
	                if (dst == null) {
	                    if (isAction(src)) 
	                        dst=lastElement();
	                    else
	                        dst=rootElement();
	                }

	                Document sdoc=src.getOwnerDocument();
	                if (sdoc != doc) {
	                    Template mgr=src.getTemplate();
	                    if (mgr !=null && mgr instanceof ProtocolBuilder){
	                        VElement clone=cloneElement(src);
	                        setOwner(clone);
	                        addElement(dst,clone);
	                        selectElement(clone,null,COPY);
	                        //((ProtocolBuilder)mgr).removeElement(src);
	                        e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
	                        e.getDropTargetContext().dropComplete(true);
	                        success = true;
	                    }
	                }
	                if(action==DnDConstants.ACTION_MOVE){
	                    e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
	                    testMoveElement(src,dst);
	                    e.getDropTargetContext().dropComplete(true);
	                    success = true;
	                }
	                else if(action==DnDConstants.ACTION_COPY){
	                    if(queue==null){
	                        src=cloneElement(src);
	                        addElement(dst,src);
	                    }
	                    selectElement(dst,null,COPY);
	                    e.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
	                    e.getDropTargetContext().dropComplete(true);
	                    success = true;
	                }
	            }
	        }
	    } catch (IOException io) {
	        io.printStackTrace();
	    } catch (UnsupportedFlavorException ufe) {
	        ufe.printStackTrace();
	    }
	    if(!success)
	        e.rejectDrop();
	}

        public void componentHidden(ComponentEvent arg0) {
            // TODO Auto-generated method stub
            
        }

        public void componentMoved(ComponentEvent arg0) {
            // TODO Auto-generated method stub
            
        }

        public void componentResized(ComponentEvent e) {
//            if (e.getSource() == tree) {
//                // TODO: Scroll down if newly added node is invisible
//
//                // NB: 216 is a magic number that the JViewport likes
//                final int minTreeWidth = 216;
//                tree.setPreferredSize(null);
//                int prefHeight = (int)tree.getPreferredSize().getHeight();
//                int treeWidth = tree.getWidth();
//                JViewport parent = (JViewport)tree.getParent();
//                parent.scrollRectToVisible(new Rectangle(0, prefHeight, 0, 0));
//                int viewWidth = Math.max(minTreeWidth, parent.getWidth());
//                Messages.postDebug("resize: treeWidth=" + treeWidth
//                        + ", viewWidth=" + viewWidth
//                        + ", prefHeight=" + prefHeight
//                        + ", new width=" + viewWidth
//                        + ", viewPrefWidth=" + parent.getPreferredSize().getWidth()
//                );
//                if (treeWidth != viewWidth && treeWidth > minTreeWidth) {
//                    Dimension dim = new Dimension(viewWidth, prefHeight);
//                    ((JViewport)parent).setViewSize(dim);
//                    tree.setPreferredSize(dim);
//                    Messages.postDebug("> width=" + tree.getWidth()
//                            + ", vpWidth=" + parent.getWidth());
//                }
//                // Force tree to be laid out anew
//                tree.setCellRenderer(null);
//                tree.setCellRenderer(renderer);
//            }
        }

        @Override
        public void componentShown(ComponentEvent arg0) {
            // TODO Auto-generated method stub
            
        }

        /**
         * Paint the whole tree
         */
        public void paintComponent(Graphics g) {
            Messages.postDebug("SQPaint", "***--- PBTree.paintComponent()"
                               + ", selectionCount=" + getSelectionCount());

            setIndents();
            setPanelColor();
            //if (isEmpty()) {
            //    // Make sure m_executingAction is displayed iff
            //    // it is actually executing.
            //    tree.remove(m_executingAction);
            //    Messages.postDebug("Removed m_executingAction");
            //}

            // Set Action widget size to max needed of all Action nodes
            // Put in mode to recalculate node sizes:
            SQActionRenderer.setMaxSize(null);
            // Make them at least wide enough to reach the right margin
            // NB: the "width" in dim is the width assuming no left margin
            int availableX = tree.getParent().getWidth() - TREE_MARGIN;
            Dimension dim = new Dimension(availableX, 10);
            dim = getMaxActionWidgetSize(rootElement(), m_treeIndent, dim);
            // Set the new calculated node extent
            SQActionRenderer.setMaxSize(dim);
            if (m_treeDirty  || !dim.equals(actionWidgetSize)) {
                m_treeDirty = false;
                // NB: Remember the SQActionRenderer's widget size
                actionWidgetSize = new Dimension(dim);
                Messages.postDebug("SQInvalidate", "Calling invalidateTree()");
                invalidateTree();
                setRowHeight(0);
                //return;
            }

            setSquishCounts(rootElement(), 0);

            //m_isExecuting  = false;
            super.paintComponent(g);
            setRowHeight(-1);
            //if (m_isExecuting) {
            //    tree.add(m_executingAction);
            //    Messages.postDebug("Added: m_executingAction");
            ////} else {
            ////    tree.remove(m_executingAction);
            ////    Messages.postDebug("Removed: m_executingAction");
            //}
        }

        @Override
        public void updateStatus(String s) {
            // TODO Auto-generated method stub
            Messages.postDebug("SQStatus", "PBTree.updateStatus: " + s);
        }

        /**
         * Get the maximum size needed by any action widget. The size returned
         * is not actually the real size. The height is correct, but the width
         * is actually the width plus the x-coordinate of the widget. I.e., the
         * position of the right-hand end of the widget. The idea is to make the
         * right ends of all the action widgets line up, regardless of their
         * indentation.
         *
         * @param parent Look at all the descendants of this node.
         * @param indent The indentation of the (first generation) children of
         *        the specified parent.
         * @param dim The minimum dimension to accept; or think of it as the
         *        largest so far.
         * @return The maximum size, where the "width" is actually the widget X
         *         offset plus the widget width.
         */
        protected Dimension getMaxActionWidgetSize(VElement parent, int indent,
                                                   Dimension dim) {

            if (!isExpanded(getPath(parent))) {
                return dim;
            }
            int n = treemodel.getChildCount(parent);
            for (int i = 0; i < n; i++) {
                VElement elem = getElement(parent, i);
                if (elem instanceof VActionElement) {
                    ((VActionElement)elem).setIndent(indent);
                    m_actionRenderer.setToDisplay((VActionElement)elem, false);
                    Dimension myDim = m_actionRenderer.getRequiredSize();

                    // Make Action Nodes span the whole window width
                    // NB: The parent of the tree is the JViewport it is in.
                    // The tree may be wider than the JViewport.
                    int availableX = tree.getParent().getWidth() - TREE_MARGIN;
                    int myMaxX = indent + myDim.width;
                    myDim.width = Math.max(availableX, myMaxX) ;

                    if (dim.width < myDim.width){
                        dim.width = myDim.width;
                    }
                    if (dim.height < myDim.height){
                        dim.height = myDim.height;
                    }
                }
                if (treemodel.getChildCount(elem) > 0) {
                    dim = getMaxActionWidgetSize(elem, indent + m_treeIndent,
                                                 dim);
                }
            }
            return dim;
        }

        /**
         * The squishCount attribute is put on all squished nodes so that
         * the node knows how many squish nodes are immediately above it.
         * Only the first few nodes in the group of squished nodes are
         * displayed at all.
         * @param parent A parent node in the tree.
         * @param squishCount The current squish count.
         * @return The current (updated) squish count.
         */
        private int setSquishCounts(VElement parent, int squishCount) {
//            if (!isExpanded(getPath(parent))) {
//                return 0;
//            }
            int n = treemodel.getChildCount(parent);
            for (int i = 0; i < n; i++) {
                VElement elem = getElement(parent, i);
                boolean isSquished = "true".equals(getAttribute(elem, ATTR_SQUISHED));
                if ("true".equals(getAttribute(elem, ATTR_SQUISHED))) {
                    int oldCount = -1;
                    try {
                        oldCount = Integer.parseInt(elem.getAttribute(ATTR_SQUISH_COUNT));
                    } catch (NumberFormatException nfe) {
                    }
                    if ((oldCount < MAX_SQUISH && squishCount >= MAX_SQUISH)
                        || (oldCount >= MAX_SQUISH & squishCount < MAX_SQUISH))
                    {
                        String newCount = Integer.toString(squishCount);
                        elem.setAttribute(ATTR_SQUISH_COUNT, newCount);
                        treemodel.nodeChanged(elem);
                    }
                    squishCount++;
                } else {
                    squishCount = 0;
                }
                if (treemodel.getChildCount(elem) > 0) {
                    squishCount = setSquishCounts(elem, squishCount);
                }
            }
            return squishCount;
        }

      public String getToolTipText(MouseEvent me) {
          TreePath treepath = getPathForLocation(me.getX(), me.getY());
          if (treepath == null) {
              return null;
          }
          Object obj = treepath.getLastPathComponent();
          if (!(obj instanceof VElement)) {
              return null;
          }
          VElement velem = (VElement)obj;
          String ttext = velem.getAttribute(ATTR_TOOLTEXT);
          if(ttext == null || ttext.length() == 0){
              String status = velem.getAttribute(ATTR_STATUS);
              String title = velem.getAttribute(ATTR_TITLE).trim();
              ttext = new StringBuffer().append(velem.getNodeName()).append(" ")
              .append(title).append(" ")
              .append(velem.getAttribute(ATTR_ID)).toString().trim();
              if (status.length() > 0) {
                  ttext = new StringBuffer(ttext).append(" [")
                  .append(status).append("]").toString();
              }
          } else {
              StringTokenizer tok = new StringTokenizer(ttext, " \t\n");
              ttext = "";
              while (tok.hasMoreTokens()) {
                  ttext += tok.nextToken().trim()+" ";
              }
          }
          return ttext;
      }

    } // class PBTree


    class ProtocolModel extends DefaultTreeModel implements Runnable
   {
       boolean m_bFilter = false;

       public ProtocolModel(MutableTreeNode root)
       {
           super(root);
       }

       public void setFilter(boolean bShow)
       {
           m_bFilter = bShow;
       }

       public boolean isFilter()
       {
           return m_bFilter;
       }

       public int getChildCount(Object parent)
       {
           MutableTreeNode node = (MutableTreeNode)parent;

           int length = 0;
           if (node == null)
               return length;

           Enumeration elist = node.children();
           while (elist.hasMoreElements())
           {
               VElement elem = (VElement)elist.nextElement();
               if (!aListProtocol.contains(elem))
                   length = length+1;
           }
           return length;
           /*if (m_bFilter && parent instanceof VProtocolElement)
                return ((VProtocolElement)node).getChildCount();
            else
                return node.getChildCount();*/
       }

       public void removeNodeFromParent(MutableTreeNode node)
       {
           if (node == null)
               return;
           super.removeNodeFromParent(node);
           /*MutableTreeNode parent = (MutableTreeNode)node.getParent();
           int index = getIndexOfChild(parent,node);
           System.out.println(index);
           ((VElement)node).dump();
           int[] childIndex = {index};
           //parent.remove(index);
           node.removeFromParent();
           Object[] removedArray = {node};
           nodesWereRemoved(parent, childIndex, removedArray);*/
       }

       public void run()
       {
            Object[] path = {root};
            int size = root.getChildCount();
            int[] childIndices = new int[size];
            Object[] children = new Object[size];
            for (int i = 0; i < size; i++)
            {
                childIndices[i] = i;
                children[i] = root.getChildAt(i);
            }
            fireTreeStructureChanged(this, path, childIndices, children);
            setTreeExpanded(firstProtocol());
        }

       public void showElement()
       {
           try {
               if (EventQueue.isDispatchThread()) {
                   this.run();
               } else {
                   SwingUtilities.invokeAndWait(this);
               }
           }
           catch (Exception e)
           {
               //e.printStackTrace();
               Messages.writeStackTrace(e);
           }
       }
    }


   public class ProtocolRenderer extends DefaultTreeCellRenderer
    {
        Font font=null;
        int iwidth=0;
        
        // For debug:
        String m_id = "--";

        public ProtocolRenderer() {}

        public Component getTreeCellRendererComponent(
                            JTree tree,
                            Object value,
                            boolean sel,
                            boolean expanded,
                            boolean leaf,
                            int row,
                            boolean hasFocus) {

//            if (value instanceof VActionElement) {
//                boolean selected = tree.isRowSelected(row);
//                m_actionRenderer.setToDisplay((VActionElement)value, selected);
//                //// If this VActionElement is executing, set the
//                //// m_executingAction to overlay this tree node.
//                //if (m_actionRenderer.getStatus().equals(EXECUTING)) {
//                //    Messages.postDebug("Node " + m_actionRenderer.getId()
//                //                       + " is executing");
//                //    m_executingAction.setToDisplay((VActionElement)obj);
//                //    m_executingNodeRect = tree.getRowBounds(row);
//                //    m_isExecuting = true;
//                //}
//                return m_actionRenderer;
//            }

            super.getTreeCellRendererComponent(
                            tree, value, sel,
                            expanded, leaf, row,
                            hasFocus);
            boolean active_flag=false;
            VElement obj=(VElement)value;
            
            // For debug:
            m_id = obj.getAttribute(ATTR_ID);

            String type=obj.getAttribute(ATTR_TYPE);
            String status = obj.getAttribute(ATTR_STATUS);
            String title = obj.getAttribute(ATTR_TITLE).trim();
            String var="PlainText";
            if (obj instanceof VProtocolElement)
                var="Protocol";
            else if(obj instanceof VActionElement){
                var="Action";
                if(status!=null && status.length()>0 && !status.equals("null")){
                    if(status.equals(SQ_SKIPPED))
                        var="NotPresent";
                    else if(status.equals(SQ_READY)){
                        if(type!=null && type.equals(REQ))
                            var="RequiredPar";
                        else
                            var="Ready";
                    }
                    else{
                        var=status;
                        if(status.equals(SQ_EXECUTING))
                            active_flag=true;
                    }
                }
            }
            DisplayStyle style = getStyleForNode(obj, var);
            font = style.getFont();
            setFont(font);

            setOpaque(false);
            setForeground(style.getFontColor());

            status = Util.getLabelString(status); // Translate status label

            StringTokenizer tok = new StringTokenizer(title, " \t\n");
            title = "";
            while(tok.hasMoreTokens()) 
                title += tok.nextToken().trim()+" "; 

            String oldtitle=getText();
            if(showtimes){
                String time=obj.getAttribute(ATTR_TIME);
                if(time !=null && time.length()>0 && !time.equals("null"))
                    title=new StringBuffer().append("[").append(time).append("] ").
                           append(title).toString();
            }
            title=Util.getLabelString(title.trim());
            if(!title.equals(oldtitle)){
                setText(title);
            }
            iwidth=0;
            if(show_icons){
                String data=obj.getAttribute(ATTR_DATA);
                String lock=obj.getAttribute(ATTR_LOCK);
                boolean has_lock=(lock !=null && lock.length()>0 && lock.equals("on"));
                boolean has_data=(data !=null && data.length()>0 && !data.equals("null"));
                if(has_lock && has_data){
                    setIcon(Util.getImageIcon("lock_echo.gif"));
                    iwidth=32;
                }
                else if(has_lock){
                    iwidth=16;
                    setIcon(Util.getImageIcon("lock.gif"));
                }
                else if(has_data){
                    iwidth=16;
                    setIcon(Util.getImageIcon("echo.gif"));
                }
                else
                    setIcon(null);
            }

            if ("Ready".equalsIgnoreCase(var)) {
                m_count++;
            }/*TMP*/
            /*Messages.postDebug("SQPaint", "*** var=" + var
                               + ",  title=" + title
                               + " (count=" + m_count + ")");/*TMP*/

            if (obj instanceof VActionElement) {
                boolean selected = tree.isRowSelected(row);
                m_actionRenderer.setToDisplay((VActionElement)obj, selected);
                //// If this VActionElement is executing, set the
                //// m_executingAction to overlay this tree node.
                //if (m_actionRenderer.getStatus().equals(EXECUTING)) {
                //    Messages.postDebug("Node " + m_actionRenderer.getId()
                //                       + " is executing");
                //    m_executingAction.setToDisplay((VActionElement)obj);
                //    m_executingNodeRect = tree.getRowBounds(row);
                //    m_isExecuting = true;
                //}
                return m_actionRenderer;
            } else {
                return this;
            }
//            return this;
        }

        public Dimension getPreferredSize(){
            String text=getText();
            if(text==null || text.length()==0) {
                //Dimension dim = super.getPreferredSize();
                Dimension dim = new Dimension(3, 1);
                return dim;
            }
            text=text.trim();
            FontMetrics metrics=getFontMetrics(font);
            int w=metrics.stringWidth(text);
            int h=metrics.getHeight() + 6; // Allow for some margin
            
            Dimension dim=new Dimension(w, h);
            Messages.postDebug("SQPaint", "getPreferredSize: "
                               + ", ID=" + m_id
                               + ", w=" + dim.getWidth());
            return dim;
        }

    } // class ProtocolRenderer

    class ProtocolUpdate implements TreeExpansionListener, TreeSelectionListener
    {
        protected JTree m_tree;
        protected Set m_aListExpandedPath = new HashSet();
        protected TreePath[] m_selectedPath = new TreePath[0];
        protected boolean m_bSelected = false;

        public ProtocolUpdate(JTree tree)
        {
            m_tree = tree;
            m_tree.addTreeExpansionListener(this);
            m_tree.addTreeSelectionListener(this);
        }

        /*public synchronized void run()
        {
            ((ProtocolModel)m_tree.getModel()).reload();
            Iterator iter = m_aListExpandedPath.iterator();
            TreePath[] paths = new TreePath[m_aListExpandedPath.size()];
            int i = 0;
            while (iter.hasNext())
            {
                TreePath path = (TreePath)iter.next();
                paths[i] = path;
                i=i+1;
            }
            int nSize = paths.length;
            for (i = 0; i < nSize; i++)
            {
                if (paths[i].getLastPathComponent() != null)
                    m_tree.expandPath(paths[i]);
            }
            m_tree.getSelectionModel().setSelectionPaths(m_selectedPath);
        }*/

        /*public synchronized void update()
        {
            try
            {
                SwingUtilities.invokeLater(this);
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }*/

        public void clearPath()
        {
            m_aListExpandedPath.clear();
        }

        public void treeExpanded(TreeExpansionEvent e)
        {
            TreePath path = e.getPath();
            Iterator iter = m_aListExpandedPath.iterator();
            while (iter.hasNext())
            {
                TreePath treepath = (TreePath)iter.next();
                if (!m_aListExpandedPath.contains(treepath) &&
                    path.isDescendant(treepath))
                {
                    m_aListExpandedPath.add(treepath);
                    VElement elem = (VElement)treepath.getLastPathComponent();
                    elem.setAttribute(ATTR_EXPANDED, "true");
                }
            }

            m_aListExpandedPath.add(path);
            VElement elem = (VElement)path.getLastPathComponent();
            elem.setAttribute(ATTR_EXPANDED, "true");
        }

        public void treeCollapsed(TreeExpansionEvent e)
        {
            TreePath path = e.getPath();
            Iterator iter = m_aListExpandedPath.iterator();
            TreePath[] paths = new TreePath[m_aListExpandedPath.size()];
            int i = 0;
            while (iter.hasNext())
            {
                TreePath treepath = (TreePath)iter.next();
                paths[i] = treepath;
                i=i+1;
            }

            int nSize = paths.length;
            for (i = 0; i < nSize; i++)
            {
                TreePath treepath = paths[i];
                if (m_aListExpandedPath.contains(treepath) &&
                    path.isDescendant(treepath))
                {
                    m_aListExpandedPath.remove(treepath);
                    VElement elem = (VElement)treepath.getLastPathComponent();
                    elem.setAttribute(ATTR_EXPANDED, "false");
                }
            }

            m_aListExpandedPath.remove(path);
            VElement elem = (VElement)path.getLastPathComponent();
            elem.setAttribute(ATTR_EXPANDED, "false");
        }

        public void valueChanged(TreeSelectionEvent e)
        {
            // TODO: Is m_selectedPath used anywhere?
            if (m_tree.getSelectionPaths() != null &&
                m_tree.getSelectionPaths().length > 0)
            {
                m_selectedPath = m_tree.getSelectionModel().getSelectionPaths();
            }
            String paths = "selectedPaths=";
            for (int i = 0; m_selectedPath != null && i < m_selectedPath.length; i++) {
                paths += m_selectedPath[i].getLastPathComponent() + " ";
            }
            Messages.postDebug("SQSelect", paths);
        }
    }
    
//    class TreeOverlayLayout implements LayoutManager {
//
//        @Override
//        public void addLayoutComponent(String name, Component comp) {
//            // TODO Auto-generated method stub
//
//        }
//
//        @Override
//        public void layoutContainer(Container parent) {
//            Component[] comps = parent.getComponents();
//            for (Component comp : comps) {
//                if (comp instanceof SQActionRenderer) {
//                    comp.setBounds(m_executingNodeRect);
//                }
//            }
//
//        }
//
//        @Override
//        public Dimension minimumLayoutSize(Container parent) {
//            // TODO Auto-generated method stub
//            return null;
//        }
//
//        @Override
//        public Dimension preferredLayoutSize(Container parent) {
//            // TODO Auto-generated method stub
//            return null;
//        }
//
//        @Override
//        public void removeLayoutComponent(Component comp) {
//            // TODO Auto-generated method stub
//
//        }
//
//    }
}
