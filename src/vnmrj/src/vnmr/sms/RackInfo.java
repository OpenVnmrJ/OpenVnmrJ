/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;

import java.io.*;
import java.util.*;

import vnmr.util.*;


public class RackInfo implements SmsDef
{
   private String infoPath;
   private String currentPath;
   private String racksPath;
   private String infoDir;
   private Vector<GrkPlate> typeList;
   private GrkPlate curPlate;
   private RackObj curRack;
   private int maxSample = 0;
   private int trayType = VAST;
   private boolean newInfo = true;
   private long fileDate = 0;
   private RackObj rackList[];

   public RackInfo() {
       init(VAST);     
   }

   public RackInfo(int type) {
       init(type);     
   }

   private void init(int type) {
       trayType = type;
       infoDir = Util.VNMRDIR+File.separator+"asm"+File.separator+"racks"+
                  File.separator;
       infoPath = infoDir+"rackInfo";
       infoDir = Util.VNMRDIR+File.separator+"asm"+File.separator+"info"+
                  File.separator;
       currentPath = infoDir+"currentRacks";
       racksPath = infoDir+"racks";
       typeList = new Vector<GrkPlate>();
       rackList = new RackObj[6];
       update();
       SmsUtil.setRackInfo(this);
   }

   public void setTrayType(int type) {
       if (trayType != type) {
           trayType = type;
           update();
       }
   }


   public int getMaxNumber() {
       return maxSample;
   }

   private void addPlate(String t) {
       if (curPlate != null) {
          if (t.equals(curPlate.getName()))
              return;
       }
       int n = 0;
       int p = -1;
       for (int k = 0; k < typeList.size(); k++) {
          curPlate = typeList.elementAt(k);
          if (curPlate != null) {
              n = t.compareTo(curPlate.getName());
              if (n == 0)
                 return;
              if (n < 0) {
                 p = k;
                 break;
              }
          }
       }
       curPlate = new GrkPlate(t);
       if (p >= 0)
          typeList.add(p, curPlate);
       else
          typeList.add(curPlate);
   }
 
   private int getNum(String s) {
       if (s.length() < 1)
          return 0;
       int n = 0;
       try {
           n = Integer.parseInt(s);
       }
       catch (NumberFormatException er) { n = 0; }
       return n;
   }

   private void setListSize(int s) {
       if (rackList.length > s)
           return;
       int n;
       RackObj newList[] = new RackObj[s+1];
       for (n = 0; n < rackList.length; n++) {
            newList[n] = rackList[n];
       }
       for (n = rackList.length; n < s+1; n++) {
            newList[n] = null;
       }
       rackList = newList;
   }
 
   private void setListSize(String s) {
       int k = getNum(s);
       if (rackList.length > k)
           return;
       setListSize(k);
   }

   private void buildInfo() {
       BufferedReader fin = null;
       String data, d2, parm, val;
       StringTokenizer tok;
       int  p1, p2;

       typeList.clear();
       curPlate = null;
       try {
          fin = new BufferedReader(new FileReader(infoPath));
          while ((data = fin.readLine()) != null) {
              d2 = data.trim();
              if (d2.length() < 14)
                  continue;
              if (!d2.startsWith("set "))
                  continue;
              tok = new StringTokenizer(d2, " (,\t\r\n");
              tok.nextToken();
              if (!tok.hasMoreTokens())
                  continue;
              parm = tok.nextToken();
              if (!parm.equals("rackInfo"))
                  continue;
              p1 = d2.indexOf('(');
              p2 = d2.indexOf(')');
              if (p1 < 10 || p2 < (p1+4))
                  continue;
              parm = d2.substring(p1+1, p2);
              val = d2.substring(p2+1).trim();
              tok = new StringTokenizer(parm, " ,\t\r");
              if (!tok.hasMoreTokens())
                  continue;
              d2 = tok.nextToken().trim();
              if (d2.equals("types"))
                  continue;
              if (!tok.hasMoreTokens())
                  continue;
              addPlate(d2); 
              if (curPlate != null)
                  curPlate.setParam(tok, val);
          }
       }
       catch(IOException e)
       {
            String strError = "Error reading file: "+ infoPath;
            Messages.writeStackTrace(e, strError);
            Messages.postError(strError);
       }
       if (fin != null) {
           try {
                fin.close();
            } catch(IOException ee) { }
       }
   }

