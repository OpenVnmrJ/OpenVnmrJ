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
import javax.swing.border.*;

/**
 * robot view and label
 *
 * @author Mark Cao
 */
public class RobotViewComp extends JPanel {
    /**
     * constructor
     */
    public RobotViewComp() {
	setBackground(Color.white);

	setBorder(BorderFactory.createBevelBorder(BevelBorder.LOWERED));
	setLayout(new BorderLayout());
	add(new JLabel("robot view"), BorderLayout.CENTER);
	add(new JLabel("robot id"), BorderLayout.SOUTH);
    } // RobotViewComp()

} // class RobotViewComp
