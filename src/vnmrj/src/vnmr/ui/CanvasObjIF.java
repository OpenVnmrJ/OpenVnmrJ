/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;

public interface CanvasObjIF {
    public void setRatio(int w, int h, boolean doAdjust);
    public void setSize(int w, int h);
    public void setVisible(boolean b);
    public boolean isVisible();
    public void setSelected(boolean b);
    public boolean isSelected();
    public boolean isResizable();
    public Rectangle getBounds();
    public int  getX();
    public int  getY();
    public int  getWidth();
    public int  getHeight();
    public void setX(int n);
    public void setY(int n);
    public void setWidth(int n);
    public void setHeight(int n);
    public void paint(Graphics2D g, Color hcolor, boolean bBorder);
    public void print(Graphics2D g, boolean bColor, int w, int h);
}
