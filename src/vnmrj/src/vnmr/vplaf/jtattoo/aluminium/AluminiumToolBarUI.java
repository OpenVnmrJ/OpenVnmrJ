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

import vnmr.vplaf.jtattoo.AbstractToolBarUI;
import java.awt.Graphics;
import javax.swing.JComponent;
import javax.swing.border.Border;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class AluminiumToolBarUI extends AbstractToolBarUI {

    public static ComponentUI createUI(JComponent c) {
        return new AluminiumToolBarUI();
    }

    public Border getRolloverBorder() {
        return AluminiumBorders.getRolloverToolButtonBorder();
    }

    public Border getNonRolloverBorder() {
        return AluminiumBorders.getToolButtonBorder();
    }

    public boolean isButtonOpaque() {
        return false;
    }

    public void paint(Graphics g, JComponent c) {
        AluminiumUtils.fillComponent(g, c);
    }
}

