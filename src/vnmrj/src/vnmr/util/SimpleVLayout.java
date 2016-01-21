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

/**
 * A layout manager that allows multiple components to be layed out
 * vertically. The components will not wrap so, for
 * example, a vertical arrangement of components will stay vertically
 * arranged when the frame is resized.
**/
 
public class SimpleVLayout implements LayoutManager {

    private int xoffset;
    private int vgap;
    private int topGap;
    private int bottomGap;
    private boolean extendLast;

    public SimpleVLayout() {
	this(0, 0, false);
    }

    public SimpleVLayout(int hgap, int vgap, boolean extend) {
	this.xoffset = hgap;
	this.vgap = vgap;
	this.topGap = 0;
	this.bottomGap = 0;
	this.extendLast = extend;
    }

    public SimpleVLayout(int hgap, int vgap, int top, int bottom, boolean extend) {
	this.xoffset = hgap;
	this.vgap = vgap;
	this.topGap = top;
	this.bottomGap = bottom;
	this.extendLast = extend;
    }

    public void addLayoutComponent(String name, Component comp) {}

    public void removeLayoutComponent(Component comp) {}


    public void setXOffset(int w) {
	xoffset = w;
    }

    public int getXOffset() {
	return xoffset;
    }

    public int getVgap() {
	return vgap;
    }

    public void setVgap(int vgap) {
	this.vgap = vgap;
    }

    public void setExtendLast(boolean b) {
	extendLast = b;
    }

    public Dimension preferredLayoutSize(Container target) {
      synchronized (target.getTreeLock()) {
	Insets insets = target.getInsets();
	int nmembers = target.getComponentCount();
        int w = 0;
        int h = 0;

	h = topGap + bottomGap + insets.top + insets.bottom;
	for (int i = 0 ; i < nmembers ; i++) {
	    Component m = target.getComponent(i);
	    if (m.isVisible()) {
		Dimension d = m.getPreferredSize();
		if (d.width > w)
		    w = d.width;
		h += d.height;
	    }
	}
        if (nmembers > 1)
            h = h + (nmembers - 1) * vgap;
	w = w + insets.left + insets.right + xoffset * 2;
	Dimension dim = new Dimension(w, h);
	return dim;
      }
    }

    public Dimension minimumLayoutSize(Container target) {
      synchronized (target.getTreeLock()) {
	Dimension dim = new Dimension(0, 0);
	return dim;
      }
    }

    public void layoutContainer(Container target) {
      synchronized (target.getTreeLock()) {
	Insets insets = target.getInsets();
	Dimension pSize = target.getSize();
	int nmembers = target.getComponentCount();
	int y = topGap + insets.top;
        int pw = pSize.width - xoffset * 2;
        int ph = pSize.height - insets.bottom;
	int h;
	Dimension d;
	Component m;
	for (int i = 0 ; i < nmembers; i++) {
	    m = target.getComponent(i);
	    if (m.isVisible()) {
		d = m.getPreferredSize();
	        h = d.height;
		if ((i == nmembers - 1) && extendLast) {
		    h = ph - y;
		    if (h < d.height)
			h = d.height;
		}
		m.setBounds(new Rectangle(xoffset, y, pw, h));
		y = y + h + vgap;
                m.validate();
	    }
	}
      }
    }
}
