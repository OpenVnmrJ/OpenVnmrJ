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

import java.awt.Graphics;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicTreeUI;

/**
 * author Michael Hagen
 */
public class BaseTreeUI extends BasicTreeUI {

    public static ComponentUI createUI(JComponent c) {
        return new BaseTreeUI();
    }

    protected void paintVerticalLine(Graphics g, JComponent c, int x, int top, int bottom) {
        drawDashedVerticalLine(g, x, top, bottom);
    }

    protected void paintHorizontalLine(Graphics g, JComponent c, int y, int left, int right) {
        drawDashedHorizontalLine(g, y, left, right);
    }
}


