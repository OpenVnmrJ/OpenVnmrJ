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
import java.awt.*;
import javax.swing.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;


public class WLoginBox extends LoginBox
{

    protected static WLoginBox m_loginBox;


    public WLoginBox()
    {
        super();

        m_lblUsername.setText("Administrator:");
        m_cmbUser.setEditable(false);
    }

    public static void showLoginBox(boolean bShow, String strHelpFile)
    {
        if (m_loginBox == null)
            m_loginBox = new WLoginBox();
        m_loginBox.setVisible(bShow, strHelpFile);
    }

    public void setVisible(boolean bShow)
    {
        if (bShow)
        {
            m_cmbUser.removeAllItems();
            Object[] aStrUser = getOperators();
            int nSize = aStrUser.length;
            for (int i = 0; i < nSize; i++)
            {
                m_cmbUser.addItem(aStrUser[i]);
            }
            setSize();
        }

        super.setVisible(bShow);
    }

    protected void enterLogin()
    {
        char[] password = m_passwordField.getPassword();
        String strUser = (String)m_cmbUser.getSelectedItem();
        setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
        boolean blogin = WUserUtil.isOperatorNameok(strUser, false);
        if (blogin)
        {
            blogin = unixPassword(strUser, password);
        }
        if (blogin)
        {
            m_lblLogin.setForeground(Color.black);
            m_lblLogin.setText("Login Successful");
            m_lblLogin.setVisible(true);
            setVisible(false);
            WUtil.setCurrentAdmin(strUser);
            WUserUtil.writeAuditTrail(new Date(), strUser,
                                      "Switched Current Administrator: " + strUser);
        }
        else
        {
            m_lblLogin.setForeground(WFontColors.getColor("Error"));
            //m_lblLogin.setText("<HTML>Incorrect username/password <p> Please try again </HTML>");
            m_lblLogin.setVisible(true);
        }
        setCursor(Cursor.getDefaultCursor());
    }

    protected void setSize()
    {
        VNMRFrame vnmrFrame = VNMRFrame.getVNMRFrame();
        Dimension size = vnmrFrame.getSize();
        if (vnmrFrame.isShowing())
            position = vnmrFrame.getLocationOnScreen();
        AppIF appIF = Util.getAppIF();
        int h = appIF.statusBar.getSize().height;
        width = size.width;
        height = size.height-h;
        setSize(width, height);
        setLocation(position);
    }

    protected String gettitle(String strFreq)
    {
        String strTitle = super.gettitle(strFreq);
        if (strTitle != null)
        {
            strTitle = strTitle.trim();
            if (strTitle.endsWith("-"))
                strTitle = strTitle.substring(0, strTitle.length()-1);
        }

        return strTitle;
    }

    protected Object[] getOperators()
    {
        ArrayList aListOperator = WUserUtil.getAdminList();
        return aListOperator.toArray();
    }

}
