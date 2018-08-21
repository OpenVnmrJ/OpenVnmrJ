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
import javax.swing.*;
import javax.swing.border.*;


public class VjPlotterPage extends JPanel {

     private VjPageAttributes attr;
     private VjPlotterArea drawArea;
     private double rx, ry, rw, rh;
     private double rx2, ry2;

     public VjPlotterPage() {
         setBackground(Color.white);
         drawArea = new VjPlotterArea(VjPrintDef.DRAW_AREA);
         this.add(drawArea);
         setLayout(null);
         setBorder(new LineBorder(Color.black));
         this.rx = 0.0;
         this.ry = 0.0;
         this.rw = 1.0;
         this.rh = 1.0;
     }

     public void setPageAttribute(VjPageAttributes a, boolean bChangeable) {
         attr = a;
         if (attr == null) {
             drawArea.setPageAttribute(a, bChangeable);
             return;
         }
         updateInfo();
         drawArea.setPageAttribute(a, bChangeable);
     }

     public void setPageAttribute(VjPageAttributes a) {
          setPageAttribute(a, true);
     }
     
     public void updateInfo() {
         if (attr == null)
             return;
         double w, h;
         w = attr.dispWidth;
         h = attr.dispHeight;
         if (w <= 0.0)
             return;
         rx = attr.leftMargin / w;
         ry = attr.topMargin / h;
         rx2 = attr.rightMargin / w;
         ry2 = attr.bottomMargin / h;
         rw = 1.0 - rx - rx2;
         rh = 1.0 - ry - ry2;
         attr.drawWidth = attr.dispWidth * rw;
         attr.drawHeight = attr.dispHeight * rh;
         drawArea.updateInfo();
     }

     private void setAreaSize(int w, int h)  {
         double dw = (double) w;
         double dh = (double) h;
         int x = (int) (dw * rx);
         int y = (int) (dh * ry);
         int iw = (int) (dw * rw);
         int ih = (int) (dh * rh);
         drawArea.setBounds(x, y, iw, ih);
     }

     public void setBounds(int x, int y, int w, int h) {
         setAreaSize(w, h); 
         super.setBounds(x, y, w, h);
     }

}
