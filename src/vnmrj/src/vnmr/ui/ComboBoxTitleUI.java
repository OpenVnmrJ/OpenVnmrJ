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
import javax.swing.border.Border;
import javax.swing.plaf.basic.*;

import vnmr.util.*;

/**
 * <p>Title: ComboBoxTitleUI </p>
 * <p>Description: Displays the label string as the default string if the
 *                 string in the combo box does not match the value, or
 *                 if it's a selmenu then it always shows the label string. </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 */

public class ComboBoxTitleUI extends BasicComboBoxUI
{

    protected String m_strLabel;
    protected ComboBoxTitleIF m_objMenu;
    MouseAdapter ml;

    public ComboBoxTitleUI(ComboBoxTitleIF objMenu)
    {
        super();
        m_objMenu = objMenu;
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
            }

            public void mouseReleased(MouseEvent evt) {
            }
 
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
        boolean bDefault = m_objMenu.getDefault();
        String strLabel = m_objMenu.getDefaultLabel();
        if (renderer == null)
            return;
        // if the default string should be shown then show the default label,
        // otherwise show the selected item in the combobox.
        String strValue = bDefault ? strLabel : (String)comboBox.getSelectedItem();

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
            c.setForeground(comboBox.getForeground());
            c.setBackground(comboBox.getBackground());
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

        // Fix for 4238829: should lay out the JPanel.
        boolean shouldValidate = false;
        if (c instanceof JPanel)  {
            shouldValidate = true;
        }
        currentValuePane.paintComponent(g,c,comboBox,bounds.x,bounds.y,
                                            bounds.width,bounds.height, shouldValidate);
    }

/*
    public void installComponents()
    {
        super.installComponents();
        arrowButton.setOpaque(true);
        arrowButton.setBackground(Util.getBgColor());
        arrowButton.setBorder(new ArrowButtonBorder());
    }
*/

    public void setbackground(Color colorbg)
    {
        arrowButton.setBackground(colorbg);
        listBox.setBackground(colorbg);
    }

    /**
     * Uses our special "MyLayoutManager".
     */
    public LayoutManager createLayoutManager() {
        return new MyLayoutManager();
    }

    public void installUI( JComponent c ) {
        super.installUI(c);
        if (listBox != null)
          listBox.addMouseListener(ml);
    }

    /**
     * Special LayoutManager to position the ArrowButton where we want it.
     */
    class MyLayoutManager extends BasicComboBoxUI.ComboBoxLayoutManager {

        public void layoutContainer(Container parent) {
            super.layoutContainer(parent);
            // Put ArrowButton at right end of ComboBox, within the border.
            // Border is assumed to be 2 pixels all around!
            // For some reason, the comboBox insets have 0 pixels at the top,
            // so we ignore that.
            Rectangle comboBounds = comboBox.getBounds();
            int y = 2;
            int height = comboBounds.height - 4;
            int x = comboBounds.width - comboBounds.height;
            int width = comboBounds.height - 2;
            arrowButton.setBounds(new Rectangle(x, y, width, height));
        }
    }


    /**
     * A border for the ArrowButton that is merged into the ComboBar.
     */
    class ArrowButtonBorder implements Border {

        // Nobody seems to care what these insets are.
        private Insets mm_insets = new Insets(2, 2, 2, 2);

        public void paintBorder(Component c, Graphics g,
                                int x, int y, int width, int height) {
            // Erase stray stuff around edges
            g.setColor(Util.getBgColor());
            g.drawLine(x, y, x + width - 1, y);
            g.drawLine(x + width - 1, y, x + width - 1, y + height - 1);
            g.drawLine(x + width - 1, y + height - 1, x, y + height - 1);

            // Draw etched border at left edge
            g.setColor(Util.getBgColor().darker());
            g.drawLine(x, y, x, y + height - 1);
            g.setColor(Util.getBgColor().brighter());
            g.drawLine(x + 1, y, x + 1, y + height - 1);
        }

        public Insets getBorderInsets(Component c) {
            return mm_insets;
        }

        public boolean isBorderOpaque() {
            return false;
        }
    }

    /*protected Dimension getDisplaySize()
    {
        Dimension result = super.getDisplaySize();
        if (m_bDefault)
        {
            Component cpn = renderer.getListCellRendererComponent(listBox, "four",
                                                                        -1, false, false);
            currentValuePane.add(cpn);
            cpn.setFont(comboBox.getFont());
            Dimension d = cpn.getPreferredSize();
            currentValuePane.remove(cpn);
            result.height = Math.max(result.height,d.height);
            result.width = Math.max(result.width,d.width);
        }

        return result;
    }*/
}
