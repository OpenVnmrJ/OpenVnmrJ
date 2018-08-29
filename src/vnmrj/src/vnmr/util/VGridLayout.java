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
import java.util.LinkedList;
import java.util.Collections;
import javax.swing.*;

import vnmr.bo.*;

public class VGridLayout implements LayoutManager {
    private int columns = 1;
    private int rows = 1;
    private int halign = -1;
    private int valign = -1;
    private java.util.List<Component> compList;

    public VGridLayout() {
        this(1, 1);
    }

    public VGridLayout(int r, int c) {
        if (r > 0) 
            rows = r;
        if (c > 0)
            columns = c;
    }

    public void setColumns(int n) {
        columns = n;
        if (columns < 1)
            columns = 1;
    }

    public void setRows(int n) {
        rows = n;
        if (rows < 1)
           rows = 1;
    }

    public void setVerticalAlignment(int n) {
        valign = n;
    }

    public void setHorizontalAlignment(int n) {
        halign = n;
    }


    public void addLayoutComponent(String name, Component comp) {}

    public void removeLayoutComponent(Component comp) {}

    public Dimension preferredLayoutSize(Container target) {
        return new Dimension(0, 0);
    }

    public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0);
    }

    public void alignLayoutContainer(Container target) {
            Dimension dim = target.getSize();
            int nmembers = target.getComponentCount();
            double xgap = (double) dim.width / (double) columns;
            double ygap = (double) dim.height / (double) rows;
            int x, y, w, h, i, span, objW;
            int ixgap, iygap;
            VGrid vgrid;
            Component comp;

            if (compList == null)
                compList = Collections.synchronizedList(new LinkedList<Component>());
            ixgap = (int) xgap;
            iygap = (int) ygap;
            for (i = 0 ; i < nmembers ; i++) {
                comp = target.getComponent(i);
                if (!(comp instanceof VGrid)) {
                   Dimension dm = comp.getPreferredSize();
                   Point pt = comp.getLocation();
                   comp.setBounds(pt.x, pt.y, dm.width, dm.height);
                }
            }
            for (int col = 0; col < columns; col++) {
                compList.clear();
                w = 0;
                for (i = 0 ; i < nmembers ; i++) {
                    comp = target.getComponent(i);
                    if (comp instanceof VGrid) {
                        vgrid = (VGrid) comp;
                        x = vgrid.getGridColumn() - 1;
                        if (x < 0)  x = 0;
                        if (x >= columns)  x = columns - 1;
                        if (x == col) {
                            span = vgrid.getGridColumnSpan(); 
                            if (span == 1) {
                                objW = ((VGrid)comp).getRealSize().width; 
                                if (objW > w)
                                    w = objW;
                                compList.add(comp);
                            }
                            else {
                                y = vgrid.getGridRow() - 1;
                                if (y < 0)  y = 0;
                                if (y >= rows)  y = rows - 1;
                                if ((x + span) > columns)
                                    span = columns - x;
                                h = vgrid.getGridRowSpan();
                                if ((y + h) > rows)
                                     h = rows - y;
                                x = (int) (xgap * x);
                                y = (int) (ygap * y);
                                h = (int) (ygap * h);
                                objW = (int) (xgap * span);
                                comp.setBounds(x, y, objW, h);
                            }
                        }
                    }
                }
                if (w < 10)
                    w = ixgap;
                w += 6;
                if (w > ixgap)
                    w = ixgap;
                x = (int) (xgap * col);
                if (halign == SwingConstants.RIGHT)
                    x = x + ixgap - w;
                else if (halign == SwingConstants.CENTER)
                    x = x + (ixgap - w) / 2;
               
                int num = compList.size();
                for (i = 0 ; i < num ; i++) {
                    comp = compList.get(i);
                    vgrid = (VGrid) comp;
                    y = vgrid.getGridRow() - 1;
                    if (y < 0)  y = 0;
                    if (y >= rows)  y = rows - 1;
                    y = (int) (ygap * y);
                    h = (int) (ygap * vgrid.getGridRowSpan());
                    if ((y + h) > dim.height)
                       h = dim.height - y;
                    comp.setBounds(x, y, w, h);
                }
            }
            compList.clear();
    }

    public void layoutContainer(Container target) {
        synchronized (target.getTreeLock()) {
            Dimension dim = target.getSize();
            int nmembers = target.getComponentCount();
            if (dim.width < 10 || dim.height < 10)
                return;
            if (nmembers < 1)
               return;
            if (halign >= 0) {
               alignLayoutContainer(target);
               return;
            }

            double xgap = (double) dim.width / (double) columns;
            double ygap = (double) dim.height / (double) rows;
            int x, y, w, h;
            VGrid vgrid;
            Component comp;

            for (int i = 0 ; i < nmembers ; i++) {
                comp = target.getComponent(i);
                if (comp instanceof VGrid) {
                   vgrid = (VGrid) comp;
                   x = vgrid.getGridColumn() - 1;
                   y = vgrid.getGridRow() - 1;
                   w = vgrid.getGridColumnSpan();
                   h = vgrid.getGridRowSpan();
                   if (x < 0)  x = 0;
                   if (x >= columns)  x = columns - 1;
                   if (y < 0)  y = 0;
                   if (y >= rows)  y = rows - 1;
                   x = (int) (xgap * x); 
                   y = (int) ( ygap * y); 
                   w = (int) (xgap * w); 
                   h = (int) (ygap * h); 
                   if ((x + w) > dim.width)
                       w = dim.width - x;
                   if ((y + h) > dim.height)
                       h = dim.height - y;
                   comp.setBounds(x, y, w, h);
                }
                else {
                   Dimension dm = comp.getPreferredSize();
                   Point pt = comp.getLocation();
                   comp.setBounds(pt.x, pt.y, dm.width, dm.height);
                }
            }
        }
    }
}

