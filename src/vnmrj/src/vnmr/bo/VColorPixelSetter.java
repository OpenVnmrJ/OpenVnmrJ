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
import javax.swing.*;
import java.awt.geom.*;
import java.awt.event.*;

import vnmr.ui.*;
// import vnmr.templates.*;


public class VColorPixelSetter extends VPlot implements VColorMapIF {
    private int numColors = 0;
    private int rgbIndex = -1;
    private int rgbTop = 0;
    private int colorIndex = -1;
    private int xoffset;
    private int yoffset;
    private int ovalW, ovalOffset;
    private int[] canXRange;
    private int[] canYRange;
    private int[][] rgbValues;
    private boolean mouseIn = false;
    private boolean mouseDrag = false;
    private boolean bLoading = false;
    private double canvasWd;
    private double canvasHt;
    private double[][] points;
    private double  xGap;
    private JPanel  plotPanel;
    private java.util.List <VColorMapIF> colorListenerList = null;

    private static Color[] COLORS = { Color.red, new Color(0, 140, 0), Color.blue }; 


    public VColorPixelSetter()
    {
        super(null, null, null);
        setLayout( new BorderLayout() );
        plotPanel = new JPanel();
        plotPanel.setLayout( new BorderLayout() );
        plotPanel.setBackground(Color.white);
        m_plot = new X2ControlPlot();
        m_plot.setBackground(Color.white);
        plotPanel.add(m_plot, BorderLayout.CENTER);
        JLabel bottomlabel = new JLabel("Normalized  Intensity", SwingConstants.CENTER);
        JLabel leftlabel = new VColorMapLabel(SwingConstants.VERTICAL, "RGB  Values");
        plotPanel.add(bottomlabel, BorderLayout.SOUTH);
        plotPanel.add(leftlabel, BorderLayout.WEST);
        add(plotPanel, BorderLayout.CENTER);
        setBorder(BorderFactory.createEtchedBorder());

        m_plot.setXFullRange(1, 64);
        m_plot.setYFullRange(0, 255);

        setColorNumber(64);
        updateScales();
    }

    public void setColorNumber(int n) {
        if (n < 2 || n == numColors)
           return;
        numColors = n;
        m_plot.setXFullRange(1, n);
        points = new double[3][n];
        rgbValues = new int[3][n];
        int c, i;
        for (c = 0; c < 3; c++) {
           double[] ys = points[c];
           double dv = 0.1 + 0.2 * c;
           for (i = 0; i < n; i++) {
             ys[i] = dv; 
           }
        }
        repaint();
    }

    public void setLoadMode(boolean b) {
        bLoading = b;
    }