   private RackObj getRack(int n) {
       if (n < 1)
          return null;
       if (n >= rackList.length)
          setListSize(n);
       RackObj rack = (RackObj)rackList[n];
       if (rack == null) {
          rack = new RackObj(n);
          rackList[n] = rack;
       }
       return rack;
   }

   private RackObj getRack(String s) {
       return getRack(getNum(s));
   }

   public GrkPlate getPlate(int n) {
       if (n < 1)
          return null;
       if (n >= rackList.length)
          setListSize(n);
       RackObj rack = (RackObj)rackList[n];
       if (rack == null) {
          rack = new RackObj(n);
          rackList[n] = rack;
       }
       return  rack.getPlate();
   }

   public GrkPlate getPlate(String t) {
       GrkPlate plate = null;
       curRack = getRack(t);
       if (curRack != null)
           plate = curRack.getPlate();
       return plate;
   }

   private void setRackData(String s1, String s2, String s3) {
       curRack = getRack(s1);
       if (curRack == null)
           return;
       if (s2.equals("file")) {
           curRack.setFile(s3);
           return;
       }
       if (s2.equals("zones")) {
           curRack.setZones(getNum(s3));
           return;
       }
       if (s2.equals("start")) {
           curRack.start = getNum(s3);
           return;
       }
       if (s2.equals("pattern")) {
           curRack.pattern = getNum(s3);
           return;
       }
   }

   private void setRackData(String s1, String s2, String s3, String s4) {
       curRack = getRack(s1);
       if (curRack == null)
           return;
       int zone = getNum(s2);
       int num = getNum(s4);
       if (s3.equals("row")) {
           curRack.setRow(zone, num);
           return;
       }
       if (s3.equals("col")) {
           curRack.setCol(zone, num);
           return;
       }
       if (s3.equals("num")) {
           curRack.setNumber(zone, num);
           return;
       }
   }


   private void buildRackInfo() {
       BufferedReader reader = null;
       String data, d2, d3, d4, v1, v2, v3;
       StringTokenizer tok;
       int  p1, p2, vc;

       try {
          reader = new BufferedReader(new FileReader(currentPath));
          while ((data = reader.readLine()) != null) {
             d2 = data.trim();
             if (d2.length() < 14)
                  continue;
             if (!d2.startsWith("set "))
                  continue;
             p1 = d2.indexOf('(');
             p2 = d2.indexOf(')');
             if (p1 < 1 || p2 < 1 || p2 < p1)
                  continue;
             d3 = d2.substring(p1+1, p2);
             d4 = d2.substring(p2+1).trim();
             tok = new StringTokenizer(d3, ",\n");
             vc = 0;
             v1 = null;
             v2 = null;
             v3 = null;
             if (tok.hasMoreTokens()) {
                 vc = 1;
                 v1 = tok.nextToken();
             }
             if (tok.hasMoreTokens()) {
                 vc = 2;
                 v2 = tok.nextToken();
             }
             if (tok.hasMoreTokens()) {
                 vc = 3;
                 v3 = tok.nextToken();
             }
             if (vc < 1)
                 continue;
             if (vc == 1) {
                 if (v1.equals("num")) {
                    setListSize(d4);
                 }
                 continue;
             }
             if (vc == 2) {
                 setRackData(v1, v2, d4);
                 continue;
             }
             if (vc == 3) {
                 setRackData(v1, v2, v3, d4);
                 continue;
             }
          }
       }
       catch(IOException e)
       {
            String strError = "Error reading file: "+ currentPath;
            Messages.writeStackTrace(e, strError);
            Messages.postError(strError);
       }
       if (reader != null) {
           try {
                reader.close();
            } catch(IOException ee) { }
       }
   }

