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
import java.lang.*;
import javax.swing.*;
import java.awt.image.BufferedImage;

public class VTextIcon implements Icon
{
    static final double deg90 = Math.toRadians(90.0);
    String label;
    int    rotation = 0;
    int    fontH = 0;
    int    ftDescent = 0;
    int    ftAscent = 10;
    int    width = 0;
    int    height = 0;
    Component comp;
    Font   font;
    private BufferedImage  img = null;

    public VTextIcon(Component component, String label, int rotate) {
         this.comp = component;
         this.label = label;
         this.rotation = rotate;
         font = comp.getFont().deriveFont(12.0f);
         calSize();
    }

    public VTextIcon(Component component, String label) {
         this(component, label, 0);
    }

    public void setName(String s) {
         if (s != null) {
            if (!s.equals(label)) {
               label = s;
               calSize();
            }
         }
    }

    public void calSize() {
        if (label == null)
           return;
        FontMetrics fm = comp.getFontMetrics(font);
        ftDescent = fm.getDescent();
        ftAscent = fm.getAscent();
        fontH = ftAscent + ftDescent;
        int  w = fm.stringWidth(label) + 6;
        if (rotation == 0) {
            width = w;
            height = fontH;
            return;
        }
        else {
            width = fontH;
            height = w;
        }
        if (!Util.isMacOs())
            return;
        // Mac Os has problem of drawing vertical string
        img = new BufferedImage(w, fontH, BufferedImage.TYPE_INT_ARGB);
        if (img == null)
            return;
        Graphics2D g = img.createGraphics();
        Color c = new Color(200,200,200, 0);
        g.setBackground(c);
        g.setFont(font);
        g.clearRect(0, 0, w, fontH);
        if (comp != null)
            g.setColor(comp.getForeground());
        else
            g.setColor(Color.black);
        g.drawString(label, 0, ftAscent);
    }

    public int getIconWidth() {
        return width;
    }

    public int getIconHeight() {
        return height;
    }

    public void paintIcon(Component c, Graphics g, int x, int y) {
        if (label == null)
            return;
        g.setColor(c.getForeground());
        g.setFont(font);
        Graphics2D g2d = (Graphics2D) g;
        if (rotation == SwingConstants.RIGHT) {
            if (img != null)
                x = x + ftAscent;
            g.translate(x,y);
            g2d.rotate(deg90);
            if (img != null)
                g.drawImage(img,0, 0, null);
            else 
                g.drawString(label, 0, 0);
            g2d.rotate(-deg90);
            g.translate(-x,-y);
        }
        else if (rotation == SwingConstants.LEFT) {
            y = y + height;
            if (img == null)
                x = x + ftAscent;
            g.translate(x, y);
            g2d.rotate(-deg90);
            if (img != null)
                g.drawImage(img,0, 0, null);
            else
                g.drawString(label, 0, 0);
            g2d.rotate(deg90);
            g.translate(-x,-y);
        }
        else {
            g.drawString(label, x, y + ftAscent);
        }
    }
}
