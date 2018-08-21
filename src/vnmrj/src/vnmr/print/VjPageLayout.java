/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.*;

public class VjPageLayout implements LayoutManager {

    public static int PAPER = 1;
    public static int SUB_AREA = 2;
    private double paperRatio;
    private double leftMargin;
    private double rightMargin;
    private double topMargin;
    private double bottomMargin;
    private int  GAP = 4;
    private int  targetType;

    public VjPageLayout(int type) {
         this.targetType = type;
         this.paperRatio = 1.0;  // height / width
         this.leftMargin = 0.1;
         this.rightMargin = 0.1;
         this.topMargin = 0.1;
         this.bottomMargin = 0.1;
    }

    public VjPageLayout() {
         this(PAPER);
    }

    public void addLayoutComponent(String name, Component comp) {
    }

    public void addLayoutComponent(Component comp, Object constraints) {
    }

    public void removeLayoutComponent(Component comp) {
    }

    public void setTargetType(int s) {
        targetType = s;
    }

    public void setMargin(double top, double left, double bottom, double right) {
        if (top >= 0.0 && top < 1.0)
            topMargin = top;
        if (left >= 0.0 && left < 1.0)
            leftMargin = left;
        if (bottom >= 0.0 && bottom < 1.0)
            bottomMargin = bottom;
        if (right >= 0.0 && right < 1.0)
            rightMargin = right;
    }

    public Dimension preferredLayoutSize(Container target) {
       synchronized (target.getTreeLock()) {
	 return new Dimension(0, 0);
       }
    }

    public Dimension minimumLayoutSize(Container target) {
      synchronized (target.getTreeLock()) {
	 return new Dimension(0, 0);
      }
    }

    private void setAreaBounds(Container container) {
        int nmembers = container.getComponentCount();
        if (nmembers < 1)
            return;
	Rectangle rc = container.getBounds();
        double dw = rc.width;
        double dh = rc.height;
        if (dw < 10 || dh < 10)
            return;

        int x = (int) (dw * leftMargin);
        int y = (int) (dh * topMargin);
        int w = (int) (dw * (1.0 - rightMargin)) - x;
        int h = (int) (dh * (1.0 - bottomMargin)) - y;
	for (int i = 0; i < nmembers; i++) {
	    Component c = container.getComponent(i);
            c.setBounds(new Rectangle(x, y, w, h));
        }
    }

    private void setPaperBounds(Container container) {
        int nmembers = container.getComponentCount();
        if (nmembers < 1)
            return;
	Rectangle rc = container.getBounds();
        double dw = rc.width - GAP * 2;
        double dh = rc.height - GAP * 2;
        if (dw < 10 || dh < 10)
            return;

        double r = paperRatio;
        if (r <= 0.0)
            r = 1.0;
        double r1 = 1.0;
        double rh = dh;
        while (true) {
            rh = dw * r * r1;
            if (rh <= dh)
                break;
            r1 = r1 - (rh - dh) / rh;
            if (r1 < 0.2)
                break;
        }
        dw = dw * r1;
        int w = (int)dw + 2;
        int h = (int)(dw * r) + 2;
	for (int i = 0; i < nmembers; i++) {
	    Component c = container.getComponent(i);
            c.setBounds(new Rectangle(GAP, GAP, w, h));
        }
    }
  
    public void setSizeRatio(double r) {
        paperRatio = r;
    }
    

    public void layoutContainer(Container target) {
      synchronized (target.getTreeLock()) {
        int nmembers = target.getComponentCount();

        if (nmembers < 1)
            return;
        if (targetType == PAPER)
            setPaperBounds(target);
        else
            setAreaBounds(target);
      }
    }
}
