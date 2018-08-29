/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import java.util.*;
import java.io.*;
import javax.swing.*;
import javax.swing.event.MouseInputAdapter;
import java.awt.geom.*;
import java.awt.event.*;
import java.beans.*;
import java.awt.RenderingHints.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.templates.*;



public class VColorControlLine {


    private static int NUM_KNOT = 3;
    private int xoffset, xoffset2;
    private int yoffset, yoffset2;
    private int selectedKnot = -1;
    private int numColors = 0;
    private int idNum;
    private int ovalSize = 7;
    private int ovalOffset = 3;
    private int numKnots = 0;
    private double canvasWd;
    private double canvasHt;
    private double xGap = 0;
    private double[] knotXs;
    private double[] knotYs;
    private int[] values;
    private int[] graphX;
    private int[] graphY;
    private int[] canXRange;
    private int[] canYRange;
    private int[] knotIndexs;   // color ids
    private int[] knotValues;   // color value
    private Color fgColor = Color.red;
    protected VPlot.CursedPlot m_plot;

    public VColorControlLine(VPlot.CursedPlot plot, int id, int numColors)
    {
        this.numColors = numColors;
        this.idNum = id;
        m_plot = plot;
        if (numColors < 2) 
            this.numColors = 64;
        values = new int[this.numColors];
        setKnotNumber(3);

        updateScales();
        // updateValues();
    }

    public VColorControlLine(VPlot.CursedPlot plot)
    {
         this(plot, 0, 64);
    }

    public void setFgColor(Color c) {
        fgColor = c;
    }

    public void setKnotNumber(int c) {
        if (c < 3)
            c = 3;
        if (numKnots == c)
            return;
  
        double dv;
        int   i, k;
        double[] newKnots = new double[c];
        newKnots[0] = 0;
        newKnots[c-1] = 1;
        if (knotXs != null) {
           newKnots[0] = knotXs[0];
           newKnots[c-1] = knotXs[numKnots - 1];
           if (c < numKnots) {
              k = 2;
              newKnots[1] = knotXs[1];
              dv = (knotXs[numKnots - 1] - knotXs[1]) / (double) (c-2);
           }
           else {
              k = 1;
              dv = (knotXs[numKnots - 1] - knotXs[0]) / (double) (c-1);
           }
        }
        else {
           dv = 1 / (double) (c-1);
           if (c == 3) {
               newKnots[1] = 0.5;
               k = c + 1;
           }
           else {
               k = 1;
           }
        }
        for (i = k; i < (c-1); i++)
           newKnots[i] = newKnots[i-1] + dv;
        knotXs = newKnots;

        newKnots = new double[c];
        newKnots[0] = 1;
        newKnots[c-1] = 0;
        if (knotYs != null) {
           newKnots[0] = knotYs[0];
           newKnots[1] = knotYs[1];
           newKnots[c-1] = knotYs[numKnots - 1];
           dv = (knotYs[numKnots - 1] - knotYs[1]) / (double) (c-2);
           k = 2;
        }
        else {
           dv = (-1) / (double) (c-1);
           if (c == 3) {
               newKnots[1] = 0.5;
               if (idNum == 1) { // green
                   newKnots[0] = 1;
                   newKnots[1] = 0;
                   newKnots[2] = 1;
               }
               else if (idNum == 2) { // blue
                   newKnots[0] = 0;
                   newKnots[2] = 1;
               }
               k = c + 1;
           }
           else {
               k = 1;
           }
        }
        for (i = k; i < (c-1); i++)
           newKnots[i] = newKnots[i-1] + dv;
        knotYs = newKnots;

        graphX = new int[c];
        graphY = new int[c];
        knotIndexs = new int[c];
        knotValues = new int[c];
        
        numKnots = c;
        updateValues();
    }

    public int getKnotNumber() {
        return numKnots;
    }

    public int[] getKnotArray() {
        return knotValues;
    }
    
    public void setColorNumber(int i) {
        if (i < 2)
            return;
        if (values != null && values.length == i)
            return;
        values = new int[i];
        numColors = i;
        if (numKnots > 1) {
           knotXs[0] = 0;
           knotXs[numKnots-1] = 1;
           knotIndexs[0] = 0;
           knotIndexs[numKnots-1] = i - 1;
        }
        updateValues();
    }

