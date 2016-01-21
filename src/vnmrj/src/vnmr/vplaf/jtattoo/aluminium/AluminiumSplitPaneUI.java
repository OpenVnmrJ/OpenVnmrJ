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

import vnmr.vplaf.jtattoo.BaseSplitPaneUI;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicSplitPaneDivider;

/**
 * @author Michael Hagen
 */
public class AluminiumSplitPaneUI extends BaseSplitPaneUI {

    public static ComponentUI createUI(JComponent c) {
        return new AluminiumSplitPaneUI();
    }

    public BasicSplitPaneDivider createDefaultDivider() {
        return new AluminiumSplitPaneDivider(this);
    }
}
