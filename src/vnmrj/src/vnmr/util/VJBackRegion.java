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


public class VJBackRegion {
     public int  pX, pY;
     public int  width, height;
     public Color fgColor;
     public boolean bValid = false;
     public Rectangle clipRect;
     
     public VJBackRegion(int x, int y, int w, int h) {
         this.pX = x;
         this.pY = y;
         this.width = w;
         this.height = h;
     }

     public VJBackRegion() {
         this(0, 0, 0, 0);
     }

     public void setColor(Color c) {
         fgColor = c;
     }

     public Color getColor() {
         return fgColor;
     }

     public boolean isValid() {
         return bValid;
     }

     public void setRegion(int x, int y, int w, int h ) {
         pX = x;
         pY = y;
         width = w;
         height = h;
     }

     public boolean equals(int x, int y, int w, int h ) {
         if (x == pX && y == pY) {
            if (w == width && h == height)
                return true;
         }
         return false;
     }
            

     public void setValid(boolean s) {
         bValid = s;
     }

     public void draw(Graphics2D g, int xOff, int yOff) {
         if (!bValid || width < 2 || height < 2)
             return;

         g.setColor(fgColor);
         g.fillRect(pX + xOff, pY + yOff, width, height);
     }
}
