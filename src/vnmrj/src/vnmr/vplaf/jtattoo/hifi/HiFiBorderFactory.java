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

import vnmr.vplaf.jtattoo.AbstractBorderFactory;
import javax.swing.border.Border;

/**
 * @author Michael Hagen
 */
public class HiFiBorderFactory implements AbstractBorderFactory {

    private static HiFiBorderFactory instance = null;

    private HiFiBorderFactory() {
    }

    public static synchronized HiFiBorderFactory getInstance() {
        if (instance == null) {
            instance = new HiFiBorderFactory();
        }
        return instance;
    }

    public Border getFocusFrameBorder() {
        return HiFiBorders.getFocusFrameBorder();
    }

    public Border getButtonBorder() {
        return HiFiBorders.getButtonBorder();
    }

    public Border getToggleButtonBorder() {
        return HiFiBorders.getToggleButtonBorder();
    }

    public Border getTextBorder() {
        return HiFiBorders.getTextBorder();
    }

    public Border getSpinnerBorder() {
        return HiFiBorders.getSpinnerBorder();
    }

    public Border getTextFieldBorder() {
        return HiFiBorders.getTextFieldBorder();
    }

    public Border getComboBoxBorder() {
        return HiFiBorders.getComboBoxBorder();
    }

    public Border getTableHeaderBorder() {
        return HiFiBorders.getTableHeaderBorder();
    }

    public Border getTableScrollPaneBorder() {
        return HiFiBorders.getTableScrollPaneBorder();
    }

    public Border getScrollPaneBorder() {
        return HiFiBorders.getScrollPaneBorder();
    }

    public Border getTabbedPaneBorder() {
        return HiFiBorders.getTabbedPaneBorder();
    }

    public Border getMenuBarBorder() {
        return HiFiBorders.getMenuBarBorder();
    }

    public Border getMenuItemBorder() {
        return HiFiBorders.getMenuItemBorder();
    }

    public Border getPopupMenuBorder() {
        return HiFiBorders.getPopupMenuBorder();
    }

    public Border getInternalFrameBorder() {
        return HiFiBorders.getInternalFrameBorder();
    }

    public Border getPaletteBorder() {
        return HiFiBorders.getPaletteBorder();
    }

    public Border getToolBarBorder() {
        return HiFiBorders.getToolBarBorder();
    }

    public Border getProgressBarBorder() {
        return HiFiBorders.getProgressBarBorder();
    }

    public Border getDesktopIconBorder() {
        return HiFiBorders.getDesktopIconBorder();
    }
}

