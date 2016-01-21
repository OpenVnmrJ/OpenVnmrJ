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
import java.lang.Math;
import java.util.*;
import java.io.*;

import vnmr.util.*;

public class DpsChannel implements DpsDef {
    private int  channelId;
    private int  viewX, viewY;
    private int  viewWidth, viewHeight;
    private int  defItems, defLines;
    private int  itemCount, dataCount;
    private int  baseY, origY;
    private int  moveY0, moveY2;
    private int  fontH, txtLevel;
    private int[] txtXPts;
    private int[] txtYPts;
    private double  locY;
    private Color   fgColor;
    private Color   bgColor;
    private Color   selectedColor;
    private String  filePath;
    // private String  channelName;
    private boolean bVisible = false;
    private boolean bMoving = false;
    private boolean bMoved = false;
    private boolean bShowBaseLine = true;
    private boolean bShowDuration = false;
    private boolean bRealRfShape = false;
    private boolean bDebug = false;
    private DpsObj  curObj;
    private DpsObj  dualObj;
    private Vector<DpsObj> objList;
    private Vector<DpsEventObj> eventList;
    private Font font;

    public DpsChannel(int n) {

         this.channelId = n;
         this.viewWidth = 0;
         this.viewHeight = 0;
         this.defItems = 0;
         this.defLines = 0;
         this.locY = 0.0;
         this.fgColor = Color.blue;
         this.bgColor = Color.lightGray;
         this.selectedColor = Color.red;
         font = new Font("SansSerif", Font.PLAIN, 12);
         fontH = font.getSize(); 
    }

    public void setFilePath(String path) {
        filePath = path;
        createData();
    }

    public void setDefaultItemSize(int items, int lines) {
        defItems = items;
        defLines = lines;
    }

    public int getId() {
        return channelId;
    }

    public Vector<DpsObj> getObjList() {
        return objList;
    }

    public Vector<DpsEventObj> getEventList() {
        return eventList;
    }

    public void clearObjs() {
        int k, num;
        DpsObj obj;

        if (objList != null) {
            num = objList.size();
            for (k = 0; k < num; k++) {
                obj = objList.elementAt(k);
                if (obj != null)
                     obj.clear();
            }
            objList.clear();
        }
        if (eventList != null) {
            num = eventList.size();
            for (k = 0; k < num; k++) {
                obj = eventList.elementAt(k);
                if (obj != null)
                     obj.clear();
            }
            eventList.clear();
        }
    }

    public void clear() {
        clearObjs();
        defItems = 0;
        defLines = 0;
        dataCount = 0;
        locY = 0.0;
        bMoving = false;
        bMoved = false;
    }

    public void setRfShape(boolean b) {
         bRealRfShape = b;
         if (channelId > 5 || objList == null)
            return;
         int num = objList.size();
         for (int n = 0; n < num; n++) {
            DpsObj obj = objList.elementAt(n);
            if (obj != null)
               obj.setRfShape(b);
         }
    }


