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
import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.basic.*;
import javax.swing.text.View;

public class BaseToggleButtonUI extends BasicToggleButtonUI {

    private static Rectangle viewRect = new Rectangle();
    private static Rectangle textRect = new Rectangle();
    private static Rectangle iconRect = new Rectangle();
    protected static Color[] rolloverPressedColors = null;

    public static ComponentUI createUI(JComponent b) {
        return new BaseToggleButtonUI();
    }

    public void installDefaults(AbstractButton b) {
        super.installDefaults(b);
        b.setOpaque(false);
        b.setRolloverEnabled(true);
        Color cArr[] = AbstractLookAndFeel.getTheme().getPressedColors();
        rolloverPressedColors = new Color[cArr.length];
        for (int i = 0; i < cArr.length; i++) {
            rolloverPressedColors[i] = ColorHelper.brighter(cArr[i], 20);
        }
    }

    public void uninstallDefaults(AbstractButton b) {
        super.uninstallDefaults(b);
        b.setOpaque(true);
        b.setRolloverEnabled(false);
    }

    protected BasicButtonListener createButtonListener(AbstractButton b) {
        return new BaseButtonListener(b);
    }

    protected void paintBackground(Graphics g, AbstractButton b) {
        if (!b.isContentAreaFilled() || (b.getParent() instanceof JMenuBar)) {
            return;
        }

        int width = b.getWidth();
        int height = b.getHeight();
        ButtonModel model = b.getModel();
        Color colors[] = null;
        if (b.isEnabled()) {
            if (b.getBackground() instanceof ColorUIResource) {
                if (model.isPressed() && model.isArmed()) {
                    colors = AbstractLookAndFeel.getTheme().getPressedColors();
                } else  if (b.isRolloverEnabled() && model.isRollover()) {
                    if (model.isSelected()) {
                        colors = rolloverPressedColors;
                    } else {
                        colors = AbstractLookAndFeel.getTheme().getRolloverColors();
                    }
                } else if (model.isSelected()) {
                    colors = AbstractLookAndFeel.getTheme().getPressedColors();
                } else {
                    if (AbstractLookAndFeel.getTheme().doShowFocusFrame() && b.hasFocus()) {
                        colors = AbstractLookAndFeel.getTheme().getFocusColors();
                    } else {
                        colors = AbstractLookAndFeel.getTheme().getButtonColors();
                    }
                }
            } else {
                if (model.isPressed() && model.isArmed()) {
                    colors = ColorHelper.createColorArr(b.getBackground(), ColorHelper.darker(b.getBackground(), 50), 20);
                } else  if (b.isRolloverEnabled() && model.isRollover()) {
                    colors = ColorHelper.createColorArr(ColorHelper.brighter(b.getBackground(), 80), ColorHelper.brighter(b.getBackground(), 20), 20);
                } else if (model.isSelected()) {
                    colors = ColorHelper.createColorArr(b.getBackground(), ColorHelper.darker(b.getBackground(), 50), 20);
                } else {
                    colors = ColorHelper.createColorArr(ColorHelper.brighter(b.getBackground(), 40), ColorHelper.darker(b.getBackground(), 20), 20);
                }
            }
        } else { // disabled
            colors = AbstractLookAndFeel.getTheme().getDisabledColors();
        }
        JTattooUtilities.fillHorGradient(g, colors, 1, 1, width - 2, height - 2);
    }

    protected void paintText(Graphics g, AbstractButton b, Rectangle textRect, String text) {
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
            if ((model.isArmed() && model.isPressed()) || model.isSelected()) {
                offs = 1;
            }
            if (model.isRollover()) {
                g.setColor(AbstractLookAndFeel.getTheme().getRolloverForegroundColor());
            } else {
                g.setColor(b.getForeground());
            }
            JTattooUtilities.drawStringUnderlineCharAt(b, g, text, mnemIndex, textRect.x + offs, textRect.y + offs + fm.getAscent());
        } else {
            g.setColor(Color.white);
            JTattooUtilities.drawStringUnderlineCharAt(b, g, text, mnemIndex, textRect.x + 1, textRect.y + 1 + fm.getAscent());
            g.setColor(AbstractLookAndFeel.getDisabledForegroundColor());
            JTattooUtilities.drawStringUnderlineCharAt(b, g, text, mnemIndex, textRect.x, textRect.y + fm.getAscent());
        }
    }

    protected void paintFocus(Graphics g, AbstractButton b, Rectangle viewRect, Rectangle textRect, Rectangle iconRect) {
        g.setColor(AbstractLookAndFeel.getFocusColor());
        BasicGraphicsUtils.drawDashedRect(g, 4, 3, b.getWidth() - 8, b.getHeight() - 6);
    }

    public void paint(Graphics g, JComponent c) {
        Graphics2D g2D = (Graphics2D) g;

        AbstractButton b = (AbstractButton) c;
        Font f = c.getFont();
        g.setFont(f);
        FontMetrics fm = g.getFontMetrics();
        Insets insets = c.getInsets();

        viewRect.x = insets.left;
        viewRect.y = insets.top;
        viewRect.width = b.getWidth() - (insets.right + viewRect.x);
        viewRect.height = b.getHeight() - (insets.bottom + viewRect.y);

        textRect.x = textRect.y = textRect.width = textRect.height = 0;
        iconRect.x = iconRect.y = iconRect.width = iconRect.height = 0;

        String text = SwingUtilities.layoutCompoundLabel(
                c, fm, b.getText(), b.getIcon(),
                b.getVerticalAlignment(), b.getHorizontalAlignment(),
                b.getVerticalTextPosition(), b.getHorizontalTextPosition(),
                viewRect, iconRect, textRect,
                b.getText() == null ? 0 : defaultTextIconGap);

        paintBackground(g, b);

        if (b.getIcon() != null) {
            if (!b.isEnabled()) {
                Composite composite = g2D.getComposite();
                AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.5f);
                g2D.setComposite(alpha);
                paintIcon(g, c, iconRect);
                g2D.setComposite(composite);
            } else {
                paintIcon(g, c, iconRect);
            }
        }

        if (text != null && !text.equals("") && textRect != null) {
            View v = (View) c.getClientProperty(BasicHTML.propertyKey);
            if (v != null) {
                Object savedRenderingHint = null;
                if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
                    savedRenderingHint = g2D.getRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING);
                    g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, AbstractLookAndFeel.getTheme().getTextAntiAliasingHint());
                }
                v.paint(g, textRect);
                if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
                    g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, savedRenderingHint);
                }
            } else {
                paintText(g, b, textRect, text);
            }
        }

        if (b.isFocusPainted() && b.hasFocus()) {
            paintFocus(g, b, viewRect, textRect, iconRect);
        }
    }
}
