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
import java.awt.event.*;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import javax.swing.*;
import javax.swing.border.Border;
import javax.swing.plaf.ComponentUI;
import javax.swing.plaf.UIResource;
import javax.swing.plaf.basic.BasicComboBoxUI;
// import sun.swing.DefaultLookup;

import vnmr.ui.ComboBoxTitleIF;

public class BaseComboBoxUI extends BasicComboBoxUI {

    private PropertyChangeListener propertyChangeListener = null;
    private FocusListener focusListener = null;
    private Border orgBorder = null;
    private Color orgBackgroundColor = null;
    private String vjTitle = null;
    private ComboBoxTitleIF vjMenu;
    private MouseAdapter ml;
    private Insets inset;

    public static ComponentUI createUI(JComponent c) {
        return new BaseComboBoxUI();
    }

    private void createMouseAdapter() {
        if (ml != null)
            return;
        ml = new MouseAdapter() {
            public void mouseEntered(MouseEvent evt) {
                if (vjMenu != null)
                    vjMenu.enterPopup();
            }

            public void mouseExited(MouseEvent evt) {
                if (vjMenu != null)
                    vjMenu.exitPopup();
            }
        };
    }


    public void installUI(JComponent c) {
        vjMenu = null;
        super.installUI(c);
        if (c instanceof ComboBoxTitleIF)
            vjMenu = (ComboBoxTitleIF) c;
        
        comboBox.setRequestFocusEnabled(true);
        if (comboBox.getEditor() != null) {
            if (comboBox.getEditor().getEditorComponent() instanceof JTextField) {
                ((JTextField) (comboBox.getEditor().getEditorComponent())).setBorder(BorderFactory.createEmptyBorder(1, 1, 1, 1));
            }
            if (listBox != null && vjMenu != null) {
                if (ml == null)
                   createMouseAdapter();
                listBox.addMouseListener(ml);
                vjMenu.listBoxMouseAdded();
            }
        }
        inset = UIManager.getInsets("ComboBox.padding");
    }

    protected void installListeners() {
        super.installListeners();
        propertyChangeListener = new PropertyChangeHandler();
        comboBox.addPropertyChangeListener(propertyChangeListener);

        if (AbstractLookAndFeel.getTheme().doShowFocusFrame()) {
            focusListener = new FocusListener() {

                public void focusGained(FocusEvent e) {
                    if (comboBox != null) {
                        orgBorder = comboBox.getBorder();
                        orgBackgroundColor = comboBox.getBackground();
                        LookAndFeel laf = UIManager.getLookAndFeel();
                        if (laf instanceof AbstractLookAndFeel) {
                            if (orgBorder instanceof UIResource) {
                                Border focusBorder = ((AbstractLookAndFeel)laf).getBorderFactory().getFocusFrameBorder();
                                comboBox.setBorder(focusBorder);
                            }
                            Color backgroundColor = AbstractLookAndFeel.getTheme().getFocusBackgroundColor();
                            comboBox.setBackground(backgroundColor);
                        }
                    }
                }

                public void focusLost(FocusEvent e) {
                    if (comboBox != null) {
                        if (orgBorder instanceof UIResource) {
                            comboBox.setBorder(orgBorder);
                        }
                        comboBox.setBackground(orgBackgroundColor);
                    }
                }
            };
            comboBox.addFocusListener(focusListener);
        }
    }

    protected void uninstallListeners() {
        comboBox.removePropertyChangeListener(propertyChangeListener);
        comboBox.removeFocusListener(focusListener);
        propertyChangeListener = null;
        focusListener = null;
        super.uninstallListeners();
    }

    public Dimension getPreferredSize(JComponent c) {
        Dimension size = super.getPreferredSize(c);
        return new Dimension(size.width + 2, size.height + 2);
    }

    public JButton createArrowButton() {
        JButton button = new ArrowButton();
        if (JTattooUtilities.isLeftToRight(comboBox)) {
            Border border = BorderFactory.createMatteBorder(0, 1, 0, 0, AbstractLookAndFeel.getFrameColor());
            button.setBorder(border);
        } else {
            Border border = BorderFactory.createMatteBorder(0, 0, 0, 1, AbstractLookAndFeel.getFrameColor());
            button.setBorder(border);
        }
        return button;
    }