    public int[] getValues() {
        return values;
    }

    public int getId() {
        return idNum;
    }
    
    public void setValues(int index0, int num, byte[] v) {

    }

    public void restoreKnotXs() {
        int n = knotIndexs.length;
        double dv = (double) numColors;


        if (n > numKnots)
            n = numKnots;
        for (int i = 0; i < n; i++)
            knotXs[i] = ((double) knotIndexs[i]) / dv;
        if (knotIndexs[n-1] >= (numColors-1))
            knotXs[n-1] = 1.0;
    }

    public void restoreKnotYs() {
    }

    public void updateValues() {
        int x0, x1, xs;
        double v0, v1;
        int i, k;
        double dv, vx;

        v0 = 0;
        v1 = 0;
        for (i = 0; i < (numKnots- 1); i++) {
            x0 = (int) (knotXs[i] * numColors);
            x1 = (int) (knotXs[i+1] * numColors);
            v0 = 255.0 - knotYs[i] * 255.0;
            v1 = 255.0 - knotYs[i+1] * 255.0;
            if (x0 >= numColors)
                x0 = numColors - 1;
            if (x1 >= numColors)
                x1 = numColors - 1;
            xs = x1 - x0;
            if (xs < 1)
                xs = 1;
            dv = (v1 - v0) / (double) xs;
            vx = v0;
            for (k = x0; k <= x1; k++) {
                values[k] = (int) vx;
                vx += dv;
            }
        }
        x0 = (int) (knotXs[0] * numColors);
        for (i = 0; i < x0; i++)
            values[i] = 0;
        k = (int) (knotXs[numKnots- 1] * numColors) + 1;
        for (i = k; i < numColors; i++)
            values[i] = 0;
        for (i = 0; i < numKnots; i++)
            knotIndexs[i] = (int) (knotXs[i] * numColors);
        if (knotIndexs[numKnots -1] >= numColors)
            knotIndexs[numKnots -1] = numColors - 1;
    }

    public void updateGraph(Graphics2D gc) {
        int i;
        int x, y, maxX, maxY;

        updateScales();
 
        maxX = canXRange[1] - ovalSize + 1;
        maxY = yoffset2 - ovalSize + 1;
        for (i = 0; i < numKnots; i++) {
           graphX[i] = xoffset + (int) (knotXs[i] * canvasWd);
           graphY[i] = yoffset + (int) (knotYs[i] * canvasHt);
        }

        gc.setColor(fgColor);
        gc.drawPolyline(graphX, graphY, numKnots);

        // Draw knots
        for (i = 0; i< numKnots; i++) {
            if (i != selectedKnot)
                gc.setColor(fgColor);
            else
                gc.setColor(Color.orange);
            x = graphX[i] - ovalOffset;
            if (x < xoffset)
                x = xoffset;
            else if (x > maxX)
                x = maxX;
            y = graphY[i] - ovalOffset;
            if (y > maxY)
                y = maxY;
            gc.fillOval(x, y, ovalSize, ovalSize);
        }
        if (selectedKnot >= 0) {
            x = graphX[selectedKnot] + 15;
            y = graphY[selectedKnot] + 20;
            knotIndexs[selectedKnot] = (int) (knotXs[selectedKnot] * numColors);
            if (knotIndexs[selectedKnot] >= numColors)
                knotIndexs[selectedKnot] = numColors - 1;
            knotValues[selectedKnot] = (int) (255.0 - knotYs[selectedKnot] * 255.0);
            if (x > (canXRange[1] - 40)) {
                 x = canXRange[1] - 40; 
                 y = y + 10;
                 if (y > (yoffset2 - 20))
                    y = graphY[selectedKnot] - 10;
            }
            if (y > yoffset2)
                 y = yoffset2;
            else if (y < 20)
                 y = 20;
            i = knotIndexs[selectedKnot] + 1;
            String mess = "("+i+", "+knotValues[selectedKnot]+")";
            gc.setColor(fgColor);
            gc.drawString(mess, x, y);
        }
    }

    public int getSelectedKnot() {
        return selectedKnot;
    }

