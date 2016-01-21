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


public class  VJLine implements GraphSeries {
     public int  x, y;
     public int  x2, y2;
     public int  lineWidth;
     public int  pX, pY, pWidth, pHeight; // container's boundary
     public float alphaSet;
     public Color fgColor;
     public boolean  bValid = false;
     public boolean  bXorMode = false;
     public boolean  bTopLayer = false;
     public Rectangle clipRect;
     private static double deg90 = Math.toRadians(90.0);
     
     public VJLine() {
         this.pX = 0;
         this.pY = 0;
         this.pWidth = 100;
         this.pHeight = 100;
         this.lineWidth = 1;
         this.alphaSet = 1.0f;
     }

     public void setColor(Color c) {
         fgColor = c;
     }

     public Color getColor() {
         return fgColor;
     }

     public void setValue(Color c, int px, int py, int px2, int py2) {
         fgColor = c;
         if (px2 < px) {
            x = px2;
            x2 = px;
            y = py2;
            y2 = py;
         }
         else {
            x = px;
            x2 = px2;
            y = py;
            y2 = py2;
         }
         if (y2 < 0)
            y2 = 0;
     }

     public boolean isValid() {
         return bValid;
     }

     public void setValid(boolean s) {
         bValid = s;
     }

     public void setXorMode(boolean b) {
         bXorMode = b;
     }

     public boolean isXorMode() {
         return bXorMode;
     }

     public void setTopLayer(boolean b) {
         bTopLayer = b;
     }

     public boolean isTopLayer() {
         return bTopLayer;
     }

     public void setLineWidth(int n) {
         lineWidth = n;
         if (lineWidth < 1 || lineWidth > 50)
            lineWidth = 1;
     }

     public int getLineWidth() {
         return lineWidth;
     }

     public void setAlpha(float n) {
         alphaSet = n;
     }

     public float getAlpha() {
         return alphaSet;
     }

     public boolean equals(Color c, int px, int py, int px2, int py2) {
         if (!bValid)
             return false;
         if (!c.equals(fgColor))
             return false;
         if (x == px && x2 == px2) {
             if (y == py && y2 == py2)
                return true;
         }
         return false;
     }

     public boolean intersects(int dx, int dy, int dx2, int dy2) {
         if (!bValid)
             return false;
         if (x2 < dx || x > dx2)
             return false;
         if (y < dy && y2 < dy)
             return false;
         if (y > dy2 && y2 > dy2)
             return false;
         return true;
     }

     public void setConatinerGeom(int x, int y, int w, int h) {
         pX = x;
         pY = y;
         pWidth = w;
         pHeight = h;
     }


     public void draw(Graphics2D g, boolean bVertical, boolean bRight) {
         if (!bValid)
             return;
         int tx = pX;
         int ty = pY;

         g.setColor(fgColor);

         if (bVertical) {
             if (bRight) {
                tx = pX + pWidth; 
                ty = pY;
                g.translate(tx, ty);
                g.rotate(deg90);
             }
             else {
                tx = pX;
                ty = pY+pHeight;
                g.translate(tx, ty);
                g.rotate(-deg90);
             }
         }
         else {
             if (tx != 0 || ty != 0)
                g.translate(tx, ty);
         }

         g.drawLine(x, y, x2, y2);

         if (bVertical) {
             if (bRight)
                g.rotate(-deg90);
             else
                g.rotate(deg90);
         }
         if (tx != 0 || ty != 0)
             g.translate(-tx, -ty);
     }
}
