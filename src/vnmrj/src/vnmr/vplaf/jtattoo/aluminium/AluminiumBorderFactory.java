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

import vnmr.vplaf.jtattoo.AbstractBorderFactory;
import javax.swing.border.Border;

/**
 * @author Michael Hagen
 */
public class AluminiumBorderFactory implements AbstractBorderFactory {

    private static AluminiumBorderFactory instance = null;

    private AluminiumBorderFactory() {
    }

    public static synchronized AluminiumBorderFactory getInstance() {
        if (instance == null) {
            instance = new AluminiumBorderFactory();
        }
        return instance;
    }

    public Border getFocusFrameBorder() {
        return AluminiumBorders.getFocusFrameBorder();
    }

    public Border getButtonBorder() {
        return AluminiumBorders.getButtonBorder();
    }

    public Border getToggleButtonBorder() {
        return AluminiumBorders.getToggleButtonBorder();
    }

    public Border getTextBorder() {
        return AluminiumBorders.getTextBorder();
    }

    public Border getSpinnerBorder() {
        return AluminiumBorders.getSpinnerBorder();
    }

    public Border getTextFieldBorder() {
        return AluminiumBorders.getTextFieldBorder();
    }

    public Border getComboBoxBorder() {
        return AluminiumBorders.getComboBoxBorder();
    }

    public Border getTableHeaderBorder() {
        return AluminiumBorders.getTableHeaderBorder();
    }

    public Border getTableScrollPaneBorder() {
        return AluminiumBorders.getTableScrollPaneBorder();
    }

    public Border getScrollPaneBorder() {
        return AluminiumBorders.getScrollPaneBorder();
    }

    public Border getTabbedPaneBorder() {
        return AluminiumBorders.getTabbedPaneBorder();
    }

    public Border getMenuBarBorder() {
        return AluminiumBorders.getMenuBarBorder();
    }

    public Border getMenuItemBorder() {
        return AluminiumBorders.getMenuItemBorder();
    }

    public Border getPopupMenuBorder() {
        return AluminiumBorders.getPopupMenuBorder();
    }

    public Border getInternalFrameBorder() {
        return AluminiumBorders.getInternalFrameBorder();
    }

    public Border getPaletteBorder() {
        return AluminiumBorders.getPaletteBorder();
    }

    public Border getToolBarBorder() {
        return AluminiumBorders.getToolBarBorder();
    }

    public Border getProgressBarBorder() {
        return AluminiumBorders.getProgressBarBorder();
    }

    public Border getDesktopIconBorder() {
        return AluminiumBorders.getDesktopIconBorder();
    }
}

