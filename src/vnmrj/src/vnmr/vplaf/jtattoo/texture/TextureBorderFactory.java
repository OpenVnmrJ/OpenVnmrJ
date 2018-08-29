/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright 2012 MH-Software-Entwicklung. All rights reserved.
 * Use is subject to license terms.
 */

package vnmr.vplaf.jtattoo.texture;

import vnmr.vplaf.jtattoo.AbstractBorderFactory;
import javax.swing.border.Border;

/**
 * @author Michael Hagen
 */
public class TextureBorderFactory implements AbstractBorderFactory {

    private static TextureBorderFactory instance = null;
    
    private TextureBorderFactory() {
    }
    
    public static synchronized TextureBorderFactory getInstance() {
        if (instance == null)
            instance = new TextureBorderFactory();
        return instance;
    }
    
    public Border getFocusFrameBorder() {
        return TextureBorders.getFocusFrameBorder();
    }

    public Border getButtonBorder() {
        return TextureBorders.getButtonBorder();
    }
    
    public Border getToggleButtonBorder() {
        return TextureBorders.getToggleButtonBorder();
    }
    
    public Border getTextBorder() {
        return TextureBorders.getTextBorder();
    }
    
    public Border getSpinnerBorder() {
        return TextureBorders.getSpinnerBorder();
    }
    
    public Border getTextFieldBorder() {
        return TextureBorders.getTextFieldBorder();
    }
    
    public Border getComboBoxBorder() {
        return TextureBorders.getComboBoxBorder();
    }
    
    public Border getTableHeaderBorder() {
        return TextureBorders.getTableHeaderBorder();
    }
    
    public Border getTableScrollPaneBorder() {
        return TextureBorders.getTableScrollPaneBorder();
    }

    public Border getScrollPaneBorder() {
        return TextureBorders.getScrollPaneBorder();
    }
    
    public Border getTabbedPaneBorder() {
        return TextureBorders.getTabbedPaneBorder();
    }
    
    public Border getMenuBarBorder() {
        return TextureBorders.getMenuBarBorder();
    }
    
    public Border getMenuItemBorder() {
        return TextureBorders.getMenuItemBorder();
    }
    
    public Border getPopupMenuBorder() {
        return TextureBorders.getPopupMenuBorder();
    }
    
    public Border getInternalFrameBorder() {
        return TextureBorders.getInternalFrameBorder();
    }
    
    public Border getPaletteBorder() {
        return TextureBorders.getPaletteBorder();
    }
    
    public Border getToolBarBorder() {
        return TextureBorders.getToolBarBorder();
    }
    
    public Border getProgressBarBorder() {
        return TextureBorders.getProgressBarBorder();
    }
    
    public Border getDesktopIconBorder() {
        return TextureBorders.getDesktopIconBorder();
    }
}

