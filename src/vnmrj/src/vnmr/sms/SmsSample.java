/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.util.*;
import java.awt.*;
import java.awt.image.BufferedImage;

public class SmsSample
   implements SmsDef
{
   public int  id;
   public int  status;
   public int  locX;
   public int  locY;
   public int  row;
   public int  col;
   public int  width;
   public int  height;
   public int  zoneId;
   public int  rackId;
   public int  infoQ;
   public int  studyNum;
   public int  statusNum;
   public int  centerX, centerY;
   public float orgx, orgy;
   public float diam;
   public float newDiam;
   public boolean selected;
   public boolean startItem;
   public boolean bPopupItem; // popup menu target
   public String  name;
   public String  idStr;
   public String  rowStr;
   public String  colStr;
   public Vector<SampleInfo> infoList;
   private int[] statusList;
   private int  statusAngle = 360; 
   private int  startAngle = 0;

   public static boolean bUseImage;
   public static Font sampleFont = null;
   public static FontMetrics fontMetrics = null;
   public static int sampleWidth = 0;
   public static int sampleHeight = 0;
   public static int imageWidth = 0;
   public static int imageHeight = 0;
   public static int imageXoff = 0;
   public static int imageYoff = 0;
   public static int lineThick = 3;
   public static BufferedImage sampleImg = null;


   public SmsSample(int n) {
       this.id = n;
       this.zoneId = 1;
       this.rackId = 1;
       this.studyNum = 0; 
       locX = 0;
       locY = 0;
       row = 0;
       col = 0;
       width = 0;
       height = 0;
       status = OPEN;
       infoQ = 0;
       idStr = Integer.toString(n);
       selected = false;
       startItem = false;
   }

   public void setSelect(boolean s) {
       selected = s;
       if (!s)
           startItem = false;
   }

   public void setId(int n) {
       id = n;
       idStr = Integer.toString(n);
   }

   public int getId() {
       return id;
   }

   public boolean getSelect() {
       return selected;
   }

   public static void setSampleFont(Font ft) {
       if (ft != null)
           sampleFont = ft;
   }

   public static void setFontMetrics(FontMetrics fm) {
       if (fm != null)
           fontMetrics = fm;
   }

   public static boolean isImageSample() {
        return bUseImage;
   }

   public void addInfo(SampleInfo info) {
       if (infoList == null)
          infoList = new Vector<SampleInfo>();
       infoList.add(info);
       studyNum++;
   }

   public void setBounds(int x, int y, int w, int h) {
       locX = x;
       locY = y;
       width = w;
       height = h;
       centerX = x + w / 2;
       centerY = y + h / 2;
       if (w != sampleWidth || h != sampleHeight) {
            sampleWidth = w;
            sampleHeight = h;
            sampleImg = SmsUtil.getSampleImage(w, h);
            if (sampleImg != null) {
               imageWidth = sampleImg.getWidth(null);
               imageHeight = sampleImg.getHeight(null);
               imageXoff = (w - imageWidth) / 2;
               imageYoff =  (h - imageHeight) / 2;
               bUseImage = true;
               if (imageWidth > 30)
                   lineThick = 3;
               else
                   lineThick = 2;
            }
            else {
               if (w > 20)
                   lineThick = 4;
               else
                   lineThick = 3;
               bUseImage = false;
            }
       }
   }

   public Vector<SampleInfo> getInfoList() {
       return infoList;
   }

   public void clear() {
       int i;

       status = OPEN;
       // selected = false;
       studyNum = 0;
       statusNum = 0;

       if (statusList != null) {
          for (i = 0; i < statusList.length; i++)
              statusList[i] = OPEN;
       }
       if (infoList == null)
          return;
       for (i = 0; i < infoList.size(); i++) {
          SampleInfo s = infoList.elementAt(i);
          if (s != null)
              s.clear();
       }
       infoList.clear();
   }

   public void inValidate() {
       int i;

       status = OPEN;
       studyNum = 0;
       statusNum = 0;

       if (statusList != null) {
          for (i = 0; i < statusList.length; i++)
              statusList[i] = OPEN;
       }
       if (infoList != null) {
           for (i = 0; i < infoList.size(); i++) {
               SampleInfo s = infoList.elementAt(i);
               if (s != null)
                   s.inValidate();
           }
       }
   }

   private void addStatus(int s) {
       int i, num;

       if (statusList == null) {
           statusList = new int[4];
           for (i = 1; i < 4; i++)
               statusList[i] = OPEN;
           statusList[0] = s;
           statusNum = 1;
           return;
       }
       num = statusList.length;
       for (i = 0; i < num; i++) {
           if (statusList[i] == s)
               return;
       }
       for (i = 0; i < num; i++) {
           if (statusList[i] == OPEN) {
               statusList[i] = s; 
               statusNum++;
               return;
           }
       }
       int[] newList = new int[num + 2];
       for (i = 0; i < num; i++)
           newList[i] = statusList[i];
       newList[num] = s;
       newList[num+1] = OPEN;
       statusNum++;
       statusList = newList;
   }

   public void reValidate() {
       int i;

       status = OPEN;
       studyNum = 0;
       statusNum = 0;
       if (statusList != null) {
          for (i = 0; i < statusList.length; i++)
              statusList[i] = OPEN;
       }
       if (infoList == null || infoList.size() < 1)
          return;
       for (i = 0; i < infoList.size(); i++) {
           SampleInfo info = infoList.elementAt(i);
           if (info != null) {
               if (info.isValidated()) {
                   studyNum++;
                   if (info.status != OPEN) {
                       addStatus(info.status);
                   }
                   if (info.status > status)
                      status = info.status;
               }
           }
       }
   }

   public void paintImage(Graphics g, FontMetrics fm, int textY) {
       int x = locX + imageXoff;
       int y = locY + imageYoff;
       int x2 = x + imageWidth;
       int y2 = y + imageHeight;
       int angle, i, cx, cy, cLen;
       int deg, skip;
       int xstatus = OPEN;

       cx = x - lineThick;
       cy = y - lineThick;
       cLen = imageWidth + lineThick * 2;
       if (bPopupItem) {
           g.setColor(new Color(8, 187, 249));
           g.fillOval(cx, cy, cLen, cLen);
       }
       else if (selected) {
           if (startItem) {
               angle = startAngle; 
               deg = 10;
               skip = 20;
               g.setColor(Color.white);
               g.fillOval(cx, cy, cLen, cLen);

               g.setColor(new Color(165, 165, 165));
               if (angle > 360)
                   angle = angle - 360;
               for (i = 0; i < 18; i++) {
                   g.fillArc(cx, cy, cLen, cLen, angle, deg);
                   angle += skip;
                   if (angle > 360)
                        angle = angle - 360;
               }
               startAngle -= 3;
               if (startAngle < 0)
                   startAngle = startAngle + 360;
           }
           else
               xstatus = SELECTED;
       }
       else {
           if (statusList != null) {
               for (i = 0; i < statusList.length; i++) {
                   if (statusList[i] > xstatus) {
                        xstatus = statusList[i];
                        status = statusList[i];
                   }
               }
           }
       }
       if (xstatus > OPEN) {
           g.setColor(SmsColorEditor.getColor(xstatus));
           g.fillOval(cx, cy, cLen, cLen);
       }

       g.drawImage(sampleImg, x, y, x2, y2, 0, 0, imageWidth, imageHeight, null);

       g.setColor(Color.darkGray);
       int tw = fm.stringWidth(idStr);
       int tx = locX + (width - tw) / 2;
       int ty = locY + textY;
       if (tx > locX)
           g.drawString(idStr, tx, ty);
   }

   public void paint(Graphics g, FontMetrics fm, int textY) {
       if (bUseImage) {
           paintImage(g, fm, textY);
           return;
       }
       int cx = locX - lineThick;
       int cy = locY - lineThick;
       int cLen = width + lineThick * 2;
       int angle, deg, i, skip;

       if (bPopupItem) {
           // g.setColor(new Color(8, 187, 249));
           g.setColor(SmsColorEditor.getColor(SELECTED));
           g.fillOval(cx, cy, cLen, cLen);
       }
       else if (selected) {
           if (startItem) {
               angle = startAngle;
               deg = 10;
               skip = 20;
               g.setColor(Color.white);
               g.fillOval(cx, cy, cLen, cLen);

               // g.setColor(new Color(165, 165, 165));
               g.setColor(new Color(128, 128, 128));
               if (angle > 360)
                   angle = angle - 360;
               for (i = 0; i < 18; i++) {
                   g.fillArc(cx, cy, cLen, cLen, angle, deg);
                   angle += skip;
                   if (angle > 360)
                        angle = angle - 360;
               }
               startAngle -= 3;
               if (startAngle < 0)
                   startAngle = startAngle + 360;
           }
           else {
               g.setColor(SmsColorEditor.getColor(SELECTED));
               g.fillOval(cx, cy, cLen, cLen);
           }
       }
       if (statusNum < 2) {
           // g.setColor(sampleColor[status]);
           g.setColor(SmsColorEditor.getColor(status));
           g.fillOval(locX, locY, width, width);
       }
       else {
           statusAngle = 360 / statusNum;
           angle = 90;
           for (i = 0; i < statusNum; i++) {
              g.setColor(SmsColorEditor.getColor(statusList[i]));
              g.fillArc(locX, locY, width, width, angle, statusAngle+2);
              angle += statusAngle;
              if (angle > 360)
                  angle -= 360;
           }
       }
       if (statusNum < 1)
           g.setColor(numColor[OPEN]);
       else
           g.setColor(Color.darkGray);
       int tw = fm.stringWidth(idStr);
       int tx = locX + (width - tw) / 2;
       int ty = locY + textY;
       if (tx > locX)
           g.drawString(idStr, tx, ty);
   }

   public int cursorAt(int x, int y) {
       if (x < 0 || y < 0) {
          return -1;
       }
       if (bUseImage || statusNum < 2)
          return status;

       double dx = x - centerX; 
       double dy = centerY - y; 
       dx = Math.atan2(dy, dx);
       int deg = (int) (dx * 57.29577951308232087679);
       if (deg < 0)
           deg = 360 + deg;
       if (deg >= 90)
           deg = deg - 90;
       else
           deg = deg + 270;
       deg = deg / statusAngle;
       return statusList[deg];
   }

}

