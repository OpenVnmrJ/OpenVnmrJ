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

import vnmr.vplaf.jtattoo.*;
import java.awt.Color;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;

/**
 *
 * @author  Michael Hagen
 */
public class HiFiScrollBarUI extends XPScrollBarUI {

    private static final Color FRAME_COLOR = new Color(112, 112, 112);

    public static ComponentUI createUI(JComponent c) {
        return new HiFiScrollBarUI();
    }

    protected void installDefaults() {
        super.installDefaults();
        Color colors[] = AbstractLookAndFeel.getTheme().getThumbColors();
        rolloverColors = new Color[colors.length];
        dragColors = new Color[colors.length];
        for (int i = 0; i < colors.length; i++) {
            rolloverColors[i] = ColorHelper.brighter(colors[i], 8);
            dragColors[i] = ColorHelper.darker(colors[i], 8);
        }
    }

    protected JButton createDecreaseButton(int orientation) {
        return new HiFiScrollButton(orientation, scrollBarWidth);
    }

    protected JButton createIncreaseButton(int orientation) {
        return new HiFiScrollButton(orientation, scrollBarWidth);
    }

    protected Color getFrameColor() {
        if (isDragging) {
            return ColorHelper.darker(FRAME_COLOR, 8);
        } else if (isRollover) {
            return ColorHelper.brighter(FRAME_COLOR, 16);
        } else {
            return FRAME_COLOR;
        }
    }

}
