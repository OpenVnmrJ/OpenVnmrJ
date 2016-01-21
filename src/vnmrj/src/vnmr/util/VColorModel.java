/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import java.awt.image.IndexColorModel;


public class VColorModel {
     private static boolean bDebugMode = false;
     private String name = null;
     private String mapName = null;
     private String filePath = null;
     private IndexColorModel model = null;
     private IndexColorModel transparentModel = null;
     private boolean bTranslucency = true;
     private double translucency = 0.0;
     private long  fileTime = 0;
     private int id = 0;
     private int colorSize = 0;
     private int alphaValue = 255;
     private int transparentValue = 255;
     private byte[] redByte;
     private byte[] grnByte;
     private byte[] bluByte;
     private byte[] alphaByte;
     private boolean[] bTransparent;
     private boolean[] bTranslucent;

     public VColorModel(int idNum, String nameStr, String path, boolean bTranslucent) {
         this.name = nameStr;
         this.id = idNum;
         this.bTranslucency = bTranslucent;
         this.colorSize = 0;
         if (bDebugMode) {
            System.out.println("new VColorModel id "+idNum);
            System.out.println("   name: "+nameStr);
            System.out.println("   path: "+path);
         }
         load(path);
     }

     public VColorModel(String path) {
         this(0, path, path, true); 
     }

     public static void setDebugMode(boolean b) {
        bDebugMode = b;
     }

     private void setBaseSize(int n) {
         if (bDebugMode)
            System.out.println("VColorModel "+id+": set size to "+n);
         if (n < 2)
            return;
         if (n > 254)
            n = 254;
         int newSize = n + 2; // add two colors for underflow and overflow
         if (newSize == colorSize)
            return;
         colorSize = newSize;
         redByte = new byte[colorSize];
         grnByte = new byte[colorSize];
         bluByte = new byte[colorSize];
         alphaByte = new byte[colorSize];
         bTransparent = new boolean[colorSize];
         bTranslucent = new boolean[colorSize];
         for (int i = 0; i < colorSize; i++) {
                redByte[i] = 0; 
                grnByte[i] = 0; 
                bluByte[i] = 0; 
                alphaByte[i] = (byte) 255; // opaque
                bTransparent[i] = false;
                bTranslucent[i] = true;
         }
         bTransparent[0] = true;
         alphaByte[0] = 0;
        /**
         bTransparent[colorSize - 1] = true;
         alphaByte[colorSize - 1] = 0;
        **/
     }

     public IndexColorModel getModel() {
         return model;
     }

     public IndexColorModel getModel(int alpha) {
         if (Math.abs(alpha - alphaValue) < 2)
             return model;
         if (Math.abs(alpha - transparentValue) < 2) {
             if (transparentModel != null)
                return transparentModel;
         }
         transparentModel = cloneModel(alpha);
         transparentValue = alpha;
      
         return transparentModel;
     }

     public String getName() {
         return name;
     }

     public String getMapName() {
         return mapName;
     }

     public void setName(String str) {
         name = str;
         if (mapName == null)
             mapName = str;
     }

     public String getFilePath() {
         return filePath;
     }

     public int getId() {
         return id;
     }

     public void setModel(IndexColorModel cm) {
         model = cm;
     }

     public void clear() {
         model = null;
         transparentModel = null;
         redByte = null;
         grnByte = null;
         bluByte = null;
         alphaByte = null;
         bTransparent = null;
         bTranslucent = null;
     }

