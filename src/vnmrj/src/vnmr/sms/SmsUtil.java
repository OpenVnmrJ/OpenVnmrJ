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
import java.awt.*;
import java.util.*;
import java.text.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import javax.swing.Timer;

import vnmr.util.*;
import vnmr.ui.shuf.*;

public final class SmsUtil implements SmsDef
{
   private static SmsPanel panel = null;
   private static RackInfo rackInfo = null;
   private static SmsTray tray = null;
   private static GrkParser grkParser = null;
   private static ZoneCreater zCreater = null;
   private static GenPlateIF curPlate = null;
   private static SmsInfoPanel infoPanel = null;
   private static int hpos = 0;
   private static int hcolor = 0;
   private static int rackId = 0;
   private static int zoneId = 0;
   private static int curStatus = 0;
   private static int colorIndex = 0;
   private static int xPnts[] = new int[7];
   private static int yPnts[] = new int[7];
   private static int sOrient = BYROW;  // selection order(horizontal or vertical)
   private static int sOrder = L_FRONT;  // selection order (from left or right)
   private static int sOrder2 = T_FRONT;  // form top or bottom
   // private static int studySearchIndex;
   private static int timerCount = 0;
   private static long timeOfSample = 0;
   private static long timeOfPlate = 0;
   private static boolean bDebug = false;
   private static boolean bReusable = false;
   private static boolean bUpdating = false;
   private static boolean bTest = false;
   private static Vector<SmsSample> hList = null;  // selected sample list
   private static String autoDir = null;
   private static String autoPath = null;
   private static String curUser = null;
   private static String user = null;
   private static String studyId = null;
   private static String sampleName = null;
   private static String page = null;
   private static String notebook = null;
   private static String solvent = null;
   private static String statusStr = null;
   private static String sampleDate = null; // the date of sample created
   private static String barCode = null;
   private static String vnmrAddr = null;
   private static String sampleFile = null;
   private static ShufDBManager dbManager = null;
   private static java.sql.ResultSet dbResult = null;
   private static StreamTokenizerQuotedNewlines stok = null;
   private static Timer updateTimer = null;
   private static Vector<sampleFileRecord> sampleFileList = null;
   private static Vector<sampleFileRecord> newSampleFileList;
   private static sampleFileRecord fileRecord;
   private static SimpleDateFormat formatter = new SimpleDateFormat ("MMM d, yyyy HH:mm:ss");

   // private static String[] qNames = {"doneQ", "enterQ", "psgQ" };
   // private static int[] qOrder = { DONEQ, ENTERQ, PSGQ };
/*
   private static String[] attrs = {"SAMPLE#", "USER", "RACK", "ZONE",
             "MACRO", "SOLVENT", "USERDIR", "DATA", "STATUS", "TEXT" };
*/
   private static String[] attrs = {"loc_", "vzone_", "operator_","studyid_",
             "samplename", "page", "studystatus", "solvent", "notebook",
             "barcode", "vrack_" };

   private static int[] sKeys = { QUEUED, NIGHTQ, ACTIVE, DONE, DONE, ERROR };
   private static String[] sNames = {"Queued", "NightQueue", "Active", "Complete", 
             "Completed", "Error" };

   private static Image[] sampleImges = null;
   private static int[] sampleImgWidths = null;
   private static int[] sampleImgHeights = null;

   public SmsUtil() {
   }

   public static void setPanel(SmsPanel p) {
       panel = p;
   }

   public static JPanel getPanel() {
       return (JPanel) panel;
   }

   public static void setDebugMode(boolean b) {
       if (b)
	  System.out.println("SMS: turn on debug. ");
       else if (bDebug)
	  System.out.println("SMS: turn off debug. ");
       bDebug = b;
   }

   public static boolean getDebugMode() {
       return bDebug;
   }

   public static void setReusable(boolean b) {
       bReusable = b;
   }

   public static boolean getTestMode() {
       return bTest;
   }

   public static void setTestMode(boolean b) {
       bTest = b;
   }

   public static void setInfoPanel(SmsInfoPanel p) {
       infoPanel = p;
   }

   public static boolean isUsable(SmsSample s) {
       if (s.status == OPEN)
           return true;
       return bReusable;
   }

   public static void setVnmrAddr(String s) {
       vnmrAddr = s;
   }

   public static String getVnmrAddr() {
       return vnmrAddr;
   }

   public static void setTray(SmsTray p) {
       tray = p;
       if (infoPanel != null)
          infoPanel.showSampleInfo(null);
   }

   public static SmsTray getTray() {
       return tray;
   }

   public static void setRackInfo(RackInfo p) {
       rackInfo = p;
   }

   public static RackInfo getRackInfo() {
       return rackInfo;
   }

   public static GrkParser getGrkParser() {
       if (grkParser == null)
            grkParser = new GrkParser();
       return grkParser;
   }

   public static ZoneCreater getZoneCreater() {
       if (zCreater == null)
           zCreater = new ZoneCreater();
       return zCreater;
   }

   public static void submitTest() {
       if (panel != null)
          panel.submit(null, "test");
   }

   public static void setVnmrInfo(int type, String message) {
      if (panel != null)
         panel.setVnmrInfo(type, message);
   }

   public static void showSampleInfo(SmsSample s) {
      if (panel != null)
         panel.showSampleInfo(s);
   }

   public static void showSelected(String s) {
      if (panel != null)
         panel.showSelected(s);
   }

   public static void showGrkPlate(String name) {
      if (panel != null) {
         panel.showGrkPlate(name);
      }
   }

   public static void setZone(String name) {
      if (panel != null)
         panel.setZone(name);
   }

   public static void setOrient(int n) {
       sOrient = n;
   }

   public static int getOrient() {
       return sOrient;
   }

   public static void setOrder(int n) {
       sOrder = n;
   }

   public static int getOrder() {
       return sOrder;
   }

   public static void setOrder2(int n) {
       sOrder2 = n;
   }

   public static int getOrder2() {
       return sOrder2;
   }

   public static void runningSample(Graphics2D g, SmsSample s) {
       int x = s.locX;
        int y = s.locY;
        int x2 = x - 1;
        int y2 = y - 1;
        int w = s.width;
        int w2 = w + 2;
        int p1;

        for (int k = 0; k < 3; k++) {
           g.setColor(hColor[k]);
           p1 = hpos + 30 * k;
           if (p1 > 360)
              p1 = p1 - 360;
           for (int n = 0; n < 4; n++) {
              g.drawArc(x, y, w, w, p1, 30);
              g.drawArc(x2, y2, w2, w2, p1, 30);
              p1 = p1 + 90;
              if (p1 > 360)
                 p1 = p1 - 360;
           }
        }
        hpos = hpos - 10;
/*
        if (hpos > 360)
            hpos = hpos - 360;
*/
        if (hpos < 0)
            hpos = hpos + 360;
   }