   private void buildRackInfo2() {
       BufferedReader reader = null;
       String data, v1, v2;
       StringTokenizer tok;

       try {
          reader = new BufferedReader(new FileReader(racksPath));
          while ((data = reader.readLine()) != null) {
             tok = new StringTokenizer(data, " \t\n");
             if (!tok.hasMoreTokens())
                  continue;
             v1 = tok.nextToken();
             if (!v1.startsWith("gRackLocTypeMap"))
                  continue;
             if (!tok.hasMoreTokens())
                  continue;
             v1 = tok.nextToken();
             if (!tok.hasMoreTokens())
                  continue;
             v2 = tok.nextToken();
             curRack = getRack(v1);
             if (curRack != null)
                  curRack.name = v2;
          }
       }
       catch(IOException e)
       {
            String strError = "Error reading file: "+ racksPath;
            Messages.writeStackTrace(e, strError);
            Messages.postError(strError);
       }
       if (reader != null) {
           try {
                reader.close();
            } catch(IOException ee) { }
       }
   }


   public boolean isNewInfo() {
       return newInfo;
   }

   public void setNewInfo(boolean b) {
       newInfo = b;
   }

   private void set4997(String name) {
       fileDate = 0;
       GrkPlate plate = null;
       if (typeList.size() > 0) {
           plate = typeList.elementAt(0);
           if (plate != null) {
                // if (!plate.getName().equals("as4896"))
                if (!plate.getName().equals(name))
                     plate = null;
           }
       }
       if (plate == null) {
           typeList.clear();
           // addPlate("as4896");
           addPlate(name);
       }
       else
           curPlate = plate;
       if (curPlate == null)
           return;
       int k;
       for (k = 0; k < rackList.length; k++) {
            curRack = (RackObj)rackList[k];
            if (curRack != null)
                 curRack.clear();
           // curRack = null;
       }
       curPlate.setZoneNum(2);
       curPlate.setRowNum(1, 8); 
       curPlate.setRowNum(2, 8); 
       curPlate.setColNum(1, 6); 
       curPlate.setColNum(2, 6); 
       curRack = getRack(1);
       setRackData("1", "start", "1");
       // setRackData("1", "pattern", "3");
       setRackData("1", "pattern", "1");
       setRackData("1", "zones", "2");
       setRackData("1", "1", "row", "8");
       setRackData("1", "1", "col", "6");
       setRackData("1", "2", "row", "8");
       setRackData("1", "2", "col", "6");
       // curRack.name = "as4896";
       curRack.name = name;
       curRack.trayType = trayType;
       curPlate.trayType = trayType;
   }

   public void update() {
       if (trayType == GRK49 || trayType == GRK97) {
           set4997("as4896");
           return;
       }
       if (trayType == GRK12) {
           set4997("as12");
           return;

       }
       File fd = new File(infoPath);
       if (fd.isFile() && fd.canRead()) {
          if (fileDate != fd.lastModified()) {
              fileDate = fd.lastModified();
              newInfo = true;
              buildInfo();
          }
       }
       if (rackList != null) {
           for (int k = 0; k < rackList.length; k++) {
              curRack = (RackObj)rackList[k];
              if (curRack != null)
                 curRack.clear();
           }
       }
       fd = new File(currentPath);
       if (fd.isFile() && fd.canRead())
           buildRackInfo();
       fd = new File(racksPath);
       if (fd.isFile() && fd.canRead())
           buildRackInfo2();
   }

   public Vector<GrkPlate> getPlateList() {
       return typeList;
   }

   public RackObj[] getRackList() {
       return rackList;
   }

}

