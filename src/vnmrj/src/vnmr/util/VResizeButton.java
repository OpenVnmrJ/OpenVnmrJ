/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class VResizeButton extends JButton {

    public static final int UP = 1;
    public static final int DOWN = 2;
    public static final int RECT = 3;
    public static final int LEFT = 4;
    public static final int RIGHT = 5;
    public static final int ROUND = 6;


    private int type, orgType;
    private boolean  isDown;
    private boolean  isPressed;
    private boolean  transparent;

    public VResizeButton(int typ) {
	this.type = typ;
	this.orgType = typ;
	this.isDown = false;
	this.isPressed = false;
	this.transparent = false;
	setOpaque(false);
	setBackground(Global.BGCOLOR);
	setBorder(BorderFactory.createEmptyBorder());
	setPreferredSize(new Dimension(10, 10));
    } // VResizeButton

    public VResizeButton(int typ, boolean s ) {
	this(typ);
	this.transparent = s;
    }


    public void setType(int s) {
	 type = s;
    }

    public void setPressed(boolean s) {
	isPressed = s;
    }

    public void setSelected(boolean b) {
	 isDown = b;
	 if (b) {
	    switch (orgType) {
		case UP:
			type = DOWN;
			break;
		case DOWN:
			type = UP;
			break;
		case LEFT:
			type = RIGHT;
			break;
		case RIGHT:
			type = LEFT;
			break;
	    }
	 }
	 else {
	    type = orgType;
	 }
    	 super.setSelected(b);
    }

    public void setShow(boolean s) {
	if (s)
	   type = orgType;
	else
	   type = RECT;
	getParent().validate();
    }

    public void paint(Graphics g) {
	boolean pressed = getModel().isPressed() && getModel().isArmed();
        Dimension  psize = getSize();
        int y = psize.height / 2;
        if (y < 0) return;
        int x = psize.width / 2;
        int x1 = 0;
        int x2 = 0;
        int y1 = 0;
        int y2 = 0;
        int w;
	if (!transparent) {
	    g.setColor(Global.BGCOLOR);
	    g.fillRect(0, 0, psize.width, psize.height);
	}
	if (psize.width < psize.height)
	    w = psize.width;
	else
	    w = psize.height;
	if (w > 7)
	    w = 7;
	if (type == UP || type == DOWN || type == RECT) {
	    x1 = x - w / 2;
	    if (x1 < 0)
		x1 = 0;
	    x2 = x + w / 2;
	    if (x2 < 2)
		return;
            y = psize.height - w - 1;
	    if (y < 0)
		y = 0;
	    y1 =  y + w;
	}
	if (type == UP) {
	    if (pressed)
               g.setColor(Global.BGCOLOR.darker());
	    else
               g.setColor(Global.BGCOLOR.brighter());
            g.drawLine(x1, y1, x, y);
	    if (pressed)
               g.setColor(Global.BGCOLOR.brighter());
	    else
               g.setColor(Global.BGCOLOR.darker());
            g.drawLine(x1+1, y1, x2+1, y1);
            g.drawLine(x+1, y, x2+1, y1);
	}
	if (type == DOWN) {
	    if (pressed)
               g.setColor(Global.BGCOLOR.darker());
	    else
               g.setColor(Global.BGCOLOR.brighter());
            g.drawLine(x1, y, x2, y);
            g.drawLine(x1, y, x, y1);
	    if (pressed)
               g.setColor(Global.BGCOLOR.brighter());
	    else
               g.setColor(Global.BGCOLOR.darker());
            g.drawLine(x+1, y1, x2+1, y);
	}
	if (type == RECT) {
	    if (pressed || isDown)
               g.setColor(Global.BGCOLOR.darker());
	    else
               g.setColor(Global.BGCOLOR.brighter());
            g.drawLine(x1, y, x2, y);
            g.drawLine(x1, y, x1, y1);
	    if (pressed || isDown) {
               g.setColor(Global.BGCOLOR.brighter());
	    }
	    else {
	       x1++;
	       x2++;
               g.setColor(Global.BGCOLOR.darker());
	    }
            g.drawLine(x1, y1, x2, y1);
            g.drawLine(x2, y, x2, y1);
	}
	if (type == LEFT) {
	    x1 = 2;
            x2 = x1 + w;
            y = psize.height / 2;
            y1 = y - w / 2;
            y2 = y + w / 2;
	    if (y < 3)
		return;
	    if (x2 >= psize.width)
		x2 = psize.width - 1;
	    if (pressed) {
                g.setColor(Global.BGCOLOR.darker());
	    }
	    else {
               g.setColor(Global.BGCOLOR.brighter());
	    }
            g.drawLine(x1, y, x2, y1);
	    if (pressed) {
               g.setColor(Global.BGCOLOR.brighter());
	    }
	    else
               g.setColor(Global.BGCOLOR.darker());
            g.drawLine(x1+1, y+1, x2, y2+1);
            g.drawLine(x2, y1, x2, y2);
	}
	if (type == RIGHT) {
	    x1 = 3;
            x2 = x1 + w;
            y = psize.height / 2;
            y1 = y - w / 2;
            y2 = y + w / 2;
	    if (y < 3)
		return;
	    if (pressed)
               g.setColor(Global.BGCOLOR.darker());
	    else
               g.setColor(Global.BGCOLOR.brighter());
            g.drawLine(x1, y1, x1, y2);
            g.drawLine(x1, y1, x2, y);
	    if (pressed)
               g.setColor(Global.BGCOLOR.brighter());
	    else
               g.setColor(Global.BGCOLOR.darker());
            g.drawLine(x1, y2, x2, y);
	}
	if (type == ROUND) {
	    x1 = x - 3;
            y1 = psize.height - 8;
            if (y1 < 0)
                y1 = 0;
            if (x1 < 0)
                x1 = 0;
            x = x1;
            y = y1;
	    if (pressed || isPressed)
               g.setColor(Global.BGCOLOR.darker());
            else
               g.setColor(Global.BGCOLOR.brighter());
            g.drawLine(x1+2, y1, x1+4, y1);
            y1++;
            g.drawLine(x1+1, y1, x1+1, y1);
            g.drawLine(x1+5, y1, x1+5, y1);
            y1++;
            g.drawLine(x1, y1, x1, y1+2);
            y1 = y1 + 3;
            g.drawLine(x1+1, y1, x1+1, y1);
            x1 = x;
            y1 = y;
            if (pressed || isPressed)
                g.setColor(Global.BGCOLOR.brighter());
            else {
		g.setColor(Global.BGCOLOR.darker());
		x1 = x+1;
		y1 = y+1;
            }
            y1++;
            g.drawLine(x1+5, y1, x1+5, y1);
            y1++;
            g.drawLine(x1+6, y1, x1+6, y1+2);
            y1 = y1 + 3;
            g.drawLine(x1+5, y1, x1+5, y1);
            y1++;
            g.drawLine(x1+2, y1, x1+4, y1);
	}
    }
}

