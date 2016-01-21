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

import javax.swing.border.Border;

/**
 * @author Michael Hagen
 */
public interface AbstractBorderFactory {

    public Border getFocusFrameBorder();

    public Border getButtonBorder();

    public Border getToggleButtonBorder();

    public Border getTextBorder();

    public Border getSpinnerBorder();

    public Border getTextFieldBorder();

    public Border getComboBoxBorder();

    public Border getTableHeaderBorder();

    public Border getTableScrollPaneBorder();

    public Border getScrollPaneBorder();

    public Border getTabbedPaneBorder();

    public Border getMenuBarBorder();

    public Border getMenuItemBorder();

    public Border getPopupMenuBorder();

    public Border getInternalFrameBorder();

    public Border getPaletteBorder();

    public Border getToolBarBorder();

    public Border getDesktopIconBorder();

    public Border getProgressBarBorder();
}

