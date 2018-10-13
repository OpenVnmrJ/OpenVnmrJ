/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.File;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.*;

import java.lang.reflect.Constructor;
import java.util.*;

import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.border.Border;
import javax.swing.plaf.basic.BasicSplitPaneDivider;
import javax.swing.plaf.basic.BasicSplitPaneUI;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.AttributesImpl;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.ui.shuf.*;
import vnmr.util.*;
import vnmr.bo.*;


public class VTabbedToolPanel extends PushpinPanel implements EditListenerIF{

    AppIF        appIF;
    Hashtable    panes                = new Hashtable();
    Dimension    zeroDim              = new Dimension(0, 0);
    Hashtable    vobjs                = new Hashtable();
    SessionShare sshare;
    JPanel       tabbedToolPanel;
    JTabbedPane  tabbedPane           = null;
    PushpinPanel pinPanel             = null;
    String       panelName;
    String       selectedTabName;
    String       buildFile            = null;                   // the source
                                                                 // of xml file
    long         dateOfbuildFile      = 0;
    Component    popupComp            = null;
    PushpinPanel locatorPanel         = null;
    ArrayList    toolList             = new ArrayList();
    ArrayList    objList              = new ArrayList();

    int          maxViews             = 9;
    int          nviews               = 0;
    Hashtable    tp_paneInfo[]        = new Hashtable[maxViews];
    ArrayList    keys                 = new ArrayList();
    ArrayList    vpInfo               = new ArrayList();

    int          vpId                 = 0;
    int          layoutId             = 99;
    int          previous_selectedTab = 0;
    public int   tp_selectedTab       = 0;
    boolean      bSwitching           = false;
    boolean      bChangeTool          = false;
    boolean      initVp               = false;
    boolean      inEditMode           = false;

