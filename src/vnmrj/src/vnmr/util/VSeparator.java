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



public class VSeparator extends JComponent
{
    public static final int HORIZONTAL = 1;
    public static final int VERTICAL = 2;
    public static final int SOUTH  = 1;
    public static final int NORTH  = 2;
    public static final int WEST  = 3;
    public static final int EAST  = 4;

    private int orientation = HORIZONTAL;
    private int attachement = SOUTH;
    private Graphics g;
    private int iMx, iMy, iMx2, iMy2;
    private int iRx, iRy, iRx2, iRy2;
    private int iSize;
    private int iCenterX, iCenterY;
    private int iMoveIndex;
    private boolean bResize;
    private boolean bMove;
    private VSplitListener listener = null;
    private JComponent rParent = null;
    private Rectangle rBound;
    private Rectangle rSelect;
    private int xPoints[], yPoints[];

    public VSeparator(JComponent parent)
    {
	super();
	this.rParent = parent;
	setOpaque(false);
	this.iSize = 0;
	addMouseListener(new MouseAdapter() {
	    public void mousePressed(MouseEvent evt) {
		int iMf = evt.getModifiers();
		bResize = false;
		bMove = false;
		if ((iMf & InputEvent.BUTTON1_MASK) != 0) {
			startResize(evt);
		}
		else if ((iMf & InputEvent.BUTTON2_MASK) != 0) {
			startMove(evt);
		}
	    }

	    public void mouseReleased(MouseEvent evt) {
		resetCursor();
		stopResize(evt);
	    }

	    public void mouseEntered(MouseEvent evt) {
		setResizeCursor();
	    }

            
	    public void mouseExited(MouseEvent evt) {
		if (!bResize && !bMove)
		    resetCursor();
	    }
	});

	addMouseMotionListener(new MouseMotionAdapter() {
	    public void mouseDragged(MouseEvent e) {
		resizeProc(e, false);
	    }
	});
    }

    public void setAttachement(int k) {
	attachement = k;
	if (attachement == SOUTH || attachement == NORTH)
	    orientation = HORIZONTAL;
	else
	    orientation = VERTICAL;
    }

    public int getAttachement() {
	return attachement;
    }

    public int getOrientation() {
        return orientation;
    }

    public void setOrientation( int k ) {
	orientation = k;
    }

    public void setListener(VSplitListener l) {
	listener = l;
    }

    public void setRegion(int x, int y, int x2, int y2 ) {
	iRx = x;
	iRx2 = x2;
	iRy = y;
	iRy2 = y2;
    }


