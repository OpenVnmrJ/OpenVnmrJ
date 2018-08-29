/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import javax.swing.*;
import java.io.*;
import java.util.*;
import java.awt.*;

import vnmr.util.*;


public class GrkPlate extends GenPlateIF implements SmsDef
{
   public  int zoneNum = 1;
   public  int totalZone = 0;
   public  int rackNum = 1;
   public  int rackId = 1;
   public  int trayType = 0;
   public  int searchPattern = 1;
   public  int searchStart = 1;
   public  boolean gilsonNum = false;
   public  boolean rotated = false;
   private int rows[];
   private int cols[];
   public  String name;
   public  String label;
   private String rackPath;
   private String rackDir;
   private Vector<GrkZone> zoneList = null;
   private Vector<GrkZone> zoneShowList = null;
   private Vector<GrkZone> selectedZones = null;
   private GrkZone hilitZone = null;
   private int lastSelected = -1;
   private SmsSample prevSample;
   private SmsSample curSample;
   private GrkCompose grkRootNode = null;
   private GrkParser grkParser = null;
   public int objW = 0;
   public int fontH = 16;
   public int fontH2 = 16;
   private Font orgFont = null;
   private Font curFont;
   private FontMetrics orgFm;
   private FontMetrics curFm;
   private JComponent tray;
   private GrkLayout grkLayout;
   private long fileDate = 0;
   private Vector<SmsSample> hilitList;
   private StringBuffer  sb;
   private boolean shiftSelect = false;
   private boolean bRandomPick = false;
   private Color labelColor = new Color(98, 98, 98);
   static final String marvinRows[] =
          { "A", "A","B","C","D","E","F","G","H","I" };
   static final String marvinCols[] =
          { "1", "1","2","3","4","5","6","7","8","9" };

   public GrkPlate(String s) {
        this.name = s;
        expandZone(1);
        rackDir = Util.VNMRDIR+File.separator+"asm"+File.separator+"racks"+
                  File.separator;
        rackPath = rackDir+"code_"+name+".grk";
        grkLayout = new GrkLayout();
        zoneShowList = new Vector<GrkZone>();
        selectedZones = new Vector<GrkZone>();
        grkLayout.setZoneList(zoneShowList);
        hilitList = new Vector<SmsSample>();
        dateOfQfiles = new long[NUMQ];
        for (int k = 0; k < NUMQ; k++)
           dateOfQfiles[k] = 0;
        sb = new StringBuffer("  ");
   }


   public void showPlate(JComponent t) {
       tray = t;
       if (zoneList == null)
           zoneList = new Vector<GrkZone>();
       if (orgFont == null) {
           orgFont = new Font("Dialog", Font.BOLD, fontH);
           orgFm = t.getFontMetrics(orgFont);
           curFont = orgFont;
           curFm = orgFm;
       }
       File fd = new File(rackPath);
       if (fd == null || !fd.isFile()) {
           String str = "Error open file: "+ rackPath;
           Messages.postError(str);
           return;
       }
       if (fileDate != fd.lastModified()) {
           fileDate = fd.lastModified();
           BufferedReader fin = null;
           zoneList.clear();
           zoneShowList.clear();
           if (grkParser == null)
               grkParser = SmsUtil.getGrkParser();
           try {
               fin = new BufferedReader(new FileReader(rackPath));
               grkRootNode = grkParser.build(fin);
           }
           catch(IOException e) {
              String strError = "Error reading file: "+ rackPath;
              Messages.writeStackTrace(e, strError);
              Messages.postError(strError);
           }
           if (fin != null) {
              try {
                  fin.close();
               } catch(IOException ee) { }
           }
           createZone();
           adjustZone();
       }
       dispZone();
   }

   public Vector<SmsSample> getSelectList() {
       return hilitList;
   }

