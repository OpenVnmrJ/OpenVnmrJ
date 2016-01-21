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


public class  VJText implements GraphSeries {
     private int  x, y;
     private int  fontAscent;
     private int  lineWidth;
     public  int  fontIndex;
     public int  pX, pY, pWidth, pHeight; // container's boundary
     private Color fgColor = Color.black;
     private float alphaSet;
     private String text;
     private boolean bVertical = false;
     private boolean bValid = false;
     private boolean bXorMode = false;
     private boolean bTopLayer = false;
     private AffineTransform tr90;
     public GraphicsFont graphFont;
     public Rectangle clipRect;
     
     public VJText() {
          this.fontAscent = 2;
          this.pX = 0;
          this.pY = 20;
          this.alphaSet = 1.0f;
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

     public void setValid(boolean s) {
         bValid = s;
         if (!s)
            graphFont = null;
     }

     public void setXorMode(boolean b) {
         bXorMode = b;
     }

     public boolean isXorMode() {
         return bXorMode;
     }

     public void setVertical(boolean s) {
         bVertical = s;
     }

     public void setTopLayer(boolean b) {
         bTopLayer = b;
     }

     public boolean isTopLayer() {
         return bTopLayer;
     }

     public void setFontAscent(int s) {
         fontAscent = s;
     }

     public void setValue(Color c, int px, int py, String s, boolean v) {
         fgColor = c;
         x = px;
         y = py;
         text = s;
         bVertical = v;
     }

     public void setLineWidth(int n) {
         lineWidth = n;
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

     public boolean equals(Color c, int px, int py, String s, boolean v) {
         if (!bValid)
             return false;
         if (v != bVertical)
             return false;
         if (x != px || y != py)
             return false;
         // if (!c.equals(fgColor))
         //     return false;
         if (!s.equals(text))
             return false;
         return true;
     }

     public boolean intersects(int dx, int dy, int dx2, int dy2) {
         if (!bValid)
             return false;
         if (x < (dx - 5) || x >= (dx2 - 5))
             return false;
         int ty = y - 5;
         int y0 = dy;
         int y1 = dy2 - 3;
         if (dy > dy2) {
             y0 = dy2;
             y1 = dy;
         }
         if (ty < y0 || y > y1)
             return false;
         return true;
     }


     public void setConatinerGeom(int x, int y, int w, int h) {
         pX = x;
         pY = y;
         pWidth = w;
         pHeight = h;
     }

     public void draw(Graphics2D g, boolean v, boolean bRight) {
         if (!bValid)
             return;
         if (graphFont != null) {
             g.setFont(graphFont.getFont(fontIndex));
             if (!graphFont.isDefault && graphFont.fontColor != null)
                fgColor = graphFont.fontColor;
         }
         g.setColor(fgColor);
         if (pX != 0 || pY != 0)
             g.translate(pX, pY);
         if (bVertical) {
             AffineTransform org = g.getTransform();
             if (tr90 == null)
                tr90 = new AffineTransform();
             tr90.setToIdentity();
             tr90.translate(x, y);
             tr90.quadrantRotate(3);
             g.setTransform(tr90);
             g.drawString(text, 0, fontAscent);
             g.setTransform(org);
         }
         else {
            /******
             Composite composite = g.getComposite();
             AlphaComposite alpha = AlphaComposite.getInstance(AlphaComposite.SRC_OVER, 0.6f);
             g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
             g.setComposite(alpha);
             g.setColor(Color.gray);   // shadow 
             g.drawString(text, x+1, y+1);
             g.setComposite(composite);
             g.setColor(fgColor);
            ******/
             g.drawString(text, x, y);
         }
         if (pX != 0 || pY != 0)
              g.translate(-pX, -pY);
     }
}
