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
package vnmr.vplaf.jtattoo.mcwin;

import vnmr.vplaf.jtattoo.*;
import java.awt.*;

/**
 * @author  Michael Hagen
 */
public class McWinUtils {

    private McWinUtils() {
    }

    public static void fillComponent(Graphics g, Component c, Color[] colors) {
        int w = c.getWidth();
        int h = c.getHeight();
        JTattooUtilities.fillHorGradient(g, colors, 0, 0, w, h);
        if (AbstractLookAndFeel.getTheme().isBackgroundPatternOn()) {
            Point p = JTattooUtilities.getRelLocation(c);
            int y = 2 - (p.y % 3);
            Color lc = McWinLookAndFeel.getTheme().getBackgroundColorDark();
            Color lineColor = ColorHelper.brighter(ColorHelper.median(lc, colors[colors.length - 1]), 50);
            while (y < h) {
                g.setColor(lineColor);
                g.drawLine(0, y, w, y);
                lineColor = ColorHelper.darker(lineColor, 1.5);
                y += 3;
            }
        }
    }

    public static void fillComponent(Graphics g, Component c) {
        if (AbstractLookAndFeel.getTheme().isBackgroundPatternOn()) {
            int w = c.getWidth();
            int h = c.getHeight();
            Point p = JTattooUtilities.getRelLocation(c);
            int y = 2 - (p.y % 3);
            g.setColor(McWinLookAndFeel.getTheme().getBackgroundColorLight());
            g.fillRect(0, 0, w, h);
            g.setColor(McWinLookAndFeel.getTheme().getBackgroundColorDark());
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
