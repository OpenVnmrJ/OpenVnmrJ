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

public class VRubberPanLayout implements LayoutManager
{
   Dimension prefDim = new Dimension(0, 0);
   Dimension actualDim = new Dimension(0, 0);
   Dimension minDim = new Dimension(0, 0);
   Dimension pageDim = new Dimension(500, 300);
   private double  xRatio = 1.0;
   private double  yRatio = 1.0;
   private boolean newLayout = true;
   private boolean  bEditMode = false;
   private Container panel = null;
   private int pWidth = 0; // parent's width
   private int pHeight = 0; // parent's height
   private String objName = null;
   private Container topObj = null;
   // private double minSquish = 0.6;
   private double minSquish = 1.0;

   public VRubberPanLayout() {
       this((Container)null);
   }

   public VRubberPanLayout(Container p) {
        this.panel = p;
        String sSquish = System.getProperty("squish");
        try {
            if (sSquish != null && sSquish.length() > 0) {
                minSquish = Double.parseDouble(sSquish);
                minSquish = Math.min(1.0, minSquish);
                minSquish = Math.max(0.3, minSquish);
            }
        } catch (NumberFormatException nfe) {
            Messages.postError("Bad \"squish\" value in -D argument: "
                               + sSquish);
        }
   }

   public void setSquish(double k) {
        minSquish = k;
        if (minSquish > 1.0)
           minSquish = 1.0;
        if (minSquish < 0.3)
           minSquish = 0.3;
   }

   public double getSquish() {
        return minSquish;
   }

   public void setName(String s) {
        objName = s;
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

   public Dimension getActualSize() {
       return actualDim;
   }

   public Dimension preferredLayoutSize( Container target )
   {
        int w = 0;
        int w2 = 0;
        int h = 0;
        int h2 = 0;
        int k;
        int count = target.getComponentCount();
        topObj = target;
        Component comp;
        Dimension d,m;
        Point pt;
        panel = target;
        if (count <= 0)
            return pageDim;

        for (k = 0; k < count; k++) {
            comp = target.getComponent(k);
            if( (comp != null) && comp.isVisible() ) {
                if (comp instanceof VObjIF)
                   pt = ((VObjIF)comp).getDefLoc();
                else
                   pt = comp.getLocation();
                d = comp.getPreferredSize();
                if ((d.width + pt.x) > w)
                    w = d.width + pt.x;
                if ((d.height + pt.y) > h)
                    h = d.height + pt.y;
                
                if (comp instanceof VGroup) { // causes panel grid bug
                    d = ((VGroup) comp).getActualSize();
                    m = ((VGroup) comp).getMinimumSize();
                    if(d.height<m.height)
                        d.height=m.height;
                    if(d.width<m.width)
                        d.width=m.width;
                }
                
                if ((d.width + pt.x) > w2)
                        w2 = d.width + pt.x;
                if ((d.height + pt.y) > h2)
                        h2 = d.height + pt.y;
               
            }
        }
        if (w < w2)
            w = w2;
        if (h < h2)
            h = h2;
        actualDim.height = h;
        actualDim.width = w;

        if (!bEditMode) {
            if ((w2 * minSquish) > pWidth)
                w = (int)(w2 * minSquish);
            else if (w > pWidth)
                w = pWidth;
            if ((h2 * minSquish) > pHeight)
                h = (int)(h2 * minSquish);
            else if (h > pHeight)
                h = pHeight;
        }
        prefDim.width = w;
        prefDim.height = h;
        minDim.height = h2;
        minDim.width = w2;
        // newLayout = false;
        return prefDim;
   }

   public void resetPreferredSize()
   {
        if (topObj != null)
            preferredLayoutSize(topObj);
   }

   public void setReferenceSize(int w, int h) {
        int oldW = pWidth;
        int oldH = pHeight;
        boolean doValidate = false;
        int xw, xh;

        w -= 4;
        h -= 4;
        pWidth = w;
        pHeight = h;
        
        if (panel == null) {
            return;
        }
        if (!bEditMode) {
            xw = prefDim.width;
            if (oldW != w) {
                xw = actualDim.width;
                doValidate = true;
                if ((minDim.width * minSquish) > w)
                    xw = (int)(minDim.width * minSquish);
                else if (xw > w)
                    xw = w;
            }
            xh = prefDim.height;
            if (oldH != h) {
                doValidate = true;
                xh = actualDim.height;
                if ((minDim.height * minSquish) > h)
                    xh = (int)(minDim.height * minSquish);
                else if (xh > h)
                    xh = h;
            }
            if (doValidate) {
                prefDim.width = xw;
                prefDim.height = xh;
               ((JComponent)panel).setPreferredSize(prefDim);
                panel.validate();
            }
        }
   }

   public Dimension minimumLayoutSize( Container target )
   {
        int w = 0;
        int h = 0;
        int k;
        int count = target.getComponentCount();
        JComponent comp;
        Dimension d;
        for (k = 0; k < count; k++) {
            comp = (JComponent)target.getComponent(k);
            if( (comp != null) && comp.isVisible() ) {
                d = comp.getMinimumSize();
                w += d.width;
                if (h < d.height)
                    h = d.height;
            }
        }
        return  new Dimension(w, h);
   }

   public void setSizeRatio(double rx, double ry) {
        xRatio = rx;
        yRatio = ry;
        if (xRatio < minSquish)
            xRatio = minSquish;
        if (yRatio < minSquish)
            yRatio = minSquish;
   }

   public void layoutContainer( Container target )
   {
        int cw = target.getSize().width;
        int ch = target.getSize().height;
        int count = target.getComponentCount();
        double rx = 1.0;
        double ry = 1.0;
        
        int k;
        Component comp;
        panel = target;
        if (count <= 0){
            return;
        }
        if (newLayout) {
            preferredLayoutSize(target);
            newLayout = false;
        }
        if (!bEditMode && minSquish < 1.0) {
            if ((cw < actualDim.width) || (ch < actualDim.height)) {
                if (cw < actualDim.width) {
                    if (cw < minDim.width) {
                        rx = (double) cw / (double) minDim.width;
                        if (rx >= 0.98)
                            rx = 0.98;
                        if (rx < 0.4)
                            rx = 0.4;
                    }
                    rx += 1.0;
                }
                if (ch < actualDim.height) {
                    if (ch < minDim.height) {
                        ry = (double) ch / (double) minDim.height;
                        if (ry >= 0.98)
                            ry = 0.98;
                        if (ry < 0.4)
                            ry = 0.4;
                    }
                    ry += 1.0;
                }
            }
           //  if ((rx != xRatio) || (ry != yRatio)) {
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
           //  }
            return;
        }

        Dimension d;
        Point pt;
        for (k = 0; k < count; k++) {
            comp = target.getComponent(k);
            if( (comp != null) && comp.isVisible() ) {
                  d = comp.getPreferredSize();
                  pt = comp.getLocation();
                  if (d.width < 8 && d.height < 8) {
                      comp.setBounds(pt.x, pt.y, cw - pt.x, ch - pt.y);
                  }
                  else
                      comp.setBounds(pt.x, pt.y, d.width, d.height);
            }
        }
   }

   public void setEditMode(boolean s) {
        boolean oldMode = bEditMode;
        bEditMode = s;
        if (s) {
            xRatio = 1.0;
            yRatio = 1.0;
        }
        else {
            if (s != oldMode)
                newLayout = true;
        }
   } 
}

