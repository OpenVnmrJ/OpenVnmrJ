/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
import java.awt.*;
import java.io.*;
import java.awt.image.*;
import java.util.List;
import java.util.Iterator;


public class VGraphics implements VGaphDef
{
   boolean  bigEndian = false;
   private boolean  bTopLayer = false;

   public VGraphics() { 
       // bigEndian = (ByteOrder.nativeOrder() == ByteOrder.BIG_ENDIAN);
   }


   public void drawYbar(DataInputStream ins, Graphics g1, Graphics g2) {
        int      k, c1;
        int      k2, d2;
        int      ybarLen, i, ds, rs;
        byte     b1, b2;
        InputStream in = (InputStream) ins;

        ybarLen = 0;
        d2 = 0;
        try {
                ybarLen = ins.readInt();
                if (aLen <= ybarLen) {
                   aLen = ybarLen + 8;
                   xPoints = new int[aLen];
                   yPoints = new int[aLen];
                }
                k = ybarLen * 4 + 16;
                if (imgByteLen < k) {
                   imgByteLen = k;
                   imgByte = new byte[k+4];
                }
                rs = ybarLen * 4;
                ds = ins.available();
                if (ds < rs) {
                    while (ds < rs) {
                       Thread.sleep(100);
                       ds = ins.available();
                    }
                    k2 = 0;
                    d2 = 0;
                    if (ds > imgByteLen)
                       ds = imgByteLen;
                    while (d2 < 4) {
                       if (k2 >= ds)
                           return;
                       c1 = in.read();
                       if (c1 == 0xEE)
                            d2++;
                       else
                            d2 = 0;
                       imgByte[k2] = (byte)c1; 
                       k2++;
                    } 
                    if (k2 != rs)
                       return;
                }
                else
                    ins.readFully(imgByte, 0, rs);
                k = 0;
                i = 0;
                if (ybarLen <= 1)
                   return;
                ybarLen--; 
                while (k < ybarLen) {
                   b1 =  imgByte[i++];
                   b2 =  imgByte[i++];
                   d2 = (int)((b1 << 8) | (b2 & 0xff));
                   b1 =  imgByte[i++]; // this is a space hold byte
                   b2 =  imgByte[i++];
                   k++;
                   if (k + d2 > ybarLen) {
                       return;
                   }
                   for (k2 = 0; k2 < d2; k2++) {
                       b1 =  imgByte[i++];
                       b2 =  imgByte[i++];
                       xPoints[k2] = (int)((b1 << 8) | (b2 & 0xff));
                       b1 =  imgByte[i++];
                       b2 =  imgByte[i++];
                       yPoints[k2] = (int)((b1 << 8) | (b2 & 0xff));
                       k++;
                   }
                   if (d2 > 1) {
                     if (g1 != null)
                        g1.drawPolyline(xPoints, yPoints, d2);
                     if (g2 != null)
                        g2.drawPolyline(xPoints, yPoints, d2);
                   }
                   else if (d2 == 1) {
                     if (g1 != null)
                        g1.drawLine(xPoints[0], yPoints[0], xPoints[0], yPoints[0]);
                     if (g2 != null)
                        g2.drawLine(xPoints[0], yPoints[0], xPoints[0], yPoints[0]);
                   }
                   d2 = 0;
                }
        }
        catch (Exception e) {
            // System.out.println(e);
            Messages.writeStackTrace(e);
            return;
        }

        if (d2 > 1) {
            if (g1 != null)
                g1.drawPolyline(xPoints, yPoints, d2);
            if (g2 != null)
                g2.drawPolyline(xPoints, yPoints, d2);
        }
   }

   private VJYbar getFreeYbar(List<VJYbar> barList) {
         Iterator<VJYbar> itr = barList.iterator();
         VJYbar ybar = null;

         while (itr.hasNext()) {
              ybar = (VJYbar)itr.next();
              if (!ybar.isValid())
                    break;
              ybar = null;
         }
         if (ybar == null) {
              ybar = new VJYbar();
              barList.add(ybar);
         }
         return ybar;
   }

