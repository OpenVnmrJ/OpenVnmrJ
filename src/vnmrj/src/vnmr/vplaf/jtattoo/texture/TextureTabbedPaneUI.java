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
import java.awt.geom.*;
import javax.swing.JComponent;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.UIResource;
import javax.swing.text.View;

/**
 * @author Michael Hagen
 */
public class TextureTabbedPaneUI extends BaseTabbedPaneUI {

    public static ComponentUI createUI(JComponent c) {
        return new TextureTabbedPaneUI();
    }

    protected void installComponents() {
        simpleButtonBorder = true;
        super.installComponents();
    }

//    protected boolean isContentOpaque() {
//        return false;
//    }

    protected Color[] getContentBorderColors(int tabPlacement) {
        Color c = AbstractLookAndFeel.getTheme().getSelectionBackgroundColorDark();
        return new Color[] { getSelectedBorderColor(0), c, c, c, ColorHelper.darker(c, 10) };
    }

    protected Color getSelectedBorderColor(int tabIndex) {
        if (AbstractLookAndFeel.getTheme().isDarkTexture()) {
            return ColorHelper.darker(super.getSelectedBorderColor(tabIndex), 20);
        } else {
            return super.getSelectedBorderColor(tabIndex);
        }
    }

    protected Color getLoBorderColor(int tabIndex) {
        Color backColor = tabPane.getBackgroundAt(tabIndex);
        if (backColor instanceof UIResource || tabIndex == rolloverIndex) {
            return AbstractLookAndFeel.getFrameColor();
        } else {
            return ColorHelper.darker(backColor, 20);
        }
    }
    
    protected Color getLoGapBorderColor(int tabIndex) {
        Color backColor = tabPane.getBackgroundAt(tabIndex);
        if (backColor instanceof UIResource) {
            return AbstractLookAndFeel.getFrameColor();
        } else {
            return ColorHelper.darker(backColor, 20);
        }
    }
    
    protected Font getTabFont(boolean isSelected) {
        if (isSelected)
            return super.getTabFont(isSelected).deriveFont(Font.BOLD);
        else
            return super.getTabFont(isSelected);
    }

    protected int getTexture() {
        return TextureUtils.BACKGROUND_TEXTURE_TYPE;
    }

    protected int getSelectedTexture() {
        return TextureUtils.SELECTED_TEXTURE_TYPE;
    }

    protected int getUnSelectedTexture(int tabIndex) {
        if (tabIndex == rolloverIndex && tabPane.isEnabledAt(tabIndex)) {
            return TextureUtils.ROLLOVER_TEXTURE_TYPE;
        }
        return TextureUtils.ALTER_BACKGROUND_TEXTURE_TYPE;
    }

