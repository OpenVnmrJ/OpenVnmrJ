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
import javax.swing.*;
import vnmr.util.Util;

public class DpsEventObj extends DpsObj {
    private Image  img;
    private int  imgHeight;
    private int  viewWidth, viewHeight;
    private int  locX, locY;

    public DpsEventObj(int n) {
         super(n);
         getObjImage();
         this.viewHeight = 500;
         this.viewWidth = 500;
    }

    @Override
    public void clear() {
       super.clear();
       img = null;
    }

    private void getObjImage() {
        ImageIcon icon = null;

        imgHeight = 16;
        switch (code) {
             case LOOP:
                   icon = Util.getImageIcon("leftloop.gif");
                   break;
             case ENDLOOP:
                   icon = Util.getImageIcon("rightloop.gif");
                   break;
             case RDBYTE:
                   icon = Util.getImageIcon("readbyte.gif");
                   break;
             case WRBYTE:
                   icon = Util.getImageIcon("writebyte.gif");
                   break;
        }

        if (icon != null) {
            img = icon.getImage();
            imgHeight = img.getHeight(null);
        }
    }

    public void setViewRect(int x, int y, int w, int h) {
        viewWidth = w;
        viewHeight = h;
    }

    public int getLocX() {
        return locX;
    }

    public void setLocX(int x) {
        locX = x;
    }

    public int getLocY() {
        return locY;
    }
 
    public void setLocY(int y) {
        locY = y;
    }

    @Override
    public boolean intersect(int x, int y) {
        if (timeXs == null || dataCount < 1)
            return false;
        if (x >= locX && x <= (locX + imgHeight)) {
            if (y >= locY && y <= (locY + imgHeight))
                return true;
        }
        return false;
    }

    @Override
    public void setDisplayPoints(double xgap, double h, int yoffset) {
        super.setDisplayPoints(xgap, h, yoffset);
        if (dataCount < 1)
            return;
            locX = timeXs[0] - imgHeight / 2;
        if (code == LOOP || code == ENDLOOP) {
            locX = timeXs[0] - imgHeight / 2;
            if (locX < 0)
                locX = 0;
            else {
               if ((locX + imgHeight) > viewWidth)
                  locX = viewWidth - imgHeight;
            }
            locY = 2;
        }
        else {
            locX = timeXs[0] + width / 2 - imgHeight / 2;
            locY = 2;
        }
    }

    @Override
    public boolean draw(Graphics2D g, Color selectedColor, boolean bShowDuration, Rectangle clip) {
        if (img == null || dataCount < 1)
            return false;
        if ((locX + imgHeight) < clip.x)
            return false;
        if (locX > (clip.x + clip.width))
            return false;
        if (bSelected)
            g.setColor(selectedColor);
        else
            g.setColor(Color.yellow);
        int x = timeXs[0];
        int y = locY + imgHeight + 2;
        if (imgHeight < 8) {
            if (img != null)
                imgHeight = img.getHeight(null);
            else
                imgHeight = 12;
        }
        g.fillOval(locX, locY, imgHeight, imgHeight);
        if (img != null)
            g.drawImage(img, locX, locY, null);
        if (code == LOOP || code == ENDLOOP) {
            g.setColor(Color.lightGray);
            g.drawLine(locX + imgHeight / 2, y, x, y + 12);
            g.drawLine(x, y + 12, x, viewHeight - 10);
        }
        if (bShowDuration) {
            if (code == RDBYTE && durationStr != null) {
                if (!bSelected)
                    g.setColor(Color.black);
                y = locY + imgHeight + 14;
                g.drawString(durationStr, locX, y);
            }
        }
        return true;
   }
}