   public VJYbar saveYbar(DataInputStream ins, Graphics g, List<VJYbar> barList, int lw, float alpha, boolean xorMode) {
        int      k, c1;
        int      k2, d2;
        int      ptr;
        int      ybarLen, i, ds, rs;
        byte     b1, b2;
        VJYbar   ybar = null;
        int[]    xs;
        int[]    ys;
        InputStream in = (InputStream) ins;

        ybarLen = 0;
        d2 = 0;
        try {
                ybarLen = ins.readInt();
                if (aLen <= ybarLen) {
                   aLen = ybarLen + 8;
                   xPoints = new int[aLen];
                   yPoints = new int[aLen];
                }
                k = ybarLen * 4 + 16;
                if (imgByteLen < k) {
                   imgByteLen = k;
                   imgByte = new byte[k+4];
                }
                rs = ybarLen * 4;
                ds = ins.available();
                if (ds < rs) {
                    while (ds < rs) {
                       Thread.sleep(100);
                       ds = ins.available();
                    }
                    k2 = 0;
                    d2 = 0;
                    if (ds > imgByteLen)
                       ds = imgByteLen;
                    while (d2 < 4) {
                       if (k2 >= ds)
                           return null;
                       c1 = in.read();
                       if (c1 == 0xEE)
                            d2++;
                       else
                            d2 = 0;
                       imgByte[k2] = (byte)c1; 
                       k2++;
                    } 
                    if (k2 != rs)
                       return null;
                }
                else
                    ins.readFully(imgByte, 0, rs);
                if (ybarLen < 2)
                   return null;
                ybar = getFreeYbar(barList);
                ybar.setCapacity(ybarLen+8);
                xs = ybar.getXPoints();
                ys = ybar.getYPoints();
                ybar.setSize(0);

                ybarLen--; 
                k = 0;
                i = 0;
                ptr = 0;
                while (k < ybarLen) {
                   b1 =  imgByte[i++];
                   b2 =  imgByte[i++];
                   d2 = (int)((b1 << 8) | (b2 & 0xff));
                   b1 =  imgByte[i++];
                   b2 =  imgByte[i++];
                   k++;
                   if (k + d2 > ybarLen) {
                       return null;
                   }
                   for (k2 = 0; k2 < d2; k2++) {
                       b1 =  imgByte[i++];
                       b2 =  imgByte[i++];
                       xs[ptr] = (int)((b1 << 8) | (b2 & 0xff));
                       b1 =  imgByte[i++];
                       b2 =  imgByte[i++];
                       ys[ptr] = (int)((b1 << 8) | (b2 & 0xff));
                       ptr++;
                   }
                   k += d2;
                   d2 = 0;
                }
                ybar.setSize(ptr);
                ybar.setValid(true);
        }
        catch (Exception e) {
            if (ybar != null)
               ybar.setValid(false);
            Messages.writeStackTrace(e);
            return null;
        }
        Color c = g.getColor();
        ybar.setColor(c);
        ybar.setLineWidth(lw);
        ybar.setAlpha(alpha);
        ybar.setXorMode(xorMode);
        ybar.setTopLayer(bTopLayer);
        if (!xorMode)
            return null;
        VJYbar bar0 = null;
        Iterator<VJYbar> itr = barList.iterator();
        k = ybar.getSize();
        k2 = 0;
        // if there is an identical set, remove it (xor)
        while (itr.hasNext()) {
            bar0 = (VJYbar)itr.next();
            if (bar0.isValid() && bar0 != ybar) {
                if (bar0.isXorMode() && bar0.getSize() == k) {
                    if (c.equals(bar0.getColor())) {
                        k2 = ybar.getSize();
                        if (k2 > 10)
                            k2 = 10;
                        int[] ps = bar0.getXPoints();
                        for (c1 = 0; c1 < k2; c1++) {
                            if (ps[c1] != xs[c1]) {
                                k2 = 0;
                                break;
                            }
                        } 
                        if (k2 > 0) {
                            ps = bar0.getYPoints();
                            for (c1 = 0; c1 < k2; c1++) {
                                if (ps[c1] != ys[c1]) {
                                    k2 = 0;
                                    break;
                                }
                            }
                        } 
                    }
                }
            }
            if (k2 != 0) // got an identical ybar
               break;
        }
        if (k2 <= 0)
            return null;
        bar0.setValid(false);
        ybar.setValid(false);
        return ybar;
   }

   public VJYbar saveYbar(DataInputStream ins, Graphics g, List<VJYbar> barList) {
        return saveYbar(ins, g, barList, 1, 1.0f, true);
   }

