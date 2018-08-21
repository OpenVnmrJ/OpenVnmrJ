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
import java.util.Vector;
import javax.swing.*;

public class SimpleHLayout implements LayoutManager {

    public static final int LEFT 	= 0;
    public static final int CENTER 	= 1;
    public static final int RIGHT 	= 2;
    public static final int TOP 	= 3;
    public static final int BOTTOM 	= 4;

    public static final int EVEN = 3;

    private int xoffset = 4;
    private int align;
    private int hgap;
    private int vgap;
    private int verticalAlignment;
    private boolean extendLast;
    private boolean adjust_1 = false;
    private Vector<Component> compList;

    public SimpleHLayout() {
	this(LEFT, 5, 5, false);
    }

    public SimpleHLayout(int align) {
	this(align, 5, 5, false);
    }

    public SimpleHLayout(int align, int hgap, int vgap, boolean extend) {
	this.hgap = hgap;
	this.vgap = vgap;
	this.extendLast = extend;
	this.align = align;
	this.verticalAlignment = 0;
        setAlignment(align);
    }

    public SimpleHLayout(int align, int hg, int vg, boolean ex, boolean a) {
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

    public void setVerticalAlignment(int align) {
	verticalAlignment = align;
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

    public void setExtendLast(boolean b) {
        extendLast = b;
    }

    public void addLayoutComponent(String name, Component comp) {
        if (compList != null)
            compList.clear();
    }


    public void addLayoutComponent(Component comp, Object constraints) {
        if (compList != null)
            compList.clear();
    }

    public void removeLayoutComponent(Component comp) {
        if (compList != null)
            compList.clear();
    }

    public Dimension preferredLayoutSize(Container target) {
      synchronized (target.getTreeLock()) {
	Dimension dim = new Dimension(0, 0);
	int nmembers = target.getComponentCount();
        int maxX = 0;

	for (int i = 0 ; i < nmembers ; i++) {
	    Component m = target.getComponent(i);
	    if (m.isVisible()) {
		Dimension d = m.getPreferredSize();
		dim.height = Math.max(dim.height, d.height);
		dim.width = dim.width + d.width+ hgap;
		Point pt = m.getLocation();
                if (maxX < (pt.x+d.width))
                    maxX = pt.x+d.width;
	    }
	}
	Insets insets = target.getInsets();
	dim.width += insets.left + insets.right + xoffset * 2;
	dim.height += insets.top + insets.bottom + vgap*2;
        if (dim.width < maxX)
            dim.width = maxX;
        
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


    private void setMemberOrder(Container target) {
	int nmembers = target.getComponentCount();
        if (compList == null)
            compList = new Vector<Component>(nmembers);
        else
            compList.clear();
        int i, x, x2, k, index;
	for (i = 0 ; i < nmembers; i++) {
	    Component comp = target.getComponent(i);
            Point pt = comp.getLocation();
            if (pt != null)
                x = pt.x;
            else
                x = 0;
            for (index = 0; index < compList.size(); index++) {
                Component c = compList.elementAt(index);
                Point pt2 = c.getLocation();
                if (pt2 != null)
                    x2 = pt2.x;
                else
                    x2 = 0;
                if (x2 > x)
                    break;
             }
             if (index >= compList.size())
                compList.add(comp);
             else
                compList.add(index, comp);
        } 

    }

    public void layoutContainer(Container target) {
      synchronized (target.getTreeLock()) {
	Insets insets = target.getInsets();
	Rectangle rc = target.getBounds();
	int maxwidth;
	int nmembers = target.getComponentCount();
	int x = 0, y = vgap;
	int mx, my;
	int rowh = 0, start = 0;
	int i, num, w;
	Dimension d;
	Component m;
        Point pt;

        if (nmembers < 1)
            return;
       //  if (compList == null || compList.size() != nmembers) {
       //     setMemberOrder(target);
       //  }
	boolean ltr = target.getComponentOrientation().isLeftToRight();

	maxwidth = rc.width - (insets.left + insets.right + xoffset * 2);
	x = insets.left + xoffset;
        // nmembers = compList.size();
	for (i = 0 ; i < nmembers ; i++) {
            // m = compList.elementAt(i);
	    m = target.getComponent(i);
	    if (m.isVisible()) {
		d = m.getPreferredSize();
                pt = m.getLocation();
                if (pt != null) {
                    mx = pt.x;
                    my = pt.y;
                }
                else {
                    mx = 0;
                    my = 0;
                }
                if (mx > x)
                    x = mx; 
	        if (verticalAlignment == CENTER)
                    y = (rc.height - d.height) / 2;
                else {
                    if (my < vgap)
                        y = my + vgap;
                    else
                        y = my;
                }
                if (y < 0)
                    y = 0;
                
		if ((i == nmembers - 1) && extendLast) {
		       d.width = maxwidth - x;
		       if (d.width < 4)
		           d.width = 4;
		}
                m.setBounds(new Rectangle(x, y, d.width, d.height));
                // m.validate();
		x = x + d.width + hgap;
	    }
	}
      }
    }
}
