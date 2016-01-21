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
import java.awt.Component;
import javax.swing.*;
import javax.swing.border.*;

public class VButtonBorder extends AbstractBorder
{
    private static final long serialVersionUID = 1L;
    private boolean use_armed=true;
    private boolean isThick = false;
    private boolean isHalf = false;
    private Insets m_insets = new Insets(2, 2, 2, 2);

    public VButtonBorder()    {
    }
    public VButtonBorder(boolean f)    {
        use_armed=f;
    }

    public void paintBorder(Component c, Graphics g, int x, int y, int w, int h) {
	// if (!(c instanceof AbstractButton))
	//    return;
        boolean bPressed = false;
	g.translate(x, y);
	if (c instanceof AbstractButton) {
	    AbstractButton b = (AbstractButton) c;
	    if (b.isSelected()) {
	        bPressed = true;
	    }
	    if (b.getModel().isPressed() && (b.getModel().isArmed() || !use_armed))
                bPressed = true;
        }
        if (bPressed) {
	    g.setColor(c.getBackground().darker().darker());
	    g.drawLine(0, 0, w-1, 0);
	    g.drawLine(0, 0, 0, h-1);
	    if (isThick) {
	    	g.setColor(c.getBackground().darker());
	    	g.drawLine(1, 1, w-2, 1);
	    	g.drawLine(1, 1, 1, h-2);
	    }
	    g.setColor(c.getBackground().brighter());
	    g.drawLine(0, h-1, w-1, h-1);
            if (isHalf) {
	        g.drawLine(w-1, 1, w-1, h-2);
            }
            else {
	        g.drawLine(w-1, 0, w-1, h-1);
	        if (isThick) {
	    	    g.drawLine(1, h-2, w-2, h-2);
	    	    g.drawLine(w-2, 1, w-2, h-2);
	        }
	    }
	}
	else {
	    // g.setColor(c.getBackground().brighter().brighter());
	    g.setColor(c.getBackground().brighter());
	    g.drawLine(0, 0, w-1, 0);
	    g.drawLine(0, 0, 0, h-1);
	    if (isThick) {
	    	g.drawLine(1, 1, w-2, 1);
	    	g.drawLine(1, 1, 1, h-2);
	    }
	    g.setColor(c.getBackground().darker().darker());
	    g.drawLine(0, h-1, w-1, h-1);
            if (isHalf) {
	       // g.drawLine(w-1, 1, w-1, h-2);
            }
            else {
	       g.drawLine(w-1, 0, w-1, h-1);
	       if (isThick) {
	           g.setColor(c.getBackground().darker());
	    	   g.drawLine(1, h-2, w-2, h-2);
	    	   g.drawLine(w-2, 1, w-2, h-2);
	       }
	    }
	}
	/*
	g.translate(-x, -y);
        g.setColor(oldColor);
	*/
    }


    public void setThick(boolean s) {
	isThick = s;
    }

    public void setHalfWay(boolean s) {
	isHalf = s;
    }

    /**
     * Returns the insets of the border.
     * @param c the component for which this border insets value applies
     */
    public Insets getBorderInsets(Component c)       {
        return m_insets;
    }

    /**
     * Sets the insets of the border.
     * @param newInsets The new insets for this border.
     */
    public void setBorderInsets(Insets newInsets)       {
        m_insets = newInsets;
    }

    /** 
     * Reinitialize the insets parameter with this Border's current Insets. 
     * @param c the component for which this border insets value applies
     * @param insets the object to be reinitialized
     */
    public Insets getBorderInsets(Component c, Insets insets) {
        insets.left = this.m_insets.left;
        insets.top = this.m_insets.top;
        insets.right = this.m_insets.right;
        insets.bottom = this.m_insets.bottom;
        return insets;
    }

    /**
     * Returns whether or not the border is opaque.
     */
    public boolean isBorderOpaque() { return true; }

}
