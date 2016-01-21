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
import java.awt.Rectangle;
import java.awt.Color;
import java.awt.Component;
import javax.swing.border.*;

public class ParamBoder extends AbstractBorder
{
    /** Raised etched type. */
    public static final int RAISED  = 0;
    /** Lowered etched type. */
    public static final int LOWERED = 1;
    /** gap */
    protected int gapX = 1;
    protected int gapX2 = 1;

    protected int etchType;
    protected Color highlight;
    protected Color shadow;

    /**
     * Creates a lowered etched border whose colors will be derived
     * from the background color of the component passed into 
     * the paintBorder method.
     */
    public ParamBoder()    {
        this(LOWERED);
    }

    /**
     * Creates an etched border with the specified etch-type
     * whose colors will be derived
     * from the background color of the component passed into 
     * the paintBorder method.
     * @param etchType the type of etch to be drawn by the border
     */
    public ParamBoder(int etchType)    {
        this(etchType, null, null);
    }

    /**
     * Creates a lowered etched border with the specified highlight and
     * shadow colors.
     * @param highlight the color to use for the etched highlight
     * @param shadow the color to use for the etched shadow
     */
    public ParamBoder(Color highlight, Color shadow)    {
        this(LOWERED, highlight, shadow);
    }

    /**
     * Creates an etched border with the specified etch-type,
     * highlight and shadow colors.
     * @param etchType the type of etch to be drawn by the border
     * @param highlight the color to use for the etched highlight
     * @param shadow the color to use for the etched shadow
     */
    public ParamBoder(int etchType, Color highlight, Color shadow)    {
        this.etchType = etchType;
        this.highlight = highlight;
        this.shadow = shadow;
    }

    public void paintBorder(Component c, Graphics g, int x, int y, int width, int height) {
	int w = width;
	int h = height;
	
	g.translate(x, y);
	
	g.setColor(etchType == LOWERED? getShadowColor(c) : getHighlightColor(c));
	g.drawRect(0, 0, w-2, h-2);
	
	g.setColor(etchType == LOWERED? getHighlightColor(c) : getShadowColor(c));
	g.drawLine(1, h-3, 1, 1);
	g.drawLine(1, 1, w-3, 1);
	
	g.drawLine(0, h-1, w-1, h-1);
	g.drawLine(w-1, h-1, w-1, 0);
	
	g.setColor(c.getBackground());
	g.drawLine(gapX, 0, gapX2, 0);
	g.drawLine(gapX, 1, gapX2, 1);
	g.translate(-x, -y);
    }

    /**
     * Returns the insets of the border.
     * @param c the component for which this border insets value applies
     */
    public Insets getBorderInsets(Component c)       {
        return new Insets(2, 2, 2, 2);
    }

    /** 
     * Reinitialize the insets parameter with this Border's current Insets. 
     * @param c the component for which this border insets value applies
     * @param insets the object to be reinitialized
     */
    public Insets getBorderInsets(Component c, Insets insets) {
        insets.left = insets.top = insets.right = insets.bottom = 2;
        return insets;
    }

    /**
     * Returns whether or not the border is opaque.
     */
    public boolean isBorderOpaque() { return true; }

    /**
     * Returns which etch-type is set on the etched border.
     */
    public int getEtchType() {
        return etchType;
    }

    public void setGap(int x, int x2) {
	gapX = x;
	gapX2 = x2;
    }

   /**
     * Returns the highlight color of the etched border.
     */
    public Color getHighlightColor(Component c)   {
        return highlight != null? highlight : 
                                       c.getBackground().brighter();
    }

   /**
     * Returns the shadow color of the etched border.
     */
    public Color getShadowColor(Component c)   {
        return shadow != null? shadow : c.getBackground().darker();
    }

}
