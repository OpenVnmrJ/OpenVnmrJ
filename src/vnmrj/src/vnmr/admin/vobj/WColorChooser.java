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
import javax.swing.*;
import java.awt.event.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.ui.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 *
 *
 *
 */

public class WColorChooser extends VColorChooser implements ActionListener
{

    public WColorChooser()
    {
        this((SessionShare)null,(ButtonIF)null,"colorchooser");
    }

    public WColorChooser(SessionShare sshare, ButtonIF vif, String typ)
    {
        super(sshare, vif, typ);

        addActionListener(this);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd=e.getActionCommand();
        if(cmd.equals("button"))
        {
            Color bg = getIconColor();
            Color newColor = JColorChooser.showDialog(this, "Choose Color", bg);
            if (newColor != null)
            {
                color = colorToString(newColor);
                setMenuChoice(color);
            }
        }
        else if (cmd.equals("menu"))
        {
            Object obj=menu.getSelectedItem();
            if(obj !=null)
            {
                color = menu.getSelectedItem().toString();
                setMenuChoice(color);
            }
        }
    }

    /** return a list of DisplayOption names. */
    public static Vector getVnmr()
    {
        if(vnmrcolors !=null)
            return vnmrcolors;
        vnmrcolors=new Vector(WFontColors.getTypes(DisplayOptions.COLOR));
        return vnmrcolors;
    }

    public String getAttribute(int nAttr)
    {
        switch(nAttr)
        {
            case KEYVAL:
                return color;
            default:
                return super.getAttribute(nAttr);
        }
    }

    public void setAttribute(int nAttr, String strValue)
    {
        switch (nAttr)
        {
            case KEYVAL:
                color = getColor();
                if (color == null)
                    color = strValue;
                setMenuChoice(color);
                break;
            case FGCOLOR:
                fg = strValue;
                fgColor=WFontColors.getColor(fg);
                repaint();
                break;
            case BGCOLOR:
                bg = strValue;
                if (strValue == null || strValue.length()==0 || strValue.equals("transparent")){
                    bgColor=null;
                    setOpaque(false);
                }
                else{
                    bgColor = WFontColors.getColor(strValue);
                    setOpaque(true);
                }
                setBackground(bgColor);
                repaint();
                break;
            default:
                super.setAttribute(nAttr, strValue);
                break;
        }
    }

    public String getColor()
    {
        String strKey = getAttribute(KEYSTR);
        String strColor = (strKey != null) ?  WFontColors.getColorOption(strKey)
                                : null;

        return strColor;
    }

    public void setColor(String s)
    {
         Color c=WFontColors.getColor(s);
         color=s;
         menu.setSelectedItem(s);
         ((VRectIcon)button.getIcon()).setColor(c);
    }

    protected void buildMenu(Vector list)
    {
    	super.buildMenu(list);
       // menu.setModel(new DefaultComboBoxModel(list));
       // String strValue = getAttribute(KEYVAL);
        setMenuChoice(getColor());
    }

}
