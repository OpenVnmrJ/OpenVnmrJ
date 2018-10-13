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

import java.awt.*;
import javax.swing.JComponent;
import javax.swing.JSeparator;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicSeparatorUI;

/**
 * @author Michael Hagen
 */
public class AluminiumPopupMenuSeparatorUI extends BasicSeparatorUI {

    private static final Dimension size = new Dimension(1, 1);

    public static ComponentUI createUI(JComponent c) {
        return new AluminiumPopupMenuSeparatorUI();
    }

    public void paint(Graphics g, JComponent c) {
        boolean horizontal = true;
        if (c instanceof JSeparator) {
            JSeparator sep = ((JSeparator) c);
            horizontal = (sep.getOrientation() == JSeparator.HORIZONTAL);
        }
        if (horizontal) {
            g.setColor(Color.lightGray);
            g.drawLine(0, 0, c.getWidth(), 0);
        } else {
            g.setColor(Color.lightGray);
            g.drawLine(0, 0, 0, c.getHeight());
        }
    }

    public Dimension getPreferredSize(JComponent c) {
        return size;
    }
}




