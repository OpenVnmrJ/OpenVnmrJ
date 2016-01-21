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
 * Title:   WToggle
 * Description: This class is extended from VToggle and has some additional
 *              variables, and methods implemented for Wanda interface.
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class WToggle extends VToggle implements WObjIF
{
    protected String m_strKeyWord = null;
    protected String m_strValue   = null;

    public WToggle(SessionShare ss, ButtonIF vif, String typ)
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
            default:
                    super.setAttribute(nAttr, strAttr);
                    break;
        }

    }

    public String getValue()
    {
        String strValue = "0";
        if (this.isSelected())
            strValue = "1";

        return strValue;

    }

    public void setValue(String strValue)
    {
        m_strValue = strValue;
        if (m_strValue != null)
        {
            if (m_strValue.equals("0"))
                setSelected(false);
            else
                setSelected(true);
        }
    }


}
