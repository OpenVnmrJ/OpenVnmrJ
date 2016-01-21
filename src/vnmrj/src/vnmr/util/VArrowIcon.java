/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.Graphics;
import java.awt.Insets;
import java.awt.Color;
import java.awt.Component;
import javax.swing.*;
import javax.swing.border.*;

public class VArrowIcon implements Icon
{
    private Color color;
    private int width;
    private int height;

    public VArrowIcon(int width, int height) {
	this.width = width;
	this.height = height;
    }

    /**
     * Returns the icon's height.
     */
    public int getIconHeight() {
	return height;
    }

    /**
     * Returns the icon's width.
     */
    public int getIconWidth() {
	return width;
    }

    /**
     * Draw the rectangle at the specified location in the current color.
     */
    public void paintIcon(Component c, Graphics g, int x, int y) {
    int[] xp=new int[3];
    int[] yp=new int[3];
    
    xp[0]=x;
    xp[1]=x+width/2;
    xp[2]=x+width;
 
    yp[0]=x+height;
    yp[1]=x;
    yp[2]=x+height;
      
	g.setColor( color == null ? c.getBackground() : color );
	g.fillPolygon(xp, yp, 3);
	g.setColor(Color.black);
	g.drawPolygon(xp, yp, 3);
    }

    /**
     * Set the color.
     */
    public void setColor(Color c) {
	color = c;
    }

    /**
     * Get the color.
     */
    public Color getColor() {
	return color;
    }
}
