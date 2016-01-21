/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.*;


public class VjPlotterArea extends JPanel {
      private int type;
      private VjPlotterArea specArea;
      private VjPageAttributes attr;
      private FontMetrics fm;
      private Font myFont;
      private Color myColor;
      private static String message1 = "drawable area";
      private static String message2 = "spectrum max width";
      private static String message3 = "spectrum max height";
      private double rx, ry, rw, rh;
      public VjPlotterArea(int type) {
           this.type = type;
           setVisible(true);
           if (type == VjPrintDef.DRAW_AREA) {
               specArea = new VjPlotterArea(VjPrintDef.SPECTRUM_AREA);
               this.add(specArea);
               setLayout(null);
               setBorder(new LineBorder(Color.green));
               setBackground(Color.gray.brighter());
               myColor = Color.black;
           }
           else {
               setBorder(new LineBorder(Color.blue));
               setBackground(new Color(184, 255, 255));
               myColor = new Color(0, 160, 0);
           }
           this.rw = 1.0;
           this.rh = 1.0;
           this.rx = 0.0;
           this.ry = 0.0;
           
           myFont = new Font(Font.SERIF, Font.BOLD, 9);
           fm = getFontMetrics(myFont);

           addMouseListener(new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
              }

              public void mousePressed(MouseEvent evt) {
              }

              public void mouseReleased(MouseEvent evt) {
              }

              public void mouseEntered(MouseEvent evt) {
              }

              public void mouseExited(MouseEvent evt) {
              }
           });

           addMouseMotionListener(new MouseMotionAdapter() {
              public void mouseDragged(MouseEvent evt) {
              }

              public void mouseMoved(MouseEvent evt) {
              }
           });

      }

      public void setPageAttribute(VjPageAttributes a, boolean changeable) {
             attr = a;
             if (type == VjPrintDef.DRAW_AREA) {
                 if (attr == null)
                     setVisible(false);
                 else
                     setVisible(true);
                 if (specArea != null)
                     specArea.setPageAttribute(a, changeable);
                 updateInfo();
             }
      }

      public void setPageAttribute(VjPageAttributes a) {
             setPageAttribute(a, true);
      }
  
      public void updateInfo() {
         if (attr == null)
             return;
         if (specArea == null)
             return;
         double w = attr.drawWidth;
         double h = attr.drawHeight;
         double offset;
         if (w <= 1.0)
             w = attr.paperWidth;
         if (h <= 1.0)
             h = attr.paperHeight;
         if (attr.raster > 0) {
             offset = attr.xoffset;
             if ((offset + attr.wcmaxmax) > w) 
                 offset = w - attr.wcmaxmax;
             if (offset < 0)
                 offset = 0;
             rx = offset / w;
             offset = attr.yoffset;
             if ((offset + attr.wc2maxmax) > h) 
                 offset = h - attr.wc2maxmax;
             if (offset < 0)
                 offset = 0;
             ry = offset / h;
         }
         else {
             rx = 0;
             ry = 0;
         }
         rw = attr.wcmaxmax / w;
         rh = attr.wc2maxmax / h;
         if (rw > 1.0)
             rw = 1.0;
         if (rh > 1.0)
             rh = 1.0;
      }

      private void setSpecSize(int w, int h) {
          double dw = (double) w;
          double dh = (double) h;
          int x = (int) (dw * rx);
          int y = (int) (dh * ry);
          int iw = (int) (dw * rw);
          int ih = (int) (dh * rh);
          y = h - ih - y; 
          specArea.setBounds(x, y, iw, ih);
      }

      public void setBounds(int x, int y, int w, int h) {
         super.setBounds(x, y, w, h);
         if (specArea != null)
             setSpecSize(w, h);
      }


      private void paintMessage1(Graphics g) {
            Dimension dim = getSize();
            if (dim.height < 20 || dim.width < 20)
                return;
            int fontW = fm.stringWidth(message1);
            int x = (dim.width - fontW) / 2;
            if (x < 0)
                x = 0;
            g.setColor(myColor);
            g.drawString(message1, x, fm.getHeight()+2);
      }

      public void paint(Graphics g) {
            super.paint(g);
            if (type != VjPrintDef.DRAW_AREA)
            if (attr == null)
                return;
            if (type == VjPrintDef.DRAW_AREA) {
                paintMessage1(g);
                return;
            }
            Dimension dim = getSize();
            int fontH = fm.getHeight();
            g.setColor(myColor);
            g.setFont(myFont);
            int x;
            int y;
            int h = dim.height - fontH - 4;
            int w = dim.width;
            if (w < 10 || dim.height < 10)
                return;
            if (h < 5) {
                h = dim.height - 2;
            }
            y = h;
            if (w  > 20)
                x = 5;
            else
                x = 2;
            g.drawLine(x, y, w - x, y); 
            x = w / 2;
            g.drawLine(x, 1, x, y); 
            x = x - 6;
            if (x <= 3)
               x = 3;
            g.drawLine(x, y - h / 4, x, y); 
            x = w / 2 + 6;
            g.drawLine(x, y - h / 3, x, y); 

            g.drawLine(1, y, 4, y - 3); 
            g.drawLine(1, y, 4, y + 3); 
            x = w - 4;
            g.drawLine(x, y - 3, w, y); 
            g.drawLine(x, y + 3, w, y); 
   
            int fontW = fm.stringWidth(message2);
            x = (w - fontW) / 2;
            y = dim.height - 3;
            if (x > 0)
                g.drawString(message2, x, y);

            g.setColor(Color.black);
            x = fontH / 2 + 2;
            if (x < 4) 
               x = 4;
            g.drawLine(x - 3, 5, x, 1); 
            g.drawLine(x, 1, x + 3, 5); 
            g.drawLine(x, 1, x, 7); 
            y = dim.height - 1;
            g.drawLine(x - 3, y - 4, x, y); 
            g.drawLine(x, y, x + 3, y - 4); 
            g.drawLine(x, y, x, y - 7); 

            Graphics2D g2d = (Graphics2D) g;
            fontW = fm.stringWidth(message3);
            y = (dim.height - fontW) / 2;
            if (y < 0)
                y = 0;
            y = dim.height - y;
            g2d.translate(fontH+2, y);
            g2d.rotate(-2 * Math.PI / 4);
            g2d.drawString(message3, 0, 0);
      }
}
