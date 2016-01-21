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

import vnmr.util.*;

/**
 * Combined text and image icon. The first "line" may have either
 * text or an icon. The second "line" may have an icon or nothing
 * at all.
 *
 * @author Mark Cao
 */
public class TextImageIcon implements Icon {
    // ==== instance variables
    /** parent component, used to get font info */
    private Component parent;
    /** Text placed on line 1. Only text or an icon may be displayed on
     * line 1 (not both). Text is unconditionally displayed if it is
     * non-null. */
    private String line1Text;
    /** if text exists, use this color */
    private Color line1Color;
    /** if text exists, is it bolded? */
    private boolean line1Bold;
    /** icon placed on line 1 */
    private Icon line1Icon;
    /** icon placed on line 2 */
    private Icon line2Icon;

    /**
     * constructor
     * @param parent parent component
     */
    public TextImageIcon(Component parent) {
	this.parent = parent;
    } // TextImageIcon()

    /**
     * clear icon
     */
    public void clear() {
	line1Text = null;
	line1Color = (parent != null) ? parent.getForeground() : Color.black;
	line1Bold = false;
	line1Icon = null;
	line2Icon = null;
    } // clear()

    /**
     * set line-1 text
     * @param text text
     */
    public void setLine1Text(String text) {
	line1Text = text;
    } // setLine1Text()

    /**
     * set color of line-1 text
     * @param color color
     */
    public void setLine1Color(Color color) {
	line1Color = color;
    } // setLine1Color()

    /**
     * set bolding of line-1 text
     * @param bold true for bold, false for plain
     */
    public void setLine1Bold(boolean bold) {
	line1Bold = bold;
    } // setLine1Bold()

    /**
     * set line-1 icon
     * @param icon icon
     */
    public void setLine1Icon(Icon icon) {
	line1Icon = icon;
    } // setLine1Icon()

    /**
     * set line-2 icon
     * @param icon icon
     */
    public void setLine2Icon(Icon icon) {
	line2Icon = icon;
    } // setLine2Icon()

    /**
     * paint icon
     */
    public void paintIcon(Component c, Graphics g, int x, int y) {
	int totalWidth = getIconWidth();

	// paint line 1
	int line1Height = 0;
	if (line1Text != null) {
	    Font font = parent.getFont();
	    if (line1Bold) {
		//font = font.deriveFont(Font.BOLD);
        font = DisplayOptions.getFont(font.getName(), Font.BOLD, font.getSize());
	    }
	    g.setFont(font);
	    g.setColor(line1Color);

	    FontMetrics metrics = parent.getFontMetrics(font);
	    g.drawString(line1Text,
			 x + center(metrics.stringWidth(line1Text),
				    totalWidth),
			 y + metrics.getAscent());
	    line1Height = metrics.getHeight();
	} else if (line1Icon != null) {
	    line1Icon.paintIcon(c, g,
				x + center(line1Icon.getIconWidth(),
					   totalWidth),
				y);
	    line1Height = line1Icon.getIconHeight();
	}


	// paint line 2
	if (line2Icon != null) {
	    // Draw a line between the two.  We don't actually have the full
	    // width of the area.  totalWidth is the width of the word in
	    // line1Icon.  So, add a bunch to make sure we have a line
	    // all the way across.  Too long does not seem to hurt anything.
	    g.setColor(Color.black);
	    g.drawLine(x, y + line1Height, totalWidth +400, y + line1Height);

	    line2Icon.paintIcon(c, g,
				x + center(line2Icon.getIconWidth(),
					   totalWidth),
				y + line1Height + 2);
	}
    } // paintIcon()

    /**
     * get width
     * @return width
     */
    public int getIconWidth() {
	int result = 0;
	if (line1Text != null) {
	    Font font = parent.getFont();
	    if (line1Bold) {
		//font = font.deriveFont(Font.BOLD);
        font = DisplayOptions.getFont(font.getName(), Font.BOLD, font.getSize());
	    }
	    FontMetrics metrics = parent.getFontMetrics(font);
	    result = Math.max(result, metrics.stringWidth(line1Text));
	} else if (line1Icon != null) {
	    result = Math.max(result, line1Icon.getIconWidth());
	}

	// paint line 2
	if (line2Icon != null) {
	    result = Math.max(result, line2Icon.getIconWidth());
	}
	return result;
    } // getIconWidth()

    /**
     * get height
     * @return height
     */
    public int getIconHeight() {
	int result = 0;
	if (line1Text != null) {
	    Font font = parent.getFont();
	    if (line1Bold) {
		//font = font.deriveFont(Font.BOLD);
        font = DisplayOptions.getFont(font.getName(), Font.BOLD, font.getSize());
	    }
	    FontMetrics metrics = parent.getFontMetrics(font);
	    result += metrics.getHeight();
	} else if (line1Icon != null) {
	    result += line1Icon.getIconHeight();
	}

	// paint line 2
	if (line2Icon != null) {
	    result += line2Icon.getIconHeight() + 1;
	}
	return result;
    } // getIconHeight()

    /**
     * Given a subject's length and a "total length" (the total length
     * is larger), return the starting position that centers the
     * subject within the total length.
     * @param subjectLen
     * @param totalLen
     * @return position
     */
    private static int center(int subjectLen, int totalLen) {
	return (totalLen - subjectLen) / 2;
    } // center()

} // class TextImageIcon
