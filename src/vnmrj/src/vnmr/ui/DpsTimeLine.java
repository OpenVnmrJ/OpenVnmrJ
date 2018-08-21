/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import javax.swing.*;

public class DpsTimeLine extends JPanel {

    private double xUnit;
    private double timeUnit;
    private double ppt;
    private Font font;
    private int fontAscent;
    private int tickShort, tickHeight;
    private int tickY;
    private FontMetrics fm;

    public DpsTimeLine() {
        init();
    }

    private void init() {
        font = new Font("Dialog", Font.PLAIN, 12);
        fm = getFontMetrics(font);
        fontAscent = fm.getAscent();
        tickShort = 2;
        tickHeight = 7;
        tickY = tickHeight + fontAscent + 2;
        DpsUtil.setTimeLineHeight(tickY + fm.getDescent() + 2);
        DpsUtil.setTimeLinePanel(this);
    }

    public double getTickUnit() {
         return xUnit * 5.0;
    }

    public void setTimeScale(double dv) {
         if (ppt == dv)
             return;
         if (dv <= 0.0)
             return;
         ppt = dv;
         timeUnit = 1.0; // 1 usec
         double seqTime = DpsUtil.getSeqTime(); 

         xUnit = timeUnit * ppt;
         while (xUnit < 9.0) {
             if (xUnit < 0.001)
                 timeUnit = timeUnit * 1000.0;
             else if (xUnit < 0.01)
                 timeUnit = timeUnit * 100.0;
             else
                 timeUnit = timeUnit * 10.0;
             xUnit = ppt * timeUnit;
         }
         while (xUnit > 16) {
             timeUnit = timeUnit / 2.0;
             xUnit = ppt * timeUnit;
         }
    }

    public void setTimeScale() {
         double dv = DpsUtil.getTimeScale();
         setTimeScale(dv);
    }

    public void paint(Graphics g) {
         super.paint(g);
         Rectangle r = g.getClipBounds();
         Dimension dim = getSize();
         int x0, x, x2, index, w;
         int y1 = tickShort + 1;
         int y2 = tickHeight + 1;
         double px;

         g.setColor(Color.blue);
         g.drawLine(0, 1, dim.width - 6, 1);
         if (ppt <= 0.0)
             return;
         x2 = r.x + r.width;
         px = (double) r.x;
         px = px / xUnit;
         index = ((int) px) % 5;
         px = xUnit * (double) index;
         x = (int) px;
         while (x <= x2) {
             if (index == 0)
                 g.drawLine(x, 1, x, y2);
             else
                 g.drawLine(x, 1, x, y1);
             px += xUnit;
             x = (int) px;
             index++;
             if (index >= 5)
                index = 0;
         }
         int tickTime = (int) (timeUnit * 5.0);
         double tickUnit = xUnit * 5.0;
         y1 =  (int) ((double) r.x / tickUnit) - 1;
         if (y1 < 0)
              y1 = 0;
         y2 =  (int) ((double) x2 / tickUnit);
         x0 = 0;
         if (y1 == 0) {
              g.drawString("0", 0, tickY);
              y1 = 1;
              x0 = 12;
         }
         for (int n = y1; n < y2; n++) {
              String str = Integer.toString(tickTime * n);
              w = fm.stringWidth(str);
              x = (int) (tickUnit * (double)n);
              x = x - w / 2;
              if (x > x0) {
                  g.drawString(str, x, tickY);
                  x0 = x + w + 20;
              }
         }
    }

    public void draw(Graphics2D g, Rectangle clip) {
         g.setFont(font);

         g.setColor(Color.black);
    }
}

