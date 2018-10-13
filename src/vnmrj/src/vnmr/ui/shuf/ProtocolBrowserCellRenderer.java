/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.awt.*;
import javax.swing.*;
import javax.swing.table.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

/**
 * Extend DefaultTableCellRenderer to display protocol browser result cells.
 *
 */
public class ProtocolBrowserCellRenderer extends DefaultTableCellRenderer {
    /** icon to be displayed in all cases */
    private TextImageIcon icon;

    /**
     * constructor
     * @param comp component
     */
    public ProtocolBrowserCellRenderer(JComponent comp) {
        super();
        icon = new TextImageIcon(comp);
        setIcon(icon);
    } // ShufCellRenderer()

    /**
     * Based on value, set the appearance of the label.
     * @param value value to be represented
     */
    protected void setValue(Object value) {
        if (value == null)
            return;

        icon.clear();

        if (value instanceof SearchResults) {
            Object content = ((SearchResults)value).content;
            if (content instanceof String) {
                String str = (String)content;
                icon.setLine1Text(str);
                setToolTipText(str);
            }
        }
    } // setValue()


    public Component getTableCellRendererComponent(JTable table, Object value,
                          boolean isSelected, boolean hasFocus, int row, int column) {
        Component result =
            super.getTableCellRendererComponent(table, value, isSelected,
                                                hasFocus, row, column);
// Specifying colors causes a problem when the theme is set to a DARK one
        // if (!isSelected && value instanceof SearchResults) {
        //     if (((SearchResults)value).match == 1) {
        //         setBackground(Color.white);
        //     } 
        //     else if (((SearchResults)value).match == 0) {
        //         setBackground(Color.lightGray);
        //     }
        //     else if (((SearchResults)value).match == 2) {
        //         setBackground(Color.green);
        //     }
        //     else if (((SearchResults)value).match == 3) {
        //         setBackground(Color.red);
        //     }
        //}
        return result;
    } // getTableCellRendererComponent()

} // class ShufCellRenderer
