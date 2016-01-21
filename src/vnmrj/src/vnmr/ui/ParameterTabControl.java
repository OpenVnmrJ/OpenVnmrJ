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
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.*;

import vnmr.util.*;
import vnmr.bo.*;
import vnmr.templates.*;

/**
 * This panel contains parameter panels.
 *
 */
public class ParameterTabControl extends JLayeredPane
    implements VObjDef, StatusListenerIF, PropertyChangeListener
{
    private boolean isEditing;
    private String panelName = null;
    private String xmlFile = null;
    private ParameterPanel activePanel = null;
    private JComponent activeAction = null;
    private ButtonIF  vnmrIf = null;
    private Vector<JComponent>    tabVector = null;
    private VPanelTab activeTab = null;
    private boolean bExpActive = false;
    private int winId;
    protected static int  LAYER0 = JLayeredPane.DEFAULT_LAYER.intValue();
    protected static int  LAYER1 = LAYER0 + 10;

    private JTabbedPane tabbedPane;
    private JPanel    actionPane;

    private VpPropertyListener pListener;
    private ComponentAdapter compAdapter;
    protected static final String m_strShow = "action show ";
    protected static final String m_strHide = "action hide ";

    /**
     * constructor
     * @param sshare session share
     */
    public ParameterTabControl(int id, SessionShare sshare, ButtonIF exp) {
        this.vnmrIf = exp;
        this.winId = id;
        setBorder(BorderFactory.createEmptyBorder(2, 2, 2, 2));
        setLayout(new ControlLayout());

        tabbedPane = new JTabbedPane();
        add(tabbedPane, new Integer(LAYER0));
        tabbedPane.setSize(800, 500);

        String appType = "";
        if (sshare != null)
            appType = sshare.user().getAppType();
        actionPane = new JPanel();
        actionPane.setSize(80, 24);
        actionPane.setLayout(new BorderLayout());
        actionPane.putClientProperty(VnmrjUiNames.PanelTexture, "no");
        if (Global.WALKUPIF.equals(appType))
            add(actionPane, new Integer(LAYER1));

        tabbedPane.addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent e) {
                tabChanged();
            }
        });

        compAdapter = new ComponentAdapter() {
            public void componentShown(ComponentEvent evt) {
                VPanelTab vtab = (VPanelTab) evt.getSource();
                updateTabVisible(vtab);
            }

            public void componentHidden(ComponentEvent evt) {
                VPanelTab vtab = (VPanelTab) evt.getSource();
                updateTabVisible(vtab);
            }
        };

        pListener = new VpPropertyListener();

        buildTopButtons();

        ExpPanel.addStatusListener(this); // register as a Infostat listener
        DisplayOptions.addChangeListener(this);

    } // ParameterTabControl()

    public void setExpActive(boolean b) {
         boolean bUpdate = false;
         if (!bExpActive)
             bUpdate = true;
         bExpActive = b;
         if (b) {
             vnmrIf.queryPanelInfo(panelName);
             updatePanel(bUpdate);
             updateAction();
         }
    }

    private void setDebug(String s){
        if(DebugOutput.isSetFor("ControlPanel")||DebugOutput.isSetFor("panels"))
             Messages.postDebug("ControlPanel "+s);
    }

    private void onShow(JComponent jc)
    {
        if (jc == null)
            return;
        if (!bExpActive)
            return;

        try {
            Component comp = jc.getComponent(0);
            if((comp != null) && (comp instanceof VGroup)) {
                VGroup grp = (VGroup)comp;
                String cmd = grp.getAttribute(CMD);
                if (cmd != null) {
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
        if (!bExpActive)
            return;

        try {
            Component comp = jc.getComponent(0);
            if((comp != null) && (comp instanceof VGroup)) {
                VGroup grp = (VGroup)comp;
                String cmd = grp.getAttribute(CMD2);
                if (cmd != null) {
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
        activePanel = null;
        activeAction = null;
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
        if (tabVector == null)
            return;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            VObjIF tab = (VObjIF) tabVector.elementAt(k);
            tab.updateValue();
        }
    }

    private void updateObj(VObjIF obj, Vector<?> v) {
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

    private void setTabVisible(VPanelTab vtab) {
        if (vtab == null)
            return;
        int index = tabbedPane.getSelectedIndex();
        int num = tabbedPane.getTabCount();
        int i, k;
        String tabName;
      
        for (k = 0; k < num; k++) {
            tabName = tabbedPane.getTitleAt(k);
            if (vtab.pname.equals(tabName)) {
                if (!vtab.isVisible()) {
                   tabbedPane.remove(k);
                   if (index == k && num > 1)
                       tabbedPane.setSelectedIndex(0);
                   tabbedPane.revalidate();
                }
                return;
            }
        }

        int nLength = tabVector.size();
        VPanelTab otherTab;
        index = -1;

        for (k = 0; k < num; k++) {
            tabName = tabbedPane.getTitleAt(k);
            otherTab = null;
            for (i = 0; i < nLength; i++) {
                otherTab = (VPanelTab) tabVector.elementAt(i);
                if (otherTab.pname.equals(tabName))
                    break;
                otherTab = null;
            }
            if (otherTab != null) {
                if (otherTab.tabId > vtab.tabId) {
                    index = k;
                    break;
                }
            }
        }
        if (index >= 0)
            tabbedPane.insertTab(vtab.pname, null, vtab.tabPanel, null, index);
        else
            tabbedPane.addTab(vtab.pname, vtab.tabPanel);
    }

    private void setTabEnabled(VPanelTab vtab) {
        if (vtab == null)
            return;
        int index = tabbedPane.getSelectedIndex();
        int num = tabbedPane.getTabCount();
        for (int k = 0; k < num; k++) {
            String tabName = tabbedPane.getTitleAt(k);
            if (vtab.pname.equals(tabName)) {
                tabbedPane.setEnabledAt(k, vtab.isEnabled());
                if (!vtab.isEnabled()) {
                    if (index == k)
                        tabbedPane.setSelectedIndex(0); 
                }
                return;
            }
        }
    }

    private void updateTabVisible(VPanelTab vpTab) {
        if (vpTab == null)
            return;
        setTabVisible(vpTab);
        if (vpTab.isVisible())
            setTabEnabled(vpTab);
    }

    /** StatusListenerIF interface */
    public void updateStatus(String msg){
        if (activeTab != null)
            activeTab.updateStatus(msg);
    }

    public void updateTabValue(Vector<?> v) {
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

    private void updatePanel(boolean bForced) {
        if (!bExpActive)
            return;
        if (tabVector == null)
            return;

        if (tabbedPane.getTabCount() < 1)
            return;
        int index = tabbedPane.getSelectedIndex();
        if (index < 0)
            return;
        String tabName = tabbedPane.getTitleAt(index);

        VPanelTab vpTab = null;
        int nLength = tabVector.size();
        for (index = 0; index < nLength; index++) {
            vpTab = (VPanelTab) tabVector.elementAt(index);
            if (vpTab.pname.equals(tabName))
                break;
            vpTab = null;
        }
        if (vpTab == null)
            return;

        if (vpTab.paramPanel == null || vpTab.needsNewParamPanel)
            return;
        if (!bForced) {
            if (vpTab.paramPanel == activePanel)
                return;
        }
 
        if (activePanel != null) {
            activePanel.setEditMode(false);
            activePanel.hidePage();
        }

        activePanel = (ParameterPanel) vpTab.paramPanel;
        activeTab = vpTab;

        String pageName = activeTab.getPanelTab();
        if (pageName != null && pageName.length()>0)
            activePanel.setTabLabel(pageName);

        ExpPanel.updateStatusListener(activeTab);

        ParamLayout pl = activePanel.getParamLayout();
        boolean echange = (pl.getEditStatus() == isEditing)?false:true;
        
        activeTab.setEditMode(isEditing);
       // activePanel.setEditMode(isEditing);
        activePanel.showPage();

        if (!echange){
            pl.setLastSelectedTab();
            activePanel.updateAllValue();
        }
        else if (bForced)
            activePanel.updateAllValue();
        ExpPanel.updateStatusListener(activePanel);
    }

    private void updatePanel() {
        updatePanel(false);
    }

    private void updateAction() {
        if (!bExpActive)
            return;

        if (tabbedPane.getTabCount() < 1)
            return;
        int k;
        int index = tabbedPane.getSelectedIndex();
        if (index < 0)
            return;
        String tabName = tabbedPane.getTitleAt(index);

        VPanelTab vpTab = null;
        int nLength = tabVector.size();
        for (k = 0; k < nLength; k++) {
            vpTab = (VPanelTab) tabVector.elementAt(k);
            if (vpTab.pname.equals(tabName))
                break;
            vpTab = null;
        }
        if (vpTab == null)
            return;
        if (vpTab.actionPanel == null)
            return;
        vpTab.actionPanel.setEditMode(isEditing);

        if (vpTab.actionPanel == activeAction)
            return;
 
        if (activeAction != null)
            onHide(activeAction);

        actionPane.removeAll();
        activeAction = vpTab.actionPanel;
        actionPane.add(activeAction, BorderLayout.CENTER);
        actionPane.revalidate();
        activeAction.repaint();

        onShow(activeAction);
    }

    private void selectTab(String name) {
        if (name == null)
            return;
        int num = tabbedPane.getTabCount();
        int index = -1;
        for (int k = 0; k < num; k++) {
            String tabName = tabbedPane.getTitleAt(k);
            if (name.equals(tabName)) {
                 index = k;
                 break;
            }
        }
        if (index >= 0)
            tabbedPane.setSelectedIndex(index);
    }


    public void setParamPanel(String name, JComponent paramPane, String filePath)
    {
        if (tabVector == null || name == null)
            return;
        
        VPanelTab vtab = null;
        JPanel tabPane = null;
        int  k, nLength;
        boolean bNewPanel = false;

        nLength = tabVector.size();
        for (k = 0; k < nLength; k++) {
            vtab = (VPanelTab) tabVector.elementAt(k);
            if (vtab.pname.equals(name)) {
                if (paramPane != null) {
                    if (paramPane != vtab.paramPanel)
                        bNewPanel = true;
                    vtab.needsNewParamPanel = false;
                }
                else
                    vtab.needsNewParamPanel = true;
                vtab.paramPanel = paramPane;
                break;
            }
            vtab = null;
        }
        if (bNewPanel && vtab != null) {
            nLength = tabbedPane.getTabCount();
            for (k = 0; k < nLength; k++) {
                String tabName = tabbedPane.getTitleAt(k);
                if (name.equals(tabName)) {
                     tabPane = (JPanel) tabbedPane.getComponentAt(k);
                     if (tabPane == null) {
                         tabPane = new JPanel();
                         tabPane.setLayout(new BorderLayout());
                         tabbedPane.insertTab(vtab.pname, null, tabPane, filePath, k);
                         vtab.tabPanel = tabPane;
                     }
                     // else
                     //     tabbedPane.setToolTipTextAt(k, filePath);
                     break;
                }
            }
            tabPane = vtab.tabPanel;
            if (tabPane != null) {
                tabPane.removeAll();
                if (paramPane != null) {
                    tabPane.add(paramPane);
                    tabPane.revalidate();
                    paramPane.repaint();
                }
            }
        }
        if (name.equals(panelName)) {
            if (vtab != null && vtab.needsNewParamPanel)
                vnmrIf.queryPanelInfo(name);
            updatePanel();
        }
    }

    public void setParamPanel(String name, JComponent paramPane) {
         setParamPanel(name, paramPane, null);
    }

    public void setActionPanel(String name, JComponent p)
    {
        setActionPanel(name, p, null);
    }

    public void setActionPanel(String name, JComponent p, String filePath) {
        if (tabVector == null || name == null)
            return;
        VPanelTab tab;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.pname.equals(name)) {
                tab.actionPanel = (ActionBar)p;
                // p.setToolTipText(filePath);
                break;
            }
        }
        if (name.equals(panelName))
            updateAction(); 
    }


    public void setParamPanel(int id, String name, JComponent p) {
        if (id != winId)
            return;
        setParamPanel(name, p, null);
    }

    public String getPanelName() {
        return panelName;
    }

    public void setPanelName(String name) {
        if (tabVector == null || name == null)
            return;
        selectTab(name);
    }

    public String getPanelType() {
        if (activeTab != null)
            return  activeTab.getAttribute(PANEL_TYPE);
        return null;
    }

    public void setActionPanel(int id, String name, JComponent p) {
        if (id != winId)
            return;
        setActionPanel(name, p, null);
    }

    public ParameterPanel getParamPanel(String name) {
        VPanelTab tab = null;

        if (tabVector == null)
            return null;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.pname.equals(name))
                return (ParameterPanel) tab.paramPanel;
        }
        return null;
    }

    public void enablePanel(boolean enable) {
        setDebug("enablePanel("+enable+")");
        if (activePanel != null) {
            ParamLayout pl = activePanel.getParamLayout();
            if(pl != null)
                pl.enablePanel(enable);
        }
    }

    public ParameterPanel getParamPanel() {
        return getParamPanel(panelName);
    }

    public JComponent getActionPanel(String name) {
        if (tabVector == null)
            return null;

        VPanelTab tab = null;
        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (tab.pname.equals(name))
                return tab.actionPanel;
        }
        return null;
    }

    public JComponent getActionPanel() {
        return getActionPanel(panelName);
    }

    public boolean getEditMode() {
        return isEditing;
    }

    public void setEditMode(boolean s) {
        isEditing = s;
        if (activeTab != null)
            activeTab.setEditMode(s);
        //if (activePanel != null)
        //   activePanel.setEditMode(s);
    }

    public void setParamPage(String name, String page) {
        if (page != null)
            page = page.trim();

        showTab(name, page);
    }

    public void showTab(String name) {
        showTab(name, null);
    }

    public void showTab(String name, String pageName) {
        if (tabVector == null)
            return;
        if (name == null || name.length() < 1)
            return;

        VPanelTab newTab = null;
        if (pageName !=null && pageName.length()>0)
            setDebug("showTab("+name+","+pageName+")");
        else
            setDebug("showTab("+name+")");

        int nLength = tabVector.size();
        for (int k = 0; k < nLength; k++) {
            newTab = (VPanelTab) tabVector.elementAt(k);
            if (newTab.pname.equals(name))
                break;
            newTab = null;
        }
        if (newTab == null)
            return;

        newTab.setPanelTab(pageName);

        if (pageName != null && pageName.length() > 0) {
            ParameterPanel panel = (ParameterPanel) newTab.paramPanel;
            if (panel != null)
                 panel.selectPage(pageName);
        }
        selectTab(name);
    }

    private void tabChanged() {
        if (tabbedPane.getTabCount() > 0) {
            int index = tabbedPane.getSelectedIndex();
            if (index < 0)
               return;
            panelName = tabbedPane.getTitleAt(index);
            vnmrIf.queryPanelInfo(panelName);
            updatePanel();
            updateAction();
        }
    }

    public void setOutOfDateFlag(String name, boolean b) {
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
        if (tabVector == null || p == null)
             return false;
        VPanelTab tab;
        int nLength = tabVector.size();

        for (int k = 0; k < nLength; k++) {
            tab = (VPanelTab) tabVector.elementAt(k);
            if (p == tab.paramPanel)
                return true;
        }
        return false;
    }

    public void buildTopButtons() {
        ExpPanel exp = null;

        if (vnmrIf != null && (vnmrIf instanceof ExpPanel))
            exp = (ExpPanel) vnmrIf;
        String path = FileUtil.openPath("INTERFACE/TopPanel.xml");
        if (path == null) {
            Messages.postError("Could not find file TopPanel.xml.");
            return;
        }
        if (xmlFile != null) {
            if (path.equals(xmlFile)) {
                if (exp != null)
                    exp.setTabPanels(tabVector);
                return;
            }
        }

        JPanel tabArea = new JPanel();
        try {
            LayoutBuilder.build(tabArea, vnmrIf, path);
        }
        catch(Exception e) {
             System.out.println(e);
             Messages.writeStackTrace(e);
             return;
        }
        int num = tabArea.getComponentCount();
        if (num < 1) {
            Messages.postError("There is no panel defined in "+path);
            return;
        }
        tabbedPane.removeAll();
        if (tabVector != null)
             tabVector.clear();
        else
             tabVector = new Vector<JComponent>();
        int k;
        int count = 0;
        for (k = 0; k < num; k++) {
             JComponent comp = (JComponent) tabArea.getComponent(k);
             if (comp instanceof VPanelTab) {
                tabVector.add(comp);
                VPanelTab vtab = (VPanelTab) comp;
                if (panelName == null)
                    panelName = vtab.pname;
                JPanel pan = new JPanel(new BorderLayout());
                pan.setBorder(null);
                tabbedPane.addTab(vtab.pname, pan);
                vtab.tabPanel = pan;
                vtab.tabId = k;
                vtab.addComponentListener(compAdapter);
                vtab.addPropertyChangeListener(pListener);
                count++;
             }
        }
        if (count < 1) {
            Messages.postError("There is no panel defined in "+path);
            return;
        }

        xmlFile = path;
        if (exp != null)
            exp.setTabPanels(tabVector);

        /*********
        ExpViewArea ep = Util.getViewArea();
        if (ep != null) {
             ep.setTabPanels(tabVector);
        }
        *********/

        selectTab(panelName);
    }

    public Vector<JComponent> getPanelTabs() {
        return tabVector;
    }

    /**
     * The layout manager for ControlPanel is a BorderLayout, but
     * lays out the TrashCan component.
     */
    private class ControlLayout implements LayoutManager {
        private int minX = 0;

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
               int tx, ty, th;
               Dimension targetSize = target.getSize();
               Insets insets = target.getInsets();
               int width = targetSize.width - insets.right - insets.left;
               int height = targetSize.height - insets.top;

               if (targetSize.width < 40 || targetSize.height < 40)
                     return;
               tx = 0;
               th = 20;
               int num = tabbedPane.getTabCount();
               for (int k = 0; k < num; k++) {
                    Rectangle r = tabbedPane.getBoundsAt(k);
                    if (r.width < 2)
                        r.width = 80;
                    if (r.height < 2)
                        r.height = 25;
                   
                    if (tx < (r.x + r.width))
                        tx = r.x + r.width;
                    if (th < (r.y + r.height))
                        th = r.y + r.height;
                    if (k == 2)
                        minX = tx + 10;
               }
               if (tx > (width - 20))
                    tx = width - 20;
               if (th > 50)
                    th = 50;
               Dimension da = actionPane.getPreferredSize(); 
               if (da == null)
                    da = new Dimension(200, 30);
               if (da.height > 50)
                    da.height = 50;
               else if (da.height < 15)
                    da.height = 15;
               ty = th - (da.height + 4);
               if (ty >= 0)
                   ty = 0;
               else if (ty < 0)
                   ty = 0 - ty;

               tabbedPane.setBounds(new Rectangle(0, ty, width, height - ty));
 
               th = width - (tx + da.width);
               if (th > 10)
                   tx = tx + 10;
               else
                   tx = tx + 4;
               if (tx < minX)
                   tx = minX;
               actionPane.setBounds( new Rectangle(tx, 0, width - tx, da.height));
           }
        } 
    } // class ControlLayout

    private class VpPropertyListener implements PropertyChangeListener {
        public void propertyChange(PropertyChangeEvent e) {
            VPanelTab vtab = (VPanelTab) e.getSource();
            updateTabVisible(vtab);
        }
    }

} // class ParameterTabControl
