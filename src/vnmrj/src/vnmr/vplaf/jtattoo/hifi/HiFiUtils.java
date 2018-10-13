/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright 2005 MH-Software-Entwicklung. All rights reserved.
 * Use is subject to license terms.
 */
package vnmr.vplaf.jtattoo.hifi;

import vnmr.vplaf.jtattoo.AbstractLookAndFeel;
import vnmr.vplaf.jtattoo.JTattooUtilities;
import java.awt.*;

/**
 * @author  Michael Hagen
 */
public class HiFiUtils {

    private HiFiUtils() {
    }

    public static void fillComponent(Graphics g, Component c) {
        if (AbstractLookAndFeel.getTheme().isBackgroundPatternOn()) {
            int w = c.getWidth();
            int h = c.getHeight();
            Point p = JTattooUtilities.getRelLocation(c);
            int y = 2 - (p.y % 3);
            g.setColor(AbstractLookAndFeel.getTheme().getBackgroundColorLight());
            g.fillRect(0, 0, w, h);
            g.setColor(AbstractLookAndFeel.getTheme().getBackgroundColorDark());
            while (y < h) {
                g.drawLine(0, y, w, y);
                y += 3;
            }
        } else {
            g.setColor(c.getBackground());
            g.fillRect(0, 0, c.getWidth(), c.getHeight());
        }
    }
}
