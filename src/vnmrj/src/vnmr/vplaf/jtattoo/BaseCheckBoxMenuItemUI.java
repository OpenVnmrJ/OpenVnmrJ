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

import javax.swing.JComponent;
import javax.swing.UIManager;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class BaseCheckBoxMenuItemUI extends BaseMenuItemUI {

    public static ComponentUI createUI(JComponent c) {
        return new BaseCheckBoxMenuItemUI();
    }

    protected void installDefaults() {
        super.installDefaults();
        checkIcon = UIManager.getIcon("CheckBoxMenuItem.checkIcon");
    }

}
