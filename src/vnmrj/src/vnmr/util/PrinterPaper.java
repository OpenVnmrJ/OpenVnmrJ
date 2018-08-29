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
import java.awt.PageAttributes.*;
import javax.print.attribute.standard.*;


public final class PrinterPaper {

    static int  index = 0;
    static int  searchIndex = 0;
    static String searchName = "";
    public static String LETTER = "letter";

    public PrinterPaper() {
    }

    public static Dimension getPaperSize(String name) {
        if (name == null)
            return new Dimension(pws[index], phs[index]);
        String s = name.toLowerCase();
        if (!s.equals(nameList[index])) {
            index = 0;
            for (int k = 0; k < nameList.length; k++)
            {
                if (s.equals(nameList[k]))
                {
                    index = k;
                    break;
                }
            }
        }
        return new Dimension(pws[index], phs[index]);
    }
    
    public static int getPaperMargin(String name) {
        if (name == null)
            return margins[index];
        String s = name.toLowerCase();
        if (!s.equals(nameList[index])) {
            for (int k = 0; k < nameList.length; k++)
            {
                if (s.equals(nameList[k]))
                {
                    index = k;
                    break;
                }
            }
        }
        return margins[index];
    }

    public static int getPaperMargin() {
        return margins[index];
    }

    static String[] nameList =
    {LETTER, "legal", "ledger", "11x17", "flsa", "flse","halfletter",
     "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8",
     "b0", "b1", "b2", "b3", "b4", "b5",
     "c0", "c1", "c2", "c3", "c4", "c5", "c6",
     "null" };

    static int[] pws =
    { 612, 612, 1224, 792, 612, 612, 396,
      2380, 1684, 1190, 842, 595, 421, 297, 210, 148,
      2836, 2004, 1418, 1002, 709, 501,
      2600, 1837, 1298, 918, 649, 459, 323,
      0 };

    static int[] phs =
    { 790, 1008, 792, 1224, 936, 936, 612,
      3368, 2380, 1684, 1190, 842, 595, 421, 297, 210,
      4008, 2386, 2004, 1418, 1002, 709,
      3677, 2600, 1837, 1298, 918, 649, 459,
      0 };

    static int[] margins =
    { 36, 36, 36, 36, 36, 36, 15,
      40, 40, 40, 36, 36, 15, 15, 10, 10,
      40, 40, 40, 40, 40, 30,
      40, 40, 40, 40, 36, 20, 20,
      0 };

    static String[] paperNames =
    {LETTER, "legal", "ledger",
     "A0", "A1", "A2", "A3", "A4", "A5", "A6",
     "B0", "B1", "B2", "B3", "B4", "B5",
     "C2", "C3", "C4", "C5", "C6",
     "Executive" };

    static double[] paperWidths =    // inch
    { 8.5, 8.5, 11.0,
      33.11, 23.39, 16.54, 11.69, 8.27, 5.83, 4.13,
      39.37, 27.83, 19.69, 13.90, 9.84, 6.93,
      25.51, 18.03, 12.76, 9.02, 6.38,
      7.25, 8.5 };

    static double[] paperHeights =    // inch
    { 11.0, 14.0, 17.0,
      46.91, 33.11, 23.39, 16.54, 11.69, 8.27, 5.83,
      55.67, 39.37, 27.83, 19.69, 13.90, 9.84,
      18.03, 12.76, 9.02, 6.38, 4.49,
      10.55, 11.0 };

    public static String[] getPaperNames() {
       return paperNames;
    }

    public static double getPaperWidth(String name) {
       searchIndex = 0;
       if (name == null || name.equals(searchName))
           return paperWidths[searchIndex];
       for (int k = 0; k < paperNames.length; k++) {
           if (name.equals(paperNames[k])) {
                searchIndex = k;
                break;
           }
       }
       return paperWidths[searchIndex];
    }

    public static double getPaperHeight(String name) {
       searchIndex = 0;
       if (name == null || name.equals(searchName))
           return paperHeights[searchIndex];
       int n = paperNames.length;
       for (int k = 0; k < n; k++) {
           if (name.equals(paperNames[k])) {
                searchIndex = k;
                break;
           }
       }
       return paperHeights[searchIndex];
    }

    public static String getPaperNameBySize(double w, double h) {
        int pnum = 0;
        double rw = 100.0;
        double rh = 100.0;
        double dw, dh;
        int n = paperNames.length;
        if (w <= 0.0 || h <= 0.0)
            return paperNames[pnum];
        for (int k = 0; k < n; k++) {
            dw = Math.abs(paperWidths[k] - w);
            dh = Math.abs(paperHeights[k] - h);
            if (paperWidths[k] >= w && paperHeights[k] >= h) {
                dw = dw / paperWidths[k];
                if (dw <= rw) {
                     dh = dh / paperHeights[k];
                     if (dh <= rh) {
                         rh = dh;
                         rw = dw;
                         pnum = k;
                     }
                }
            }
        }
        return paperNames[pnum];
    }
}