   public static void hilitSample(Graphics g, Vector<SmsSample> v) {
       int num = v.size();
       if (num < 1)
           return;
       if (Util.isMenuUp())
           return;
       if (tray != null) {
           if (SmsSample.isImageSample()) {
               return;
           }
       }
       boolean bFirst = true;
       int x, y, w, k;
       SmsSample obj = (SmsSample) v.elementAt(0);
       Graphics2D g2d = (Graphics2D)g;
       Color scolor = new Color(8, 187, 249); 

       g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

       for (k = 0; k < num; k++) {
           obj = (SmsSample) v.elementAt(k);
           if (!obj.selected)
              continue;
           if (bFirst) {
              runningSample(g2d, obj);
              bFirst = false;
           }
           else {
              x = obj.locX + 1;
              y = obj.locY + 1;
              w = obj.width - 2;
              // g.setColor(sampleColor[OPEN]);
              g.setColor(scolor);
              g.drawOval(x, y, w, w);
              x--;
              y--;
              w += 2;
              // g.setColor(Color.yellow);
              g.drawOval(x, y, w, w);
              x--;
              y--;
              w += 2;
              // g.setColor(Color.cyan);
              g.drawOval(x, y, w, w);
              x--;
              y--;
              w += 2;
              // g.setColor(Color.red);
              // g.drawOval(x, y, w, w);
           }
        }
   }

   public static void old_hilitSample(Graphics g, Vector<SmsSample> v) {
       int num = v.size();
       if (num < 1)
           return;
       if (Util.isMenuUp())
           return;
       SmsSample obj = (SmsSample) v.elementAt(0);
       int k, n,  w, hc;
       int x1, x2, startX;
       int y1, y2;
       int px1, px2;
       int py1, py2, hi;
       float hw;
       boolean bDraw = true;
       w = obj.width + 1;
       hw = w / 3;
       if (hw < 3)
          hw = 3;
       hi = (int) (hw + 0.5);
       if (hpos > hi)
          hpos = hi;
       y1 = 0;
       y2 = 0;
       x1 = 0;
       for (k = 0; k < num; k++) {
           obj = (SmsSample) v.elementAt(k);
           if (!obj.selected)
              continue;
           px1 = obj.locX - 1;
           px2 = px1 + w;
           py1 = obj.locY - 1;
           py2 = py1 + w;
           startX = hpos;
           hc = hcolor;
           if (num > 1 && !obj.startItem) {
                startX = hi / 2;
                hc = 0;
           }
           for (n = 0; n < 3; n++) {
                 x2 = px1 + startX + n * hi;
                 bDraw = true;
                 g.setColor(hColor[hc]);
                 while (bDraw) {
                        x1 = x2 - hi;
                        if (x1 < px1)
                            x1 = px1;
                        if (x2 > px2) {
                            y2 = py1 + x2 - px2;
                            x2 = px2;
                            bDraw = false;
                        }
                        if (x1 >= px2)
                            break;
                        g.drawLine(x1, py1, x2, py1);
                        g.drawLine(x1, py1+1, x2, py1+1);
                        x2 = x2 + w;
                     }
                     bDraw = true;
                     while (bDraw) {
                        y1 = y2 - hi;
                        if (y1 < py1)
                           y1 = py1;
                        if (y2 > py2) {
                           x1 = px2 - y2 + py2;
                           y2 = py2;
                            bDraw = false;
                        }
                        if (y1 >= py2)
                            break;
                        g.drawLine(px2, y1, px2, y2);
                        g.drawLine(px2-1, y1, px2-1, y2);
                        y2 = y2 + w;
                     }
                     bDraw = true;
                     while (bDraw) {
                        x2 = x1 + hi;
                        if (x2 > px2)
                           x2 = px2;
                        if (x1 < px1) {
                            y1 = py2 - px1 + x1;
                            x1 = px1;
                            bDraw = false;
                        }
                        if (x2 <= px1)
                            break;
                        g.drawLine(x1, py2, x2, py2);
                        g.drawLine(x1, py2-1, x2, py2-1);
                        x1 = x1 - w;
                     }
                     bDraw = true;
                     while (bDraw) {
                        y2 = y1 + hi;
                        if (y2 > py2)
                           y2 = py2;
                        if (y2 < py1)
                            break;
                        if (y1 < py1) {
                            y1 = py1;
                            bDraw = false;
                        }
                        g.drawLine(px1, y1, px1, y2);
                        g.drawLine(px1+1, y1, px1+1, y2);
                        y1 = y1 - w;
                     }
                     hc++;
                     if (hc > 2)
                         hc = 0;
           }
       }
       if (hi > 15)
          hpos += 4;
       else {
          if (hi > 6)
             hpos += 3;
          else
             hpos += 1;
       }
       if (hpos > hi) {
           hpos = hpos - hi;
           hcolor--;
           if (hcolor < 0)
              hcolor = 2;
       }
   }

   // when submit sample, set sample to pre-defined status
   public static void preQueueSamples(Vector<SmsSample> v, String status) {
       int num = v.size();
       for (int k = 0; k < num; k++) {
           SmsSample obj = v.elementAt(k);
           if (obj.selected) {
               // obj.statusStr = status;
               obj.status = getStatus(status);
           }
       }
   }

   private static void cleanSampleInfo() {
        curStatus = OPEN;
        user = null;
        studyId = null;
        sampleName = null;
        page = null;
        statusStr = null;
        solvent = null;
        notebook = null;
        sampleDate = null;
        barCode = null;
        sampleFile = null;
   }

   private static void fillSampleInfo(SmsSample s, boolean bCheckDate) {
       int k;

       if (studyId == null)
          return;
       if (bDebug)
          System.out.println("SMS: add sample: "+s.id+"  study id: '"+studyId+"' ");
       SampleInfo new_info = new SampleInfo();
       new_info.user = user;
       new_info.studyId = studyId;
       new_info.solvent = solvent;
       new_info.name = sampleName;
       new_info.notebook = notebook;
       new_info.page = page;
       new_info.statusStr = statusStr;
       new_info.date = sampleDate;
       new_info.status = curStatus;
       new_info.iTime = timeOfSample;
       new_info.barcode = barCode;
       new_info.fileName = sampleFile;
       new_info.validate();
       Vector<SampleInfo> v = s.infoList;
       if (v == null || v.size() <= 0) {
           s.addInfo(new_info);
           s.status = curStatus;
           s.studyNum = 1;
           return;
       }
       Vector<SampleInfo> newList = new Vector<SampleInfo>();

       int last = v.size();
       int num = 0;
       long newTime = timeOfSample;
       SampleInfo old;

       for (k = 0; k < last; k++) {
           // remove the same ID info and keep the latest one
           old = v.elementAt(k);
           if (old == null)
               continue;
           if (studyId.equals(old.studyId)) {
               if (bCheckDate) {
                    if (old.iTime <= newTime)
                         old.clear();
                    else {
                         new_info.clear();
                         v.setElementAt(null, k);
                         new_info = old;
                         old.validate();
                         newTime = old.iTime;
                    }
               }
               else
                    old.clear();
           }
       }
       for (k = 0; k < last; k++) {
           old = v.elementAt(k);
           if (old == null || old.studyId == null)
               continue;
           if (old.iTime <= newTime)
                newList.add(old);
           else {
                if (new_info != null && new_info.studyId != null)
                    newList.add(new_info);
                new_info = old;
                newTime = old.iTime;
            }
       }
       if (new_info != null && new_info.studyId != null)
           newList.add(new_info);

       v.clear();
       v = null;

       s.infoList = newList;
       curStatus = OPEN;
       /* set status to the highest one */
       num = 0;
       newTime = 0;
       last = newList.size();
       for (k = 0; k < last; k++) {
           old = newList.elementAt(k);
           if (old != null && old.isValidated()) {
               num++;
               if (old.status > curStatus)
                   curStatus = old.status;
/*
               else if (old.iTime > newTime) {
                   curStatus = old.status;
                   newTime = old.iTime;
               }
*/
           }
       }
       s.studyNum = num;
       s.status = curStatus;
   }