    private void parseInfo(String type, StringTokenizer tok) {
        int  iv;
        double dv;
        String str;

        try {
            if (type.equals("#channel")) {
                channelId = Integer.parseInt(tok.nextToken()); 
                return;
            }
            if (type.equals("#name")) {
                // channelName = tok.nextToken(); 
                return;
            }
            if (type.equals("#seqtime")) {
                // seqTime = Double.parseDouble(tok.nextToken());
                return;
            }
            if (type.equals("#columns")) {
                // cols = Integer.parseInt(tok.nextToken()); 
                return;
            }
            if (type.equals("#item")) {
                int objCode = Integer.parseInt(tok.nextToken()); 
                if (objCode < 300) {
                    curObj = new DpsObj(objCode);
                    objList.setElementAt(curObj, itemCount);
                    curObj.setDataStartId(dataCount);
                    curObj.setRfShape(bRealRfShape);
                    if (objCode == DPESHGR)
                        dualObj = curObj;
                    else if (objCode == DPESHGR2) {
                        if (dualObj != null)
                           dualObj.setDualObject(curObj);
                    }
                    else
                        dualObj = null;
                    itemCount++;
                }
                else {
                    curObj = new DpsEventObj(objCode);
                    if (eventList == null)
                        eventList = new Vector<DpsEventObj>();
                    eventList.add((DpsEventObj)curObj);
                }
                curObj.setDataStartId(dataCount);
                curObj.setChanelId(channelId);
                return;
            }
            if (type.equals("#itemname")) {
                if (curObj != null)
                    curObj.setName(tok.nextToken());
                return;
            }
            if (type.equals("#size")) {
                iv = Integer.parseInt(tok.nextToken()); 
                if (curObj != null)
                    curObj.setDataSize(iv);
                return;
            }
            if (type.equals("#pattern")) {
                str = tok.nextToken(); 
                if (curObj != null)
                    curObj.setPattern(str);
                return;
            }
            if (type.equals("#filename")) {
                str = tok.nextToken(); 
                if (curObj != null)
                    curObj.setPatternPath(str);
                return;
            }
            if (type.equals("#duration")) {
                dv = Double.parseDouble(tok.nextToken());
                if (curObj != null)
                    curObj.setDuration(dv);
                return;
            }
            if (type.equals("#info")) {
                if (curObj != null)
                     curObj.addInfo(tok.nextToken("\n")+"\n");
                return;
            }
            if (type.equals("#value")) {
                dv = Double.parseDouble(tok.nextToken());
                if (curObj != null)
                    curObj.setValue(dv);
                return;
            }
            if (type.equals("#debug")) {
                bDebug = false; 
                iv = Integer.parseInt(tok.nextToken()); 
                if (iv > 0)
                   bDebug = true; 
                return;
            }
        }
        catch (Exception e) {
        }
    }

    private void createData() {
        String path = FileUtil.openPath(filePath);
        if (path == null)
            return;
        if (defItems < 1 || defLines < 1)
            return;
        BufferedReader fin = null;
        String line, data;
        // int k;
        // int repeates = 1;
        double time, amp, phase, rfValue;
        channelId = 1;
        clearObjs();
        if (objList == null)
            objList = new Vector<DpsObj>();
        objList.setSize(defItems+2);
        viewWidth = 0;
        viewHeight = 0;
        itemCount = 0;
        dataCount = 0;
        bDebug = false;
        curObj = new DpsObj(0);
        try {
            fin = new BufferedReader(new FileReader(path));
            while ((line = fin.readLine()) != null) {
                StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (data.startsWith("#")) {
                    parseInfo(data, tok);
                    continue;
                }
                time = Double.parseDouble(data);
                amp = Double.parseDouble(tok.nextToken());
                phase = Double.parseDouble(tok.nextToken());
                rfValue = Double.parseDouble(tok.nextToken());
                // if (tok.hasMoreTokens())
                //     repeates = Integer.parseInt(tok.nextToken()); 

                curObj.addData(time, amp, phase, rfValue);
                dataCount++;
            }
        } catch (Exception e) {
        }
        finally {
            try {
               if (fin != null)
                   fin.close();
               if (!bDebug) {
                   File f = new File(path);
                   f.delete();
               }
            }
            catch (Exception e2) {}
        }
    }

    public DpsObj findDpsObj(int x, int y) {
        if (y < viewY)
            return null; 
        if (y > (viewY + viewHeight))
            return null; 
        int num = objList.size();
        for (int k = 0; k < num; k++) {
            DpsObj obj = objList.elementAt(k);
            if (obj != null && obj.getCodeId() > 0) {
                if (obj.intersect(x, y)) {
                    return obj; 
                }
            }
        }
        return null; 
    }

    public Rectangle getViewRect() {
        return new Rectangle(viewX, viewY, viewWidth, viewHeight);
    }

    public boolean isMoved() {
         return bMoved;
    }

    public int getOriginY() {
        return baseY;
    }

    public double getLocy() {
        return locY;
    }

    public void setLocY(double p) {
        locY = p;
    }

