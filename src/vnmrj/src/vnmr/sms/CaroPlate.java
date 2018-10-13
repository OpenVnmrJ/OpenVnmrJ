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
import java.awt.*;
import java.util.*;


public class CaroPlate extends GenPlateIF implements  SmsDef
{
   public int type = 0;
   public int firstSample = 1;
   public int sampleNum = 1;
   public int lastSample = 1;
   public int objW = 0;
   public int fontH = 16;
   public int fontH2 = 16;
   public int lastSelected = 0;
   public int startAngle = 036;
   public SmsSample sList[] = null;
   private Font orgFont; 
   private Font curFont; 
   private FontMetrics orgFm; 
   private FontMetrics curFm; 
   private CarouselLayout caroLayout = null; 
   private JComponent tray; 
   private Vector<SmsSample> hilitList;

   public CaroPlate(JComponent t, int n) {
       this.tray = t;
       this.sampleNum = n;
       this.firstSample = 1;
       this.lastSample = n;
       sList = new SmsSample[n+1];
       for (int i = 0; i <= n; i++) {
            sList[i] = new SmsSample(i);
       }
       orgFont = new Font("Dialog", Font.BOLD, fontH);
       orgFm = t.getFontMetrics(orgFont);
       curFont = orgFont;
       curFm = orgFm;
       caroLayout = new CarouselLayout();
       caroLayout.setSampleList(sList);
       caroLayout.setStartSample(1);
       caroLayout.setLastSample(n);
       hilitList = new Vector<SmsSample>();
       dateOfQfiles = new long[NUMQ];
       for (int k = 0; k < NUMQ; k++)
            dateOfQfiles[k] = 0;
   }


   public SmsSample[] getSampleList() {
       return sList;
   }

   public LayoutManager getLayoutMgr() {
       return caroLayout;
   }

   public SmsSample getSample(int zone, int num) {
/*
       if (zone != 0)
           return null;
*/
       if (num < 1 || num > sampleNum)
           return null;
       return sList[num];
   }

   public void setZone(int s) {
   }

   public int getSampleNum() {
       return sampleNum;
   }

   public void clearAllSample() {
       for (int k = 0; k <= sampleNum; k++) {
           sList[k].clear();
       }
   }

   public void invalidateAllSample() {
       for (int k = 0; k <= sampleNum; k++) {
           sList[k].inValidate();
       }
   }

   public void revalidateAllSample() {
       for (int k = 0; k <= sampleNum; k++) {
           sList[k].reValidate();
       }
   }

   private void updateSelectList() {
       SmsSample obj;
       hilitList.clear();
       for (int k = 0; k <= sampleNum; k++) {
           obj = sList[k];
           obj.startItem = false;
           if (obj.selected)
              hilitList.add(obj);
       }
       if (hilitList.size() > 0) {
           obj = hilitList.elementAt(0);
           obj.startItem = true;
           // if (lastSelected < 0)
               lastSelected = obj.id;
       }
       else
           lastSelected = -1;
   }

   public Vector<SmsSample> getSelectList() {
       return hilitList;
   }

