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
import java.awt.geom.AffineTransform;
import java.awt.geom.GeneralPath;


public class VJArrow implements GraphSeries {
     private Color fgColor;
     private float alphaSet;
     private Color bgColor;
     private boolean  bValid = false;
     private boolean  bXorMode = false;
     private boolean  bTopLayer = false;
     private int lineWidth;
     private int startX = 0;
     private int startY = 0;
     private int endX = 0;
     private int endY = 0;
     private int arrowSize = 5;
     private double lineScale = 1.0;
     
     public VJArrow(int x1, int y1, int x2, int y2) {
         this.alphaSet = 1.0f;
         this.lineWidth = 1;
         this.startX = x1;
         this.startY = y1;
         this.endX = x2;
         this.endY = y2;
     }

     public VJArrow() {
         this(0, 0, 100, 100);
     }
     
     public void setEndPoints(int x1, int y1, int x2, int y2) {
         startX = x1;
         startY = y1;
         endX = x2;
         endY = y2;
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

     public void setLineWidth(int n) {
         lineWidth = n;
     }

     public int getLineWidth() {
         return lineWidth;
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
         return false;
     }

     public void setConatinerGeom(int x, int y, int w, int h) {
     }

     public void setScale(double n) {
          lineScale = n;
     }

     public void draw(Graphics2D g, int x1, int y1, int x2, int y2, int thick) {
          double dx = x2 - x1;
          double dy = y2 - y1;
          double angle = Math.atan2(dy, dx);
          int len = (int) Math.sqrt(dx*dx + dy*dy);
          startX = x1;
          startY = y1;
          endX = x2;
          endY = y2;
          lineWidth = thick;
          AffineTransform oldAt = g.getTransform();
          AffineTransform at = AffineTransform.getTranslateInstance(x1, y1);

          at.concatenate(AffineTransform.getRotateInstance(angle));
          g.transform(at);
          arrowSize = 4 + thick;
          arrowSize = (int) (lineScale * arrowSize);
          g.drawLine(0, 0, len, 0);
          g.fillPolygon(new int[] {len, len-arrowSize, len-arrowSize, len},
                        new int[] {0, -arrowSize + 1, arrowSize - 1, 0}, 4);
          g.setTransform(oldAt);
     }

     public void draw(Graphics2D g1, int x1, int y1, int x2, int y2) {
          draw(g1, x1, y1, x2, y2, 1);
     }

     public void draw(Graphics2D g, boolean bVertical, boolean bRight) {
          double dx = endX - startX;
          double dy = endY - startY;
          int x2 = endX;
          int y2 = endY;
          double angle = Math.atan2(dy, dx) + Math.PI;
          GeneralPath arrow = new GeneralPath();

          if (lineWidth > 2) {
              double len = Math.sqrt(dx*dx + dy*dy) - lineWidth - 1;
              if (len > 2) {
                  x2 = startX + (int)(Math.cos(angle - Math.PI) * len);
                  y2 = startY + (int)(Math.sin(angle - Math.PI) * len);
              }
          }
          g.setPaint(fgColor);
          g.drawLine(startX, startY, x2, y2);
          arrowSize = 6 + lineWidth * 2;
          arrowSize = (int) (lineScale * arrowSize);
          double da = Math.PI / 5.0;
          double leftX = endX + Math.cos(angle + da) * arrowSize;
          double leftY = endY + Math.sin(angle + da) * arrowSize;
          double rightX = endX + Math.cos(angle - da) * arrowSize;
          double rightY = endY + Math.sin(angle - da) * arrowSize;

          arrow.moveTo((float) endX, (float) endY);
          arrow.lineTo((float) leftX, (float) leftY);
          arrow.lineTo((float) rightX, (float) rightY);
          arrow.closePath();
          g.fill(arrow);
     }
}