    protected void paintContentBorder(Graphics g, int tabPlacement, int selectedIndex, int x, int y, int w, int h) {
        int textureType = TextureUtils.getTextureType(tabPane);
        if (isContentOpaque()) {
            TextureUtils.fillComponent(g, tabPane, textureType);
        }
        if (!AbstractLookAndFeel.getTheme().isDarkTexture()) {
            super.paintContentBorder(g, tabPlacement, selectedIndex, x, y, w, h);
            return;
        }
        int sepHeight = tabAreaInsets.bottom;
        if (sepHeight > 0) {
            Insets bi = new Insets(0, 0, 0, 0);
            if (tabPane.getBorder() != null) {
                bi = tabPane.getBorder().getBorderInsets(tabPane);
            }
            switch (tabPlacement) {
                case TOP: {
                    int tabAreaHeight = calculateTabAreaHeight(tabPlacement, runCount, maxTabHeight);
                    TextureUtils.fillComponent(g, tabPane, x, y + tabAreaHeight - sepHeight + bi.top, w, sepHeight, getSelectedTexture());
                    if (textureType == TextureUtils.MENUBAR_TEXTURE_TYPE) {
                        Graphics2D g2D = (Graphics2D) g;
                        Composite saveComposite = g2D.getComposite();
                        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.5f);
                        g2D.setComposite(alpha);
                        g2D.setColor(Color.black);
                        g2D.drawLine(x, y, w, y);
                        g2D.drawLine(w, y, w, y + tabAreaHeight - sepHeight);
                        alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.2f);
                        g2D.setComposite(alpha);
                        g2D.setColor(Color.white);
                        g2D.drawLine(x, y + tabAreaHeight - sepHeight, w, y + tabAreaHeight - sepHeight);
                        alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.1f);
                        g2D.setComposite(alpha);
                        g2D.drawLine(x, y + 1, x, y + tabAreaHeight - sepHeight - 1);
                        g2D.setComposite(saveComposite);
                    }
                    break;
                }
                case LEFT: {
                    int tabAreaWidth = calculateTabAreaWidth(tabPlacement, runCount, maxTabWidth);
                    TextureUtils.fillComponent(g, tabPane, x + tabAreaWidth - sepHeight + bi.left, y, sepHeight, h, getSelectedTexture());
                    break;
                }
                case BOTTOM: {
                    int tabAreaHeight = calculateTabAreaHeight(tabPlacement, runCount, maxTabHeight);
                    TextureUtils.fillComponent(g, tabPane, x, y + h - tabAreaHeight - bi.bottom, w, sepHeight, getSelectedTexture());
                    break;
                }
                case RIGHT: {
                    int tabAreaWidth = calculateTabAreaWidth(tabPlacement, runCount, maxTabWidth);
                    TextureUtils.fillComponent(g, tabPane, x + w - tabAreaWidth - bi.right, y, sepHeight, h, getSelectedTexture());
                    break;
                }
            }
        }
    }

    protected void paintTabBackground(Graphics g, int tabPlacement, int tabIndex, int x, int y, int w, int h, boolean isSelected) {
        Color backColor = tabPane.getBackgroundAt(tabIndex);
        if (!(backColor instanceof UIResource) || !AbstractLookAndFeel.getTheme().isDarkTexture()) {
            super.paintTabBackground(g, tabPlacement, tabIndex, x, y, w, h, isSelected);
            return;
        }
        if (isTabOpaque() || isSelected) {
            Graphics2D g2D = (Graphics2D) g;
            Composite savedComposite = g2D.getComposite();
            Shape savedClip = g.getClip();
            Area orgClipArea = new Area(savedClip);
            int d = 2 * GAP;
            switch (tabPlacement) {
                case TOP:
                default:
                    if (isSelected) {
                        Area clipArea = new Area(new RoundRectangle2D.Double(x, y, w , h + 4, d, d));
                        Area rectArea = new Area(new Rectangle2D.Double(x, y, w, h + 1));
                        clipArea.intersect(rectArea);
                        clipArea.intersect(orgClipArea);
                        g2D.setClip(clipArea);
                        TextureUtils.fillRect(g, tabPane, x, y, w, h + 4, getSelectedTexture());
                        g2D.setClip(savedClip);
                    } else {
                        Area clipArea = new Area(new RoundRectangle2D.Double(x, y, w, h + 4, d, d));
                        Area rectArea = new Area(new Rectangle2D.Double(x, y, w, h));
                        clipArea.intersect(rectArea);
                        clipArea.intersect(orgClipArea);
                        g2D.setClip(clipArea);
                        TextureUtils.fillRect(g, tabPane, x, y, w, h + 4, getUnSelectedTexture(tabIndex));
                        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.5f);
                        g2D.setComposite(alpha);
                        Color colors[] = AbstractLookAndFeel.getTheme().getButtonColors();
                        JTattooUtilities.fillHorGradient(g, colors, x, y, w, h + 4);
                        g2D.setComposite(savedComposite);
                        g2D.setClip(savedClip);
                    }
                    break;
                case LEFT:
                    if (isSelected) {
                        TextureUtils.fillComponent(g, tabPane, x + 1, y + 1, w, h - 1, getSelectedTexture());
                    } else {
                        TextureUtils.fillComponent(g, tabPane, x + 1, y + 1, w - 1, h - 1, getUnSelectedTexture(tabIndex));
                    }
                    break;
                case BOTTOM:
                    if (isSelected) {
                        Area clipArea = new Area(new RoundRectangle2D.Double(x, y - 4, w, h + 4, d, d));
                        Area rectArea = new Area(new Rectangle2D.Double(x, y - 1, w, h + 1));
                        clipArea.intersect(rectArea);
                        clipArea.intersect(orgClipArea);
                        g2D.setClip(clipArea);
                        TextureUtils.fillRect(g, tabPane, x, y - 4, w, h + 4, getSelectedTexture());
                        g2D.setClip(savedClip);
                    } else {
                        Area clipArea = new Area(new RoundRectangle2D.Double(x, y - 4, w, h + 4, d, d));
                        Area rectArea = new Area(new Rectangle2D.Double(x, y, w, h));
                        clipArea.intersect(rectArea);
                        clipArea.intersect(orgClipArea);
                        g2D.setClip(clipArea);
                        TextureUtils.fillRect(g, tabPane, x, y - 4, w, h + 4, getUnSelectedTexture(tabIndex));
                        AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.5f);
                        g2D.setComposite(alpha);
                        Color colors[] = AbstractLookAndFeel.getTheme().getButtonColors();
                        JTattooUtilities.fillHorGradient(g, colors, x, y - 4, w, h + 4);
                        g2D.setComposite(savedComposite);
                        g2D.setClip(savedClip);
                    }
                    break;
                case RIGHT:
                    if (isSelected) {
                        TextureUtils.fillComponent(g, tabPane, x, y + 1, w, h - 1, getSelectedTexture());
                    } else {
                        TextureUtils.fillComponent(g, tabPane, x, y + 1, w, h - 1, getUnSelectedTexture(tabIndex));
                    }
                    break;
            }
        }
    }

    protected void paintText(Graphics g, int tabPlacement, Font font, FontMetrics metrics, int tabIndex, String title, Rectangle textRect, boolean isSelected) {
        Color backColor = tabPane.getBackgroundAt(tabIndex);
        if (!(backColor instanceof UIResource) || !AbstractLookAndFeel.getTheme().isDarkTexture()) {
            super.paintText(g, tabPlacement, font, metrics, tabIndex, title, textRect, isSelected);
            return;
        }
        g.setFont(font);
        View v = getTextViewForTab(tabIndex);
        if (v != null) {
            // html
            Graphics2D g2D = (Graphics2D)g;
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
            // plain text
            int mnemIndex = -1;
            if (JTattooUtilities.getJavaVersion() >= 1.4)
                mnemIndex = tabPane.getDisplayedMnemonicIndexAt(tabIndex);

            if (tabPane.isEnabled() && tabPane.isEnabledAt(tabIndex)) {
                if (isSelected) {
                    Color titleColor = AbstractLookAndFeel.getWindowTitleForegroundColor();
                    if (ColorHelper.getGrayValue(titleColor) > 164)
                        g.setColor(Color.black);
                    else
                        g.setColor(Color.white);
                    JTattooUtilities.drawStringUnderlineCharAt(tabPane, g, title, mnemIndex, textRect.x + 1, textRect.y + 1 + metrics.getAscent());
                    g.setColor(titleColor);
                } else {
                    g.setColor(tabPane.getForegroundAt(tabIndex));
                }

                if (isSelected) {
                    Graphics2D g2D = (Graphics2D)g;
                    Shape savedClip = g2D.getClip();

                    Area clipArea = new Area(new Rectangle2D.Double(textRect.x, textRect.y, textRect.width, textRect.height / 2 + 1));
                    clipArea.intersect(new Area(savedClip));
                    g2D.setClip(clipArea);
                    JTattooUtilities.drawStringUnderlineCharAt(tabPane, g, title, mnemIndex, textRect.x, textRect.y + metrics.getAscent());

                    clipArea = new Area(new Rectangle2D.Double(textRect.x, textRect.y + (textRect.height / 2) + 1, textRect.width, textRect.height));
                    clipArea.intersect(new Area(savedClip));
                    g2D.setClip(clipArea);
                    g2D.setColor(ColorHelper.darker(AbstractLookAndFeel.getWindowTitleForegroundColor(), 20));
                    JTattooUtilities.drawStringUnderlineCharAt(tabPane, g, title, mnemIndex, textRect.x, textRect.y + metrics.getAscent());
                    g2D.setClip(savedClip);
                } else {
                    JTattooUtilities.drawStringUnderlineCharAt(tabPane, g, title, mnemIndex, textRect.x, textRect.y + metrics.getAscent());
                }

            } else { // tab disabled
                Graphics2D g2D = (Graphics2D) g;
                Composite savedComposite = g2D.getComposite();
                AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.6f);
                g2D.setComposite(alpha);
                g.setColor(Color.white);
                JTattooUtilities.drawStringUnderlineCharAt(tabPane, g, title, mnemIndex, textRect.x, textRect.y + metrics.getAscent() + 1);
                g2D.setComposite(savedComposite);
                g.setColor(AbstractLookAndFeel.getDisabledForegroundColor());
                JTattooUtilities.drawStringUnderlineCharAt(tabPane, g, title, mnemIndex, textRect.x, textRect.y + metrics.getAscent());
            }
        }
    }

}
