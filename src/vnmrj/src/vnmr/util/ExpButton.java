/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.awt.event.*;
import java.beans.*;
import javax.swing.*;

import vnmr.ui.*;

public class ExpButton extends JButton implements VTooltipIF, PropertyChangeListener{
    private int  id = 0;
    private String  tip;
    private String  iconName;
    private String  cmdStr;
    private int  iconDir;
    private boolean  hasIcon;
    private boolean  isUsed = true;
    private boolean  bGraphicsCtrl = false;

    public ExpButton(Icon icon, int num) {
	this.hasIcon = false;
	if (icon != null) {
	    setIcon(icon);
	    this.hasIcon = true;
	}
	this.iconName = "";
	this.iconDir = -1;
	setMargin(new Insets(0,0,0,0));
        //setOpaque(false);
	setBackground(Util.getBgColor());
	this.id = num;
	setBorder(new VButtonBorder());

	DisplayOptions.addChangeListener(this);
    }

    public ExpButton(int num) {
	this (null, num);
    }

    public void setIconData(String str) {
	if (hasIcon) {
	   if (str.equals(iconName))
		return;
	}
	setIcon(Util.getVnmrImageIcon(str));
	iconName = str;
	hasIcon = true;
    }

    public void setIconData(String name, int dir) {
	if (hasIcon) {
	   if (name.equals(iconName) && (dir == iconDir))
		return;
	}
	setIcon(Util.getButtonIcon(name, dir));
	iconName = name;
	iconDir = dir;
	hasIcon = true;
    }

    public void setNeeded(boolean b) {
	isUsed = b;
    }

    public boolean isNeeded() {
	return isUsed;
    }

    public void setCommand(String cmd) {
        cmdStr = cmd;
    }

    public String getCommand() {
        return cmdStr;
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
	setBackground(Util.getBgColor());
    }

    public void setTooltip(String str) {
	tip = Util.getTooltipString(str);
	if ( !Util.isNativeGraphics()) {
		setToolTipText(tip);
	}

	// if there is a key accelerator for this button,
	// then append the key string to the tool tip.
	// if (ExperimentIF.isKeyBinded(str))
	//     tip += "    " + ExperimentIF.getKeyBinded(str);

        VnmrjIF vjIf = Util.getVjIF();
        if (vjIf != null) {
            if (vjIf.isKeyBinded(str))
                 tip += "    " + vjIf.getKeyBinded(str);
        }
    }

    public String getTooltip() {
	return tip;
    }

    public String getTooltip(MouseEvent e) {
	return tip;
    }

    public void setId(int num) {
	id = num;
    }

    public int getId() {
	return id;
    }

    public void setGraphicsCtrl(boolean b) {
        bGraphicsCtrl = b;
    }

    @Override
    public void revalidate() {
        if (!bGraphicsCtrl)
            super.revalidate();
    }
}

