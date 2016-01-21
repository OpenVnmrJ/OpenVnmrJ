/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;
import java.awt.event.*;

public class XVButton extends XJButton implements MouseListener
{
    public Color bgColor; 

    public XVButton()
    {
        super();
    }

    public XVButton(Icon icon)
    {
        super(icon);
        setVButtonStyle();
    }

    public XVButton(String text)
    {
        super(text);
        setVButtonStyle();
    }

    public XVButton(Action a)
    {
        super(a);
        setVButtonStyle();
    }

    public XVButton(String text, Icon icon)
    {
        super(text, icon);
        setVButtonStyle();
    }

    private void setVButtonStyle() {
        set3D(false);
        bgColor = null;
        addMouseListener(this);
        setBackground(null);
        setBorderInsets(new Insets(4,8,4,8));
    }

    public void setBgColor(Color c) {
        bgColor = c;
        setBackground(c);
    }

    public Color getBgColor() {
        return bgColor;
    }

     public void mouseEntered(MouseEvent me) {
        if(getModel().isEnabled()) {
            Color bg = bgColor;
            if(bg != null || (bg = Util.getParentBackground(this)) != null) {
                setBackground(Util.changeBrightness(bg, 10));
                repaint();
            }
        }
    }

    public void mouseExited(MouseEvent me) {
        if(getModel().isEnabled()) {
            setBackground(bgColor);
            repaint();
        }
    }

    public void mouseClicked(MouseEvent me) {
    }

    public void mousePressed(MouseEvent me) {
    }

    public void mouseReleased(MouseEvent me) {
    }
}
