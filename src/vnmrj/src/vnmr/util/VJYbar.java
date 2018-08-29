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


public class  VJYbar implements GraphSeries {
     public int  dataSize;
     public int  capacity;
     public int  lineWidth;
     public int  pX, pY, pWidth, pHeight; // container's boundary
     public int[] xPoints;
     public int[] yPoints;
     public Color fgColor;
     public float alphaSet;
     public Color bgColor;
     public boolean  bValid = false;
     public boolean  bXorMode = false;
     public boolean  bTopLayer = false;
     public Rectangle clipRect;
     private static double deg90 = Math.toRadians(90.0);
     
     public VJYbar() {
         this.dataSize = 0;
         this.capacity = 0;
         this.lineWidth = 1;
         this.pWidth = 100;
         this.pHeight = 100;
         this.pX = 0;
         this.pY = 0;
         this.alphaSet = 1.0f;
     }

     public int[] getXPoints() {
         return xPoints; 
     }
     
     public void setXPoints(int[] pnts) {
         xPoints = pnts; 
     }
     
     public int[] getYPoints() {
         return yPoints; 
     }
     
     public void setYPoints(int[] pnts) {
         yPoints = pnts; 
     }
     
     public void setColor(Color c) {
         fgColor = c;
     }

     public Color getColor() {
         return fgColor;
     }

     public void setBgColor(Color c) {
         bgColor = c;
     }

     public Color getBgColor() {
         return bgColor;
     }

     public void setSize(int s) {
         dataSize = s;
         setCapacity(s);
     }

     public int getSize() {
         return dataSize;
     }

     public void setLineWidth(int n) {
         lineWidth = n;
         if (lineWidth < 1 || lineWidth > 50)
            lineWidth = 1;
     }

     public int getLineWidth() {
         return lineWidth;
     }

     public void setCapacity(int s) {
         if (capacity >= s)
             return;
         if (s < 4)
             s = 4;
         capacity = s;
         xPoints = new int[s];
         yPoints = new int[s];
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

     public void setAlpha(float n) {
         alphaSet = n;
     }

     public float getAlpha() {
         return alphaSet;
     }

     public boolean intersects(int x, int y, int x2, int y2) {
         if (!bValid || capacity <= 0)
             return false;
         if (x2 < xPoints[0] || x > xPoints[dataSize - 1])
             return false;
         if (y2 < yPoints[0] || y > yPoints[0])
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
         if (!bValid || dataSize < 1)
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
         
         if (dataSize > 1)
             g.drawPolyline(xPoints, yPoints, dataSize);
         else
             g.drawLine(xPoints[0], yPoints[0], xPoints[0], yPoints[0]);

         if (bVertical) {
             if (bRight) {
                g.rotate(-deg90);
             }
             else
                g.rotate(deg90);
         }
         if (tx != 0 || ty != 0)
             g.translate(-tx, -ty);
     }
}