   public void setGilsonNum(boolean b) {
       if (gilsonNum != b) {
           gilsonNum = b;
           for (int i = 0; i < zoneList.size(); i++) {
              GrkZone  z = zoneList.elementAt(i);
              if (z != null) {
                 z.setGilsonNum(b);
                 adjustFont(z);
              }
           }
           SmsUtil.getPanel().repaint();
       }
   }

   public void setRotate(boolean b) {
       if (rotated != b) {
           rotated = b;
           for (int i = 0; i < zoneList.size(); i++) {
              GrkZone  z = zoneList.elementAt(i);
              if (z != null)
                 z.setRotate(b);
           }
           clearSelect();
           SmsUtil.showSelected(null);
           grkLayout.setRotate(b);
           SmsUtil.getTray().setRotate(b);
           // SmsUtil.getPanel().repaint();
       }
   }

   public void setZone(int n) {
       clearSelect();
       SmsUtil.showSelected(null);
       if (n > 900)
           dispAllZone();
       else
           dispZone(n);
   }

   public LayoutManager getLayoutMgr() {
       return grkLayout;
   }

   public String getName() {
       return name;
   }

   public void setRackId(int n) {
       rackId = n;
   }

   public int getRackId() {
       return rackId;
   }

   public SmsSample[] getSampleList() {
       return null;
   }

   public Vector<GrkZone> getZoneList() {
       return zoneList;
   }

   public void setZoneNum(int n) {
       zoneNum = n;
       if (zoneNum > totalZone)
           expandZone(zoneNum);
   }

   public int getZoneNum() {
       int n = 0;
       if (zoneList != null)
           n = zoneList.size();
       return  n;
   }

   public SmsSample getSample(int zoneId, int num) {
       if (zoneList == null)
          return null;
       if (zoneId < 1 || zoneId > zoneList.size())
          return null;
       if (num < 0)
          return null;
       GrkZone  z = zoneList.elementAt(zoneId - 1);
       SmsSample sample = z.getSample(num);
       if (sample != null)
          return sample;
    
       if (z.trayType == GRK97)
       {
           for (int n = 0; n < zoneList.size(); n++) {
              z = zoneList.elementAt(n);
              sample = z.getSample(num);
              if (sample != null)
                   return sample;
           }
       }
       return null;
   }

   public void setRowNum(int zone, int n) {
       if (zone < rows.length)
          rows[zone] = n; 
   }

   public void setColNum(int zone, int n) {
       if (zone < cols.length)
          cols[zone] = n; 
   }

   public int getRowNum(int zone) {
       if (zone > zoneNum)
          return 0;
       return rows[zone];
   }

   public int getColNum(int zone) {
       if (zone > zoneNum)
          return 0;
       return cols[zone];
   }

   public void setParam(StringTokenizer tok, String v) {
       if (!tok.hasMoreTokens())
           return;
       if (v == null || v.length() < 1)
           return;
       String p = tok.nextToken().trim();
       if (p.equals("label")) {
           label = v;
           return;
       }
       int a = 0;
       int zoneId = 0;
       try {
          a = Integer.parseInt(v);
       }
       catch (NumberFormatException er) { return; }

       if (p.equals("zones")) {
           zoneNum = a;
           if (zoneNum > totalZone)
               expandZone(zoneNum);
           return;
       }
       if (p.equals("racks")) {
           rackNum = a;
           return;
       }
       if (!tok.hasMoreTokens())
           return;

       try {
          zoneId = Integer.parseInt(p);
       }
       catch (NumberFormatException er) { return; }

       if (zoneId <= 0)
           return;
       if (zoneId > totalZone) {
           expandZone(zoneId+2);
       }
       p = tok.nextToken().trim();
       if (p.equals("row")) {
           rows[zoneId] = a;
           return;
       }
       if (p.equals("col")) {
           cols[zoneId] = a;
           return;
       }
   }


