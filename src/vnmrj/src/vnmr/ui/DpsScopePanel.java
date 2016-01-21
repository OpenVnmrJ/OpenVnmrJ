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
import java.awt.event.*;
import java.lang.Math;
import java.util.*;
import javax.swing.*;

import vnmr.util.*;

public class DpsScopePanel extends JPanel {

    private DpsChannel[] channels;
    private int NUM = 10;
    private int panelWidth, panelHeight;
    private int cursorX0, cursorX1;
    private int pressedMouseBut = 0;
    private int vpX, vpWidth, vpHeight;
    private int spWidth;
    private int sbWidth = 20;
    private int lineWidth = 1;
    private int prtXOffset = 0;
    private double tpp;  // time per pixel
    private double ppt;  // pixel per time
    private Vector<DpsEventObj> eventList;
    private long lastClickTime = 0;
    private DpsChannelName namePan;
    private DpsTimeLine timePan;
    private DpsObj selectedObj;
    private JScrollPane sp;
    private JScrollBar hScrlBar;
    private DpsScrollBar dpsScrlBar;
    private JViewport vp;
    private JLabel cr1, cr2, delta;
    private JPanel infoPanel;
    private Color selectedColor;
    private BasicStroke lineStroke = null;
    private BasicStroke dashStroke;
    private DpsOptions optionPanel;
    private DpsScopeContainer scopeContainer;
    private Runnable setHScrollProc;
    private Runnable setTimePanelProc;
    private Runnable selectProc;

    public DpsScopePanel() {
         NUM = DpsWindow.DPSCHANNELS;
         channels = new DpsChannel[NUM];
         setLayout(new DpsScopeLayout());

         cursorX0 = 2;
         cursorX1 = 4;
         panelWidth = 12;
         panelHeight = 12;
         dashStroke = new BasicStroke(1.0f,
               BasicStroke.CAP_SQUARE, BasicStroke.JOIN_MITER,
               10.0f,
               new float[] {5.0f,5.0f}, // Dash pattern
               0.0f); 
         addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int nClick = evt.getClickCount();
                long when = evt.getWhen();
                if ((when - lastClickTime) < 400)
                     nClick = 2;
                lastClickTime = when;
                if (nClick > 1)
                    selectItem(evt.getX(), evt.getY());
            }
            public void mousePressed(MouseEvent evt) {
                int x = evt.getX();
                int modifier = evt.getModifiersEx();

                pressedMouseBut = 1;
                if ((modifier & InputEvent.BUTTON3_DOWN_MASK) != 0)
                    pressedMouseBut = 3;
                else if ((modifier & InputEvent.BUTTON2_DOWN_MASK) != 0)
                    pressedMouseBut = 2;
                if (pressedMouseBut == 1)
                    cursorX0 = x;
                else if (pressedMouseBut == 3)
                    cursorX1 = x;
                repaint();
            }

            public void mouseReleased(MouseEvent evt) {
                int x;
                if (pressedMouseBut != 2) {
                    if (cursorX0 > cursorX1) {
                        x = cursorX0;
                        cursorX0 = cursorX1;
                        cursorX1 = x;
                    }
                    setTimeMessage();
                }
                else {
                    selectItem(evt.getX(), evt.getY());
                }
                pressedMouseBut = 0;
            }

            public void mouseEntered(MouseEvent evt) {
            }

