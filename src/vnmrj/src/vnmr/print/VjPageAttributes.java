/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import javax.print.attribute.standard.*;
import javax.print.attribute.Size2DSyntax;

public class VjPageAttributes  {

    public static int HPGL_Layout = 0; // raster number
    public static int PCL_Portrait = 1; // raster number
    public static int PCL_Landscape = 2;
    public static int PS_Portrait = 3;
    public static int PS_Landscape = 4;
    public static int PCL = 0;  // the order in formatList
    public static int PS = 1;
    public static int HPGL = 2;
    public static int DICOM = 3;
    public static int OFFNUM = 3;
    // public static String[] formatList = {"PCL", "POSTSCRIPT" , "HPGL", "DICOM"};
    public static String[] formatList = {"PCL", "POSTSCRIPT" , "HPGL"};
    public static String customStr = "Custom";
    public static String monoStr = "Mono";

    public String typeName;
    public String printCap;
    public String ppmmStr;
    public String rasterStr;
    public String raster_charsizeStr;
    public String raster_pageStr;
    public String resolutionStr;
    public String raster_resolutionStr;
    public String right_edgeStr;
    public String left_edgeStr;
    public String top_edgeStr;
    public String bottom_edgeStr;
    public String xoffsetStr;
    public String yoffsetStr;
    public String xoffset1Str;
    public String yoffset1Str;
    public String xcharp1Str;
    public String ycharp1Str;
    public String wcmaxminStr;
    public String wcmaxmaxStr;
    public String wc2maxminStr;
    public String wc2maxmaxStr;
    public String formatStr;  // HPGL, PCL, POSTSCRIPT, DICOM
    public String paperSize;
    public String paperWidthStr;
    public String paperHeightStr;
    public String paperMediaName;
    public String colorStr;
    public String linewidthStr;

    public String new_rasterStr;
    public String new_resolutionStr;
    public String new_ppmmStr;
    public String new_right_edgeStr;
    public String new_left_edgeStr;
    public String new_top_edgeStr;
    public String new_bottom_edgeStr;
    public String new_xoffsetStr;
    public String new_yoffsetStr;
    public String new_xoffset1Str;
    public String new_yoffset1Str;
    public String new_wcmaxmaxStr;
    public String new_wc2maxmaxStr;
    public String new_formatStr;
    public String new_paperSize;
    public String new_paperWidthStr;
    public String new_paperHeightStr;
    public String new_colorStr;
    public String new_linewidthStr;

    public int    raster; // plot format 0: HPGL, 1-2: PCL  3-4: PS
    public int    raster_charsize;
    public int    raster_page;
    public int    xcharp1;
    public int    ycharp1;
    public int    linkedNum; // the number of plotters using this layout
    public double  ppmm;
    public double  paperRatio;
    public double  xoffset;
    public double  yoffset;
    public double  xoffset1;
    public double  yoffset1;
    public double  X0_offsets[];
    public double  wc_maxes[];
    public double  Y0_offsets[];
    public double  wc2_maxes[];
    public double  wcmaxmin;
    public double  wcmaxmax;
    public double  wc2maxmin;
    public double  wc2maxmax;
    public double  leftMargin;
    public double  rightMargin;
    public double  topMargin;
    public double  bottomMargin;
    public double  paperWidth;
    public double  paperHeight;
    public double  actualWidth;
    public double  actualHeight;
    public double  dispWidth;
    public double  dispHeight;
    public double  drawWidth;
    public double  drawHeight;
    public double  drawXoffset;
    public double  drawYoffset;
    public boolean bSystem;
    public boolean bNewSet;
    public boolean bChanged;
    public boolean bChangeable;
    public boolean bInited;
    public boolean bRemoved;
    public boolean bOldHpgl;

    public VjPageAttributes(String name, boolean s) {
        this.typeName = name;
        this.bSystem = s;
        this.bNewSet = false;
        this.bChangeable = false;
        this.bInited = false;
        this.bRemoved = false;
        this.bChanged = false;
        this.bOldHpgl = false;
        this.linkedNum = 0;
    }

    public VjPageAttributes(String name) {
        this(name, false);
    }

    public void setChangeable(boolean s) {
         bChangeable = s;
    }

