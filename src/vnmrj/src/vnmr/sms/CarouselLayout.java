/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.awt.*;


public class  CarouselLayout implements LayoutManager, SmsLayoutIF
{
   private SmsSample sList[] = null;
   private int first = 1;
   private int last = 9;
   private int scaleDir = 0;

   public void setSampleList(SmsSample s[]) {
       sList = s;
   }

   public void setStartSample(int s) {
       first = s;
   }

   public void setLastSample(int s) {
       last = s;
   }

   public int zoomDir() {
       return scaleDir;
   }

   public void addLayoutComponent(String name, Component comp) {}

   public void removeLayoutComponent(Component comp) {}

   public Dimension minimumLayoutSize(Container target) {
        return new Dimension(0, 0); // unused
   } 

   public Dimension preferredLayoutSize(Container target) {
       Dimension dim = new Dimension(0, 0);
       return dim;
   }
    
   public void layoutContainer(Container target) {
      if (sList == null)
         return; 
      if (last - first < 2)
         return; 
      synchronized (target.getTreeLock()) {
         Dimension pSize = target.getSize();
         int x;
         int y;
         int xc, yc;
         int objW = 0;
         int hW = 0;
         int ox, oy;
         int num = last - first + 1;
         double rw = 0;
         double deg;
         double dg;
         int w = pSize.width;
         int h = pSize.height;
         if (w < 20 || h < 20)
             return;
         xc = w / 2;
         yc = h / 2;
         scaleDir = 0;
         dg = 0;
         if (w > h) {
            dg = ((double)(w - h )) / (double) w;
            w = h;
            scaleDir = 1;
         }
         else if (w < h) {
            dg = ((double)(h - w )) / (double) h;
            scaleDir = 2;
         }
         if (dg < 0.1)
            scaleDir = 0;
         objW = w / 7;
         hW = objW / 2;
         if (objW > 60)
            objW = 60;
         rw = (double) objW * 3 - 4;
         deg = 3.1416;
         dg = deg * 2 / num;
         if (objW < 50)
            objW += 4; 
         SmsSample obj;
         for (int k = first; k <= last; k++) {
            obj = sList[k];
            x = (int) (rw * Math.cos(deg));
            y = (int) (rw * Math.sin(deg));
            // obj.locX = xc + x - hW; 
            // obj.locY = yc - y - hW; 
            // obj.width = objW;
            ox = xc + x - hW;
            oy = yc - y - hW; 
            obj.setBounds(ox, oy, objW, objW);
            deg = deg - dg; 
         }
         if (objW < 60) {
            obj = sList[first];
            y = obj.locY;
         }
      }
   }
}