   private static void fillSampleInfo(SmsSample s) {
       fillSampleInfo(s, true);
   }

   private static void fillDummySample() {
        SmsSample sample;
	int i, k;

	i = 10;
	for (k = 0; k < 6; k++) {
           sample = curPlate.getSample(1, i); 
           if (sample == null)
		break;
           statusStr = sNames[k];
           curStatus = sKeys[k];
	   user = curUser;
	   fillSampleInfo(sample);
	   i++;
           sample = curPlate.getSample(1, i); 
           if (sample == null)
		break;
           curStatus = curStatus - 1;
	   user = "other";
	   fillSampleInfo(sample);
	   i++;
	}
   }

   public static SmsSample updateSample(String path, boolean checkDate, long t) {
       UNFile file = new UNFile(path);
       if (file == null || (!file.exists())) {
            if (bDebug)
                 System.out.println("SMS: file not found: "+path);
            return null;
       }
       if (!file.isFile())
          return null;
       timeOfSample = file.lastModified();
       // if (checkDate && timeOfSample <= curPlate.timeOfUpdated) {
       if (checkDate && timeOfSample <= t) {
            return null;
       }
       
       BufferedReader fin = null;
       try {
          fin = new BufferedReader(new FileReader(file));
       }
       catch(Exception e) {
          return null;
       }
       if (timeOfSample > timeOfPlate)
          timeOfPlate = timeOfSample;
       cleanSampleInfo();
       int     k, numVals, index, objType;
       int     rackNum, locNum, zoneNum;
       String  basicType;
       String  param;
       java.util.Date  timeDate;
       SmsSample sample = null;
       timeDate = new java.util.Date(timeOfSample);
       sampleDate = formatter.format(timeDate);
       sampleFile = path;
       // StreamTokenizerQuotedNewlines tok;
/*
       if (curPlate instanceof GrkPlate)
           rackId = curPlate.getRackId();
       else
           rackId = 0;
*/

       zoneNum = 0;
       locNum = 0;
       rackNum = 0;
       /* the followings were duplicated from FillDBManager */
       /* stok can be reusable */
       if (stok == null) {
          stok = new StreamTokenizerQuotedNewlines(fin);
          stok.quoteChar('\"');
          stok.wordChars('_', '_'); 
          stok.ordinaryChars('+', '.');
          stok.wordChars('+', '.');
          stok.ordinaryChars('0', '9');
          stok.wordChars('0', '9');
          stok.wordChars('$', '$');
       }
       else
          stok.setInputStream(fin);
       try {
          while (stok.nextToken() != StreamTokenizerQuotedNewlines.TT_EOF) {
             param = stok.sval;
             stok.nextToken(); // subtype
             stok.nextToken();
             basicType = stok.sval;
             objType = 0;
             if (basicType.equals("1"))
                 objType = 1;  // REAL
             else if (basicType.equals("2"))
                 objType = 2;  // STRING
             if (objType == 0) {  // bad format
                 fin.close();
                 return null;
             }
             for (k = 0; k < 9; k++)
                 stok.nextToken();

             numVals = Integer.parseInt(stok.sval);
             index = getAttrKey(param);
             if (index < 1 || numVals < 1) {
                 for (k = 0; k < numVals; k++)
                     stok.nextToken();
             }
             else {
                 stok.nextToken();
                 numVals--;
                 switch (index) {
                   case 1:   // loc_
                           locNum = Integer.parseInt(stok.sval);   
                           break;
                   case 2:   // vzone_ 
                           zoneNum = Integer.parseInt(stok.sval);   
                           break;
                   case 3:   // operator_
                           user = stok.sval;   
                           break;
                   case 4:   // studyid_
                           studyId = stok.sval.trim();   
                           break;
                   case 5:   // samplename
                           sampleName = stok.sval;   
                           break;
                   case 6:   // page
                           page = stok.sval;   
                           break;
                   case 7:   // studystatus
                           statusStr = stok.sval;
                           curStatus = getStatus(statusStr);
                           break;
                   case 8:   // solvent
                           solvent = stok.sval;   
                           break;
                   case 9:   // notebook
                           notebook = stok.sval;   
                           break;
                   case 10:   // barcode
                           barCode = stok.sval;   
                           break;
                   case 11:   // vrack_
                           rackNum = Integer.parseInt(stok.sval);   
                           break;
                   default:
                           break;
                 }
                 for (k = 0; k < numVals; k++)
                     stok.nextToken();
             }
             stok.nextToken();
             numVals = Integer.parseInt(stok.sval);
             for (k = 0; k < numVals; k++)
                     stok.nextToken();
          } // while loop 
          fin.close();
       }
       catch(Exception e) {
          try {
              fin.close();
          } catch(Exception eio) {}
          return null;
       }
       if (studyId == null)
          return null;
       if (rackId > 0) {
          if (rackId != rackNum) {
             if (checkDate || rackInfo == null)  // not delta update
                 return null;
             GenPlateIF plate = rackInfo.getPlate(rackNum);
             // if (plate == null || plate.timeOfUpdated < 1)
             if (plate == null)
                 return null;
             sample = plate.getSample(zoneNum, locNum);
             if (sample == null)
                 return null;
          }
       }
     /***
       if (!curUser.equals(user))
          curStatus = curStatus - OTHER;
     ***/
       if (sample == null) {
          sample = curPlate.getSample(zoneNum, locNum);
          if (sample == null)
              return null;
       }
       if (fileRecord != null) {
           fileRecord.timeOfFile = timeOfSample;
           fileRecord.zone = zoneNum;
           fileRecord.sampleLoc = locNum;
       }
       fillSampleInfo(sample, checkDate);
       return sample;
   }


   private static void clearPlateInfo() {
       if (sampleFileList != null)
           sampleFileList.clear();
       if (curPlate == null)
           return;
       curPlate.timeOfUpdated = 0;
       curPlate.clearAllSample();
       curPlate.sampleDataDir = null;
   }

   // does sample has this file info?
   /*************
   private static boolean isNewSampleFile(String name, String path) {
       if (bDebug)
           System.out.println(" sample file: "+name);
       UNFile file = new UNFile(path);
       if (file == null || !file.exists())
           return false;
       if (sampleFileList == null) {
           fileRecord = new sampleFileRecord();
           fileRecord.shortName = name;
           fileRecord.longName = path;
           newSampleFileList.add(fileRecord);
           return true;
       }
       sampleFileRecord oldRecord;
       int i, k;
       int index = -1;
 
       if (studySearchIndex > 0)
           i = studySearchIndex;
       else
           i = 0;
       for (k = i; k < sampleFileList.size(); k++) {
           oldRecord = sampleFileList.elementAt(k);
           if (oldRecord != null) {
               if (name.equals(oldRecord.shortName) &&
                          path.equals(oldRecord.longName)) {
                   index = k;
                   break;
               }
           }
       }
       if ((index < 0) && (i > 0)) {
           for (k = 0; k < i; k++) {
              oldRecord = sampleFileList.elementAt(k);
              if (oldRecord != null) {
                  if (name.equals(oldRecord.shortName) &&
                          path.equals(oldRecord.longName)) {
                      index = k;
                      break;
                  }
               }
           }
       }
       if (index < 0) {
           fileRecord = new sampleFileRecord();
           fileRecord.shortName = name;
           fileRecord.longName = path;
       }
       else
           fileRecord = sampleFileList.elementAt(index);
       studySearchIndex = index;
       newSampleFileList.add(fileRecord);
       if (index < 0 || (fileRecord.timeOfFile != file.lastModified()))
           return true; 

       SmsSample sample = curPlate.getSample(fileRecord.zone, fileRecord.sampleLoc);
       if (sample == null)
           return true;
       
       Vector<SampleInfo> infoList = sample.getInfoList();
       if (infoList == null)
           return true;
       SampleInfo info = null;
       index = -1;
       for (i = 0; i < infoList.size(); i++) {
           info = infoList.elementAt(i);
           if (info != null && (info.fileName != null)) {
               if (info.fileName.equals(path)) {
                   index = i;
                   break;
               }
           }
       }
       if (index < 0 || (info.iTime != file.lastModified()))
           return true;
       
       info.validate();
       if (bDebug)
           System.out.println(" .. sample info no change "+name);
       return false;
   }
   *************/

