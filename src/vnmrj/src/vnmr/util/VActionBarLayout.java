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
public class VActionBarLayout implements LayoutManager {
    private Dimension prefDim = new Dimension(0, 0);
    private Dimension actualDim = new Dimension(0, 0);
    private int curCount = 0;
    private int visibleCount = 0;
    private int level = 0;
    private boolean newLayout = true;
    private boolean newMode = true;
    private boolean firstEdit = true;
    private boolean bSquish = false;
    private String name = "no name";
    protected static int prefHeight = 25;
    protected static int maxHeight = 25;
    private boolean bEditMode = false;
    static final Comparator<Component> XORDER = new Comparator<Component>() {
        public int compare(Component e1, Component e2) {
            Rectangle r1 = e1.getBounds();
            Rectangle r2 = e2.getBounds();
            // Point pt1 = e1.getLocation();
            // Point pt2 = e2.getLocation();
            return (r1.x < r2.x ? -1 : r1.x == r2.x ? 0 : 1);
        }
    };

    public VActionBarLayout() {
    }

    public VActionBarLayout(int level) {
        this.level = level;
    }

    private void setDebug(String s) {
        if (DebugOutput.isSetFor("VActionBarLayout"))
            Messages.postDebug("VActionBarLayout(" + name + ")" + s);
    }

    public int getLevel() {
        return level;
    }

    public void setEditMode(boolean s) {
        boolean oldMode = bEditMode;
        bEditMode = s;
        if (s != oldMode) {
            newMode = true;
            if (s)
                firstEdit = true;
            else
                firstEdit = false;
        }  
        else
            newMode = false;
        // System.out.println("SETEDITMODE new="+newMode);

    }

    public void setName(String str) {
        name = str;
    }

    public static void resetPreferedHeight() {
        prefHeight = 25;
    }

    void addLayoutComponent(Component comp, Object constraints) {
        newLayout = true;
    }

    public void addLayoutComponent(String name, Component comp) {
        newLayout = true;
    }

    public void removeLayoutComponent(Component comp) {
        newLayout = true;
    }

    public void setSquish(boolean b) {
        bSquish = b;
    }

    public Dimension maximumLayoutSize(Container target) {
        return new Dimension(Short.MAX_VALUE, Short.MAX_VALUE);
    }

    public Dimension minimumLayoutSize(Container target) {
        return new Dimension(0, 0);
    }

    private void sortChildren(Container target) {
        int count = target.getComponentCount();
        ArrayList<Component> list = new ArrayList<Component>(count);
        int k;
        int w = 0;
        Component comp;
        for (k = 0; k < count; k++) {
            comp = target.getComponent(k);
            list.add(comp);
        }
        Collections.sort(list, XORDER);
        target.removeAll();
        for (k = 0; k < count; k++) {
            comp = list.get(k);
            target.add(comp);
            Rectangle r1 = comp.getBounds();
            // System.out.println(" "+k+" "+r1.x+" "+r1.width+" "+(r1.x+r1.width));
        }
        comp = list.get(count - 1);
        Rectangle r = comp.getBounds();
        w = r.x + r.width;
        if (bEditMode)
            target.setPreferredSize(new Dimension(w, target.getHeight()));
        setDebug(".sortChildren width:" + w + " children:" + count);

    }

    public Dimension preferredLayoutSize(Container target) {
        int w = 0;
        int h = 0;
        int k;
        int count = target.getComponentCount();
        Component comp;
        Dimension d;
        Insets insets = target.getInsets();

        prefDim.width = w;
        prefDim.height = h;
        visibleCount = 0;
        if (count <= 0)
            return prefDim;
        for (k = 0; k < count; k++) {
            comp = target.getComponent(k);
            if ((comp != null) && (comp.isVisible()||bEditMode)) {
                d = comp.getPreferredSize();
                w += d.width;
                if (d.height > h)
                    h = d.height;
                visibleCount++;
            }
        }
        if (h > prefHeight) {
            prefHeight = h;
            if (prefHeight > maxHeight)
                prefHeight = maxHeight;
        }
        if (visibleCount < 1)
            visibleCount = 1;
        prefDim.width = w + insets.left + insets.right + (visibleCount - 1) * 4;
        prefDim.height = prefHeight;
        actualDim.height = prefDim.height;
        actualDim.width = w + visibleCount - 1;
        return prefDim;
    }

    public void layoutContainer(Container target) {
        synchronized (target.getTreeLock()) {
            Point pt;
            Insets insets = target.getInsets();
            int cw = target.getSize().width - insets.left - insets.right - 4;
            int ch = target.getSize().height;
            int count = target.getComponentCount();
            if (count <= 0) {
                curCount = 0;
                return;
            }
            setDebug(".layoutContainer init width:" + cw);

            Component comp;
            int k, x, y, w, h, xgap;
            boolean bChanged = false;
            Dimension d;

            if (cw < 10 || ch < 4)
                return;
            if (curCount != count) {
                newLayout = true;
                curCount = count;
            }
            if (newLayout)
                preferredLayoutSize(target);
            double ratio = 1.0;
            xgap = 4;
            if (cw < actualDim.width) {
                xgap = 1;
                ratio = (double) cw / (double) actualDim.width;
            } else if (cw < prefDim.width) {
                while (xgap > 1) {
                    xgap--;
                    w = cw - actualDim.width + visibleCount * xgap;
                    if (w >= 0)
                        break;
                }
            }
            if (ratio < 1.0)
                bChanged = true;

            x = 0;
            visibleCount = 0;
            for (k = 0; k < count; k++) {
                comp = target.getComponent(k);
                if (bEditMode && !firstEdit) {
                    Rectangle r=comp.getBounds();
                    comp.setBounds(r.x, 0, r.width, ch);
                    comp.setPreferredSize(new Dimension(r.width,ch));
                } else if(bEditMode || comp.isVisible()){
                    d = comp.getPreferredSize();
                    if (d == null) {
                        w = 10;
                        h = ch - 2;
                    } else {
                        w = d.width;
                        h = d.height;
                        if (w < 10)
                            w = 10;
                    }
                    if (bChanged)
                        w = (int) ((double) w * ratio);
                    if (bSquish)
                        comp.setBounds(x, 0, w, ch);
                    else {
                        if (comp instanceof VObjIF) {
                            pt = ((VObjIF) comp).getDefLoc();
                            x = pt.x;
                        }
                        y = ch - h;
                        if (y < 0)
                            y = 0;
                        comp.setBounds(x, y, w, h);
                    }
                    x += w + xgap;
                }
                // System.out.println(comp.getBounds());
            }
            if (newMode)
                sortChildren(target);
            firstEdit = false;
            newLayout = false;
        }
    }
}
