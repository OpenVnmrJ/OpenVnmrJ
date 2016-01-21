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

public class H3Layout implements LayoutManager {
    public static int   x1 = 0;
    public static int   x2 = 0;
    public static int   vgap = 0;
    public static int   hgap = 10;

    public H3Layout() {
    }

    public void addLayoutComponent(String name, Component comp) {
    }

    public void removeLayoutComponent(Component comp) {
    }

    public static void  setFirstX(int x) {
          x1 = x;
    }

    public static void  setSecondX(int x) {
          x2 = x;
    }

    public static void  setHGap(int w) {
          hgap = w;
    }

    public static void  setVGap(int w) {
          vgap = w;
    }

    public Dimension preferredLayoutSize(Container target) {
       synchronized (target.getTreeLock()) {
            Dimension dim = new Dimension(0, 0);
            int nmembers = target.getComponentCount();

            for (int i = 0 ; i < nmembers ; i++) {
                Component m = target.getComponent(i);
                Dimension d = m.getPreferredSize();
                dim.height = Math.max(dim.height, d.height);
                dim.width += d.width;
            }
            Insets insets = target.getInsets();
            dim.width += insets.left + insets.right + hgap*2;
            dim.height += insets.top + insets.bottom + vgap*2;
            return dim;
      }
   }

   public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
   }

   public void layoutContainer(Container target) {
        synchronized (target.getTreeLock()) {
            int x = 0,y = 0;
            int nmembers = target.getComponentCount();
            Dimension d;
            Component m;

	    x = hgap;
            for (int i = 0; i < nmembers; i++) {
                m = target.getComponent(i);
		d = m.getPreferredSize();
                Rectangle rc2 = m.getBounds();
                m.setSize(d);
                m.setLocation(x, rc2.y);
		x = x + d.width;
                if (i == 0)
		    x = x1 + hgap;
                if (i == 1)
		    x = x2 + hgap;
            }
        }
   }
}
