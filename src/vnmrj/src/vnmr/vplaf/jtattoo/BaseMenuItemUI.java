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

import java.awt.*;
import javax.swing.*;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicMenuItemUI;

/**
 * @author Michael Hagen
 */
public class BaseMenuItemUI extends BasicMenuItemUI {

    public static ComponentUI createUI(JComponent c) {
        return new BaseMenuItemUI();
    }

    public void installUI(JComponent c) {
        super.installUI(c);
        c.setOpaque(false);
    }

    public void uninstallUI(JComponent c) {
        c.setOpaque(true);
        super.uninstallUI(c);
    }

    public void update(Graphics g, JComponent c) {
        paintBackground(g, c, 0, 0, c.getWidth(), c.getHeight());
        paint(g, c);
    }

    protected void paintBackground(Graphics g, JMenuItem menuItem, Color bgColor) {
        if (menuItem.isOpaque()) {
            int w = menuItem.getWidth();
            int h = menuItem.getHeight();
            paintBackground(g, menuItem, 0, 0, w, h);
        }
    }

    protected void paintBackground(Graphics g, JComponent c, int x, int y, int w, int h) {
        JMenuItem b = (JMenuItem) c;
        ButtonModel model = b.getModel();
        if (model.isArmed() || (c instanceof JMenu && model.isSelected())) {
            g.setColor(AbstractLookAndFeel.getMenuSelectionBackgroundColor());
            g.fillRect(x, y, w, h);
        } else if (!AbstractLookAndFeel.getTheme().isMenuOpaque()) {
            Graphics2D g2D = (Graphics2D) g;
            Composite composite = g2D.getComposite();
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, AbstractLookAndFeel.getTheme().getMenuAlpha());
            g2D.setComposite(alpha);
            g.setColor(AbstractLookAndFeel.getMenuBackgroundColor());
            g.fillRect(x, y, w, h);
            g2D.setComposite(composite);
        } else {
            g.setColor(AbstractLookAndFeel.getMenuBackgroundColor());
            g.fillRect(x, y, w, h);
        }
        if (menuItem.isSelected() && menuItem.isArmed()) {
            g.setColor(AbstractLookAndFeel.getMenuSelectionForegroundColor());
        } else {
            g.setColor(AbstractLookAndFeel.getMenuForegroundColor(menuItem.getForeground()));
        }
    }

    protected void paintText(Graphics g, JMenuItem menuItem, Rectangle textRect, String text) {
        Graphics2D g2D = (Graphics2D) g;
        Object savedRenderingHint = null;
        if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
            savedRenderingHint = g2D.getRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING);
            g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, AbstractLookAndFeel.getTheme().getTextAntiAliasingHint());
        }
        if (menuItem.isSelected() && menuItem.isArmed()) {
            g.setColor(AbstractLookAndFeel.getMenuSelectionForegroundColor());
        } else {
            g.setColor(AbstractLookAndFeel.getMenuForegroundColor(menuItem.getForeground()));
        }
        super.paintText(g, menuItem, textRect, text);
        if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
            g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, savedRenderingHint);
        }
    }
}