    public void init() {
         if (!bInited) {
             bInited = true;
             initialiaze();
         }
    }

    public void setDefaultType() {
        bChanged = true;
        bNewSet = true;
        // setPaper("letter");
        ppmmStr = "11.81";
        rasterStr = "4";    // ps landscape
        formatStr = formatList[PS];
        colorStr = monoStr;
        linewidthStr = "1";
        xcharp1Str = "15";
        ycharp1Str = "30";
        xoffsetStr = zeroStr;
        yoffsetStr = zeroStr;
        xoffset1Str = zeroStr;
        yoffset1Str = zeroStr;
        wcmaxminStr = "50.0";
        wcmaxmaxStr = "250.0";
        wc2maxminStr = "37.2";
        wc2maxmaxStr = "186.0";
       /*** portrait
        wcmaxminStr = "37.2";
        wcmaxmaxStr = "186.0";
        wc2maxminStr = "28.6";
        wc2maxmaxStr = "143.0";
      ***/
    }

    public static boolean checkAttrs(VjPageAttributes obj) {
        if (!obj.rasterStr.equals(obj.new_rasterStr))
           return true;
        if (!obj.resolutionStr.equals(obj.new_resolutionStr))
           return true;
        if (!obj.wcmaxmaxStr.equals(obj.new_wcmaxmaxStr))
           return true;
        if (!obj.wc2maxmaxStr.equals(obj.new_wc2maxmaxStr))
           return true;
        if (!obj.formatStr.equals(obj.new_formatStr))
           return true;
        if (!obj.paperWidthStr.equals(obj.new_paperWidthStr))
           return true;
        if (!obj.paperHeightStr.equals(obj.new_paperHeightStr))
           return true;
        if (!obj.colorStr.equals(obj.new_colorStr))
           return true;
        if (!obj.linewidthStr.equals(obj.new_linewidthStr))
           return true;
        return false;
    }

    public boolean isChanged () {
        if (!bChangeable)
           return false;
        if (bChanged || bNewSet)
           return true;
        if (bInited)
           return false;
        if (!typeName.endsWith(VjPlotterTable.appendStr))
           return false;
        init();
        return checkAttrs(this);
    }

    private void resetOffsets() {
        if (X0_offsets == null) {
            X0_offsets = new double[OFFNUM];
            wc_maxes = new double[OFFNUM];
            Y0_offsets = new double[OFFNUM];
            wc2_maxes = new double[OFFNUM];
        }
        for (int n = 0; n < OFFNUM; n++) {
            X0_offsets[n] = 0;
            wc_maxes[n] = 0;
            Y0_offsets[n] = 0;
            wc2_maxes[n] = 0;
        }
    }

    private void reset () {
        bChanged = false;
        new_rasterStr = rasterStr;
        new_resolutionStr = resolutionStr;
        new_ppmmStr = ppmmStr;
        new_right_edgeStr = right_edgeStr;
        new_left_edgeStr = left_edgeStr;
        new_top_edgeStr = top_edgeStr;
        new_bottom_edgeStr = bottom_edgeStr;
        new_xoffsetStr = xoffsetStr;
        new_yoffsetStr = yoffsetStr;
        new_xoffset1Str = xoffset1Str;
        new_yoffset1Str = yoffset1Str;
        new_wcmaxmaxStr = wcmaxmaxStr;
        new_wc2maxmaxStr = wc2maxmaxStr;
        new_formatStr = formatStr;
        new_paperSize = paperSize;
        new_paperWidthStr = paperWidthStr;
        new_paperHeightStr = paperHeightStr;
        new_colorStr = colorStr;
        new_linewidthStr = linewidthStr;
        resetOffsets();
    }

    public void resetToOrigin () {
        reset();
        normalize();
        setRasterPaperSize();
        retrieveOffsets();
        adjustPlotGeom();
    }

    public void changeWcMax() {
        normalize();
        setRasterPaperSize();
        adjustPlotGeom();
    }

