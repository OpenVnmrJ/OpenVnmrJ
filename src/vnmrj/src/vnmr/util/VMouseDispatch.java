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
import java.awt.event.*;

import vnmr.bo.*;

public class VMouseDispatch {

    public static void fireEvent(MouseEvent e, Component c, int x, int y) {
         MouseEvent me = new MouseEvent(c,
               e.getID(), e.getWhen(),
               e.getModifiersEx() | e.getModifiers(),
               x, y, e.getXOnScreen(), e.getYOnScreen(),
               e.getClickCount(),
               e.isPopupTrigger(),
               e.getButton());
         c.dispatchEvent(me);
    }

    public static boolean next(MouseEvent e, Container c, int x, int y) {
        int num = c.getComponentCount();
        boolean bFound = false;

        for (int i = 0; i < num; i++) {
             Component comp = c.getComponent(i);
             if (!comp.isVisible() || !comp.isEnabled())
                  continue;
             Point pt = comp.getLocation();
             Dimension dim = comp.getSize();
             if (x >= pt.x && x <= (pt.x + dim.width)) {
                 if (y >= pt.y && y <= (pt.y + dim.height)) {
                     if (comp instanceof VGroup)
                         bFound = next(e, (Container)comp, x - pt.x, y - pt.y);
                     else {
                         bFound = true;
                         fireEvent(e, comp, x - pt.x, y - pt.y);
                     }
                 }
                 if (bFound)
                    break;
             }
        }
        return bFound;
    }

    public static void dispatch(MouseEvent e) {
        Object obj = e.getSource();
        if (!(obj instanceof Container))
            return;
        Container c = (Container) obj;
        Container p = c.getParent();
        if (p == null)
            return;

        int num = p.getComponentCount();
        boolean bStart = false;
        boolean bFound = false;
        int x = e.getX();
        int y = e.getY();
        Point pt = c.getLocation();
        x = pt.x + x;
        y = pt.y + y;

        for (int i = 0; i < num; i++) {
             Component comp = p.getComponent(i);
             if (!bStart) {
                  if (comp == c)
                       bStart = true;
                  continue;
             }
             if (!comp.isVisible() || !comp.isEnabled())
                  continue;
             Point pt2 = comp.getLocation();
             Dimension dim = comp.getSize();
             if (x >= pt2.x && x <= (pt2.x + dim.width)) {
                 if (y >= pt2.y && y <= (pt2.y + dim.height)) {
                     if (comp instanceof VGroup)
                         bFound = next(e, (Container)comp, x - pt2.x, y - pt2.y);
                     else {
                         bFound = true;
                         fireEvent(e, comp, x - pt2.x, y - pt2.y);
                     }
                 }
             }
             if (bFound)
                 break;
        }
    }
}

