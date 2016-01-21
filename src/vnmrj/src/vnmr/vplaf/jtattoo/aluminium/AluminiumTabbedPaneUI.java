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

import vnmr.vplaf.jtattoo.*;
import java.awt.*;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.UIResource;

/**
 * @author Michael Hagen
 */
public class AluminiumTabbedPaneUI extends BaseTabbedPaneUI {

    private static final Color TOP_SELECTED_TAB_COLORS[] = ColorHelper.createColorArr(new Color(204, 206, 202), new Color(220, 222, 218), 20);
    private static final Color BOTTOM_SELECTED_TAB_COLORS[] = ColorHelper.createColorArr(new Color(220, 222, 218), new Color(204, 206, 202), 20);

    public static ComponentUI createUI(JComponent c) {
        return new AluminiumTabbedPaneUI();
    }

    public void installDefaults() {
        super.installDefaults();
        tabAreaInsets = new Insets(2, 6, 2, 6);
        contentBorderInsets = new Insets(0, 0, 0, 0);
    }

    protected Font getTabFont(boolean isSelected) {
        if (isSelected) {
            return super.getTabFont(isSelected).deriveFont(Font.BOLD);
        } else {
            return super.getTabFont(isSelected);
        }
    }

    protected Color[] getTabColors(int tabIndex, boolean isSelected) {
        Color backColor = tabPane.getBackgroundAt(tabIndex);
        if ((backColor instanceof UIResource) && isSelected) {
            if (tabPane.getTabPlacement() == BOTTOM) {
                return BOTTOM_SELECTED_TAB_COLORS;
            } else {
                return TOP_SELECTED_TAB_COLORS;
            }
        } 
        return super.getTabColors(tabIndex, isSelected);
    }

    protected boolean isContentOpaque() {
        return false;
    }

    protected boolean hasInnerBorder() {
        return true;
    }

    protected void paintTabBackground(Graphics g, int tabPlacement, int tabIndex, int x, int y, int w, int h, boolean isSelected) {
        Color backColor = tabPane.getBackgroundAt(tabIndex);
        if (!(backColor instanceof UIResource)) {
            super.paintTabBackground(g, tabPlacement, tabIndex, x, y, w, h, isSelected);
            return;
        }
        if (JTattooUtilities.isMac()) {
            if (isSelected) {
                Color colorArr[] = getTabColors(tabIndex, isSelected);
                switch (tabPlacement) {
                    case LEFT:
                        JTattooUtilities.fillHorGradient(g, colorArr, x + 1, y + 1, w + 1, h - 1);
                        break;
                    case RIGHT:
                        JTattooUtilities.fillHorGradient(g, colorArr, x - 1, y + 1, w + 1, h - 1);
                        break;
                    case BOTTOM:
                        JTattooUtilities.fillHorGradient(g, colorArr, x + 1, y - 1, w - 1, h);
                        break;
                    case TOP:
                    default:
                        JTattooUtilities.fillHorGradient(g, colorArr, x + 1, y + 1, w - 1, h + 1);
                        break;
                }
            } else {
                super.paintTabBackground(g, tabPlacement, tabIndex, x, y, w, h, isSelected);
            }
        } else {
            if (isSelected) {
                if (tabPane.getBackgroundAt(tabIndex) instanceof UIResource) {
                    g.setColor(AluminiumLookAndFeel.getBackgroundColor());
                    if (tabPlacement == TOP)
                        AluminiumUtils.fillComponent(g, tabPane, x + 1, y + 1, w - 1, h + 1);
                    else if (tabPlacement == LEFT)
                        AluminiumUtils.fillComponent(g, tabPane, x + 1, y + 1, w + 1, h - 1);
                    else if (tabPlacement == BOTTOM)
                        AluminiumUtils.fillComponent(g, tabPane, x + 1, y - 2, w - 1, h + 1);
                    else
                        AluminiumUtils.fillComponent(g, tabPane, x - 1, y + 1, w + 1, h - 1);
                }
                else
                    super.paintTabBackground(g, tabPlacement, tabIndex, x, y, w, h, isSelected);
            }
            else {
                super.paintTabBackground(g, tabPlacement, tabIndex, x, y, w, h, isSelected);
            }
        }
    }

}
