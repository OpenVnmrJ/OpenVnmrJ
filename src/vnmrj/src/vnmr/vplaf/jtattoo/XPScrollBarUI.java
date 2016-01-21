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
package vnmr.vplaf.jtattoo;

import vnmr.vplaf.jtattoo.texture.TextureScrollButton;
import java.awt.*;
import javax.swing.*;
import javax.swing.plaf.ComponentUI;

/**
 *
 * @author  Michael Hagen
 */
public class XPScrollBarUI extends BaseScrollBarUI {

    protected static Color rolloverColors[] = null;
    protected static Color dragColors[] = null;

    public static ComponentUI createUI(JComponent c) {
        return new XPScrollBarUI();
    }

    protected void installDefaults() {
        super.installDefaults();
        Color colors[] = AbstractLookAndFeel.getTheme().getThumbColors();
        rolloverColors = new Color[colors.length];
        dragColors = new Color[colors.length];
        for (int i = 0; i < colors.length; i++) {
            rolloverColors[i] = ColorHelper.brighter(colors[i], 16);
            dragColors[i] = ColorHelper.darker(colors[i], 8);
        }
    }

    protected JButton createDecreaseButton(int orientation) {
        return new TextureScrollButton(orientation, scrollBarWidth);
    }

    protected JButton createIncreaseButton(int orientation) {
        return new TextureScrollButton(orientation, scrollBarWidth);
    }

    protected Color getFrameColor() {
        return Color.white;
    }

    protected Color[] getThumbColors() {
        if (isDragging) {
            return dragColors;
        }
        if (isRollover) {
            return rolloverColors;
        }
        return AbstractLookAndFeel.getTheme().getThumbColors();
    }

    protected void paintThumb(Graphics g, JComponent c, Rectangle thumbBounds) {
        if (!c.isEnabled()) {
            return;
        }

        Graphics2D g2D = (Graphics2D) g;
        Composite savedComposite = g2D.getComposite();

        int x = thumbBounds.x;
        int y = thumbBounds.y;
        int width = thumbBounds.width;
        int height = thumbBounds.height;

        g.translate(x, y);

        Color[] colors = getThumbColors();
        if (scrollbar.getOrientation() == JScrollBar.VERTICAL) {
            JTattooUtilities.fillVerGradient(g, colors, 0, 0, width, height);
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.6f);
            g2D.setComposite(alpha);
            int dx = 6;
            int dy = height / 2 - 3;
            int dw = width - 13;
            Color c1 = ColorHelper.brighter(colors[0], 60);
            Color c2 = ColorHelper.darker(colors[0], 30);
            for (int i = 0; i < 4; i++) {
                g.setColor(c1);
                g.drawLine(dx, dy, dx + dw, dy);
                dy++;
                g.setColor(c2);
                g.drawLine(dx, dy, dx + dw, dy);
                dy++;
            }
        } else {
            JTattooUtilities.fillHorGradient(g, colors, 0, 0, width, height);
            int dx = width / 2 - 3;
            int dy = 6;
            int dh = height - 13;
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.8f);
            g2D.setComposite(alpha);
            Color c1 = ColorHelper.brighter(colors[0], 60);
            Color c2 = ColorHelper.darker(colors[0], 30);
            for (int i = 0; i < 4; i++) {
                g.setColor(c1);
                g.drawLine(dx, dy, dx, dy + dh);
                dx++;
                g.setColor(c2);
                g.drawLine(dx, dy, dx, dy + dh);
                dx++;
            }
        }

        g2D.setComposite(savedComposite);
        g.setColor(getFrameColor());
        g.drawLine(1, 1, width - 2, 1);
        g.drawLine(1, 2, 1, height - 3);
        g.drawLine(width - 2, 2, width - 2, height - 3);
        g.drawLine(2, height - 2, width - 3, height - 2);

        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.5f);
        g2D.setComposite(alpha);
        Color fc = colors[colors.length - 1];
        g2D.setColor(fc);
        g.drawLine(2, 2, width - 3, 2);
        g.drawLine(2, 3, 2, height - 3);

        g.setColor(ColorHelper.darker(fc, 40));
        g.drawLine(width - 1, 2, width - 1, height - 3);
        g.drawLine(3, height - 1, width - 3, height - 1);
        alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.2f);
        g2D.setComposite(alpha);
        g.drawLine(1, height - 2, 2, height - 1);
        g.drawLine(width - 1, height - 2, width - 2, height - 1);

        g.translate(-x, -y);
        g2D.setComposite(savedComposite);
    }
}
