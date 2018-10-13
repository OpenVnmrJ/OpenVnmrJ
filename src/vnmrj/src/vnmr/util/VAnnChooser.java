/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.util.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.text.*;

import vnmr.bo.*;

public class VAnnChooser extends JComboBox implements ActionListener,
    VObjDef
{
    private String sel=null;
    Hashtable types=new Hashtable();
    private VObjIF sobj=null;
    private boolean color_only=false;
    Font font;
    Color fgColor;
    Color bgColor;
    Color orgBg;
    boolean inAddMode = true;
    boolean inSetMode = false;

    public VAnnChooser(String type) {
        inAddMode=true;
        orgBg=getBackground();
        if(type.equals("color"))
            setColorOnly(true);
        ArrayList list = DisplayOptions.getSymbols();
        for(int i=0;i<list.size();i++){
            String s=(String)list.get(i);
            addItem(s);
            types.put(s,s);
        }
        if(color_only){
           addItem("transparent");
           types.put("transparent","transparent");
        }
        addActionListener(this);
        setRenderer(new StyleMenuRenderer());
        setPreferredSize(new Dimension(120,22));
        setOpaque(false);
            inAddMode=false;
    }

    public void actionPerformed(ActionEvent evt) {
        if(inAddMode)
            return;
        sel = (String)getSelectedItem();
        StyleData style=new StyleData(sel);
        setFont(style.font);
        setBackground(style.bg);
        setForeground(style.fg);
        if(!inSetMode && sobj!=null)
                setStyle(sobj);
        repaint();
    }

    public void setColorOnly(boolean c) {
       color_only=c;
    }

    public void setStyleObject(VObjIF v) {
        sobj=v;
    }

    public String getType() {
        return (String)getSelectedItem();
    }

    public void setType(String s) {
        setSelectedItem(s);
    }

    public void setDefaultType() {
        setSelectedIndex(0);
    }

    public boolean isType(String s) {
        return types.containsKey(s);
    }

    public void setStyle(VObjIF obj) {
        if(obj==null)
            return;
        sel = (String)getSelectedItem();
        obj.setAttribute(FONT_STYLE, sel);
        obj.setAttribute(FGCOLOR, sel);
        obj.changeFont();
    }

    public void setColor(VObjIF obj) {
        if(obj==null)
            return;
        sel = (String)getSelectedItem();
        obj.setAttribute(FGCOLOR, sel);
    }

    class StyleData {
        public Color fg;
        public Color bg;
        public Font font;
        float f1[];
        float f2[];

        public StyleData(String sel){
            f1 = new float[3];
            f2 = new float[3];
            if(color_only){
                font=DisplayOptions.getFont(null,null,null);
                bg=DisplayOptions.getColor(sel);
                if(intensity(bg)<=1)
                    fg=Color.white;
                else
                    fg=Color.black;
            }
            else{
                font=DisplayOptions.getFont(sel,sel,sel);
                fg=DisplayOptions.getColor(sel);
                bg=orgBg;
                if(contrast(fg,bg)<0.3){
                    bg=DisplayOptions.getColor(sel);
                    if(intensity(bg)<=1)
                        fg=Color.white;
                    else
                        fg=Color.black;
                }
            }
        }

        double intensity(Color c){
            if (c == null)
               return Double.NaN;
            c.getColorComponents(f1);
            double r=f1[0];
            double g=f1[1];
            double b=f1[2];
            double mag=(r*r+g*g+b*b);
            return mag;
        }

        double contrast(Color c1,Color c2){
            if (c1 == null || c2 == null) {
                    return Double.NaN;
            }
            c1.getColorComponents(f1);
            c2.getColorComponents(f2);
            double r=f1[0]-f2[0];
            double g=f1[1]-f2[1];
            double b=f1[2]-f2[2];
            double mag=(r*r+g*g+b*b);
            return mag;
        }
    } // class StyleData

    // cell renderer for menu items

    public class StyleMenuRenderer extends JLabel implements ListCellRenderer {
        public StyleMenuRenderer() {
            setOpaque(true);
        }
        public Component getListCellRendererComponent(
            JList list,
            Object value,
            int index,
            boolean isSelected,
            boolean cellHasFocus)
        {
            if (value == null)
                return this;
            String sel=value.toString();
            if (sel == null)
                return this;
            StyleData style=new StyleData(sel);
            setText(sel);
            setFont(style.font);
            setBackground(style.bg);
            setForeground(style.fg);
            return this;
        }
    } // class StyleMenuRenderer
}


