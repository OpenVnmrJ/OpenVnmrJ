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
import java.awt.print.*;
import javax.swing.*;

public class DpsGraphPanel extends JPanel implements Printable {
    private DpsChannelName channelPan;
    private DpsScopeContainer scopePan;
    private JScrollPane channelScroll;
    private JScrollBar channelBar;
    private PrinterJob printJob = null;

    public DpsGraphPanel() {
         buildGUi();
    }

    private void buildGUi() {
         setLayout(new BorderLayout());
         // setBorder(BorderFactory.createEtchedBorder());

         channelPan = new DpsChannelName();
         channelPan.setBorder(BorderFactory.createEmptyBorder());
         channelPan.setBackground(Color.white);
         DpsUtil.setNamePanel(channelPan);

         channelScroll = new JScrollPane(channelPan,
                  JScrollPane.VERTICAL_SCROLLBAR_NEVER,
                  JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);

         channelBar = channelScroll.getVerticalScrollBar();
         add(channelScroll, BorderLayout.WEST);
         channelScroll.setBorder(BorderFactory.createEmptyBorder());

         scopePan = new DpsScopeContainer();
         add(scopePan, BorderLayout.CENTER);
         scopePan.addVerticalScrollListener(channelBar);
         DpsUtil.setNameScrollPanel(channelScroll);
         JPanel toolPan = DpsUtil.getToolPanel();
         if (toolPan != null) {
             toolPan.setBorder(BorderFactory.createEtchedBorder());
             add(toolPan, BorderLayout.NORTH);
         }
    }

    private void execPrint() {
         if (printJob == null) {
             printJob = PrinterJob.getPrinterJob();
             PageFormat landscape = printJob.defaultPage();
             landscape.setOrientation(PageFormat.LANDSCAPE);
             printJob.setPrintable(this, landscape);
         }
         if (printJob.printDialog()) {
             try {
                   printJob.print();
             } 
             catch(PrinterException pe) {
             } 
         }
    }

    public void execCmd(String cmd) {
         if (cmd.equals(DpsCmds.Print)) {
            execPrint();
            return;
         }
         scopePan.execCmd(cmd);
    }

    public void clear() {
         scopePan.clear();
    }

    public int print(Graphics g, PageFormat pageFormat, int pageIndex) {
         if (pageIndex > 0)
             return(NO_SUCH_PAGE);
         Rectangle r = g.getClipBounds();
         RepaintManager currentManager = RepaintManager.currentManager(this);
         boolean bDoubleBuffering = currentManager.isDoubleBufferingEnabled();
         currentManager.setDoubleBufferingEnabled(false);
         Graphics2D g2d = (Graphics2D)g;
         int mx = (int) pageFormat.getImageableX();
         int my = (int) pageFormat.getImageableY();
         g2d.translate(mx, my);
         Rectangle rec = scopePan.getViewRect();
         Dimension dim = channelPan.getSize();
         Double rw = (double) r.width / (double) (dim.width + rec.width);
         Double rh = (double) r.height / (double) (rec.height);
         g2d.scale(rw, rh);
         g2d.translate(0, -rec.y);
         channelPan.print(g2d);
         g2d.translate(-rec.x + dim.width, 0);
         r = g.getClipBounds();
         mx = r.x + r.width;
         g2d.setClip(rec.x, r.y, mx - rec.x, r.height);
         scopePan.print(g2d, rec.x);
         g2d.translate(0, rec.y);
         scopePan.printTimeLine(g2d);
         currentManager.setDoubleBufferingEnabled(bDoubleBuffering);
         return(PAGE_EXISTS);
    }
}