    public VTabbedToolPanel(SessionShare sshare, AppIF appIF) {
        // super( new BorderLayout() );
        this.sshare = sshare;
        this.appIF = appIF;
        this.tabbedToolPanel = new JPanel();
        this.pinPanel = this;
        this.selectedTabName = null;
        setPinObj(this.tabbedToolPanel);
        this.tabbedToolPanel.setLayout(new BorderLayout());
        this.tabbedPane = new JTabbedPane();
        panelName = "Tab Panel";
        setTitle(panelName);
        setName(panelName);

        tabbedPane.addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent e) {
                tabChanged();
                /**** the following was moved to tabChanged()
		if(tabbedPane.getTabCount() > 1 &&
		   Util.getRQPanel() != null) {
		   int ind = tabbedPane.getSelectedIndex();
		   if(ind >= 0 && ind < tabbedPane.getTabCount())
		      Util.getRQPanel().updatePopup(tabbedPane.getTitleAt(ind));
		}
                 ***********/
            }
        });

        // Add Mouse Listener for CSH
        MouseAdapter ml = new CSHMouseAdapter();
        tabbedPane.addMouseListener(ml);

        Object obj = sshare.userInfo().get("canvasnum");	
        if(obj != null) {
            Dimension dim = (Dimension)obj;
            nviews = (dim.height)*(dim.width);
        } else nviews = 1;

        for(int i=0; i<nviews; i++)
            tp_paneInfo[i] = new Hashtable();

        /*
	obj = sshare.userInfo().get("activeWin");	
	if(obj != null) {
            vpId = ((Integer)obj).intValue();
	} else vpId = 0;
         */

        //System.out.println("VToolPanel nviews vpId "+nviews+" "+vpId);

        fillHashtable();
        Util.setVTabbedToolPanel(this);
        ParamInfo.addEditListener(this);
    }

    private void fillHashtable() {
        addTool("Locator",              vnmr.ui.VToolPanel.class);
        addTool("XMLToolPanel",         vnmr.ui.XMLToolPanel.class);
    }

    private void addTool(String key, Class tool) {
        Class[] constructArgs = null;
        if(key.equals("XMLToolPanel")) {
            constructArgs = new  Class[3];
            constructArgs[0] = vnmr.ui.SessionShare.class;
            constructArgs[1] = String.class;
            constructArgs[2] = String.class;
        } else {
            constructArgs = new  Class[1];
            constructArgs[0] = vnmr.ui.SessionShare.class;
        } 

        try {
            Constructor c = tool.getConstructor(constructArgs);
            vobjs.put(key,c);
        }
        catch (NoSuchMethodException nse) {
            Messages.postError("Problem initiating " + key + " area.");
            Messages.writeStackTrace(nse, tool +  " not found.");
        }
        catch (SecurityException se) {
            Messages.postError("Problem initiating " + key + " area.");
            Messages.writeStackTrace(se);
        }
    }

    public void setEditMode(boolean s) {
        inEditMode=s;
        updateSelectedObj();
    }
    public void setResource(SessionShare sshare, AppIF appIF) {
        sshare = sshare;
        appIF = appIF;
    }

    private Constructor getTool(String key) {
        return ( (Constructor) vobjs.get(key) );
    }

    private void displayTool(Component comp) {
        if(tabbedPane != null && tabbedPane.getTabCount() > 1) {
            if (tabbedPane.indexOfComponent(comp) >= 0) {
                tabbedPane.setSelectedComponent(comp);
            }
        }
        if (comp instanceof PushpinObj)
            comp = ((PushpinObj) comp).getPinObj();
        if (comp instanceof XMLToolPanel){
            ((XMLToolPanel) comp).updateChange();
            ((XMLToolPanel) comp).setEditMode(inEditMode);
        }
        else if (comp instanceof VToolPanel)
            ((VToolPanel) comp).updateValue();
    }

    public JComponent searchTool(String name, int id) {
        VToolPanel vp = null;
        int num = keys.size();
        if (num <= 0 || name == null)
            return null; 
        name = Util.getLabelString(name);
        for(int i=0; i < num; i++) {
            String key = (String)keys.get(i);
            JComponent obj = (JComponent)panes.get(key);
            if (obj instanceof VToolPanel)
                vp = (VToolPanel) obj;
            else if (name.equals(key) && obj != null) {
                String Value = (String)tp_paneInfo[id].get(key);
                if (Value.equals("yes")) {
                    return obj;
                }
            }
        }
        return (JComponent) vp;
    }

    public JComponent searchTool(String name) {
        return searchTool(name, vpId);
    }

    private boolean containTool(JComponent obj) {
        int k, num;

        if (obj == null)
            return false;
        Component src = (Component) obj;
        Component comp;
        if (tabbedPane != null) {
            num = tabbedPane.getTabCount();
            for (k = 0; k < num; k++) {
                comp = tabbedPane.getComponentAt(k);
                if (src == comp)
                    return true;
            }
        }
        if (tabbedToolPanel != null) {
            num = tabbedToolPanel.getComponentCount();
            for (k = 0; k < num; k++) {
                comp = tabbedToolPanel.getComponent(k);
                if (src == comp)
                    return true;
            }
        }
        return false;
    }

    public boolean openTool(String name, boolean bOpen, boolean bShowOnly) {
        if (!bOpen)
            return closeTool(name);

        JComponent obj = searchTool(name);
        if (obj == null)
            return false;
        if (obj instanceof VToolPanel) {
            if (!((VToolPanel)obj).openTool(name, bOpen, bShowOnly))
                return false;
        }
        else {
            if (obj instanceof PushpinIF) {
                PushpinIF pobj = (PushpinIF) obj;
                if (bShowOnly) {
                    if (!pobj.isClose())
                        return false; // no change
                }
                pobj.setStatus("open");
            }
        }
        recordCurrentLayout();
        bChangeTool = true;
        setCurrentLayout();
        if (!pinPanel.isOpen()) {
            pinPanel.setVisible(false);
            bChangeTool = false;
            return false;
        }
        if (!bShowOnly)
            displayTool((Component) obj);
        bChangeTool = false;
        if (!pinPanel.isVisible())
            pinPanel.setVisible(true);
        if (bShowOnly)
            setSelectedTab(tp_selectedTab, selectedTabName);
        validate();
        repaint();

        return true; 
    }

    public boolean openTool(String name, boolean bOpen) {
        return openTool(name, bOpen, false);
    }

    public boolean openTool(String name) {
        return openTool(name, true, false);
    }

    public boolean showTool(String name) {
        return openTool(name, true, true);
    }

    public boolean closeTool(String name) {
        JComponent obj = searchTool(name);
        if (obj == null)
            return false;
        if (obj instanceof VToolPanel) {
            if (!((VToolPanel)obj).closeTool(name))
                return false;
        }
        else {
            if (obj instanceof PushpinIF) {
                PushpinIF pobj = (PushpinIF) obj;
                if (pobj.isClose())
                    return false;
                pobj.setStatus("close");
            }
        }
        setCurrentLayout();
        if (!pinPanel.isOpen()) {
            pinPanel.setVisible(false);
            //  return false;
        }

        validate();
        repaint();

        return true; 
    }

    public boolean hideTool(String name) {
        JComponent obj = searchTool(name);
        if (obj == null)
            return false;
        if (obj instanceof VToolPanel) {
            if (!((VToolPanel)obj).hideTool(name))
                return false;
        }
        else {
            if (obj instanceof PushpinIF) {
                PushpinIF pobj = (PushpinIF) obj;
                pobj.setStatus("hide");
            }
        }
        setCurrentLayout();
        if (!pinPanel.isOpen()) {
            pinPanel.setVisible(false);
        }

        validate();
        repaint();

        return true;
    }


    public boolean popupTool(String name) {
        JComponent obj = searchTool(name);
        if (obj == null)
            return false;
        if (obj instanceof VToolPanel) {
            if (!((VToolPanel)obj).popupTool(name))
                return false;
        }
        else {
            if (obj instanceof PushpinIF) {
                PushpinIF pobj = (PushpinIF) obj;
                if (!pobj.isOpen())
                    pobj.pinPopup(true);
            }
        }
        bChangeTool = true;
        setCurrentLayout();
        if (!pinPanel.isOpen()) {
            pinPanel.setVisible(false);
            bChangeTool = false;
            return false;
        }
        displayTool((Component) obj);
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            vif.raiseToolPanel(true);
        bChangeTool = false;
        validate();
        repaint();
        return true; 
    }

    public boolean popdnTool(String name) {
        JComponent obj = searchTool(name);
        if (obj == null)
            return false;
        if (obj instanceof VToolPanel) {
            if (!((VToolPanel)obj).popdnTool(name))
                return false;
        }
        else {
            if (obj instanceof PushpinIF) {
                PushpinIF pobj = (PushpinIF) obj;
                pobj.pinPopup(false);
            }
            boolean bContain = containTool(obj);
            if (!bContain)
                return false;
        }
        bChangeTool = true;
        setCurrentLayout();
        if (previous_selectedTab >= 1 && tabbedPane != null) {
            if (tabbedPane.getTabCount() > previous_selectedTab)
                tabbedPane.setSelectedIndex(previous_selectedTab);
        }
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            vif.raiseToolPanel(false);
        if (!pinPanel.isOpen())
            pinPanel.setVisible(false);
        bChangeTool = false;
        updateSelectedObj();
        repaint();
        return true; 
    }

    public void putHsLayout(int id) {
        Hashtable hs = sshare.userInfo();
        if (hs == null)
            return;
        String key, name;
        JComponent  obj;
        PushpinIF   pobj;

        for(int i=0; i < keys.size(); i++) {
            key = (String)keys.get(i);
            obj = (JComponent)panes.get(key);
            if (obj != null && (obj instanceof PushpinIF)) {
                pobj = (PushpinIF) obj;
                name = "tabTool."+id+"."+pobj.getName()+".";
                hs.put(name+"refY", new Float(pobj.getRefY()));
                hs.put(name+"refX", new Float(pobj.getRefX()));
                hs.put(name+"refH", new Float(pobj.getRefH()));
                key = "open";
                if (pobj.isHide())
                    key = "hide";
                else if (pobj.isClose())
                    key = "close";
                hs.put(name+"status", key);
            }
        }
        /*
        name = "tabTool."+id+".TabPanel.";
        key = pinPanel.getLastName();
        if (key != null)
           hs.put(name+"lastName", key);
        key = "open";
        if (pinPanel.isHide())
           key = "hide";
        else if (pinPanel.isClose())
           key = "close";
        hs.put(name+"status", key);
         */
    }

    public void getHsLayout(int id) {
        Hashtable hs = sshare.userInfo();
        if (hs == null)
            return;
        String  key, name, status;
        Float   fstr;
        float   fv;

        for (int j=0; j<keys.size(); j++) {
            key = (String)keys.get(j);
            JComponent  obj = (JComponent)panes.get(key);
            if (obj instanceof PushpinIF) {
                PushpinIF pobj = (PushpinIF) obj;
                name = "tabTool."+id+"."+pobj.getName()+".";
                fstr = (Float) hs.get(name+"refY");
                if (fstr != null)
                    pobj.setRefY(fstr.floatValue());
                fstr = (Float) hs.get(name+"refX");
                if (fstr != null)
                    pobj.setRefX(fstr.floatValue());
                fstr = (Float) hs.get(name+"refH");
                if (fstr != null)
                    pobj.setRefH(fstr.floatValue());
                status = (String) hs.get(name+"status");
                if (status != null)
                    pobj.setStatus(status);
            }
        }
    }

    public void showPinObj (PushpinIF pobj, boolean on) {
        Component comp = (Component) pobj;
        if (comp == null)
            return;
        Container p = comp.getParent();
        if (!on) {
            pobj.setPopup(false, true);
            if (p != null && p != tabbedPane) {
                p.remove(comp);
                p.validate();
                p.repaint();
            } 
            if (popupComp == comp)
                popupComp = null;
            return;
        }

        if (popupComp != null) {
            if (popupComp != comp) {
                ((PushpinIF) popupComp).setPopup(false, true);
                p = popupComp.getParent();
                if (p != null && p != tabbedPane) {
                    p.remove(popupComp);
                }
            }
            popupComp = null; 
        }
        /*
        if (!isShowing()) {
            return;
        }
         */
        if (comp.isShowing()) {
            return;
        }
        Container p2 = null;
        p = pinPanel.getParent();
        while (p != null) {
            if (p instanceof JLayeredPane)
                p2 = p;
            p = p.getParent();
        }
        if (p2 == null)
            return;
        if (!isShowing()) {
            VnmrjIF vif = Util.getVjIF();
            vif.raiseToolPanel(on);
            setVisible(true);
        }

        popupComp = comp;
        p = p2;
        pobj.setPopup(true,true);
        /*
        Point pt0 = p.getLocationOnScreen();
        Point pt1 = getLocationOnScreen();
         */
        Point pt1 = getLocation();
        Dimension dim = getSize();
        int y0 = (int) ((float) dim.height * pobj.getRefY());
        int h = (int) ((float) dim.height * pobj.getRefH());
        int x = pt1.x + 2;
        int y = pt1.y + y0;
        p.add(comp, JLayeredPane.MODAL_LAYER);
        comp.setBounds(x, y, dim.width, dim.height - y0);
        ((JComponent)p).validate();
        /*
        p.repaint();
         */
    }


    public void recordCurrentLayout() {
        if(tabbedPane != null) {
            tp_selectedTab = tabbedPane.getSelectedIndex();
            if (tp_selectedTab >= 0)
                selectedTabName = tabbedPane.getTitleAt(tp_selectedTab);
            else
                selectedTabName = null;
        }
    }

    public void copyCurrentLayout(VpLayoutInfo info) {
        info.tp_selectedTab = tp_selectedTab;
        info.setVerticalTabName(selectedTabName);
    }

    public boolean comparePanelLayout(int currId, int newId) {
        if(currId >= nviews || newId >= nviews) return false;
        if(tp_paneInfo[currId].size() != tp_paneInfo[newId].size()) return true;

        String key;
        Object value1;
        Object value2;
        for(Enumeration e=tp_paneInfo[currId].keys(); e.hasMoreElements();) {
            key = (String)e.nextElement();
            value1 = tp_paneInfo[currId].get(key);
            value2 = tp_paneInfo[newId].get(key);
            if(!value1.equals(value2)) return true;
        }

        return false;
    }

    public boolean compareCurrentLayout(VpLayoutInfo info) {
        boolean b = false;

        if(tp_selectedTab != info.tp_selectedTab) {
            b = true;
            tp_selectedTab = info.tp_selectedTab;
        }

        return b;
    }

    public void switchLayout(int newId, boolean bLayout) {

        if (layoutId == newId) return;
        if(newId >= nviews)
            updateVpInfo(newId+1);

        if (bSwitching) {
            return;
        }

        bSwitching = true;
        int oldId = layoutId;
        vpId = newId;
        layoutId = newId;

        recordCurrentLayout();
        VpLayoutInfo vInfo = Util.getViewArea().getLayoutInfo(oldId);
        if (vInfo != null) {  // save current layout info
            vInfo.tp_selectedTab = tp_selectedTab;
            vInfo.setVerticalTabName(selectedTabName);
            // copyCurrentLayout(vInfo);
        }

        vInfo = Util.getViewArea().getLayoutInfo(newId);

        putHsLayout(oldId);
        if (bLayout)
            getHsLayout(vpId);

        for(int i=0; i< toolList.size(); i++)
            ((VToolPanel) toolList.get(i)).switchLayout(newId,bLayout);   

        if (bLayout)
            setCurrentLayout();
        /*
	 if(comparePanelLayout(oldId, newId)) {
		setCurrentLayout();
         }
         if ((vInfo != null) && vInfo.bAvailable &&
		compareCurrentLayout(vInfo)) {
                setCurrentLayout(vInfo);
         }

	 for(int i=0; i< toolList.size(); i++)
	     ((VToolPanel) toolList.get(i)).switchLayout(newId);   
         */

        // setViewPort(newId);

        if (bLayout)
            setCurrentLayout(vInfo);

        updateValue();

        if (bLayout) {
            if (pinPanel.isOpen()) {
                if (!pinPanel.isVisible())
                    pinPanel.setVisible(true);
            }
            else
                pinPanel.setVisible(false);
        }
        validate();
        repaint();

        bSwitching = false;
    }

    public void switchLayout(int newId) {
        switchLayout(newId, true);
    }

    public void setViewPort(int id) {

        if(id >= nviews || id < 0) return;

        String key;
        String value;
        vpId = id;
        for(Enumeration e=tp_paneInfo[id].keys(); e.hasMoreElements();) {
            key = (String)e.nextElement();
            value = (String)tp_paneInfo[id].get(key);
            if(value.equals("yes")) {
                JComponent comp = (JComponent) panes.get(key);
                if(comp instanceof VToolPanel) {
                    ((VToolPanel)comp).setViewPort(id);
                }
                else {
                    if (comp instanceof PushpinIF)
                        comp = ((PushpinIF)comp).getPinObj();
                    if (comp instanceof XMLToolPanel)
                        ((XMLToolPanel)comp).setViewPort(id);
                }
            }
        }
    }

    private void setSelectedTab(int id, String name) {
        if(tabbedPane == null)
            return;
        int selectId = id;
        if (name != null) {
            for (int k = 0; k < tabbedPane.getTabCount(); k++) {
                String tabName = tabbedPane.getTitleAt(k);
                if (name.equals(tabName)) {
                    selectId = k;
                    break;
                }
            }
        }
        if (selectId >= 0 && selectId < tabbedPane.getTabCount())
            tabbedPane.setSelectedIndex(selectId);
    }

    public void setCurrentLayout(VpLayoutInfo info) {
        if ((info == null) || (!info.bAvailable))
            return;
        tp_selectedTab = info.tp_selectedTab;
        selectedTabName = info.getVerticalTabName();
        setSelectedTab(tp_selectedTab, selectedTabName);
    }

    private String getLocatorName() {
        String name = "Holding";
        if (Util.isImagingUser())
            name = "Study";
        if (Util.isWalkupUser())
            name = "Protocols";
        return Util.getLabelString(name);
    }

    public void setCurrentLayout(int newId) {

        if(newId >= nviews) return;

        tabbedPane.removeAll();
        tabbedToolPanel.removeAll();
        String key;
        String currValue;
        JComponent obj;
        PushpinIF  pobj;
        clearPushpinComp();
        for(int i=0; i<keys.size(); i++) {
            key = (String)keys.get(i);
            currValue = (String)tp_paneInfo[newId].get(key);
            obj = (JComponent)panes.get(key);
            pobj = null;
            if(currValue.equals("yes") && obj != null) {
                if (obj instanceof PushpinIF) {
                    pobj = (PushpinIF) obj;
                    pobj.setAvailable(true);
                    if (!pobj.isOpen()) {
                        if (!pobj.isPopup())
                            obj = null;
                    }
                }
                if (obj != null)
                {
                    if (key.equals("Locator"))
                        key = getLocatorName();
                    tabbedPane.addTab(key, null, obj, "");
                }
                // tabbedPane.addTab(key, null, (JComponent)panes.get(key), "");
            } 
        }

        if(tabbedPane.getTabCount() < 1) {
            pinPanel.setAvailable(false);
            pinPanel.setStatus("close");
            // setVisible(false);
            return;
        }

        pinPanel.setAvailable(true);
        pinPanel.setStatus("open");
        if(tabbedPane.getTabCount() == 1) {
            tabbedToolPanel.add(tabbedPane.getComponentAt(0));
            //tabbedToolPanel.add(tabbedPane);
        } else {
            tabbedToolPanel.add(tabbedPane);
        }
        setSelectedTab(tp_selectedTab, selectedTabName);

        tabbedToolPanel.validate();
        // repaint();
    }

    public void setCurrentLayout() {
        if(vpId >= nviews) return;
        setCurrentLayout(vpId);
    }

    public int getOpenCount() {
        int num = tabbedToolPanel.getComponentCount();
        if (num != 1)
            return num;
        Component comp = tabbedToolPanel.getComponent(0);
        if(comp instanceof JTabbedPane) {
            JTabbedPane tp = (JTabbedPane)comp;
            num = tp.getTabCount();
            if (num != 1)
                return num;
            comp = tp.getComponentAt(0);
        }
        if (comp instanceof VToolPanel)
            num = ((VToolPanel) comp).getOpenCount();
        else if (comp instanceof PushpinIF) {
            if (!((PushpinIF)comp).isOpen())
                num = 0;
        }

        return num;
    }

    /**
    public void setCurrentLayout(int newId) {

        if(newId >= nviews || vpId >= nviews) return;

        String key;
        String newValue;
        String currValue;

        for(int i=0; i<keys.size(); i++) {
           key = (String)keys.get(i);
	   newValue = (String)tp_paneInfo[newId].get(key);
	   currValue = (String)tp_paneInfo[vpId].get(key);
           JComponent obj = (JComponent)panes.get(key);
	   if(newValue.equals("yes")) {
                if (tabbedPane.indexOfComponent((Component)obj) < 0)
                    tabbedPane.addTab(key, null, obj, "");
                if (obj instanceof PushpinIF)
                    ((PushpinIF) obj).setAvailable(true);
	   } else if(newValue.equals("no")) {
                if (tabbedPane.indexOfComponent((Component)obj) >= 0)
		    tabbedPane.remove(obj);
                if (obj instanceof PushpinIF)
                    ((PushpinIF) obj).setAvailable(false);
	   } 
	}

	tabbedToolPanel.removeAll();
	if(tabbedPane.getTabCount() > 0) {
	   if(tabbedPane.getTabCount() == 1) {
	       tabbedToolPanel.add(tabbedPane.getComponentAt(0));
	   } else if(tabbedPane.getTabCount() > 1) {
	       tabbedToolPanel.add(tabbedPane);
	   }
           pinPanel.setStatus("open");
	}
        else {
           pinPanel.setStatus("close");
           setVisible(false);
        }
	validate();
	repaint();
    }
     **/


    public void initPanel() {

        if(panes == null || panes.size() <= 0) return;

        setCurrentLayout();
        String key;
        for(Enumeration e=panes.keys(); e.hasMoreElements();) {
            key = (String)e.nextElement();
            JComponent comp = (JComponent)panes.get(key);
            if (comp instanceof VToolPanel)
                ((VToolPanel)comp).initPanel();
            else {
                if (comp instanceof PushpinIF)
                    comp = ((PushpinIF)comp).getPinObj();
                if (comp instanceof XMLToolPanel)
                    ((XMLToolPanel)comp).buildPanel();
            } 
            /****
           if(comp instanceof XMLToolPanel) {
		((XMLToolPanel)comp).buildPanel();
	   } else if(comp instanceof VToolPanel) {
		((VToolPanel)comp).initPanel();
	   }
             ****/
        }
        if (pinPanel.isOpen()) {
            if (!pinPanel.isVisible())
                pinPanel.setVisible(true);
        }
        else
            pinPanel.setVisible(false);
    }

    private Component getSelectedObj() {
        if(tabbedToolPanel.getComponentCount() < 1)
            return null;
        Component comp = tabbedToolPanel.getComponent(0);
        if(comp instanceof JTabbedPane) {
            JTabbedPane tp = (JTabbedPane)comp;
            if(tp.getTabCount() == 1) { 
                comp = tp.getComponentAt(0);
            } else if(tp.getTabCount() > 1) {
                comp = tp.getSelectedComponent();
            }
        }
        if(comp instanceof VToolPanel)
            return comp;

        if(!(comp instanceof XMLToolPanel)) { 
            if(comp instanceof PushpinIF)
                comp = ((PushpinIF) comp).getPinObj();
        }
        return comp;
    }

    private void setCompUpdatable() {
        for (int i = 0; i < objList.size(); i++) {
            JComponent obj = (JComponent) objList.get(i);
            if (obj != null) {
                if (obj instanceof XMLToolPanel)
                    ((XMLToolPanel)obj).valueChanged();
            }
        }
    }

    public void updateValue(Vector v) {

        if(!initVp) {
            initVp = true;
            //   Messages.postDebug("initVp");
            setViewPort(vpId);

            updateValue();
            return;
        }

        setCompUpdatable();
        Component comp = getSelectedObj();
        if (comp != null) {
            if(comp instanceof ExpListenerIF)
                ((ExpListenerIF)comp).updateValue(v);
        }
    }

    private void updateSelectedObj() {
        //System.out.println("VtabbedToolPanel updateValue ");

        Component comp = getSelectedObj();
        if (comp != null) {
            if(comp instanceof ExpListenerIF)
                ((ExpListenerIF)comp).updateValue();
            if(comp instanceof EditListenerIF){
                ((EditListenerIF)comp).setEditMode(inEditMode);
                comp.repaint();
            }

        }
    }

    public void updateValue() {
        setCompUpdatable();
        updateSelectedObj();
    }

    private void tabChanged() {
        if (bChangeTool)
            return;
        int index = -1;
        previous_selectedTab = 0;
        if (tabbedPane.getTabCount() > 1) {
            index = tabbedPane.getSelectedIndex();
            Component comp = tabbedPane.getSelectedComponent();
            if (comp != null && (comp instanceof PushpinIF)) {
                PushpinIF pobj = (PushpinIF) comp;
                if (!pobj.isPopup())
                    previous_selectedTab = index;
            }
        }
        updateSelectedObj();
        if (index >= 0) {
            if (Util.getRQPanel() != null) {
                Util.getRQPanel().updatePopup(tabbedPane.getTitleAt(index));
            }
        }
    }

    public void initUiLayout() {

        getHsLayout(vpId);
        for (int i=0; i< toolList.size(); i++)
            ((VToolPanel) toolList.get(i)).initUiLayout();

        initPanel();
        updatePinTabs();

        Hashtable hs = sshare.userInfo();
        if (hs == null || tabbedPane == null || tabbedPane.getTabCount() <= 0)
            return;
        Object obj = hs.get("tabbedToolPanel");
        if(obj == null) return;
        int indx = ((Integer)obj).intValue();
        if(indx < 0) return;
        // if(indx >= 0 && indx < tabbedPane.getTabCount())
        // tabbedPane.setSelectedIndex(indx);
        setSelectedTab(tp_selectedTab, null);
    }

    public void saveUiLayout() {

        for (int i=0; i< toolList.size(); i++)
            ((VToolPanel) toolList.get(i)).saveUiLayout();

        Hashtable hs = sshare.userInfo();
        /*
        if (hs == null || tabbedPane == null || tabbedPane.getTabCount() <= 0)
            return;
         */
        if (hs == null || tabbedPane == null)
            return;
        if (tabbedPane.getTabCount() > 0) {
            Integer hashValue = new Integer(tabbedPane.getSelectedIndex()); 
            hs.put("tabbedToolPanel", hashValue);
        }
        putHsLayout(vpId);
    }

    public ArrayList getToolPanels() {
        return  toolList;
    }

    public void resetPushPinLayout() {
        putHsLayout(vpId);
        setCurrentLayout();
        // revalidate();
        updatePinTabs();
    }

    private void clearPanel() {
        tabbedPane.removeAll();
        tabbedToolPanel.removeAll();
        panes.clear();
        keys.clear();
        vpInfo.clear();
        toolList.clear();
        objList.clear();
        removeAllPushpinComp();
        for(int i=0; i<nviews; i++)
            tp_paneInfo[i].clear();
    }

    public void fill() {
        int i;
        boolean bSameFile = true;
        File fd = null;
        VToolPanel toolPanel = null;

        String toolPanelFile = FileUtil.openPath("INTERFACE/TabbedToolPanel.xml");
        if (toolPanelFile != null)
            fd = new File(toolPanelFile);
        if (fd != null && fd.exists()) {
            bSameFile = false;
            if (buildFile != null && buildFile.equals(toolPanelFile)) {
                if (fd.lastModified() == dateOfbuildFile) // same file
                    bSameFile = true;
            }
        }
        if (bSameFile) {
            for (i = 0; i < objList.size(); i++) {
                JComponent obj = (JComponent) objList.get(i);
                if (obj != null) {
                    if (obj instanceof VToolPanel)
                        toolPanel = (VToolPanel) obj;
                    else {
                        if (obj instanceof StatusListenerIF)
                            ExpPanel.addStatusListener((StatusListenerIF) obj);
                        else if (obj instanceof ExpListenerIF)
                            ExpPanel.addExpListener((ExpListenerIF) obj);
                    }
                }
            }
            if (toolPanel != null)
                toolPanel.fill();
            return;
        }
        for (i = 0; i < objList.size(); i++) {
            for (i = 0; i < objList.size(); i++) {
                JComponent obj = (JComponent) objList.get(i);
                if (obj != null) {
                    if (obj instanceof VToolPanel)
                        toolPanel = (VToolPanel) obj;
                }
            }
        }
        if (toolPanel != null)
            toolPanel.clearAll();

        buildFile = toolPanelFile;
        if (fd != null)
            dateOfbuildFile = fd.lastModified();

        clearPanel();
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false);  // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            SAXParser parser = spf.newSAXParser();
            if(toolPanelFile == null) {
                /* get VToolPanel only */
                String key = "Locator"; 
                PushpinIF pObj;
                Constructor c = (Constructor)getTool(key);
                Object[] vargs = new Object[1];
                vargs[0] = sshare;
                if (c != null) {
                    JComponent comp = (JComponent)c.newInstance(vargs);
                    if(comp instanceof VToolPanel) {
                        toolList.add(comp);
                    }
                    tabbedToolPanel.add(comp);
                    objList.add(comp);
                    if (!(comp instanceof PushpinIF)) {
                        pObj = new PushpinObj(comp, pinPanel);
                        // pObj.setTitle("Tool Panel");
                        pObj.showPushPin(false);
                        pObj.showTitle(false);
                        // pObj.alwaysShowTab(true);
                    }
                    else
                        pObj = (PushpinIF) comp;
                    pObj.setContainer(pinPanel);
                    pObj.setSuperContainer(pinPanel);
                    addTabComp(pObj);
                    panes.put(key, pObj);
                    // panes.put(key,comp);
                    keys.add(key);
                    vpInfo.add("all");
                    for(i=0; i<nviews; i++) tp_paneInfo[i].put(key,"yes");
                }
            } else { 
                parser.parse( new File(toolPanelFile), new MySaxHandler() );
            }

        } catch (ParserConfigurationException pce) {
            System.out.println("The underlying parser does not support the " +
                    "requested feature(s).");
        } catch (FactoryConfigurationError fce) {
            System.out.println("Error occurred obtaining SAX Parser Factory.");
        } catch (Exception e) {
            e.printStackTrace();
        }

        for (i=0; i< toolList.size(); i++)
            ((VToolPanel)toolList.get(i)).fill();
    }

    public class MySaxHandler extends DefaultHandler {

        JComponent  lastComp=null;
        String      lastName;

        public void endDocument() {
            // System.out.println("End of Document");
            updateVpInfo();
        }

        public void endElement(String uri, String localName, String qName) {
            // System.out.println("End of Element '"+qName+"'");
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
            //System.out.println("Start of Element '"+qName+"'");
            //int numOfAttr = attr.getLength();
            //System.out.println("   Number of Attributes is "+numOfAttr);
            //for (int i=0; i<numOfAttr; i++) {
            // System.out.println("   with attr["+i+"]='"+attr.getValue(i)+"'");
            //}
            if ( ! qName.equals("tool")) return;

            boolean bScroll = true;
            PushpinIF pObj = null;
            lastName = attr.getValue("name");
            String helplink = attr.getValue("helplink");
            Constructor c = (Constructor)getTool(lastName);
            Object[] vargs;
            String toolFile; 
            if (lastName.equals("XMLToolPanel")) {
                lastName = attr.getValue("label");
                lastName = Util.getLabelString(lastName);
                String scroll = attr.getValue("scrollbar");
                if (scroll != null) {
                    if (scroll.equalsIgnoreCase("no"))
                        bScroll = false;
                }
                vargs = new Object[3];
                vargs[0] = sshare;
                vargs[1] = lastName;
                toolFile = attr.getValue("file");
                vargs[2] = toolFile;
                String f = "LAYOUT"+File.separator+"toolPanels"+File.separator+toolFile;
                toolFile = FileUtil.openPath(f);
                if ( toolFile == null ) return;

            } else {
                vargs = new Object[1];
                vargs[0] = sshare;
            }
            if (c != null) {
                try {
                    lastComp = (JComponent)c.newInstance(vargs);
                    if(lastComp instanceof VToolPanel) {
                        toolList.add(lastComp);
                        pObj = (PushpinIF) lastComp;
                    }
                    objList.add(lastComp);
                }
                catch (Exception e) {
                    lastComp = new JLabel(lastName);
                }
            }
            else {
                lastComp = new JLabel(lastName);
            }

            String vps = attr.getValue("viewport");

            if (lastComp != null && (lastComp instanceof XMLToolPanel))
                ((XMLToolPanel)lastComp).setScrollAble(bScroll);
            if (pObj == null) {
                PushpinObj nObj = new PushpinObj(lastComp, pinPanel);
                pObj = nObj;
                pObj.setTitle(lastName);
                pObj.showPushPin(true);
                pObj.showTitle(true);
                pObj.setTabOnTop(true);
            }
            pObj.setName(lastName);
            addTabComp(pObj);
            pObj.setContainer(pinPanel);
            pObj.setSuperContainer(pinPanel);
            panes.put(lastName, pObj);
            // panes.put(lastName, lastComp);
            keys.add(lastName);
            vpInfo.add(vps);
        }
        public void warning(SAXParseException spe) {
            System.out.println("Warning at line "+spe.getLineNumber()+
                    ", column "+spe.getColumnNumber());
        }
    }

    public void updateVpInfo(int n) {

        if(n == nviews) return;

        n = Math.min(n,maxViews);

        if(n > nviews) 
            for(int i=nviews; i<n; i++)
                tp_paneInfo[i] = new Hashtable();

        nviews = n;

        updateVpInfo();

        for(int i=0; i< toolList.size(); i++)
            ((VToolPanel)toolList.get(i)).updateVpInfo(n);   
    }

    private void updateVpInfo() {

        String key;
        String vps;
        for(int j=0; j<keys.size(); j++) {
            key = (String)keys.get(j);
            vps = (String)vpInfo.get(j);
            if(vps == null || vps.length() <= 0 || vps.equals("all")) {
                for(int i=0; i<nviews; i++) {
                    tp_paneInfo[i].put(key,"yes");
                }
            } else {
                for(int i=0; i<nviews; i++) tp_paneInfo[i].put(key,"no");
                StringTokenizer tok = new StringTokenizer(vps, " ,\n"); 
                while(tok.hasMoreTokens()) {
                    int vp = Integer.valueOf(tok.nextToken()).intValue();
                    vp--;
                    if(vp >= 0 && vp < nviews) {
                        tp_paneInfo[vp].remove(key);
                        tp_paneInfo[vp].put(key,"yes");
                    }
                }
            }
        }
    }
    /* CSHMouseAdapter
     * 
     * Mouse Listener to put up Context Sensitive Help (CSH) Menu and
     * respond to selection of that menu.  The panel's .xml file must have
     * "helplink" set to the keyword/topic for Robohelp.  It must be a 
     * topic listed in the .properties file for this help manual.
     * If helplink is not set, it will open the main manual.
     */
    private class CSHMouseAdapter extends MouseAdapter  {

        public CSHMouseAdapter() {
            super();
        }

        public void mouseClicked(MouseEvent evt) {
            int btn = evt.getButton();
            if(btn == MouseEvent.BUTTON3) {
                JPopupMenu helpMenu = new JPopupMenu();
                String helpLabel = Util.getLabel("CSHMenu");
                JMenuItem helpMenuItem = new JMenuItem(helpLabel);
                helpMenuItem.setActionCommand("help");
                helpMenu.add(helpMenuItem);

                ActionListener alMenuItem = new ActionListener()
                {
                    public void actionPerformed(ActionEvent e)
                    {
                        String topic = "";
                        try {
                            JComponent c1 = (JComponent)tabbedPane.getSelectedComponent();
                            // This component should contain the following embedded items.
                            // We want m_helplink out of the top VGroup.
                            //   PushpinObj
                            //     XMLToolPanel
                            //       JScrollpane
                            //         JViewport
                            //           JPanel
                            //             VGroup
                            // Work down to the VGroup, checking for class type along
                            // the way.
                            if(c1 instanceof PushpinObj) {
                                Component c2[] = c1.getComponents();
                                int cnt;
                                for(cnt=0; cnt < c2.length; cnt++) {
                                    // The PushpinObj can have multiple items, find the one we want
                                    if(c2[cnt] instanceof XMLToolPanel)
                                        break;
                                }
                                if(cnt < c2.length && c2[cnt] instanceof XMLToolPanel) {
                                    Component c3[] = ((JComponent)c2[cnt]).getComponents();
                                    if(c3[0] instanceof JScrollPane) {
                                        Component c4[] = ((JComponent)c3[0]).getComponents();
                                        if(c4[0] instanceof JViewport) {
                                            Component c5[] = ((JComponent)c4[0]).getComponents();
                                            if(c5[0] instanceof JPanel) {
                                                Component c6[] = ((JComponent)c5[0]).getComponents();
                                                if(c6[0] instanceof VGroup) {
                                                    // Get the helplink info from the VGroup
                                                    topic =((VGroup)c6[0]).getAttribute(VObjDef.HELPLINK);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            // If no helplink found, try the Tab's name
                            if(topic == null || topic.length() == 0) {
                                topic = c1.getName();
                                if(topic.equals("Locator"))
                                    topic = getLocatorName();
                                topic = topic.replace(" ", "_");
                            }
                        }
                        catch (Exception ex) {
                        }
                        // Get the ID and display the help content
                        CSH_Util.displayCSHelp(topic);
                    }
                };
                helpMenuItem.addActionListener(alMenuItem);

                Point pt = evt.getPoint();
                helpMenu.show(VTabbedToolPanel.this, (int)pt.getX(), (int)pt.getY());
            }  
        }
    }  /* End CSHMouseAdapter class */

}
