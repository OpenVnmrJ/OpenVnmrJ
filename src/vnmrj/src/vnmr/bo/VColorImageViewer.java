/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.lang.Math;
import java.util.*;
// import java.io.ByteArrayOutputStream;
import java.io.*;
import javax.swing.*;
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.plaf.basic.BasicSplitPaneUI;
import javax.imageio.*;


public class VColorImageViewer extends JPanel {
    BufferedImage bufImage;
    int imgW = 256;
    int imgH = 256;
    int viewW = 256;
    int viewH = 256;
    int index0 = 0;
    int indexLast = 0;
    int imgColorNum = 64;
    int colorSize = 0;
    int maxValue = 0;
    int minValue = 0;
    int errNum = 0;
    Image  viewImage;
    byte[] redByte;
    byte[] grnByte;
    byte[] bluByte;
    byte[] alphaByte;
    byte[] rawData;
    boolean bDebugMode = false;
    IndexColorModel indexCm = null;

    public VColorImageViewer()
    {


    }

    public void cloneColorModel(IndexColorModel cm) {
        if (cm == null)
            return;
        int cmSize = cm.getMapSize();
        if (colorSize < cmSize) {
             colorSize = cmSize;
             redByte = new byte[colorSize];
             grnByte = new byte[colorSize];
             bluByte = new byte[colorSize];
             alphaByte = new byte[colorSize];
         }
         cm.getReds(redByte);
         cm.getGreens(grnByte);
         cm.getBlues(bluByte);
         cm.getAlphas(alphaByte);
         indexCm = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);
    }

    public Image getImage() {
        return viewImage;
    }

    private void recreateImage() {
        if (rawData == null || indexCm == null)
            return;
        try {
            bufImage = getGraphicsConfiguration()
                   .createCompatibleImage(imgW, imgH, Transparency.TRANSLUCENT);
            Graphics2D gc = bufImage.createGraphics();
            MemoryImageSource mis = new MemoryImageSource(imgW, imgH,
                   indexCm, rawData, 0, imgW);
            viewImage = Toolkit.getDefaultToolkit().createImage(mis);
            gc.drawImage(viewImage, 0, 0, viewW, viewH, 0, 0, viewW, viewH, null);
        }
        catch (Exception e) {
           //  e.printStackTrace();
        }
        repaint();
    }

    public void setColorModel(IndexColorModel cm) {
        indexCm = cm;
        recreateImage();
    }

    public void setColors(byte[] r, byte[] g, byte[] b, byte[] a) {
         if (r == null)
            return;
         int num = r.length;
         if (colorSize < num) {
             colorSize = num;
             redByte = new byte[num];
             grnByte = new byte[num];
             bluByte = new byte[num];
             alphaByte = new byte[num];
         }
         indexCm = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);

    }

    public void setColors(int id, int[] src) {
         if (src == null)
            return;
         int num = src.length;
         if (id > 2 || num < 1)
            return;
         int lastIndex = index0 + num;
         int i, k;
         if (colorSize < lastIndex || num < 1)
            return;
         byte[] dst = null;
         if (id == 0)
            dst = redByte;
         else if (id == 1)
            dst = grnByte;
         else if (id == 2)
            dst = bluByte;
         else if (id == 3)
            dst = alphaByte;
         k = 0;
         for (i = index0; i < lastIndex; i++) {
            dst[i] = (byte) src[k];
            k++;
         }

         indexCm = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);
         recreateImage();
    }

    public void setTransparent(boolean[] src) {
         if (src == null)
            return;
         int num = src.length;
         int lastIndex = index0 + num + 1;
         int i, k;
         if (colorSize < lastIndex || num < 1)
            return;
         k = 0;
         for (i = index0; i < lastIndex; i++) {
            if (src[k])
                alphaByte[i] = 0;
            else
                alphaByte[i] = (byte) 255;
            k++;
         }
         indexCm = new IndexColorModel(8, colorSize, redByte, grnByte, bluByte, alphaByte);
         recreateImage();
    }

    public int getMaxDataValue() {
        return maxValue;
    }

    public int getMinDataValue() {
        return minValue;
    }

    public int getFirstIndex() {
         return index0;
    }

    public int getLastIndex() {
         return indexLast;
    }

    public int getColorSize() {
         return imgColorNum;
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

    public byte[] getAlphaBytes() {
         return alphaByte;
    }

    public void setDebugMode(boolean b) {
        bDebugMode = b;
    }

    // grayStart is the first index of image color used in IndexColorModel
    public void setImageColorIndex(IndexColorModel cm, int grayStart, int grayNum) {
         index0 = grayStart;
         imgColorNum = grayNum;
         indexLast = index0 + grayNum;
         cloneColorModel(cm);
    }

    private int getPixelIndex(int rgb) {
         if (redByte == null)
             return 0;
         byte r, g, b;
         int i;

         r = (byte) ((rgb  & 0xff0000) >> 16);
         g = (byte) ((rgb  & 0xff00) >> 8);
         b = (byte) (rgb  & 0xff);

         for (i = index0; i < indexLast; i++) {
             if (r == redByte[i] && g == grnByte[i] && b == bluByte[i]) {
                return i;
             }
         }
         for (i = 0; i < index0; i++) {
             if (r == redByte[i] && g == grnByte[i] && b == bluByte[i]) {
                return i;
             }
         }
         for (i = indexLast; i < colorSize; i++) {
             if (r == redByte[i] && g == grnByte[i] && b == bluByte[i]) {
                return i;
             }
         }
         errNum++;
         return 0;
    }



    public void setImage(BufferedImage img, int w, int h, IndexColorModel icm,
                              int first, int num) {
        bufImage = img; 
        if (bufImage == null)
            return;
        viewW = w;
        viewH = h;
        imgW = bufImage.getWidth(null);
        imgH = bufImage.getHeight(null);

        WritableRaster raster = img.getRaster();
        if (raster == null) 
            return;
        if (num > 1) {
            index0 = first;
            imgColorNum = num;
            indexLast = index0 + num;
        }
        if (icm == null) {
            ColorModel cm = img.getColorModel();
            if (cm != null && (cm instanceof IndexColorModel)) {
                cloneColorModel((IndexColorModel)cm);
            }
        }
        else
           cloneColorModel(icm);

        if (bDebugMode)
           System.out.println("viewer image wh: "+imgW+" "+imgH);
        DataBuffer db = raster.getDataBuffer();
        if (db == null)
            return;
        int[] data = ((DataBufferInt)db).getData();
        if (data == null || data.length < 2)
            return;
        if (rawData == null || rawData.length < data.length)
            rawData = new byte[data.length];
        byte r, g, b;
        int min, maxv;
        int  px;
        int  row, col, ptr, rgb;
        int  numRow = data.length / imgW;
        minValue = 260;
        maxValue = 0;
        errNum = 0;
        for (row = 0; row < numRow; row++) {
             ptr = row * imgW;
             for (col = 0; col < imgW; col++) {
                 px = getPixelIndex(data[ptr]);
                 rawData[ptr] = (byte) px;
                 if (px > maxValue)
                     maxValue = px;
                 if (px < minValue)
                     minValue = px;
                 ptr++;
             }
        }
        if (bDebugMode) {
           System.out.println(" image data min: "+minValue+" max: "+maxValue);
           if (errNum > 0)
              System.out.println(" colormap errors: "+errNum);
        }
        repaint();
    }

    public void setImage(BufferedImage img, IndexColorModel icm, int first,
               int num) {
        bufImage = img; 
        if (bufImage == null)
            return;
        int w = bufImage.getWidth(null);
        int h = bufImage.getHeight(null);
        setImage(img, w, h, icm, first ,num);
    }

    public void setImage(BufferedImage img) {
        setImage(img, null, 0, 0);
    }

    public void paint(Graphics g) {
        Dimension dim = getSize();
        Graphics2D g2d = (Graphics2D) g;
        super.paint(g);

        if (bufImage != null) {
            /*******
            int x = (dim.width - viewW) / 2;
            int y = (dim.height - viewH) / 2;
            int w = viewW;
            int h = viewH;

            if (x < 1) {
                x = 1;
                w = dim.width - 2;
            }
            if (y < 1) {
                y = 1;
                h = dim.height - 2;
            }

            g2d.drawImage(bufImage, x, y, x + w, y + h,
                        0, 0, viewW, viewH, null);
            *********/
            g2d.drawImage(bufImage, 0, 0,  dim.width, dim.height,
                        0, 0, viewW, viewH, null);
        }
        g.setColor(Color.blue);
        g.drawRect(1, 1, dim.width-2, dim.height - 2);
    }

}
