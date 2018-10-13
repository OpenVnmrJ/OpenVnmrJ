/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.beans.*;

import javax.swing.*;

import vnmr.util.*;
import vnmr.bo.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;

/**
 * The pane containing acquire/process panels. Because this pane is
 * has features beyond the typical tabbed pane, it implements
 * its own tabbed-pane-like behavior, rather than subclassing
 * JTabbedPane.
 *
 */
public class ControlPanel extends JPanel
    implements VObjDef, StatusListenerIF, PropertyChangeListener,EditListenerIF
{
    public static final int RIGHTPAD = 15;
    /** button bar */
    private ControlButtonBar controlButtonBar;
    /** content panel (contains actual acquire panel or process panel) */
    private ControlContent controlContent;

    private boolean isEditing;
    private SessionShare sshare;
    /** current panel name can */
    private String panelName = null;
    private String panelType = null;
    private ParameterPanel curPanel = null;
    private ActionBar curAction = null;
    private JComponent tabArea;
    private JComponent actionArea;
    private ButtonIF  vnmrIf = null;
    private Vector<JComponent> tabVector = null;
    private VPanelTab activeTab = null;
    private ActionListener tabAction;
    private boolean bTabReady = false;
    private boolean bObsolete = true; 

    private ParameterTabControl paramTabCtrl = null;

    protected static final String m_strShow = "action show ";
    protected static final String m_strHide = "action hide ";


    /**
     * constructor
     * @param sshare session share
     */
    public ControlPanel(SessionShare sshare) {
        this.sshare = sshare;
        setBorder(BorderFactory.createEmptyBorder(2, 2, 2, 2));

        setOpaque(true);

        this.bObsolete = true;        

        buildUi();

        DisplayOptions.addChangeListener(this);
        ParamInfo.addEditListener(this);
    } // ControlPanel()

    private void buildUi() {

        if (bObsolete) { // moved to ParameterTabControl
            setLayout(new BorderLayout());
            return;
        }

        setLayout(new ControlPanelLayout());
        controlButtonBar = new ControlButtonBar(sshare);
        add(controlButtonBar, BorderLayout.NORTH);

        tabArea = controlButtonBar.getTabArea();
        actionArea = controlButtonBar.getActionArea();

        controlContent = new ControlContent(sshare);
        add(controlContent, BorderLayout.CENTER);

        tabAction = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                Component obj = (Component) evt.getSource();
                if (obj instanceof JButton) {
                   JButton but = (JButton) obj;
                   setDebug("button pressed "+but.getText());
                   switchTab(but.getText());
                }
            }
        };
        ExpPanel.addStatusListener(this); // register as a Infostat listener
    }

    public void setParameterTabControl(ParameterTabControl p) {
        if (!bObsolete)
           return;
        if (p == null || p == paramTabCtrl)
           return;
        if (paramTabCtrl != null)
           paramTabCtrl.setEditMode(false);
        removeAll();
        paramTabCtrl = p;
        paramTabCtrl.setEditMode(isEditing);
        add(p, BorderLayout.CENTER); 
        revalidate();
        repaint();
    }

    private void setDebug(String s){
        if(DebugOutput.isSetFor("ControlPanel")||DebugOutput.isSetFor("panels"))
             Messages.postDebug("ControlPanel "+s);
    }

    private void onShow(JComponent jc)
    {
        if (jc == null)
            return;
        try {
            Component comp=jc.getComponent(0);
            if((comp != null) && (comp instanceof VGroup)) {
                VGroup grp=(VGroup)comp;
                String cmd=grp.getAttribute(CMD);
                if(cmd !=null){
                    vnmrIf.sendVnmrCmd(grp, cmd);
                    setDebug(new StringBuffer().append(m_strShow).append(cmd).toString());
                }
            }
        }
        catch (ArrayIndexOutOfBoundsException e) {}
    }

    private void onHide(JComponent jc)
    {
        if (jc == null)
            return;
        try {
            Component comp=jc.getComponent(0);
            if((comp != null) && (comp instanceof VGroup)) {
                VGroup grp=(VGroup)comp;
                String cmd=grp.getAttribute(CMD2);
                if(cmd !=null){
                    vnmrIf.sendVnmrCmd(grp, cmd);
                    setDebug(new StringBuffer().append(m_strHide).append(cmd).toString());
               }
            }
        }
        catch (ArrayIndexOutOfBoundsException e) {}
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        // setBackground(Util.getBgColor());
    }


    public void freeObj() {
/*
        controlContent.dispose();
*/
        controlContent = null;
        controlButtonBar = null;
        curPanel = null;
        curAction = null;
    }


    public void setVnmrIf(ButtonIF vif) {
        vnmrIf = vif;
        if (tabVector == null)
            return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            VObjIF tab = (VObjIF) tabVector.elementAt(k);
            tab.setVnmrIF(vif);
        }
    }

    public void updateTabValue() {
        if (paramTabCtrl != null) {
            paramTabCtrl.updateTabValue();
            return;
        }
        if (tabVector == null)
            return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            VObjIF tab = (VObjIF) tabVector.elementAt(k);
            tab.updateValue();
        }
    }

    private void updateObj(VObjIF obj, Vector<String> v) {
        String d;
        String s = obj.getAttribute(VARIABLE);
        if (s == null)
            return;
        StringTokenizer tok = new StringTokenizer(s, " ,\n");
        int nLength = v.size();
        while (tok.hasMoreTokens()) {
            d = tok.nextToken();
            for (int k = 0; k < nLength; k++) {
               if (d.equals(v.elementAt(k))) {
                   obj.updateValue();
                   return;
               }
            }
        }
    }


    /** StatusListenerIF interface */
    public void updateStatus(String msg){
        if (paramTabCtrl != null) {
            paramTabCtrl.updateStatus(msg);
            return;
        }
        if (activeTab != null)
            activeTab.updateStatus(msg);
    }

    public void updateTabValue(Vector<String> v) {
        if (paramTabCtrl != null) {
            paramTabCtrl.updateTabValue(v);
            return;
        }
        if (tabVector == null)
            return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            JComponent obj = (JComponent) tabVector.elementAt(k);
            if (obj instanceof VPanelTab) {
               updateObj ((VObjIF)obj, v);
               int nSize = obj.getComponentCount();
               for (int i = 0; i < nSize; i++) {
                     JComponent obj2 = (JComponent) obj.getComponent(i);
                     if (obj2 instanceof VObjIF) {
                         updateObj ((VObjIF)obj2, v);
                     }
                }
            }
            else if(obj instanceof VObjIF) {
                updateObj((VObjIF)obj, v);
            }
        }
    }

    public void setParamPanel(String name, JComponent p) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setParamPanel(name, p);
            return;
        }
        VPanelTab tab;
        if (tabVector == null)
            return;
        AppIF apIf = Util.getAppIF();
        if (apIf != null)
            apIf.disableResize();
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.pname.equals(name)) {
                if (p != null) {
                    tab.needsNewParamPanel = false;
                    if (tab.paramPanel == p && p.isShowing())
                       break;
                }
                tab.paramPanel = p;
                if (name.equals(panelName)) {
                     switchPanel(name);
                }
                break;
            }
        }
        if (apIf != null)
            apIf.enableResize();
    }

    public void setActionPanel(String name, JComponent p)
    {
        setActionPanel(name, p, "");
    }

    public void setActionPanel(String name, JComponent p, String panel) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setActionPanel(name, p);
            return;
        }
        VPanelTab tab;
        if (tabVector == null)
            return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.pname.equals(name)) {
                tab.actionPanel = (ActionBar)p;
                if (name.equals(panelName))
                    switchAction (name, p, panel);
                break;
            }
        }
    }


    public void setParamPanel(int id, String name, JComponent p) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setParamPanel(id, name, p);
            return;
        }
        setParamPanel(name, p);
        if (name.equals(panelName) && p == null)
            vnmrIf.queryPanelInfo(name);
    }

    public String getPanelName() {
        if (paramTabCtrl != null) {
            return paramTabCtrl.getPanelName();
        }
        return panelName;
    }

    public void setPanelName(String name) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setPanelName(name);
            return;
        }
        VPanelTab tab;
        if (tabVector == null || name == null)
            return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.pname.equals(name)) {
                panelName = name;
                break;
            }
        }
    }

    public String getPanelType() {
        if (paramTabCtrl != null) {
            return paramTabCtrl.getPanelType();
        }
        return panelType;
    }

    public void setActionPanel(int id, String name, JComponent p) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setActionPanel(id, name, p);
            return;
        }
        setActionPanel(name, p);
    }

    public ParameterPanel getParamPanel(String name) {
        if (paramTabCtrl != null) {
            return paramTabCtrl.getParamPanel(name);
        }
        VPanelTab newTab = null;

        if (tabVector == null)
            return null;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            newTab = (VPanelTab) tabVector.elementAt(k);
            if (newTab.pname.equals(name))
                return (ParameterPanel) newTab.paramPanel;
        }
        return null;
    }

    public void enablePanel(boolean enable) {
        if (paramTabCtrl != null) {
            paramTabCtrl.enablePanel(enable);
            return;
        }
        setDebug("enablePanel("+enable+")");
        if(curPanel!=null){
            ParamLayout pl=curPanel.getParamLayout();
            if(pl !=null)
                pl.enablePanel(enable);
        }
    }

    public ParameterPanel getParamPanel() {
        if (paramTabCtrl != null) {
            return paramTabCtrl.getParamPanel();
        }
        return (ParameterPanel) curPanel;
    }

    public JComponent getActionPanel(String name) {
        if (paramTabCtrl != null) {
            return paramTabCtrl.getActionPanel(name);
        }
        VPanelTab newTab = null;

        if (tabVector == null)
            return null;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            newTab = (VPanelTab) tabVector.elementAt(k);
            if (newTab.pname.equals(name))
                return newTab.actionPanel;
        }
        return null;
    }

    public JComponent getActionPanel() {
        if (paramTabCtrl != null) {
            return paramTabCtrl.getActionPanel();
        }
        return curAction;
    }

    public boolean getEditMode() {
        return isEditing;
    }

    public void setEditMode(boolean s) {
        isEditing = s;

        if (s) {
             LocatorHistoryList lhl = sshare.getLocatorHistoryList();
             lhl.setLocatorHistory(LocatorHistoryList.EDIT_PANEL_LH);
        }
        if (paramTabCtrl != null) 
             paramTabCtrl.setEditMode(s);        
    }

    public void setParamPage(String name, String page) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setParamPage(name, page);
            return;
        }
        page=page.trim();
        if(page==null || page.length()==0){
            setDebug("setParamPage("+name+")");
            switchTab(name);
        }
        else{
            if (curPanel != null)
        	curPanel.setTabLabel(page);
            else
                setDebug("Error on setParamPage, panel is NULL ");
            setDebug("setParamPage("+name+","+page+")");
            switchTab(name,page);
        }
    }

    public void switchTab(String name) {
        if (paramTabCtrl != null) {
            paramTabCtrl.showTab(name);
            return;
        }
        switchTab(name,null);
    }

    public void switchTab(String name,String page) {
        if (paramTabCtrl != null) {
            paramTabCtrl.showTab(name);
            return;
        }
        VPanelTab newTab = null;
        if(page !=null && page.length()>0)
            setDebug("switchTab("+name+","+page+")");
        else
            setDebug("switchTab("+name+")");
        if (tabVector == null)
            return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            newTab = (VPanelTab) tabVector.elementAt(k);
            if (newTab.pname.equals(name))
                break;
            newTab = null;
        }
        if (newTab == null)
            return;
        newTab.setPanelTab(page);
        panelType=newTab.getAttribute(PANEL_TYPE);
        if (name.equals(panelName)) {
            if (newTab.equals(activeTab)) {
                newTab.setOpaque(false);
                newTab.setActive(true);
                if(page !=null && page.length()>0) {
                    if (curPanel != null)
                        curPanel.selectPage(page);
                }
                return;
            }
        }
        panelName = name;
        if (vnmrIf == null)
            return;

        if (activeTab != null) {
           activeTab.setOpaque(true);
           activeTab.setActive(false);
        }
        vnmrIf.queryPanelInfo(newTab.pname);
        if (newTab.paramPanel == null || newTab.needsNewParamPanel)
           return;
        switchPanel(name);
    }

    public void switchPanel(String name) {
        if (paramTabCtrl != null) {
            paramTabCtrl.showTab(name);
            return;
        }
        setDebug("switchPanel("+name+")");
        if(curPanel!=null)
            curPanel.hidePage();
        VPanelTab newTab = null;
        if (tabVector == null) return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            newTab = (VPanelTab) tabVector.elementAt(k);
            if (newTab.pname.equals(name))
                break;
            newTab = null;
        }
        if (newTab == null)
            return;
        panelName = name;
        panelType=newTab.getAttribute(PANEL_TYPE);

        if (activeTab != null) {
           activeTab.setOpaque(true);
           activeTab.setActive(false);
        }
        curPanel = (ParameterPanel) newTab.paramPanel;
        activeTab = newTab;

        String page=activeTab.getPanelTab();
        if(page !=null && page.length()>0) {
            if (curPanel != null)
                curPanel.setTabLabel(page);
        }

        switchAction(name, activeTab.actionPanel, "panel");
        controlContent.showPanel(name, curPanel);
        if (curPanel == null)
            return;
        activeTab.setOpaque(false);
        activeTab.setActive(true);
        ExpPanel.updateStatusListener(activeTab);

        if (activeTab.isShowing() && controlContent.isShowing()) {
            Rectangle rect = activeTab.getBounds();
            Point pt = activeTab.getLocationOnScreen();
            Point pt2 = controlContent.getLocationOnScreen();
            curPanel.setBorderGap(pt.x - pt2.x, rect.width);
            repaint();
        }
        ParamLayout pl=curPanel.getParamLayout();
		boolean echange=(pl.getEditStatus()==isEditing)?false:true;
        curPanel.setEditMode(isEditing);
        curPanel.showPage();
        if(!echange){
            pl.setLastSelectedTab();
        	curPanel.updateAllValue();
        }
        ExpPanel.updateStatusListener(curPanel);
    }

    public void switchAction(String name, JComponent pan, String panel) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setActionPanel(name, pan);
            return;
        }

        if (actionArea == null || pan == null)
           return;
        if(pan==curAction)
        {
            if (panel == null || !panel.equals("panel"))
                onShow(curAction);
            return;
        }
        if(curAction != null)
        	onHide(curAction);
        actionArea.removeAll();
        curAction = (ActionBar)pan;
        actionArea.add(pan);
        onShow(curAction);
        controlButtonBar.revalidate();
        actionArea.repaint();
    }

    public void setOutOfDateFlag(String name, boolean b) {
        if (paramTabCtrl != null) {
            paramTabCtrl.setOutOfDateFlag(name, b);
            return;
        }
        VPanelTab tab;
        if (tabVector == null) return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.pname.equals(name)) {
                tab.needsNewParamPanel = b;
                break;
            }
        }
    }

    public boolean havePanel(ParameterPanel p) {
        if (paramTabCtrl != null) {
            return paramTabCtrl.havePanel(p);
        }
        VPanelTab tab;
        if (tabVector == null) return false;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.paramPanel == p) {
                return true;
            }
        }
        return false;
    }

    public void buildTopButtons() {

        if (bObsolete)
            return;

        if (bTabReady)
            return;
        try {
            String path=FileUtil.openPath("INTERFACE/TopPanel.xml");
            LayoutBuilder.build(tabArea, vnmrIf, path);
        }
        catch(Exception e) {
             System.out.println(e);
             Messages.writeStackTrace(e);
             return;
        }
        int num = tabArea.getComponentCount();
        if (num < 1)
             return;
        bTabReady = true;
        if (tabVector != null)
             tabVector.clear();
        else
             tabVector = new Vector<JComponent>();
        int k;
        for (k = 0; k < num; k++) {
             JComponent comp = (JComponent) tabArea.getComponent(k);
             if (comp instanceof VPanelTab) {
                tabVector.add(comp);
                VPanelTab tab = (VPanelTab) comp;
                tab.setTabAction(tabAction);
                if (k == 0)
                    panelName = tab.pname;
             }
        }

        /* reorder tabArea, let the right most widget to be on top */
        tabArea.removeAll();
        for (k = num - 1; k >= 0; k--) {
             JComponent obj = (JComponent) tabVector.elementAt(k);
             tabArea.add(obj);
        }
        tabArea.revalidate();

        ExpViewArea ep = Util.getViewArea();
        if (ep != null) {
             ep.setTabPanels(tabVector);
        }
    }

    public Vector<JComponent> getPanelTabs() {
        if (paramTabCtrl != null) {
            return paramTabCtrl.getPanelTabs();
        }
        return tabVector;
    }

    /**
     * The layout manager for ControlPanel is a BorderLayout, but
     * lays out the TrashCan component.
     */
    class ControlPanelLayout extends BorderLayout {
        private boolean isReady = false;
        public void addLayoutComponent(Component comp, Object constraints) {
                super.addLayoutComponent(comp, constraints);
        } // addLayoutComponent()

        /**
         * do the layout as usual, but also layout the trash can
         */
        public void layoutContainer(Container target) {
           if (!isReady) {
                isReady = true;
                buildTopButtons();
                // tabArea.revalidate();
           }
/*
           if (!isShowing())
                return;
*/
           synchronized (target.getTreeLock()) {
                super.layoutContainer(target);
           }
        } // layoutContainer()

    } // class ControlPanelLayout

/**
 * This button bar includes the tabs and buttons that line
 * the top of the ControlPanel.
 */
class ControlContent extends JComponent {

        CardLayout cardLayout;
        // private SessionShare sshare;
    /**
     * constructor
     * @param sshare session share
     */
    public ControlContent(SessionShare sshare) {
        cardLayout = new CardLayout();
        setLayout(cardLayout);
        // this.sshare = sshare;
    } // ControlContent()


    void showPanel(String name, JComponent panel) {
        removeAll();
        if (panel == null)
            return;
        add(name, panel);
        cardLayout.show(this, name);
        revalidate();
    }

    void dispose() {
        cardLayout = null;
    }

} // class ControlContent
} // class ControlPanel