   private void expandZone(int num) {
       int oldVal[] = rows;
       int k;
       rows = new int[num+1];
       rows[0] = 0; 
       k = 1;
       if (totalZone > 0) {
          while (k < totalZone) {
             rows[k] = oldVal[k];
             k++;
          }
       } 
       while (k < num) {
          rows[k++] = 0;
       }
       oldVal = cols;
       cols = new int[num+1];
       cols[0] = 0; 
       k = 1;
       if (totalZone > 0) {
          while (k < totalZone) {
             cols[k] = oldVal[k];
             k++;
          }
       } 
       while (k < num) {
          cols[k++] = 0;
       }
       totalZone = num;
   }

   private void createZone() {
       if (grkRootNode == null)
           return;
       Vector<GrkObj> grkList = grkRootNode.getObjList();
       if (grkList == null)
           return;

       int num = grkList.size();
       ZoneCreater creater = SmsUtil.getZoneCreater();
       for (int i = 0; i < num; i++) {
           GrkObj obj = (GrkObj) grkList.elementAt(i);
           int count = getRowNum(i+1) * getColNum(i+1);
           if (obj.name != null) {
              GrkZone zone = creater.buildZone(count, obj, grkRootNode);
              zone.setId(i+1);
              zone.rowNum = rows[i+1];
              zone.colNum = cols[i+1];
              zoneList.add(zone);
           }
       }
   }

   private void grk12ShiftSelect() {
       if (prevSample == null || curSample == null)
           return;
       hilitList.clear();
       Vector<SmsSample> sList = hilitZone.getSampleList();
       if (sList == null)
           return;
       hilitList.addElement(prevSample);
       int curId = prevSample.id + 1;
       int lastId = curSample.id;
       int sSize = sList.size();
       int k;
       if (lastId > sSize)
           lastId = sSize;
       while (true) {
           SmsSample s = null;
           if (curId > sSize)
              curId = 1; 
           s = (SmsSample) sList.elementAt(curId - 1);
           if (s.id != curId) {
               for (k = 0; k < sSize; k++) {
                   s = (SmsSample) sList.elementAt(k);
                   if (s.id == curId)
                         break;
               }
               if (k >= sSize)
                    s = null;
           }
           if (s != null) {
               if (SmsUtil.isUsable(s)) {
                   s.selected = true;
                   hilitList.addElement(s);
               }
           }
           if (curId == lastId)
               break;
           curId++;
       }
   }

   private SmsSample getFirstSelectObj() {
       if (hilitList.size() < 1)
           return null;
       for (int i = 0; i < hilitList.size(); i++) {
           SmsSample obj = hilitList.elementAt(i);
           if (obj != null) {
              if (obj.selected && obj.startItem)
                 return obj;
           }
       }
       return null;
   }

   private SmsSample setFirstSelectObj() {
       if (hilitList.size() < 1)
           return null;
       SmsSample obj = getFirstSelectObj();
       if (obj != null)
           return obj;
       for (int k = 0; k < hilitList.size(); k++) {
           obj = hilitList.elementAt(k);
           if (obj != null && obj.selected) {
              obj.startItem = true;
              return obj;
           }
       }
       return null;
   }

   private int addCtrlSelect(SmsSample obj) {
       if (obj.selected) {
           obj.setSelect(false);
           if (hilitList.contains(obj))
               hilitList.removeElement(obj);
       }
       else {
           obj.setSelect(true);
           if (!hilitList.contains(obj))
               hilitList.addElement(obj);
       }
       SmsSample o = getFirstSelectObj();
       if (o != null)
           return o.id;
       for (int i = 0; i < hilitList.size(); i++) {
           o = hilitList.elementAt(i);
           if (o != null && o.selected) {
               o.startItem = true;
               return o.id;
           }
       }
       // no selected item left
       hilitList.clear();
       return -1;
   }

   private void clearHilitSelect() {
       for (int i = 0; i < hilitList.size(); i++) {
           SmsSample o = hilitList.elementAt(i);
           if (o != null)
               o.setSelect(false);
       }
   }

