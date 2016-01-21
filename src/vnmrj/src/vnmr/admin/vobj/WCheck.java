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

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

/**
 * Title:   WCheck
 * Description: This class is extended from VCheck, and has some additional
 *              variables and methods implemented for the Wanda interface.
 * Copyright:    Copyright (c) 2002
 */

public class WCheck extends VCheck implements WObjIF
{

    protected String m_strKeyWord = null;
    protected String m_strValue   = null;
    protected String m_strEdit    = null;

    public WCheck(SessionShare ss, ButtonIF vif, String typ)
    {
        super(ss, vif, typ);
    }

    public boolean isRequestFocusEnabled()
    {
        return true;
    }

    public String getAttribute(int nAttr)
    {
        switch (nAttr)
        {
            case KEYWORD:
                    return m_strKeyWord;
            case EDITABLE:
                    return m_strEdit;
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
                    break;
            case EDITABLE:
                     if (strAttr.equals("yes") || strAttr.equals("true"))
                     {
                        setEnabled(true);
                        m_strEdit = "yes";
                     }
                     else
                     {
                        setEnabled(false);
                        m_strEdit = "no";
                     }
                     break;
            default:
                    super.setAttribute(nAttr, strAttr);
                    break;
        }

    }

    public String getValue()
    {
        String strValue = " ";
        if (isSelected())
            strValue = getAttribute(LABEL);

        return strValue;
    }

    public void setValue(String strValue)
    {
        m_strValue = strValue;
        if (strValue != null)
        {
            if (strValue.trim().equals(label))
                setSelected(true);
            else
                setSelected(false);
        }
    }

}