    private void getDefaultPaper() {
        double w, h, t;

        if (paperWidthStr != null && paperHeightStr != null) {
            w = VjPrintUtil.getDouble(paperWidthStr); // MM
            h = VjPrintUtil.getDouble(paperHeightStr);
        }
        else {
            if (wcmaxmax <= 0 || wc2maxmax <= 0) {
               setDefaultType();
               if (raster == 0)
                   rasterStr = "0";
               normalize();
            }
            w = wcmaxmax;
            h = wc2maxmax;
            if (!bOldHpgl) {
                if (xoffset1 > 0 && xoffset1 < wcmaxmax)
                    w = w + xoffset1;
                if (yoffset1 > 0 && yoffset1 < wc2maxmax)
                    h = h + yoffset1;
            }
            if (w > h) {
                if (raster == 0 || raster == 2 || raster == 4) {
                    t = w;
                    w = h;
                    h = t;
                }
            }
            w = w + leftMargin + rightMargin;
            h = h + topMargin + bottomMargin;
            if (raster == 1 || raster == 3) {
                if (h < 279) // letter size
                    h = 279;
            }
        }
        paperSize = VjPaperMedia.getPaperName(w, h);
    }

    private void saveOffsets() {
        if (raster < 0)
            return;
        
        int index = 0; 
        if (raster > 0) {
            if (raster == 1 || raster == 3)
                index = 1;
            else
                index = 2;
        }
        X0_offsets[index] = xoffset;
        wc_maxes[index] = wcmaxmax;
        Y0_offsets[index] = yoffset;
        wc2_maxes[index] = wc2maxmax;
    }

    private void retrieveOffsets() {
        if (raster < 0)
            return;
        int index = 0; 
        if (raster > 0) {
            if (raster == 1 || raster == 3)
                index = 1;
            else
                index = 2;
        }
        xoffset = X0_offsets[index];
        wcmaxmax = wc_maxes[index];
        if (wcmaxmax <= 0)
            wcmaxmax = actualWidth - leftMargin - rightMargin - xoffset;
        yoffset = Y0_offsets[index];
        wc2maxmax = wc2_maxes[index];
        if (wc2maxmax <= 0) {
            if (index == 1)  // portrait
                wc2maxmax = actualHeight * 0.512;
            else
                wc2maxmax = actualHeight - topMargin - bottomMargin - yoffset;
        }
        new_xoffsetStr = Double.toString(xoffset);
        new_yoffsetStr = Double.toString(yoffset);
        wcmaxmax = VjPrintUtil.getDouble(wcmaxmax);
        wc2maxmax = VjPrintUtil.getDouble(wc2maxmax);
        new_wcmaxmaxStr = Double.toString(wcmaxmax);
        new_wc2maxmaxStr = Double.toString(wc2maxmax);
    }


    private void setRasterPaperSize() {
       if (new_paperWidthStr == null || new_paperHeightStr == null)
          return;
       paperWidth = VjPrintUtil.getDouble(new_paperWidthStr);
       paperHeight = VjPrintUtil.getDouble(new_paperHeightStr);
       actualWidth = paperWidth;
       actualHeight = paperHeight;
       if (raster == 1 || raster == 3) { // portrait
          if (paperWidth > paperHeight) {
              actualHeight = paperWidth;
              actualWidth = paperHeight;
          }
          return;
       }
       if (paperHeight > paperWidth) {
          actualHeight = paperWidth;
          actualWidth = paperHeight;
       }
    }