   private void zoneMultiSelect() {
       int  k, m;

       if (hilitZone == null)
           return;
       if (prevSample == null || curSample == null)
           return;
       clearHilitSelect();
       if (prevSample != null) {
           prevSample.selected = true;
           prevSample.startItem = true;
       }
       if (curSample != null)
           curSample.selected = true;
       if (hilitZone.trayType == GRK12)
           grk12ShiftSelect();
       else
           SmsUtil.zoneSelection(hilitZone, prevSample, curSample);
       sb.delete(1, sb.length());
       if (hilitList.size() > 1) {
           shiftSelect = true;
           SmsSample lastObj = null;
           SmsSample obj = hilitList.elementAt(0);
           obj.startItem = true;
           lastSelected = obj.id;
           prevSample = obj;
           // sb.append(lastSelected);
           sb.append(obj.idStr);
           sb.append(" ... ");
           m = hilitList.size();
           for (k = 1; k < m; k++) {
               obj = hilitList.elementAt(k);
               obj.startItem = false;
               if (obj.selected)
                  lastObj = obj;
           }
           if (lastObj != null)
               sb.append(lastObj.idStr);
       }
       else {
           shiftSelect = false;
           // sb.append(curSample.id);
           sb.append(curSample.idStr);
       }
       SmsUtil.showSelected(sb.toString());
   }

   private boolean updateShiftList(SmsSample o) {
       SmsSample obj;
       int  k, m, p, p2;
       boolean ret = false;
       
       m = hilitList.size();
       p2 = 0;
       p = 3100;
       for (k = 0; k < m; k++) {
           obj = hilitList.elementAt(k);
           if (obj.startItem)
              p2 = k;
           if (obj.id == o.id) {
              p = k;
              if (o.selected)
                  o.setSelect(false);
              else if (SmsUtil.isUsable(o))
                  o.setSelect(true);
              ret = true;
           }
       }
       sb.delete(1, sb.length());
       if (o.selected) {
           if (p <= p2) {
              obj = hilitList.elementAt(p2);
              obj.startItem = false;
              o.startItem = true;
              lastSelected = o.id;
              prevSample = o;
           }
       }
       else {
           lastSelected = -1;
           for (k = 0; k < m; k++) {
              obj = hilitList.elementAt(k);
              if (obj.selected) {
                  obj.startItem = true;
                  lastSelected = obj.id;
                  prevSample = obj;
                  break;
              }
           }
           if (lastSelected < 0)
              clearSelect(true);
       }
       if (lastSelected >= 0) {
           obj = hilitList.elementAt(0);
           sb.append(obj.idStr);
           if (m > 1) {
              for (k = m-1; k > 1; k--) {
                 obj = hilitList.elementAt(k);
                 if (obj.selected) {
                    sb.append(" ... ");
                    sb.append(obj.idStr);
                    break;
                 }
              }
           }
       }
       SmsUtil.showSelected(sb.toString());
       return ret;
   }

   public void clearSelect(boolean clearLastSelect) {
       int  k, m;
       Vector<SmsSample> oList;

       hilitList.clear();
       shiftSelect = false;
       // SmsUtil.showSelected(null);
       if (clearLastSelect) {
           lastSelected = -1;
           prevSample = null;
       }
       // if (hilitZone == null)
       //    return;
       for (int i = 0; i < selectedZones.size(); i++) {
           GrkZone  z = selectedZones.elementAt(i);
           if (z != null) {
              oList = z.getSampleList();
              m = oList.size();
              for (k = 0; k < m; k++) {
                  SmsSample o = oList.elementAt(k);
                  o.setSelect(false);
              }
           }
       }
       selectedZones.clear();
   }

   public void clearSelect() {
       clearSelect(true);
   }

   public void setSelectable(boolean b) {
   }

   public boolean setSelection(Vector<String> v) {
        boolean bGot = false;

        for (int k = 0; k < v.size(); k++) {
            String data = (String) v.elementAt(k);
            try {
                if (data != null) {
                    int num = Integer.parseInt(data);
                    if (num >= 1) {
                       if (selectSample(num))
                           bGot = true;
                    }
                }
            }
            catch (NumberFormatException er) { 
                return bGot;
            }
        }
        return bGot;
   }

