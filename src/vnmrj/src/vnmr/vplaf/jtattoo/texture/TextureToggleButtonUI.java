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
import javax.swing.plaf.basic.BasicGraphicsUtils;

/**
 * @author Michael Hagen
 */
public class TextureToggleButtonUI extends BaseToggleButtonUI {

    public static ComponentUI createUI(JComponent c) {
        return new TextureToggleButtonUI();
    }

    protected void paintIcon(Graphics g, JComponent c, Rectangle iconRect) {
        AbstractButton b = (AbstractButton)c;
        Graphics2D g2D = (Graphics2D) g;
        Composite savedComposite = g2D.getComposite();
        if (!b.isContentAreaFilled()) {
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.9f);
            g2D.setComposite(alpha);
        }
        super.paintIcon(g, c, iconRect);
        g2D.setComposite(savedComposite);
    }

    protected void paintText(Graphics g, AbstractButton b, Rectangle textRect, String text) {
        Graphics2D g2D = (Graphics2D) g;
        Composite savedComposite = g2D.getComposite();
        ButtonModel model = b.getModel();
        FontMetrics fm = g.getFontMetrics();
        int mnemIndex = -1;
        if (JTattooUtilities.getJavaVersion() >= 1.4) {
            mnemIndex = b.getDisplayedMnemonicIndex();
        } else {
            mnemIndex = JTattooUtilities.findDisplayedMnemonicIndex(b.getText(), model.getMnemonic());
        }

        if (model.isEnabled()) {
            int offs = 0;
            if (model.isArmed() && model.isPressed()) {
                offs = 1;
            }
            Color fc = b.getForeground();
            if (model.isPressed() || model.isSelected()) {
                fc = AbstractLookAndFeel.getTheme().getPressedForegroundColor();
            } else if (b.isRolloverEnabled() && model.isRollover()) {
                fc = AbstractLookAndFeel.getTheme().getRolloverForegroundColor();
            }
            if (AbstractLookAndFeel.getTheme().isTextShadowOn() && ColorHelper.getGrayValue(fc) > 164) {
                AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.4f);
                g2D.setComposite(alpha);
                g.setColor(Color.black);
                JTattooUtilities.drawStringUnderlineCharAt(b, g, text, mnemIndex, textRect.x + offs, textRect.y + offs + fm.getAscent() + 1);
                g2D.setComposite(savedComposite);
            }
            g.setColor(fc);
            JTattooUtilities.drawStringUnderlineCharAt(b, g, text, mnemIndex, textRect.x + offs, textRect.y + offs + fm.getAscent());
        } else {
            AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.8f);
            g2D.setComposite(alpha);
            Color fc = b.getForeground();
            if (ColorHelper.getGrayValue(fc) > 164) {
                fc = ColorHelper.brighter(AbstractLookAndFeel.getDisabledForegroundColor(), 40);
                g.setColor(Color.black);
            } else {
                fc = AbstractLookAndFeel.getDisabledForegroundColor();
                g.setColor(Color.white);
            }
            JTattooUtilities.drawStringUnderlineCharAt(b, g, text, mnemIndex, textRect.x, textRect.y + 1 + fm.getAscent());
            g2D.setComposite(savedComposite);
            g.setColor(fc);
            JTattooUtilities.drawStringUnderlineCharAt(b, g, text, mnemIndex, textRect.x, textRect.y + fm.getAscent());
        }
    }

    protected void paintFocus(Graphics g, AbstractButton b, Rectangle viewRect, Rectangle textRect, Rectangle iconRect) {
        if (!AbstractLookAndFeel.getTheme().doShowFocusFrame()) {
            g.setColor(AbstractLookAndFeel.getFocusColor());
            BasicGraphicsUtils.drawDashedRect(g, 3, 2, b.getWidth() - 6, b.getHeight() - 5);
            BasicGraphicsUtils.drawDashedRect(g, 4, 3, b.getWidth() - 8, b.getHeight() - 7);
        }
    }

}
