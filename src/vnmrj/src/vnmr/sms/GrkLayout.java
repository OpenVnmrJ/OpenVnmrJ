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
import java.util.*;


public class GrkLayout implements LayoutManager, SmsLayoutIF, SmsDef
{
   private Vector<GrkZone> zList;
   private int scaleDir = 0;
   private Container cont = null;
   private boolean rotated = false;

   public void setZoneList(Vector<GrkZone> list) {
       zList = list;
   }

   public void setSampleList(SmsSample s[]) {
   }

   public void setStartSample(int s) {
   }

   public void setLastSample(int s) {
   }

   public void setRev(int s) {
   }

   public int zoomDir() {
       return scaleDir;
   }

   public void setRotate(boolean b) {
       if (rotated != b) {
           rotated = b;
           if (cont != null)
               layoutContainer(cont);
       }
   }

   private void rotateLayout() {
       Dimension pSize = cont.getSize();
       int x, x1;
       int y, y1;
       int n, k, i;
       int pw = pSize.width - 12;
       int ph = pSize.height - 12;
       float fw, fh;
       float rx, ry, rd;
       boolean multiZone = false;
       SmsSample obj;
       Vector<SmsSample> oList;
       GrkZone zone;
       fw = 0;
       fh = 0;
       if (zList.size() == 1) {
          zone = zList.elementAt(0);
          x = 0;
          y = 0;
          fh = zone.newWidth;
          fw = zone.newHeight;
       }
       else {
          multiZone = true;
          ph = ph - 12;
          for (n = 0; n < zList.size(); n++) {
              zone = zList.elementAt(n);
              rx = zone.orgx + zone.newWidth;
              if (rx > fh)
                  fh = rx;
              rx = zone.orgy + zone.newHeight;
              if (rx > fw)
                  fw = rx;
          }
       }
       rx = (float) pw / fw;
       ry = (float) ph / fh;
       scaleDir = 0;
       rd = 0;
       if (rx > ry) {
             rd = (rx - ry) / rx;
             rx = ry;
             scaleDir = 1;
       }
       else if (rx < ry) {
             rd = (ry - rx) / ry;
             scaleDir = 2;
       }
       if (rd < 0.1)
           scaleDir = 0;
 
       x1 = (int) (fw * rx);
       x1 = (pw - x1) / 2 + 6;
       y1 = (int) (fh * rx);
       y1 = (ph - y1) / 2;
       y1 = ph - y1 + 6;
       if (multiZone)
           y1 += 12;
       for (n = 0; n < zList.size(); n++) {
             zone = zList.elementAt(n);
             oList = zone.getSampleList();
             if (multiZone) {
                 x = x1 + (int) (zone.orgy * rx);
                 y = y1 - (int) (zone.orgx * rx);
             }
             else {
                 x = x1;
                 y = y1;
             }
             zone.locX = x;
             zone.locX2 = x + (int) (zone.newHeight * rx);
             zone.locY = y - (int) (zone.newWidth * rx);
             zone.locY2 = y;
             i = (int) (zone.newDiam * rx);
             if (i < 4)
                 i = 4;
             zone.minWidth = i;
             k = oList.size();
             for (i = 0; i < k; i++) {
                 obj = oList.elementAt(i);
                 obj.locX = x + (int) (obj.orgy * rx);
                 // obj.locY = y - (int) (obj.orgx * rx);
                 obj.width = (int) (obj.newDiam * rx);
                 obj.locY = y - (int) (obj.orgx * rx) - obj.width;
             }
       }
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
      synchronized (target.getTreeLock()) {
         cont = target;
         if (zList == null)
             return;
         if (zList.size() < 1)
             return;
         if (rotated) {
             rotateLayout();
             return;
         }
         Dimension pSize = target.getSize();
         int x, x1;
         int y, y1;
         int n, k, i;
         int ox, oy, ow;
         int pw = pSize.width - 12;
         int ph = pSize.height - 12;
         float fw, fh;
         float rx, ry, rd;
         boolean multiZone = false;
         boolean marvin = false;
         SmsSample obj;
         Vector<SmsSample> oList;
         GrkZone zone;
         fw = 0;
         fh = 0;
         zone = zList.elementAt(0);
         if (zone.trayType == GRK49 || zone.trayType == GRK97)
             marvin = true;
         if (zList.size() == 1) { // one zone only
             // zone = (GrkZone) zList.elementAt(0);
             x = 0;
             y = 0;
             fw = zone.newWidth;
             fh = zone.newHeight;
             if (marvin) {
                 ph = ph - 34;
                 pw = pw - 20;
             }
         }
         else {
             multiZone = true;
             ph = ph - 14;
             if (marvin) {
                 ph = ph - 14;
                 pw = pw - 14;
             }
             for (n = 0; n < zList.size(); n++) {
                 zone = zList.elementAt(n);
                 rx = zone.orgx + zone.newWidth;
                 if (rx > fw)
                     fw = rx;
                 rx = zone.orgy + zone.newHeight;
                 if (rx > fh)
                     fh = rx;
             }
         }
         rx = (float) pw / fw;
         ry = (float) ph / fh;
         scaleDir = 0;
         rd = 0;
         if (rx > ry) {
             rd = (rx - ry) / rx;
             rx = ry;
             scaleDir = 1;
         }
         else if (rx < ry) {
             rd = (ry - rx) / ry;
             scaleDir = 2;
         }
         if (rd < 0.1)
             scaleDir = 0;
         x1 = (int) (fw * rx);
         x1 = (pw - x1) / 2 + 6;
         y1 = (int) (fh * rx);
         y1 = (ph - y1) / 2 + 6;
         if (multiZone || marvin) {
             y1 += 14;
             if (marvin)
                 x1 += 14;
         }
         for (n = 0; n < zList.size(); n++) {
             zone = zList.elementAt(n);
             oList = zone.getSampleList();
             if (multiZone) {
                 x = x1 + (int) (zone.orgx * rx);
                 y = y1 + (int) (zone.orgy * rx);
             }
             else {
                 x = x1;
                 y = y1;
             }
             zone.locX = x;
             zone.locY = y;
             zone.locX2 = x + (int) (zone.newWidth * rx);
             zone.locY2 = y + (int) (zone.newHeight * rx);
             i = (int) (zone.newDiam * rx);
             if (i < 4)
                 i = 4;
             zone.minWidth = i;
             zone.ratio = rx;
             k = oList.size();
             for (i = 0; i < k; i++) {
                 obj = oList.elementAt(i);
                 // obj.locX = x + (int) (obj.orgx * rx);
                 // obj.locY = y + (int) (obj.orgy * rx);
                 // obj.width = (int) (obj.newDiam * rx);
                 ox = x + (int) (obj.orgx * rx);
                 oy = y + (int) (obj.orgy * rx);
                 ow = (int) (obj.newDiam * rx);
                 obj.setBounds(ox, oy, ow, ow);
             }
         }
      }
   }
}

