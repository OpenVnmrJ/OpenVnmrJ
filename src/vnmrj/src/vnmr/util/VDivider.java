/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.beans.*;



public class VDivider extends JSeparator implements PropertyChangeListener
{
    private JComponent client = null;
    private int locY = 0;
    private int locX = 0;
    private int leftX = 0;
    private int rightX = 100;
    private int topY = 0;
    private int bottomY = 100;
    private int ctrlW = 100;
    private int ctrlH = 100;
    private int orient;
    private Dimension mySize;
    private float locRatio = 0.5f;
    private Canvas moveCanvas = null;


    public VDivider(JComponent p)
    {
	super();
        this.client = p; 
        this.orient = SwingConstants.VERTICAL;
        addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent ev) {
                startResize(ev);
                ev.consume();
            }

            public void mouseReleased(MouseEvent e) {
                e.consume();
                doResize(e);
            }
            public void mouseEntered(MouseEvent evt) {
            }

            public void mouseExited(MouseEvent evt) {
            }
        });

        addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseMoved(MouseEvent e) {
            }

            public void mouseDragged(MouseEvent e) {
                e.consume();
                dragResize(e);
            }
        });

	setBg();
	DisplayOptions.addChangeListener(this);
    }

    private void setBg() {
	setBackground(Util.getSeparatorBg());
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
	setBg();
    }

    @Override
    public void setOrientation(int d) {
        super.setOrientation(d);
        orient = d;
        if (d == SwingConstants.VERTICAL)
            setCursor(Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR));
        else
            setCursor(Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR));
    }

    public int getOrientation() {
        return orient;
    }

    public void setCtrlRange(int x, int y, int x2, int y2) {
        leftX = x;
        rightX = x2;
        topY = y;
        bottomY = y2;
        ctrlW = x2 - x;
        ctrlH = y2 - y;
    }

    public float getLocRatio() {
        return locRatio;
    }

    public void setLocRatio(float r) {
        locRatio = r;
    }

    public void startResize(MouseEvent ev) {
        if (client == null)
           return;
     
        Point locPt = getLocation();
        Rectangle r = getBounds();
        mySize  = getSize();
        locX = locPt.x;
        locY = locPt.y;

        if (moveCanvas == null) {
           moveCanvas = new Canvas() {
                public void paint(Graphics g) {
                    g.setColor(Color.red);
                    if (orient == SwingConstants.VERTICAL) { 
                        g.fillRect(0, 0, 4, mySize.height - 1);
                    } else {
                        g.fillRect(0, 0, mySize.width - 1, 4);
                    }
                }
            };
            if (orient == SwingConstants.VERTICAL)
                moveCanvas.setCursor(Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR));
            else
                moveCanvas.setCursor(Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR));
            client.add(moveCanvas, 0);
        }
        if (client.getComponentZOrder(moveCanvas) > 0)
            client.setComponentZOrder(moveCanvas, 0);
        moveCanvas.setBounds(r);
        moveCanvas.setVisible(true);
	setBackground(Color.green);
    }



    private void doResize(MouseEvent ev) {
        if (client == null)
           return;
        int x = ev.getX() + locX;
        int y = ev.getY() + locY;

        if (orient == SwingConstants.VERTICAL) { // move horizontally
            x = x - leftX;
            if (x < 0)
               x = 0;
            else if (x > (ctrlW - 4))
               x = ctrlW - 4;
            locRatio = (float) x / (float) (ctrlW);
        }
        else {
            y = y - topY;
            if (y < 0)
               y = 0;
            else if (y > (ctrlH - 4))
               y = ctrlH - 4;
            locRatio = (float) y / (float) (ctrlH);
        }
        if (locRatio < 0.0f)
            locRatio = 0.0f;
        if (locRatio > 1.0f)
            locRatio = 1.0f;
          
        if (moveCanvas != null) {
           moveCanvas.setVisible(false);
        }
	setBg();
        client.revalidate();
    }

    public void dragResize(MouseEvent ev) {
        if (moveCanvas == null)
            return;
        int x = ev.getX() + locX;
        int y = ev.getY() + locY;

        if (orient == SwingConstants.VERTICAL) { // move horizontally
            if (x < leftX || x >= rightX)
               return;
            moveCanvas.setLocation(x, locY);
        }
        else {
            if (y < topY || y > bottomY)
               return;
            moveCanvas.setLocation(locX, y);
        }
    }
}
