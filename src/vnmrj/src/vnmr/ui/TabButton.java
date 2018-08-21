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
import java.beans.*;
import javax.swing.border.*;

import vnmr.util.*;

/**
 * tab button
 */
public class TabButton extends JButton {
    boolean isActive = false;
    private VTabBorder border;
    private Color bgColor;
    /**
     * constructor
     */

    public TabButton() {
	this("");
    } // TabButton()

    /**
     * constructor
     */
    public TabButton(String text) {
        super(text);
        border = new VTabBorder();
        setBorder(border);
        bgColor = Util.getBgColor();
        setBackground(bgColor);
        DisplayOptions.addChangeListener(new PropertyChangeListener()
        {
            public void propertyChange(PropertyChangeEvent e)
            {
                String strProperty = e.getPropertyName();
                if (strProperty == null)
                    return;
                strProperty = strProperty.toLowerCase();
                if (strProperty.indexOf("vjbackground") >= 0)
                {
                    Color colorbg = Util.getBgColor();
                    setBackground(colorbg);
                    if (isActive())
                        setBackground(colorbg.darker());
                }
            }
        });

    } // TabButton()

    public void setActive(boolean a) {
	isActive = a;
    }

    public boolean isRequestFocusEnabled()
    {
        CommandInput cmdLine = Util.getAppIF().commandArea;
        if (cmdLine != null && cmdLine.isShowing())
        {
            Util.getAppIF().setInputFocus();
            return false;
        }
        return super.isRequestFocusEnabled();
    }

    public Insets getBorderInsets() {
	 return border.getBorderInsets();
    }

    public void setBorderInsets(Insets insets) {
	border.setBorderInsets(insets);
    }

    public boolean isActive() {
	return isActive;
    }
} // class TabButton
