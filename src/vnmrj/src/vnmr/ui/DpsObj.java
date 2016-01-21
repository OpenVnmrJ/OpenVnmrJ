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

public class DpsObj implements DpsDef {
    public  int  code, dataSize;
    public  int  channelId;
    public  int  dataIndex;
    public  int  dataCount;
    public  int  baseY;
    public  int  width;
    public  int  height;
    public  int  originY;
    public  int  txtY;
    protected int  maxY;
    protected double  duration;
    protected double  value;
    protected boolean bSelected;
    protected boolean bRealRfShape = false;

    protected int[]  timeXs;
    protected int[]  ampYs;
    protected int[]  ampDualYs;
    protected int[]  phaseYs;
    protected int[]  phaseDualYs;
    protected double[]  times;
    protected double[]  amps;
    protected double[]  phases;
    protected double[]  rfVs;

    protected DpsObj  dualObj;
    protected String  name;
    protected String  durationStr;
    protected String  patternName;
    protected String  patternPath;
    protected String  infoStr;


    public DpsObj(int n) {
         this.code = n;
         this.channelId = 0;
	 this.dataSize = 0;
         this.dataCount = 0;
    }

    public void setName(String str) {
        name = str;
    }

    public String getName() {
        return name;
    }

    public void setDuration(double n) {
        duration = n;
        if (n > 0.0)
            durationStr = Double.toString(n);
    }

    public void setChanelId(int n) {
        channelId = n;
    }

    public int getChanelId() {
        return channelId;
    }

    public int getCodeId() {
        return code;
    }

    public void setDataStartId(int n) {
        dataIndex = n;
    }

    public void setDataSize(int n) {
        int oldSize = dataSize;
        dataSize = n;
        if (n < 1 || dataSize == oldSize)
            return;
        dataCount = 0;
        times = new double[n + 2];
        amps = new double[n + 2];
        timeXs = new int[n + 2];
        ampYs = new int[n + 2];
        if (channelId <= 5) {
            phases = new double[n+2];
            // rfVs = new double[n+2];
            phaseYs = new int[n + 2];
        }
    }

    public void addData(double time, double amp, double phase, double rf) {
        if (dataCount > dataSize)
           return;
        times[dataCount] = time;
        amps[dataCount] = amp;
        if (channelId <= 5) {
            phases[dataCount] = phase;
        }
        dataCount++; 
    }

    public void setRfShape(boolean b) {
        bRealRfShape = b;
    }

    public double getStartTime() {
        if (times == null || dataCount < 2)
            return 0.0;
        return times[0];
    }

    public double getEndTime() {
        if (times == null || dataCount < 2)
            return 0.0;
        return times[dataCount - 1];
    }

    public boolean intersect(int x, int y) {
        if (timeXs == null || dataCount < 2)
            return false;
        if (x >= timeXs[0] && x <= timeXs[dataCount - 1]) {
            return true;
        }
        return false;
    }

    public void setDualObject(DpsObj obj) {
        dualObj = obj;
    }

    public DpsObj getDualObject() {
        return dualObj;
    }


    public void clear() {
       times = null;
       amps = null;
       phases = null;
       rfVs = null;
       timeXs = null;
       ampYs = null;
       ampDualYs = null;
       phaseYs = null;
       phaseDualYs = null;
       dataSize = 0;
    }

    public void setDisplayPoints(double xgap, double h, int yoffset) {
        baseY = yoffset;
        originY = yoffset - (int) h;
        int  n, y;
        int  y0 = yoffset;

        if (dataSize < 1)
            return;
        height = (int) h;
        for (n = 0; n < dataCount; n++) {
            timeXs[n] = (int) (xgap * times[n]);
            y = yoffset - (int) (h * amps[n]);
            ampYs[n] = y;
            if (y < y0)
                y0 = y;
        }
        y0 = y0 - 6;
        if (y0 < 12)
           y0 = 12;
        txtY = y0;
        width = 0;
        if (dataCount > 1)
            width = timeXs[dataCount -1] - timeXs[0];
        if (code == GRPPULSE) {
            if (ampDualYs == null)
                ampDualYs = new int[dataSize + 2];
            y0 = yoffset - 6;
            for (n = 0; n < dataCount; n++) {
                ampDualYs[n] = y0 - (int) (h * amps[n]);
            }

        }
        if (channelId <= 5 && phases != null) {
            for (n = 0; n < dataCount; n++) {
                phaseYs[n] = yoffset - (int) (h * phases[n]);
            }
            if (code == GRPPULSE) {
                if (phaseDualYs == null)
                    phaseDualYs = new int[dataSize + 2];
                y0 = yoffset - 5;
                for (n = 0; n < dataCount; n++) {
                    phaseDualYs[n] = y0 - (int) (h * phases[n]);
                }
            }
        }
        double maxV = 0.0;
        double minV = 0.0;
        int   index = 0;
        if (code == PEGRAD) {
            y = dataCount;
            if (y > 5)
                y = 5;
            for (n = 0; n < y; n++) {
                if (amps[n] != 0.0) {
                    if (amps[n] > maxV) {
                        maxV = amps[n];
                        index = n;
                    }
                    else if (amps[n] < minV) {
                        minV = amps[n];
                        index = n;
                    }
                 }
            }
        }
        maxY = ampYs[index];
    }