     public IndexColorModel cloneModel() {
         IndexColorModel cm = null;
         if (colorSize < 4)
            return model;
         if (bTranslucency)
            cm = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);
         else
            cm = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, 0);
         return cm;
     }

     public IndexColorModel cloneModel(int alpha) {
         if (!bTranslucency || colorSize < 4)
            return model;
         if (bDebugMode)
            System.out.println("cloneModel "+id+": "+alpha);
         if (alpha >= 253)
            return model;

         byte a = (byte) alpha;
         for (int i = 0; i < colorSize; i++) {
             if (!bTransparent[i]) {
                 if (bTranslucent[i]) {
                      alphaByte[i] = a;
                 }
             }
         }
         IndexColorModel cm = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);
         return cm;
     }

     public void createModel() {
        if (colorSize < 2)
            return;
        if (bTranslucency)
            model = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);
        else
            model = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, 0);
     }

     private void addColor(int id, StringTokenizer tok) {
         int  r, g, b, tr;
         double d;

         if (id >= colorSize)
            return;
         try {
             if (!tok.hasMoreTokens())
                 return;
             d = Double.parseDouble(tok.nextToken());
             r = (int) (d * 255.0);
             if (r > 255)
                 r = 255;

             if (!tok.hasMoreTokens())
                 return;
             d = Double.parseDouble(tok.nextToken());
             g = (int) (d * 255.0);
             if (g > 255)
                 g = 255;
             if (!tok.hasMoreTokens())
                 return;
             d = Double.parseDouble(tok.nextToken());
             b = (int) (d * 255.0);
             if (b > 255)
                 b = 255;

             redByte[id] = (byte) r; 
             grnByte[id] = (byte) g;
             bluByte[id] = (byte) b;

             tr = 0;
             if (tok.hasMoreTokens())
                 tr = Integer.parseInt(tok.nextToken());
             if (tr != 0)
                bTransparent[id] = true;
             else
                bTransparent[id] = false;

             tr = 1;
             if (tok.hasMoreTokens())
                 tr = Integer.parseInt(tok.nextToken());
             if (tr == 0)
                bTranslucent[id] = false;
             else
                bTranslucent[id] = true;
         }
         catch (NumberFormatException ex) { }
         if (bTransparent[id])
             alphaByte[id] = 0;
         else {
             if (bTranslucent[id])
                 alphaByte[id] = (byte) alphaValue;
             else
                 alphaByte[id] = (byte) 255;
         }
     }

     private void loadMap(String path) {
         File fd = null;

         if (path != null)
            fd = new File(path);
         if (fd == null || !fd.exists() || !fd.canRead()) {
             if (colorSize <= 0)
                 setMapSize(66);
             fileTime = 0;
             return;
         }
         fileTime = fd.lastModified();
         filePath = path;
         translucency = 0.0;
         alphaValue = 255;  // opaque

         BufferedReader ins = null;
         String line;
         boolean bBegin = false;
         try {
            ins = new BufferedReader(new FileReader(path));
            while ((line = ins.readLine()) != null) {
                if (line.startsWith("#"))
                    continue;
                StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
                if (tok.hasMoreTokens()) {
                    String data = tok.nextToken();
                    if (data.equals(VColorMapFile.SIZE_NAME)) {
                        if (tok.hasMoreTokens()) {
                           data = tok.nextToken();
                           int num = Integer.parseInt(data);
                           setBaseSize(num); 
                        }
                        continue;
                    }
                    if (data.equals(VColorMapFile.TRANSLUCENCY_NAME)) {
                        if (tok.hasMoreTokens()) {
                            translucency = Double.parseDouble(tok.nextToken());
                            double dv = 1.0 - translucency;
                            if (dv < 0.0)  dv = 0.0;
                            if (dv > 1.0)  dv = 1.0;
                            dv = dv * 255.0;
                            alphaValue = (int) dv;
                        }
                        continue;
                    }

                    if (data.equals(VColorMapFile.BEGIN_NAME)) {
                        bBegin = true;
                        continue;
                    }
                    if (data.equals(VColorMapFile.END_NAME)) {
                        break;
                    }
                    if (bBegin) {
                        int id = Integer.parseInt(data);
                        if (tok.hasMoreTokens())
                           addColor(id, tok);
                    }
                }
            }
         } catch (Exception e) {  }
         finally {
            try {
                if (ins != null)
                    ins.close();
            }
            catch (Exception e2) {}
         }
         if (colorSize < 2)
            setMapSize(66);

         File pfd = null;
         if (path.endsWith(VColorMapFile.MAP_NAME))
             pfd = fd.getParentFile();
         if (pfd != null && pfd.exists())
             mapName = pfd.getName();
         else
             mapName = name;
     }

     public void load(String path) {
         if (bDebugMode)
            System.out.println("VColorModel "+id+": loading "+path);
         if (path == null)
             return;
         if (path.equals(filePath)) {
            File fd = new File(path);
            if (fileTime == fd.lastModified()) {
               if (bDebugMode)
                  System.out.println("  same file, no change. ");
               return;
            }
         }
         loadMap(path);
         createModel();
         transparentValue = alphaValue;
         transparentModel = model;
     }

     public int getMapSize() {
         if (model != null)
             return model.getMapSize();
         return 0;
     }

     public void setMapSize(int n) {
         if (getMapSize() == n)
            return;
         int baseNum = n - 2;
         if (baseNum < 6)
            baseNum = 64;
         if (baseNum > 254)
            baseNum = 254;
         setBaseSize(baseNum);

         if (bDebugMode)
            System.out.println("  set default grayscale.");
         float dv, df;
         byte  iv;
         int i;
         df = 255.0f / (float) baseNum;
         dv = 0.0f;
         for (i = 1; i <= baseNum; i++) { // set gray colors
             iv = (byte) dv;
             redByte[i] = iv;
             grnByte[i] = iv;
             bluByte[i] = iv;
             alphaByte[i] = (byte) 255;
             dv += df;
             if (dv > 255.0f)
                dv = 255.0f;
         }
     }

     public void setGradientMap(float startValue, float endValue) {
         float dv, df, v0;
         byte  iv;
         int i;

         int num = getMapSize();
         if (num < 8)
             return;
         num = num - 2;
         if (startValue < 0.0f)
             startValue = 0.0f;
         else if (startValue > 255.0f)
             startValue = 255.0f;
         if (endValue < 0.0f)
             endValue = 0.0f;
         else if (endValue > 255.0f)
             endValue = 255.0f;
         df = (endValue - startValue) / (float) num;
         dv = startValue;
         for (i = 1; i <= num; i++) {
             iv = (byte) dv;
             redByte[i] = iv;
             grnByte[i] = iv;
             bluByte[i] = iv;
             dv += df;
             if (dv > 255.0f)
                dv = 255.0f;
         }
         redByte[0] = redByte[1];
         grnByte[0] = grnByte[1];
         bluByte[0] = bluByte[1];
         redByte[num+1] = redByte[num];
         grnByte[num+1] = grnByte[num];
         bluByte[num+1] = bluByte[num];
     }


     public byte[] getRedBytes() {
         return redByte;
     }

     public byte[] getGreenBytes() {
         return grnByte;
     }

     public byte[] getBlueBytes() {
         return bluByte;
     }

     public boolean[] getTransparentValues() {
         return bTransparent;
     }

     public boolean[] getTranslucentValues() {
         return bTranslucent;
     }

     public int getAlphaValue() {
         return alphaValue;
     }

     public double getTranslucency() {
         return translucency;
     }
}

