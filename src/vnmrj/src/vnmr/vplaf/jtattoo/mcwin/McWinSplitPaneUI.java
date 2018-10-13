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

import vnmr.vplaf.jtattoo.BaseSplitPaneUI;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicSplitPaneDivider;

/**
 * @author Michael Hagen
 */
public class McWinSplitPaneUI extends BaseSplitPaneUI {

    public static ComponentUI createUI(JComponent c) {
        return new McWinSplitPaneUI();
    }

    public BasicSplitPaneDivider createDefaultDivider() {
        return new McWinSplitPaneDivider(this);
    }
}
