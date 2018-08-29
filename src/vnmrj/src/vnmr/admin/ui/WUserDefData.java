/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;

/**
 * Title:  WUserDefData
 * Description: This file contains the user default data.
 * Copyright:    Copyright (c) 2002
 */

public class WUserDefData
{
    /** The value for the field e.g. value for home is '/export/home'   */
    protected String m_strValue     = null;

    /** The show condition for the field either 'yes' or 'no' e.g. show condition
        for username is 'no', which says don't show the field in the panel.  */
    protected String m_strShow      = null;

    /** The private condition for the field either 'yes' or 'no' e.g. private condition
        for username is 'yes', which says that this field should be stored as a private
        field, and private condition for home is 'no', which says that this field
        should be stored as a public field.     */
    protected String m_strPrivate   = null;

    protected final static String PRIVATE = "private";
    protected final static String SHOW    = "show";
    protected final static String REQ     = "required";

    public WUserDefData(String strValue, String strShow,
                            String strPrivate)
    {
        m_strValue = strValue;
        m_strShow = strShow;
        m_strPrivate = strPrivate;
    }

    public void setValue(String value)
    {
        m_strValue = value;
    }

    public String getValue()
    {
        return m_strValue;
    }

    public void setShow(String show)
    {
        m_strShow = show;
    }

    public String getShow()
    {
        return m_strShow;
    }

    public void setPrivate(String privat)
    {
        m_strPrivate = privat;
    }

    public String getPrivate()
    {
        return m_strPrivate;
    }

    public static ArrayList getPrvFields(HashMap hmDef)
    {
        return getPublic(hmDef, false);
    }

    public static ArrayList getPbFields(HashMap hmDef)
    {
        return getPublic(hmDef, true);
    }

    public static ArrayList getShowFields(HashMap hmDef)
    {
        Iterator keySetItr = hmDef.keySet().iterator();
        String strKey = null;
        ArrayList aListShow = new ArrayList();

        while(keySetItr.hasNext())
        {
            strKey = (String)keySetItr.next();
            if (isKeyShow(strKey, hmDef))
                aListShow.add(strKey);
        }
        return aListShow;
    }

    public static ArrayList getReqFields(HashMap hmDef)
    {
        Iterator keySetItr = hmDef.keySet().iterator();
        String strKey = null;
        ArrayList aListReq = new ArrayList();

        while(keySetItr.hasNext())
        {
            strKey = (String)keySetItr.next();
            if (isKeyReq(strKey, hmDef))
                aListReq.add(strKey);
        }
        return aListReq;
    }

    public static boolean isKeyPublic(String strKey, HashMap hmDefaults)
    {
        boolean bPrv = getValue(strKey, hmDefaults, PRIVATE);
        return !bPrv;
    }

    public static boolean isKeyShow(String strKey, HashMap hmDefaults)
    {
        return getValue(strKey, hmDefaults, SHOW);
    }

    public static boolean isKeyReq(String strKey, HashMap hmDefaults)
    {
        return getValue(strKey, hmDefaults, REQ);
    }

    protected static ArrayList getPublic(HashMap hmDef, boolean bGetPb)
    {
        Iterator keySetItr = hmDef.keySet().iterator();
        String strKey = null;
        ArrayList aListFields = new ArrayList();
        boolean bPublic = false;

        while(keySetItr.hasNext())
        {
            strKey = (String)keySetItr.next();
            bPublic = isKeyPublic(strKey, hmDef);

            // if the key is public and bGetPb is true, which means that the
            // public field list is required, add it to the list
            // or if the key is private and bGetPb is false, which means that the
            // private field list is required, add it to the list.
            if ((bGetPb && bPublic) || (!bGetPb && !bPublic))
                aListFields.add(strKey);
        }
        return aListFields;
    }

    private static boolean getValue(String strKey, HashMap hmDef, String strLabel)
    {
        String strValue = "yes";
        Object objDef = hmDef.get(strKey);
        if (objDef instanceof WUserDefData)
        {
            WUserDefData userDef = (WUserDefData)objDef;
            if (strLabel.equals(PRIVATE))
                strValue = userDef.getPrivate();
            else if (strLabel.equals(SHOW))
                strValue = userDef.getShow();
            /*else if (strLabel.equals(REQ))
                strValue = userDef.getReq();*/
        }

        boolean bValue = (strValue.equalsIgnoreCase("yes") || strValue.equalsIgnoreCase("true"))
                            ? true : false;

        return bValue;
    }

}
