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
import javax.swing.*;


public class ResizableContainer extends JComponent {
    protected int borderWidth = 5;
    protected int cornerWidth = 12;
    private int  pressX, pressY;
    private int  pressDX, pressDY;
    private int  parentW, parentH;
    private int  curCursor = Cursor.DEFAULT_CURSOR;
    private boolean bMousePressed = false;
    private Color focusColor = DisplayOptions.getColor("Focused");

    public ResizableContainer() {
        super();
        initEventListeners();
    }

    private void  initEventListeners() {
        addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                bMousePressed = false;
            }

            public void mousePressed(MouseEvent evt) {
                int modifier = evt.getModifiersEx();
                if ((modifier & InputEvent.BUTTON1_DOWN_MASK) != 0) {
                    processMousePress(evt, true); 
                }
            }

            public void mouseReleased(MouseEvent evt) {
                if (bMousePressed)
                    processMousePress(evt, false); 
            }

            public void mouseEntered(MouseEvent evt) {
                processMouseMove(evt, true); 
            }

            public void mouseExited(MouseEvent evt) {
                processMouseMove(evt, false); 
            }
        }); 

        addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent evt) {
                processMouseDrag(evt); 
            }

            public void mouseMoved(MouseEvent evt) {
                processMouseMove(evt, true); 
            }
        }); 
    }

    private void processMousePress(MouseEvent ev, boolean bPressed) {
        bMousePressed = bPressed;
        if (bPressed) {
            Dimension dim = getSize();
            pressX = ev.getX();
            pressY = ev.getY();
            pressDX = dim.width - pressX;
            pressDY = dim.height - pressY;
            Container p = getParent();
            if (p != null) {
                Dimension dim2 = p.getSize();
                parentW = dim2.width;
                parentH = dim2.height;
            }
            if (parentW < dim.width)
                parentW = dim.width * 2;
            if (parentH < dim.height)
                parentH = dim.height * 2;
            focusColor = DisplayOptions.getColor("Focused");
        }
        repaint();
    }

    private void processMouseMove(MouseEvent ev, boolean bInside) {
         if (bMousePressed) {
             return;
         }
         int cursor = Cursor.DEFAULT_CURSOR;
         if (!bInside) {
             if (curCursor != cursor) {
                 curCursor = cursor;
                 setCursor(Cursor.getPredefinedCursor(cursor));
             }
             return;
         }
         int x = ev.getX();
         int y = ev.getY();
         int x2 = getWidth()- cornerWidth;
         int y2 = getHeight() - cornerWidth;
         if (x <= cornerWidth) {
             if (y >= 0) {
                 if (y <= cornerWidth)
                    cursor = Cursor.NW_RESIZE_CURSOR;
                 else {
                     if (y >= y2)
                        cursor = Cursor.SW_RESIZE_CURSOR;
                     else
                        cursor = Cursor.MOVE_CURSOR;
                  }
              }
         }
         else if (x >= x2) {
             if (y >= 0) {
                 if (y <= cornerWidth)
                    cursor = Cursor.NE_RESIZE_CURSOR;
                 else {
                     if (y >= y2)
                        cursor = Cursor.SE_RESIZE_CURSOR;
                     else
                        cursor = Cursor.MOVE_CURSOR;
                  }
              }
         }
         else if (y >= 0) {
              if (y < 6)
                  cursor = Cursor.MOVE_CURSOR;
              else if (y >= y2)
                  cursor = Cursor.MOVE_CURSOR;
         }
         if (cursor != curCursor) {
             curCursor = cursor;
             setCursor(Cursor.getPredefinedCursor(cursor));
         } 
    }

    private void processMouseDrag(MouseEvent ev) {
         if (!bMousePressed)
             return;
         if (curCursor == Cursor.DEFAULT_CURSOR)
             return;
         Point loc = getLocation();
         Dimension dim = getSize();
         int px = ev.getX();
         int py = ev.getY();
         int dx = px - pressX;
         int dy = py - pressY;
         int x = loc.x;
         int y = loc.y;
         int x2 = x + dim.width;
         int y2 = y + dim.height;
         if (curCursor == Cursor.MOVE_CURSOR) {
            x += dx;
            if (x < 0)
                x = 0;
            y += dy;
            if (y < 0)
                y = 0;
            x2 = x + dim.width;
            y2 = y + dim.height;
            if (x2 >= parentW)
                x = parentW - dim.width - 1;
            if (y2 >= parentH)
                y = parentH - dim.height - 1;
            setLocation(x, y);
            return;
         }

         x2 = x + dim.width;
         y2 = y + dim.height;
         int minW = 12;
         switch (curCursor) {
            case Cursor.NW_RESIZE_CURSOR:
                x = x + dx;
                if (x < 0)
                    x = 0;
                else if (x >= (x2 - minW))
                    x = x2 - minW;;
                y = y + dy;
                if (y < 0)
                    y = 0;
                else if (y >= (y2 - minW))
                    y = y2 - minW;
                break;
            case Cursor.NE_RESIZE_CURSOR:
                x2 = x + px + pressDX;
                if (x2 <= (x + minW))
                    x2 = x + minW;
                y = y + dy;
                if (y < 0)
                    y = 0;
                else if (y >= (y2 - minW))
                    y = y2 - minW;
                if (x2 >= parentW)
                   x2 = parentW - 1;
                break;
            case Cursor.SW_RESIZE_CURSOR:
                x = x + dx;
                if (x < 0)
                    x = 0;
                else if (x >= (x2 - minW))
                    x = x2 - minW;;
                y2 = y + py + pressDY;
                if (y2 <= (y + minW))
                    y2 = y + minW;
                if (y2 >= parentH)
                   y2 = parentH - 1;
                break;
            case Cursor.SE_RESIZE_CURSOR:
                x2 = x + px + pressDX;
                if (x2 <= (x + minW))
                    x2 = x + minW;
                y2 = y + py + pressDY;
                if (y2 <= (y + minW))
                    y2 = y + minW;
                if (x2 >= parentW)
                   x2 = parentW - 1;
                if (y2 >= parentH)
                   y2 = parentH - 1;
                break;
            default:
                return;
         }
         setBounds(x, y, x2 - x, y2 - y); 
         doLayout();
    }

    public void setBorderWidth(int n) {
         borderWidth = n;
         if (borderWidth < 1)
             borderWidth = 1;
    }

    public void setCornerWidth(int n) {
         cornerWidth = n;
         if (cornerWidth < 1)
             cornerWidth = 1;
    }

    public void paint(Graphics g) {
        super.paint(g);
        if (!bMousePressed)
            return;
        Dimension dim = getSize(); 
        g.setColor(focusColor);
        g.drawRect(0, 0, dim.width - 1, dim.height - 1);
    }
}

