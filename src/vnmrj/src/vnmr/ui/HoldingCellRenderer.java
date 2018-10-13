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
import javax.swing.*;
import javax.swing.table.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.util.*;

/**
 * Extend DefaultTableCellRenderer to display cells in holding area.
 *
 * @author Mark Cao
 */
public class HoldingCellRenderer extends DefaultTableCellRenderer {
    /** icon to be displayed in all cases */
    private TextImageIcon icon;
//     /** image icon for locked */
//     private static final ImageIcon lockIcon = Util.getImageIcon("lock.gif");

    /**
     * constructor
     * @param comp component
     */
    public HoldingCellRenderer(JComponent comp) {
	super();
	icon = new TextImageIcon(comp);
	setIcon(icon);
    } // HoldingCellRenderer()

    /**
     * Based on value, set the appearance of the label.
     * @param value value to be represented
     */
    protected void setValue(Object str) {
	icon.clear();

	if (str instanceof String[]) {
	    String[] info = (String[]) str;
	    icon.setLine1Text(info[1]);
	    setToolTipText(info[0] + " = " + info[1]);
	}
    } // setValue()

    public Component getTableCellRendererComponent(JTable table, Object value,
                 boolean isSelected, boolean hasFocus, int row, int column) {

	Component result =
	    super.getTableCellRendererComponent(table, value, isSelected,
						hasFocus, row, column);
	if (!isSelected)
	    setBackground(Color.white);
	
	return result;
    } // getTableCellRendererComponent()

} // class HoldingCellRenderer
