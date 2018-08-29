/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright 2012 MH-Software-Entwicklung. All rights reserved.
 * Use is subject to license terms.
 */
package vnmr.vplaf.jtattoo.texture;

import vnmr.vplaf.jtattoo.AbstractLookAndFeel;
import vnmr.vplaf.jtattoo.ColorHelper;
import java.awt.*;
import javax.swing.JComponent;
import javax.swing.JSeparator;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicSeparatorUI;

/**
 * @author Michael Hagen
 */
public class TextureSeparatorUI extends BasicSeparatorUI {

    private static final Dimension size = new Dimension(2, 3);

    public static ComponentUI createUI(JComponent c) {
        return new TextureSeparatorUI();
    }

    public void paint(Graphics g, JComponent c) {
        boolean horizontal = true;
        if (c instanceof JSeparator) {
            horizontal = (((JSeparator) c).getOrientation() == JSeparator.HORIZONTAL);
        }
        Graphics2D g2D = (Graphics2D) g;
        Composite savedComposite = g2D.getComposite();
        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.6f);
        g2D.setComposite(alpha);
        if (horizontal) {
            int w = c.getWidth();
            g.setColor(AbstractLookAndFeel.getBackgroundColor());
            g.drawLine(0, 0, w, 0);
            g.setColor(ColorHelper.darker(AbstractLookAndFeel.getBackgroundColor(), 30));
            g.drawLine(0, 1, w, 1);
            g.setColor(ColorHelper.brighter(AbstractLookAndFeel.getBackgroundColor(), 50));
            g.drawLine(0, 2, w, 2);
        } else {
            int h = c.getHeight();
            g.setColor(ColorHelper.darker(AbstractLookAndFeel.getBackgroundColor(), 30));
            g.drawLine(0, 0, 0, h);
            g.setColor(ColorHelper.brighter(AbstractLookAndFeel.getBackgroundColor(), 50));
            g.drawLine(1, 0, 1, h);
        }
        g2D.setComposite(savedComposite);
    }

    public Dimension getPreferredSize(JComponent c) {
        return size;
    }
}




