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
import javax.swing.*;

public class VButtonIcon extends ImageIcon
{
    Image img = null;
    int   iw, ih;
    int   bw = 0;
    int   bh = 0;
    private int stretchDir = VStretchConstants.NONE;

    public VButtonIcon (Image image) {
        super(image);
	this.img = image;
	if (image != null) {
           iw = image.getWidth(null);
           ih = image.getHeight(null);
        }
	else {
	   iw = 0;
	   ih = 0;
        }
        bw = iw;
        bh = ih;
    }

    public int getIconHeight() {
        return  bh;
    }

    public int getIconWidth() {
        return  bw;
    }

    public void setStretch(int n) {
        stretchDir = n;
    }

    private void stretchImage(int w, int h) {
	if (img == null)
            return;
        if (iw <= 0 || ih <= 0)
            return;
        bw = iw;
        bh = ih;
        int dif = w - iw;
        if (w > 80)
           dif = 8;
        if (dif > 1) {
           // set margin
           if (dif > 4)
               w = w - 4;
           else
               w = w - 2;
        }
        dif = h - ih;
        if (h > 80)
           dif = 8;
        if (dif > 1) {
           if (dif > 4)
               h = h - 4;
           else
               h = h - 2;
        }
        if (stretchDir == VStretchConstants.HORIZONTAL) {
            bw = w;
            return;
        }
        if (stretchDir == VStretchConstants.VERTICAL) {
            bh = h;
            return;
        }
        else if (stretchDir == VStretchConstants.BOTH) {
            bw = w;
            bh = h;
            return;
        }
        if (stretchDir == VStretchConstants.EVEN) {
            double rw = (double) w / (double) iw;
            double rh = (double) h / (double) ih;
            if (rw > rh)
               rw = rh;
            bw = (int) ((double) iw * rw);
            bh = (int) ((double) ih * rw);
        }
    }

    public void setSize(int w, int h) {
        bw = w;
        bh = h;
        if (stretchDir != VStretchConstants.NONE) {
           stretchImage(w, h);
           return;
        }
        if (bw > iw)
           bw = iw;
        if (bh > ih)
           bh = ih;
    }    

    public synchronized void paintIcon(Component c, Graphics g, int x, int y) {
        if (img == null)
           return;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        g.drawImage(img, x, y, bw, bh,  null);
    }
}

