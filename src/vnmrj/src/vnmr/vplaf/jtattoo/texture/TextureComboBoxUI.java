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

import vnmr.vplaf.jtattoo.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.plaf.ComponentUI;

public class TextureComboBoxUI extends BaseComboBoxUI {

    public static ComponentUI createUI(JComponent c) {
        return new TextureComboBoxUI();
    }

    public JButton createArrowButton() {
        return new ArrowButton();
    }

    protected void setButtonBorder() {
    }

//--------------------------------------------------------------------------------------------------    
    static class ArrowButton extends NoFocusButton {

        public ArrowButton() {
            setBorder(BorderFactory.createEmptyBorder());
            setBorderPainted(false);
            setContentAreaFilled(false);
        }

        public void paint(Graphics g) {
            Graphics2D g2D = (Graphics2D) g;

            boolean isPressed = getModel().isPressed();
            boolean isRollover = getModel().isRollover();

            int width = getWidth();
            int height = getHeight();

            Color[] tc = AbstractLookAndFeel.getTheme().getThumbColors();
            Color c1 = tc[0];
            Color c2 = tc[tc.length - 1];
            if (isPressed) {
                c1 = ColorHelper.darker(c1, 5);
                c2 = ColorHelper.darker(c2, 5);
            } else if (isRollover) {
                c1 = ColorHelper.brighter(c1, 20);
                c2 = ColorHelper.brighter(c2, 20);
            }

            g2D.setPaint(new GradientPaint(0, 0, c1, width, height, c2));
            g2D.fillRect(0, 0, width, height);
            g2D.setPaint(null);
            g2D.setColor(Color.white);
            if (JTattooUtilities.isLeftToRight(this)) {
                g2D.drawRect(1, 0, width - 2, height - 1);
            } else {
                g2D.drawRect(0, 0, width - 2, height - 1);
            }

            Composite composite = g2D.getComposite();
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.6f);
            g2D.setComposite(alpha);
            g2D.setColor(c2);
            if (JTattooUtilities.isLeftToRight(this)) {
                g.drawLine(2, 1, width - 2, 1);
                g.drawLine(2, 2, 2, height - 2);
            } else {
                g.drawLine(1, 1, width - 3, 1);
                g.drawLine(1, 2, 1, height - 2);
            }
            g2D.setComposite(composite);

            // paint the icon
            Icon icon = LunaIcons.getComboBoxIcon();
            int x = (width - icon.getIconWidth()) / 2;
            int y = (height - icon.getIconHeight()) / 2;
            int dx = (JTattooUtilities.isLeftToRight(this)) ? 0 : -1;
            if (getModel().isPressed() && getModel().isArmed()) {
                icon.paintIcon(this, g, x + dx + 2, y + 1);
            } else {
                icon.paintIcon(this, g, x + dx + 1, y);
            }
        }
    } // end class ArrowButton
}
