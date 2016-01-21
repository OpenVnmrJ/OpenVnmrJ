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
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;
import java.util.*;
import java.io.*;

import  vnmr.util.*;
import  vnmr.bo.*;

import  vnmr.templates.*;
public class TemplateLayout implements LayoutManager {
	/** margins */
	private Insets margin = new Insets(4, 2, 2, 2);
	/** horizontal gap -- should be greater than the left margin */
	private int hgap = 6;

	/**
	 * get left margin
	 * @return margin
	 */
	public int getLeftMargin() {
	    return margin.left;
	} // getLeftMargin()
    public TemplateLayout(){
        setInsets(new Insets(4, 2, 2, 2));
    }

    public TemplateLayout(Insets m){
        setInsets(m);
    }
	public void addLayoutComponent(String name, Component comp) {}

	public void removeLayoutComponent(Component comp) {}

    public void setInsets(Insets m){
        margin=m;
    }
	/**
	 * calculate the preferred size
	 * @param target component to be laid out
	 * @see #minimumLayoutSize
	 */
	public Dimension preferredLayoutSize(Container target) {
	    synchronized (target.getTreeLock()) {
		int maxW = 0;
		int maxH = 0;
		Rectangle r;
		Dimension dim;
		int nmembers = target.getComponentCount();
		for (int i = 0; i < nmembers; i++) {
		    r = target.getComponent(i).getBounds();
		    dim = target.getComponent(i).getPreferredSize();
		    if (r.x + dim.width > maxW)
			maxW = r.x + dim.width;
		    if (r.y + dim.height > maxH)
			maxH = r.y + dim.height;
		}
		maxW = maxW + margin.left + margin.right;
		maxH = maxH + margin.top + margin.bottom;
		return new Dimension(maxW, maxH);
	    }
	} // preferredLayoutSize()

	/**
	 * calculate the minimum size
	 * @param target component to be laid out
	 * @see #preferredLayoutSize
	 */
	public Dimension minimumLayoutSize(Container target) {
	    return preferredLayoutSize(target);
	} // minimumLayoutSize()

	/**
	 * do the layout
	 * @param target component to be laid out
	 */
	public void layoutContainer(Container target) {
	    synchronized (target.getTreeLock()) {
		Dimension dim = target.getSize();
                Insets insets = target.getInsets();
		Point loc;
                dim.width = dim.width - insets.right - 1;
		int nmembers = target.getComponentCount();
		for (int i = 0; i < nmembers; i++) {
		    Component comp = target.getComponent(i);
		    Dimension size = comp.getPreferredSize();
		    loc = comp.getLocation();
                    if (loc.x + size.width >= dim.width) {
                         if (loc.x < dim.width)
                             size.width = dim.width - loc.x;
                    }
		    comp.setBounds(new Rectangle(loc.x, loc.y, size.width,
						 size.height));
		    comp.validate();
		   // comp.setVisible(true);
		}
	    }
	} // layoutContainer()

    } // class TemplateLayout
