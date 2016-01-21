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
import java.awt.*;
import javax.swing.JComponent;
import javax.swing.JMenuBar;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicMenuBarUI;


/**
 * @author Michael Hagen
 */
public class TextureMenuBarUI extends BasicMenuBarUI {

    public static ComponentUI createUI(JComponent x) {
        return new TextureMenuBarUI();
    }

    public void installUI(JComponent c) {
        super.installUI(c);
        if ((c != null) && (c instanceof JMenuBar)) {
            ((JMenuBar) c).setBorder(TextureBorders.getMenuBarBorder());
            ((JMenuBar) c).setBorderPainted(true);
        }
    }

    public void paint(Graphics g, JComponent c) {
        TextureUtils.fillComponent(g, c, TextureUtils.MENUBAR_TEXTURE_TYPE);

        if (AbstractLookAndFeel.getTheme().isDarkTexture()) {
            Graphics2D g2D = (Graphics2D) g;
            Composite savedComposite = g2D.getComposite();
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.6f);
            g2D.setComposite(alpha);
            g2D.setColor(Color.black);
            g2D.drawLine(0, 0, c.getWidth() - 1, 0);
            alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.2f);
            g2D.setComposite(alpha);
            g2D.setColor(Color.white);
            g2D.drawLine(0, c.getHeight() - 1, c.getWidth() - 1, c.getHeight() - 1);
            g2D.setComposite(savedComposite);
        }
    }
}
