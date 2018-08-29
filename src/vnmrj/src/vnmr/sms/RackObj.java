/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.sms;


public class RackObj
{
   public String name;
   public String idStr;
   public int  rackId;
   public int  zones;
   public int  rows[], cols[], numbers[];
   public int  pattern, start;
   public int  trayType = 0;
   public boolean  gilsonNumber;
   public boolean  rotated;
   public String  file;
   public GrkPlate  plate;

   public RackObj(int id) {
        clear();
        this.rackId = id;
        this.idStr = Integer.toString(id);
   }

   public void clear() {
        zones = 0;
        name = null;
        file = null;
        plate = null;
        gilsonNumber = false;
        rotated = false;
        trayType = 0;
   }

   public GrkPlate getPlate() {
       if (plate == null) {
           if (name != null && zones > 0) {
               plate = new GrkPlate(name);
               plate.setRackId(rackId);
               plate.setZoneNum(zones);
               plate.gilsonNum = gilsonNumber;
               plate.rotated = rotated;
               plate.trayType = trayType;
               plate.searchPattern = pattern;
               plate.searchStart = start;
               for (int n = 1; n <= zones; n++) {
                  plate.setRowNum(n, rows[n]);
                  plate.setColNum(n, rows[n]);
               }
           }
       }
       return plate; 
   }

   public void setZones(int n) {
        zones = n;
        int newArray[];
        int k;
        n++; 
        if (rows == null) {
            rows = new int[n];
            for (k = 0; k < n; k++)
               rows[k] = 0;
        }
        else if (rows.length < n) {
            newArray = new int[n];
            for (k = 0; k < rows.length; k++)
              newArray[k] = rows[k];
            rows = newArray;
        }
        if (cols == null) {
            cols = new int[n];
            for (k = 0; k < n; k++)
               cols[k] = 0;
        }
        else if (cols.length < n) {
            newArray = new int[n];
            for (k = 0; k < cols.length; k++)
              newArray[k] = cols[k];
            cols = newArray;
        }
        if (numbers == null) {
            numbers = new int[n];
            for (k = 0; k < n; k++)
               numbers[k] = 0;
        }
        else if (numbers.length < n) {
            newArray = new int[n];
            for (k = 0; k < numbers.length; k++)
              newArray[k] = numbers[k];
            numbers = newArray;
        }
   }

   public void setFile(String s) {
        file = s;
        gilsonNumber = false;
        if (s != null) {
            if (s.equals("gilsonNumber") || s.equals("\"gilsonNumber\""))
               gilsonNumber = true;
        }
   }

   public void setRow(int id, int num) {
        if (rows == null || rows.length <= id)
           return;
        rows[id] = num;
   }

   public void setCol(int id, int num) {
        if (cols == null || cols.length <= id)
           return;
        cols[id] = num;
   }

   public void setNumber(int id, int num) {
        if (numbers == null || numbers.length <= id)
           return;
        numbers[id] = num;
   }

   public void setGilsonNum(boolean b) {
       gilsonNumber = b;
       if (plate != null)
          plate.setGilsonNum(b);
   }

   public void setRotate(boolean b) {
       rotated = b;
       if (plate != null)
          plate.setRotate(b);
   }
}

