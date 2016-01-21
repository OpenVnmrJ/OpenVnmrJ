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
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicLabelUI;

/**
 * @author Michael Hagen
 */
public class TextureLabelUI extends BasicLabelUI {

    private static TextureLabelUI hifiLabelUI = null;

    public static ComponentUI createUI(JComponent c) {
        if (hifiLabelUI == null) {
            hifiLabelUI = new TextureLabelUI();
        }
        return hifiLabelUI;
    }

    protected void paintEnabledText(JLabel l, Graphics g, String s, int textX, int textY) {
        int mnemIndex = -1;
        if (JTattooUtilities.getJavaVersion() >= 1.4) {
            mnemIndex = l.getDisplayedMnemonicIndex();
        } else {
            mnemIndex = JTattooUtilities.findDisplayedMnemonicIndex(l.getText(), l.getDisplayedMnemonic());
        }
        Color fc = l.getForeground();
        if (AbstractLookAndFeel.getTheme().isTextShadowOn() && ColorHelper.getGrayValue(fc) > 164) {
            g.setColor(Color.black);
            JTattooUtilities.drawStringUnderlineCharAt(l, g, s, mnemIndex, textX, textY + 1);
        }
        g.setColor(fc);
        JTattooUtilities.drawStringUnderlineCharAt(l, g, s, mnemIndex, textX, textY);
    }

    protected void paintDisabledText(JLabel l, Graphics g, String s, int textX, int textY) {
        int mnemIndex = -1;
        if (JTattooUtilities.getJavaVersion() >= 1.4) {
            mnemIndex = l.getDisplayedMnemonicIndex();
        } else {
            mnemIndex = JTattooUtilities.findDisplayedMnemonicIndex(l.getText(), l.getDisplayedMnemonic());
        }
        Graphics2D g2D = (Graphics2D) g;
        Composite savedComposite = g2D.getComposite();
        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.8f);
        g2D.setComposite(alpha);
        Color fc = l.getForeground();
        if (ColorHelper.getGrayValue(fc) > 164) {
            fc = ColorHelper.brighter(AbstractLookAndFeel.getDisabledForegroundColor(), 40);
            g.setColor(Color.black);
        } else {
            fc = AbstractLookAndFeel.getDisabledForegroundColor();
            g.setColor(Color.white);
        }
        JTattooUtilities.drawStringUnderlineCharAt(l, g, s, mnemIndex, textX, textY + 1);
        g2D.setComposite(savedComposite);
        g.setColor(fc);
        JTattooUtilities.drawStringUnderlineCharAt(l, g, s, mnemIndex, textX, textY);
    }

}
