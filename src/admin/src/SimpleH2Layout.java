/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */

import java.awt.*;
import javax.swing.*;

public class SimpleH2Layout implements LayoutManager {

    public static final int LEFT 	= 0;

    public static final int CENTER 	= 1;

    public static final int RIGHT 	= 2;

    public static final int EVEN = 3;
    public static int   width_1 = 0;
    public static int   width_2 = 0;

    private int align;
    private int hgap;
    private int vgap;
    private boolean extendLast;
    private boolean adjust_1 = false;
    private boolean firstTime = true;

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
        setAlignment(align);
    }

    public SimpleH2Layout(int align, int hg, int vg, boolean ex, boolean a) {
	this.hgap = hg;
	this.vgap = vg;
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

    public static void  setLastWidth(int w) {
	width_2 = w;
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
	dim.width += insets.left + insets.right + hgap*2;
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
	dim.width += insets.left + insets.right + hgap*2;
	dim.height += insets.top + insets.bottom + vgap*2;
	return dim;
      }
    }

    private void moveComponents(Container target, int x, int y, int width, int height,
                                int rowStart, int rowEnd, boolean ltr) {
      synchronized (target.getTreeLock()) {
	int	gap = hgap;
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
	    gap = width / (rowEnd + 1);
	    x = gap;
	    break;
	}
	for (int i = rowStart ; i < rowEnd ; i++) {
	    Component m = target.getComponent(i);
	    Rectangle rc = m.getBounds();
	    Rectangle rc2 = target.getBounds();
	    if (m.isVisible()) {
	        if (ltr) {
        	    m.setLocation(x, y + (height - rc.height) / 2);
	        } else {
	            m.setLocation(rc2.width - x - rc.width, y + (height - rc.height) / 2);
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
	int x = 0, y = insets.top + vgap;
	int rowh = 0, start = 0;
	int	i;
	Dimension d;
	Component m;

        boolean ltr = target.getComponentOrientation().isLeftToRight();

	if (align == EVEN && firstTime && !extendLast) {
	    maxwidth = 0;
	    for (i = 0 ; i < nmembers ; i++) {
	        m = target.getComponent(i);
	        if (m.isVisible()) {
		    d = m.getPreferredSize();
		    if (d.width > maxwidth)
			maxwidth = d.width;
		}
	    }
	    for (i = 0 ; i < nmembers ; i++) {
	        m = target.getComponent(i);
	        if (m.isVisible()) {
		    d = m.getPreferredSize();
		    JComponent jm = (JComponent) m;
		    d.width = maxwidth;
		    m.setSize(d);
		    jm.setPreferredSize(d);
		}
	    }
	    firstTime = false;
	}

	maxwidth = rc.width - (insets.left + insets.right + hgap*2);
	for (i = 0 ; i < nmembers ; i++) {
	    m = target.getComponent(i);
	    if (m.isVisible()) {
		d = m.getPreferredSize();
		if (i == 0 && width_1 > 0 && adjust_1) {
		   d.width = width_1;
	        }
		m.setSize(d);

		if ((x == 0) || ((x + d.width) <= maxwidth)) {
		    if (x > 0) {
			x += hgap;
		    }
		    if ((i == 1) && extendLast) {
			d.width = maxwidth - x - width_2 - hgap;
			m.setSize(d);
		        x = x + d.width + hgap;
		    }
		    else
		        x += d.width;
		    rowh = Math.max(rowh, d.height);
		} else {
		    moveComponents(target, insets.left + hgap, y, maxwidth - x, rowh, start, i, ltr);
		    x = d.width;
		    y += vgap + rowh;
		    rowh = d.height;
		    start = i;
		}
	    }
	}
	moveComponents(target, insets.left + hgap, y, maxwidth - x, rowh, start, nmembers, ltr);
      }
    }
}
