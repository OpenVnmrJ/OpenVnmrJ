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
package vnmr.vplaf.jtattoo.mcwin;

import vnmr.vplaf.jtattoo.*;
import java.awt.Color;
import java.awt.Graphics;
import javax.swing.JComponent;
import javax.swing.JMenuBar;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class McWinMenuBarUI extends BaseMenuBarUI {

    private static final Color shadowColors[] = ColorHelper.createColorArr(Color.white, new Color(240, 240, 240), 8);

    public static ComponentUI createUI(JComponent x) {
        return new McWinMenuBarUI();
    }

    public void installUI(JComponent c) {
        super.installUI(c);
        if ((c != null) && (c instanceof JMenuBar)) {
            ((JMenuBar) c).setBorder(McWinBorders.getMenuBarBorder());
            ((JMenuBar) c).setBorderPainted(true);
        }
    }

    public void paint(Graphics g, JComponent c) {
        if (AbstractLookAndFeel.getTheme().isBackgroundPatternOn()) {
            McWinUtils.fillComponent(g, c, shadowColors);
        } else {
            super.paint(g, c);
        }
    }
}
