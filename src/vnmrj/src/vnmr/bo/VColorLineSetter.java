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

import vnmr.ui.*;
import vnmr.util.*;


public class VColorLineSetter extends VPlot implements VColorMapIF {
    private VColorControlLine curCtrl;
    private VColorControlLine[] controls;
    private int numColors = 64;
    private String cltFile = null;
    private String cltFileExpr = null;
    private boolean mouseDown = false;
    private boolean mouseIn = false;
    private boolean bLoading = false;
    private JPanel ctrlPanel;
    private JPanel mainPanel;
    private JPanel graphPanel;
    private JPanel pixelPanel;
    private JPanel chooserPanel;
    private JPanel plotPanel;
    private VColorPixelSetter pixelSetter;
    private JComboBox knotChooser;
    private java.util.List <VColorMapIF> colorListenerList = null;

    private static int NUM_CTRL = 3;
    private static Color[] COLORS = { Color.red, new Color(0, 140, 0), Color.blue }; 
    private static String[] knotNameList = { "3","4","5","7","9","11","13","15",
                                          "17","19","32" };
    private static int[] knotValueList = { 3,4,5,7,9,11,13,15,17,19,32 };


    public VColorLineSetter()
    {
        super(null, null, null);
        setLayout( new BorderLayout() );
        plotPanel = new JPanel();
        plotPanel.setLayout( new BorderLayout() );
        plotPanel.setBackground(Color.white);
        m_plot = new XCursedPlot();
        m_plot.setBackground(Color.white);
        plotPanel.add(m_plot, BorderLayout.CENTER);
        JLabel bottomlabel = new JLabel("Normalized  Intensity", SwingConstants.CENTER);
        JLabel leftlabel = new VColorMapLabel(SwingConstants.VERTICAL, "RGB  Values");
        plotPanel.add(bottomlabel, BorderLayout.SOUTH);
        plotPanel.add(leftlabel, BorderLayout.WEST);
        add(plotPanel, BorderLayout.CENTER);

        setBorder(BorderFactory.createEtchedBorder());
        numColors = 64;
        m_plot.setXFullRange(1, numColors);
        m_plot.setYFullRange(0, 255);

        chooserPanel = new JPanel();
        // add(chooserPanel, BorderLayout.SOUTH);

        controls = new VColorControlLine[NUM_CTRL];
        for (int i = 0; i < NUM_CTRL; i++) {
           controls[i] = new VColorControlLine(m_plot, i, numColors);
           controls[i].setFgColor(COLORS[i]);
        }
        curCtrl = controls[0];
        String str = Util.getLabel("_Number_of_Knots", "Number of Knots")+": ";
        JLabel lb = new JLabel(str);
        chooserPanel.add(lb);
        lb.setLocation(2, 2);
        knotChooser = new JComboBox();
        for (int i = 0; i < knotNameList.length; i++)
           knotChooser.addItem(knotNameList[i]);
        chooserPanel.add(knotChooser);
        knotChooser.addActionListener(new KnotListener());
    }

    public JComponent getKnotChooserPanel() {
        return chooserPanel;
    }

    public int getKnotNumber() {
        return controls[0].getKnotNumber();
    }

    private int getChooserNumber() {
        String s = knotChooser.getSelectedItem().toString();
        int num = 3;
        try {
            num = Integer.parseInt(s);
        }
        catch (NumberFormatException er) { }
        return num;
    }

    public void setKnotNumber(int n, boolean bFromCb) {
        if (n < 3 || n > 64)
            return;
        int i;
        int knots = getKnotNumber();
        if (knots != n) {
           for (i = 0; i < NUM_CTRL; i++) {
              controls[i].setKnotNumber(n);
              controls[i].restoreKnotXs();
           }
           updateListeners();
        }

        if (!bFromCb) {
            int c = getChooserNumber();
            if (c != n)
               knotChooser.setSelectedItem(Integer.toString(n));
        } 

        if (knots != n)
            repaint();
    }

    public void setKnotNumber(int n) {
        setKnotNumber(n, false);
    }

