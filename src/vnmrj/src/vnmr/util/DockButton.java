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
import javax.swing.border.*;

public class DockButton extends JButton implements DockConstants {
    private boolean bActive = false;
    private int  id;
    private int  dockPoint;
    private int  iX, iX2;
    private int  iY, iY2;
    private int  imgW;
    private int  imgH;
    private int  imgX;
    private int  imgY;
    private Image img;
    private Image inImg;
    private Image outImg;

    public DockButton(int n, Image i, int w, int h) {
         this.id = n;
         this.img = i;
         this.imgW = w;
         this.imgH = h;
         this.setOpaque(false);
         this.dockPoint = DOCK_EXP_LEFT;
         setBorder(null);
         getDockImages();
    }

    public void setDock(int a) {
         dockPoint = a;
    }

    public int getDock() {
         return dockPoint;
    }

    public void setRange(int x, int y, int w, int h) { 
         iX = x;
         if (iX < 0)
            iX = 0;
         iY = y;
         iX2 = iX + w;
         iY2 = iY + h;
         if (imgW < 1)
             getImageWidth();
         setGeometry();
    }

    public int getImageWidth() {
         if (img != null) {
             imgW = img.getWidth(null);
             imgH = img.getHeight(null);
             if (imgW < 1) {
                imgW = 24;
                imgH = 24;
             }
         }
         return imgW;
    }

    private void getDockImages() {
         ImageIcon icon = Util.getImageIcon("pin_yellow.png");
         if (icon == null) {
             icon = Util.getImageIcon("pin_yellow.gif");
             if (icon == null)
                 return;
         }
         outImg = icon.getImage();
         icon = Util.getImageIcon("pin_green.png");
         if (icon == null) {
             icon = Util.getImageIcon("pin_green.gif");
             if (icon == null) {
                 outImg = null;
                 return;
             }
         }
         inImg = icon.getImage();
         img = outImg;
         getImageWidth();
    }

    private void setGeometry() {
        int x = iX;
        int y = iY;
        int w = imgW + 4;
        int h = imgH + 4;
        switch (dockPoint) {
           case DOCK_EXP_LEFT:
                      x = iX2 - w;
                      iY = iY - 4;
                      break;
           case DOCK_EXP_RIGHT:
                      x = iX2 - w;
                      iY = iY - 4;
                      break;
           case DOCK_FRAME_LEFT:
                      iY = iY - 4;
                      break;
           case DOCK_NORTH_EAST:
                      x = iX2 - w;
                      break;
           case DOCK_SOUTH_WEST:
                      y = iY2 - h;
                      break;
           case DOCK_SOUTH_EAST:
                      x = iX2 - w;
                      y = iY2 - h;
                      break;
           case DOCK_ABOVE_EXP:
                      y = iY - 4;
                      if (y < 1)
                         y = 1;
                      iY = iY - 8;
                      x = iX + 2;
                      iX = iX - 6;
                      break;
           case DOCK_BELLOW_EXP:
                      iY = iY - 8;
                      x = iX + 2;
                      iX = iX - 6;
                      break;
        }
        if (iY < 0)
           iY = 0;
        if (iX < 0)
           iX = 0;
        setBounds(x, y, w, h);
    }

    public boolean contain(int x, int y){
         if (x >= iX && x <= iX2) {
            if (y >= iY && y <= iY2)
               return true;
         }
         return false;
    }

    public int getId() {
         return id;
    }

    public void setActive(boolean a) {
        if (bActive != a) {
            bActive = a;
            repaint();
        }
    }


    public boolean isActive() {
	return bActive;
    }

    public void paint(Graphics g) {
        if (img == null)
           return;
        if (inImg != null) {
            if (bActive) {
                g.drawImage(inImg, 3, 2, imgW + 3, imgH + 2, 0, 0, imgW, imgH, null);
            }
            else {
                g.drawImage(outImg, 3, 2, imgW + 3, imgH + 2, 0, 0, imgW, imgH, null);
            }
           return;
        }
        if (bActive)
           g.setColor(Color.green);
        else
           g.setColor(Color.yellow);
        g.fillOval(0, 0, imgW + 4, imgW + 4);
        g.drawImage(img, 3, 2, imgW + 3, imgH + 2,
              0, 0, imgW, imgH, null);
    }

} // class DockButton