    private void adjustPlotGeom() {
       if (bOldHpgl)
           return;
       double maxv;
       double minv = actualWidth / 10.0;
       double k = leftMargin + rightMargin + xoffset;
       k = actualWidth - k;
       if ((k < minv) || (wcmaxmax > k)) {
          xoffset = 0;
          xoffset1 = 0;
          maxv = actualWidth / 3.0;
          if (leftMargin > maxv)
              leftMargin = maxv;
          if (rightMargin > maxv)
              rightMargin = maxv;
          rightMargin = VjPrintUtil.getDouble(rightMargin);
          leftMargin = VjPrintUtil.getDouble(leftMargin);
          new_right_edgeStr = Double.toString(rightMargin);
          new_left_edgeStr = Double.toString(leftMargin);
          new_xoffsetStr = zeroStr;
          new_xoffset1Str = zeroStr;
          k = actualWidth - leftMargin - rightMargin;
       }
       k = VjPrintUtil.getDouble(k);
       if (wcmaxmax > k)
          wcmaxmax = k;
       new_wcmaxmaxStr = Double.toString(wcmaxmax);

       minv = actualHeight / 10.0;
       k = topMargin + bottomMargin + yoffset;
       k = actualHeight - k;
       if ((k < minv) || (wc2maxmax > k)) {
          yoffset = 0;
          yoffset1 = 0;
          maxv = actualHeight / 3.0;
          if (topMargin > maxv)
              topMargin = maxv;
          if (bottomMargin > maxv)
              bottomMargin = maxv;
          topMargin = VjPrintUtil.getDouble(topMargin);
          bottomMargin = VjPrintUtil.getDouble(bottomMargin);
          new_top_edgeStr = Double.toString(topMargin);
          new_bottom_edgeStr = Double.toString(bottomMargin);
          new_yoffsetStr = zeroStr;
          new_yoffset1Str = zeroStr;
          k = actualHeight - topMargin - bottomMargin;
       }
       k = VjPrintUtil.getDouble(k);
       if (wc2maxmax > k)
          wc2maxmax = k;
       new_wc2maxmaxStr = Double.toString(wc2maxmax);
    }

    public void setPaper(String pname) {
        if (pname == null)
            return;
        double d;
        paperSize = pname;
        if (pname.equals(customStr)) {
            if (paperWidthStr == null) {
                d = wcmaxmax + leftMargin + rightMargin;
                paperWidthStr = Double.toString(d);
            }
            if (paperHeightStr == null) {
                d = wc2maxmax + topMargin + bottomMargin;
                paperHeightStr = Double.toString(d);
            }
        }
        else {
            MediaSize md = VjPaperMedia.getPaperMedia(pname);
            float dim[] = md.getSize(Size2DSyntax.MM);
            d = VjPrintUtil.getDouble(dim[0]);
            paperWidthStr = Double.toString(d);
            d = VjPrintUtil.getDouble(dim[1]);
            paperHeightStr = Double.toString(d);
            paperSize = VjPaperMedia.getPaperName(md);
        }
        new_paperHeightStr = paperHeightStr;
        new_paperWidthStr = paperWidthStr;
        setRasterPaperSize();
        adjustPlotGeom();
    }

    public void setNewPaper(String p) {
        if (p == null)
            return;
        resetOffsets();
        new_paperSize = p;
        if (p.equals(customStr)) {
            if (new_paperWidthStr == null) {
                if (new_wcmaxmaxStr != null)
                   new_paperWidthStr = new_wcmaxmaxStr;
                else
                   new_paperWidthStr = wcmaxmaxStr;
            }
            if (new_paperHeightStr == null) {
                if (new_wc2maxmaxStr != null)
                    new_paperHeightStr = new_wc2maxmaxStr;
                else
                    new_paperHeightStr = wc2maxmaxStr;
            }
        }
        else {
            MediaSize md = VjPaperMedia.getPaperMedia(p);
            float dim[] = md.getSize(Size2DSyntax.MM);
            double d = VjPrintUtil.getDouble(dim[0]);
            new_paperWidthStr = Double.toString(d);
            d = VjPrintUtil.getDouble(dim[1]);
            new_paperHeightStr = Double.toString(d);
            new_paperSize = VjPaperMedia.getPaperName(md);
        }
        setRasterPaperSize();
        retrieveOffsets();
        adjustPlotGeom();
    }

    public void setNewPaper(VjMediaSizeObj obj) {
        if (obj == null)
            return;
        String name = obj.getMediaName();
        MediaSize md = obj.getMediaSize();
        new_paperSize = name;
        resetOffsets();
        if (md == null || (name.equals(customStr))) {
            if (new_paperWidthStr == null) {
                if (new_wcmaxmaxStr != null)
                   new_paperWidthStr = new_wcmaxmaxStr;
                else
                   new_paperWidthStr = wcmaxmaxStr;
            }
            if (new_paperHeightStr == null) {
                if (new_wc2maxmaxStr != null)
                    new_paperHeightStr = new_wc2maxmaxStr;
                else
                    new_paperHeightStr = wc2maxmaxStr;
            }
        }
        else {
            float dim[] = md.getSize(Size2DSyntax.MM);
            double d = VjPrintUtil.getDouble(dim[0]);
            new_paperWidthStr = Double.toString(d);
            d = VjPrintUtil.getDouble(dim[1]);
            new_paperHeightStr = Double.toString(d);
        }
        setRasterPaperSize();
        retrieveOffsets();
        adjustPlotGeom();
    }