    public void setSelected(boolean b) {
       bSelected = b;
    }

    public boolean isSelected() {
       return bSelected;
    }

    public int getStartX() {
        if (timeXs == null)
            return 0;
        return timeXs[0];
    }
     
    public int getEndX() {
        if (timeXs == null)
            return 0;
        return timeXs[dataCount - 1];
    }

    public int getTextY() {
        return txtY;
    }

    public int getDataStartId() {
        return dataIndex;
    }
     
    public int getDataLastId() {
        return (dataIndex+dataSize);
    }

    public void setPattern(String str) {
        patternName = str;
    }

    public String getPattern() {
        if (dualObj != null) {
            if (dualObj.getPattern() != null) {
               return patternName+",  "+dualObj.getPattern();
            }
        }
           
        return patternName;
    }

    public void setPatternPath(String str) {
        patternPath = str;
    }

    public String getPatternPath() {
        return patternPath;
    }

    public void setValue(double n) {
        value = n;
    }

    public String getInfo() {
        // if (durationStr != null)
        //    return "Duration: "+durationStr+" us";
        return infoStr;
    }

    public void addInfo(String str) {
        if (infoStr == null) {
           infoStr = str;
           return;
        }
        infoStr = infoStr + str;
    }

    public String getDurationString() {
        return durationStr;
    }

    public void xdraw(Graphics2D g) {
        if (dataCount < 2)
            return;
        int x0, x1, y0, d, h;

        if (code == JSHPULSE || code == JGRPPULSE) {
             d = width / 4;
             if (d < 1)
                 d = 1;
             h = height / 4;
             if (h < 1)
                 h = 1;
             y0 = baseY - h;
             h = h * 2;
             x0 = timeXs[0];
             g.drawArc(x0, y0, d, h, 0, 180);
             x0 = timeXs[0] + width - d - 1;
             g.drawArc(x0, y0, d, h, 0, 180);
             x0 = timeXs[0] + d;
             d = width / 2;
             if (d < 1)
                 d = 1;
             h = height * 3 / 4;
             y0 = baseY - h;
             h = h * 2;
             g.drawArc(x0, y0, d, h, 0, 180);
             if (code == JGRPPULSE) {
                y0 = y0 - 8;
                g.drawArc(x0, y0, d, h, 0, 180);
             }
             return;
        }
        if (code == JSHGRAD) {
             d = width / 8;
             x0 = timeXs[0] + d;
             h = height * 3 / 4;
             y0 = baseY - h;
             g.drawLine(timeXs[0], baseY, x0, y0);
             x1 = timeXs[0] + width - d;
             g.drawLine(x0, y0, x1, y0);
             x0 = timeXs[0] + width;
             g.drawLine(x1, y0, x0, baseY);
        }
    }

    public boolean draw(Graphics2D g, Color selectedColor, boolean bShowDuration, Rectangle clip) {
        if (code < 1)
            return false;
        if (timeXs == null || dataCount < 1)
            return false;
        if (timeXs[dataCount-1] < clip.x)
            return false;
        if (timeXs[0] > (clip.x + clip.width))
            return false;
        if (bSelected)
            g.setColor(selectedColor);
        else {
            if (code == DPESHGR2)
               g.setColor(Color.orange);
        }
        if (code >= JSHPULSE) {
            xdraw(g);
            return true;
        }
        if (bRealRfShape && phaseYs != null)
            g.drawPolyline(timeXs, phaseYs, dataCount);
        else
            g.drawPolyline(timeXs, ampYs, dataCount);
        if (code == PEGRAD && dataCount > 2) {
            if (maxY != 0) {
                 int x0 = timeXs[0];
                 int x1 = timeXs[dataCount - 1];
                 int ygap = (maxY - baseY) / 3;
                 int y = baseY;
                 for (int n = 0; n < 3; n++) {
                     g.drawLine(x0, y, x1, y);
                     y = y + ygap;
                 }
            }
        }
        else if (code == GRPPULSE && dataCount > 2) {
            if (bRealRfShape && phaseDualYs != null)
                g.drawPolyline(timeXs, phaseDualYs, dataCount);
            else if (ampDualYs != null)
                g.drawPolyline(timeXs, ampDualYs, dataCount);
        }
        /***
        if (bShowDuration && code != DPESHGR2) {
            if (durationStr != null) {
                 g.drawString(durationStr, timeXs[0], txtY);
            }
        }
        ***/
        return true;
   }
}

