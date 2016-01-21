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
 * Title:   WRadio
 * Description: This class is extended from VRadio and has some additional
 *              variables and methods implemented for Wanda interface.
 * Copyright:    Copyright (c) 2002
 */

public class WRadio extends VRadio implements WObjIF
{

    protected String m_strKeyWord = null;
    protected String m_strKeyVal = null;
    protected String m_strValue   = null;
    protected String m_strEdit    = null;

    public WRadio(SessionShare ss, ButtonIF vif, String typ)
    {
        super(ss, vif, typ);

        this.addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                doAction(false);
            }
        });

    	this.addMouseListener(new MouseAdapter() 
	{
       	    public void mouseClicked(MouseEvent evt) 
	    {
                doAction(true);
            }
    	});

    }

    public boolean isRequestFocusEnabled()
    {
        return true;
    }

    public String getAttribute(int nAttr) {
        switch(nAttr) {
        case KEYWORD:
            return m_strKeyWord;
        case KEYVAL:
            return m_strKeyVal;
        case EDITABLE:
            return m_strEdit;
        default:
            return super.getAttribute(nAttr);
        }
    }

    public void setAttribute(int nAttr, String strAttr) {
        switch(nAttr) {
        case KEYWORD:
            m_strKeyWord = strAttr;
            break;
        case KEYVAL:
            m_strKeyVal = strAttr;
            break;
        case EDITABLE:
            if(strAttr.equals("yes") || strAttr.equals("true")) {
                setEnabled(true);
                m_strEdit = "yes";
            } else {
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
        if (isSelected()){
            strValue = getAttribute(KEYVAL);
            if(strValue==null)
                strValue = getAttribute(LABEL);
        }
        return strValue;
    }

    public void setValue(String strValue)
    {
        m_strValue = strValue;
        if (m_strValue != null)
        {
            strValue=strValue.trim();
            if (strValue.equals(m_strKeyVal)||strValue.equals(label))
            {
                setSelected(true);
                doAction(false);
            }
            else
                setSelected(false);
        }
    }

    protected void doAction(boolean bResetAppDir)
    {
        Container compParent = getParent();
        if (m_strKeyWord == null && compParent instanceof WObjIF)
        {
            m_strKeyWord = ((WObjIF)compParent).getAttribute(KEYWORD);
        }

        if (m_strKeyWord != null && m_strKeyWord.equalsIgnoreCase("itype"))
        {
            AppIF appIf = Util.getAppIF();
            String strItype = getAttribute(KEYVAL);
            if(strItype==null)
                strItype=getValue();

            if (appIf instanceof VAdminIF)
            {
                VDetailArea pnlArea = ((VAdminIF)appIf).getDetailArea1();
                pnlArea.setAppDir(strItype, bResetAppDir);
            }
        }
    }

}