    public void setColorNumber(int n) {
        if (n < 3 || n == numColors)
           return;
        int i;
        numColors = n;
        m_plot.setXFullRange(1, n);
        for (i = 0; i < NUM_CTRL; i++)
           controls[i].setColorNumber(n);
        int knots = getKnotNumber();
        if (knots < numColors) {
           for (i = 0; i < NUM_CTRL; i++)
               controls[i].restoreKnotXs();
           updateListeners();
           return;
        }
        for (i = 0; i < knotValueList.length; i++) {
           if (knotValueList[i] >= numColors) {
              i--;
              break;
           }
        }
        if (i >= 0)
           setKnotNumber(knotValueList[i]);
    }

    public void setLoadMode(boolean b) {
        bLoading = b;
    }

    public void setRedValues(int firstIndex, int num, byte[] v) {
        controls[0].setValues(firstIndex, num, v);
    }

    public void setGreenValues(int firstIndex,int num, byte[] v) {
        controls[1].setValues(firstIndex, num, v);
    }

    public void setBlueValues(int firstIndex, int num, byte[] v) {
        controls[2].setValues(firstIndex, num, v);
    }

    public int[] getRedValues() {
        return controls[0].getValues();
    }

    public int[] getGreenValues() {
        return controls[1].getValues();
    }

    public int[] getBlueValues() {
        return controls[2].getValues();
    }

    private void updateListeners() {
        if (colorListenerList == null)
            return;
        if (bLoading || !isVisible())
            return;

        int[] r = getRedValues();
        int[] g = getGreenValues();
        int[] b = getBlueValues();

        synchronized(colorListenerList) {
            Iterator itr = colorListenerList.iterator();
            while (itr.hasNext()) {
                VColorMapIF l = (VColorMapIF)itr.next();
                l.setRgbs(r, g, b);
            }
        }
    }


    public void updateGraph(Graphics2D gc) {
        gc.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        for (int i = 0; i < NUM_CTRL; i++) {
           if (controls[i] != curCtrl)
               controls[i].updateGraph(gc);
        }
        if (curCtrl != null)
           curCtrl.updateGraph(gc);
    }


    private void updateSelection(int x, int y) {
        int i;
        VColorControlLine newCtrl = null;
        int ret = -1;
        boolean changed = false;
        
        if (curCtrl != null) {
            ret = curCtrl.updateSelection(x, y);
            if (ret < 0)
                changed = true;
        }
        if (ret < 1) {
            for (i = NUM_CTRL - 1; i >= 0; i--) {
                if (controls[i] != curCtrl) {
                    ret = controls[i].updateSelection(x, y);
                    if (ret > 0) {
                        newCtrl = controls[i];
                        break;
                    }
                    if (ret < 0)
                        changed = true;
                }
            }
        }
        if (newCtrl != null)
            curCtrl = newCtrl;
        for (i = 0; i < NUM_CTRL; i++) {
            if (controls[i] != curCtrl) {
               if (controls[i].setSelectedKnot(-1) > 0)
                  changed = true;
            }
        }
        
        if (ret > 1 || changed)
            repaint();
    }

    private void moveKnot(int x, int y) {
        if (curCtrl == null)
            return;
        int s = curCtrl.moveKnot(x, y);
        if (s > 0) {
            repaint();
            if (s > 1) {
                updateListeners();
            }
        }
    }

    public void setLookupRgbs(int[] rgbs) {
        int num = rgbs.length;
        if (num < 2)
            return;
        for (int i = 0; i < 3; i++)
            controls[i].setKnotValues(rgbs);
    }

    // VColorMapIF

