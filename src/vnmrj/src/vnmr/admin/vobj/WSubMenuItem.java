/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.vobj;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.beans.*;
import java.awt.event.*;
import javax.swing.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.*;
// import vnmr.admin.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2001
 * Company:
 * @author
 * @version 1.0
 */

public class WSubMenuItem extends VSubMenuItem implements WObjIF
{

    protected final static String SPEC       = "spectrometers";
    protected final static String PRINTERS   = "printers";
    protected final static String USERS      = "users";
    protected final static String ACCNTG     = "accounting";
    protected final static String SOFTWARE   = "software";

    protected AppIF m_objAppif = null;

    /** Property Change Support object.  */
    protected static PropertyChangeSupport m_pcsTypesMgr;

    protected String m_strPath1 = "";
    protected String m_strPath2 = "";

    public WSubMenuItem(SessionShare ss, ButtonIF vif, String typ)
    {
        super(ss, vif, typ);
        //m_objAppif = ss.getAppIF();
        m_objAppif = Util.getAppIF();
        setOptions();

        m_pcsTypesMgr=new PropertyChangeSupport(this);
    }

    public void actionPerformed(ActionEvent e)
    {
        //System.out.println("Meun itme clicked  " + this.getAttribute(LABEL));

        VAdminIF objAppIf = getAdminIF();

        String strPropName = getAttribute(KEYSTR);
        if(strPropName == null || strPropName.length() <= 0)
            strPropName = getAttribute(LABEL);

        Container container = getParent();
        if (container instanceof JPopupMenu)
        {
            JPopupMenu popupMenu = (JPopupMenu)container;
            VSubMenu menu = (VSubMenu)popupMenu.getInvoker();
            String strMenuLabel = menu.getAttribute(LABEL);
            if (strMenuLabel.equalsIgnoreCase("Management") &&
                    getAttribute(LABEL).indexOf("...") < 0)
            {
                doManageAction(strPropName);
                if (objAppIf != null)
                    objAppIf.getCurrInfo().setMenuItem(this);
            }

        }
        m_pcsTypesMgr.firePropertyChange(strPropName, "all", "");
    }

    public void changeLook()
    {
        //super.changeLook();
        setOptions();
    }

    protected void setOptions()
    {
        String strColName=WFontColors.getColorOption(WGlobal.ADMIN_BGCOLOR);
        Color bgColor = (strColName != null) ? WFontColors.getColor(strColName)
                                            : Global.BGCOLOR;
        setAttribute(BGCOLOR, strColName);
        setBackground(bgColor);
    }

    public String getAttribute(int nAttr)
    {
        switch (nAttr)
        {
            case PATH1:
                return m_strPath1;
            case PATH2:
                return m_strPath2;
            default:
                return super.getAttribute(nAttr);
        }
    }

    public void setAttribute(int nAttr, String strAttr)
    {
        switch (nAttr)
        {
            case LABEL:
                super.setAttribute(nAttr, strAttr);
                break;
            case PATH1:
                m_strPath1 = strAttr;
                break;
            case PATH2:
                m_strPath2 = strAttr;
                break;
            default:
                super.setAttribute(nAttr, strAttr);
                break;
        }
    }

    public String getValue()
    {
        return getAttribute(LABEL);
    }

    public void setValue(String strVal)
    {
        label = strVal;
        setText(strVal);
    }

    public static void firePropertyChng(String name, String oldValue, String newValue)
    {
        m_pcsTypesMgr.firePropertyChange(name, oldValue, newValue);
    }

    public static void showUsersPanel()
    {
        String strLabel = "users";
        String strPropName = strLabel + WGlobal.SEPERATOR;
        m_pcsTypesMgr.firePropertyChange(strPropName+WGlobal.AREA1, strLabel, "SYSTEM/USRS/userlist");
        m_pcsTypesMgr.firePropertyChange(strPropName+WGlobal.AREA2, strLabel, "");
        m_pcsTypesMgr.firePropertyChange("users", "all", "");
        VAdminIF objAppIf = getAdminIF();
        if (objAppIf != null)
            objAppIf.getCurrInfo().setSelMenuLabel(strLabel);
       	if(!UtilB.iswindows())
       		setDragPnlVis(true);
    }

    protected void doManageAction(String strLabel)
    {
        String strPath1 = getAttribute(PATH1);
        String strPath2 = getAttribute(PATH2);

        if (strLabel != null && strLabel.equalsIgnoreCase(USERS))
            setDragPnlVis(true);
        else
            setDragPnlVis(false);

        resetPane(strLabel);
        String strPropName = strLabel + WGlobal.SEPERATOR;
        m_pcsTypesMgr.firePropertyChange(strPropName+WGlobal.AREA1, strLabel, strPath1);
        m_pcsTypesMgr.firePropertyChange(strPropName+WGlobal.AREA2, strLabel , strPath2);
    }

    protected static void setDragPnlVis(boolean bVisible)
    {
        VAdminIF objAdminIf = getAdminIF();
        if (objAdminIf != null)
            objAdminIf.setDragPanelVis(bVisible);
    }

    protected static void resetPane(String strLabel)
    {
        VAdminIF objAdminIf = getAdminIF();
        if (objAdminIf == null)
            return;

        strLabel = strLabel.toLowerCase();
        /*if (strLabel.indexOf(WGlobal.PART11) >= 0)
        {
           objAdminIf.setFullSplitPane(1, 1, false);
        }
        else if (strLabel.indexOf("users") >= 0)
        {
            objAdminIf.setSplitPaneDefaults();
        }
        else
        {*/
           objAdminIf.setSplitPaneDefaults();
        //}
    }

    protected void clearPanels()
    {
        VAdminIF objAdminIf = getAdminIF();
        if (objAdminIf != null)
            objAdminIf.clearPanels();
    }

    protected static VAdminIF getAdminIF()
    {
        AppIF objAppIf = Util.getAppIF();
        if (objAppIf instanceof VAdminIF)
            return (VAdminIF)objAppIf;
        else
            return null;
    }

    //==================================================================
    //   PropertyChangeListener implementation Follows ...
    //==================================================================

    /**
     *  Adds the specified change listener.
     *  @param l    the property change listener to be added.
     */
    public static void addChangeListener(PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(l);
    }

    /**
     *  Adds the specified change listener with the specified property.
     *  @param strProperty  the property to which the listener should be listening to.
     *  @param l            the property change listener to be added.
     */
    public static void addChangeListener(String strProperty, PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
        {
            m_pcsTypesMgr.addPropertyChangeListener(strProperty, l);
        }
    }

    /**
     *  Removes the specified change listener.
     */
    public static void removeChangeListener(PropertyChangeListener l)
    {
        if(m_pcsTypesMgr != null)
            m_pcsTypesMgr.removePropertyChangeListener(l);
    }

}
