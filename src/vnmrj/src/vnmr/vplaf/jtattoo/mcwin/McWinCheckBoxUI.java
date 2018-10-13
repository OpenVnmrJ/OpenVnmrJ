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

import javax.swing.*;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class McWinCheckBoxUI extends McWinRadioButtonUI {

    private static McWinCheckBoxUI checkBoxUI = null;

    public static ComponentUI createUI(JComponent b) {
        if (checkBoxUI == null) {
            checkBoxUI = new McWinCheckBoxUI();
        }
        return checkBoxUI;
    }

    public void installDefaults(AbstractButton b) {
        super.installDefaults(b);
        icon = UIManager.getIcon("CheckBox.icon");
    }
}