    private void sendColor() {
        if (colorListenerList == null)
            return;
        if (colorIndex < 0)
            return;
        int r, g, b;

        r = (int) (255.0 - points[0][colorIndex] * 255.0);
        g = (int) (255.0 - points[1][colorIndex] * 255.0);
        b = (int) (255.0 - points[2][colorIndex] * 255.0);

        synchronized(colorListenerList) {
            Iterator<VColorMapIF> itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setOneRgb(colorIndex, r, g, b);
            }
        }
    }

    private void updateListeners() {
        if (colorListenerList == null || numColors < 2)
            return;
        if (bLoading)
            return;
        int iv[];
        double dv[];

        for (int a = 0; a < 3; a++) { 
            iv = rgbValues[a];
            dv = points[a];
            for (int i = 0; i < numColors; i++)
               iv[i] = (int) (255.0 - dv[i] * 255.0);
        }
        synchronized(colorListenerList) {
            Iterator<VColorMapIF> itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setRgbs(rgbValues[0], rgbValues[1], rgbValues[2]);
            }
        }
    }

    private void sendColorIndex() {
        if (colorIndex < 0)
            return;
        if (colorListenerList == null)
            return;
        synchronized(colorListenerList) {
            Iterator itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setSelectedIndex(colorIndex+1);
            }
        }
    }

    @Override
    public void setVisible(boolean b) {
        super.setVisible(b);
        
        if (!b || colorListenerList == null)
            return;
        updateListeners();
      /************
        Color[] colors = null;
        synchronized(colorListenerList) {
            Iterator itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                colors = l.getColorArray();
                if (colors != null)
                    break;
            }
        }
        int num = colors.length - 2;
        if  (num < 2)
            return;
        if (num > numColors)
            num = numColors; 
        num++;
        for (int i = 1; i < num; i++) {
            Color c = colors[i];
            double v = (double) (255 - c.getRed());
            points[0][i-1] = v / 255.0; 
            v = (double) (255 - c.getGreen());
            points[1][i-1] = v / 255.0; 
            v = (double) (255 - c.getBlue());
            points[2][i-1] = v / 255.0; 
        }
       ***************/
        // repaint(); 
    }


    public void updateGraph(Graphics2D gc) {
        gc.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        updateScales();

        int c, i, dy, x, y, txy;
        double dx;

        gc.setColor(COLORS[2]);
        ovalOffset = (int) (xGap / 2);
        if (ovalOffset < 2)
           ovalOffset = 2;
        else if (ovalOffset > 5)
           ovalOffset = 5;
        if (colorIndex >= 0) {
            dx = xGap * colorIndex + xoffset + xGap / 2;
            x = (int) dx + ovalOffset;
            y = (int) canvasHt + yoffset;
            gc.setColor(Color.lightGray);
            gc.drawLine(x, yoffset, x, y);
        }

        ovalW = ovalOffset * 2 + 1;
        dy = yoffset - ovalOffset;
        gc.setColor(COLORS[2]);
        double[] ys;
        for (c = 0; c < 3; c++) {
           if (c != rgbTop) {
              dx = xGap / 2 + xoffset;
              gc.setColor(COLORS[c]);
              ys = points[c];
              for (i = 0; i < numColors; i++) {
                  x = (int) dx;
                  y = dy + (int) (canvasHt * ys[i]);
                  gc.fillOval(x, y, ovalW, ovalW); 
                  dx += xGap;
              }
           }
        }
        if (rgbTop >= 0) {
           dx = xGap / 2 + xoffset;
           gc.setColor(COLORS[rgbTop]);
           ys = points[rgbTop];
           for (i = 0; i < numColors; i++) {
               x = (int) dx;
               y = dy + (int) (canvasHt * ys[i]);
               gc.fillOval(x, y, ovalW, ovalW); 
               dx += xGap;
           }
        }

        if (rgbIndex < 0 || rgbIndex > 2)
           return;
        gc.setColor(Color.orange);
        dx = xGap / 2 + xoffset + xGap * colorIndex;
        x = (int) dx;
        y = dy + (int) (canvasHt * points[rgbIndex][colorIndex]);
        gc.fillOval(x, y, ovalW, ovalW); 

        dx = 255.0 - points[rgbIndex][colorIndex] * 255.0;
        c = (int) dx;
        x = x + ovalW + 8;
        if (y < 18)
            y = 18;
        txy = y + ovalW + 2;
        if (x > (canXRange[1] - 40)) {
             x = canXRange[1] - 40;
             txy = txy + 30;
             if (y > (canYRange[1] - 24))
                txy = canYRange[1] - 24;
        }
        if (txy > canYRange[1])
             txy = canYRange[1];
        gc.setColor(COLORS[rgbIndex]);
        i = colorIndex + 1;
        String mess = "("+i+", "+c+")";
        gc.drawString(mess, x, txy);
    }

    private void clearSelection() {
        boolean bRepaint = false;

        if ((colorIndex >= 0) && (rgbIndex >= 0))
           bRepaint = true;
        colorIndex = -1;
        rgbIndex = -1;
        if (bRepaint)
           repaint(); 
    }

    private void updateSelection(int px, int py) {
        int c, i, p, x, x0, y;
        double dx;

        c = -1;
        p = -1;
        dx = xGap / 2;
        x0 = px - xoffset - (int) dx;
        dx = (double) x0;
        if (dx >= 0) {
           c = (int) (dx / xGap); 
           if (c >= numColors)
               c = -1;
           if (c >= 0) {
               x = (int) (xGap * c);
               if ((x0 < x) || (x0 > (x + ovalW)))
                   c = -1;
           }
        }
        if (c >= 0) {
           if (rgbTop >= 0) {
               y = yoffset - ovalOffset + (int)(canvasHt * points[rgbTop][c]);
               if ((py >= y) && (py <= (y + ovalW)))
                   p = rgbTop;
           }
           if (p < 0) {
              for (i = 0; i < 3; i++) {
                 y = yoffset - ovalOffset + (int) (canvasHt * points[i][c]);
                 if ((py >= y) && (py <= (y + ovalW))) {
                      p = i;
                      break;
                 }
              }
           }
        }
        if ((c == colorIndex) && (p == rgbIndex))
            return;
        colorIndex = c;
        rgbIndex = p;
        sendColorIndex();
        
        repaint();
    }

    private void moveKnot(int px, int py) {
        if (colorIndex < 0 || rgbIndex < 0)
            return;
        boolean bRepaint = false;
        double y0, y1;

        rgbTop = rgbIndex;
        y1 = (double) (py - yoffset);
        if (y1 < 0)
            y1 = 0;
        else if (y1 > canvasHt)
            y1 = canvasHt;

        y0 = points[rgbIndex][colorIndex];
        y1 = y1 / canvasHt;
        
        if (y0 != y1) {
           bRepaint = true;
           points[rgbIndex][colorIndex] = y1;
        }
        if (bRepaint) {
           repaint();
           sendColor();
        }
    }

    public void updateScales() {
        canXRange = m_plot.getXPixelRange();
        canYRange = m_plot.getYCanvasRange();
        xoffset = canXRange[0];
        yoffset = canYRange[0];
        canvasWd = (double) (canXRange[1] - xoffset);
        canvasHt = (double) (canYRange[1] - yoffset);
        xGap = canvasWd / (numColors + 1);
    }

    public void setLookupRgbs(int[] src) {
        int num = src.length;
        if (num < 2)
            return;
        int index0 = 0;

        if (rgbValues == null)
            rgbValues = new int[3][numColors];

        if (num > numColors) { // from lookup table
            index0 = 1;
            num = numColors + 1;
        }

        int s, dest;
        double dv;
        int[] r = rgbValues[0];
        int[] g = rgbValues[1];
        int[] b = rgbValues[2];
        double[] rPnts = points[0];
        double[] gPnts = points[1];
        double[] bPnts = points[2];
  
        dest = 0;
        for (int i = index0; i < num; i++) {
            s = src[i];
            r[dest] = (s >> 16) & 0xff; 
            dv = (double) (255 - r[dest]);
            rPnts[dest] = dv / 255.0;

            g[dest] = (s >> 8) & 0xff; 
            dv = (double) (255 - g[dest]);
            gPnts[dest] = dv / 255.0;

            b[dest] = s & 0xff;
            dv = (double) (255 - b[dest]);
            bPnts[dest] = dv / 255.0;
            dest++;
        }
    }

    // VColorMapIF

    public void setRgbs(int index, int[] r, int[] g, int[] b) {
        if (r == null || g == null || b == null)
            return;
        int num = r.length;
        if (num < 2)
            return;
        if (rgbValues == null)
            rgbValues = new int[3][numColors];
        if (num > numColors)
            num = numColors;

        for (int i = index; i < num; i++) {
            double v = (double) (255 - r[i]);
            points[0][i] = v / 255.0;
            v = (double) (255 - g[i]);
            points[1][i] = v / 255.0;
            v = (double) (255 - b[i]);
            points[2][i] = v / 255.0;
        }
        updateListeners();
        repaint();
    }

    public void setRgbs(int[] r, int[] g, int[] b) {
        setRgbs(0, r, g, b);
    }

    public void setRgbs(int index, byte[] r, byte[] g, byte[] b, byte[] a) {
    }

    public void setRgbs(byte[] r, byte[] g, byte[] b, byte[] a) {
    }

    public void setOneRgb(int index, int r, int g, int b) {
    }

    public int[] getRgbs() {
        return null;
    }

    public void setColorEditable(boolean b) {
    }

    public void setColor(int n, Color c) {
    }

    public Color[] getColorArray() {
        return null;
    }

    public void setSelectedIndex(int i) {
    }

    public void addColorEventListener(VColorMapIF l) {
        if (colorListenerList == null)
            colorListenerList = Collections.synchronizedList(new LinkedList<VColorMapIF>());
        if (!colorListenerList.contains(l)) {
            colorListenerList.add(l);
        }
    }

    public void clearColorEventListener() {
        if (colorListenerList != null)
            colorListenerList.clear();
    }

    // end of VColorMapIF


    private class X2ControlPlot extends VPlot.CursedPlot {
        protected MouseMotionListener ptMouseMotionListener = null;

        public X2ControlPlot() {
            super(false);      // Don't install base mouse listeners
            MouseInputListener mouseListener = new MouseInputListener();
            addMouseMotionListener(mouseListener);
            addMouseListener(mouseListener);
        }

        @Override
        protected void _drawCanvas(Graphics g, int x, int y, int wd, int ht) {
            g.setColor(Color.black);
            g.drawRect(x, y, wd, ht);
        }

        public void paintComponent(Graphics g) {
            m_bLeftCursorOn = m_bRightCursorOn = false;
            super.paintComponent(g);  //paint background
        }

        protected synchronized void _drawPlot(Graphics graphics,
                                              boolean clearfirst,
                                              Rectangle drawRectangle) {
            super._drawPlot(graphics, clearfirst, drawRectangle);
            updateGraph((Graphics2D)graphics);
        }
            
       public void setCursor(int nCursor, String strValue) {
        }

        public void setCursor(int nCursor, double value) {
        }

        class MouseInputListener extends VPlot.CursedPlot.MouseInputListener {

            private boolean otherCursorChanged;
            private VsFunction vsFunc = null;

            public void mouseMoved(MouseEvent e) {
                mouseIn = true;
                updateSelection(e.getX(), e.getY());
            }

            public void mouseDragged(MouseEvent e){
                mouseDrag = true;
                moveKnot(e.getX(), e.getY());
            }

            public void mouseReleased(MouseEvent e){
                mouseDrag = false;
                if (!mouseIn) {
                    clearSelection();
                } else {
                    updateSelection(e.getX(), e.getY());
                }
            }

            public void mousePressed(MouseEvent e) {
            }

            public void mouseClicked(MouseEvent e){}

            public void mouseExited(MouseEvent e) {
                mouseIn = false;
                if (!mouseDrag)
                    clearSelection();
            }

            public void mouseEntered(MouseEvent e) {
                mouseIn = true;
            }

            protected void mousePosition(MouseEvent e, int nMouseAction) {
                Point2D.Double point;
                if (SwingUtilities.isLeftMouseButton(e)) {
                    movePointToMouse(e, leftPoint);
                    m_strLCValue = Double.toString(leftPoint.x);
                } else {
                    return;
                }
                repaint();
            }

            public void sendCursorCmds(MouseEvent e, int nMouseAction) {
            }
                    
        }
    }
}
