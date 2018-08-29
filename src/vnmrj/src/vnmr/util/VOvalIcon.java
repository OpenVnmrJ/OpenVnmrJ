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

public class VOvalIcon implements Icon
{
    private Color color;
    private int width;
    private int height;

    public VOvalIcon(int width, int height) {
	this.width = width;
	this.height = height;
    }

    public VOvalIcon(int width, int height, Color color) {
	this.width = width;
	this.height = height;
	this.color = color;
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
	g.setColor( color == null ? c.getBackground() : color );
	g.fillOval(x, y, width, height);
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