    public void setRgbs(int index, int[] r, int[] g, int[] b) {
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
            updateListeners();
        }
    }

    public void clearColorEventListener() {
        if (colorListenerList != null)
            colorListenerList.clear();
    }

    @Override
    public void setVisible(boolean b) {
        super.setVisible(b);

        chooserPanel.setVisible(b);
        if (!b || colorListenerList == null)
            return;
        updateListeners();
    }

    public void saveKnotsValues(PrintWriter outs) {
        int i, k, n;
        double dv;
        int num = getKnotNumber();
        outs.println("# Number of Knots");
        outs.println(VColorMapFile.KNOTS_SIZE+"  "+Integer.toString(num));
        for (i = 0; i < NUM_CTRL; i++) {
            controls[i].updateValues();
            int[] xs = controls[i].getKnotIndexs();
            double[] ys = controls[i].getKnotValues();
            int len = xs.length;
            if (i == 0) 
               outs.print(VColorMapFile.RED_KNOTS_INDEX);
            else if (i == 1) 
               outs.print(VColorMapFile.GREEN_KNOTS_INDEX);
            else if (i == 2) 
               outs.print(VColorMapFile.BLUE_KNOTS_INDEX);
            if (len > num)
                len = num;
            for (k = 0; k < len; k++)
                outs.print(" "+Integer.toString(xs[k]));
            outs.println(" ");
        }
    }

    private void setKnotIndexs(int id, int num,StringTokenizer tok) {
         if (id < 0 || id > 2)
             return;
         if (num < 3 || num > 64)
             return;
         try {
             controls[id].setKnotNumber(num);
             int[] xs = controls[id].getKnotIndexs();
             int len = xs.length;
             if (len > num)
                  len = num;
             for (int i = 0; i < len; i++) {
                 if (!tok.hasMoreTokens())
                     break;
                 String data = tok.nextToken();
                 xs[i] = Integer.parseInt(data);
             }
             controls[id].restoreKnotXs();
         }
         catch (NumberFormatException ex) {
         }
    }

    public void loadKnotValues(String path) {
       String line, data;
       int  id, knots;
       BufferedReader ins = null;

       if (path != null)
          path = FileUtil.openPath(path);
       if (path == null) {
          setKnotNumber(3);
          return;
       }

       knots = 0;
       try {
          ins = new BufferedReader(new FileReader(path));
          while ((line = ins.readLine()) != null) {
             if (line.startsWith("#"))
                continue;
             id = -1;
             StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
             if (tok.hasMoreTokens()) {
                 data = tok.nextToken();
                 if (data.equals(VColorMapFile.KNOTS_SIZE)) {
                     if (tok.hasMoreTokens())
                        knots = Integer.parseInt(tok.nextToken());
                     continue;
                 }
                 if (data.equals(VColorMapFile.RED_KNOTS_INDEX))
                      id = 0;
                 else if (data.equals(VColorMapFile.GREEN_KNOTS_INDEX))
                      id = 1;
                 else if (data.equals(VColorMapFile.BLUE_KNOTS_INDEX))
                       id = 2;
                 if (id >= 0) {
                     setKnotIndexs(id, knots, tok);
                 }
             }
          }
       }
       catch (Exception e)  { }
       finally {
          try {
               if (ins != null)
                   ins.close();
          }  catch (Exception e2) {}
       }
       if (knots > 2)
          setKnotNumber(knots);
    }

    // end of VColorMapIF


    private class XCursedPlot extends VPlot.CursedPlot {
        protected MouseMotionListener ptMouseMotionListener = null;

        public XCursedPlot() {
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
            //updateGraph((Graphics2D)g);
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
                moveKnot(e.getX(), e.getY());
            }

            public void mouseReleased(MouseEvent e){
                mouseDown = false;
                if (curCtrl == null)
                    return;
                if (curCtrl.getSelectedKnot() < 0)
                    return;
                if (!mouseIn) {
                    curCtrl.setSelectedKnot(-1);
                    repaint();
                } else {
                    updateSelection(e.getX(), e.getY());
                }
            }

            public void mousePressed(MouseEvent e) {
                mouseDown = true;
            }

            public void mouseClicked(MouseEvent e){}

            public void mouseExited(MouseEvent e) {
                mouseIn = false;
                if (!mouseDown && curCtrl.getSelectedKnot() >= 0) {
                    curCtrl.setSelectedKnot(-1);
                    repaint();
                }
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

    private class KnotListener implements ActionListener {
        public void actionPerformed(ActionEvent  e) {
            int i = getChooserNumber();
            setKnotNumber(i, true);
        }
    }
}
