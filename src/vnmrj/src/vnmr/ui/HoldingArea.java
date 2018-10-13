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
import java.awt.dnd.*;
import java.util.*;
import java.beans.*;
import javax.swing.*;
import javax.swing.border.*;

import  vnmr.bo.*;
import  vnmr.util.*;

/**
 * The holding-area panel.
 *
 */
public class HoldingArea extends JPanel implements PropertyChangeListener{
    /** session share */
    private SessionShare sshare;
    /** drop target */
    private JScrollPane sp;
    private DropTarget dropTarget;
    HoldingTable m_holdingTable;
    JViewport m_vp;

    /**
     * constructor
     * @param sshare session share
     */
    public HoldingArea(SessionShare sshare) {
	this.sshare = sshare;

	setOpaque(true);
	setLayout(new BorderLayout());

	sp = new JScrollPane();
	m_holdingTable = new HoldingTable(sshare);
	m_holdingTable.setOpaque(true);
        //holdingTable.setBackground(Global.BGCOLOR);
	sp.setViewportView(m_holdingTable);
	m_vp = sp.getViewport();
	//vp.setBackground(Global.BGCOLOR);

/*
	Border outsideBorder =
	    BorderFactory.createMatteBorder(10, 1, 1, 1, Global.BGCOLOR);
	Border insideBorder =
	    BorderFactory.createBevelBorder(BevelBorder.LOWERED);
	sp.setBorder(BorderFactory.createCompoundBorder(outsideBorder,
						     insideBorder));
*/
	sp.setBorder(BorderFactory.createBevelBorder(BevelBorder.LOWERED));

	add(sp, BorderLayout.CENTER);
	// make the holding area a drop target
	dropTarget =
	    new DropTarget(sp.getViewport(),
			   new HoldingDropTargetListener(sshare));
	DisplayOptions.addChangeListener(this);

        Util.setHoldingArea(this);

    } // HoldingArea()

    public void propertyChange(PropertyChangeEvent evt)
    {
    }

} // class HoldingArea
