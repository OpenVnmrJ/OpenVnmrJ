/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  unascribed
 *
 */

public class VSelFileMenu extends VFileMenu
{


    public VSelFileMenu(SessionShare ss, ButtonIF vif, String typ)
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
     *  Returns true so that the ui could display the default label as the title
     *  of the combobox everytime.
     */
    public boolean getDefault()
    {
        return true;
    }

}