   public boolean setSelection(boolean b) {
       if (!b) {
          clearSelect();
          return true;
       }
       return false;
   }


   private void adjustFont(GrkZone zone) {
       if (objW == zone.minWidth)
            return;

       String str = "99";
       int  ext = 0;
       if (zone.sampleList.size() >= 100) 
           ext = 2;
       if (zone.sampleList.size() >= 120) 
           str = "999";
       if (zone.gilsonNum)
            str = "H12";
       int tw = orgFm.stringWidth(str) + ext;
       int th = orgFm.getHeight();
       int w = zone.minWidth;
       float fh;

       if (SmsSample.isImageSample()) {
            if (SmsSample.imageWidth > 1)
                w = SmsSample.imageWidth;
       }

       objW = w;
       if ((w - th > 4) && (w - tw > 6)) {
            curFont = orgFont;
            curFm = orgFm;
            fontH2 = fontH;
            return;
       }
       tw = curFm.stringWidth(str) + ext;
       fh = (float) fontH - 2.0f;
       if (w - tw < 6)
            fh =  (float) fontH2 - 1.0f;
       while (fh >= 7) {
            curFont = orgFont.deriveFont(fh);
            curFm = tray.getFontMetrics(curFont);
            tw = curFm.stringWidth(str) + ext;
            th = curFm.getHeight();
            if ((w - th > 4) && (w - tw > 6)) {
                fontH2 = (int) fh;
                break;
            }
            fh = fh - 1.0f;
       }
   }

   public void paintSample(Graphics g, boolean paintSelected) {
       GrkZone z;
       Vector<SmsSample> oList;
       int x, y, w, i, k, m, tw, fh, fw, fw2, fa;
       int textY;
       int count;
       boolean  marvin = false;
 
       count = zoneShowList.size(); // the number of zones
       if (count < 1)
           return;

       SmsSample.setSampleFont(curFont);
       SmsSample.setFontMetrics(curFm);
       z = zoneShowList.elementAt(0);
       if (shiftSelect) {
           // if (z.trayType != GRK12) {
           if (!bRandomPick) {
               if (hilitList.size() > 1)
                   SmsUtil.linkSelectList((Graphics2D)g, hilitZone);
           }
       }
       for (i = 0; i < count; i++) {
           z = zoneShowList.elementAt(i);
           adjustFont(z);
           fh = curFm.getHeight();
           fa = curFm.getAscent();
           fw = curFm.stringWidth("A");
           fw2 = fw;
           oList = z.getSampleList();
           if (z.trayType == GRK49 || z.trayType == GRK97)
               marvin = true;
           if (count > 1 || marvin) {
              tw = orgFm.stringWidth(z.idStr);
              if (tw > (z.locX2 - z.locX)) {
                  g.setFont(curFont);
                  tw = curFm.stringWidth(z.idStr);
              }
              else {
                  g.setFont(orgFont);
                  fw = orgFm.stringWidth("H");
              }
              x = (z.locX2 - z.locX - tw) / 2;
              if (x < 0)
                 x = 0;
              y = z.locY - 4;
              if (count <= 1) {
                 y = fh + 2;
                 if (y >= z.locY)
                    y = z.locY - 4;
              }
              // g.setColor(Color.black);
              g.setColor(labelColor);
              g.drawString(z.idStr, z.locX + x, y);
           }
           g.setFont(curFont);
           m = oList.size();
           if (count <= 1) {
               if (fw < 16)
                   fw = 16;
           }
           else {
               if (fw < 10)
                   fw = 10;
           }
           SmsSample o = oList.elementAt(0);
           textY = (o.width - fh) / 2 + fa;
           for (k = 0; k < m; k++) {
              o = oList.elementAt(k);
              o.paint(g, curFm, textY);
              if (marvin) {
                 w = o.width;
                 if (o.col == 1 || o.row == 8) {
                     // g.setColor(Color.black);
                     g.setColor(labelColor);
                     if (o.col == 1) {
                         x = o.locX - fw;
                         y = o.locY + (w - fh) / 2 + fa;
                         g.drawString(marvinRows[o.row], x, y);
                     }
                     if (o.row == 8) {
                         x = o.locX + (w - fw2) / 2;
                         y = o.locY + w + fh;
                         g.drawString(marvinCols[o.col], x, y);
                     }
                 }
              }
           }
       }
       if (paintSelected)
           hilitSelect();
   }

