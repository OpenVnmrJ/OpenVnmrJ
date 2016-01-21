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
import javax.swing.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.ui.*;

/**
 * Title:   WGroup
 * Description: This class is extended from VGroup and has some additional
 *              variables and methods implemented for Wanda interface.
 * Copyright:    Copyright (c) 2002
 */

public class WGroup extends VGroup implements WObjIF
{
    protected String m_strKeyWord = null;
    protected String m_strValue   = null;
    protected String m_strRow     = null;
    protected String m_strColumn  = "2"; // number of columns in the panel

    protected GridBagLayout m_gbLayout = new GridBagLayout();
    protected GridBagConstraints m_gbConstraints = new GridBagConstraints();
    protected int m_nCompIndex = 0;

    public WGroup(SessionShare ss, ButtonIF vif, String typ)
    {
        super(ss, vif, typ);
        setLayout(m_gbLayout);
    }

    public Component add(Component comp)
    {
        dolayout(comp);
        Component newComp = super.add(comp);
        m_nCompIndex++;
        return newComp;
    }

    protected void dolayout(Component comp)
    {
        int nCol = 2;
        try
        {
            nCol = Integer.parseInt(m_strColumn);
        }
        catch (Exception e) {}
        int nWidth = (m_nCompIndex%nCol == 0) ? m_gbConstraints.RELATIVE
                                      : m_gbConstraints.REMAINDER;
        setLayout( comp,m_nCompIndex/nCol, m_nCompIndex%nCol, nWidth, 1, 1, 1 );
        if (comp instanceof VObjIF)
            ((VObjIF)comp).setAttribute(BGCOLOR, getAttribute(BGCOLOR));

        revalidate();
        repaint();
    }

    protected void setLayout(Component comp, int row, int col, int width, int height, int weightX, int weightY)
    {
        if (comp instanceof JLabel)
        {
            JLabel lblComp = (JLabel)comp;
            String strText = ((JLabel)comp).getText();
            lblComp.setText(strText.trim());
        }

        m_gbConstraints.gridx = col;
        m_gbConstraints.gridy = row;
        m_gbConstraints.gridwidth = width;
        m_gbConstraints.gridheight = height;
        m_gbConstraints.weightx = weightX;
        m_gbConstraints.weighty = weightY;
        m_gbConstraints.fill = GridBagConstraints.HORIZONTAL;

        m_gbLayout.setConstraints(comp, m_gbConstraints);
    }

    public String getAttribute(int nAttr)
    {
        switch (nAttr)
        {
            case KEYWORD:
                    return m_strKeyWord;
            case ROW:
                    return m_strRow;
            case COLUMN:
                    return m_strColumn;
            default:
                    return super.getAttribute(nAttr);
        }
    }

    public void setAttribute(int nAttr, String strAttr)
    {
        switch(nAttr)
        {
            case KEYWORD:
                    m_strKeyWord = strAttr;
                    setEntry(strAttr);
                    break;
            case BGCOLOR:
                    super.setAttribute(BGCOLOR, strAttr);
                    setCompsBgColor(strAttr);
                    break;
            case ROW:
                    m_strRow = strAttr;
                    break;
            case COLUMN:
                    m_strColumn = strAttr;
                    break;
            default:
                    super.setAttribute(nAttr, strAttr);
                    break;
        }

    }

    public String getValue()
    {
        String strValue = "";
        String strTmp = null;

        for (int i = 0; i < getComponentCount(); i++)
        {
            Component comp = getComponent(i);
            if (comp instanceof WCheck)
            {
                strValue = strValue + "~" + ((WCheck)comp).getValue();
            }
            else if (comp instanceof WObjIF)
            {
                strTmp = ((WObjIF)comp).getValue();
                if (strTmp != null)
                    strValue = strValue + strTmp;
            }
        }
        return strValue;
    }

    public void setValue(String strValue)
    {
        m_strValue = strValue;

        for (int i = 0; i < getComponentCount(); i++)
        {
            Component comp = getComponent(i);
            if (comp instanceof WCheck)
                strValue = setCheckValue(strValue, (WCheck)comp);
            else if (comp instanceof WObjIF)
                ((WObjIF)comp).setValue(strValue);
        }
    }

    /**
     *  Sets the value for the checkbox in a group.
     *  The value for the checkbox in a groups is in the following form:
     *      value1~value2~value3
     *  Therefore go through each value, and set the value for the respective
     *  checkbox.
     */
    protected String setCheckValue(String strValue, WCheck objCheck)
    {
        if (strValue == null)
            strValue = "";
        StringTokenizer strTok = new StringTokenizer(strValue, "~");
        String strNextTok = "";

        if (strTok.hasMoreTokens())
            strNextTok = strTok.nextToken();
        else
            strNextTok = "";

        objCheck.setValue(strNextTok);

        int nIndex = strValue.indexOf(strNextTok);
        int nLength = strNextTok.length();
        if (nIndex >= 0 && nLength > 0 && nLength+1 < strValue.length())
            strValue = strValue.substring(nLength+1);

        return strValue.trim();
    }

    /**
     *  Sets the value for the entry in a group.
     */
    protected void setEntry(String strAttr)
    {
        int nCompCount = getComponentCount();
        for (int i = 0; i < nCompCount; i++)
        {
            Component comp = getComponent(i);
            if (comp instanceof WEntry)
                ((WEntry)comp).setAttribute(KEYWORD, strAttr);
        }
    }

    protected void setCompsBgColor(String strAttr)
    {
        int nCompCount = getComponentCount();

        for (int i = 0; i < nCompCount; i++)
        {
            Component comp = getComponent(i);
            if (comp instanceof WObjIF)
                ((WObjIF)comp).setAttribute(BGCOLOR, strAttr);
        }
    }


}
