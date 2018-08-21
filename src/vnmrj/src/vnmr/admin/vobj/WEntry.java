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
import java.awt.event.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

/**
 * Title:   WEntry
 * Description: This class is extended from VEntry and has some additional
 *              variables and methods implemented for the Wanda interface.
 * Copyright:    Copyright (c) 2002
 */

public class WEntry extends VEntry implements WObjIF
{

    protected SessionShare m_sshare = null;
    protected String m_strKeyWord = null;
    protected String m_strValue   = null;
    protected String m_strEdit    = null;

    public WEntry(SessionShare ss, ButtonIF vif, String typ)
    {
        super(ss, vif, typ);

        m_sshare = ss;

        addFocusListener(new FocusAdapter()
        {
            public void focusLost(FocusEvent e)
            {
                if (!e.isTemporary())
                    doAction();
            }
        });

        addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                doAction();
            }
        });
    }

    public String getAttribute(int nAttr)
    {
        switch (nAttr)
        {
            case KEYWORD:
                    if (m_strKeyWord == null)
                    {
                        Container container = getParent();
                        if (container != null && container instanceof WGroup)
                            m_strKeyWord = ((WGroup)container).getAttribute(KEYWORD);
                    }
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
                    setLoginObj(m_strKeyWord);
                    break;
            case EDITABLE:
                     if (strAttr.equals("yes") || strAttr.equals("true"))
                     {
                        setEditable(true);
                        m_strEdit = "yes";
                     }
                     else
                     {
                        setEditable(false);
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
        return getText();
    }

    public void setValue(String strValue)
    {
        m_strValue = strValue;
        if (!getText().equals(m_strValue))
        {
            if (m_strValue != null)
                m_strValue = m_strValue.trim();
            setText(m_strValue);
        }
    }

    protected void setLoginObj(String strKey)
    {
        if (strKey.equals(WGlobal.NAME))
            WCurrInfo.setCurrLoginObj(this);
    }

    protected void doFocusLostAction(FocusEvent e)
    {
        String strValue = getText();
        String strAttr  = getAttribute(KEYWORD);
        if (strAttr.equalsIgnoreCase(WGlobal.NAME)
                && !(strValue.equals(m_strValue)))
        {
            setValue(strValue);
            Container comp = getParent();
            if (comp instanceof VDetailArea)
            {
                VDetailArea pnlArea = (VDetailArea)comp;
                String strArea = pnlArea.getName();
                if (strArea.equals(WGlobal.AREA1))
                    VItemArea1.firePropertyChng(strArea, strValue);
                else if (strArea.equals(WGlobal.AREA2))
                    VItemArea2.firePropertyChng(strArea, strValue);
            }
        }

    }

    protected boolean isparsedValue(String struser, HashMap hmDef)
    {
        HashMap hmUser = WFileUtil.getHashMapUser(struser);
        if (hmUser == null || hmUser.isEmpty())
            return false;

        String strValue = "$"+m_strKeyWord;
        Iterator iter = hmDef.keySet().iterator();
        while (iter.hasNext())
        {
            String strkey = (String)iter.next();
            String strvalue = WFileUtil.lookUpValue(hmDef, strkey);
            if (strvalue != null && strvalue.indexOf(strValue) > 0)
                return false;
        }

        return true;
    }

    /**
     *  If 'return' is pressed in the entry which is the name of the user,
     *  then enter the defaults for that user.
     */
    protected void doAction()
    {
        if (m_strKeyWord != null /*&& m_strKeyWord.equalsIgnoreCase(WGlobal.NAME)*/
                && isEditable())
        {
            AppIF appIf = Util.getAppIF();
            String strValue = getValue();
            boolean bName = false;
            boolean bOk = true;

            if ((appIf instanceof VAdminIF))
            {
                HashMap hmDef = ((VAdminIF)appIf).getUserDefaults();
                String strPropName = WGlobal.AREA1 + WGlobal.SEPERATOR + WGlobal.BUILD;

                // if the keyword is the name, then the values of the other
                // fields should be filled based on the defaults and the login name
                if (m_strKeyWord.equalsIgnoreCase(WGlobal.NAME))
                {
                    bOk = (strValue != null && strValue.trim().length() > 0)
                            ? true : false;
                    if (strValue != null)
                        strValue = strValue.trim();
                    if (strValue == null || strValue.indexOf(" ") >= 0)
                        return;
                    // check if the unix user already exists,
                    // if it does, then use it's home account
                    if (bOk && WUserUtil.isUnixUser(strValue))
                    {
                        WUserUtil.setUserHome(strValue, hmDef);
                    }
                    bName = true;
                }
                // else the keyword is something else, get the name entered
                // from detail area, and put it in the hashmap, and put
                // the value of the keyword in the hashmap.
                else
                {
                    VDetailArea pnlArea = ((VAdminIF)appIf).getDetailArea1();
                    String struser = pnlArea.getNameEntered();
                    if (struser == null || struser.indexOf(" ") >= 0 ||
                        isparsedValue(struser, hmDef))
                        return;
                    hmDef.put(WGlobal.NAME, struser);
                    hmDef.put(m_strKeyWord, strValue);
                    if (WUserUtil.isUnixUser(struser))
                       WUserUtil.setUserHome(struser, hmDef);
                    Iterator iter = hmDef.keySet().iterator();
                    while (iter.hasNext())
                    {
                        String strKey = (String)iter.next();
                        String strValue2 = WFileUtil.lookUpValue(hmDef, strKey);
                        if (strValue2.indexOf("$") < 0)
                        {
                            strValue2 = pnlArea.getItemValue(strKey);
                            hmDef.put(strKey, strValue2);
                        }
                    }
                    bName = false;
                    strValue = struser;
                }

                //WUserUtil.setAppdir(hmDef);
                if (bOk)
                    VItemArea1.firePropertyChng(strPropName, strValue, hmDef, bName);
            }

        }
    }

}