   public void paintSample(Graphics g) {
        paintSample(g, true);
   }

   private void addSelectZone(GrkZone z) {
       if (hilitList.size() < 1)
           return;
       if (!selectedZones.contains(z))
           selectedZones.add(z);
   }
 

   public boolean hilitSelect() {
       if (hilitList.size() < 1)
          return false;
       Graphics gr = tray.getGraphics();
       SmsUtil.hilitSample(gr, hilitList);
       gr.dispose();
       return true;
   }

   public boolean setSelect(int x, int y, int key, int clicks) {
       SmsSample obj;
       int i, k, m, d, s;
       GrkZone z;
       Vector<SmsSample> oList;
       boolean shiftKey = false;
       boolean ctrlKey = false;
       boolean bDebug = SmsUtil.getDebugMode();

       if (bDebug) {
           System.out.println(" setSelect xy: "+x+" "+y);
           System.out.println(" tray type: "+trayType);
       }
       z = null;
       curSample = null;
       for (i = 0; i < zoneShowList.size(); i++) {
           z = zoneShowList.elementAt(i);
           if (x > z.locX && x < z.locX2) {
               if (y > z.locY && y < z.locY2)
                   break;
           }
           z = null;
       }
       if (z == null)
           return false;
       oList = z.getSampleList();
       m = oList.size();
       s = -1;
       obj = null;
       for (k = 0; k < m; k++) {
            obj = oList.elementAt(k);
            d = x - obj.locX;
            if (d >= 0 && d < obj.width) {
               d = y - obj.locY;
               if (d >= 0 && d < obj.width) {
                   s = obj.getId();
                   break;
               }
           }
       }
       if (s < 0)
           return false;
       SmsUtil.showSampleInfo(obj);
       if (clicks >= 2) {
           return false;
       }
       if (!SmsUtil.isUsable(obj))
           return false;
       if (z.trayType == GRK12 || z.trayType == GRK49 || z.trayType == GRK97)
           bRandomPick = true;
       else
           bRandomPick = false;
       if (bDebug)
           System.out.println(" selected loc: "+s+"  selected: "+obj.selected);
       curSample = obj;
       if ((key & Event.CTRL_MASK) != 0)
           ctrlKey = true;
       else if ((key & Event.SHIFT_MASK) != 0)
           shiftKey = true;
       if (ctrlKey) {
           if (!selectedZones.contains(z))
               selectedZones.add(z);
       }
       else {
           if (selectedZones.size() > 1)
               clearSelect();
       }
       hilitZone = z;
       if (bRandomPick) {
           SmsSample obj1 = getFirstSelectObj();
           if (obj1 != null)
               lastSelected = obj1.id;
       }
       if (bDebug)
           System.out.println(" last selected loc: "+lastSelected);
       if (shiftKey && lastSelected >= 0) {
           if (s != lastSelected) {
               zoneMultiSelect();
               addSelectZone(z);
           }
           return true;
       }
       if (ctrlKey) {
           if (bRandomPick) {
              addCtrlSelect(obj);
              addSelectZone(z);
           }
           else if (shiftSelect) {
              boolean retv = updateShiftList(obj);
              addSelectZone(z);
              return retv;
           }
       }
       else {
           if (obj.selected)
              k = 1;
           else
              k = 0;
           clearSelect(false);
           if (k == 1)
              obj.selected = true;
           if (bRandomPick)
               addCtrlSelect(obj);
           else {
               hilitList.removeElement(obj);
               if (obj.selected) {
                    obj.setSelect(false);
               }
               else {
                    obj.setSelect(true);
                    hilitList.addElement(obj);
               }
           }
       }
       sb.delete(1, sb.length());
       prevSample = setFirstSelectObj();
       if (prevSample != null) {
            lastSelected = prevSample.id;
            sb.append(prevSample.idStr);
            m = hilitList.size();
            for (k = 0; k < m; k++) {
               obj = hilitList.elementAt(k);
               if (obj != null && obj != prevSample) {
                   if (obj.selected) {
                      sb.append(", ");
                      sb.append(obj.idStr);
                   }
               }
            }
       }
       else {
            prevSample = null;
            lastSelected = -1;
       }
       if (bDebug)
            System.out.println(" first selected loc: "+lastSelected);
       SmsUtil.showSelected(sb.toString());
       addSelectZone(z);
       return true;
   }

