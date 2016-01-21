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

import vnmr.vplaf.jtattoo.*;
import java.awt.Graphics;
import javax.swing.JInternalFrame;

/**
 * @author Michael Hagen
 */
public class AluminiumInternalFrameTitlePane extends BaseInternalFrameTitlePane {

    public AluminiumInternalFrameTitlePane(JInternalFrame f) {
        super(f);
    }

    public void paintBorder(Graphics g) {
        if (isActive()) {
            g.setColor(AbstractLookAndFeel.getTheme().getWindowBorderColor());
        } else {
            g.setColor(AbstractLookAndFeel.getTheme().getWindowInactiveBorderColor());
        }
        g.drawLine(0, getHeight() - 1, getWidth(), getHeight() - 1);
    }

    public void paintText(Graphics g, int x, int y, String title) {
        if (isActive()) {
            g.setColor(AbstractLookAndFeel.getWindowTitleBackgroundColor());
            JTattooUtilities.drawString(frame, g, title, x + 1, y + 1);
            g.setColor(AbstractLookAndFeel.getWindowTitleForegroundColor());
            JTattooUtilities.drawString(frame, g, title, x, y);
        } else {
            g.setColor(AbstractLookAndFeel.getWindowTitleBackgroundColor());
            JTattooUtilities.drawString(frame, g, title, x + 1, y + 1);
            g.setColor(AbstractLookAndFeel.getWindowInactiveTitleForegroundColor());
            JTattooUtilities.drawString(frame, g, title, x, y);
        }
    }
}
