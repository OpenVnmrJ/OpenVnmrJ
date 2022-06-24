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


public class SmsLayout implements LayoutManager, SmsLayoutIF
{
   private SmsSample sList[] = null;
   private int first;
   private int last;
   private int rev = 0;
   private int scaleDir = 0;
   static int[] nums = {6, 7, 6, 7, 6, 7, 6, 5, 6, 7};

   public void setSampleList(SmsSample s[]) {
       sList = s;
   }

   public void setStartSample(int s) {
       first = s;
   }

   public void setLastSample(int s) {
       last = s;
   }

   public void setRev(int s) {
       rev = s;
   }

   public int zoomDir() {
       return scaleDir;
   }

   public void addLayoutComponent(String name, Component comp) {}

   public void removeLayoutComponent(Component comp) {}

   public Dimension minimumLayoutSize(Container target) {
        return new Dimension(100, 60); // unused
   } 

   public Dimension preferredLayoutSize(Container target) {
       Dimension dim = new Dimension(100, 100);
       return dim;
   }
    
   public void layoutContainer(Container target) {
      if (sList == null)
         return; 
      synchronized (target.getTreeLock()) {
         Dimension pSize = target.getSize();
         int vgap = 20;
         int hgap = 20;
         int x, x1;
         int n;
         int y, y1;
         int index;
         int w1 = 0;
         int w2 = 0;
         int h1 = 0;
         int rows = 8;
         int cols = 7;
         float rd = 0;
         int samples = 50;
         boolean vertical = true;
         int w = pSize.width;
         int h = pSize.height;
         if (w < 50 || h < 50)
             return;
         if ((w - h) > 10)
             vertical = false;
         samples = last - first + 1;
         if (samples < 50 || samples > 100) 
             return;
         if (samples > 50) {
             if (vertical) {
                 rows = 17;
                 cols = 7;
             }
             else {
                 rows = 8;
                 cols = 15;
             }
         }
         while (vgap > 0) {
             w1 = (w - hgap * (cols + 1)) / cols;
             h1 = (h - vgap * (rows + 1)) / rows;
             if (w1 > h1)
                w2 = h1;
             else
                w2 = w1;
             if (w2 > 40)
                break;
             if (w1 > h1) {
                if (vgap > 4)
                   vgap -= 2;
                else
                   break;
             }
             else {
                if (hgap > 4)
                   hgap -= 2;
                else
                   break;
             }
         }
         vgap = (h - w2 * rows) / (rows + 1);
         hgap = (w - w2 * cols) / (cols + 1);
         scaleDir = 0;
         if (hgap > vgap) {
            rd = ((float) hgap - vgap) / (float) hgap;
            hgap = vgap;
            scaleDir = 1;
         }
         else if (hgap < vgap) {
            rd = ((float) vgap - hgap) / (float) vgap;
            scaleDir = 2;
         }
         if (rd < 0.1)
            scaleDir = 0;
         SmsSample obj;
         y1 = (h - w2 * rows - hgap * (rows - 1) ) / 2;
         x1 = (w - w2 * cols - hgap * (cols - 1) ) / 2;
         if (rev == 1)
         {
            if (vertical)
               y1 = y1 + w2 * 9 + hgap * 8;
            else
               x1 = x1 + w2 * 8 + hgap * 7;
         }
         index = first;
         y = y1;
         samples = first + 49;
         for (int loop = 0; loop < 2; loop++) {
            for (int i = 0; i < 8; i++) {
                n = nums[i];
                if (n == 6)
                   x = x1 + w2 / 2;
                else
                   x = x1;
                for (int k = 0; k < n; k++) {
                   obj = sList[index++];
                   obj.locX = x;
                   obj.locY = y;
                   obj.width = w2;
                   x = x + w2 + hgap;
                   if (index > samples)
                      break;
                }
                y = y + w2 + hgap;
            }
            if (index > last)
                break;
            samples += 50;
            if (vertical) {
               if (rev == 1)
                  y = (h - w2 * rows - hgap * (rows - 1) ) / 2;
               else
                  y = y1 + w2 * 9 + hgap * 8;
            }
            else {
               y = y1;
               if (rev == 1)
                  x1 = (w - w2 * cols - hgap * (cols - 1) ) / 2;
               else
                  x1 = x1 + w2 * 8 + hgap * 7;
            }
         }
      }
   }
}