   private void removeXorBar(List<VJYbar> barList, VJYbar ybar) {
        int      k, k1, k2;
        VJYbar   bar0 = null;

        if (ybar == null)
            return;

        Color c = ybar.getColor();

        int[] xs = ybar.getXPoints();
        int[] ys = ybar.getYPoints();
        Iterator<VJYbar> itr = barList.iterator();
        k = ybar.getSize();
        k2 = 0;

        while (itr.hasNext()) {
            bar0 = (VJYbar)itr.next();
            if (bar0.isValid() && bar0 != ybar) {
                if (bar0.isXorMode() && bar0.getSize() == k) {
                    if (c.equals(bar0.getColor())) {
                        k2 = ybar.getSize();
                        if (k2 > 10)
                            k2 = 10;
                        int[] ps = bar0.getXPoints();
                        for (k1 = 0; k1 < k2; k1++) {
                            if (ps[k1] != xs[k1]) {
                                k2 = 0;
                                break;
                            }
                        }
                        if (k2 > 0) {
                            ps = bar0.getYPoints();
                            for (k1 = 0; k1 < k2; k1++) {
                                if (ps[k1] != ys[k1]) {
                                    k2 = 0;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (k2 != 0) // got an identical ybar
               break;
        }
        if (k2 <= 0)
            return;
        bar0.setValid(false);
        ybar.setValid(false);
   }

   public VJYbar savePolyline(DataInputStream ins, Graphics g,
               List<VJYbar> barList, int lw, float alpha, boolean xorMode) {
        int      k, c1, c2;
        int      k2, d2;
        int      ybarLen;
        int[]    xs;
        int[]    ys;
        VJYbar   ybar = null;
        InputStream in = (InputStream) ins;

        ybarLen = 0;
        d2 = 0;
        try {
                ybarLen = ins.readInt();
                if (ybarLen < 2)
                   return null;
                ybar = getFreeYbar(barList);
                ybar.setCapacity(ybarLen+8);
                xs = ybar.getXPoints();
                ys = ybar.getYPoints();
                ybar.setSize(0);

                k = 0;
                while (k < ybarLen) {
                   c1 = in.read();
                   c2 = in.read();
                   d2 = (c1 << 8) + c2;
                   c1 = in.read(); // dummy integer
                   c2 = in.read();
                   k++;
                   for (k2 = 0; k2 < d2; k2++) {
                       c1 = in.read();
                       c2 = in.read();
                       xs[k2] = (c1 << 8) + c2;
                       c1 = in.read();
                       c2 = in.read();
                       ys[k2] = (c1 << 8) + c2;
                       k++;
                   }
                   if (d2 > 1) {
                       Color c = g.getColor();
                       ybar.setColor(c);
                       ybar.setLineWidth(lw);
                       ybar.setAlpha(alpha);
                       ybar.setXorMode(xorMode);
                       ybar.setTopLayer(bTopLayer);
                       ybar.setValid(true);
                       ybar.setSize(d2);
                       if (xorMode)
                           removeXorBar(barList, ybar);
                       d2 = 0;
                       if (k < ybarLen) {
                            ybar = getFreeYbar(barList);
                            ybar.setCapacity(ybarLen+8);
                            xs = ybar.getXPoints();
                            ys = ybar.getYPoints();
                            ybar.setSize(0);
                       }
                   }
                }
        }
        catch (Exception e) {
            System.out.println(e);
            Messages.writeStackTrace(e);
            return null;
        }
        return ybar;
   }

   public void drawPolyline(DataInputStream ins, Graphics g1, Graphics g2) {
        int      k, c1, c2;
        int      k2, d2;
        int      ybarLen;
        InputStream in = (InputStream) ins;

        ybarLen = 0;
        d2 = 0;
        try {
                ybarLen = ins.readInt();
                if (aLen <= ybarLen) {
                   aLen = ybarLen + 8;
                   xPoints = new int[aLen];
                   yPoints = new int[aLen];
                }
                k = 0;
                while (k < ybarLen) {
                   c1 = in.read();
                   c2 = in.read();
                   d2 = (c1 << 8) + c2;
                   c1 = in.read(); // dummy integer
                   c2 = in.read();
                   k++;
                   for (k2 = 0; k2 < d2; k2++) {
                       c1 = in.read();
                       c2 = in.read();
                       xPoints[k2] = (c1 << 8) + c2;
                       c1 = in.read();
                       c2 = in.read();
                       yPoints[k2] = (c1 << 8) + c2;
                       k++;
                   }
                   if (d2 > 1) {
                     if (g1 != null)
                        g1.drawPolyline(xPoints, yPoints, d2);
                     if (g2 != null)
                        g2.drawPolyline(xPoints, yPoints, d2);
                     d2 = 0;
                   }
                }
        }
        catch (Exception e) {
            System.out.println(e);
            Messages.writeStackTrace(e);
            return;
        }

       if (d2 > 1) {
            if (g1 != null)
                g1.drawPolyline(xPoints, yPoints, d2);
            if (g2 != null)
                g2.drawPolyline(xPoints, yPoints, d2);
        }
   }


   public void drawBar(DataInputStream ins, Graphics g1, Graphics g2) {
        int	 k, c1, c2, barLen;
        InputStream in = (InputStream) ins;

        barLen = 0;
        try {
            barLen = ins.readInt();
            if (bLen <= barLen) {
                bLen = barLen + 8;
                x2Points = new int[bLen];
                y2Points = new int[bLen];
            }
            for (k = 0; k < barLen; k++) {
                c1 = in.read();
                c2 = in.read();
                x2Points[k] = (c1 << 8) + c2;
                c1 = in.read();
                c2 = in.read();
                y2Points[k] = (c1 << 8) + c2;
            }
        }
        catch (Exception e) {
            // System.out.println(e);
            Messages.writeStackTrace(e);
            return;
        }

        if (barLen > 0) {
            if (g1 != null)
                g1.drawPolyline(x2Points, y2Points, barLen);
            if (g2 != null)
                g2.drawPolyline(x2Points, y2Points, barLen);
        }
   }

   public void drawRasterImage(DataInputStream ins,
            IndexColorModel cm, Graphics g1, Graphics g2)
   {
        int y, r, d, p, rs, ds;

        try {
            rs = ins.readInt();
            imgX = ins.readInt();
            imgY = ins.readInt();
            imgRepeat = ins.readInt();

            if (imgByteLen < rs) {
                imgByteLen = rs;
                imgByte = new byte[rs+4];
                mis = null;
            }
            imgLen = rs - 4;
            if (imgRastLen != imgLen) {
                mis = null;
                imgRastLen = imgLen;
            }
            ds = ins.available();
            if (ds < rs) {
                while (ds < rs) {
                     Thread.sleep(100);
                     ds = ins.available();
                }
                d = 0;
                p = 0;
                InputStream in = (InputStream) ins;
                if (ds > imgByteLen)
                   ds = imgByteLen;
                while (d < 4) {
                   if (p >= ds)
                        return;
                   r = in.read();
                   if (r == 0xEE)
                        d++;
                   else
                        d = 0;
                   imgByte[p] = (byte)r;
                   p++;
                }
                if (p != rs) {
                    return;
                }
            }
            else
                ins.readFully(imgByte, 0, rs);
/*
            s = imgLen;
            p = 0; 
            while (s > 0) {
               d = ins.read(imgByte, p, s);
               s -= d;
               p += d;
            }
*/
        }
        catch (Exception e) {
            // System.out.println(e);
            Messages.writeStackTrace(e);
            return;
        }
        if (tk == null)
            tk = Toolkit.getDefaultToolkit();
        if (mis == null)
            mis = new MemoryImageSource(imgLen, 1, cm, imgByte, 0, imgLen);
        else 
            mis.newPixels(imgByte, cm, 0, imgLen);
        imgData = tk.createImage(mis);
        if (imgData == null)
            return;
        if (g1 != null) {
            y = imgY;
            r = imgRepeat;
            do {
                g1.drawImage(imgData, imgX, y, imgLen, 1, null);
                r--;
                y--;
            } while (r > 0);
        }
        if (g2 != null) {
            y = imgY;
            r = imgRepeat;
            do {
                g2.drawImage(imgData, imgX, y, imgLen, 1, null);
                r--;
                y--;
            } while (r > 0);
        }
   }

   public void fillPolygon(DataInputStream ins, Graphics g1, Graphics g2) {
        int      k, c1, c2;
        int      k2, d2;
        int      ybarLen;
        InputStream in = (InputStream) ins;

        ybarLen = 0;
        d2 = 0;
        try {
                ybarLen = ins.readInt();
                if (aLen <= ybarLen) {
                   aLen = ybarLen + 8;
                   xPoints = new int[aLen];
                   yPoints = new int[aLen];
                }
                k = 0;
                while (k < ybarLen) {
                   c1 = in.read();
                   c2 = in.read();
                   d2 = (c1 << 8) + c2;
                   c1 = in.read(); // dummy integer
                   c2 = in.read();
                   k++;
                   for (k2 = 0; k2 < d2; k2++) {
                       c1 = in.read();
                       c2 = in.read();
                       xPoints[k2] = (c1 << 8) + c2;
                       c1 = in.read();
                       c2 = in.read();
                       yPoints[k2] = (c1 << 8) + c2;
                       k++;
                   }
                   if (d2 > 1) {
                     if (g1 != null)
                        g1.fillPolygon(xPoints, yPoints, d2);
                     if (g2 != null)
                        g2.fillPolygon(xPoints, yPoints, d2);
                     d2 = 0;
                   }
                }
        }
        catch (Exception e) {
            // System.out.println(e);
            Messages.writeStackTrace(e);
            return;
        }

        if (d2 > 1) {
            if (g1 != null)
                g1.fillPolygon(xPoints, yPoints, d2);
            if (g2 != null)
                g2.fillPolygon(xPoints, yPoints, d2);
        }
   }


   public void dpcon(CanvasIF canvas, DataInputStream ins, Graphics g1, Graphics g2) {
        int      k, c1;
        int      k2, d, d2, i;
        int      len, dcolor;
        int      ybarLen, rs, ds;
        byte     b1, b2;
        InputStream in;

        ybarLen = 0;
        dcolor = -1;
        len = 0;
        try {
                ybarLen = ins.readInt();
                if (aLen <= ybarLen) {
                   aLen = ybarLen + 8;
                   xPoints = new int[aLen];
                   yPoints = new int[aLen];
                }
                rs = ybarLen * 4;
                k = rs + 8;
                if (imgByteLen < k) {
                   imgByteLen = k;
                   imgByte = new byte[k+4];
                }
                ds = ins.available();
                if (ds < rs) {
	            i = 2;
                    while ((ds < rs) && (i > 0)) {
                       Thread.sleep(100);
                       ds = ins.available();
		       i--;
                    }
                    k2 = 0;
                    d2 = 0;
                    in = (InputStream) ins;
                    if (ds > imgByteLen)
                       ds = imgByteLen;
                    while ((d2 < 4) && (k2 < rs)) {
                       if (k2 >= ds)
		       {
                           i = ins.available();
	                   if (i == 0)
                              return;
			   ds += i;
		       }
                       c1 = in.read();
                       if (c1 == 0xEE)
                            d2++;
                       else
                            d2 = 0;
                       imgByte[k2] = (byte)c1;
                       k2++;
                    }
                    if (k2 != rs)
                       return;
                }
                else
                    ins.readFully(imgByte, 0, rs);
                d = 0;
                k = 0;
                i = 0;
                ybarLen--;
                while (k < ybarLen) {
                   b1 =  imgByte[i++];
                   b2 =  imgByte[i++];
                   len = (int)((b1 << 8) | (b2 & 0xff));
                   b1 = imgByte[i++];
                   b2 = imgByte[i++];
                   d = (int)((b1 << 8) | (b2 & 0xff));
                   if (d != dcolor) {
                       dcolor = d;
                       Color color = canvas.getColor(d);
                       if (g1 != null)
                          g1.setColor(color);
                       if (g2 != null)
                          g2.setColor(color);
                   }
                   k++;
                   if (len + k > ybarLen) {
                       return;
                   }
                   for (k2 = 0; k2 < len; k2++) {
                       b1 = imgByte[i++];
                       b2 = imgByte[i++];
                       xPoints[k2] = (int)((b1 << 8) | (b2 & 0xff));
                       b1 = imgByte[i++];
                       b2 = imgByte[i++];
                       yPoints[k2] = (int)((b1 << 8) | (b2 & 0xff));
                   }
                   k = k + len;
                   if (len > 1) {
                     if (g1 != null)
                        g1.drawPolyline(xPoints, yPoints, len);
                     if (g2 != null)
                        g2.drawPolyline(xPoints, yPoints, len);
                     len = 0;
                   }
                }
        }
        catch (Exception e) {
            // System.out.println(e);
            Messages.writeStackTrace(e);
            return;
        }

        if (len > 1) {
            if (g1 != null)
                g1.drawPolyline(xPoints, yPoints, len);
            if (g2 != null)
                g2.drawPolyline(xPoints, yPoints, len);
        }
   }

   public void setTopLayerOn(boolean b) {
        bTopLayer = b;
   }

   private int[] xPoints;
   private int[] yPoints;
   private int[] x2Points;
   private int[] y2Points;
   private byte[] imgByte;
   private int aLen = 0;
   private int bLen = 0;
   private int imgRepeat = 0;
   private int imgByteLen = 0;
   private int imgRastLen = 0;
   private int imgX = 0;
   private int imgY = 0;
   private int imgLen = 0;
   private Image imgData = null;
   private Toolkit tk = null;
   private MemoryImageSource mis = null;
}