    public int setSelectedKnot(int n) {
        int changed = 0;
        if (selectedKnot != n)
            changed = 1;
        selectedKnot = n;
        return changed;
    }
 
    public int updateSelection(int x, int y) {
        int prevKnot = selectedKnot;

        // Are we near an existing point?
        selectedKnot = -1;	// Index of selected knot
        for (int i=0; i< numKnots; i++) {
            int x1 = graphX[i] - ovalOffset;
            int x2 = graphX[i] + ovalSize + 2;
            int y1, y2;
            if (x >= x1 && x <= x2) {
                y1 = graphY[i] - ovalOffset - 1;
                y2 = graphY[i] + ovalSize + 2;
                if  (y >= y1 && y <= y2) {
                    selectedKnot = i;
                    break;
                }
            }
        }
        if (selectedKnot >= 0) {
            if (selectedKnot != prevKnot)
               return 2;
            return 1;
        }
        if (selectedKnot != prevKnot)
            return (-1);
        return 0;
    }

    public int moveKnot(int x, int y) {
        if (selectedKnot < 0)
            return 0;
        if (x < xoffset) { x = xoffset; }
        if (x > xoffset2) { x = xoffset2; }
        if (y < yoffset) { y = yoffset; }
        if (y > yoffset2) { y = yoffset2; }

        int x2;
        double dx, dy;

        if (selectedKnot < (numKnots-1)) {
           dx = xGap * knotIndexs[selectedKnot + 1];
           x2 = (int)dx - ovalOffset + xoffset;
           if (x >= x2)
               x = x2 - 1;
        }
        if (selectedKnot > 0) {
           dx = xGap * (knotIndexs[selectedKnot - 1] + 1);
           x2 = (int)dx + ovalOffset + xoffset;
           if (x < x2)
               x = x2;
        }
        
        dx = (double) (x - xoffset);
        dy = (double) (y - yoffset);
        if (dx < 0.0)
            dx = 0.0;
        knotXs[selectedKnot] = dx / canvasWd; 
        knotYs[selectedKnot] = dy / canvasHt; 
        int c = (int) (knotXs[selectedKnot] * numColors);
        int r = (int) (255.0 - knotYs[selectedKnot] * 255.0);
        if (c > numColors)
            c = numColors;
        if (c != knotIndexs[selectedKnot] || r != knotValues[selectedKnot])
        {
            updateValues();
            return 2;
        }
        return 1;
    }


    public void updateScales() {
        canXRange = m_plot.getXPixelRange();
        canYRange = m_plot.getYCanvasRange();
        canvasWd = (double) (canXRange[1] - canXRange[0]);
        canvasHt = (double) (canYRange[1] - canYRange[0]);
        xGap = canvasWd / (double) numColors;
        if (xGap >= 7.0) {
           ovalSize = 7;
           ovalOffset = 3;
        }
        else {
           ovalSize = 5;
           ovalOffset = 2;
        }
        xoffset = canXRange[0];
        xoffset2 = canXRange[1] - 1;
        yoffset = canYRange[0];
        yoffset2 = canYRange[1];
    }

    public int[] getKnotIndexs() {
        return knotIndexs;
    }

    public double[] getKnotValues() {
        return knotYs;
    }

    public void setKnotValues(int[] rgbs) {
        int i, k, len, offset, num, v;
        double dv;

        len = rgbs.length;
        num = knotIndexs.length;

        if (len < 3 || num < 2)
            return;

        offset = 0;
        if (rgbs.length > numColors) // from lookup table
            offset = 1;
        for (i = 0; i < num; i++) {
            k = knotIndexs[i] + offset;
            if (k >= len)
                break;
            if (idNum == 0) // red
                v = (rgbs[k] >> 16) & 0xff;
            else if (idNum == 1)
                v = (rgbs[k] >> 8) & 0xff;
            else
                v = rgbs[k] & 0xff;
            dv = 1.0 - ((double) v) / 255.0 - 0.00005;
            if (dv < 0.0)
               dv = 0.0; 
            else if (dv > 1.0)
               dv = 1.0; 
            knotYs[i] = dv; 
        }
        updateValues();
    }
}
