/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import javax.swing.*;
import javax.swing.table.*;

import  vnmr.bo.*;
import  vnmr.ui.*;
import  vnmr.util.*;

/**
 * Extend DefaultTableCellRenderer to display header cells.
 *
 * @author Mark Cao
 */
public class HeaderValRenderer extends DefaultTableCellRenderer {
    /** image icon for locked */
    private static final ImageIcon lockIcon = Util.getImageIcon("lock.gif");
    /** image icon pointing up */
    private static final ImageIcon upIcon = Util.getImageIcon("up11x17.gif");
    /** image icon pointing down */
    private static final ImageIcon downIcon =Util.getImageIcon("down11x17.gif");
    /** image icon showing gray box (for disabled) */
    private static final ImageIcon grayIcon = Util.getImageIcon("boxGray.gif");

    /**
     * get width of lock icon
     * @return width
     */
    public static int getLockWidth() {
	return lockIcon.getIconWidth() + 2;
    } // getLockWidth()

    /**
     * constructor
     * @param comp component
     */
    public HeaderValRenderer(JComponent comp) {
	super();

        // The text and icon will be set later when setValue is called,
        // This is just what shows up before everything gets started.
	setIcon(grayIcon);

        // Put the label in the center of the column
        setHorizontalAlignment(JLabel.CENTER);

        // Put the Text on the left and thus the icon on the right
        setHorizontalTextPosition(JLabel.LEFT);

        // Increase the gap between the text and icon a little
        setIconTextGap(12);

	setToolTipText(Util.getLabel("_Click_to_select_or_sort"));

    } // HeaderValRenderer()

    /**
     * Based on value, set the appearance of the label. 
     * WARNING: Implement this method efficiently to prevent performance
     * problems!
     * @param value value to be represented
     */
    protected void setValue(Object value) {
	if (value == null)
	    return;

	HeaderValue hValue = (HeaderValue)value;


        setText(hValue.text);

        if (hValue.twoLine) {
            if (hValue.keyed) {
                if (hValue.ascending)
                    setIcon(downIcon);
                else
                    setIcon(upIcon);
            } 
            else {
                setIcon(grayIcon);
            }
        }
    } // setValue()

    /** WARNING: Implement this method efficientlyk to prevent performance
     * problems!
     */
    public Component getTableCellRendererComponent(JTable table, Object value,
                          boolean isSelected, boolean hasFocus, int row, int column) {
	if (table != null) {
	    JTableHeader header = table.getTableHeader();
	    if (header != null) {
		setForeground(header.getForeground());
		setBackground(header.getBackground());
		setFont(header.getFont());
	    }
	}

	setValue(value);
	setBorder(BorderFactory.createMatteBorder(0, 0, 1, 1, Color.black));
	return this;
    } // getTableCellRendererComponent()

} // class HeaderValRenderer
