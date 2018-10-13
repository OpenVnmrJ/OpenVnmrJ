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
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;

import  vnmr.util.*;

/**
 * experiment control button
 *
 * @author Mark Cao
 */
public class ExpControlButton extends JButton {
    /**
     * constructor
     */
    public ExpControlButton() {
	setBackground(Color.cyan);
	Border outsideBorder =
	    BorderFactory.createLineBorder(Global.BGCOLOR, 1);
	Border insideBorder =
	    BorderFactory.createBevelBorder(BevelBorder.LOWERED);
	setBorder(BorderFactory.
		  createCompoundBorder(outsideBorder, insideBorder));
	setFont(new Font("Dialog", Font.PLAIN, 20));
	showGo();

	// install behavior
	addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		Util.log("experiment control button pressed (" +
			 getText() + " state)");
	    }
	});
    } // ExpControlButton()

    /**
     * make the button a "go" button
     */
    public void showGo() {
	setText("Go");
    } // showGo()

    /**
     * make the button a "cancel" button
     */
    public void showCancel() {
	setText("Cancel");
    } // showCancel()

    /**
     * make the button a "remove" button
     */
    public void showRemove() {
	setText("Remove");
    } // showRemove()

} // class ExpControlButton
