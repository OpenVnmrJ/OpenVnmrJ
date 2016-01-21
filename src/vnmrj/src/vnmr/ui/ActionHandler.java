/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class ActionHandler
{
    protected ArrayList aListActionListener = new ArrayList();

    public ActionHandler()
    {

    }

    public void addActionListener(ActionListener a)
    {
        aListActionListener.add(a);
    }

    public void removeActionListener(ActionListener a)
    {
        aListActionListener.remove(a);
    }

    public ArrayList getListener()
    {
        return aListActionListener;
    }

    public void fireAction(ActionEvent e)
    {
        if (aListActionListener.isEmpty())
            return;

        ActionListener a;
        for (int i = 0; i < aListActionListener.size(); i++)
        {
            a = (ActionListener)aListActionListener.get(i);
            fireAction(e, a);
        }
    }

    public void fireAction(ActionEvent e, ActionListener a)
    {
        a.actionPerformed(e);
    }

}

