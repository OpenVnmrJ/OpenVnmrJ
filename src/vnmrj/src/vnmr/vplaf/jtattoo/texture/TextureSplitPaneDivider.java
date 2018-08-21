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
import vnmr.vplaf.jtattoo.BaseSplitPaneDivider;
import java.awt.*;
import javax.swing.*;

/**
 * @author Michael Hagen
 */
public class TextureSplitPaneDivider extends BaseSplitPaneDivider {

    public TextureSplitPaneDivider(TextureSplitPaneUI ui) {
        super(ui);
    }

    public void paint(Graphics g) {
        TextureUtils.fillComponent(g, this, TextureUtils.getTextureType(splitPane));

        Graphics2D g2D = (Graphics2D) g;
        Composite savedComposite = g2D.getComposite();
        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.5f);
        g2D.setComposite(alpha);

        int width = getSize().width;
        int height = getSize().height;
        int dx = 0;
        int dy = 0;
        if ((width % 2) == 1) {
            dx = 1;
        }
        if ((height % 2) == 1) {
            dy = 1;
        }

        Icon horBumps = null;
        Icon verBumps = null;
        if (UIManager.getLookAndFeel() instanceof AbstractLookAndFeel) {
            AbstractLookAndFeel laf = (AbstractLookAndFeel) UIManager.getLookAndFeel();
            horBumps = laf.getIconFactory().getSplitterHorBumpIcon();
            verBumps = laf.getIconFactory().getSplitterVerBumpIcon();
        }
        if (orientation == JSplitPane.HORIZONTAL_SPLIT) {
            if ((horBumps != null) && (width > horBumps.getIconWidth())) {
                if (splitPane.isOneTouchExpandable() && centerOneTouchButtons) {
                    int centerY = height / 2;
                    int x = (width - horBumps.getIconWidth()) / 2 + dx;
                    int y = centerY - horBumps.getIconHeight() - 40;
                    horBumps.paintIcon(this, g, x, y);
                    y = centerY + 40;
                    horBumps.paintIcon(this, g, x, y);
                } else {
                    int x = (width - horBumps.getIconWidth()) / 2 + dx;
                    int y = (height - horBumps.getIconHeight()) / 2;
                    horBumps.paintIcon(this, g, x, y);
                }
            }
        } else {
            if ((verBumps != null) && (height > verBumps.getIconHeight())) {
                if (splitPane.isOneTouchExpandable() && centerOneTouchButtons) {
                    int centerX = width / 2;
                    int x = centerX - verBumps.getIconWidth() - 40;
                    int y = (height - verBumps.getIconHeight()) / 2 + dy;
                    verBumps.paintIcon(this, g, x, y);
                    x = centerX + 40;
                    verBumps.paintIcon(this, g, x, y);
                } else {
                    int x = (width - verBumps.getIconWidth()) / 2;
                    int y = (height - verBumps.getIconHeight()) / 2 + dy;
                    verBumps.paintIcon(this, g, x, y);
                }
            }
        }
        g2D.setComposite(savedComposite);
        paintComponents(g);
    }
}
