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

import vnmr.vplaf.jtattoo.BaseScrollPaneUI;
import javax.swing.*;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class HiFiScrollPaneUI extends BaseScrollPaneUI {

    public static ComponentUI createUI(JComponent c) {
        return new HiFiScrollPaneUI();
    }

    public void installDefaults(JScrollPane p) {
        super.installDefaults(p);
        p.getViewport().setScrollMode(JViewport.SIMPLE_SCROLL_MODE);
    }
}