   private boolean selectSample(int id) {
       SmsSample obj;
       int i, k, key;
       boolean bGot;
       GrkZone z;
       Vector<SmsSample> oList;

       obj = null;
       bGot = false;
       key = 0;
       if (lastSelected > 0)
           key = Event.CTRL_MASK; 
       for (i = 0; i < zoneShowList.size(); i++) {
           z = zoneShowList.elementAt(i);
           oList = z.getSampleList();
           for (k = 0; k < oList.size(); k++) {
                obj = oList.elementAt(k);
                if (id == obj.getId()) {
                     bGot = true;
                     break;
                }
           }
           if (bGot)
               break;
       }
       if (!bGot)
           return bGot;
       if (obj.selected) {
           if (hilitList.contains(obj))
               return false;
           obj.setSelect(false);
       }
       bGot = setSelect(obj.locX + 1, obj.locY + 1, key, 1); 
       return bGot;
   }

   public void clearAllSample() {
       if (zoneList == null)
          return;
       for (int i = 0; i < zoneList.size(); i++) {
           GrkZone z = zoneList.elementAt(i);
           Vector<SmsSample> v = z.getSampleList();
           for (int k = 0; k < v.size(); k++) {
              SmsSample  s = v.elementAt(k);
              s.clear();
           } 
       }
   }

   public void invalidateAllSample() {
       if (zoneList == null)
          return;
       for (int i = 0; i < zoneList.size(); i++) {
           GrkZone  z = zoneList.elementAt(i);
           Vector<SmsSample> v = z.getSampleList();
           for (int k = 0; k < v.size(); k++) {
              SmsSample  s = v.elementAt(k);
              s.inValidate();
           } 
       }
   }

   public void revalidateAllSample() {
       if (zoneList == null)
          return;
       for (int i = 0; i < zoneList.size(); i++) {
           GrkZone z = zoneList.elementAt(i);
           Vector<SmsSample> v = z.getSampleList();
           for (int k = 0; k < v.size(); k++) {
              SmsSample  s = v.elementAt(k);
              s.reValidate();
           } 
       }
   }

   // check sample size to determin whether change sample or window size 
   public boolean setScale(int scale) {
       GrkZone  z;
       int i;

       if (scale == 0) {
           for (i = 0; i < zoneList.size(); i++) {
               z = zoneList.elementAt(i);
               z.setScale(scale);
           }
           return true;
       }
       boolean res = false;
       if (scale < 0) {
          res = true;
          for (i = 0; i < zoneShowList.size(); i++) {
              z = zoneShowList.elementAt(i);
              if (!z.setScale(scale))
                 res = false;
          } 
       }
       else {
          for (i = 0; i < zoneShowList.size(); i++) {
              z = zoneShowList.elementAt(i);
              if (z.setScale(scale))
                 res = true;
          } 
       }
       return res;
   }