    public void setRaster(int r) {
       new_rasterStr = Integer.toString(r);
       if (raster == r)
           return;
       saveOffsets();
       raster = r;
       if (new_paperWidthStr == null) {
           if (paperWidthStr == null)
               initialiaze();
           else
               reset();
       }
       setRasterPaperSize();
       retrieveOffsets();
       adjustPlotGeom();
    }


    public void normalize() {
        raster = VjPrintUtil.getInteger(new_rasterStr); 
        xoffset = VjPrintUtil.getDouble(new_xoffsetStr); 
        yoffset = VjPrintUtil.getDouble(new_yoffsetStr); 
        xoffset1 = VjPrintUtil.getDouble(new_xoffset1Str); 
        yoffset1 = VjPrintUtil.getDouble(new_yoffset1Str); 
        xcharp1 = VjPrintUtil.getInteger(xcharp1Str);
        ycharp1 = VjPrintUtil.getInteger(ycharp1Str);
        wcmaxmax = VjPrintUtil.getDouble(new_wcmaxmaxStr); 
        wc2maxmax = VjPrintUtil.getDouble(new_wc2maxmaxStr); 
        leftMargin = VjPrintUtil.getDouble(new_left_edgeStr);
        rightMargin = VjPrintUtil.getDouble(new_right_edgeStr);
        topMargin = VjPrintUtil.getDouble(new_top_edgeStr);
        bottomMargin = VjPrintUtil.getDouble(new_bottom_edgeStr);
        if (leftMargin < 0) {
           new_left_edgeStr = zeroStr;
           leftMargin = 0;
        }
        if (rightMargin < 0) {
           new_right_edgeStr = zeroStr;
           rightMargin = 0;
        }
        if (topMargin < 0) {
           new_top_edgeStr = zeroStr;
           topMargin = 0;
        }
        if (bottomMargin < 0) {
           new_bottom_edgeStr = zeroStr;
           bottomMargin = 0;
        }
        if (xoffset < 0) {
           xoffset = 0;
           new_xoffsetStr = zeroStr;
        }
        if (xoffset1 < 0) {
           xoffset1 = 0;
           new_xoffset1Str = zeroStr;
        }
        if (yoffset < 0) {
           yoffset = 0;
           new_yoffsetStr = zeroStr;
        }
        if (yoffset1 < 0) {
           yoffset1 = 0;
           new_yoffset1Str = zeroStr;
        }
    }
    
    public void initialiaze() {
        if (rasterStr == null || wcmaxmaxStr == null || wc2maxmaxStr == null)
            setDefaultType();
        if (ppmmStr == null)
            ppmmStr = "5.905";  // 150 dpi
        double d0 = VjPrintUtil.getDouble(ppmmStr);
        int i = (int) (d0 * 25.4 + 0.5);
        resolutionStr = Integer.toString(i);
        raster = VjPrintUtil.getInteger(rasterStr); 
        if (left_edgeStr == null || top_edgeStr == null) { // old version
            if (raster == 0) {
                right_edgeStr = zeroStr; 
                left_edgeStr = zeroStr; 
                top_edgeStr = zeroStr; 
                bottom_edgeStr = zeroStr; 
            }
            else {
                right_edgeStr = marginStr; 
                left_edgeStr = marginStr;
                top_edgeStr = marginStr;
                bottom_edgeStr = marginStr;
            }
        }
        if (raster == 0) {
            formatStr = formatList[HPGL];
            colorStr = monoStr;
            linewidthStr = "1";
        }
        else {
            if (raster == PCL_Portrait || raster == PCL_Landscape) 
                formatStr = formatList[PCL];
            else
                formatStr = formatList[PS];
        }
        reset();
        normalize();
        if (paperSize == null)
            getDefaultPaper();
        if (colorStr == null)
            colorStr = monoStr;
        if (linewidthStr == null)
            linewidthStr = "1";
        setPaper(paperSize);
    }

