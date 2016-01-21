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
package vnmr.vplaf.jtattoo;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Color;
import javax.swing.JComponent;
import javax.swing.JSeparator;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicSeparatorUI;

/**
 * @author Michael Hagen
 */
public class BaseSeparatorUI extends BasicSeparatorUI {

    private static final Dimension size = new Dimension(2, 3);

    public static ComponentUI createUI(JComponent c) {
        return new BaseSeparatorUI();
    }

    public void paint(Graphics g, JComponent c) {
        boolean horizontal = true;
        Color color = AbstractLookAndFeel.getSeparatorBackgroundColor();
        if (c instanceof JSeparator) {
            horizontal = (((JSeparator) c).getOrientation() == JSeparator.HORIZONTAL);
        }
        if (horizontal) {
            int w = c.getWidth();
            g.setColor(color);
            g.drawLine(0, 0, w, 0);
            g.setColor(ColorHelper.darker(color, 30));
            g.drawLine(0, 1, w, 1);
            g.setColor(ColorHelper.brighter(color, 50));
            g.drawLine(0, 2, w, 2);
        } else {
            int h = c.getHeight();
            g.setColor(ColorHelper.darker(color, 30));
            g.drawLine(0, 0, 0, h);
            g.setColor(ColorHelper.brighter(color, 50));
            g.drawLine(1, 0, 1, h);
        }
    }

    public Dimension getPreferredSize(JComponent c) {
        return size;
    }
}




