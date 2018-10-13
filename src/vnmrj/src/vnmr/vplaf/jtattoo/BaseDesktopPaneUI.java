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

import java.awt.Graphics;
import javax.swing.Icon;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicDesktopPaneUI;

/**
 * @author Michael Hagen
 */
public class BaseDesktopPaneUI extends BasicDesktopPaneUI {

    private static BaseDesktopPaneUI desktopPaneUI = null;

    public static ComponentUI createUI(JComponent c) {
        if (desktopPaneUI == null) {
            desktopPaneUI = new BaseDesktopPaneUI();
        }
        return desktopPaneUI;
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
}
