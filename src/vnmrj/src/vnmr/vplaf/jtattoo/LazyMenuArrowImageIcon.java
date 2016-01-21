/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.vplaf.jtattoo;

import java.awt.*;
import javax.swing.Icon;
import javax.swing.ImageIcon;

/**
 *
 * @author Michael Hagen
 */
public class LazyMenuArrowImageIcon implements Icon {

    private String leftToRightName = null;
    private String rightToLefttName = null;
    private Icon leftToRightIcon = null;
    private Icon rightToLeftIcon = null;

    public LazyMenuArrowImageIcon(String leftToRightName, String rightToLefttName) {
        this.leftToRightName = leftToRightName;
        this.rightToLefttName = rightToLefttName;
    }

    private Icon getLeftToRightIcon() {
        if (leftToRightIcon == null) {
            try {
                leftToRightIcon = new ImageIcon(LazyMenuArrowImageIcon.class.getResource(leftToRightName));
            } catch (Throwable t) {
                System.out.println("ERROR: loading image " + leftToRightName + " failed!");
            }
        }
        return leftToRightIcon;
    }

    private Icon getRightToLeftIcon() {
        if (rightToLeftIcon == null) {
            try {
                rightToLeftIcon = new ImageIcon(LazyMenuArrowImageIcon.class.getResource(rightToLefttName));
            } catch (Throwable t) {
                System.out.println("ERROR: loading image " + rightToLefttName + " failed!");
            }
        }
        return rightToLeftIcon;
    }
    
    private Icon getIcon(Component c) {
       if (JTattooUtilities.isLeftToRight(c)) {
           return getLeftToRightIcon();
       } else {
           return getRightToLeftIcon();
       }
    }
    
    public int getIconHeight() {
        Icon ico = getIcon(null);
        if (ico != null) {
            return ico.getIconHeight();
        } else {
            return 16;
        }
    }

    public int getIconWidth() {
        Icon ico = getIcon(null);
        if (ico != null) {
            return ico.getIconWidth();
        } else {
            return 16;
        }
    }

    public void paintIcon(Component c, Graphics g, int x, int y) {
        Icon ico = getIcon(c);
        if (ico != null) {
            ico.paintIcon(c, g, x, y);
        } else {
            g.setColor(Color.red);
            g.fillRect(x, y, 16, 16);
            g.setColor(Color.white);
            g.drawLine(x, y, x + 15, y + 15);
            g.drawLine(x + 15, y, x, y + 15);
        }
    }

}