    public void clearLinkCount () {
         linkedNum = 0;
    }

    public void increaseLinkCount () {
         linkedNum++;
    }

    public void decreaseLinkCount () {
         linkedNum--;
         if (linkedNum < 0)
            linkedNum = 0;
    }

    public int linkCount() {
         return linkedNum;
    }

    private static String duplicateString(String src) {
         if (src != null)
             return (new String(src));
         return null;
    }

    public static void cloneAttrs(VjPageAttributes obj, VjPageAttributes org) {
        obj.ppmmStr = duplicateString(org.ppmmStr);
        obj.rasterStr = duplicateString(org.rasterStr);
        obj.formatStr = duplicateString(org.formatStr);
        obj.colorStr = duplicateString(org.colorStr);
        obj.linewidthStr = duplicateString(org.linewidthStr);
        obj.raster_charsizeStr = duplicateString(org.raster_charsizeStr);
        obj.raster_pageStr = duplicateString(org.raster_pageStr);
        obj.resolutionStr = duplicateString(org.resolutionStr);
        obj.raster_resolutionStr = duplicateString(org.raster_resolutionStr);
        obj.right_edgeStr = duplicateString(org.right_edgeStr);
        obj.left_edgeStr = duplicateString(org.left_edgeStr);
        obj.top_edgeStr = duplicateString(org.top_edgeStr);
        obj.bottom_edgeStr = duplicateString(org.bottom_edgeStr);
        obj.xoffsetStr = duplicateString(org.xoffsetStr);
        obj.yoffsetStr = duplicateString(org.yoffsetStr);
        obj.xoffset1Str = duplicateString(org.xoffset1Str);
        obj.yoffset1Str = duplicateString(org.yoffset1Str);
        obj.xcharp1Str = duplicateString(org.xcharp1Str);
        obj.ycharp1Str = duplicateString(org.ycharp1Str);
        obj.wcmaxminStr = duplicateString(org.wcmaxminStr);
        obj.wcmaxmaxStr = duplicateString(org.wcmaxmaxStr);
        obj.wc2maxminStr = duplicateString(org.wc2maxminStr);
        obj.wc2maxmaxStr = duplicateString(org.wc2maxmaxStr);
        obj.paperSize = duplicateString(org.paperSize);
        obj.paperWidthStr = duplicateString(org.paperWidthStr);
        obj.paperHeightStr = duplicateString(org.paperHeightStr);
        obj.paperMediaName = duplicateString(org.paperMediaName);
        obj.bSystem = org.bSystem;
        obj.bChangeable = org.bChangeable;
        obj.bOldHpgl = org.bOldHpgl;
    }

    private static String originalVal;
    private static String newVal;
    private static String zeroStr = "0";
    private static String marginStr = "14.5";

