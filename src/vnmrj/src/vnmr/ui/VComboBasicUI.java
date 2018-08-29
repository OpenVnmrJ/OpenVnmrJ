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
import javax.swing.plaf.basic.*;

// import sun.swing.DefaultLookup;
import vnmr.util.*;

/**
 * <p>Title: VComboBasicUI </p>
 * <p>Description: Displays the label string as the default string if the
 *                 string in the combo box does not match the value, or
 *                 if it's a selmenu then it always shows the label string. </p>
 * <p> </p>
 */

public class VComboBasicUI extends BasicComboBoxUI implements VComboBoxUI
{

    protected String title = null;
    protected ComboBoxTitleIF m_objMenu;
    MouseAdapter ml;

    public VComboBasicUI(ComboBoxTitleIF objMenu)
    {
        super();
        m_objMenu = objMenu;
        ml = new MouseAdapter() {
            public void mouseEntered(MouseEvent evt) {
                m_objMenu.enterPopup();
            }
 
            public void mouseExited(MouseEvent evt) {
                m_objMenu.exitPopup();
            }
        };
    }

    /**
     * Paints the currently selected item.
     */
    public void paintCurrentValue(Graphics g,Rectangle bounds,boolean hasFocus) {
        ListCellRenderer renderer = comboBox.getRenderer();
        Component c;
        if (renderer == null)
            return;
        String strValue = title;
        if (title == null) {
            boolean bDefault = m_objMenu.getDefault();
            if (bDefault)
                strValue = (String)m_objMenu.getDefaultLabel();
            else
                strValue = (String)comboBox.getSelectedItem();
        }

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
        currentValuePane.paintComponent(g,c,comboBox,bounds.x,bounds.y,
                                            bounds.width,bounds.height, shouldValidate);
    }

 
    public void installUI( JComponent c ) {
        super.installUI(c);
        if (listBox != null)
          listBox.addMouseListener(ml);
    }

/*
    public void installComponents()
    {
        super.installComponents();
    }
*/

    protected JButton createArrowButton() {
        JButton button = new BasicArrowButton(BasicArrowButton.SOUTH,
                            UIManager.getColor("ComboBox.buttonBackground"),
                            UIManager.getColor("ComboBox.buttonShadow"),
                            Color.black,
                            UIManager.getColor("ComboBox.buttonHighlight"));
        button.setName("ComboBox.arrowButton");
        return button;
    }

    public void setBgColor(Color bg)
    {
        arrowButton.setBackground(bg);
        listBox.setBackground(bg);
        listBox.setSelectionBackground(UIManager.getColor("ComboBox.selectionBackground"));
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