   private static boolean hasNewSampleFile() {
       if (autoPath == null || curPlate == null)
           return false;
       if (curPlate.sampleDataDir == null)
           return true;
       if (!curPlate.sampleDataDir.equals(autoPath))
           return true;

       long oldTime = curPlate.timeOfUpdated;
       String  asPath = new StringBuffer().append(autoPath).append(
                 File.separator).append("autostudies").toString();
       UNFile file = new UNFile(asPath);
       if (file.exists() && file.isFile()) {
            if (file.lastModified() > oldTime)
                return true;
       }
       String files[] = null;
       UNFile dir = null;
       dir = new UNFile(autoPath);
       if (dir != null && dir.isDirectory())
            files = dir.list();
       if (files == null || files.length < 1)
            return true;
       if (dir.lastModified() != curPlate.timeOfLastModified) {
            curPlate.timeOfLastModified = dir.lastModified();
            return true;
       }

       try {
          for (int i = 0; i < files.length; i++) {
             String f = new StringBuffer().append(autoPath).append(File.separator).append(
                 files[i]).append(File.separator).append("studypar").toString();
             file = new UNFile(f);
             if (file.exists() && file.isFile()) {
                 if (file.lastModified() > oldTime)
                      return true;
             }
          }
       }
       catch (Exception e) { }

       return false;
   }

   private static String studyparPath(String s) {
       String f;
       // the ':' is part of Windows's path, e.g. C:\autodir\sample1
       if (!s.startsWith(File.separator) && s.indexOf(":") < 1) {
          f = new StringBuffer().append(autoPath).append( File.separator).
              append(s).append(File.separator).append("studypar").toString();
       }
       else
          f = new StringBuffer().append(s).append(File.separator).append("studypar").toString();
       return f;
   }

   public static void updatePlateInfo() {
       if (bDebug)
           System.out.println("SMS: updatePlateInfo ");
       if (bUpdating) {
           if (bDebug)
              System.out.println("SMS: is already updating... ");
           return;
       }
       bUpdating = true;
       if (bDebug)
           System.out.println("SMS: update all sample status.");
       if (autoDir == null || curPlate == null) {
          if (bDebug) {
             if (curPlate == null)
                 System.out.println("SMS: plate is null.");
             else
                 System.out.println("SMS: autodir is null.");
          }
          bUpdating = false;
          return;
       }
       if (panel != null) {
          if (!panel.isVisible()) {
              if (bDebug)
                  System.out.println("SMS: panel is not visible ");
              bUpdating = false;
              return;
          }
       }

       autoPath = FileUtil.openPath(autoDir);
       if (autoPath != null) {
           if (!hasNewSampleFile()) {
               bUpdating = false;
               return;
           }
       }

       timeOfPlate = curPlate.timeOfUpdated;

       clearPlateInfo();

       String files[] = null;
       UNFile dir = null;
       if (autoPath != null) {
           dir = new UNFile(autoPath);
           if (dir != null && dir.isDirectory())
               files = dir.list();
       }
       if (files == null) {
          bUpdating = false;
          tray.getParent().repaint();
          return;
       }
       curPlate.timeOfLastModified = dir.lastModified();
       if (curPlate instanceof GrkPlate)
           rackId = curPlate.getRackId();
       else
           rackId = 0;
       curPlate.invalidateAllSample();
       int i;
       newSampleFileList = new Vector<sampleFileRecord> (files.length+50, 50);
       if (bDebug) {
           System.out.println("SMS: old sampleDataDir: "+curPlate.sampleDataDir);
           System.out.println("SMS: new sampleDataDir: "+autoDir);
       }
       if (curPlate.sampleDataDir != null) {
          if (!curPlate.sampleDataDir.equals(autoPath))
             curPlate.sampleDataDir = null;
          // the lastModified of directory was not reliable
          // else if (dir.lastModified() > curPlate.timeOfUpdated) {
          //    curPlate.sampleDataDir = null;
          // }
       }
       if (curPlate.sampleDataDir == null) {
          // clearPlateInfo();
          // curPlate.sampleDataDir = autoPath;
       }
       curPlate.sampleDataDir = autoPath;

       timeOfPlate = 0;

       // studySearchIndex = -1;
       String  f;
        
       try {
          for (i = 0; i < files.length; i++) {
             f = new StringBuffer().append(autoPath).append(File.separator).append(
                 files[i]).append(File.separator).append("studypar").toString();
             // if (isNewSampleFile(files[i], f)) {
                 if (bDebug)
                     System.out.println("  new sample info  "+files[i]);
                 updateSample(f, true, 0);
             // }
          }
       }
       catch (Exception e) {
            Messages.writeStackTrace(e);
            bUpdating = false;
            return;
       }

       String  inLine, data;
       StringTokenizer tok;
       String  asPath = new StringBuffer().append(autoPath).append(
                 File.separator).append("autostudies").toString();
       UNFile asFile = new UNFile(asPath);
       // if (asFile.exists() && asFile.isFile() && asFile.lastModified() > curPlate.timeOfUpdated) {
       if (asFile.exists() && asFile.isFile()) {
          try {
              BufferedReader in = new BufferedReader(new FileReader(asFile));
              while ((inLine = in.readLine()) != null) {
                 tok = new StringTokenizer(inLine, " ,\t\r\n");
                 if (!tok.hasMoreTokens())
                     continue;
                 data = tok.nextToken().trim();
                 f = studyparPath(data);
                 // if (isNewSampleFile(data, f)) {
                      if (bDebug)
                          System.out.println("  new sample info  "+data);
                      updateSample(f, true, 0);
                 // }
              }
              in.close();
          }
          catch(Exception e) { }
          if (asFile.lastModified() > timeOfPlate)
              timeOfPlate = asFile.lastModified();
       }

       curPlate.timeOfUpdated = timeOfPlate;
       if (sampleFileList != null)
          sampleFileList.clear();
       sampleFileList = newSampleFileList;
       if (bDebug)
           System.out.println("SMS: revalidateAllSample ");
       curPlate.revalidateAllSample();
       tray.getParent().repaint();
       infoPanel.showSampleInfo();
       if (bDebug)
           System.out.println("SMS: updatePlateInfo done ");
       bUpdating = false;
   }

   public static void repaintTray() {
       if (tray != null)
          tray.getParent().repaint();
   }

