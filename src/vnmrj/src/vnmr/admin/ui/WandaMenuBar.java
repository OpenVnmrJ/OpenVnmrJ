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
import java.awt.*;
import java.beans.*;
import java.util.*;
import java.awt.event.*;
import javax.swing.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.bo.*;
import vnmr.admin.util.*;


/**
 * Title:   WandaMenuBar
 * Description: This class extends from VMenuBar, and has some additional
 *              variables and methods needed for Wanda interface.
 * Copyright:    Copyright (c) 2001
 * Company:
 * @author
 * @version 1.0
 */

public class WandaMenuBar extends VMenuBar implements PropertyChangeListener
{

    public WandaMenuBar(SessionShare sh, ButtonIF vif, int num)
    {
        super(sh, vif, num, "AdminMenu.xml");

        setBgColor();
    }

    public void initChngListeners()
    {
        WFontColors.addChangeListener(this);
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        int k = getMenuCount();
        for (int i = 0; i < k; i++)
        {
            JMenu m = getMenu(i);
            if (m != null && (m instanceof VMenuitemIF))
            {
                VMenuitemIF vf = (VMenuitemIF) m;
                vf.changeLook();
            }
        }
        setBgColor();
    }

    protected void setBgColor()
    {
        String strColName=WFontColors.getColorOption(WGlobal.ADMIN_BGCOLOR);
        Color colBG = (strColName != null) ? WFontColors.getColor(strColName)
                                            : Global.BGCOLOR;
        setBackground(colBG);
        WUtil.doColorAction(colBG, strColName, this);
    }

}
