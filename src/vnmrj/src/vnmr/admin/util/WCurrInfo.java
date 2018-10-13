/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.io.*;
import java.util.*;

import vnmr.bo.*;
import vnmr.admin.vobj.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class WCurrInfo
{

    protected static WObjIF m_objLogin = null;
    protected WSubMenuItem m_objMenuItem;
    protected String m_strMenuLabel;

    public WCurrInfo()
    {

    }

    public static String getCurrLogin()
    {
        return m_objLogin.getValue();
    }

    public static void setCurrLoginObj(WObjIF obj)
    {
        m_objLogin = obj;
    }

    public String getSelMenuLabel()
    {
        return (m_objMenuItem != null ? m_objMenuItem.getAttribute(VObjDef.LABEL)
                        : m_strMenuLabel);
    }

    public void setSelMenuLabel(String strLabel)
    {
        m_strMenuLabel = strLabel;
    }

    public WSubMenuItem getSelMenuItem()
    {
        return m_objMenuItem;
    }

    public void setMenuItem(WSubMenuItem objItem)
    {
        m_objMenuItem = objItem;
        if (objItem != null)
            m_strMenuLabel = objItem.getAttribute(VObjDef.LABEL);
    }
}
