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

public class SimpleH2Layout implements LayoutManager {

    public static final int LEFT 	= 0;

    public static final int CENTER 	= 1;

    public static final int RIGHT 	= 2;

    public static final int EVEN = 3;
    public static int   width_1 = 0;

    private int xoffset = 4;
    private int align;
    private int hgap;
    private int vgap;
    private boolean extendLast;
    private boolean adjust_1 = false;

    public SimpleH2Layout() {
	this(CENTER, 5, 5, false);
    }

    public SimpleH2Layout(int align) {
	this(align, 5, 5, false);
    }

    public SimpleH2Layout(int align, int hgap, int vgap, boolean extend) {
	this.hgap = hgap;
	this.vgap = vgap;
	this.extendLast = extend;
	this.align = align;
        setAlignment(align);
    }

    public SimpleH2Layout(int align, int hg, int vg, boolean ex, boolean a) {
	this.hgap = hg;
	this.vgap = vg;
	this.align = align;
	this.extendLast = ex;
	this.adjust_1 = a;
        setAlignment(align);
    }

    public int getAlignment() {
	return align;
    }

    public void setAlignment(int align) {
	this.align = align;
    }

    public void setXOffset(int w) {
	xoffset = w;
    }

    public int getXOffset() {
	return xoffset;
    }

    public int getHgap() {
	return hgap;
    }

    public void setHgap(int hgap) {
	this.hgap = hgap;
    }

    public int getVgap() {
	return vgap;
    }

    public void setVgap(int vgap) {
	this.vgap = vgap;
    }

    public void addLayoutComponent(String name, Component comp) {
    }

    public void removeLayoutComponent(Component comp) {
    }

    public static void  setFirstWidth(int w) {
	width_1 = w;
    }

    public Dimension preferredLayoutSize(Container target) {
      synchronized (target.getTreeLock()) {
	Dimension dim = new Dimension(0, 0);
	int nmembers = target.getComponentCount();

	for (int i = 0 ; i < nmembers ; i++) {
	    Component m = target.getComponent(i);
	    if (m.isVisible()) {
		Dimension d = m.getPreferredSize();
		dim.height = Math.max(dim.height, d.height);
		if (i > 0) {
		    dim.width += hgap + d.width;
		} else if (adjust_1 && width_1 > 0) {
		    dim.width += width_1;
		} else {
		    dim.width += d.width;
		}
	    }
	}
	Insets insets = target.getInsets();
	dim.width += insets.left + insets.right + xoffset * 2;
	dim.height += insets.top + insets.bottom + vgap*2;
        
	return dim;
      }
    }

    public Dimension minimumLayoutSize(Container target) {
      synchronized (target.getTreeLock()) {
	Dimension dim = new Dimension(0, 0);
	int nmembers = target.getComponentCount();

	for (int i = 0 ; i < nmembers ; i++) {
	    Component m = target.getComponent(i);
	    if (m.isVisible()) {
		Dimension d = m.getMinimumSize();
		dim.height = Math.max(dim.height, d.height);
		if (i > 0) {
		    dim.width += hgap;
		}
		dim.width += d.width;
	    }
	}
	Insets insets = target.getInsets();
	dim.width += insets.left + insets.right + xoffset * 2;
	dim.height += insets.top + insets.bottom + vgap*2;
	return dim;
      }
    }

    private void moveComponents(Container target, int width, boolean ltr) {
      synchronized (target.getTreeLock()) {
	int	gap = hgap;
	int     members = target.getComponentCount();
	int     x = xoffset;
	int     y = vgap;
	if (members <= 0)
		return;
	if (width < 0 && align == CENTER) {
            while (gap > 0) {
                gap--;
                width += members;
                if (width >= 0)
                   break;
            }
        }
	switch (align) {
	case LEFT:
	    x += ltr ? 0 : width;
	    break;
	case CENTER:
	    x += width / 2;
	    break;
	case RIGHT:
	    x += ltr ? width : 0;
	    break;
	case EVEN:
	    gap = width / (members + 1);
	    x = gap;
	    break;
	}
	Dimension rc2 = target.getSize();
	for (int i = 0 ; i < members ; i++) {
	    Component m = target.getComponent(i);
	    Dimension rc = m.getSize();
	    if (m.isVisible()) {
		y = vgap + (rc2.height - rc.height) / 2;
		if (y < 0)
		    y = 0;
	        if (ltr) {
        	    m.setLocation(x, y);
	        } else {
	            m.setLocation(rc2.width - x - rc.width, y);
                }
                x += rc.width + gap;
	    }
	}
      }
    }

    public void layoutContainer(Container target) {
      synchronized (target.getTreeLock()) {
	Insets insets = target.getInsets();
	Rectangle rc = target.getBounds();
	int maxwidth;
	int nmembers = target.getComponentCount();
	int x = 0, y = vgap;
	int rowh = 0, start = 0;
	int	i, num, w;
	Dimension d;
	Component m;

	boolean ltr = target.getComponentOrientation().isLeftToRight();

	maxwidth = rc.width - (insets.left + insets.right + xoffset * 2);
	x = insets.left + xoffset;
	if (align == EVEN) {
	    num = 0;
	    for (i = 0 ; i < nmembers ; i++) {
	        m = target.getComponent(i);
	        if (m.isVisible())
		    num++;
	    }
	    if (num > 1) {
		w = maxwidth - hgap * (num - 1);
		w = maxwidth / num;
	    }
	    else
		w = maxwidth;
	    if (w < 4)
		w = 4;
	    for (i = 0 ; i < nmembers ; i++) {
	        m = target.getComponent(i);
	        if (m.isVisible()) {
		    d = m.getPreferredSize();
		    JComponent jm = (JComponent) m;
		    d.width = w;
		    m.setSize(d);
		    jm.setPreferredSize(d);
		}
	    }
	    moveComponents(target, maxwidth, ltr);
	    return;
	}

	for (i = 0 ; i < nmembers ; i++) {
	    m = target.getComponent(i);
	    if (m.isVisible()) {
		d = m.getPreferredSize();
		if (i == 0) {
		    if (width_1 > 0 && adjust_1)
		       d.width = width_1;
	        }
		else {
		    if ((i == nmembers - 1) && extendLast) {
		       d.width = maxwidth - x;
		       if (d.width < 4)
		           d.width = 4;
	            }
		}
		m.setSize(d);
		x = x + d.width + hgap;
	    }
	}
	if (align == CENTER) {
	    maxwidth = maxwidth - x;
	}
	moveComponents(target, maxwidth, ltr);
      }
    }
}
