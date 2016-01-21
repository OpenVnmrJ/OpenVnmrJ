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

import vnmr.vplaf.jtattoo.BaseComboBoxUI;
import vnmr.vplaf.jtattoo.NoFocusButton;
import java.awt.*;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.border.AbstractBorder;
import javax.swing.plaf.ComponentUI;

/**
 * @author Michael Hagen
 */
public class HiFiComboBoxUI extends BaseComboBoxUI {

    public static ComponentUI createUI(JComponent c) {
        return new HiFiComboBoxUI();
    }

    public JButton createArrowButton() {
        JButton button = new NoFocusButton(HiFiIcons.getComboBoxIcon());
        button.setBorder(new ArrowButtonBorder());
        return button;
    }

    protected void setButtonBorder() {
    }

//--------------------------------------------------------------------------------------------------    
    static class ArrowButtonBorder extends AbstractBorder {

        private static final Insets insets = new Insets(1, 3, 1, 2);
        private static final Color frameLoColor = new Color(120, 120, 120);
        private static final Color frameLowerColor = new Color(104, 104, 104);
        private static final Color frameLowerLoColor = new Color(64, 64, 64);
        private static final Color frameLowestColor = new Color(32, 32, 32);

        public void paintBorder(Component c, Graphics g, int x, int y, int w, int h) {
            Graphics2D g2D = (Graphics2D) g;
            g.translate(x, y);

            g.setColor(frameLoColor);
            g.drawLine(1, 0, w - 1, 0);
            g.drawLine(1, 1, 1, h - 2);
            g.setColor(frameLowerColor);
            g.drawLine(w - 1, 1, w - 1, h - 2);
            g.drawLine(2, h - 1, w - 2, h - 1);

            Composite composite = g2D.getComposite();
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.5f);
            g2D.setComposite(alpha);
            g.setColor(frameLowestColor);
            g.drawLine(2, 1, w - 2, 1);
            g.drawLine(2, 2, 2, h - 3);
            g.setColor(frameLowerLoColor);
            g.drawLine(0, 0, 0, h);
            g2D.setComposite(composite);

            g.translate(-x, -y);
        }

        public Insets getBorderInsets(Component c) {
            return new Insets(insets.top, insets.left, insets.bottom, insets.right);
        }

        public Insets getBorderInsets(Component c, Insets borderInsets) {
            borderInsets.left = insets.left;
            borderInsets.top = insets.top;
            borderInsets.right = insets.right;
            borderInsets.bottom = insets.bottom;
            return borderInsets;
        }

    }
}
