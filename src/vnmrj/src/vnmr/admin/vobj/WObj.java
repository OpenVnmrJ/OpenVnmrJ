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
 * Title:   WObj
 * Description: The wobj object is extended from vobj and has some additional
 *              methods and variables needed for the Wanda interface.
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class WObj extends VObj implements WObjIF
{

    protected String m_strKeyWord = null;
    protected SessionShare m_sshare;

    public WObj(SessionShare sshare)
    {
        this(sshare, null, "port");
        m_sshare = sshare;
    }

    public WObj(SessionShare sshare, ButtonIF vif, String typ)
    {
        super(sshare, vif, typ);
        m_sshare = sshare;
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

    public void setValue(String strVal)
    {
        //setvalue();

    }

    public SessionShare getsshare()
    {
        return m_sshare;
    }

}
