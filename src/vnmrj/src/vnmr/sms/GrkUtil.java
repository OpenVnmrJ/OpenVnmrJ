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


public final class GrkUtil implements SmsDef
{
   private static GrkZone zone = null;
   private static int curRows = 0;
   private static int curCols = 0;
   private static int totalCols = 0;
   private static float posX[];
   private static Vector<SmsSample> sList;
   private static SmsSample sArray[][];
   static final String colNames[] =
          { "H", "G", "F", "E", "D", "C", "B", "A" };
   static final String cNames[] =
          { "A","B","C","D","E","F","G","H","I","J","K","L","M","N",
            "O","P","Q","R","S","T","U","V","W","X","Y","Z" };
   static final String rowNames[] =
          { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
   static final String row2Names[] =
          { "12", "11", "10", "9", "8", "7", "6", "5", "4", "3", "2", "1" };
   static final String marvinColNames[] =
          { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
   static final String marvinRowNames[] =
          { "A","B","C","D","E","F","G","H","I","J","K","L","M","N" };
   

   public GrkUtil() {
   }

   public static void arrangeRowCol(GrkZone z) {
        int x, x2, k;
        int m;
        SmsSample o;
        int cols = 0;
        int rows = 0;

        zone = z;
        if (zone.colNum > totalCols) {
             totalCols = zone.colNum;
             posX = new float[totalCols+1];
        }
        for (k = 0; k <= totalCols; k++)
              posX[k] = z.maxX;

        // calculate the number of column
        sList = zone.getSampleList();
        m = sList.size();
        if (m <= 0)
             return;
        for (k = 0; k < m; k++) {
           o = sList.elementAt(k);
           for (x = 0; x < cols; x++) {
              if (o.orgx == posX[x])
                 break;
              if (o.orgx < posX[x]) {
                  for (x2 = cols; x2 > x; x2--)
                      posX[x2] = posX[x2 - 1];
                  cols++;
                  posX[x] = o.orgx;
                  break;
              }
           }
           if (x >= cols) { // new column
              cols++;
              if (cols >= totalCols) {
                   totalCols = cols + 6;
                   float nposX[];
                   nposX = new float[totalCols+1];
                   for (x2 = 0; x2 < cols; x2++)
                          nposX[x2] = posX[x2];
                   for (x2 = cols; x2 <= totalCols; x2++)
                          nposX[x2] = z.maxX;
                   posX = nposX;
              }
              posX[x] = o.orgx;
           }
        }
        if (cols < 1)
           cols = 1;
        rows = (sList.size() + cols - 1) / cols;
        if (rows < 1)
           rows = 1;
        zone.colNum = cols;
        zone.rowNum = rows;
        curRows = rows;
        curCols = cols;
        zone.sArray = new SmsSample[rows][cols];

        // String rackName = zone.rackName;
        // if (rackName.equals("205") || rackName.equals("505")) {
        setCommonRack();
        if (zone.gilsonNum) {
             setGilosnRack(zone, true);
        }
   }

   public static void setGilosnRack(GrkZone z, boolean gilson) {
        int  r, c, n;
        int  cols, rows;
        SmsSample o;
        sArray = z.sArray;
        String rStr;

        rows = z.rowNum;
        cols = z.colNum;
        if (gilson) {
           for (r = 0; r < rows; r++) {
               rStr = Integer.toString(r+1);
               n = 0;
               for (c = cols-1; c >= 0; c--) {
                  o = sArray[r][c];
                  if (o != null) {
                      o.idStr = cNames[n]+rStr;
                  }
                  n++;
               }
           }
        }
        else {
           for (r = 0; r < rows; r++) {
               for (c = 0; c < cols; c++) {
                  o = sArray[r][c];
                  if (o != null) {
                      o.setId(o.id);
                  }
               }
           }
        }

/*
        if (zone.gilsonNum) {
            cNames = colNames;
            rNames = rowNames;
        }
        else {
        }
        for (row = 0; row < curRows; row++) {
            for (col = 0; col < curCols; col++) {
                 c = sArray[row][col];
                 if (c != null) {
                    c.idStr = cNames[col]+rNames[row];
                    c.row = row + 1;
                    c.col = col + 1;
                 }
            }
        }
*/
   }


   public static void setCommonRack() {
        int  i, k, col, row;
        SmsSample o, c;
        int m = sList.size();
        sArray = zone.sArray;
        for (k = 0; k < m; k++) {
           o = sList.elementAt(k);
           col = -1;
           for (i = 0; i < curCols; i++) {
              if (o.orgx == posX[i]) {
                  col = i;
                  break;
              }
           }
           if (col >= 0) {
              for (row = 0; row < curRows; row++) {
                  if (sArray[row][col] == null) { 
                      sArray[row][col] = o;
                      break;
                  }
                  c = sArray[row][col];
                  if (c.orgy > o.orgy) {
                      for (i = curRows - 1; i > row; i--) {
                         sArray[i][col] = sArray[i-1][col]; 
                      }
                      sArray[row][col] = o;
                      break;
                  }
              }
           }
        }
        if (zone.trayType == GRK12)
            return;
        k = 1;
        if (zone.trayType == GRK49 || zone.trayType == GRK97) {
            k = (zone.id - 1) * 48 + 1;
/*
            for (col = 0; col < curCols; col++) {
                 for (row = 0; row < curRows; row++) {
                     c = sArray[row][col];
                     if (c != null) {
                        c.row = row + 1;
                        c.col = col + 1;
                        c.colStr = marvinColNames[col];
                        c.rowStr = marvinRowNames[row];
                        c.setId(k);
                        k++;
                     }
                 }
            }
*/
            for (row = 0; row < curRows; row++) {
                 for (col = 0; col < curCols; col++) {
                     c = sArray[row][col];
                     if (c != null) {
                        c.row = row + 1;
                        c.col = col + 1;
                        c.colStr = marvinColNames[col];
                        c.rowStr = marvinRowNames[row];
                        c.setId(k);
                        k++;
                     }
                 }
            }
            return;
        }
        for (row = 0; row < curRows; row++) {
            for (col = 0; col < curCols; col++) {
                 c = sArray[row][col];
                 if (c != null) {
                    c.row = row + 1;
                    c.col = col + 1;
                    c.setId(k);
                    k++;
                 }
            }
        }
   }

   public static void rotateZone(GrkZone z, boolean rotate) {
        int  cols, rows;
        int  r, c;
        SmsSample o;
        if (rotate) {
           sArray = z.vArray;
           rows = z.colNum;
           cols = z.rowNum;
        }
        else {
           sArray = z.sArray;
           rows = z.rowNum;
           cols = z.colNum;
        }
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
               o = sArray[r][c];
               if (o != null) {
                   o.row = r + 1;
                   o.col = c + 1;
               }
            }
        }
   }

}
