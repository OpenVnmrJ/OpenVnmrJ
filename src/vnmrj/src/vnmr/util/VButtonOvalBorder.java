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
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.Insets;
import java.awt.Color;
import java.awt.Component;
import javax.swing.*;
import javax.swing.border.*;

public class VButtonOvalBorder extends AbstractBorder
{
    public VButtonOvalBorder()    {
    }

    public void paintBorder(Component c, Graphics g1, int x, int y, int w, int h) {
	Graphics2D g = (Graphics2D)g1;
	if (!(c instanceof AbstractButton))
	    return;
	AbstractButton b = (AbstractButton) c;
	g.translate(x, y);
	
	g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
			   RenderingHints.VALUE_ANTIALIAS_ON);
	if (b.getModel().isPressed()) {
	    g.setColor(c.getBackground().darker());
	}
	else {
	    g.setColor(c.getBackground().brighter().brighter());
	}
	g.drawArc(0, 0, w-1, h-1, 45, 180);
	if (b.getModel().isPressed())
	    g.setColor(c.getBackground().brighter().brighter());
	else
	    g.setColor(c.getBackground().darker().darker());
	g.drawArc(0, 0, w-1, h-1, 225, 180);
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

}
