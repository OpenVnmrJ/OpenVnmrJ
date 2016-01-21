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

import vnmr.vplaf.jtattoo.BasePanelUI;
import vnmr.vplaf.jtattoo.AbstractLookAndFeel;
import java.awt.Graphics;
import java.awt.Color;
import javax.swing.JComponent;
import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.ComponentUI;
import vnmr.ui.VnmrjUiNames;

/**
 * @author Michael Hagen
 */
public class HiFiPanelUI extends BasePanelUI {

    private static HiFiPanelUI panelUI = null;

    public static ComponentUI createUI(JComponent c) {
        if (panelUI == null) {
            panelUI = new HiFiPanelUI();
        }
        return panelUI;
    }

    public void update(Graphics g, JComponent c) {
        if (!c.isOpaque())
            return;
        Color color = c.getBackground();
        if (color == null)
            return;
        if ((color instanceof ColorUIResource)) {
            if (c.getClientProperty(VnmrjUiNames.PanelTexture) == null) {
                HiFiUtils.fillComponent(g, c);
            }
            else {
               //  System.out.println("  not ColorUIResource "+c.getClass().getName());
                int w = c.getWidth();
                int h = c.getHeight();
                g.setColor(AbstractLookAndFeel.getTheme().getBackgroundColor());
                g.fillRect(0, 0, w, h);
            }
            return;
        }
        super.update(g, c);

        /***  origin 
        if (c.isOpaque() && c.getBackground() instanceof ColorUIResource && c.getClientProperty("backgroundTexture") == null) {
            HiFiUtils.fillComponent(g, c);
        } else {
            super.update(g, c);
        }
        ***/
    }
}
