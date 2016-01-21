/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.plaf.*;
import javax.swing.plaf.metal.*;

import vnmr.util.*;

/**
 * <p>Title: VComboMetalUI </p>
 * <p>Description: Displays the label string as the default string if the
 *                 string in the combo box does not match the value, or
 *                 if it's a selmenu then it always shows the label string. </p>
 * <p> </p>
 */

public class VComboMetalUI extends MetalComboBoxUI implements VComboBoxUI
{

    protected String title;
    protected ComboBoxTitleIF vjMenu;
    private MouseAdapter ml;
    private String vjTitle = null;
    private Insets inset;

    public static ComponentUI createUI(JComponent c) {
        return new VComboMetalUI();
    }

    public VComboMetalUI() {
        super();
        createMouseAdapter();
    }

    public VComboMetalUI(ComboBoxTitleIF objMenu)
    {
        super();
        vjMenu = objMenu;
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

    /**
     * Paints the currently selected item.
     */
    public void paintCurrentValue(Graphics g,Rectangle bounds,boolean hasFocus) {
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

      //  if (MetalLookAndFeel.usingOcean()) {
            bounds.x += 2;
            bounds.width -= 3;
            if (arrowButton != null) {
                Insets buttonInsets = arrowButton.getInsets();
                bounds.y += buttonInsets.top;
                bounds.height -= (buttonInsets.top + buttonInsets.bottom);
            }
            else {
                bounds.y += 2;
                bounds.height -= 4;
            }
       // }

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


    public void installUI( JComponent c ) {
        vjMenu = null;
        super.installUI(c);
        if (c instanceof ComboBoxTitleIF) {
            vjMenu = (ComboBoxTitleIF) c;
            if (listBox != null) {
               if (ml == null)
                   createMouseAdapter();
               listBox.addMouseListener(ml);
               vjMenu.listBoxMouseAdded();
            }
        }
        inset = UIManager.getInsets("ComboBox.padding");
    }

    public void installComponents()
    {
        super.installComponents();
        arrowButton.setOpaque(true);
        arrowButton.setBackground(Util.getBgColor());
    }

    public void setBgColor(Color bg)
    {
        arrowButton.setBackground(bg);
        listBox.setBackground(bg);
    }

    public void setFgColor(Color fg)
    {
        arrowButton.setForeground(fg);
        listBox.setForeground(fg);
    }

    public void setTitle(String s) {
        if (s == null || s.length() < 1)
            title = null;
        else
            title = s;
    }
}