    public static void getValue(VjPageAttributes obj, int index) {
        if (index == VjPlotDef.TYPE_NAME) {
            originalVal = obj.typeName;
            newVal = obj.typeName;
            return;
        }
        if (index == VjPlotDef.PRINTCAP) {
            originalVal = obj.printCap;
            newVal = obj.printCap;
            return;
        }
        if (index == VjPlotDef.PPMM) {
            newVal = obj.new_ppmmStr;
            if (obj.ppmmStr == null)
               obj.ppmmStr = "11.81"; // 300 dpi
            originalVal = obj.ppmmStr;
            return;
        }
        if (index == VjPlotDef.RASTER) {
            newVal = obj.new_rasterStr;
            if (obj.rasterStr == null)
               obj.rasterStr = "4"; // PS_Landscape
            originalVal = obj.rasterStr;
            return;
        }
        if (index == VjPlotDef.RESOLUTION) {
            newVal = obj.new_resolutionStr;
            if (obj.resolutionStr == null)
               obj.resolutionStr = "300"; // 300 dpi
            originalVal = obj.resolutionStr;
            return;
        }
        if (index == VjPlotDef.OFFSET_X) {
            newVal = obj.new_xoffsetStr;
            originalVal = obj.xoffsetStr;
            return;
        }
        if (index == VjPlotDef.OFFSET_Y) {
            newVal = obj.new_yoffsetStr;
            originalVal = obj.yoffsetStr;
            return;
        }
        if (index == VjPlotDef.OFFSET_1_X) {
            newVal = obj.new_xoffset1Str;
            originalVal = obj.xoffset1Str;
            return;
        }
        if (index == VjPlotDef.OFFSET_1_Y) {
            newVal = obj.new_yoffset1Str;
            originalVal = obj.yoffset1Str;
            return;
        }
        if (index == VjPlotDef.CHAR_SIZE) {
            newVal = obj.raster_charsizeStr;
            originalVal = zeroStr;
            return;
        }
        if (index == VjPlotDef.CHAR_XSIZE) {
            newVal = obj.xcharp1Str;
            originalVal = zeroStr;
            return;
        }
        if (index == VjPlotDef.CHAR_YSIZE) {
            newVal = obj.ycharp1Str;
            originalVal = zeroStr;
            return;
        }
        if (index == VjPlotDef.WC_MIN) {
            newVal = zeroStr;
            originalVal = zeroStr;
            return;
        }
        if (index == VjPlotDef.WC_MAX) {
            newVal = obj.new_wcmaxmaxStr;
            if (obj.wcmaxmaxStr == null)
                obj.wcmaxmaxStr = "250.0";
            originalVal = obj.wcmaxmaxStr;
            return;
        }
        if (index == VjPlotDef.WC2_MIN) {
            newVal = zeroStr;
            originalVal = zeroStr;
            return;
        }
        if (index == VjPlotDef.WC2_MAX) {
            newVal = obj.new_wc2maxmaxStr;
            if (obj.wc2maxmaxStr == null)
                obj.wc2maxmaxStr = "186.0";
            originalVal = obj.wc2maxmaxStr;
            return;
        }
        if (index == VjPlotDef.FORMAT) {
            newVal = obj.new_formatStr;
            if (obj.formatStr == null) {
                if (obj.raster == 0) 
                    obj.formatStr = formatList[HPGL];
                else {
                    if (obj.raster <= PCL_Landscape) 
                        obj.formatStr = formatList[PCL];
                    else
                        obj.formatStr = formatList[PS];
                }
            }
            originalVal = obj.formatStr;
            return;
        }
        if (index == VjPlotDef.COLOR) {
            newVal = obj.new_colorStr;
            if (obj.colorStr == null) {
                 obj.colorStr = monoStr;
            }
            originalVal = obj.colorStr;
            return;
        }
        if (index == VjPlotDef.PAPER) {
            if (obj.new_paperSize == null)
               obj.new_paperSize = obj.paperSize;
            newVal = obj.new_paperSize;
            originalVal = obj.paperSize;
            return;
        }
        if (index == VjPlotDef.PAPER_WIDTH) {
            newVal = obj.new_paperWidthStr;
            originalVal = obj.paperWidthStr;
            return;
        }
        if (index == VjPlotDef.PAPER_HEIGHT) {
            newVal = obj.new_paperHeightStr;
            originalVal = obj.paperHeightStr;
            return;
        }
        if (index == VjPlotDef.RIGHT_EDGE) {
            newVal = obj.new_right_edgeStr;
            originalVal = obj.right_edgeStr;
            return;
        }
        if (index == VjPlotDef.LEFT_EDGE) {
            newVal = obj.new_left_edgeStr;
            originalVal = obj.left_edgeStr;
            return;
        }
        if (index == VjPlotDef.TOP_EDGE) {
            newVal = obj.new_top_edgeStr;
            originalVal = obj.top_edgeStr;
            return;
        }
        if (index == VjPlotDef.BOTTOM_EDGE) {
            newVal = obj.new_bottom_edgeStr;
            originalVal = obj.bottom_edgeStr;
            return;
        }
        if (index == VjPlotDef.LINEWIDTH) {
            newVal = obj.new_linewidthStr;
            originalVal = obj.linewidthStr;
            return;
        }
        newVal = null;
        originalVal = null;
    }

    public static String getAttr(VjPageAttributes obj, int index) {
        if (obj == null)
            return null;
        getValue(obj, index);
        if ((newVal != null) && (newVal.length() > 0))
            return newVal;
        return originalVal;
    }

} // end of VjPageAttributes

