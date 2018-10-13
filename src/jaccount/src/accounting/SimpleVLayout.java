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

package accounting;

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

    public SimpleVLayout() {
	this(0, 0, 0, 0);
    }

    public SimpleVLayout(int hgap, int vgap) {
	this(hgap, vgap, 0, 0);
    }

    public SimpleVLayout(int hgap, int vgap, int top, int bottom) {
	this.xoffset = hgap;
	this.vgap = vgap;
	this.topGap = top;
	this.bottomGap = bottom;
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

    public Dimension preferredLayoutSize(Container target) {
      synchronized (target.getTreeLock()) {
	Insets insets = target.getInsets();
	int nmembers = target.getComponentCount();
        int w = 0;
        int h = 0;

	h = topGap + bottomGap + insets.top + insets.bottom;
	for (int i = 0 ; i < nmembers ; i++) {
	    Component m = target.getComponent(i);
	    Dimension d = m.getPreferredSize();
	    if (d.width > w)
                w = d.width;
            h += d.height;
	}
        if (nmembers > 1)
            h = h + (nmembers - 1) * vgap;
	return (new Dimension(w, h));
      }
    }

    public Dimension minimumLayoutSize(Container target) {
	return (new Dimension(0, 0));
    }

    public void layoutContainer(Container target) {
      synchronized (target.getTreeLock()) {
	Insets insets = target.getInsets();
	Dimension pSize = target.getSize();
	int nmembers = target.getComponentCount();
	int y = topGap + insets.top;
        int pw = pSize.width - xoffset * 2;

        for (int i = 0; i < nmembers; i++) {
            Component m = target.getComponent(i);
            Dimension d = m.getPreferredSize();
            m.setBounds(new Rectangle(0, y, pw, d.height));
            y = y + d.height + vgap;
            m.validate();
        }
      }
    }
}
