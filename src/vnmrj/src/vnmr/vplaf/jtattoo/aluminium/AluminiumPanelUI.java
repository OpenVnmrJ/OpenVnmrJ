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
package vnmr.vplaf.jtattoo.aluminium;

import vnmr.vplaf.jtattoo.BasePanelUI;
import java.awt.Graphics;
import javax.swing.JComponent;
import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class AluminiumPanelUI extends BasePanelUI {

    private static AluminiumPanelUI panelUI = null;

    public static ComponentUI createUI(JComponent c) {
        if (panelUI == null) {
            panelUI = new AluminiumPanelUI();
        }
        return panelUI;
    }

    public void update(Graphics g, JComponent c) {
        if (c.isOpaque() && c.getBackground() instanceof ColorUIResource && c.getClientProperty("backgroundTexture") == null) {
            AluminiumUtils.fillComponent(g, c);
        } else {
            super.update(g, c);
        }
    }
}
