/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

/**
 * Title: VSelMenu
 * Description: Specialized Menu class
 * Copyright:    Copyright (c) 2001
 */

import java.io.*;
import java.util.*;
import javax.swing.*;
import java.awt.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 *  This menu is just like VMenu except the selected item always displays:
 *  "Make a selection", so that you have to make a selection each and every time.
 */
public class VSelMenu extends VMenu
{
    /**
     *  Constructor.
     */
    public VSelMenu(SessionShare ss, ButtonIF vif, String typ)
    {
        super(ss, vif, typ);
        m_strDefault = "Make a selection";
        setSelMode(true);
    }

    public void setAttribute(int nAttr, String strAttr)
    {
        switch (nAttr)
        {
            case LABEL:
                super.setAttribute(nAttr, strAttr);
                m_strDefault = titleStr;
                break;
            default:
                super.setAttribute(nAttr, strAttr);
                break;

            }
    }

    /**
     *  Returns true so that the ui could display the default titleStr as the title
     *  of the combobox everytime.
     */
    public boolean getDefault()
    {
        if (titleStr != null && titleStr.length() > 0)
            return true;
        return false;
    }
}
