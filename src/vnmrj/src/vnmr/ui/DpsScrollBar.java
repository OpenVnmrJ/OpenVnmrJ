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
import java.util.*;
import javax.swing.*;

public class DpsScrollBar extends JScrollBar {
    private int viewWidth, viewHeight;
    private int objCount, objCapacity;
    private int xOffset, yOffset;
    private int[] objXs;
    private int[] objWs;
    private int[] xPoints;
    private int[] yPoints;
    private double[] objMiddleTimes;
    private double[] objHalfTimes;
    private double seqTime;
    private JViewport vp;

    public DpsScrollBar() {
        super(HORIZONTAL);
    }

    public DpsScrollBar(int orientation) {
        super(orientation);
    }

    public void setViewPort(JViewport p) {
        vp = p;
    }

    public int getBlockIncrement(int direction) {
        if (vp == null)
             return 120;
        if (vp.getView() instanceof Scrollable) {
              Scrollable view = (Scrollable)(vp.getView());
              Rectangle vr = vp.getViewRect();
              return view.getScrollableBlockIncrement(vr, getOrientation(), direction);
        }
        else if (getOrientation() == VERTICAL) {
             return vp.getExtentSize().height;
        }
        return vp.getExtentSize().width;
    }

    public void addObjList(Vector<DpsObj> list) {
        if (list == null)
            return;
        int num = list.size();
        if (objCapacity <= objCount + num)
            setObjCapacity(objCount + num + 20);
        for (int k = 0; k < num; k++) {
            DpsObj obj = list.elementAt(k);
            if (obj != null && obj.getCodeId() > 0) {
                 double t0 = obj.getStartTime();
                 double t1 = obj.getEndTime();
                 objHalfTimes[objCount] = (t1 - t0) / 2.0;
                 objMiddleTimes[objCount] = t0 + objHalfTimes[objCount];
                 objCount++;
            }
        }
    }

    public void finishAdd() {
        seqTime = DpsUtil.getSeqTime();
        viewWidth = 1;
        viewHeight = 1;
        adjustSize();
    }

    public void clear() {
        objCount = 0;
        objCapacity = 0;
        objMiddleTimes = null;
        objHalfTimes = null;
        objXs = null;
        objWs = null;
        xOffset = 18;
        yOffset = 3;
    }

    private void setObjCapacity(int num) {
        int k;
        double[] newMtimes;
        double[] newHtimes;

        newMtimes = new double[num];
        newHtimes = new double[num];
        if (objCapacity > 0) {
            for (k = 0; k < objCapacity; k++) {
                newMtimes[k] = objMiddleTimes[k];
                newHtimes[k] = objHalfTimes[k];
            }
        }
        objCapacity = num; 
        objMiddleTimes = newMtimes;
        objHalfTimes = newHtimes;
    }

    private void adjustSize() {
        Dimension dim = getSize();
        yOffset = dim.height / 4;
        if (yOffset < 2)
            yOffset = 2;
        int w = dim.width - xOffset * 2 ;
        int h = dim.height - yOffset * 2;
        if (w == viewWidth && h == viewHeight)
            return;
        if (w < 20)
            return;
        seqTime = DpsUtil.getSeqTime();
        if (seqTime <= 0.0)
            return;
        double gap = (double) w / seqTime;
        if (objXs == null) {
            objXs = new int[objCount];
            objWs = new int[objCount];
        }
        for (int n = 0; n < objCount; n++) {
            objXs[n] = (int) (gap * objMiddleTimes[n]) + xOffset;
            objWs[n] = (int) (gap * objHalfTimes[n]);
        }
        
        viewWidth = w;
        viewHeight = h;
    }

    public void paint(Graphics g) {
        Dimension dim = getSize();

        super.paint(g);
        adjustSize();
        if (objXs == null)
            return;
        if (xPoints == null || yPoints == null) {
            xPoints = new int[3];
            yPoints = new int[3];
        }
        yPoints[0] = dim.height - yOffset;
        yPoints[1] = yOffset;
        yPoints[2] = dim.height - yOffset;
        g.setColor(Color.green.darker());
        for (int n = 0; n < objCount; n++){
            xPoints[0] = objXs[n] - objWs[n];
            xPoints[1] = objXs[n];
            xPoints[2] = objXs[n] + objWs[n];
            g.drawPolygon(xPoints, yPoints, 3);
        }
    }
}
