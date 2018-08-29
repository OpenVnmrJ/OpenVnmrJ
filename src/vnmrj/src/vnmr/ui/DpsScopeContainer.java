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
import java.util.*;
import javax.swing.*;

public class DpsScopeContainer extends JLayeredPane {
    private JScrollPane scopeScroll;
    private JScrollPane timeScroll;
    private JScrollBar scopeHBar;
    private JScrollBar scopeVBar;
    private JScrollBar timeHBar;
    private DpsScopePanel scopePan;
    private DpsTimeLine  timeLine;
    private Runnable setTimePanelProc;
    private Vector<JScrollBar> scrollListeners;

    public DpsScopeContainer() {
        buildGUi();
    }

    private void buildGUi() {
      
        setLayout(new ScopeLayout());
        setBackground(Color.white);

        timeLine = new DpsTimeLine();
        timeScroll = new JScrollPane(timeLine,
                  JScrollPane.VERTICAL_SCROLLBAR_NEVER,
                  JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        timeScroll.setBorder(BorderFactory.createEmptyBorder());
        timeHBar = timeScroll.getHorizontalScrollBar();
        timeLine.setBackground(Color.white);
        timeScroll.setBackground(Color.white);

        scopePan = new DpsScopePanel();
        scopeScroll = new JScrollPane(scopePan,
                  JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                  JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        DpsScrollBar dpsBar = new DpsScrollBar();
        dpsBar.setViewPort(scopeScroll.getViewport());
        scopeScroll.setHorizontalScrollBar(dpsBar);
        scopeScroll.setBorder(BorderFactory.createEmptyBorder());
        scopeVBar = scopeScroll.getVerticalScrollBar();
        scopeHBar = scopeScroll.getHorizontalScrollBar();
        scopePan.setScrollPanel(scopeScroll);
        scopePan.setBackground(Color.white);

        add(scopeScroll, JLayeredPane.DEFAULT_LAYER);
        add(timeScroll, JLayeredPane.PALETTE_LAYER);
        scopePan.setScopeContainer(this);

        if (scopeVBar != null) {
             scopeVBar.addAdjustmentListener(new AdjustmentListener() {
                 public void adjustmentValueChanged(AdjustmentEvent e) {
                     scrollVerticalChanged();
                 }
             });
        }

        if (scopeHBar != null && timeHBar != null) {
             scopeHBar.addAdjustmentListener(new AdjustmentListener() {
                 public void adjustmentValueChanged(AdjustmentEvent e) {
                     scrollHorizontalChanged();
                 }
             });
        }
        DpsUtil.setScopePanel(scopePan);
        DpsUtil.setScopeContainer(this);

        setTimePanelProc = new Runnable() {
            public void run() {
                   scrollHorizontalChanged();
            }
        };
    }

    public void addVerticalScrollListener(JScrollBar bar) {
        if (scrollListeners == null)
            scrollListeners = new Vector<JScrollBar>();
        if (!scrollListeners.contains(bar))
            scrollListeners.add(bar);
    }

    private void scrollVerticalChanged() {
        if (scrollListeners == null)
            return;
        int v = scopeVBar.getValue();
        for (int k = 0; k < scrollListeners.size(); k++) {
            JScrollBar bar = scrollListeners.elementAt(k);
            if (bar != null)
               bar.setValue(v);
        }
    }

    private void scrollHorizontalChanged() {
        timeHBar.setValue(scopeHBar.getValue());
    }

    public void execCmd(String cmd) {
        scopePan.execCmd(cmd);
    }

    public void clear() {
         scopePan.clear();
    }

    public Rectangle getViewRect() {
        return scopePan.getViewRect();
    }

    public Dimension getViewSize() {
        return scopePan.getViewSize();
    }

    public void print(Graphics2D g, int xoffset) {
         scopePan.print(g, xoffset);
    }

    public void printTimeLine(Graphics2D g) {
         Point pt = timeScroll.getLocation();
         g.translate(0, pt.y);
         timeLine.paint(g);
    }

    private void layoutTimePanel() {
        Dimension dim = getSize();
        int bh = 0;
        int bw = 0;
        int timeH = 30;
        int y = 0;

        if (scopeHBar != null && scopeHBar.isVisible()) {
            bh = scopeHBar.getHeight();
            if (bh < 2)
               bh = 20;
        }
        if (scopeVBar != null && scopeVBar.isVisible()) {
            bw = scopeVBar.getWidth();
            if (bw < 2)
               bw = 20;
        }
        timeH = DpsUtil.getTimeLineHeight();
        y = dim.height - bh - timeH;
        bw = dim.width - bw;
        dim = scopePan.getSize();
        dim.height = timeH;
        timeLine.setPreferredSize(dim);
        timeLine.setSize(dim);
        timeScroll.setBounds(0, y, bw, timeH);
        timeScroll.revalidate();
    }

    public void setTimePanelGeom() {
        layoutTimePanel();
        SwingUtilities.invokeLater(setTimePanelProc); 
    }

    private void layoutComponents(Dimension dim) {
        scopeScroll.setBounds(0, 0, dim.width, dim.height);
        layoutTimePanel();
    }

    private class ScopeLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }


        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(100, 100);
        }

        public void layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
                Dimension dim = target.getSize();
                layoutComponents(dim);
            }
        }
    }

}