   public void clearSelect(boolean clearLast) {
       SmsSample obj;
       for (int k = 0; k <= sampleNum; k++) {
           obj = sList[k];
           obj.selected = false;
           obj.startItem = false;
       }
       hilitList.clear();
       if (clearLast)
           lastSelected = -1;
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
           return false;
        }
        return false;
   }


   public void adjustFont() {
       SmsSample obj = sList[lastSample];
       int w = obj.width;   
       float fh;

       if (SmsSample.isImageSample()) {
            if (SmsSample.imageWidth > 1)
                w = SmsSample.imageWidth;
       }

       if (objW == w)
            return;
       objW = w;

       int tw = orgFm.stringWidth(obj.idStr);
       int th = orgFm.getHeight();
       if ((w - th > 4) && (w - tw > 6)) {
            curFont = orgFont;
            curFm = orgFm;
            fontH2 = fontH;
            return;
       }
       tw = curFm.stringWidth(obj.idStr);
       fh = (float) fontH - 2.0f;
       if (w - tw < 6)
            fh =  (float) fontH2 - 1.0f;
       while (fh >= 7) {
            curFont = orgFont.deriveFont(fh);
            curFm = tray.getFontMetrics(curFont);
            tw = curFm.stringWidth(obj.idStr);
            th = curFm.getHeight();
            if ((w - th > 4) && (w - tw > 6)) {
                fontH2 = (int) fh;
                break;
            }
            fh = fh - 1.0f;
       }
   }

   public void paintSample(Graphics g, boolean paintSelected) {
       int k = firstSample;
       int w;
       int th;
       SmsSample obj;
       adjustFont();
       obj = sList[lastSample];
       w = obj.width;   
       g.setFont(curFont);
       SmsSample.setSampleFont(curFont);
       SmsSample.setFontMetrics(curFm);
       th = (w - curFm.getHeight()) / 2 + curFm.getAscent();
       while (k <= lastSample) {
           obj = sList[k];
           obj.paint(g, curFm, th);
           k++;
       }
/****
       int x, y, x2, y2;
       int tw = curFm.stringWidth(obj.idStr);
       while (k <= lastSample) {
           obj = sList[k];
           g.setColor(sampleColor[obj.status]);
           w = obj.width;   
           x = obj.locX;
           y = obj.locY;
           g.fillOval(x, y, w, w);
           g.setColor(numColor[obj.status]);
           tw = curFm.stringWidth(obj.idStr);
           x2 = x + (w - tw) / 2;
           y2 = y + th;
           g.drawString(obj.idStr, x2, y2);
           k++;
       }
       // if (hilitList.size() > 0)
       //   SmsUtil.hilitSample(g, hilitList);
*****/
       if (paintSelected)
           hilitSelect();
   }

   public void paintSample(Graphics g) {
        paintSample(g, true);
   }

   public boolean hilitSelect() {
      if (hilitList.size() < 1)
          return false;
      Graphics gr = tray.getGraphics();
      SmsUtil.hilitSample((Graphics2D) gr, hilitList);
      gr.dispose();
      return true;
   }

   public boolean setSelect(int x, int y, int key, int clicks) {
       SmsSample obj = null;
       int d, s, k;
       boolean shiftKey = false;
       boolean ctrlKey = false;
       s = -1;
       for (k = firstSample; k <= lastSample; k++) {
           obj = sList[k];
           d = x - obj.locX;
           if (d >= 0 && d < obj.width) {
               d = y - obj.locY;
               if (d >= 0 && d < obj.width) {
                   s = k;
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
       if ((key & Event.CTRL_MASK) != 0)
           ctrlKey = true;
       else if ((key & Event.SHIFT_MASK) != 0)
           shiftKey = true;
       if (!SmsUtil.isUsable(obj)) {
          if (!shiftKey)
              return false;
          if (lastSelected < 0)
              return false;
       }
       if (ctrlKey) {
           if (obj.selected) {
              obj.selected = false;
           }
           else
              obj.selected = true;
           lastSelected = s;
       } 
       else if (shiftKey) {
           clearSelect(false);
           if (lastSelected <= 0) {
              obj.selected = true;
              lastSelected = s;
           }
           else {
              if (s < lastSelected) {
                 d = lastSelected;
              }
              else {
                 d = s;
                 s = lastSelected;
              }
              for (x = s; x <= d; x++) {
                 obj = sList[x];
                 if (SmsUtil.isUsable(obj))
                     obj.selected = true;
              }
           }
       } 
       else {
           clearSelect(false);
           if (s != lastSelected) {
              obj = sList[s];
              obj.selected = true;
              lastSelected = s;
           }
           else
              lastSelected = -1;
       } 
       updateSelectList();
       return true; 
   }

   public SmsSample locateSample(int x, int y) {
       int d, k;
       for (k = firstSample; k <= lastSample; k++) {
           SmsSample obj = sList[k];
           d = x - obj.locX;
           if (d >= 0 && d < obj.width) {
               d = y - obj.locY;
               if (d >= 0 && d < obj.width)
                   return obj;
           }
       } 
       return null;
   }

   private boolean selectSample(int id) {
       SmsSample obj;
       int key;
       boolean bGot;

       if (id > lastSample)
           return false;
       obj = sList[id];
       if (obj.selected) {
           if (hilitList.contains(obj))
               return false;
           obj.setSelect(false);
       }
       key = 0;
       if (lastSelected > 0)
           key = Event.CTRL_MASK;
       bGot = setSelect(obj.locX + 1, obj.locY + 1, key, 1);
       return bGot;
   }

}

