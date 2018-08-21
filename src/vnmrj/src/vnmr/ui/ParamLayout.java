/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.io.File;
import java.util.*;
import java.awt.*;
import java.awt.dnd.*;
import java.beans.PropertyChangeListener;

import javax.swing.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;

/**
 *  the panel to display parameters.
 */
public class ParamLayout extends ParamPanel
        implements VObjIF, VEditIF, VObjDef, StatusListenerIF, PropertyChangeListener
{
     /**
     * 
     */
    private static final long serialVersionUID = 1L;
    /**
     * NB: listModel is used from the event thread to paint the TabList.
     *     If unsynchronized changes are made to listModel, disaster
     *     can result.  Encapsulate all changes in
     *        synchronized(listModel) { ... change listModel ... }
     *     blocks.
     */
    private DefaultListModel listModel = null;
    private Vector<Component> tabs;
    private Component seltab=null;
    private String selLabel=null;
    private JList tabList=null;
    private HashMap menu_names;
    private ParameterPanel paramPanel=null;
    private boolean isNewPanel = true;
    protected Vector<Component>   groups;

    public static boolean tabupdate=false;

    /**
     * constructor
     * @param sshare session share
     */
    public ParamLayout(SessionShare sshare, ButtonIF vif) {
        this.sshare = sshare;
        this.VnmrIf = vif;
        this.panel = this;
        tabs = new Vector<Component>();
        groups = new Vector<Component>();
        menu_names = new HashMap();
        paramsLayout = new VRubberPanLayout();
        setLayout(paramsLayout);
        setType(ParamInfo.PARAMLAYOUT);
        
        setMouseAdaptor();
        
        String str = System.getProperty("updateHiddenTabs");
        if(str !=null){
            if(str.equals("yes") || str.equals("true"))
                tabupdate=true;
        }

        // drag-and-drop behavior

        new DropTarget(this, new LayoutDropTargetListener());
        DisplayOptions.addChangeListener(this);

    } // ParamLayout()

    protected void setDebug(String s){
        if(DebugOutput.isSetFor("panels")|| DebugOutput.isSetFor("ParamLayout"))
            Messages.postDebug(new StringBuffer().append("ParamLayout ").append(s).toString());
    }

    public void setParameterPanel(ParameterPanel p) {
        paramPanel=p;
    }
    public ParameterPanel getParameterPanel() {
        return paramPanel;
    }

    public void setTabList(JList t) {
        tabList = t;
        if (t != null)
            listModel=(DefaultListModel)tabList.getModel();
        else
            listModel = null;
    }

    public JList getTabList() {
        return tabList;
    }

    private void addToMenuList(Component comp) {
        if (listModel == null || comp==null)
            return;
        String name=getTabLabel(comp);
        // name=name.trim();
        if (name == null)
            return;
        name=name.trim();
        if (name.length() == 0)
            return;
        if(DebugOutput.isSetFor("panelmenu"))
            System.out.println(new StringBuffer().append("addToMenuList(").
                               append(name).append(")").toString());

        try {
            synchronized (listModel) {
                listModel.addElement(name);
            }
        } catch (NullPointerException npe) {
            // listModel was probably null
            if (listModel != null) {
                Messages.writeStackTrace(npe);
            }
        }
        menu_names.put(name,comp);
    }

    private void clearMenuList() {
        menu_names.clear();
        try {
            synchronized (listModel) {
                listModel.removeAllElements();
            }
        } catch (NullPointerException npe) {
            // listModel was probably null
            if (listModel != null) {
                Messages.writeStackTrace(npe);
            }
        }
    }

    /** return the Component of a tab name (or null if not found).*/
    public Component findTab(String name) {
         return name==null?null:(Component)menu_names.get(name);
    }

    /** test if a String (name) is a tab. */
    public boolean isTabLabel(String name) {
        if(name==null || name.length()==0)
            return false;
        return menu_names.containsKey(name);
    }

    /** return the tab label of a VObjIF.*/
    private String getTabLabel(Component obj) {
        String name=((VObjIF)obj).getAttribute(LABEL);
        if(name==null || name.length()==0)
            return null;
        return name.trim();
    }

    /** remove a component.  */
    public void removeTab(VObjIF obj) {
        ParamEditUtil.clearDummyObj();
        remove((JComponent)obj);

        String name=obj.getAttribute(LABEL);
        if(name==null || name.length()==0)
            return;
        name=name.trim();
        if(isTabLabel(name)){
            rebuildTabs();
            if(listModel == null || listModel.isEmpty())
                ParamEditUtil.setEditObj(this);
            else
                setFirstTab();
            validate();
            repaint();
        }
    }

    public void freeObj() {
        super.freeObj();
        listModel = null;
        paramsLayout = null;
    }

    private void freeLists() {
        clearMenuList();
        groups.removeAllElements();
        tabs.removeAllElements();
    }
    /** enable or disable panel controls. */
    public void enablePanel(boolean enable) {
        if (enabled != enable) {
           enabled=enable;
           int nLength = groups.size();
           for (int i = 0; i < nLength; i++) {
               VGroup grp = (VGroup)groups.get(i);
               grp.enableChildren(enable);
           }
        }
    }
    public void restore() {
        File file=new File(restoreFile);
        if (!file.exists()) {
            Messages.postError("restore : "+restoreFile+" does not exist");
            return;
        }
        ParamEditUtil.setEditObj(this);
        setDebug("restore()");
        removeAll();
        try {
            LayoutBuilder.build(panel,VnmrIf,restoreFile);
        }
        catch(Exception e) {
            Messages.postError("error rebuilding panel");
            Messages.writeStackTrace(e);
        }
        rebuildTabs();
        setLastSelectedTab();
        validate();
        repaint();
    }

    /** update all VObjIF values using a parameter list.*/

    public void updateValue (Vector params) {
        /*
         * This is now set up to do the updating of visible TabGroup last.
         * Why?  Because a pnew may come in that hides the current group
         * and enables other groups.  As soon as we update the current
         * TabGroup's show condition, it goes away, and some other
         * TabGroup must be chosen to be made the visible one.
         * If the show conditions of the other groups are already
         * updated, we have a fighting chance of choosing the correct
         * group to make visible.
         */
        int nums = getComponentCount();

        setDebug(new StringBuffer().append("updateValue(").append(params.firstElement()).
                 append(", ..)").toString());

        ArrayList<Component> deferredGroups = new ArrayList<Component>();
        for (int i = 0; i < nums; i++) {
            Component comp = getComponent(i);
            if (!(comp instanceof VObjIF))
                continue;
            VObjIF obj = (VObjIF) comp;

            if(isTabGroup(comp)){
                if(comp.isShowing()) {
                    deferredGroups.add(comp); // Remember this one to do last
                } else if (tabupdate) {
                    ((VGroup)comp).update(params);
                } else {
                     String showVal=obj.getAttribute(SHOW);
                     if(showVal!=null && hasVariable(obj,params)) {
                         VnmrIf.asyncQueryShow(obj, showVal);
                     }
                 }
            }
            else if (comp instanceof ExpListenerIF) {
                ((ExpListenerIF)comp).updateValue(params);
            }
            else if(hasVariable(obj,params)){
                 obj.updateValue();
           }
        }
        // Do any groups we skipped before (should be only one)
        for (int i = 0; i < deferredGroups.size(); i++) {
            ((VGroup)deferredGroups.get(i)).update(params);
        }
    }

    /** call hide command for outgoing page. */
    public void hideGroup() {
        if(seltab==null || !(seltab instanceof VGroup) || editMode)
            return;
        VGroup grp=(VGroup)seltab;
        String cmd=grp.getAttribute(CMD2);
        if(cmd !=null)
             setDebug(new StringBuffer().append("hideGroup(").append(cmd).
                      append(")").toString());
        grp.group_hide();
    }

    /** call show command for incoming page. */
    public void showGroup() {
        if(seltab==null || !(seltab instanceof VGroup) || editMode)
            return;
        VGroup grp=(VGroup)seltab;
        String cmd=grp.getAttribute(CMD);
        if(cmd !=null)
             setDebug(new StringBuffer().append("showGroup(").append(cmd).append(")").toString());

        grp.group_show();
    }

    /** select tab component. */
    public void setSelected(Component comp) {
        String name=getTabLabel(comp);
        if(listModel == null || listModel.isEmpty())
            return;
        if(isTabLabel(name)){
            VGroup grp= null;
            if(comp instanceof VGroup){
                grp=(VGroup)comp;;
                setSizeRatio(grp.getXRatio(), grp.getYRatio());
               // paramsLayout.setSizeRatio(grp.getXRatio(), grp.getYRatio());
            }
            if(grp!=null && comp!=seltab ){
                hideGroup();
                if(seltab!=null && isTabGroup(seltab))
                    seltab.setVisible(false);
                seltab=comp;
                selLabel=getTabLabel(seltab);
                setDebug(new StringBuffer().append("setSelected(").
                         append(grp.getAttribute(LABEL)).toString());
                expandGroup(comp);
                if(editMode){
                    grp.showGroup(true);
                    grp.setEditMode(true);
                }
                else{
                    if(!isNewPanel){
                        updateGrpAllValue(grp);
                        ExpPanel.updateStatusListener(grp);
                    }
                    isNewPanel=false;
                    showGroup();
                    grp.setVisible(true);
                }
                tabList.setSelectedValue(selLabel,true);
                if(editMode)
                    ParamEditUtil.setEditObj((VObjIF)seltab);
                grp.enableChildren(enabled);
            }
        }
        setPreferredLayoutSize();
        //paramsLayout.preferredLayoutSize(this);
    }

    /** select first tab. */
    private void setFirstTab() {
        if(listModel == null || listModel.isEmpty())
            return;
        setDebug("setFirstTab()");
        tabList.setSelectedIndex(0);
        String name=(String)tabList.getSelectedValue();
        JComponent comp=(JComponent)findTab(name);
        if(comp !=null && comp !=seltab)
             setSelected(comp);
     }

    /** select a tab (user selection. called from ParameterPanel). */
    public void selectTab(String name) {
        if(changeMode)
            return;
        name = name.trim();
        JComponent comp=(JComponent)findTab(name);
        if(comp!=null){
            setDebug(new StringBuffer().append("selectTab(").append(name).
                     append(")").toString());
            if(comp !=seltab)
                setSelected(comp);
         }
    }

    public boolean replaceTab(VGroup old, VGroup grp){
        int i;
        int nLength = getComponentCount();
        for (i = 0; i < nLength; i++) {
            Component comp = getComponent(i);
            if(old==comp)
                break;
        }
        if(i==getComponentCount()){
            setDebug("replace tab group not found");
            return false;
        }
        remove(i);
        add((Component)grp,i);
        i=groups.indexOf(old);
        if(i>=0)
            groups.setElementAt(grp,i);
        i=tabs.indexOf(old);
        if(i>=0)
            tabs.setElementAt(grp,i);
        String name=getTabLabel((JComponent)grp);
        menu_names.put(name,(JComponent)grp);
        return true;
    }

    private boolean expandGroup(Component c) {
        int index=groups.indexOf(c);
        if(index<0)
            return false;
        VGroup grp = (VGroup)groups.get(index);
        if(grp.getComponentCount()>0)
            return false;
        String attr=grp.getAttribute(EXPANDED);
        if(attr == null || attr.equals("no")){
            String name=grp.getAttribute(LABEL);
            String file=grp.getAttribute(REFERENCE);
            file=FileUtil.fileName(file); // strip off extensions
            String useref=grp.getAttribute(USEREF);
            if(useref.equals("yes") && file !=null && file.length()>0){
                String path=FileUtil.getLayoutPath(layoutDir,new StringBuffer().
                                                             append(file).append(".xml").toString());
                if(path==null)
                    path=FileUtil.openPath(new StringBuffer().append("PANELITEMS/").
                                           append(file).append(".xml").toString());
                if(path==null){
                    Messages.postError(new StringBuffer().append("couldn't open reference ").
                                       append(name).toString());
                    return false;
                }
                isNewPanel=false;
                try {
                    JComponent comp;
                    setDebug(new StringBuffer().append("building ").append(name).toString());
                    LayoutBuilder.build(grp,VnmrIf,path,true);
                    comp=(JComponent)grp.getComponent(0);
                    Dimension dim=comp.getSize();
                    Component[] comps = comp.getComponents();
                    setDebug(new StringBuffer().append("expanding ").append(name).
                             append(" ").append(comps.length).toString());
                    comp.removeAll();
                    grp.remove(comp);
                    ((VObjIF)comp).destroy();

                    int nSize = comps.length;
                    for(int j=0;j<nSize;j++){
                        grp.add(comps[j]);
                    }
                    grp.setAttribute(EXPANDED,"yes");
                    grp.setAttribute(USEREF,"yes");
                    grp.setPreferredSize(dim);
                    return true;
                }
               catch (Exception e) {
                    Messages.postError(new StringBuffer().append("error expanding reference ").
                                       append(file).toString());
                    Messages.writeStackTrace(e);
               }
            }
        }
        return false;
    }

    /** changed tab visibility (called by VGroup). */
    public void setChangedTab(String s, boolean vis) {
        setDebug(new StringBuffer().append("setChangedTab(").append(s).append(",").
                 append(vis).append(")").toString());
        boolean inlist=isTabLabel(s);
        if((inlist && !vis) || (!inlist && vis)){
            buildTabMenu();
            setLastSelectedTab();
        }
    }

    /** changed tab label (called by VGroup). */
    public void relabelTab(VGroup group, boolean istab) {
        int index=groups.indexOf(group);
        String name=getTabLabel((Component)group);
        setDebug(new StringBuffer().append("ParamLayout.relabelTab(").append(name).
                 append(",").append(istab).append(")").toString());
        if(index>=0){  // currently in groups
            if(istab){
                selLabel=name;
                buildTabMenu();
            }
            else{
                groups.remove(group);
                rebuildTabs();
            }
        }
        else if(istab && isTabGroup(group)){
            selLabel=name;
            rebuildTabs();
            setLastSelectedTab();
        }
    }

    /** return bounding rectangle of selected tab. */
    public Rectangle getSelectedTabPos() {
        JComponent  tobj = (JComponent)seltab;
        if (tobj == null)
            return (new Rectangle(0, 0, 10, 10));
        Point pt = getRelativeLoc(tobj);
        Rectangle rect = tobj.getBounds();
        rect.x = pt.x;
        rect.y = pt.y;
        return (rect);
    }

    /** test if a Component is a tab (VTab or VGroup). */
    static public boolean isTab(Component obj) {
        if(isTabComp(obj) || isTabGroup(obj))
            return true;
        return false;
    }

    /** test if a Component is a VTab.*/
    static public boolean isTabComp(Component obj) {
        if(!(obj instanceof VObjIF))
            return false;
        String type = ((VObjIF)obj).getAttribute(TAB);
        if(type==null)
            return false;
        String label=((VObjIF)obj).getAttribute(LABEL);
        if(label==null)
            return false;
        label=label.trim();
        if(label.length()==0)
            return false;
        if(type.equals("yes")||type.equals("True")||type.equals("true"))
            return true;
        return false;
    }

    /** test if a Component is a tabgroup (VGroup with tab and label).*/
    static public boolean isTabGroup(Component obj) {
        if(!(obj instanceof VGroup))
            return false;
        if(isTabComp(obj))
            return true;
        return false;
    }

    /** return true if comp is the currently selected tab group.*/
    public boolean isSelectedTabGroup(Component comp) {
         if(comp==seltab && isTabGroup(seltab))
             return true;
         return false;
    }

    /** rebuild the tab lists from the JComponent tree.*/
    public void rebuildTabs() {
        buildTabs();
        buildTabMenu();
    }

    /** set preferred tab for panel.*/
    public void setTabLabel(String s){
        selLabel=s;
    }

    /** set the tab to that last selected (or first if last not valid).*/
    public void setLastSelectedTab() {
        Component comp=findTab(selLabel);
        if(comp == null){
            setFirstTab();
            return;
        }
        int index=groups.indexOf(comp);
        if(index>=0){  // already selected
            setDebug(new StringBuffer().append("setLastSelectedTab(").append(index).
                     append(")").toString());
            VGroup grp=(VGroup)comp;
            tabList.setSelectedValue(selLabel,true);
            if(editMode){
                grp.showGroup(true);
                grp.setEditMode(true);
                ParamEditUtil.setEditObj(grp);
            }
        }
        else{
            setDebug(new StringBuffer().append("setLastSelectedTab(").append(selLabel).
                     append(")").toString());
            setSelected(comp);
        }
    }

    /** move the position of a tab in the tab list.*/
    public void moveTab(String s, int i){
        Component comp=findTab(s);
        if(comp==null)
            return;
        int index=groups.indexOf(comp);
        if(index>=0 && i<groups.size()){
            ParamInfo.setItemEdited(this);
            groups.remove(index);
            groups.add(i,comp);
            buildTabMenu();
        }
    }

    /** build the tab lists.*/
    public void buildTabs() {
        int i,j;

        freeLists();
        int nLength = getComponentCount();
        for (i = 0; i < nLength; i++) {
            Component comp = getComponent(i);
            if(isTabGroup(comp)){
                groups.add(comp);
                comp.setVisible(false);
            }
            else if(isTabComp(comp))
                tabs.add(comp);
            else if(comp instanceof VGroupIF){
                JComponent grp = (JComponent)comp;
                int nLength2 = grp.getComponentCount();
                for (j = 0; j < nLength2; j++) {
                    comp = grp.getComponent(j);
                    if(isTabComp(comp))
                        tabs.add(comp);
                }
            }
        }

        // reorder old-style tabs based on x position

        Vector<Point> plist=new Vector<Point>();
        Vector<Component>tlist=new Vector<Component>();
        int nLength2 = tabs.size();
        for (i = 0; i < nLength2; i++) {
            Component comp = (Component) tabs.elementAt(i);
            Point pt = getRelativeLoc(comp);
            j = 0;
            int nLength3 = plist.size();
            while ( j < nLength3) {
                Point pt2 = plist.elementAt(j);
                if (pt.x < pt2.x)
                    break;
                if (pt.x == pt2.x && pt.y < pt2.y)
                    break;
                j++;
            }
            plist.add(j, pt);
            tlist.add(j, comp);
        }
        tabs.removeAllElements();
        tabs.addAll(tlist);
    }

    private boolean changedTabMenu() {
        int i;
        int nLength = getComponentCount();
        for (i = 0; i < nLength; i++) {
            Component comp = getComponent(i);
            int index=groups.indexOf(comp);
            if(index!=i)
                return true;
        }
        return false;
    }

    /** build the tab menu from the tab lists.*/
    public void buildTabMenu() {
        int i;
        if(changeMode){
            Messages.postError("RE-ENTRY buildTabMenu");
            return;
        }

        changeMode=true;
        clearMenuList();
        if(groups.size()>0){
            if(changedTabMenu()){
                setDebug("buildTabMenu <changed>");

                // reorder JComponent tree to mirror order of groups Vector
                for (i = 0; i <  getComponentCount(); i++) {
                    Component comp = getComponent(i);
                    if(groups.contains(comp))
                        remove(i);
                }
                int nLength = groups.size();
                for (i = 0; i < nLength; i++) {
                    VGroup comp = (VGroup)groups.elementAt(i);
                    add(comp);
                    if(comp.showVal==null || comp.showing || editMode)
                        addToMenuList(comp);
                }
          }
          else{
                setDebug("buildTabMenu <unchanged>");
                int nLength = groups.size();
                for (i = 0; i < nLength; i++) {
                    VGroup comp = (VGroup)groups.elementAt(i);
                    if(comp.showVal==null || comp.showing || editMode)
                        addToMenuList(comp);
                }
            }
        }
        else
            setDebug("buildTabMenu <initial>");
        int nLength = tabs.size();
        for (i = 0; i < nLength; i++) {
            Component comp = (Component) tabs.elementAt(i);
            addToMenuList(comp);
        }
        changeMode=false;
    if (paramPanel != null)
        paramPanel.revalidate();
    }

    /** add a component by parsing an XML file.*/
    public void pasteXmlObj(String file, int x, int y) {
        JComponent comp=null;
        try{
            comp=LayoutBuilder.build(panel, VnmrIf, file, x, y);
        }
        catch(Exception be) {
            Messages.writeStackTrace(be);
        }
        if ((comp == null) || !(comp instanceof VObjIF))
            return;
        comp.setVisible(true);
        VObjIF vobj = (VObjIF) comp;
        Point pt = getLocationOnScreen();
        x = pt.x + x;
        y = pt.y + y;

        ParamEditUtil.setEditObj(vobj);
        vobj.setEditMode(true);
        comp.setLocation(x, y);
        ParamEditUtil.relocateObj(vobj);
        if (isTab(comp)) {
	        rebuildTabs();
	        setSelected(comp);
        }
    }

    /** reload a reference group.*/
    public VObjIF  reloadObject(VObjIF obj, String file, boolean expand_page,boolean select) {
        if(!(obj instanceof VGroup) && !(obj instanceof VTabbedPane))
            return null;
        JComponent comp=(JComponent)obj;
        
        int index=groups.indexOf(comp);
        if(!expand_page && index>=0){ // pages are always references
            seltab=null;
            comp.removeAll();
            obj.setAttribute(USEREF,"yes");
            obj.setAttribute(EXPANDED,"no");
            setSelected(comp);
            return obj;
        }
        
        obj.setAttribute(USEREF,"yes");
        obj.setAttribute(EXPANDED,"no");

        JComponent parent=(JComponent)comp.getParent();
        if(parent==null){
            Messages.postError(new StringBuffer().append("error reloading ").
                               append(file).toString());
            return null;
        }

        index=-1;
        int nLength = parent.getComponentCount();
        for(int i=0;i<nLength;i++){
            Component child=parent.getComponent(i);
            if(child==comp){
                index=i;
                break;
            }
        }
        if(index<0){
            Messages.postError(new StringBuffer().append("error reloading ").
                               append(file).toString());
            return null;
        }
        JComponent ref=null;
        int x=0;
        int y=0;
        boolean showing=false;
        if(comp.isShowing() && !isTabGroup(comp)){
            Point loc=comp.getLocation();
            Point pt = getLocationOnScreen();
            x = pt.x + loc.x;
            y = pt.y + loc.y;
            showing=true;
        }
        JPanel pnl=new JPanel();
        try{
            LayoutBuilder.build(pnl, VnmrIf, file);
        }
        catch(Exception be) {
            Messages.postError(new StringBuffer().append("error reloading ").
                               append(file).toString());
            Messages.writeStackTrace(be);
            return null;
        }
        setDebug(new StringBuffer().append("reloadObject(").append(file).
                 append(")").toString());

        ref=(JComponent)pnl.getComponent(0);
        if ((ref == null) || !(ref instanceof VObjIF)){
            Messages.postError(new StringBuffer().append("error reloading ").
                               append(file).toString());
            return null;
        }

        parent.remove(index);
        obj.destroy();
        VObjIF vobj = (VObjIF)ref;
        parent.add(ref,index);
        if(showing && editMode)
            ref.setVisible(true);
        vobj.setAttribute(USEREF,"yes");
        vobj.setEditMode(editMode);
        ref.setLocation(x, y);
        if(!editMode) {
            vobj.updateValue();
        }
        if(showing && select){
            ParamEditUtil.setEditObj(vobj);
            ParamEditUtil.relocateObj(vobj);
        }
        if (isTab(ref)){
            rebuildTabs();
            if(showing && select);
                setSelected(ref);
        }
        return vobj;
    }

    // VObjIF interface

    public void setEditMode(boolean s) {
        setDebug(new StringBuffer().append("setEditMode(").append(s).append(")").toString());
        boolean echange=(s!=editMode);
        paramsLayout.setEditMode(s);
        editMode = s;
        if (editMode) {
            setSnapGap(ParamEditUtil.getSnapGap());
            setGridColor(Util.getGridColor());
            addMouseListener(ml);
            changeOpaque();
        }
        else{
            ParamEditUtil.setEditObj(null);
            removeMouseListener(ml);
            setOpaque(true);
        }
        if(echange){
            buildTabMenu();
            setLastSelectedTab();
            setEditModeToAll(editMode);
            if(!editMode)
                updateAllValue();
        }
    }

    /* (non-Javadoc)
     * @see vnmr.ui.ParamPanel#dropAction(javax.swing.JComponent, int, int)
     */
    public void dropAction(JComponent comp,int x,int y){   
        if ((comp == null) || !(comp instanceof VObjIF))
            return;
        VObjIF vobj = (VObjIF) comp;

        if (isTabGroup(comp)) {
            comp.setLocation(0, 0);
            rebuildTabs();
            setSelected(comp); // calls setEditObj
        } else {
            comp.setLocation(x, y);
            ParamEditUtil.setEditObj(vobj);
            ParamEditUtil.relocateObj(vobj);
            if (isTab(comp)) { // old style tab
                rebuildTabs();
                setSelected(comp);
            }
        }
    }

} // class ParamLayout
