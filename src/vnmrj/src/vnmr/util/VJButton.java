/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.beans.*;

/**
 * Title:   VJButton
 * Description: Wrapper class for JButton, used to set background color from DisplayOptions.
 * Copyright:    Copyright (c) 2001
 * Company:
 * @author
 * @version 1.0
 */

public class VJButton extends JButton implements PropertyChangeListener
{
    private int nButtonMask = InputEvent.BUTTON1_MASK;
    private int nModify;
    private boolean    bPaintBorder = false;
    private boolean    bBorderAlways = false;

    public VJButton()
    {
        this(null, null, false);
    }

    public VJButton(Icon icon)
    {
        this(null, icon, false);
    }

    public VJButton(String str)
    {
        this(str, null, false);
    }

    public VJButton(boolean bContentArea)
    {
        this(null, null, bContentArea);
    }

    public VJButton(String str, Icon icon, boolean bContentArea)
    {
        super(str, icon);
        setAttributes(bContentArea);
    }

    public void setAttributes(boolean bContentArea) {
        setBackground(Util.getBgColor());
        DisplayOptions.addChangeListener(this);
        setBorder(new VButtonBorder());
        setOpaque(false);
        setBorderPainted(false);
        setContentAreaFilled(bContentArea);
        addMouseListener(new MouseAdapter() {
            public void mouseEntered(MouseEvent evt) {
                showBorder();
            }
            public void mouseExited(MouseEvent evt) {
                hideBorder();
            }

            public void mousePressed(MouseEvent e) {
		boolean bDo = false;

		nModify = e.getModifiers();
		// Button1 will do the default job
		if ((nModify & InputEvent.BUTTON2_MASK) != 0 &&
		     (nButtonMask & InputEvent.BUTTON2_MASK) != 0) {
		     bDo = true;
		}
		if ((nModify & InputEvent.BUTTON3_MASK) != 0 &&
		     (nButtonMask & InputEvent.BUTTON3_MASK) != 0) {
		     bDo = true;
		}

		if (bDo) {
          	   AbstractButton b = (AbstractButton) e.getSource();
          	   if(b.contains(e.getX(), e.getY())) {
         		ButtonModel model = b.getModel();
             		if (!model.isEnabled()) {
                	    return;
             		}
             		if (!model.isArmed()) {
                	    model.setArmed(true);
             		}
             		model.setPressed(true);
            	    }
		}
	    }

            public void mouseReleased(MouseEvent e) {
                AbstractButton b = (AbstractButton) e.getSource();
                ButtonModel model = b.getModel();
                if (model.isPressed())
                        model.setPressed(false);
            }
        });
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        setBackground(Util.getBgColor());
    }

    public void showBorder() {
	bPaintBorder = true;
	repaint();
    }

    public void hideBorder() {
        if (bBorderAlways)
            return;
	bPaintBorder = false;
	repaint();
    }

    public void setBorderAlways(boolean b) {
        bBorderAlways = b;
        if (b)
           showBorder();
        else
           hideBorder();
    }

    // override the default method to avoid calling setBorderPainted
    // because setBorderPainted will trigger revalidate
    public boolean isBorderPainted() {
        return bPaintBorder;
    }

    public int getModifiers() {
	return nModify;
    }

    public void addButtonMask(int m) {
	nButtonMask = nButtonMask | m;
    }

    public void removeButtonMask(int m) {
	nButtonMask = nButtonMask & (~m);
    }
}
