/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import javax.swing.*;
import java.beans.*;
import javax.swing.border.*;

import vnmr.util.*;

/**
 * A two-line button consists of two text lines.
 */
public class TwoLineButton extends JToggleButton implements PropertyChangeListener {
    /** two-line icon */
    private TwoLineIcon icon;
    protected Font font;

    /**
     * constructor
     */
    public TwoLineButton() {
		icon = new TwoLineIcon(this);
		setIcon(icon);
	    setEnabled(false);
	    setOpaque(true);
	    setBorder(BorderFactory.createRaisedBevelBorder());
        DisplayOptions.addChangeListener(this);
    } // TwoLineButton()

    public void propertyChange(PropertyChangeEvent e)
    {
        String strProperty = e.getPropertyName();
        if (strProperty == null)
            return;
        strProperty = strProperty.toLowerCase();
        if (strProperty.indexOf("vjbackground") >= 0)
            setBackground(Util.getBgColor());
    }

    /**
     * set first line
     * @param lineStr string
     */
    public void setLine1(String lineStr) {
	if (lineStr == null)
	    icon.line1Str = "";
	else
	    icon.line1Str = lineStr;
    } // setLine1()

    public boolean isRequestFocusEnabled()
    {
        return Util.isFocusTraversal();
    }

    /**
     * set first line's foreground
     * @param color color
     */
    public void setLine1Color(Color color) {
	icon.line1Color = color;
    } // setLine1Color()

    /**
     * set first line's font
     */
    public void setLine1Font(Font f) {
		icon.line1Font = f;
    } // setLine1Font()

    /**
     * set second line
     * @param lineStr string
     */
    public void setLine2(String lineStr) {
	if (lineStr == null)
	   icon.line2Str = "";
	else
	   icon.line2Str = lineStr;
    } // setLine2()

    /**
     * set second line's foreground
     * @param color color
     */
    public void setLine2Color(Color color) {
	icon.line2Color = color;
    } // setLine2Color()

    /**
     * set second line's font
     */
    public void setLine2Font(Font f) {
		icon.line2Font = f;
    } // setLine1Font()

} // class TwoLineButton

/**
 * two-line icon
 */
class TwoLineIcon implements Icon {
    // ==== instance variables
    /** parent component */
    private Component parent;
    /** first line */
    protected String line1Str;
    /** first line's foreground color */
    protected Color line1Color = Color.black;
    /** first line's font */
    protected Font line1Font;
    /** second line */
    protected String line2Str;
    /** second line's foreground color */
    protected Color line2Color = Color.black;
    /** second line's font */
    protected Font line2Font;

    /**
     * constructor
     * @param parent parent component
     */
    public TwoLineIcon(Component parent) {
		this.parent = parent;
    	line1Font = DisplayOptions.getFont("Dialog", Font.PLAIN, 12);
    	line2Font = DisplayOptions.getFont("Dialog", Font.PLAIN, 12);
    	line1Str = " ";
    	line2Str = " ";
   } // TwoLineIcon()

    /**
     * paint icon
     * The std java interface assumes only one x,y location.
     * This TwolineButton has always been starting both lines at the x location.
     * That means that the label (such as Spin) moves around depending on the
     * size of the value on line 2.  This has now been modified to adjust
     * the x,y for each line such that both lines are centered based on their
     * individual widths.
     */
    public void paintIcon(Component c, Graphics g, int x, int y) {
        int w1=0,w2=0,xnew;
		FontMetrics metrics = c.getFontMetrics(line1Font);
        w1=metrics.stringWidth(line1Str);
		if (metrics != null)
		   y += metrics.getAscent();
	  	else
		   y += 10;
        metrics = parent.getFontMetrics(line2Font);
        w2=metrics.stringWidth(line2Str);
        if(w1 < w2)
            // Adjust x for centering of line 1
            xnew = x + (w2 - w1)/2;
        else
            xnew = x;
		g.setFont(line1Font);
		g.setColor(line1Color);
		g.drawString(line1Str, xnew, y);
		if (metrics != null)
		    y += metrics.getHeight();
	  	else
		   y += 10;
		if(w1 > w2)
            // Adjust x for centering of line 2
            xnew = x + (w1 - w2)/2;
        else
            xnew = x;
		g.setColor(line2Color);
		g.setFont(line2Font);
		g.drawString(line2Str, xnew, y);
    } // paintIcon()

    /**
     * get width
     * @return width
     */
    public int getIconWidth() {
		int w1=0,w2=0;
		FontMetrics metrics;
	    metrics = parent.getFontMetrics(line1Font);
	    w1=metrics.stringWidth(line1Str);
	    metrics = parent.getFontMetrics(line2Font);
	    w2=metrics.stringWidth(line2Str);
	    return w1>w2?w1:w2;
    } // getIconWidth()

    /**
     * get height
     * @return height
     */
    public int getIconHeight() {
		int ht;
	    FontMetrics metrics = parent.getFontMetrics(line1Font);
	    ht=metrics.getHeight();
	    metrics = parent.getFontMetrics(line2Font);
	    return ht+metrics.getHeight();
    } // getIconHeight()

} // class TwoLineIcon