    public void setResizeCursor() {
	if (bMove || bResize)
	    return;
	Cursor cursor = null;

	switch (attachement) {
	    case NORTH:
		    cursor = Cursor.getPredefinedCursor(Cursor.S_RESIZE_CURSOR);
		    break;
	    case EAST:
		    cursor = Cursor.getPredefinedCursor(Cursor.W_RESIZE_CURSOR);
		    break;
	    case WEST:
		    cursor = Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR);
		    break;
	    default:
		    cursor = Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR);
		    break;
	}
	getParent().setCursor(cursor);
    }

    public void setMoveCursor() {
	getParent().setCursor(Cursor.getPredefinedCursor(Cursor.CROSSHAIR_CURSOR));
    }

    public void resetCursor() {
	getParent().setCursor(null);
    }

    private void hilitArea(int k) {
	int  x, y, w, h;

	g.setXORMode(Color.yellow);
	if (rSelect != null) {
	    g.fillPolygon(xPoints, yPoints, 3);
	    g.drawRect(rSelect.x,rSelect.y,rSelect.width - 1,rSelect.height - 1);
	    rSelect = null;
	}
	switch (k) {
	    case NORTH:
		      xPoints[0] = iCenterX - 5;
		      yPoints[0] = iCenterY - 10;
		      xPoints[1] = iCenterX;
		      yPoints[1] = iCenterY - 20;
		      xPoints[2] = iCenterX + 5;
		      yPoints[2] = iCenterY - 10;
		      x = iRx;
                      y = iRy;
                      h = (iRy2 - iRy) * iSize / 100;
                      w = iRx2 - iRx;
		      break;
	    case SOUTH:
		      xPoints[0] = iCenterX - 5;
		      yPoints[0] = iCenterY + 10;
		      xPoints[1] = iCenterX + 5;
		      yPoints[1] = iCenterY + 10;
		      xPoints[2] = iCenterX;
		      yPoints[2] = iCenterY + 20;
		      x = iRx;
                      h = (iRy2 - iRy) * iSize / 100;
                      w = iRx2 - iRx;
                      y = iRy2 - h;
		      break;
	    case EAST:
		      xPoints[0] = iCenterX + 10;
		      yPoints[0] = iCenterY - 5;
		      xPoints[1] = iCenterX + 20;
		      yPoints[1] = iCenterY;
		      xPoints[2] = iCenterX + 10;
		      yPoints[2] = iCenterY + 5;
		      y = iRy;
                      w = (iRx2 - iRx) * iSize / 100;
                      h = iRy2 - iRy;
                      x = iRx2 - w;
		      break;
	    case WEST:
		      xPoints[0] = iCenterX - 20;
		      yPoints[0] = iCenterY;
		      xPoints[1] = iCenterX - 10;
		      yPoints[1] = iCenterY - 5;
		      xPoints[2] = iCenterX - 10;
		      yPoints[2] = iCenterY + 5;
		      x = iRx;
                      y = iRy;
                      w = (iRx2 - iRx) * iSize / 100;
                      h = iRy2 - iRy;
		      break;
	    default:
		      return;
	}
	g.fillPolygon(xPoints, yPoints, 3);
	rSelect = new Rectangle(x, y, w, h);
	g.drawRect(x, y, w - 1, h - 1);
    }

    public void drawMark() {
	g.setXORMode(Color.yellow);
	g.drawLine(iCenterX, iCenterY - 10, iCenterX, iCenterY + 10);
	g.drawLine(iCenterX - 10, iCenterY, iCenterX + 10, iCenterY);
	g.setXORMode(Color.white);
	int x = iCenterX - 20;
	int y = iCenterY;
	// draw west triangle
	g.drawLine(x, y, x + 10, y + 5);
	g.drawLine(x, y, x + 10, y - 5);
	g.drawLine(x + 10 , y - 5, x + 10, y + 5);
	// draw north triangle
	y = iCenterY - 10;
	x = iCenterX - 5;
	g.drawLine(x, y, x + 10, y);
	g.drawLine(x, y, x + 5, y - 10);
	g.drawLine(x + 5, y - 10, x + 10, y);
	// draw east triangle
	x = iCenterX + 10;
	y = iCenterY - 5;
	g.drawLine(x, y, x, y + 10);
	g.drawLine(x, y, x + 10, y + 5);
	g.drawLine(x, y + 10, x + 10, y + 5);
	// draw south triangle
	x = iCenterX - 5;
	y = iCenterY + 10;
	g.drawLine(x, y, x + 10, y);
	g.drawLine(x, y, x + 5, y + 10);
	g.drawLine(x + 5, y + 10, x + 10, y);
    }

    public void startMove(MouseEvent e) {
	if (rParent == null)
	    return;
	rSelect = null;
	setMoveCursor();
        rBound = getBounds();
	g = rParent.getGraphics();
	iCenterX = (iRx2 - iRx) / 2;
	iCenterY = (iRy2 - iRy) / 2;
	if (xPoints == null) {
	    xPoints = new int[3];
	    yPoints = new int[3];
	}
	bMove = true;
	drawMark();
	hilitArea(attachement);
	iMoveIndex = attachement;
    }

    public void startResize(MouseEvent e) {
	if (rParent == null)
	    return;
	g = rParent.getGraphics();
	g.setXORMode(Color.yellow);
        rBound = getBounds();
	if (orientation == HORIZONTAL) {
	    iMx = iRx;
	    iMx2 = iRx2;
	    iMy = e.getY() + rBound.y;
	    iMy2 = iMy;
	}
	else {
	    iMx = e.getX() + rBound.x;
	    iMx2 = iMx;
	    iMy = iRy;
	    iMy2 = iRy2;
	}
	g.drawLine(iMx, iMy, iMx2, iMy2);
	bResize = true;
    }

    public int getResize() {
	int w, ratio, d;

        rBound = getBounds();
	switch (attachement) {
	    case NORTH:
		    w = iRy2 - iRy;
		    d = rBound.y;
		    ratio = d / w;
		    break;
	    case SOUTH:
		    w = iRy2 - iRy;
		    d = rBound.y;
		    ratio = (w - d) / w;
		    break;
	    case EAST:
		    w = iRx2 - iRx;
		    d = rBound.x;
		    break;
	    default: 
		    w = iRx2 - iRx;
		    d = rBound.x;
		    break;
	}
	ratio = 100 * d / w;
	if (ratio > 100)
	    ratio = 100;
	if (ratio < 0)
	    ratio = 0;
	return ratio;
    }

    public void doResize() {
	int w, d;

	switch (attachement) {
	    case NORTH:
		    w = iRy2 - iRy;
		    d = iMy - iRy;
		    break;
	    case SOUTH:
		    w = iRy2 - iRy;
		    d = iRy2 - iMy;
		    break;
	    case EAST:
		    w = iRx2 - iRx;
		    d = iRx2 - iMx;
		    break;
	    default: 
		    w = iRx2 - iRx;
		    d = iMx - iRx;
		    break;
	}
	iSize = 100 * d / w;
	if (iSize < 0)
	    iSize = 0;
	if (iSize > 100)
	    iSize = 100;
	if (listener != null)
	    listener.setSplitSize(this, iSize);
    }

    public void stopResize(MouseEvent e) {
	if (bMove) {
	    moveProc(e, true);
	}
	else if (bResize) {
	    resizeProc(e, true);
	}
	iMx = -1;
	bResize = false;
	bMove = false;
	if (g != null) {
	    g.dispose();
	    g = null;
	}
    }

    public void moveProc(MouseEvent e, boolean relize) {
	    int x = e.getX() + rBound.x;
	    int y = e.getY() + rBound.y;
	    int newIndex = iMoveIndex;

	    double x1 = (double) (x - iCenterX);
	    double y1 = (double) (iCenterY - y);
	    double d = Math.sqrt(x1 * x1 + y1 * y1);
	    double rad = Math.asin(y1 / d);
	    rad = Math.toDegrees(rad);
	    if (rad >= 0.0) {
		if (rad >= 45.0)
		    newIndex = NORTH;
		else {
		     if (x1 >= 0.0)
			newIndex = EAST;
		     else
			newIndex = WEST;
		}
	    }
	    else {
		if (rad < -45.0)
		    newIndex = SOUTH;
		else {
		     if (x1 >= 0.0)
			newIndex = EAST;
		     else
			newIndex = WEST;
		}
	    }

	    if (iMoveIndex != newIndex) {
		iMoveIndex = newIndex;
		hilitArea(newIndex);
	    }
	    if (relize) {
		hilitArea(0);
		drawMark(); // clear window
		if (attachement != iMoveIndex) {
		   if (listener != null)
	    	       listener.setSplitPos(this, iMoveIndex);
		}
	    }
    }

    public void resizeProc(MouseEvent e, boolean relize) {
	if (bMove) {
	    moveProc(e, relize);
	    return;
	}
	if (!bResize)
	    return;
	if (iMx >= 0)
	    g.drawLine(iMx, iMy, iMx2, iMy2);
	if (orientation == HORIZONTAL) {
	    iMy = e.getY() + rBound.y;
	    if (iMy < iRy)
		iMy = iRy;
	    else if (iMy > iRy2)
		iMy = iRy2;
	    iMy2 = iMy;
	}
	else {
	    iMx = e.getX() + rBound.x;
	    if (iMx < iRx)
		iMx = iRx;
	    else if (iMx > iRx2)
		iMx = iRx2;
	    iMx2 = iMx;
	}
	if (relize) {
	    doResize();
	    return;
	}
	g.drawLine(iMx, iMy, iMx2, iMy2);
    }

    public void setRatio(int k) {
	iSize = k;
    }

    public int getRatio() {
	return iSize;
    }

    public void paint(Graphics g) {
    }
}
