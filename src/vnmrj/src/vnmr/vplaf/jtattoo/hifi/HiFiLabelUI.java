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

import vnmr.vplaf.jtattoo.*;
import java.awt.Color;
import java.awt.Graphics;
import javax.swing.JComponent;
import javax.swing.JLabel;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.BasicLabelUI;

/**
 * @author Michael Hagen
 */
public class HiFiLabelUI extends BasicLabelUI {

    private static HiFiLabelUI hifiLabelUI = null;

    public static ComponentUI createUI(JComponent c) {
        if (hifiLabelUI == null) {
            hifiLabelUI = new HiFiLabelUI();
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
        if (AbstractLookAndFeel.getTheme().isTextShadowOn() && ColorHelper.getGrayValue(fc) > 128) {
            g.setColor(Color.black);
            JTattooUtilities.drawStringUnderlineCharAt(l, g, s, mnemIndex, textX + 1, textY + 1);
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
        g.setColor(Color.black);
        JTattooUtilities.drawStringUnderlineCharAt(l, g, s, mnemIndex, textX + 1, textY + 1);
        g.setColor(AbstractLookAndFeel.getDisabledForegroundColor());
        JTattooUtilities.drawStringUnderlineCharAt(l, g, s, mnemIndex, textX, textY);
    }
}