   public static void deltaSmaple(String f) {
       String p;
       if (autoPath == null)
           return;
       p = studyparPath(f);
       if (bDebug)
           System.out.println("SMS: delta "+f);
       fileRecord = null;
       SmsSample sample = updateSample(p, false, 0);
       if (sample != null)
           sample.reValidate();
       else
       {
          removeSmaple(f);
          curPlate.revalidateAllSample();
          tray.getParent().repaint();
       }
   }

   private static boolean removeZoneSmaple(String f, Vector<SmsSample> v) {
       if (v == null)
           return false;
       SmsSample s = null;
       SampleInfo info;
       Vector<SampleInfo> iV;
       int a, b, k;
       boolean bFound = false;
       int n = v.size();
       for (k = 0; k < n; k++) {
           s = (SmsSample) v.elementAt(k);
           if (s != null && s.infoList != null) {
               iV = s.infoList;
               a = iV.size();
               for (b = 0; b < a; b++) {  
                   info = (SampleInfo) iV.elementAt(b);
                   if (info.fileName.equals(f)) {
                       iV.removeElementAt(b);
                       bFound = true;
                       break;
                   }
               }
           } 
           if ( bFound )
              break;
       }
       if (!bFound || s == null)
           return false;
       iV = s.infoList;
       a = iV.size();
       curStatus = OPEN;
       k = 0;
       for (b = 0; b < a; b++) {  
           info = (SampleInfo) iV.elementAt(b);
           if (info != null) {
               k++;
               if (info.status > curStatus)
                   curStatus = info.status;
           }
       }
       s.studyNum = k;
       s.status = curStatus;
       return bFound;
   }

   private static void removePlateSmaple(String f, SmsSample[] l) {
       if (l == null)
           return;
       SmsSample s = null;
       Vector<SampleInfo> v;
       SampleInfo info;
       int  a, b, k;
       int  n = l.length;
       boolean bFound = false;
       for (k = 0; k < n; k++) {
           s = l[k];
           if (s != null && s.infoList != null) {
               v = s.infoList;
               a = v.size();
               for (b = 0; b < a; b++) {  
                   info = (SampleInfo) v.elementAt(b);
                   if (info.fileName.equals(f)) {
                       v.removeElementAt(b);
                       bFound = true;
                       break;
                   }
               }
           }
           if ( bFound )
              break;
       }
       if (!bFound || s == null)
           return;
       v = s.infoList;
       a = v.size();
       curStatus = OPEN;
       k = 0;
       for (b = 0; b < a; b++) {  
           info = (SampleInfo) v.elementAt(b);
           if (info != null) {
               k++;
               if (info.status > curStatus)
                   curStatus = info.status;
           }
       }
       s.studyNum = k;
       s.status = curStatus;
   }

   public static void removeSmaple(String f) {
       if (curPlate == null)
           return;
       int    n, num, k;
       SmsSample[] sList;
       String p = studyparPath(f);

       if (!(curPlate instanceof GrkPlate)) {
//           sList = curPlate.getSampleList();
           sList = curPlate.getSampleList();
           n = curPlate.getSampleNum();
           if (n > 0 && sList != null)
               removePlateSmaple(p, sList);
           return;
       }
       if (rackInfo == null)
           return;
       RackObj rackList[] = rackInfo.getRackList();
       if (rackList == null)
           return;
       num = rackList.length;
       for (n = 0; n < num; n++) {
           RackObj rack = (RackObj) rackList[n];
           GrkPlate plate = null;
           if (rack != null) {
              plate = rack.plate;
           }
           if (plate != null && plate.timeOfUpdated > 0) {
              Vector<GrkZone> zList = plate.getZoneList();
              if (zList != null) {
                   for (k = 0; k < zList.size(); k++) {
                       GrkZone  zone = (GrkZone) zList.elementAt(k);
                       Vector<SmsSample> zV = zone.getSampleList();
                       if (removeZoneSmaple(p, zV))
                          return;
                   }
              } 
           }
       }
   }

   public static void setSelectPattern(GrkPlate p) {
       int pattern = p.searchPattern;
       int start = p.searchStart;
       sOrient = BYROW;
       sOrder = L_FRONT;
       sOrder2 = TOP_DOWN;
       if (pattern <= 2) {
           sOrient = BYROW;
           switch (start) {
                case 1: 
                        sOrder = L_FRONT;
                        if (pattern == 2)
                           sOrder = L_BACK;
                        break;
                case 2: 
                        sOrder = R_FRONT;
                        if (pattern == 2)
                           sOrder = R_BACK;
                        break;
                case 3: 
                        sOrder2 = BOT_UP;
                        sOrder = L_FRONT;
                        if (pattern == 2)
                           sOrder = L_BACK;
                        break;
                case 4: 
                        sOrder2 = BOT_UP;
                        sOrder = R_FRONT;
                        if (pattern == 2)
                           sOrder = R_BACK;
                        break;
           }
       }
       else {
           sOrient = BYCOL;
           switch (start) {
                case 1: 
                        sOrder = L_FRONT;
                        if (pattern == 4)
                           sOrder = L_BACK;
                        break;
                case 2: 
                        sOrder = R_FRONT;
                        if (pattern == 4)
                           sOrder = R_BACK;
                        break;
                case 3: 
                        sOrder2 = BOT_UP;
                        sOrder = L_FRONT;
                        if (pattern == 4)
                           sOrder = L_BACK;
                        break;
                case 4: 
                        sOrder2 = BOT_UP;
                        sOrder = R_FRONT;
                        if (pattern == 4)
                           sOrder = R_BACK;
                        break;
           }
       }
   }

   public static void setTrayPlate(GenPlateIF plate) {
       if (curUser == null)
           curUser = System.getProperty("user.name");
       if (curPlate != plate) {
/*
           if (curPlate != null)
               curPlate.clearAllSample(); // remove all data
*/
           curPlate = plate;
           if (curPlate != null) {
               updatePlateInfo();
               if (plate instanceof GrkPlate) {
                   setSelectPattern((GrkPlate) curPlate);
               }
           }
       }
   }

   public static void setOperator( String s) {
       curUser = s;
   }

   private static void updateByTimer() {
        if (bUpdating) {
            timerCount ++;
            if (timerCount < 3) {
               updateTimer.restart();
               return;
            }
            bUpdating = false;
        }
        timerCount = 0;
        updatePlateInfo();
   }

