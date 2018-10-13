/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VIcon implements CanvasObjIF
{

    protected int nX = 0;
    protected int nY = 0;
    protected int nWidth = 0;
    protected int nHeight = 0;
    protected int nPw = 50;
    protected int nPh = 50;
    protected float rx, rx2;
    protected float ry, ry2;
    protected boolean m_bMol = false;
    protected boolean bVisible = true;
    protected boolean m_bSelected = false;
    public    String iconId;
    public    String iconFile;
    protected Image img;

    public VIcon()
    {
        this(0, 0, 20, 20, false);
    }

    public VIcon(int x, int y, int w, int h)
    {
        this(x, y, w, h, false);
    }

    public VIcon(int x, int y, int w, int h, boolean bMol)
    {
        this(x, y, w, h, bMol, null);
    }

    public VIcon(int x, int y, int w, int h, boolean bMol, Image mg)
    {
        nX = x;
        nY = y;
        nWidth = w;
        nHeight = h;
        m_bMol = bMol;
        img = mg;
        rx = 0;
        ry = 0;
        rx2 = 0.3f;
        ry2 = 0.3f;
    }

    public int getX()
    {
        return nX;
    }

    public void setX(int x)
    {
        nX = x;
    }

    public int getY()
    {
        return nY;
    }

    public void setY(int y)
    {
        nY = y;
    }

    public int getWidth()
    {
        return nWidth;
    }

    public void setWidth(int width)
    {
        nWidth = width;
    }

    public int getHeight()
    {
        return nHeight;
    }

    public void setHeight(int height)
    {
        nHeight = height;
    }

    public void setDefaultSize()
    {
         if (img == null)
             return;
         nWidth = img.getWidth(null);
         nHeight = img.getHeight(null);
    }

    public void setSizeRatio(float ratio)
    {
         nWidth = (int) ((float) nWidth * ratio);
         nHeight = (int) ((float) nHeight * ratio);
         if (nX + nWidth > nPw) {
             nX = nPw - nWidth;
             if (nX < 0) {
                 nX = 0;
                 nWidth = nPw;
             }
         }
         if (nY + nHeight > nPh) {
             nY = nPh - nHeight;
             if (nY < 0) {
                 nY = 0;
                 nHeight = nPh;
             }
         }
         setRatio(nPw, nPh, false);
    }

    public Rectangle getBounds()
    {
        return (new Rectangle(nX, nY, nWidth, nHeight));
    }

    public void setBounds(Rectangle rectangle)
    {
        setBounds(rectangle.x, rectangle.y, rectangle.width, rectangle.height);
    }

    public void setBounds(int x, int y, int width, int height)
    {
        nX = x;
        nY = y;
        nWidth = width;
        nHeight = height;
    }

    public String getIconId()
    {
        return iconId;
    }

    public void setIconId(String strIcon)
    {
        iconId = strIcon;
    }

    public String getFile()
    {
        return iconFile;
    }

    public void setFile(String strfile)
    {
        iconFile = strfile;
    }

    public boolean isMol()
    {
        return m_bMol;
    }

    public void setMol(boolean bMol)
    {
        m_bMol = bMol;
    }

    public boolean isSelected()
    {
        return m_bSelected;
    }

    public void setSelected(boolean bSelected)
    {
        m_bSelected = bSelected;
    }

    public Image getImage()
    {
        return img;
    }

    public void setImage(Image mg)
    {
        img = mg;
        if (nWidth <= 1 || nHeight <= 1)
             setDefaultSize();
    }

    public void setSize(int w, int h) {
        nWidth = w;
        nHeight = h;
    }

    public void adjustSize() {
        int orgW = nWidth;
        int orgH = nHeight;

        setDefaultSize();
        if ((nWidth == orgW) && (nHeight == orgH))
            return;
        if (nWidth < 1.0 || nHeight < 1.0)
            return;
        float f1 = (float)orgW /(float) nWidth;
        float f2 = (float)orgH / (float) nHeight;
        if (f1 > f2)
            f1 = f2;
        if (f1 < 0.2f)
            f1 = 0.2f;
        else if (f1 > 1.0f)
            f1 = 1.0f;
        nWidth = (int) (f1 * (float) nWidth);
        nHeight = (int) (f1 * (float) nHeight);
    }

    public void setRatio(int w, int h, boolean doAdjust) {
        if (w < 4 || h < 4)
            return;
        nPw = w;
        nPh = h;
        if (doAdjust) { // container size was changed, needs change position only
            adjustXY(w, h);
            return;
        }
        rx = (float) nX / (float) w;
        ry = (float) nY / (float) h;
        rx2 = (float) (w - nX - nWidth) / (float) w;
        ry2 = (float) (h - nY - nHeight) / (float) h;
        if (rx2 < 0)  rx2 = 0;
        if (ry2 < 0)  ry2 = 0;
    }

    public void adjustXY(int w, int h) {
         if (rx <= rx2)
             nX = (int) (rx * (float) w);
         else
             nX = (int) ((1.0f - rx2) * (float) w) - nWidth;
         if (ry <= ry2)
             nY = (int) (ry * (float) h);
         else
             nY = (int) ((1.0f - ry2) * (float) h) - nHeight;
    }


    public void setVisible(boolean b) {
          bVisible = b;
          if (!b)
              m_bSelected = b;
    }

    public boolean isVisible() {
          return bVisible;
    }

    public boolean isResizable() {
          return true;
    }

    public void paint(Graphics2D g, Color hcolor, boolean bBorder) {
         if (!bVisible || img == null)
            return; 
         if (nWidth <= 1 || nHeight <= 1)
            setDefaultSize();
         g.drawImage(img, nX, nY, nWidth, nHeight, null);
         if (!m_bSelected)
            return; 
         g.setColor(hcolor);
         if (bBorder) {
            g.drawRect(nX, nY, nWidth-1, nHeight-1);
            return; 
         }
         int x = nX - 1;
         int y = nY - 1;
         int y2 = nY + nHeight;
         int x2 = nX + nWidth;
         if (x < 0) x = 0;
         if (y < 0) y = 0;
         g.drawLine( x, y, x + 8, y);
         g.drawLine( x, y, x, y + 8);
         g.drawLine( x, y2, x + 8, y2);
         g.drawLine( x, y2 - 8 , x, y2);
         g.drawLine( x2 - 8, y, x2, y);
         g.drawLine( x2, y, x2, y + 8);
         g.drawLine( x2 - 8, y2, x2, y2);
         g.drawLine( x2, y2 - 8, x2, y2);
    }

    public void print(Graphics2D g, boolean bColor, int pw, int ph) {
         if (!bVisible || img == null)
            return; 
         if (nWidth <= 1 || nHeight <= 1)
            setDefaultSize();
         int x, y, w, h;
         float f1 = (float)nWidth /(float) nPw;
         float f2 = (float)nHeight / (float) nWidth;

         w = (int) ((float) pw * f1);
         h = (int) ((float) w * f2 + 0.5);
         if (rx <= rx2)
             x = (int) (rx * (float) pw);
         else
             x = pw - ((int) (rx2 * (float) pw)) - w;
         if (ry <= ry2)
             y = (int) (ry * (float) ph);
         else
             y = ph - ((int) (ry2 * (float) ph)) - h;
         if (x < 0) x = 0;
         if (y < 0) y = 0;
         g.drawImage(img, x, y, w, h, null);
    }
}
