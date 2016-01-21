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
import vnmr.vplaf.jtattoo.BaseRadioButtonUI;
import java.awt.*;
import javax.swing.JComponent;
import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicGraphicsUtils;

/**
 * @author Michael Hagen
 */
public class TextureRadioButtonUI extends BaseRadioButtonUI {

    private static TextureRadioButtonUI radioButtonUI = null;

    public static ComponentUI createUI(JComponent c) {
        if (radioButtonUI == null) {
            radioButtonUI = new TextureRadioButtonUI();
        }
        return radioButtonUI;
    }

    public void paintBackground(Graphics g, JComponent c) {
        if (c.isOpaque()) {
            if ((c.getBackground().equals(AbstractLookAndFeel.getBackgroundColor())) && (c.getBackground() instanceof ColorUIResource)) {
                TextureUtils.fillComponent(g, c, TextureUtils.BACKGROUND_TEXTURE_TYPE);
            } else {
                g.setColor(c.getBackground());
                g.fillRect(0, 0, c.getWidth(), c.getHeight());
            }
        }
    }

    protected void paintFocus(Graphics g, Rectangle t, Dimension d) {
        g.setColor(AbstractLookAndFeel.getFocusColor());
        BasicGraphicsUtils.drawDashedRect(g, t.x - 3, t.y - 1, t.width + 6, t.height + 2);
        BasicGraphicsUtils.drawDashedRect(g, t.x - 2, t.y, t.width + 4, t.height);
    }

}
