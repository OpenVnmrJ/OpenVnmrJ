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

import java.awt.FontMetrics;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicTableUI;

/**
 * @author  Michael Hagen
 */
public class BaseTableUI extends BasicTableUI {

    public static ComponentUI createUI(JComponent c) {
        return new BaseTableUI();
    }

    public void installDefaults() {
        super.installDefaults();
        // Setup the rowheight. The font may change if UI switches
        FontMetrics fm = table.getFontMetrics(table.getFont());
        table.setRowHeight(fm.getHeight() + (fm.getHeight() / 4));
    }

}
