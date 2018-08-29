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
import java.util.*;
import javax.swing.*;
import vnmr.bo.*;

// This LayoutManager can only handle one row layout
public class VRubberBarLayout implements LayoutManager
{
   Dimension prefDim = new Dimension(0, 0);
   Dimension actualDim = new Dimension(0, 0);
   Dimension minDim = new Dimension(0, 0);
   Dimension oldDim = new Dimension(0, 0);
   private Vector vecComp = null;
   private int ptLen = 0;
   private int oldCw = 0;
   private int[] compX = null;
   private int curCount = 0;
   private double  xRatio = 0.0;
   private double  yRatio = 0.0;
   private double  rx = 1.0; // old x ratio
   private double  ry = 1.0; // old y ratio
   private boolean newLayout = true;
   private String  name = "no name";
   private boolean doFloatLayout = false;

   public VRubberBarLayout() {
   }

   public void setName( String str) {
	name = str;
   }

   void addLayoutComponent(Component comp, Object constraints) {
	newLayout = true;
   }


   public void addLayoutComponent( String name, Component comp ) {
	newLayout = true;
   }

   public void removeLayoutComponent( Component comp ) {
	newLayout = true;
   }

   public Dimension preferredLayoutSize( Container target )
   {
	int w = 0;
	int pw = 0;
	int w2 = 0;
	int wm = 0;
	int h = 0;
	int k;
	int count = target.getComponentCount();
	Component comp;
	Dimension d, d2;
	Point pt;
        Insets insets = target.getInsets();

	prefDim.width = w;
	prefDim.height = h;
	if (count <= 0)
            return prefDim;
	doFloatLayout = false;
	for (k = 0; k < count; k++) {
	    comp = target.getComponent(k);
	    if( (comp != null) && comp.isVisible() ) {
	        d = comp.getPreferredSize();
		if (comp instanceof VObjIF)
	           pt = ((VObjIF)comp).getDefLoc();
		else
	           pt = comp.getLocation();
		if ((d.width + pt.x) > pw)
		    pw = pt.x + d.width;
	        if (h < d.height)
	            h = d.height;
                w2 += d.width;
		if (comp instanceof VGroup) {
		   d = ((VGroup) comp).getActualSize();
                   d2 = ((VGroup) comp).getRealSize();
                   wm += d2.width;
                }
                else
                   wm += d.width;
                w += d.width;
    	    }
	}
        if (w < wm)
            w = wm;
	if (pw < w) {
	    pw = w;
	    doFloatLayout = true;
	}
	if (pw < w2) {
	    pw = w2;
	    doFloatLayout = true;
	}
	prefDim.width = pw + insets.left + insets.right;
	prefDim.height = h + insets.top + insets.bottom;
	actualDim.height = prefDim.height;
	actualDim.width = w;
	return prefDim;
   }

   public Dimension maximumLayoutSize( Container target ) {
	return new Dimension(Short.MAX_VALUE, Short.MAX_VALUE);
   }

   public Dimension minimumLayoutSize( Container target )
   {
        return  new Dimension(0, 0);
   }

   public void layoutContainer( Container target )
   {
        Insets insets = target.getInsets();
	int cw = target.getSize().width - insets.left - insets.right - 4;
        int ch = target.getSize().height;
        int count = target.getComponentCount();
        if (count <= 0) {
	    curCount = 0;
	    return;
	}

	Component comp;
	int k, x;
        Dimension d;
        Point pt;

	if (curCount != count) {
	    newLayout = true;
	    curCount = count;
	}
	if (newLayout) {
	    preferredLayoutSize(target);
	    // newLayout = false;
	    sortOrder(target);
	    xRatio = 0.0;
	    yRatio = 0.0;
	}
        if (newLayout || (oldDim.width != actualDim.width) || (oldCw != cw)) {
            oldDim.width = actualDim.width;
            oldCw = cw;
            if (cw < actualDim.width) {
		rx = (double) cw / (double) actualDim.width;
		if (rx >= 0.98)
		 	rx = 0.98;
		if (rx < 0.3)
			rx = 0.3;
                rx += 1.0;
            }
            else
                rx = 1.0;
	}
	if (rx != xRatio || newLayout) {
             newLayout = false;
             xRatio = rx;
             yRatio = ry;
             for (k = 0; k < count; k++) {
             	comp = target.getComponent(k);
             	if( (comp != null) && comp.isVisible() ) {
                    if (comp instanceof VObjIF) {
                          ((VObjIF)comp).setSizeRatio(rx, ry);
		    }
                 }
             }
	     if (doFloatLayout || (rx != 1.0) ) {
		x = insets.left;
		count = vecComp.size();
             	for (k = 0; k < count; k++) {
             	    comp = (Component)vecComp.elementAt(k);
             	    if( (comp != null) && comp.isVisible() ) {
                   	d = comp.getSize();
                   	// comp.setBounds(x, 2, d.width, d.height);
                   	comp.setLocation(x, insets.top);
			x += d.width;
		    }
		}
	     }
	     else {
		x = insets.left;
		count = vecComp.size();
             	for (k = 0; k < count; k++) {
             	    comp = (Component)vecComp.elementAt(k);
             	    if( (comp != null) && comp.isVisible() ) {
                   	d = comp.getSize();
                   	pt = comp.getLocation();
			if (pt.x < x)
			    pt.x = x;
                   	// comp.setBounds(pt.x, 2, d.width, d.height);
                   	comp.setLocation(pt.x, insets.top);
			x = pt.x + d.width;
		    }
		}
	     }
	}
   }


   private void sortOrder(Container target) {
      int count = target.getComponentCount();
      int x = 0;
      int y = 0;
      int k;
      Component c1, c2;
      if ((vecComp == null) || (vecComp.size() < count))
           vecComp = new Vector(count);
      if (ptLen < count) {
           ptLen = count + 2;
           compX = new int[ptLen];
      }
      vecComp.clear();
      for( int i = 0; i < count; i++ ) {
           c1 = target.getComponent( i );
	   if (c1 == null)
		continue;
           x = c1.getLocation().x;
           if (i == 0) {
                vecComp.add(c1);
           }
           else {
               k = i / 2;
               while (k > 0) {
                   c2 = (Component) vecComp.elementAt(k);
                   if (c2.getLocation().x > x)
                        k = k / 2;
                   else
                        break;
               }
               while (k < i) {
                   c2 = (Component) vecComp.elementAt(k);
                   if (c2.getLocation().x > x) {
                        vecComp.insertElementAt(c1, k);
                        break;
                   }
                   k++;
                }
                if (k == i) { // room not found
                   vecComp.add(c1);
                }
            }
      }
      for(k = 0; k < vecComp.size(); k++ ) {
           c1 = (Component) vecComp.elementAt(k);
           if (c1 != null) {
              compX[k] = c1.getLocation().x;
	      if (c1 instanceof VGroup) {
		((VGroup)c1).setOneRowLayout(true);
	      }
           }
      }

   }
}

