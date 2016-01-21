/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VParameter extends VGroup implements ExpListenerIF
{

    protected VLabel m_label;
    protected VEntry m_entry;
    protected VCheck m_check;
    protected VObjIF m_compUnits;
    protected String m_vnmrVar2 = "";
    protected String m_strEnabled;
    protected String m_strEntrySize;
    protected String m_strUnitsEnabled;
    protected String m_strUnitsSize;
    protected SessionShare sessionshare;
    protected ButtonIF appIf;
    protected String type;
    protected String m_strCheckEnabled;
    protected String m_strCheckValue;
    protected String m_strCheckcmd;
    protected String m_strCheckcmd2;
    protected String m_strEntryValue;
    protected String m_strEntrycmd;
    protected String m_strDigit = "";
    protected String m_strDisable = VEntry.m_arrStrDisAbl[0];
    protected String m_strUnitsLabel;
    protected String m_strUnitsValue;
    protected String m_strUnitscmd;
    protected boolean m_bInitialize = false;
    protected String m_strMenu;
    protected String m_strMenuValue;
    protected String objName;

    public static final String[] UNITS = {"None", "Label", "Text Message", "Menu"};


    public VParameter(SessionShare sshare, ButtonIF vif, String strtype)
    {
        super(sshare, vif, strtype);
        sessionshare = sshare;
        appIf = vif;
        type = strtype;
        setLayout(new VParameterLayout());
    }

    public Component add(Component comp)
    {
        Component component = super.add(comp);
        if (component instanceof VLabel)
        {
            String strValue = ((VLabel)component).getAttribute(KEYSTR);
            if (strValue != null && strValue.equals("units"))
            {
                m_compUnits = (VLabel)component;
                setUnitsAttribute();
                setUnitsSize();
            }
            else
            {
                m_label = (VLabel) component;
                m_label.setAttribute(LABEL, title);
                setLabelAttribute();
            }
        }
        else if (component instanceof VCheck)
        {
            m_check = (VCheck)component;
            setCheckAttribute();
            component.setPreferredSize(null);
        }
        else if (component instanceof VEntry)
        {
            m_entry = (VEntry)component;
            setEntryAttribute();
        }
        else if (component instanceof VTextMsg)
        {
            m_compUnits = (VTextMsg)component;
            setUnitsAttribute();
            setUnitsSize();
        }
        else if (component instanceof VMenu)
        {
            m_compUnits = (VMenu)component;
            setUnitsAttribute();
            setUnitsSize();
            component.setPreferredSize(null);
        }
        return component;
    }

    public String getAttribute(int attr)
    {
        switch (attr)
        {
            case VAR2:
                return m_vnmrVar2;
            case LABEL:
                return title;
            case ENABLE:
                Container container = getParent();
                if (container instanceof VGroup)
                    return ((VGroup)container).getAttribute(ENABLE);
            case ENABLED:
                return m_strEnabled;
            case CHECKENABLED:
                return m_strCheckEnabled;
            case CHECKVALUE:
                return m_strCheckValue;
            case CHECKCMD:
                return m_strCheckcmd;
            case CHECKCMD2:
                return m_strCheckcmd2;
            case ENTRYVALUE:
                return m_strEntryValue;
            case ENTRYCMD:
                return m_strEntrycmd;
            case NUMDIGIT:
                return m_strDigit;
            case DISABLE:
                return m_strDisable;
            case ENTRYSIZE:
                return m_strEntrySize;
            case UNITSENABLED:
                return m_strUnitsEnabled;
            case UNITSSIZE:
                return m_strUnitsSize;
            case UNITSLABEL:
                return m_strUnitsLabel;
            case UNITSVALUE:
                return m_strUnitsValue;
            case UNITSCMD:
                return m_strUnitscmd;
            case CHVAL:
                return m_strMenu;
            case VALUES:
                return m_strMenuValue;
            case PANEL_NAME:
                return objName;
            default:
                return super.getAttribute(attr);
        }
    }

    public void setAttribute(int attr, String value)
    {
        if (value != null)
            value = value.trim();
        try
        {
        switch (attr)
        {
            case VAR2:
                m_vnmrVar2 = value;
                if (vnmrVar != null)
                    value = vnmrVar + " " + value;
                settitle(m_vnmrVar2);
                setCompAttribute(VARIABLE, value);
                break;
            case LABEL:
                title = value;
                if (m_label != null && value != null && !value.equals(""))
                    m_label.setAttribute(LABEL, value);
                break;
            case VARIABLE:
                vnmrVar = value;
                if (m_vnmrVar2 != null)
                    value = vnmrVar + " " + m_vnmrVar2;
                setCompAttribute(VARIABLE, value);
                break;
            case ENABLED:
                m_strEnabled = value;
                setEnabled(value);
                break;
            case FGCOLOR:
                super.setAttribute(FGCOLOR, value);
                setCompAttribute(FGCOLOR, value);
                break;
            case FONT_NAME:
                super.setAttribute(FONT_NAME, value);
                setCompAttribute(FONT_NAME, value);
                break;
            case FONT_STYLE:
                super.setAttribute(FONT_STYLE, value);
                setCompAttribute(FONT_STYLE, value);
                break;
            case FONT_SIZE:
                super.setAttribute(FONT_SIZE, value);
                setCompAttribute(FONT_SIZE, value);
                break;
            case CHECKENABLED:
                m_strCheckEnabled = value;
                if (value == null || value.equals(""))
                    value = m_strEnabled;
                if (m_check != null)
                    m_check.setAttribute(SHOW, value);
                break;
            case CHECKVALUE:
                m_strCheckValue = value;
                if (m_check != null)
                    m_check.setAttribute(SETVAL, value);
                if (inEditMode) 
                    setCheckVisible();
                break;
            case CHECKCMD:
                m_strCheckcmd = value;
                if (m_check != null)
                    m_check.setAttribute(CMD, value);
                if (inEditMode) 
                    setCheckVisible();
                break;
            case CHECKCMD2:
                m_strCheckcmd2 = value;
                if (m_check != null)
                    m_check.setAttribute(CMD2, value);
                if (inEditMode) 
                    setCheckVisible();
                break;
            case ENTRYVALUE:
                m_strEntryValue = value;
                if (m_entry != null)
                    m_entry.setAttribute(SETVAL, value);
                if (inEditMode) 
                    setEntryVisible();
                break;
            case ENTRYCMD:
                m_strEntrycmd = value;
                if (m_entry != null)
                    m_entry.setAttribute(CMD, value);
                if (inEditMode) 
                    setEntryVisible();
                break;
            case NUMDIGIT:
                m_strDigit = value;
                if (m_entry != null)
                    m_entry.setAttribute(NUMDIGIT, value);
                break;
            case DISABLE:
                m_strDisable = value;
                if (m_entry != null)
                    m_entry.setAttribute(DISABLE, value);
                break;
            case ENTRYSIZE:
                if (value != null && !value.equals(m_strEntrySize))
                {
                    m_strEntrySize = value;
                    try
                    {
                        if (m_entry != null)
                            m_entry.setSize(Integer.parseInt(value), m_entry.getHeight());
                    }
                    catch (Exception e) { }
                }
                break;
            case UNITSENABLED:
                m_strUnitsEnabled = value;
                setUnitsEnable(value);
                break;
            case UNITSSIZE:
                if (value != null && !value.equals(m_strUnitsSize))
                {
                    m_strUnitsSize = value;
                    setUnitsSize();
                }
                break;
            case UNITSLABEL:
                m_strUnitsLabel = value;
                setUnitsLabel(value);
                break;
            case UNITSVALUE:
                m_strUnitsValue = value;
                setUnitsValue(value);
                break;
            case UNITSCMD:
                m_strUnitscmd = value;
                if (m_compUnits != null && m_compUnits instanceof VMenu)
                    m_compUnits.setAttribute(CMD, value);
                break;
            case CHVAL:
                m_strMenu = value;
                if (m_compUnits != null && m_compUnits instanceof VMenu)
                    m_compUnits.setAttribute(SETCHOICE, value);
                break;
            case VALUES:
                m_strMenuValue = value;
                if (m_compUnits != null)
                    m_compUnits.setAttribute(SETCHVAL, value);
                break;
            case PANEL_NAME:
                objName = value;
                break;
            default:
                super.setAttribute(attr, value);
                break;
        }
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    public void showGroup(boolean vis) {
        super.showGroup(vis);
        setUnitsEnable(m_strUnitsEnabled);
    }

    public void setEnabled(String value)
    {
        if (m_check != null && (m_strCheckEnabled == null || m_strCheckEnabled.equals("")))
            m_check.setAttribute(SHOW, value);
        if (m_entry != null)
            m_entry.setAttribute(SHOW, value);
        if (m_compUnits != null) {
            if (m_compUnits instanceof VTextMsg || m_compUnits instanceof VMenu)
                m_compUnits.setAttribute(SHOW, value);
        }
    }

    /**
     * If the parameter is enabled or disabled.
     * @return
     */
    public boolean getParameterEnabled()
    {
        boolean bShow = true;
        if (m_entry != null)
        {
            bShow = m_entry.isEnabled();
        }
        return bShow;
    }

    public void setEditMode(boolean bShow)
    {
        super.setEditMode(bShow);
        if (bShow && m_compUnits != null) {
            if (m_strUnitsEnabled != null && m_strUnitsEnabled.equals(UNITS[0]))
                ((JComponent)m_compUnits).setVisible(false);
        }
    }

    public void updateValue()
    {
        setUnitsEnable(m_strUnitsEnabled);
        setEntryVisible();
        setCheckVisible();
        super.updateValue();
    }

    protected void setObjFont(VObjIF obj) {
        if (fg != null)
           obj.setAttribute(FGCOLOR, fg);
        if (fontStyle != null)
           obj.setAttribute(FONT_STYLE, fontStyle);
        if (fontName != null)
           obj.setAttribute(FONT_NAME, fontName);
        if (fontSize != null)
           obj.setAttribute(FONT_SIZE, fontSize);
    }

    protected void setLabelAttribute()
    {
        if (m_label == null)
            return;
        settitle(m_vnmrVar2);
        setObjFont(m_label);
    }

    protected void setCheckVisible()
    {
        if (m_check == null)
            return;
        if (m_strCheckcmd != null || m_strCheckcmd2 != null || m_strCheckValue != null)
        {
            m_check.setVisible(true);
            if (m_strCheckcmd == null && m_strCheckcmd2 == null)
               m_check.setEnabled(false);
            else
               m_check.setEnabled(true);
        }
        else
            m_check.setVisible(false);
    }

    protected void setCheckAttribute()
    {
        if (m_check == null)
            return;

        if (m_strCheckcmd == null)
            m_strCheckcmd = m_check.getAttribute(CMD);
        else
            m_check.setAttribute(CMD, m_strCheckcmd);

        if (m_strCheckcmd2 == null)
            m_strCheckcmd2 = m_check.getAttribute(CMD2);
        else
            m_check.setAttribute(CMD2, m_strCheckcmd2);

        if (m_strCheckEnabled == null)
            m_strCheckEnabled = m_check.getAttribute(SHOW);
        else
            m_check.setAttribute(SHOW, m_strCheckEnabled);

        if (m_strCheckEnabled == null || m_strCheckEnabled.equals(""))
            m_check.setAttribute(SHOW, m_strEnabled);

        if (m_strCheckValue == null)
            m_strCheckValue = m_check.getAttribute(SETVAL);
        else
            m_check.setAttribute(SETVAL, m_strCheckValue);

        setObjFont(m_check);
        setCheckVisible(); 
        // m_check.updateValue();
    }

    protected void setEntryVisible()
    {
        if (m_entry == null)
            return;
        if (m_strEntryValue != null || m_strEntrycmd != null)
            m_entry.setVisible(true);
        else
            m_entry.setVisible(false);
    }

    protected void setEntryAttribute()
    {
        if (m_entry == null)
            return;
        if (m_strEntrycmd == null)
            m_strEntrycmd = m_entry.getAttribute(CMD);
        else
            m_entry.setAttribute(CMD, m_strEntrycmd);

        if (m_strEntryValue == null)
            m_strEntryValue = m_entry.getAttribute(SETVAL);
        else
            m_entry.setAttribute(SETVAL, m_strEntryValue);

        if (m_strDigit == null)
            m_strDigit = m_entry.getAttribute(NUMDIGIT);
        else
            m_entry.setAttribute(NUMDIGIT, m_strDigit);

        if (m_strDisable == null)
            m_strDigit = m_entry.getAttribute(DISABLE);
        else
            m_entry.setAttribute(DISABLE, m_strDisable);

        if (m_strEnabled == null)
            m_strEnabled = m_entry.getAttribute(SHOW);
        else
            m_entry.setAttribute(SHOW, m_strEnabled);

        setObjFont(m_entry);
        setEntryVisible();
        // m_entry.updateValue();
    }

    protected void setUnitsAttribute()
    {
        if (m_compUnits == null)
            return;
        if (m_strUnitscmd == null)
            m_strUnitscmd = m_compUnits.getAttribute(CMD);
        else
            m_compUnits.setAttribute(CMD, m_strUnitscmd);

        if (m_strUnitsEnabled == null)
            m_strUnitsEnabled = m_compUnits.getAttribute(ENABLED);
        else
            m_compUnits.setAttribute(ENABLED, m_strUnitsEnabled);

        m_bInitialize = true;
        if (m_compUnits instanceof VLabel) {
            if (m_strUnitsLabel == null)
               m_strUnitsLabel = m_compUnits.getAttribute(LABEL);
            else
               m_compUnits.setAttribute(LABEL, m_strUnitsLabel);
        }
        else {
            if (m_strUnitsValue == null)
               m_strUnitsValue = m_compUnits.getAttribute(SETVAL);
            else
               m_compUnits.setAttribute(SETVAL, m_strUnitsValue);
        }

        m_compUnits.setAttribute(SUBTYPE, "parameter");
        setObjFont(m_compUnits);

        // m_compUnits.updateValue();
    }

    protected void setCompAttribute(int attr, String value)
    {
        if (m_label != null)
            m_label.setAttribute(attr, value);
        if (m_check != null)
            m_check.setAttribute(attr, value);
        if (m_entry != null)
            m_entry.setAttribute(attr, value);
        if (m_compUnits != null)
            m_compUnits.setAttribute(attr, value);
    }

    protected void settitle(String value)
    {
        if (m_label != null && (title == null || title.equals("")) &&
            value != null)
        {
            value = ParamResourceBundle.getValue(value);
            if (value != null)
                m_label.setAttribute(LABEL, value);
        }
    }

    protected void setUnitsLabel(String value)
    {
        if (m_compUnits != null && m_compUnits instanceof VLabel)
            m_compUnits.setAttribute(LABEL, value);
    }

    protected void setUnitsValue(String value)
    {
        if (m_compUnits == null)
            return;
        if (m_compUnits instanceof VTextMsg || m_compUnits instanceof VMenu)
            m_compUnits.setAttribute(SETVAL, value);
    }

    protected void setUnitsSize()
    {
        if (m_compUnits == null || m_strUnitsSize == null)
            return;
        try
        {
            JComponent comp = (JComponent)m_compUnits;
            Dimension size = comp.getSize();
            size.width = Integer.parseInt(m_strUnitsSize);
            comp.setSize(size);
        }
        catch (Exception e) { return; }
        revalidate();
    }

    /**
     * The units could be a label, textmessage or menu, this would set the units
     * to the value.
     */
    protected void setUnitsEnable(String value)
    {
        if (value == null || value.equals(""))
            return;
        value = value.trim();
        JComponent comp = (JComponent)m_compUnits;

        if (value.equals(UNITS[0]))
        {
            if (comp != null) {
                comp.setVisible(false);
                if (comp instanceof VMenu)
                    ((VMenu)comp).setShow(false);
            }
        }
        else if (value.equals(UNITS[1]) || value.equals(UNITS[2]) ||
                 value.equals(UNITS[3]))
        {
            Dimension size = null;
            Point loc = null;
            if (comp != null)
            {
                size = comp.getSize();
                loc = m_compUnits.getDefLoc();
            }
            if (value.equals(UNITS[1]) && !(m_compUnits instanceof VLabel))
            {
                if (comp != null)
                    remove(comp);
                m_compUnits = new VLabel(sessionshare, appIf, "label");
                m_compUnits.setAttribute(KEYSTR, "units");
            }
            else if (value.equals(UNITS[2]) && !(m_compUnits instanceof VTextMsg))
            {
                if (comp != null)
                    remove(comp);
                m_compUnits = new VTextMsg(sessionshare, appIf, "textmessage");
            }
            else if (value.equals(UNITS[3]) && !(m_compUnits instanceof VMenu))
            {
                if (comp != null)
                    remove(comp);
                m_compUnits = new VMenu(sessionshare, appIf, "menu");
            }

            if (comp != (JComponent)m_compUnits)
            {
                comp = (JComponent)m_compUnits;
                if (m_bInitialize)
                {
                    add(comp);
                    setUnitsAttribute();
                }
                if (size != null)
                {
                    comp.setSize(size);
                    comp.setLocation(loc);
                    m_compUnits.setDefLoc(loc.x, loc.y);
                }
            }
            comp.setVisible(true);
            if (comp instanceof VMenu) {
                ((VMenu)comp).setShow(true);
                // comp.setPreferredSize(null);
            }
            revalidate();
            repaint();
        }
    }

    public Dimension getActualSize()
    {
        int w = 0;
        int w2 = 0;
        int h = 0;
        Dimension dim;
        Point pt;
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
            Component comp = getComponent(i);
            if ((comp != null) && comp.isVisible()) {
                if (comp instanceof VObjIF)
                    pt = ((VObjIF) comp).getDefLoc();
                else
                    pt = comp.getLocation();
                if (comp instanceof VGroup)
                    dim = ((VGroup) comp).getActualSize();
                else
                    dim = comp.getPreferredSize();
                w2 += dim.width;
                if ((pt.x + dim.width) > w)
                    w = pt.x + dim.width;
                if ((pt.y + dim.height) > h)
                    h = pt.y + dim.height;
            }
        }

        actualDim.width = w + 1;
        actualDim.height = h + 1;
        if (!inEditMode) {
            if (!isPreferredSizeSet()) {
                defDim = getPreferredSize();
                setPreferredSize(defDim);
            }
            if ((defDim.width < w) || (defDim.height < h)) {
                if (defDim.width > 0) {
                    /*if (defDim.width < w)
                        defDim.width = w + 2;
                    if (defDim.height < h)
                        defDim.height = h + 2;*/
                    setPreferredSize(defDim);
                }
            }
            if (bKeepSize || isOpaque()) {
                if (defDim.width <= 0)
                    defDim = getPreferredSize();
                if (actualDim.width < defDim.width)
                    actualDim.width = defDim.width;
                if (actualDim.height < defDim.height)
                    actualDim.height = defDim.height;
            }
        }
        validate();
        return actualDim;
    }


    public Object[][] getAttributes() { return attributes; }

    private final static Object[][] attributes = {
        {new Integer(VAR2),         "Parameter Name:"},
        {new Integer(LABEL),        "Label of item:"},
        {new Integer(ENABLED),         "Enable Condition:"},
        {new Integer(VARIABLE),     "Vnmr variables:    "},
        {new Integer(CHECKENABLED), "Checkbox Enable Condition:"},
        {new Integer(CHECKVALUE),   "Checkbox Value:"},
        {new Integer(CHECKCMD),     "Checkbox Vnmr Command:"},
        {new Integer(CHECKCMD2),    "Checkbox Vnmr Command2:"},
        {new Integer(ENTRYVALUE),   "Entry value:"},
        {new Integer(ENTRYSIZE),    "Entry size:"},
        {new Integer(ENTRYCMD),     "Entry Vnmr Command:"},
        {new Integer(NUMDIGIT),     "Entry Decimal Places:"},
        {new Integer(DISABLE),      "Entry Disable Style:", "menu", VEntry.m_arrStrDisAbl},
        {new Integer(UNITSENABLED), "Units Enable:", "menu", UNITS},
        {new Integer(UNITSSIZE),    "Units size:"},
        {new Integer(UNITSLABEL),   "Units Label:"},
        {new Integer(UNITSVALUE),   "Units value:"},
        {new Integer(UNITSCMD),     "Units Vnmr Command:"},
        {new Integer(CHVAL),       "Menu Choice Label:"},
        {new Integer(VALUES),        "Menu Choice Value:"},
    };

    class VParameterLayout implements LayoutManager
    {

        private Dimension psize;
        private int maxH;

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public Dimension preferredLayoutSize(Container target) {
            int nmembers = target.getComponentCount();
            int w = 0;
            int h = 0;
            Point pt;
            for (int i = 0; i < nmembers; i++) {
                Component comp = target.getComponent(i);
                if  (comp != null) {
                    psize = comp.getPreferredSize();
                    pt = comp.getLocation();
                    if (pt.x + psize.width > w)
                        w = pt.x + psize.width;
                    if (pt.y + psize.height > h)
                        h = pt.y + psize.height;
                }
            }
            prefDim.width = w + 2;
            prefDim.height = h + 2;
            return prefDim;
        } //preferredLayoutSize()

        private int setObjBounds(Component comp, int w, int h, int mode) {
            if (!comp.isVisible())
               return w;
            int x, y;
            Dimension size = comp.getSize();
            y = 0;
            if (mode > 0) {
                psize = comp.getPreferredSize();
                if (psize != null) {
                   if (psize.width > 8) {
                      if (mode == 1)
                          size.width = psize.width;
                      else if (psize.width > size.width)
                          size.width = psize.width;
                   }
                   if (psize.height > 8) {
                      if (mode == 1)
                          size.height = psize.height;
                      else if (psize.height > size.height)
                          size.height = psize.height;
                   }
                }
            }
            x = w - size.width;
            if (x < 0)
                x = 0;
            if (size.height > h)
                size.height = h;
            if (mode == 1) {
                if (maxH > size.height)
                   y = (maxH - size.height) / 2;
            }
            if (size.height > maxH)
                maxH = size.height;
            comp.setBounds(x, y, size.width, size.height);
            return (x - 2);
        }

        public void layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
                int w = target.getWidth();
                int h = target.getHeight();
                Dimension size;
                Component comp;

                maxH = 0;
                if (m_compUnits != null) {
                    comp = (Component) m_compUnits;
                    if (m_compUnits instanceof VMenu) {
                        comp.setPreferredSize(null);
                        w = setObjBounds(comp, w, h, 2);
                    }
                    else
                        w = setObjBounds(comp, w, h, 0);
                }
                if (m_entry != null) {
                    w = setObjBounds((Component) m_entry, w, h, 0);
                }
                if (m_check != null) {
                    w = setObjBounds((Component) m_check, w, h, 1);
                }
                if (m_label == null || !m_label.isVisible())
                    return;
                comp = (Component) m_label;
                size = comp.getSize();
                w -= 2;
                if (w < 6)
                    w = 6;
                size.width = w;
                if (size.height > h)
                    size.height = h;
                comp.setBounds(0, 0, size.width, size.height);
            }
        }


      /***********************
        public void old_layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
                int x = target.getWidth();
                Point loc;
                int count = target.getComponentCount();
                members = 0;
                Dimension size;
                Dimension sizeLabel = new Dimension(0, 0);
                for (int i = 0; i < count; i++) {
                    Component comp = target.getComponent(i);
                    if  ((comp != null) && comp.isVisible()) {
                        if (inEditMode)
                            psize = comp.getPreferredSize();
                        else
                            psize = comp.getSize();
                        if (comp != m_label && m_label != null)
                            sizeLabel = inEditMode ? m_label.getPreferredSize() :
                                        m_label.getSize();
                        loc = comp.getLocation();
                        if (comp == m_label)
                        {
                            loc.x = 0;
                            if (m_check != null && m_check.isVisible())
                            {
                                int x2 = m_check.getLocation().x;
                                if (psize.width > x2)
                                    psize.width = x2;
                                else
                                {
                                    int width = psize.width + 20;
                                    if (x2 > width)
                                        psize.width = width;
                                }
                            }
                        }
                        else if (comp == m_check)
                        {
                            int x2 = x;
                            if (m_compUnits != null && ((JComponent)m_compUnits).isVisible())
                            {
                                size = inEditMode ? ((JComponent)m_compUnits).getPreferredSize() :
                                       ((JComponent)m_compUnits).getSize();
                                x2 = x2 - size.width;
                            }
                            if (m_entry != null)
                            {
                                size = inEditMode ? m_entry.getPreferredSize() :
                                       m_entry.getSize();
                                x2 = x2 - size.width;
                            }
                            x2 = x2 - psize.width;
                            loc.x = x2;
                            if (sizeLabel.width > loc.x)
                                m_label.setBounds(0, 0, x2, sizeLabel.height);
                            else
                            {
                                x2 = sizeLabel.width + 20;
                                if (loc.x > x2)
                                    m_label.setBounds(0, 0, x2, sizeLabel.height);
                            }
                        }
                        else if (comp == m_entry)
                        {
                            int x2 = x;
                            if (m_compUnits != null && ((JComponent)m_compUnits).isVisible())
                            {
                                size = inEditMode ? ((JComponent)m_compUnits).getPreferredSize() :
                                       ((JComponent)m_compUnits).getSize();
                                x2 = x2 - size.width;
                            }
                            x2 = x2 - psize.width;
                            loc.x = x2;
                            if (sizeLabel.width > loc.x)
                                m_label.setBounds(0, 0, x2, sizeLabel.height);
                            else
                            {
                                x2 = sizeLabel.width + 20;
                                if (loc.x > x2)
                                    m_label.setBounds(0, 0, x2, sizeLabel.height);
                            }
                        }
                        else if (comp == m_compUnits)
                        {
                            int x2 = x - psize.width;
                            loc.x = x2;
                        }
                        //loc = comp.getLocation();
                        comp.setBounds(loc.x, loc.y, psize.width, psize.height);
                        members++;
                    }
                }
            }
        }
      ***********************/
    } // end of class VParameterLayout

}