   private static void setUpdateTimer() {
       if (updateTimer == null) {
             ActionListener updateAction = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                     if (bDebug)
                        System.out.println("SMS: update timer is up ");
                     updateByTimer();
                }
             };
             updateTimer = new Timer(5000, updateAction);
             updateTimer.setRepeats(false);
        }
        updateTimer.restart();
   }

   public static void setAutoDir( String s) {
       if (bDebug)
           System.out.println("SMS: set autodir to "+s);
       if (s == null) {
           autoDir = s;
           return;
       }
       if (bTest) {
           autoDir = s;
           updatePlateInfo();
           return;
       }

       Vector mp = MountPaths.getPathAndHost(s);
/*
       String dhost = (String) mp.get(Shuf.HOST);
*/
       String dpath = (String) mp.get(Shuf.PATH);
       autoDir = dpath;
       updatePlateInfo();
       setUpdateTimer();
   }

   public static int getStatus(String s) {
       return SmsColorEditor.getStatusNo(s);
      /***
       if (s == null)
           return OPEN;
       for (int n = 0; n < sNames.length; n++) {
            if (s.equals(sNames[n]))
                return sKeys[n];
       }
       return OPEN;
      ***/
   }

   private static int getAttrKey(String s) {
       for (int n = 0; n < attrs.length; n++) {
            if (s.equals(attrs[n]))
                return n+1;
       }
       return 0;
   }


   public static boolean prepareRackSample(GrkPlate  plate) {
       Vector<GrkZone> zoneList = plate.getZoneList();
       if (zoneList == null || zoneList.size() < 1)
           return false;
       SmsSample s;
       GrkZone curZone = null;
       Vector<SmsSample> sampleList = null;
       for (int i = 0; i < zoneList.size(); i++) {
           curZone = zoneList.elementAt(i);
           sampleList = curZone.getSampleList();
           for (int k = 0; k < sampleList.size(); k++) {
              s = (SmsSample) sampleList.elementAt(k);
              s.status = OPEN;
           }
       }
       return true;
   }

   public static void hSelection(GrkZone z, SmsSample s1, SmsSample s2) {
       int cols;
       SmsSample obj;
       int r1, c1, r2, c2;
       int inc;
       boolean fromLeft = true;
       boolean bBack = false;
       SmsSample sampleArray[][] = null;
       
       if (z.rotated) {
          sampleArray = z.vArray;
          cols = z.rowNum;
       }
       else {
          sampleArray = z.sArray;
          cols = z.colNum;
       }
       if (sampleArray == null)
          return;

       r1 = s1.row - 1;
       c1 = s1.col - 1;
       r2 = s2.row - 1;
       c2 = s2.col - 1;
       inc = 1;
       if (sOrder == R_BACK || sOrder == R_FRONT) {
           fromLeft = false;
           inc = -1;
       }
       if (sOrder == R_BACK || sOrder == L_BACK)
           bBack = true;
       hList.clear();
       hList.add(s1);
       if (s1.status == OPEN)
            s1.selected = true;
       else if (bReusable) {
            s1.selected = true;
       }
       while (true) {
           c1 = c1 + inc;
           if (c1 >= cols) {
               if (bBack) {
                  c1 = cols - 1; 
                  inc = -1;
               }
               else {
                  if (fromLeft) {
                     c1 = 0;
                     inc = 1;
                  }
                  else {
                     c1 = cols - 1; 
                     inc = -1;
                  }
               }
               if (sOrder2 == TOP_DOWN) {
                  r1++;
                  if (r1 > r2)
                     break;
               }
               else {
                  r1--;
                  if (r1 < r2)
                     break;
               }
           }
           else if (c1 < 0) {
               if (bBack) {
                  c1 = 0;
                  inc = 1;
               }
               else {
                  if (fromLeft) {
                     c1 = 0;
                     inc = 1;
                  }
                  else {
                     c1 = cols - 1;
                     inc = -1;
                  }
               }
               if (sOrder2 == TOP_DOWN) {
                  r1++;
                  if (r1 > r2)
                     break;
               }
               else {
                  r1--;
                  if (r1 < r2)
                     break;
               }
           }
           obj = sampleArray[r1][c1];
           if (obj != null) {
               hList.add(obj);
               if (obj.status == OPEN) {
                   obj.selected = true;
                  //  hList.add(obj);
               }
               else if (bReusable) {
                   obj.selected = true;
/*
                   if (obj.status <= OTHER) {
                       obj.selected = true;
                   }
*/
               }
           }
           if (r1 == r2 && c1 == c2)
               break;
       }
   }

   public static void vSelection(GrkZone z, SmsSample s1, SmsSample s2) {
       int rows;
       SmsSample obj;
       int r1, c1, r2, c2;
       int inc;
       boolean fromLeft = true;
       boolean bBack = false;
       SmsSample sampleArray[][] = null;

       if (z.rotated) {
          sampleArray = z.vArray;
          rows = z.colNum;
       }
       else {
          sampleArray = z.sArray;
          rows = z.rowNum;
       }
       if (sampleArray == null)
          return;

       r1 = s1.row - 1;
       c1 = s1.col - 1;
       r2 = s2.row - 1;
       c2 = s2.col - 1;
       inc = 1;

       if (sOrder2 == BOT_UP)
           inc = -1;
       if (sOrder == R_BACK || sOrder == R_FRONT)
           fromLeft = false;
       if (sOrder == L_BACK || sOrder == R_BACK)
           bBack = true;
       hList.clear();
       hList.add(s1);
       if (s1.status == OPEN)
            s1.selected = true;
       else if (bReusable) {
            s1.selected = true;
       }
       while (true) {
           r1 = r1 + inc;
           if (r1 >= rows) {
               if (bBack) {
                  r1 = rows - 1;
                  inc = -1;
               }
               else {
                  if (sOrder2 == TOP_DOWN) {
                     r1 = 0;
                     inc = 1;
                  }
                  else {
                     r1 = rows - 1;
                     inc = -1;
                  }
               }
               if (fromLeft) {
                  c1++;
                  if (c1 > c2)
                     break;
               }
               else {
                  c1--;
                  if (c1 < c2)
                     break;
               }
           }
           else if (r1 < 0) {
               if (bBack) {
                  r1 = 0;
                  inc = 1;
               }
               else {
                  if (sOrder2 == TOP_DOWN) {
                     r1 = 0;
                     inc = 1;
                  }
                  else {
                     r1 = rows - 1;
                     inc = -1;
                  }
               }
               if (fromLeft) {
                  c1++;
                  if (c1 > c2)
                     break;
               }
               else {
                  c1--;
                  if (c1 < c2)
                     break;
               }
           }
           obj = sampleArray[r1][c1];
           if (obj != null) {
               hList.add(obj);
               if (obj.status == OPEN) {
                   obj.selected = true;
               }
               else if (bReusable) {
                   obj.selected = true;
/*
                   if (obj.status <= OTHER) {
                       obj.selected = true;
                   }
*/
               }
           }
           if (r1 == r2 && c1 == c2)
               break;
       }
   }

   private static boolean doSelection(GrkZone z, SmsSample s1, SmsSample s2) {
       int r, c, r2, c2;
       
       if (z.sArray == null || curPlate == null)
            return false;
       hList = curPlate.getSelectList();
       if (hList == null)
            return false;
       // s1.selected = true;
       s1.startItem = true;
       r = s1.row;
       c = s1.col;
       r2 = s2.row;
       c2 = s2.col;
       if (sOrient == BYROW) {
            if (sOrder2 == TOP_DOWN) {
                if (r > r2)
                   return false; 
            }
            else {
                if (r2 > r)
                   return false; 
            }
            if (r == r2) {
                if (c < c2) {
                   if (sOrder == R_BACK || sOrder == R_FRONT) {
                       return false;
                   }
                }
                if (c > c2) {
                   if (sOrder == L_BACK || sOrder == L_FRONT) {
                       return false;
                   }
                }
            }
       }
       else {
            if (sOrder == L_BACK || sOrder == L_FRONT) {
                 if (c > c2)
                    return false; 
            }
            else {
                 if (c < c2)
                    return false; 
            }
            if (c == c2) {
                if (r < r2) {
                   if (sOrder2 == BOT_UP)
                       return false;
                }
                if (r > r2) {
                   if (sOrder2 == TOP_DOWN)
                       return false;
                }
            }
       }
       if (sOrient == BYROW) {
            hSelection(z, s1, s2);
       }
       else {
            vSelection(z, s1, s2);
       }
       return true;
   }

   public static void zoneSelection(GrkZone z, SmsSample s1, SmsSample s2) {
       if (!doSelection(z, s1, s2))
            doSelection(z, s2, s1);
   }

   private static void setPointer(int type, int gap) {
       switch (type) {
          case 1:  // left
                  xPnts[0] = 0;
                  yPnts[0] = -2;
                  xPnts[1] = gap;
                  yPnts[1] = -2;
                  xPnts[2] = gap;
                  yPnts[2] = -6;
                  xPnts[3] = gap * 2;
                  yPnts[3] = 0;
                  xPnts[4] = gap;
                  yPnts[4] = 6;
                  xPnts[5] = gap;
                  yPnts[5] = 2;
                  xPnts[6] = 0;
                  yPnts[6] = 2;
                  break;
          case 2:  // right
                  xPnts[0] = 0;
                  yPnts[0] = 0;
                  xPnts[1] = gap;
                  yPnts[1] = 6;
                  xPnts[2] = gap;
                  yPnts[2] = 2;
                  xPnts[3] = gap * 2;
                  yPnts[3] = 2;
                  xPnts[4] = gap * 2;
                  yPnts[4] = -2;
                  xPnts[5] = gap;
                  yPnts[5] = -2;
                  xPnts[6] = gap;
                  yPnts[6] = -6;
                  break;
          case 3:  // up
                  xPnts[0] = -2;
                  yPnts[0] = gap * 2;
                  xPnts[1] = -2;
                  yPnts[1] = gap;
                  xPnts[2] = -6;
                  yPnts[2] = gap;
                  xPnts[3] = 0;
                  yPnts[3] = 0;
                  xPnts[4] = 6;
                  yPnts[4] = gap;
                  xPnts[5] = 2;
                  yPnts[5] = gap;
                  xPnts[6] = 2;
                  yPnts[6] = gap * 2;
                  break;
          case 4:  // down
                  xPnts[0] = -2;
                  yPnts[0] = 0;
                  xPnts[1] = -2;
                  yPnts[1] = gap;
                  xPnts[2] = -6;
                  yPnts[2] = gap;
                  xPnts[3] = 0;
                  yPnts[3] = gap * 2;
                  xPnts[4] = 6;
                  yPnts[4] = gap;
                  xPnts[5] = 2;
                  yPnts[5] = gap;
                  xPnts[6] = 2;
                  yPnts[6] = 0;
                  break;
       }
   }


   public static void vLinkList(Graphics2D g, GrkZone z) {
       SmsSample obj, obj2;
       int r, c;
       int r1, c1;
       int x, y;
       int x1, y1;
       int inc, pointer;
       int k, num;
       int d, d1, d2;
       double deg;

       obj = (SmsSample) hList.elementAt(0);
       r = obj.row - 1;
       c = obj.col - 1;
       x = 0;
       x1 = 0;
       y = 0;
       y1 = 0;
       pointer = 0;
       d = 0;
       d1 = 0;
       d2 = 0;
       deg = 0;
       num = hList.size();
       for (k = 1; k < num; k++) {
           obj2 = (SmsSample) hList.elementAt(k);
           r1 = obj2.row - 1;
           c1 = obj2.col - 1;
           if (c == c1) { // at the same column
              if (r1 > r) { // downward
                  inc = 4;
                  x = obj.locX + obj.width / 2;
                  y = obj.locY + obj.width;
                  x1 = obj2.locX + obj2.width / 2;
                  y1 = obj2.locY;
              }
              else { // upward
                  inc = 3;
                  x = obj2.locX + obj2.width / 2;
                  y = obj2.locY + obj2.width;
                  x1 = obj.locX + obj.width / 2;
                  y1 = obj.locY;
              }
              if (pointer != inc) {
                  d = y1 - y;
                  if (d > 8)
                     d2 = 3;
                  else if (d > 6)
                     d2 = 2;
                  else
                     d2 = 1;
                  d1 = (d - d2 * 2) / 2;
                  if (d1 < 1)  d1 = 1;
                  pointer = inc;
                  setPointer(pointer, d2);
              }
              deg = 0;
              if (x != x1) {
                   if (x > x1) {
                      deg = 0.3;
                      x = x - obj.width / 2;
                   }
                   else {
                      deg = -0.3;
                      x = x + obj.width / 2;
                   }
              }
              y = y + d1;
              g.translate(x, y);
              if (deg != 0)
                  g.rotate(deg);
              // g.drawPolyline(xPnts, yPnts, 7);
              g.fillPolygon(xPnts, yPnts, 7);
              if (deg != 0)
                 g.rotate(-deg);
              g.translate(-x, -y);
           }
           else {  //  c != c1    change column
              if  (colorIndex == 1) {
                 g.setColor(Color.green.darker());
                 colorIndex = 2;
              }
              else if  (colorIndex == 2) {
                 g.setColor(Color.black);
                 colorIndex = 3;
              }
              else {
                 g.setColor(Color.blue);
                 colorIndex = 1;
              }
              if (r == r1) { // same row
                  if (c < c1) {
                     inc = 1;
                     x = obj.locX + obj.width;
                     x1 = obj2.locX;
                     y = obj.locY + obj.width / 2;
                     y1 = obj2.locY + obj2.width / 2;
                  } 
                  else {
                     inc = 2;
                     x = obj2.locX + obj2.width;
                     x1 = obj.locX;
                     y = obj2.locY + obj2.width / 2;
                     y1 = obj.locY + obj.width / 2;
                  } 
                  d = x1 - x;
                  if (d > 8)
                     d2 = 3;
                  else if (d > 6)
                     d2 = 2;
                  else
                     d2 = 1;
                  d1 = (d - d2 * 2) / 2;
                  if (d1 < 1)
                     d1 = 1;
                  if (pointer != inc)
                     setPointer(inc, d2);
                  pointer = inc;
                  deg = 0;
                  if (y != y1) {
                      if (y > y1) {
                          deg = -0.3;
                          y = y - obj.width / 2;
                      }
                      else {
                          deg = 0.3;
                          y = y + obj.width / 2;
                      }
                  }
                  x = x + d1;
                  g.translate(x, y);
                  if (deg != 0)
                      g.rotate(deg);
                  // g.drawPolyline(xPnts, yPnts, 7);
                  g.fillPolygon(xPnts, yPnts, 7);
                  if (deg != 0)
                      g.rotate(-deg);
                  g.translate(-x, -y);
              }
              else { //  r != r1   at different row and col
                  if (r > 0) {
                      y = obj.locY + obj.width + 4;
                  }
                  else {
                      y = obj.locY - d2 * 2 - 4;
                  }
                  x = obj.locX + obj.width / 2;
                  g.translate(x, y);
                  g.fillPolygon(xPnts, yPnts, 7);
                  g.translate(-x, -y);
                  x = obj2.locX + obj2.width / 2;
                  if (r1 > 0)
                      y = obj2.locY + obj2.width + 4;
                  else
                      y = obj2.locY - d2 * 2 - 4;
                  g.translate(x, y);
                  g.fillPolygon(xPnts, yPnts, 7);
                  g.translate(-x, -y);
              }
           }
           r = r1;
           c = c1;
           obj = obj2;
       }
   }


   public static void hLinkList(Graphics2D g, GrkZone z) {
       SmsSample obj, obj1;
       int r, c;
       int r1, c1;
       int x, y;
       int x1, y1;
       int d, d1, d2;
       int k, num;
       int inc, pointer;
       double  deg;

       obj = (SmsSample) hList.elementAt(0);
       r = obj.row - 1;
       c = obj.col - 1;
       pointer = 0;
       x = 0;
       x1 = 0;
       y = 0;
       y1 = 0;
       d1 = 0;
       d2 = 0;
       deg = 0;
       num = hList.size();
       for (k = 1; k < num; k++) {
           obj1 = (SmsSample) hList.elementAt(k);
           r1 = obj1.row - 1;
           c1 = obj1.col - 1;
           if (r == r1) { // at the same row
              if (c1 > c) {
                 inc = 1;
                 y = obj.locY + obj.width / 2;
                 x = obj.locX + obj.width;
                 y1 = obj1.locY + obj1.width / 2;
                 x1 = obj1.locX;
              }
              else {
                 inc = 2;
                 y = obj1.locY + obj1.width / 2;
                 x = obj1.locX + obj1.width;
                 y1 = obj.locY + obj.width / 2;
                 x1 = obj.locX;
              }
              if (pointer != inc) {
                 d = x1 - x;
                 if (d > 8)
                     d2 = 3; 
                 else if (d > 6)
                     d2 = 2; 
                 else
                     d2 = 1; 
                 d1 = (d - d2 * 2) / 2;
                 if (d1 < 1)
                     d1 = 1;
                 pointer = inc;
                 setPointer(pointer, d2);
              }
              deg = 0;
              if (y != y1) {
                 if (y > y1) {
                     deg = -0.4;
                     y = y - obj.width / 2;
                 }
                 else {
                     deg = 0.4;
                     y = y + obj.width / 2;
                 }
              }
              g.translate(x + d1, y);
              if (deg != 0)
                  g.rotate(deg);
              g.fillPolygon(xPnts, yPnts, 7);
              if (deg != 0)
                 g.rotate(-deg);
              g.translate(-x - d1, -y);
           }
           else {  // r != r1
              if  (colorIndex == 1) {
                 g.setColor(Color.green.darker());
                 colorIndex = 2;
              }
              else if  (colorIndex == 2) {
                 g.setColor(Color.black);
                 colorIndex = 3;
              }
              else {
                 g.setColor(Color.blue);
                 colorIndex = 1;
              }
              if (c == c1) {  // same column
                 if (r < r1) {
                     inc = 4;
                     y = obj.locY + obj.width;
                     y1 = obj1.locY;
                     x = obj.locX + obj.width / 2;
                     x1 = obj1.locX + obj1.width / 2;
                 }
                 else {
                     inc = 3;
                     y = obj1.locY;
                     y1 = obj.locY + obj.width;
                     x = obj1.locX + obj1.width / 2;
                     x1 = obj.locX + obj.width / 2;
                 }
                 d = y1 - y;
                 if (d > 8)
                     d2 = 3; 
                 else if (d > 6)
                    d2 = 2; 
                 else
                     d2 = 1; 
                 d1 = (d - d2 * 2) / 2;
                 if (d1 < 1)
                     d1 = 1;
                 if (pointer != inc)
                     setPointer(inc, d2);
                 pointer = inc;
                 deg = 0;
                 if (x != x1) {
                   if (x > x1) {
                      deg = 0.3;
                      x = x - obj.width / 2;
                   }
                   else {
                      deg = -0.3;
                      x = x + obj.width / 2;
                   }
                 }
                 y = y + d1 + 1;
                 g.translate(x, y);
                 if (deg != 0)
                    g.rotate(deg);
                 g.fillPolygon(xPnts, yPnts, 7);
                 if (deg != 0)
                    g.rotate(-deg);
                 g.translate(-x, -y);
              }
              else {
                 if (c > 0) {
                     x = obj.locX + obj.width + 4;
                 }
                 else {
                     x = obj.locX - d2 * 2 - 4;
                 }
                 y = obj.locY + obj.width / 2;
                 g.translate(x, y);
                 g.fillPolygon(xPnts, yPnts, 7);
                 g.translate(-x, -y);
                 y = obj1.locY + obj1.width / 2;
                 if (c1 > 0) {
                      x = obj1.locX + obj1.width + 4;
                 }
                 else {
                      x = obj1.locX - d2 * 2 - 4;
                 }
                 g.translate(x, y);
                 g.fillPolygon(xPnts, yPnts, 7);
                 g.translate(-x, -y);
              }
           }
           r = r1;
           c = c1;
           obj = obj1;
       }
   }

   public static void linkSelectList(Graphics2D g, GrkZone z) {
       
       if (z.sArray == null || curPlate == null)
            return;
       hList = curPlate.getSelectList();
       if (hList == null || hList.size() < 2)
            return;
       g.setColor(Color.blue);
       colorIndex = 1;
       if (sOrient == BYROW) {
           hLinkList(g, z);
       }
       else
           vLinkList(g, z);
   }

   public static BufferedImage getSampleImage(int w, int h) {
        return null;
   }

   public static BufferedImage old_getSampleImage(int w, int h) {
       if (panel == null)
           return null;
       if (sampleImges == null) {
           sampleImges = new Image[3];
           sampleImgWidths = new int[3];
           sampleImgHeights = new int[3];
           ImageIcon icon = Util.getImageIcon("sampleSmall.png");
           if (icon != null) {
               sampleImges[0] = icon.getImage();
               sampleImgWidths[0] = icon.getIconWidth();
               sampleImgHeights[0] = icon.getIconHeight();
           }
           icon = Util.getImageIcon("sampleMedium.png");
           if (icon != null) {
               sampleImges[1] = icon.getImage();
               sampleImgWidths[1] = icon.getIconWidth();
               sampleImgHeights[1] = icon.getIconHeight();
           }
           icon = Util.getImageIcon("sampleLarge.png");
           if (icon != null) {
               sampleImges[2] = icon.getImage();
               sampleImgWidths[2] = icon.getIconWidth();
               sampleImgHeights[2] = icon.getIconHeight();
           }
       }
       if (w < 4 || h < 4)
           return null;

       Image img = null;
       for (int i = 0; i < 3; i++) {
           if (sampleImges[i] != null) {
               if (sampleImgWidths[i] <= w && sampleImgHeights[i] <= h) {
                   img = sampleImges[i];
               }
               else if (img == null) {
                   img = sampleImges[i];
               }
           }
       }
       if (img == null)
           return null;

       BufferedImage bufImage = null;
       int imgW = img.getWidth(null);
       int imgH = img.getHeight(null);
       int bufW = imgW;
       int bufH = imgH;

       if (bufW > w)
           bufW = w;
       if (bufH > h)
           bufH = h;

       try {
           bufImage = panel.getGraphicsConfiguration()
                   .createCompatibleImage(bufW, bufH, Transparency.TRANSLUCENT);
           Graphics2D gc = bufImage.createGraphics();
           gc.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
           gc.setRenderingHint(RenderingHints.KEY_DITHERING,RenderingHints.VALUE_DITHER_ENABLE);
           gc.drawImage(img, 0, 0, bufW, bufH, 0, 0, imgW, imgH, null);
       }
       catch (Exception e) {
           bufImage = null;
       }
       return bufImage;
   }

   private static class sampleFileRecord {
       public String shortName;
       public String longName;
       public long   timeOfFile;
       public int    zone;
       public int    sampleLoc;

       public sampleFileRecord() {
           this.zone = -1;
           this.sampleLoc = -1;
           this.timeOfFile = 0;
       }
   }
}