    public void setViewRect(int x, int y, int w, int h) {
        int    n;
        double xgap, ygap;

        if (!bMoving) {
           if (Math.abs(w - viewWidth) < 5) {
               if (Math.abs(w - viewHeight) < 5)
                  return;
           }
        }
        viewX = x;
        viewY = y;
        viewWidth = w;
        viewHeight = h;
        if (dataCount < 1)
            return;
        if (!bMoved)
            origY = viewY;
        xgap = DpsUtil.getTimeScale();
        baseY = y + viewHeight / 2;
        if (channelId <= 5)
            baseY = y + viewHeight * 2 / 3;
        ygap = (double) (baseY - y) - 2.0;
        int num = objList.size();
        for (n = 0; n < num; n++) {
            DpsObj obj = objList.elementAt(n);
            if (obj != null) {
               obj.setDisplayPoints(xgap, ygap, baseY);
            }
        }
        if (txtXPts == null) {
            txtXPts = new int[4];
            txtYPts = new int[4];
        }
        txtLevel = (baseY - y) / fontH;
        if (txtLevel > 4)
            txtLevel = 4;
        for (n = txtLevel - 1; n >= 0; n--) {
            txtYPts[n] = y;
            y += fontH;
        }
    }

    public void setVisible(boolean b) {
         bVisible = b;
    }

    public boolean isVisible() {
        return bVisible;
    }

    public int[] getXpoints() {
        return null;
    }

    public int[] getYpoints() {
        return null;
    }

    public void setChannelColor(Color c) {
        fgColor = c;
    }

    public void setBaseLineColor(Color c) {
        bgColor = c;
    }

    public void setSelectedColor(Color c) {
        selectedColor = c;
    }

    public void setBaseLineVisible(boolean b) {
        bShowBaseLine = b;
    }

    public void setDurationVisible(boolean b) {
        bShowDuration = b;
    }

    public void startMove(int y) {
        bMoved = true;
        bMoving = true;
        moveY0 = viewY;
        moveY2 = y - viewHeight;
    }

    public void stopMove() {
        bMoving = false;
        if (viewY != origY)
            bMoved = true;
        else
            bMoved = false;
    }

    public void moveChannel(int y) {
        y = moveY0 + y;
        if (y < 0)
           y = 0;
        if (y > moveY2)
           y = moveY2;
        if (y != viewY)
           setViewRect(viewX, y, viewWidth, viewHeight);
    }

    public void paint(Graphics2D g, Stroke s1, Stroke s2, int xoffset) {
       Rectangle r = g.getClipBounds();
       g.setFont(font);
       FontMetrics fm = null;
       if (bShowBaseLine) {
          g.setColor(bgColor);
          g.drawLine(xoffset, baseY, viewWidth, baseY);
       }
       int num = objList.size();
       int  x0, y0, n;
       if (s2 != null)
           g.setStroke(s2);
       if (bShowDuration) {
           fm = g.getFontMetrics();
           for (n = 0; n < txtLevel; n++)
              txtXPts[n] = 0;
       }
       for (int k = 0; k < num; k++) {
            DpsObj obj = objList.elementAt(k);
            if (obj != null) {
                g.setColor(fgColor);
                if (obj.draw(g, selectedColor, bShowDuration, r)) {
                    if (bShowDuration && (DPESHGR2 != obj.getCodeId())) {
                        String durationStr = obj.getDurationString();
                        if (durationStr != null) {
                             x0 = obj.getStartX();
                             y0 = obj.getTextY();
                             n = 0;
                             while (n < txtLevel) {
                                 if (txtYPts[n] <= y0) {
                                    if (txtXPts[n] < x0) {
                                        y0 = txtYPts[n];
                                        break;
                                    }
                                 }
                                 n++;
                             }
                             if (n < txtLevel) {
                                 int sw = fm.stringWidth(durationStr);
                                 txtXPts[n] = x0 + sw + 4;
                                 g.drawString(durationStr, x0, y0);
                             }
                        }
                    }
                }
            }
       }
       if (s1 != null)
           g.setStroke(s1);
    }
}

