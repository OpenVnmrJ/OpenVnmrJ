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
import java.io.*;
import javax.swing.*;

public class GraphicsFont  {
     public Font[] fontList;
     public Font   font;
     public FontMetrics fontMetric;
     public String fontName;
     public Color fontColor;
     public int originHeight = 15;
     public int originWidth = 12;
     public int fontAscent;
     public int fontDescent;
     public int fontWidth;
     public int fontHeight;
     public int fontNum = 0;
     public int index;
     public int displayWidth;
     public int displayHeight;
     public int[] charWidths;
     public int[] charHeights;
     public int[] Descents;
     public int[] Ascents;
     public boolean isDefault = false;
     private float scaled = 1.0f;
     public static String defaultName = "Default";
     private static JLabel refComp = null;
     private static String refStr = "ppMM9";

     public GraphicsFont(String name, boolean  empty) {
          this.fontName = name;
          this.index = 0;
          if (!empty)
             update();
     }

     public GraphicsFont(String name) {
          this(name, false);
     }

     public GraphicsFont() {
          this(defaultName, false);
     }

     public boolean update() {
         if (fontName == null)
             fontName = defaultName;
         if (!fontName.equals(defaultName)) {
             font = DisplayOptions.getFont(fontName);
             fontColor = DisplayOptions.getColor(fontName);
         }
         else {
             isDefault = true;
             font = DisplayOptions.getFont("GraphText");
             /***********
             font = DisplayOptions.getFont("AxisLabel");
             if (fontList != null)
                 return false;
             font = UIManager.getFont("Canvas.font");
             if (font == null)
                font = UIManager.getFont("VJ.font");
             ***********/
             fontColor = Color.black;
         }
         if (font == null)
             font = new Font("SanSerif", Font.ITALIC, 12);
         int w = originWidth;
         int h = originHeight;
         scaled = 1.0f;
         updateFontList(font);
         originHeight = Ascents[0];
         originWidth = charWidths[0];
         if (w != originWidth || h != originHeight) {
             displayWidth = 1;
             return true;
         }
         return false;
     }

     public void update(float newSize) {
         if (font == null)
              update();
         if (newSize < 6.0f)
              return;
         Font ft = font.deriveFont(newSize);
         updateFontList(ft);
     }

     public void scale(float f) {
         if (f == scaled)
             return;
         float h = f * (float) originHeight; 
         scaled = f;
         update(h); 
     }

     private void updateFontList(Font fnt) {
         int n;
         float step, fsize;

         /********   Deprecated
         Toolkit tk = Toolkit.getDefaultToolkit();
         fontMetric = tk.getFontMetrics(fnt);
         fontMetric = tk.getFontMetrics(fnt);
         ********/
         if (refComp == null)
             refComp = new JLabel();
         fontMetric = refComp.getFontMetrics(fnt);
         fontAscent = fontMetric.getAscent();
         fontDescent = fontMetric.getDescent();
         fontWidth = fontMetric.stringWidth(refStr) / 5; // average width
         fontHeight = fontAscent + fontDescent;
         if (fontHeight < 6)
             fontHeight = fnt.getSize();

         if (fontList == null) {
            fontNum = 5;
            fontList = new Font[fontNum];
            charWidths = new int[fontNum];
            charHeights = new int[fontNum];
            Descents = new int[fontNum];
            Ascents = new int[fontNum];
         }
         index = 0;
         displayWidth = 1;

         fontList[0] = fnt;
         charWidths[0] = fontWidth;
         charHeights[0] = fontHeight;
         Descents[0] = fontMetric.getDescent();
         Ascents[0] = fontMetric.getAscent();
         step = ((float)charHeights[0] - 9.0f) / 5.0f;
         if (step < 1.0f)
             step = 1.0f;
         fsize = (float)charHeights[0];
         for (n = 1; n < fontNum; n++) {
             fsize -= step;
             getSmallerFont(n, fsize);
         }
     }

     private void getSmallerFont(int n, float fsize) {
         int i = n - 1;
         int iw = charWidths[i];
         Font fnt = fontList[i];
         FontMetrics fm = refComp.getFontMetrics(fnt);

         while (fsize >= 9.0f) {
             fsize = fsize - 1.0f;
             if (fsize < 8.0f)
                 fsize = 8.0f;
             fnt = fnt.deriveFont(fsize);
             fm = refComp.getFontMetrics(fnt);
             iw = fm.stringWidth(refStr) / 5;
             if (iw < charWidths[i])
                 break;
         }
         fontList[n] = fnt;
         charWidths[n] = iw;
         Descents[n] = fm.getDescent();
         Ascents[n] = fm.getAscent();
         i = Ascents[n] + Descents[n];
         if (i < 6)
            i = fnt.getSize();
         charHeights[n] = i;
     }

    public Font getFont(int w, int h) {
         if (fontList == null)
             update();
         if (w == displayWidth && h == displayHeight)
             return fontList[index];
         int lines = 20;
         displayWidth = w;
         displayHeight = h;
         for (index = 0; index < fontNum; index++) {
             if (w >= charWidths[index] * 70) {
                  if (h >= charHeights[index] * lines)
                        break;
             } 
             lines = lines - 2;
         }
         if (index >= fontNum)
            index = fontNum - 1;
         fontMetric = refComp.getFontMetrics(fontList[index]);
         return fontList[index];
    }

    public Font getFont(int n) {
         if (n >= fontNum)
            n = fontNum - 1;
         if (n < 0)
            n = 0;
         return fontList[n];
    }

    public Font getFont() {
         return fontList[index];
    }

    public int getFontDescent() {
         return Descents[index];
    }

    public int getFontAscent() {
         return Ascents[index];
    }

    public int getFontHeight() {
         return charHeights[index];
    }

    public int getFontWdith() {

         return charWidths[index];
    }

    public int getFontSize(int n) {
         if (n >= fontNum)
            n = fontNum - 1;
         return charHeights[n];
    }

    public void writeToFile(PrintWriter os) {
        for (int n = 0; n < fontNum; n++) {
            os.print(fontName+" "+n+" "+charWidths[n]+" "+charHeights[n]);
            os.println(" "+Ascents[n] +" "+Descents[n]);
        }
    }

    public void clone(GraphicsFont obj) {
        obj.fontList = fontList;
        obj.font = font;
        obj.fontName = fontName;
        obj.fontNum = fontNum;
        obj.fontColor = fontColor;
        obj.charWidths = charWidths;
        obj.charHeights = charHeights;
        obj.Descents = Descents;
        obj.Ascents = Ascents;
        obj.isDefault = isDefault;
        obj.scaled = scaled;
        obj.index = 0;
        obj.originHeight = originHeight;
        obj.originWidth = originWidth;
    }

    public void sendToVnmr(ButtonIF vif) {
        if (vif == null)
            return;
        for (int n = 0; n < fontNum; n++) {
           String mess = new StringBuffer().append("jFunc(")
               .append(VnmrKey.FONTARRAY)
               .append(",'").append(fontName).append("'")
               .append(",").append(n)
               .append(",").append(charWidths[n])
               .append(",").append(charHeights[n])
               .append(",").append(Ascents[n])
               .append(",").append(Descents[n])
               .append(")\n").toString();
           vif.sendToVnmr(mess);
        }
    }
}