            public void mouseExited(MouseEvent evt) {
            }
        });

        addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent evt) {
                int x = evt.getX();
                if (pressedMouseBut != 2) {
                    if (x < 0)
                        x = 0;
                    if (pressedMouseBut == 1)
                        cursorX0 = x;
                    else if (pressedMouseBut == 3)
                        cursorX1 = x;
                    repaint();
                }
            }

            public void mouseMoved(MouseEvent evt) {
            }
        });

        setHScrollProc = new Runnable() {
            public void run() {
                   setHScrollBar();
            }
        };

        setTimePanelProc = new Runnable() {
            public void run() {
                   setTimePanelGeom();
            }
        };
    }

    public void setScrollPanel(JScrollPane p) {
         sp = p;
         if (p == null)
            return;
         vp = p.getViewport();
         hScrlBar = p.getHorizontalScrollBar();
         if (hScrlBar instanceof DpsScrollBar)
            dpsScrlBar = (DpsScrollBar) hScrlBar;
         sbWidth = hScrlBar.getHeight();
         if (sbWidth < 2)
             sbWidth = 20;
         hScrlBar.setUnitIncrement(10);
    }

    public void setScopeContainer(DpsScopeContainer p) {
         scopeContainer = p;
    }

    private void setInfoPanel() {
         Dimension dim;
         int   x;

         if (cr1 != null)
            return;
         infoPanel = DpsUtil.getInfoPanel();
         if (infoPanel == null)
            return;
         infoPanel.setLayout(new SimpleHLayout());
         cr1 = new JLabel("cr1:123456789.999 ");
         cr2 = new JLabel("cr2:   ");
         delta = new JLabel("delta:   ");
         dim = cr1.getPreferredSize();
         x = 2;
         cr1.setLocation(x, 0);
         x += dim.width;
         cr2.setLocation(x, 0);
         x += dim.width;
         delta.setLocation(x, 0);
         infoPanel.add(cr1);
         infoPanel.add(cr2);
         infoPanel.add(delta);
         cr1.setText("cr1:  ");
    }

    private DpsChannel getChannel(int id) {
        if (id >= NUM)
            return null;
        DpsChannel chan = channels[id];
        if (chan != null && chan.isVisible())
            return chan;
        return null;
    }

    private void addEventObjs(Vector<DpsEventObj> list) {
        if (list == null || list.size() < 1)
            return;
        if (eventList == null)
            eventList = new Vector<DpsEventObj>();
        int num = list.size();
        for (int k = 0; k < num; k++) {
            DpsEventObj obj = list.elementAt(k);
            if (obj != null)
                eventList.add(obj);
        }
    }

    public void clear() {
         selectedObj = null;
         for (int k = 0; k < NUM; k++) {
             if (channels[k] != null) {
                 channels[k].setVisible(false);
                 channels[k].clear();
             }
         }
         if (eventList != null)
             eventList.clear();
         if (dpsScrlBar != null)
             dpsScrlBar.clear();
    }

    public void setRfShape(boolean b) {
        for (int k = 1; k <= 5; k++) {
           DpsChannel chan = channels[1];
           if (chan != null)
               chan.setRfShape(b);
        }
        repaint();
    }

    public void startAdd() {
         clear();
         optionPanel = DpsUtil.getOptionPanel();
         optionPanel.reset();
         selectedColor = optionPanel.getSelectedColor();
         setLineWidth(optionPanel.getLineWidth());
    }

    public void finishAdd() {
         layoutChannels(true); 
         cursorX0 = 2;
         cursorX1 = panelWidth - 6;
         resetSize();
         if (dpsScrlBar != null)
             dpsScrlBar.finishAdd();
    }

    public void addChannel(int id, String path, int items, int lines) {
        if (id >= NUM)
            return;
        if (path == null || path.length() < 1)
            return;
        if (optionPanel == null)
            optionPanel = DpsUtil.getOptionPanel();
        DpsChannel chan = channels[id];
        if (chan == null) {
            chan = new DpsChannel(id);
            channels[id] = chan;
        }
        chan.setDefaultItemSize(items, lines);
        chan.setFilePath(path);
        optionPanel.showChannelOption(id, true);
        chan.setVisible(optionPanel.isChannelVisible(id)); 
        chan.setBaseLineVisible(optionPanel.isBaseLineVisible()); 
        chan.setDurationVisible(optionPanel.isDurationVisible()); 
        chan.setChannelColor(optionPanel.getChannelColor(id)); 
        chan.setBaseLineColor(optionPanel.getBaseLineColor()); 
        chan.setSelectedColor(optionPanel.getSelectedColor()); 
        addEventObjs(chan.getEventList());
        if (dpsScrlBar != null)
             dpsScrlBar.addObjList(chan.getObjList());
    }

    public void setChannelColor(int id, Color c) {
        if (id >= NUM)
            return;
        DpsChannel chan = channels[id];
        if (chan == null)
            return;
        chan.setChannelColor(c);
        namePan = DpsUtil.getNamePanel();
        namePan.setChannelColor(id, c);
        repaint();
    }

    public void setBaseLineColor(Color c) {
        for (int n = 1; n < NUM; n++) {
            DpsChannel chan = channels[n];
            if (chan != null)
               chan.setBaseLineColor(c); 
        }
        repaint();
    }

    public void setSelectedColor(Color c) {
         selectedColor = c;
        for (int n = 1; n < NUM; n++) {
            DpsChannel chan = channels[n];
            if (chan != null)
               chan.setSelectedColor(c); 
        }
        repaint();
    }

    public void setChannelVisible(int id, boolean b) {
        if (id >= NUM)
            return;
        DpsChannel chan = channels[id];
        if (chan == null)
            return;
        chan.setVisible(b);
        namePan = DpsUtil.getNamePanel();
        namePan.setChannelVisible(id, b);
        layoutChannels(true);
        repaint();
    }

    public void setBaseLineVisible(boolean b) {
        for (int n = 1; n < NUM; n++) {
            DpsChannel chan = channels[n];
            if (chan != null)
               chan.setBaseLineVisible(b); 
        }
        repaint();
    }

    public void setDurationVisible(boolean b) {
        for (int n = 1; n < NUM; n++) {
            DpsChannel chan = channels[n];
            if (chan != null)
               chan.setDurationVisible(b); 
        }
        repaint();
    }

    public void setLineWidth(int n) {
        if (lineWidth == n) 
            return;
        lineWidth = n;
        lineStroke = new BasicStroke((float) n,
                      BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND);
        repaint();
    }

    public void moveChannel(int id, int y) {
        DpsChannel chan = getChannel(id);
        if (chan == null)
            return;
        chan.moveChannel(y);
        repaint();    
    }

    public void startMoveChannel(int id) {
        DpsChannel chan = getChannel(id);
        if (chan == null)
            return;
        chan.startMove(panelHeight);
    }

    public void stopMoveChannel(int id) {
        DpsChannel chan = getChannel(id);
        if (chan == null)
            return;
        chan.stopMove();
        Rectangle r = chan.getViewRect();
        double y = (double) r.y / (double)panelHeight;
        if (y < 0.0)
           y = 0.0;
        chan.setLocY(y); 
        layoutChannels(true); 
        if (namePan != null)
           namePan.doLayout();
        repaint();    
    }
   
    @Override
    public void paint(Graphics g) {
       super.paint(g);
       DpsChannel chan;
       int  k, x;
       Graphics2D g2d = (Graphics2D) g;
       Stroke st1 = null;
       Stroke st2 = null;

       if (timePan != null) {
           double tickUnit = timePan.getTickUnit();
           if (tickUnit > 0.0) {
              st1 = g2d.getStroke();
              g2d.setStroke(dashStroke);
              Rectangle r = g.getClipBounds();
              Color gcolor = new Color(220, 220, 220);
              g.setColor(gcolor);
              int n0 = (int) ((double) r.x / tickUnit) - 1;
              if (n0 < 0)
                  n0 = 0;
              int n1 = (int) ((double) (r.x + r.width) / tickUnit);
              for (k = n0; k <= n1 ; k++) {
                  x = (int) (tickUnit * k);
                  g.drawLine(x, 1, x, panelHeight);
              }
              g2d.setStroke(st1);
              st1 = null;
           }
       }
       if (lineWidth > 1) {
           st1 = g2d.getStroke();
           st2 = lineStroke;
       }
       if (eventList != null) {
           Rectangle r = g.getClipBounds();
           boolean bShowTime = optionPanel.isDurationVisible(); 
           for (k = 0; k < eventList.size(); k++) {
                DpsEventObj obj = eventList.elementAt(k);
                if (obj != null) {
                   obj.draw(g2d, selectedColor, bShowTime, r);
                }
           }
       }
       for (k = 0; k < NUM; k++) {
           chan = channels[k];
           if (chan != null && chan.isVisible())
                chan.paint(g2d, st1, st2, prtXOffset);
       }

       g.setColor(Color.red);
       g.drawLine(cursorX0, 2, cursorX0, panelHeight - 2);
       g.drawLine(cursorX1, 2, cursorX1, panelHeight - 2);
    }

    public void print(Graphics2D g, int xoffset) {
         boolean bOpaque = isOpaque();
         prtXOffset = xoffset;
         setOpaque(false);
         paint(g);
         setOpaque(bOpaque);
         prtXOffset = 0;
    }

    public Rectangle getViewRect() {
        return vp.getViewRect();
    }

    public Dimension getViewSize() {
        return sp.getSize();
    }

    private void setTimeMessage() {
       double t0, t1, t2;
       int n;

       if (cr1 == null) {
           setInfoPanel();
           if (cr1 == null)
                return;
       }
       t0 = tpp * cursorX0;
       n = (int) (t0 * 1000.0);
       t0 = ((double) n) / 1000.0;
       t1 = tpp * cursorX1;
       n = (int) (t1 * 1000.0);
       t1 = ((double) n) / 1000.0;
       t2 = t1 - t0;
       n = (int) (t2 * 1000.0);
       t2 = ((double) n) / 1000.0;
       String mess = "cr1: "+Double.toString(t0);
       cr1.setText(mess);
       mess = "cr2: "+Double.toString(t1);
       cr2.setText(mess);
       mess = "delta: "+Double.toString(t2)+" usec";
       delta.setText(mess);
    }

    private void setTimePanelGeom() {
        scopeContainer.setTimePanelGeom();
    }

    private void setHScrollBar() {
       if (hScrlBar != null && hScrlBar.isVisible()) {
           if (cursorX0 > 2)
               hScrlBar.setValue(cursorX0 - 2);
       }
       setTimePanelGeom();
    }

    private void getViewPortRect() {
        if (vp == null) {
            vpX = 0;
            vpWidth = panelWidth;
            vpHeight = panelHeight;
            return;
        }
        Rectangle r = vp.getViewRect();
        vpX = r.x;
        vpWidth = r.width;
        vpHeight = r.height;
        Dimension dim = sp.getSize();
        spWidth = dim.width;
        sbWidth = hScrlBar.getHeight();
        if (sbWidth < 2)
            sbWidth = 20;
    }

    private void execExpand() {
       int x2, w, h;
       double d, r;

       getViewPortRect();
       x2 = vpX + vpWidth;
       if (cursorX0 < vpX)
           cursorX0 = vpX;
       else if (cursorX0 > x2)
           cursorX0 = x2;
       if (cursorX1 < vpX || cursorX1 > x2)
           cursorX1 = x2;

       d = (double) Math.abs(cursorX1 - cursorX0);
       if (d < 4)
           d = 4; 
       r = (double) vpWidth / d;
       if (r < 1.01)
           return;
       if (r > 30.0)
           r = 30.0;
       w = (int) (panelWidth * r);
       cursorX0 = (int) (cursorX0 * r);
       cursorX1 = (int) (cursorX1 * r) - 4; 
       h = panelHeight;
       if (hScrlBar != null && !hScrlBar.isVisible()) {
          cursorX1 = cursorX1 - sbWidth; 
          h = panelHeight - sbWidth;
       }
       Dimension dim = new Dimension(w, h);
       setPreferredSize(dim);
       setSize(dim);
       sp.revalidate();
       SwingUtilities.invokeLater(setHScrollProc);
    }

    private void execShrink() {
       int  w;
       double dpr;

       getViewPortRect();
       if (panelWidth <= spWidth)
          return; 
       dpr = (double) vpX / (double) panelWidth;
       w = (int) (panelWidth * 0.6);
       if (w <= spWidth) 
           w = spWidth - 2;
       Dimension dim = new Dimension(w, panelHeight);
       setPreferredSize(dim);
       setSize(dim);
       cursorX0 = (int) (w * dpr) + 2;
       sp.revalidate();
       SwingUtilities.invokeLater(setHScrollProc);
    }

    private void resetSize() {
       setInfoPanel();
       getViewPortRect();
       if (Math.abs(vpWidth - panelWidth) < 6) {
           if (Math.abs(vpHeight - panelHeight) < 6)
                  return;
       }
       Dimension dim = new Dimension(vpWidth, vpHeight);
       setPreferredSize(dim);
       setSize(dim);
       namePan = DpsUtil.getNamePanel();
       dim = namePan.getSize();
       namePan.setSize(new Dimension(dim.width, vpHeight));
       sp.revalidate();
       SwingUtilities.invokeLater(setTimePanelProc);
    }

    private void execFitWidth() {
       getViewPortRect();
       setPreferredSize(new Dimension(vpWidth, panelHeight));
       sp.revalidate();
       SwingUtilities.invokeLater(setTimePanelProc);
    }

    private void execFitHeight() {
       getViewPortRect();
       setPreferredSize(new Dimension(panelWidth, vpHeight));
       namePan = DpsUtil.getNamePanel();
       Dimension dim = namePan.getSize();
       namePan.setSize(new Dimension(dim.width, vpHeight));
       sp.revalidate();
       SwingUtilities.invokeLater(setTimePanelProc);
    }

    private void execZoomIn() {
       int x2, w, h;
       double dpr, dpr2;
       Dimension dim;

       getViewPortRect();
       x2 = vpX + vpWidth;
       if (cursorX0 < vpX || cursorX0 > x2)
            cursorX0 = vpX + vpWidth / 4;
       if (cursorX1 < vpX || cursorX1 > x2)
            cursorX1 = vpX + vpWidth / 2;
       if (cursorX0 > cursorX1) {
            x2 = cursorX0;
            cursorX0 = cursorX1;
            cursorX1 = x2;
       }
       dpr = (double) cursorX0 / (double) panelWidth;
       dpr2 = (double) cursorX1 / (double) panelWidth;
       w = (int) ((double)panelWidth * 1.4);
       h = (int) ((double)panelHeight * 1.4);
       dim = new Dimension(w, h);
       setPreferredSize(dim);
       setSize(dim);
       namePan = DpsUtil.getNamePanel();
       dim = namePan.getSize();
       dim.height = h + sbWidth;
       namePan.setPreferredSize(dim);
       namePan.setSize(dim);
       cursorX0 = (int) (w * dpr) + 2;
       cursorX1 = (int) (w * dpr2) + 2;
       sp.revalidate();
       SwingUtilities.invokeLater(setHScrollProc);
    }

    private void execZoomOut() {
       int x2, w, h;
       double dpr, dpr2;
       Dimension dim;

       getViewPortRect();
       x2 = vpX + vpWidth;
       if (cursorX0 < vpX || cursorX0 > x2)
            cursorX0 = vpX + vpWidth / 4;
       if (cursorX1 < vpX || cursorX1 > x2)
            cursorX1 = vpX + vpWidth / 2;
       if (cursorX0 > cursorX1) {
            x2 = cursorX0;
            cursorX0 = cursorX1;
            cursorX1 = x2;
       }
       dpr = (double) cursorX0 / (double) panelWidth;
       dpr2 = (double) cursorX1 / (double) panelWidth;
       w = (int) ((double)panelWidth * 0.7);
       h = (int) ((double)panelHeight * 0.7);
       if (w < vpWidth)
            w = vpWidth;
       if (h < vpHeight)
            h = vpHeight;
       dim = new Dimension(w, h);
       setPreferredSize(dim);
       setSize(dim);
       namePan = DpsUtil.getNamePanel();
       dim = namePan.getSize();
       if (h > vpHeight)
            dim.height = h + sbWidth;
       else
            dim.height = h;
       namePan.setPreferredSize(dim);
       namePan.setSize(dim);
       cursorX0 = (int) (w * dpr) + 2;
       cursorX1 = (int) (w * dpr2) + 2;
       sp.revalidate();
       SwingUtilities.invokeLater(setHScrollProc);
    }

    private void execOption() {
        DpsOptions pane = DpsUtil.getOptionPanel();
        if (pane != null)
            pane.setVisible(true);
    }

    private void execPrint() {
    }

    public void execCmd(String cmd) {
       if (cmd.equalsIgnoreCase(DpsCmds.Expand)) {
            execExpand();
            return;
       }
       if (cmd.equalsIgnoreCase(DpsCmds.Shrink)) {
            execShrink();
            return;
       }
       if (cmd.equalsIgnoreCase(DpsCmds.FitWidth)) {
            execFitWidth();
            return;
       }
       if (cmd.equalsIgnoreCase(DpsCmds.FitHeight)) {
            execFitHeight();
            return;
       }
       if (cmd.equalsIgnoreCase(DpsCmds.ZoomIn)) {
            execZoomIn();
            return;
       }
       if (cmd.equalsIgnoreCase(DpsCmds.ZoomOut)) {
            execZoomOut();
            return;
       }
       if (cmd.equalsIgnoreCase(DpsCmds.Print)) {
            execPrint();
            return;
       }
       if (cmd.equalsIgnoreCase(DpsCmds.Option)) {
            execOption();
            return;
       }
    }

    private void showObjInfo() {
        if (selectedObj == null)
            return;
        DpsUtil.showDpsObjInfo(selectedObj);
    }

    private void selectItem(int x, int y) {
        DpsObj obj = null;        
        int k;
        DpsChannel chan;

        if (eventList != null) {
            for (k = 0; k < eventList.size(); k++) {
                DpsEventObj xobj = eventList.elementAt(k);
                if (xobj != null) {
                    if (xobj.intersect(x, y)) {
                        obj = (DpsObj) xobj;
                    }
                    else if (x < (xobj.getLocX() - 20))
                        break;
                }
            }
        }
        if (obj == null) {
            for (k = 0; k < NUM; k++) {
                chan = channels[k];
                if (chan != null && chan.isVisible()) {
                    obj = chan.findDpsObj(x, y);
                    if (obj != null)
                        break;
                }
            }
        }
        if (obj == null || obj == selectedObj)
            return;
        if (selectProc == null) {
            selectProc = new Runnable() {
                public void run() {
                       showObjInfo();
                }
           };
        }
        if (selectedObj != null) {
            selectedObj.setSelected(false);
        }
        selectedObj = obj;
        selectedObj.setSelected(true);
        SwingUtilities.invokeLater(selectProc);
        repaint();
    }

    private void layoutChannels(boolean bEnforced) {
        Dimension dim = getSize();
        if (dim.width < 10 || dim.height < 10)
            return;
        getViewPortRect();

        if (!bEnforced) {
           if (Math.abs(dim.width - panelWidth) < 5) {
               if (Math.abs(dim.height - panelHeight) < 5)
                  return;
           }
        }
        panelWidth = dim.width;
        panelHeight = dim.height;

        int k, w, y, y1, ygap; 
        double ry; 
        DpsChannel chan;
        int count = 0;

        for (k = 0; k < NUM; k++) {
             if (channels[k] != null) {
                 if (channels[k].isVisible())
                     count++;
             }
        }
        if (count < 1)
            return;
        y = 0;
        if (eventList != null && eventList.size() > 0)
            y = 12;
        namePan = DpsUtil.getNamePanel();
        ygap = panelHeight - y;
        if (hScrlBar != null && !hScrlBar.isVisible())
            ygap = ygap - DpsUtil.getTimeLineHeight();
        ygap = ygap / count;
        w = panelWidth - 6;
        double seqTime = DpsUtil.getSeqTime();
        if (seqTime < 1.0)
            seqTime = 100.0;
        tpp = seqTime / (double) w;
        ppt = (double)w / seqTime;
        DpsUtil.setTimeScale(ppt);
        timePan = DpsUtil.getTimeLinePanel();
        if (timePan != null)
            timePan.setTimeScale(ppt);
        for (k = 0; k < NUM; k++) {
            chan = channels[k];
            if (chan != null && chan.isVisible()) {
                 if (chan.isMoved()) {
                     ry = chan.getLocy();
                     y1 = (int)((double)panelHeight * ry);
                 }
                 else
                     y1 = y;
                 chan.setViewRect(0, y1, w, ygap);
                 if (namePan != null)
                     namePan.setLocY(k, chan.getOriginY());
                 y = y + ygap;
            }
        }
        if (namePan != null) {
            namePan.doLayout();
            namePan.repaint();
        }
        if (eventList == null || eventList.size() < 1)
            return;
        y = ygap + 2;
        int x0 = -100;
        int x1 = 0;
        int y0 = -100;
        for (k = 0; k < eventList.size(); k++) {
            DpsEventObj obj = eventList.elementAt(k);
            if (obj != null) {
               obj.setViewRect(0, 0, panelWidth, panelHeight);
               obj.setDisplayPoints(ppt, ygap, y);
               x1 = obj.getLocX();
               y1 = obj.getLocY();
               if (Math.abs(x1 - x0) < 14) {
                   if ((y1 < y0) || Math.abs(y1 - y0) < 14) {
                        if (y1 < y0)
                            y1 = y0 + 16;
                        else
                            y1 = y1 + 16;
                        obj.setLocY(y1);
                   }
               }
               x0 = x1;
               y0 = y1;
            }
        }
    }


    private class DpsScopeLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(20, 20);
        }

        public void layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
                layoutChannels(false);
            }
        }
    }

}

