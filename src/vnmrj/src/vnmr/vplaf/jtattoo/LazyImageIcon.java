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
public class LazyImageIcon implements Icon {

    private String name = null;
    private Icon icon = null;

    public LazyImageIcon(String name) {
        this.name = name;
    }

    private Icon getIcon() {
        if (icon == null) {
            try {
                icon = new ImageIcon(LazyImageIcon.class.getResource(name));
            } catch (Throwable t) {
                System.out.println("ERROR: loading image " + name + " failed!");
            }
        }
        return icon;
    }

    public int getIconHeight() {
        Icon ico = getIcon();
        if (ico != null) {
            return ico.getIconHeight();
        } else {
            return 16;
        }
    }

    public int getIconWidth() {
        Icon ico = getIcon();
        if (ico != null) {
            return ico.getIconWidth();
        } else {
            return 16;
        }
    }

    public void paintIcon(Component c, Graphics g, int x, int y) {
        Icon ico = getIcon();
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
