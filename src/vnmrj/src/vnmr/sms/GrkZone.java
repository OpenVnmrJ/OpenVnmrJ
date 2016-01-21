/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.util.*;


public class GrkZone implements SmsDef
{
   public int id;
   public int rowNum;
   public int colNum;
   public int locX, locY;
   public int locX2, locY2;
   public int minWidth;
   public int trayType;
   public boolean gilsonNum;
   public float width, height;
   public float newWidth, newHeight;
   public float ratio; // the ratio of scale
   public float difX, difY;
   public float maxX, maxY;
   public String idStr;
   public String rackName;
   public float orgx, orgy;
   public float diam;
   public float newDiam;
   public boolean visible;
   public boolean gilsonType;
   public boolean rotated;
   public boolean vDiam; // smaple diameters are diferent 
   public Vector<SmsSample> sampleList;
   public SmsSample sArray[][]; // sample array
   public SmsSample vArray[][] = null; // rotated array

   public GrkZone(int num, float x, float y) {
       this.id = 1;
       this.orgx = x;
       this.orgy = y;
       this.ratio = 1;
       this.difX = 0;
       this.difY = 0;
       this.gilsonNum = false;
       this.visible = false;
       this.trayType = 0;
       this.gilsonType = false;
       this.rotated = false;
       this.vDiam = false;
       if (num < 10)
           num = 10;
       this.sampleList = new Vector<SmsSample>(num);
   }

   public void setId(int n) {
       id = n;
       idStr = "Zone "+Integer.toString(n);
   }

   public int getId() {
       return id;
   }

   public void add(SmsSample s) {
       sampleList.add(s);
   }

   public Vector<SmsSample> getSampleList() {
       return sampleList;
   }

   public SmsSample getSample(int num) {
       if (num < 1)
           return null;
       if (num > sampleList.size())
       {
           if (trayType != GRK97)
              return null;
       }
       int m = sampleList.size();
       for (int k = 0; k < m; k++) {
           SmsSample obj = sampleList.elementAt(k);
           if (obj.id == num)
              return obj;
       }
       return null;
   }

   public void setGilsonNum(boolean b) {
       if (gilsonType) {
           if (gilsonNum != b)
              GrkUtil.setGilosnRack(this, b);
           gilsonNum = b;
       }
       else
           gilsonNum = false;
   }

   public void setRotate(boolean b) {
       if (b && vArray == null) {
           vArray = new SmsSample[colNum][rowNum];
           for (int r = 0; r < colNum; r++) {
              int c1 = colNum - r - 1;
              for (int c = 0; c < rowNum; c++)
                 vArray[r][c] = sArray[c][c1];
           }
       }
       if (rotated != b)
          GrkUtil.rotateZone(this, b); 
       rotated = b;
   }

   public void setupRowCol(String name, boolean type) {
       rackName = name;
       gilsonType = false;
       if (name.equals("205") || name.equals("505"))
           gilsonType = true;
       if (name.equals("205h") || name.equals("505h"))
           gilsonType = true;
       if (gilsonType) {
           if (rowNum * colNum < 90)
               gilsonType = false;
       }
       if (gilsonType)
           gilsonNum = type;
       else
           gilsonNum = false;
       // sArray = new SmsSample[rowNum][colNum];
       GrkUtil.arrangeRowCol(this);
   }

   public boolean setScale(int n) {
       int k, m, row, col;
       float dx, dy, d1, dk;
       SmsSample obj;
       m = sampleList.size();
       if (n == 0) {
           newDiam = diam;
           newWidth = width;
           newHeight = height;
           for (k = 0; k < m; k++) {
              obj = sampleList.elementAt(k);
              obj.newDiam = obj.diam;
           }
           return true;
       }
       if (sArray == null || vArray == null)
           return true;
       if (sArray[0][0] == null || vArray[0][0] == null)
           return true;
       if (sArray[0][1] == null || vArray[0][1] == null)
           return true;
       if (rotated) {
           row = colNum;
           col = rowNum;
       }
       else { 
           row = rowNum;
           col = colNum;
       }
       dx = 25;
       dy = 25;
       dk = 12;
       if (col > 1) {
          if (rotated) {
             dx = vArray[0][1].locX - vArray[0][0].locX - vArray[0][0].width;
             if (vArray[0][0].locY != vArray[0][1].locY)
                 dk = 15;
          }
          else {
             dx = sArray[0][1].locX - sArray[0][0].locX - sArray[0][0].width;
             if (sArray[0][0].locY != sArray[0][1].locY)
                 dk = 15;
          }
       }
       if (row > 1) {
          if (rotated) {
             dy = vArray[1][0].locY - vArray[0][0].locY - vArray[0][0].width;
             if (vArray[0][0].locX != vArray[1][0].locX)
                 dk = 15;
          }
          else {
             dy = sArray[1][0].locY - sArray[0][0].locY - sArray[0][0].width;
             if (sArray[0][0].locX != sArray[1][0].locX)
                 dk = 15;
          }
       }
       if (dy > dx) {
           d1 = dx;
       }
       else {
           d1 = dy;
       }
       if (n > 0) {
           if (d1 < dk) 
              return true;
           dx = (d1 - dk) / ratio;

           if (dk > 10)
              dk = 1;
           else
              dk = 1.5f;
           if (dx < 0.3f)
              return true;
           if (dx > dk)
              dx = dk;
           newDiam += dx;
           newWidth += dx;
           newHeight += dx;
           for (k = 0; k < m; k++) {
              obj = sampleList.elementAt(k);
              obj.newDiam += dx;
           }
           return false;
       }
       // n < 0
       if (d1 > 20) 
           return true;
       d1 = sArray[0][0].newDiam;
       d1 -= 2;
       if (d1 <= 0.2)
           return true;
       dx = d1 / 4;
       if (dx < 0.2f)
           dx = 0.2f;
       if (dx > 0.5f)
           dx = 0.5f;
       newDiam -= dx;
       newWidth -= dx;
       newHeight -= dx;
       for (k = 0; k < m; k++) {
           obj = sampleList.elementAt(k);
           obj.newDiam -= dx;
       }
       return false;
   }
}

