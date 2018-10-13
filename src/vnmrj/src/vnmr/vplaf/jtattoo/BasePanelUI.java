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

import java.awt.*;
import javax.swing.*;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicPanelUI;

/**
 * @author Michael Hagen
 */
public class BasePanelUI extends BasicPanelUI {

    private static BasePanelUI panelUI = null;

    public static ComponentUI createUI(JComponent c) {
        if (panelUI == null) {
            panelUI = new BasePanelUI();
        }
        return panelUI;
    }

    protected void installDefaults(JPanel p) {
        super.installDefaults(p);
        p.setFont(AbstractLookAndFeel.getTheme().getControlTextFont());
    }

    public void update(Graphics g, JComponent c) {
        if (c.isOpaque()) {
            Object backgroundTexture = c.getClientProperty("backgroundTexture");
            if (backgroundTexture instanceof Icon) {
                JTattooUtilities.fillComponent(g, c, (Icon)backgroundTexture);
            } else {
                g.setColor(c.getBackground());
                g.fillRect(0, 0, c.getWidth(), c.getHeight());
            }
        }
    }

    public void paint(Graphics g, JComponent c) {
        Graphics2D g2D = (Graphics2D) g;
        Object savedRenderingHint = null;
        if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
            savedRenderingHint = g2D.getRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING);
            g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, AbstractLookAndFeel.getTheme().getTextAntiAliasingHint());
        }
        super.paint(g, c);
        if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
            g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, savedRenderingHint);
        }
    }
}