    public void paintCurrentValue(Graphics g,Rectangle bounds,boolean hasFocus) {
        // String strValue = vjTitle;
        String strValue = null;
        if (vjMenu != null) {
            if (vjMenu.getDefault())
                strValue = vjMenu.getDefaultLabel();
            else
                strValue = vjMenu.getTitleLabel();
        }
        if (strValue == null) {
            super.paintCurrentValue(g, bounds, hasFocus);
            return;
        }
        ListCellRenderer renderer = comboBox.getRenderer();
        Component c;
        if (renderer == null)
            return;
        if ( hasFocus && !isPopupVisible(comboBox) ) {
            c = renderer.getListCellRendererComponent( listBox,
                                                       strValue,
                                                       -1,
                                                       true,
                                                       false );
        }
        else {
            c = renderer.getListCellRendererComponent( listBox,
                                                       strValue,
                                                       -1,
                                                       false,
                                                       false );
            c.setBackground(UIManager.getColor("ComboBox.background"));
        }
        c.setFont(comboBox.getFont());
        if ( hasFocus && !isPopupVisible(comboBox) ) {
            c.setForeground(listBox.getSelectionForeground());
            c.setBackground(listBox.getSelectionBackground());
        }
        else {
            if ( comboBox.isEnabled() ) {
                c.setForeground(comboBox.getForeground());
                c.setBackground(comboBox.getBackground());
            }
            else {
                c.setForeground(UIManager.getColor("ComboBox.disabledForeground"));
                c.setBackground(UIManager.getColor("ComboBox.disabledBackground"));
                /*****
                c.setForeground(DefaultLookup.getColor(
                         comboBox, this, "ComboBox.disabledForeground", null));
                c.setBackground(DefaultLookup.getColor(
                         comboBox, this, "ComboBox.disabledBackground", null));
                *****/
            }
        }
        boolean shouldValidate = false;
        if (c instanceof JPanel)  {
            shouldValidate = true;
        }

        int x = bounds.x, y = bounds.y, w = bounds.width, h = bounds.height;
        if (inset != null) {
            x = bounds.x + inset.left;
            y = bounds.y + inset.top;
            w = bounds.width - (inset.left + inset.right);
            h = bounds.height - (inset.top + inset.bottom);
        }

        currentValuePane.paintComponent(g,c,comboBox,x,y,w,h,shouldValidate);
    }


    protected void setButtonBorder() {
        if (JTattooUtilities.isLeftToRight(comboBox)) {
            Border border = BorderFactory.createMatteBorder(0, 1, 0, 0, AbstractLookAndFeel.getFrameColor());
            arrowButton.setBorder(border);
        } else {
            Border border = BorderFactory.createMatteBorder(0, 0, 0, 1, AbstractLookAndFeel.getFrameColor());
            arrowButton.setBorder(border);
        }
    }

    public class PropertyChangeHandler implements PropertyChangeListener {

        public void propertyChange(PropertyChangeEvent e) {
            String name = e.getPropertyName();
            if (name.equals("componentOrientation")) {
                setButtonBorder();
            }
        }
    }
//-----------------------------------------------------------------------------    

    public static class ArrowButton extends NoFocusButton {

        public void paint(Graphics g) {
            Dimension size = getSize();
            if (isEnabled()) {
                if (getModel().isArmed() && getModel().isPressed()) {
                    JTattooUtilities.fillHorGradient(g, AbstractLookAndFeel.getTheme().getPressedColors(), 0, 0, size.width, size.height);
                } else if (getModel().isRollover()) {
                    JTattooUtilities.fillHorGradient(g, AbstractLookAndFeel.getTheme().getRolloverColors(), 0, 0, size.width, size.height);
                } else {
                    JTattooUtilities.fillHorGradient(g, AbstractLookAndFeel.getTheme().getButtonColors(), 0, 0, size.width, size.height);
                }
            } else {
                JTattooUtilities.fillHorGradient(g, AbstractLookAndFeel.getTheme().getDisabledColors(), 0, 0, size.width, size.height);
            }
            Icon icon = BaseIcons.getComboBoxIcon();
            int x = (size.width - icon.getIconWidth()) / 2;
            int y = (size.height - icon.getIconHeight()) / 2;
            if (getModel().isPressed() && getModel().isArmed()) {
                icon.paintIcon(this, g, x + 2, y + 1);
            } else {
                icon.paintIcon(this, g, x + 1, y);
            }
            paintBorder(g);
        }
    }
}
