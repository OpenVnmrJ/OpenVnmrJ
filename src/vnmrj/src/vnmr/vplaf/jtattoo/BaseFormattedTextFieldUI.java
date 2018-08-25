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
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import javax.swing.*;
import javax.swing.border.Border;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.UIResource;
import javax.swing.plaf.basic.BasicFormattedTextFieldUI;

/**
 * @author Michael Hagen
 */
public class BaseFormattedTextFieldUI extends BasicFormattedTextFieldUI {

    private Border orgBorder = null;
    private FocusListener focusListener = null;

    public static ComponentUI createUI(JComponent c) {
        return new BaseFormattedTextFieldUI();
    }

    protected void installListeners() {
        super.installListeners();

        if (AbstractLookAndFeel.getTheme().doShowFocusFrame()) {
            focusListener = new FocusListener() {

                public void focusGained(FocusEvent e) {
                    if (getComponent() != null) {
                        orgBorder = getComponent().getBorder();
                        LookAndFeel laf = UIManager.getLookAndFeel();
                        if (laf instanceof AbstractLookAndFeel && orgBorder instanceof UIResource) {
                            Border focusBorder = ((AbstractLookAndFeel)laf).getBorderFactory().getFocusFrameBorder();
                            getComponent().setBorder(focusBorder);
                        }
                        getComponent().invalidate();
                        getComponent().repaint();
                    }
                }

                public void focusLost(FocusEvent e) {
                    if (getComponent() != null) {
                        if (orgBorder instanceof UIResource) {
                            getComponent().setBorder(orgBorder);
                            getComponent().invalidate();
                            getComponent().repaint();
                        }
                    }
                }
            };
            getComponent().addFocusListener(focusListener);
        }
    }

    protected void uninstallListeners() {
        getComponent().removeFocusListener(focusListener);
        focusListener = null;
        super.uninstallListeners();
    }

    protected void paintBackground(Graphics g) {
        g.setColor(getComponent().getBackground());
        if (AbstractLookAndFeel.getTheme().doShowFocusFrame()) {
            if (getComponent().hasFocus() && getComponent().isEditable()) {
                g.setColor(AbstractLookAndFeel.getTheme().getFocusBackgroundColor());
            }
        }
        g.fillRect(0, 0, getComponent().getWidth(), getComponent().getHeight());
    }

    protected void paintSafely(Graphics g) {
        Graphics2D g2D = (Graphics2D) g;
        Object savedRenderingHint = null;
        if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
            savedRenderingHint = g2D.getRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING);
            g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, AbstractLookAndFeel.getTheme().getTextAntiAliasingHint());
        }
        super.paintSafely(g);
        if (AbstractLookAndFeel.getTheme().isTextAntiAliasingOn()) {
            g2D.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, savedRenderingHint);
        }
    }
}
