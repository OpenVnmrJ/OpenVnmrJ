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
import javax.swing.*;
import javax.swing.border.*;
import vnmr.ui.*;

public class VTabBorder extends AbstractBorder
{
    private static Border blackLine;
    private static Border grayLine;
    private Insets myinsets;

    protected int thickness;
    protected Color lineColor;
    protected boolean roundedCorners;


    /** 
     * Creates a line border with the specified color and a 
     * thickness = 1.
     * @param color the color for the border
     */
    public VTabBorder() {
	this.roundedCorners = true;
	this.thickness = 4;
	myinsets = new Insets(4, 4, 4, 4);
    }

    public VTabBorder(Color color) {
        this(color, 4, false);
    }

    /**
     * Creates a line border with the specified color and thickness.
     * @param color the color of the border
     * @param thickness the thickness of the border
     */
    public VTabBorder(Color color, int thickness)  {
        this(color, thickness, false);
    }

    /**
     * Creates a line border with the specified color, thickness,
     * and corner shape.
     * @param color the color of the border
     * @param thickness the thickness of the border
     * @param roundedCorners whether or not border corners should be round
     */
    VTabBorder(Color color, int thickness, boolean roundedCorners)  {
        lineColor = color;
        this.thickness = thickness;
	this.roundedCorners = roundedCorners;
	myinsets = new Insets(thickness, thickness, thickness, thickness);
    }

    public void paintVtabBorder(Component c, Graphics g, int x, int y, int w, int h, int orient) {
        Color oldColor = g.getColor();
        int x2, y2;

        g.setColor(Util.getBgColor());
        // g.setColor( c.getBackground().brighter());
        g.setColor( c.getBackground().darker());
        x2 = w - 1;
        y2 = h - 1;
        if (orient == SwingConstants.RIGHT) { 
	    g.drawLine(0, 0, x2 - 4, 0);
	    g.drawLine(x2 - 4, 0, x2, 6);
	    g.drawLine(x2, 7, x2, y2);
	    g.drawLine(0, y2, x2, y2);
            return;
        }
        if (orient == SwingConstants.LEFT) { 
	    g.drawLine(0, 6, 4, 0);
	    g.drawLine(4, 0, x2, 0);
	    g.drawLine(0, 7, 0, y2);
	    g.drawLine(0, y2, x2, y2);
            return;
        }
        else {
	    g.drawLine(0, 0, 0, y2 - 4);
	    g.drawLine(0, y2 - 4, 6, y2);
	    g.drawLine(6, y2, x2, y2);
	    g.drawLine(x2, 0, x2, y2);
            return;
        }
    }

    public void paintBorder(Component c, Graphics g, int x, int y, int width, int height) {
        Color oldColor = g.getColor();
        int i;
	boolean drawBottom = true;
	TabButton tab;
	if (c instanceof PushpinTab) {
              PushpinTab ptab = (PushpinTab) c;
              int orient = ptab.getOrientation();
              if (orient != SwingConstants.TOP && orient != 0) {
                  paintVtabBorder(c, g, x, y, width, height, orient);
                  return;
              }
        }
	if (c instanceof TabButton) {
              tab = (TabButton) c;
              if (tab.isActive())
		drawBottom = false;
	}
	else
	    return;

	// PENDING(klobad) How/should do we support Roundtangles?
        //g.setColor( c.getBackground());
	//g.drawRect(0,0, 3, 3);
        g.setColor(Util.getBgColor());
        int[] xvec = {0, 0, 3};
        int[] yvec = {0, 3, 0};
        g.fillPolygon(xvec, yvec, 3);
        g.setColor( c.getBackground().brighter());
	g.drawLine(0, 3, 0, height -1);
	g.drawLine(0, 3, 3, 0);
	g.drawLine(3, 0, width -1, 0);
        g.setColor( c.getBackground().darker());
	g.drawLine(width -1, 0, width -1, height -1);
	//if (drawBottom) {
	//    g.drawLine(0, height - 1, width -1, height -1);
	//}
        g.setColor(oldColor);
    }

    /**
     * Returns the insets of the border.
     * @param c the component for which this border insets value applies
     */
    public Insets getBorderInsets(Component c)       {
        return myinsets;
    }

    public Insets getBorderInsets()       {
        return myinsets;
    }

    /** 
     * Reinitialize the insets parameter with this Border's current Insets. 
     * @param c the component for which this border insets value applies
     * @param insets the object to be reinitialized
     */
    public Insets getBorderInsets(Component c, Insets insets) {
	insets = myinsets;
        return insets;
    }

    public void setBorderInsets(Insets insets) {
	myinsets = insets;
    }

    /**
     * Returns whether or not the border is opaque.
     */
    public boolean isBorderOpaque() { return true; }

}