   private void adjustZone() {
       if (zoneList == null)
          return;
       int i, k;
       int index = 1;
       float minX, minY;
       float maxX, maxY;
       float difX, difY;
       float diam;
       SmsSample s;
       GrkZone  z;
       for (i = 0; i < zoneList.size(); i++) {
           z = zoneList.elementAt(i);
           z.trayType = trayType;
           Vector<SmsSample> v = z.getSampleList();
           minX = 0;
           minY = 0;
           maxX = 0;
           maxY = 0;
           diam = 999;
           index = 1;
           for (k = 0; k < v.size(); k++) {
              s = v.elementAt(k);
              s.setId(index);
              s.rackId = rackId;
              s.zoneId = z.id;
              index++;
              s.newDiam = s.diam;
              s.orgx = s.orgx - s.diam / 2;
              s.orgy = s.orgy - s.diam / 2;
              if (s.orgx < minX)
                  minX = s.orgx;
              if (s.orgx+s.diam > maxX)
                  maxX = s.orgx + s.diam;
              if (s.orgy < minY)
                  minY = s.orgy;
              if (s.orgy + s.diam > maxY)
                  maxY = s.orgy + s.diam;
              if (s.diam < diam) {
                  if (diam != 999)
                     z.vDiam = true;
                  diam = s.diam;
              }
           }
           s = v.elementAt(0);
           z.width = maxX - minX;
           z.height = maxY - minY;
           z.newWidth = z.width;
           z.newHeight = z.height;
           z.diam = diam;
           z.newDiam = diam;
           difX = 0 - minX;
           difY = 0 - minY;
           z.difX = difX;
           z.difY = difY;
           z.orgx = z.orgx - difX;
           z.orgy = z.orgy - difY;
           for (k = 0; k < v.size(); k++) {
              s = v.elementAt(k);
              s.orgx = s.orgx + difX;
              s.orgy = s.orgy + difY;
           }
           z.maxX = maxX + difX;
           z.maxY = maxY + difY;
       }
       minX = 0;
       minY = 0;
       k = zoneList.size();
       for (i = 0; i < k; i++) {
           z = zoneList.elementAt(i);
           if (z.orgx < minX)
               minX = z.orgx;
           if (z.orgy < minY)
               minY = z.orgy;
       }
       difX = 0 - minX;
       difY = 0 - minY;
       for (i = 0; i < k; i++) {
           z = zoneList.elementAt(i);
           z.orgx = z.orgx + difX;
           z.orgy = z.orgy + difY;
           z.setupRowCol(name, gilsonNum);
       }
   }

   public void dispAllZone() {
       GrkZone z;

       zoneShowList.clear();
       for (int i = 0; i < zoneList.size(); i++) {
           z = zoneList.elementAt(i);
           z.visible = true;
           z.setScale(0);
           zoneShowList.add(z);
       }
   }

   public void dispZone(int n) {
       if (n <= 0)
           return;
       zoneShowList.clear();
       GrkZone z;
       for (int i = 0; i < zoneList.size(); i++) {
           z = zoneList.elementAt(i);
           z.visible = false;
           if (n == z.getId()) {
              z.visible = true;
              z.setScale(0);
              zoneShowList.add(z);
           }
       }
   }

   public void dispZone() {
       dispZone(1);
   }

   public SmsSample locateSample(int x, int y) {
       int i, k, d;
       GrkZone z;
       Vector<SmsSample> oList;

       z = null;
       k = -1;
       for (i = 0; i < zoneShowList.size(); i++) {
           z = zoneShowList.elementAt(i);
           if (x > z.locX && x < z.locX2) {
               if (y > z.locY && y < z.locY2) {
                   k = i;
                   break;
                }
           }
       }
       if (k < 0)
           return null;
       oList = z.getSampleList();
       k = oList.size();
       for (i = 0; i < k; i++) {
            SmsSample obj = oList.elementAt(i);
            d = x - obj.locX;
            if (d >= 0 && d < obj.width) {
               d = y - obj.locY;
               if (d >= 0 && d < obj.width) {
                   return obj;
               }
           }
       }
       return null;
   }

}

