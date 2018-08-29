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
import java.io.*;
import java.util.*;
import javax.swing.*;

import vnmr.util.*;

public class SpecPanel extends JPanel {

    private int   specSize = 0;
    private int   capacity = 0;
    private int   specWidth = 0;
    private boolean  bAA = true;  // anti-aliasing
    private boolean  bNoAA = false;

    private int[] xPoints;
    private int[] ys;
    private int[] yPoints;

    public SpecPanel() {
         super();
    }

    public void load(String name) {
        int i, num;
        double dx, dv;
        String line, data, value;
        String fpath = FileUtil.openPath("USER/spec.dat");
        BufferedReader fin = null;
        Dimension dim = getSize();

        specSize = 0;
        specWidth = 0;
        if (fpath == null) {
            return;
        }
        bAA = DisplayOptions.isAAEnabled();
        if (bAA)
            bNoAA = false;
        if (name.equals("spec2")) {
            bAA = true;
            bNoAA = true;
        }
        if (!bAA)
            bNoAA = true;

        i = 0;
        dx = 10.0;
        dv = 0.0;
        try {
            fin = new BufferedReader(new FileReader(fpath));
            while ((line = fin.readLine()) != null) {
                StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (data.startsWith("#")) {
                    if (!tok.hasMoreTokens())
                        continue;
                    value = tok.nextToken();
                    num = Integer.parseInt(value);
                    if (data.equalsIgnoreCase("#size")) {
                        specSize = num;
                        if (num > capacity) {
                            xPoints = new int[num + 3];
                            yPoints = new int[num + 3];
                            ys = new int[num + 3];
                            capacity = num;
                            i = 0;
                        }
                        if (specWidth < 1)
                            specWidth = specSize;
                    }
                    else if (data.equalsIgnoreCase("#width")) {
                        specWidth = num;
                        dx = (double) (dim.width - num) / 2.0;
                        if (dx < 1.0)
                            dx = 1.0; 
                    }
                    if (specSize > 1 && specWidth > 1)
                        dv = (double) specWidth / (double) specSize; 
                }
                else {
                    if (i > capacity)
                        break;
                    while (true) {
                        ys[i] = Integer.parseInt(data);
                        xPoints[i] = (int) dx;
                        i++;
                        dx = dx + dv;
                        if (!tok.hasMoreTokens())
                             break;
                        data = tok.nextToken();
                    }
                }
            }
        }
        catch (Exception e) { }
        finally {
            try {
               if (fin != null)
                   fin.close();
               fin = null;
            }
            catch (Exception e2) {}
        }
        if (i > 2) {
            specSize = i;
            repaint();
        }
    }

    public void setVisible(boolean b) {

         super.setVisible(b);
         if (!b) {
            xPoints = null;
            yPoints = null;
            ys = null;
            capacity = 0;
            specSize = 0;
         }
    }

    private void adjustData(int d) {

         Dimension dim = getSize();
         int h = dim.height - d;
         int v;

         for (int i = 0; i < specSize; i++) {
             v = h - ys[i];
             if (v < 1)
                v = 1;
             yPoints[i] = v;
         }
    }

    public void paint(Graphics g) {
         super.paint(g);
         if (specSize < 2) {
              return;
         }
         int gap = 10;
         Graphics2D  g2d = (Graphics2D) g;
         g2d.setColor(Color.blue);
         if (bNoAA) {
             adjustData(gap);
             g2d.drawPolyline(xPoints, yPoints, specSize);
             gap += 40;
         }
         if (bAA) {
             adjustData(gap);
             g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
             g2d.drawPolyline(xPoints, yPoints, specSize);
         }
    }
}

