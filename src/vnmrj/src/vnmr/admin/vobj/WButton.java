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
 * Title:   WButton
 * Description: This class is extended from VButton, and has some additional
 *              variables and methods implemented for the Wanda interface.
 * Copyright:    Copyright (c) 2002
 */

public class WButton extends VButton implements WObjIF
{

    protected String m_strKeyWord = null;
    protected String m_strValue   = null;

    public WButton(SessionShare ss, ButtonIF vif, String typ)
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
        return getAttribute(VALUE);
    }

    public void setValue(String strValue)
    {
        label = strValue;
        setText(strValue);
    }
}
