/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.Graphics;
import java.awt.Image;
import java.awt.image.*;
import java.awt.Rectangle;

import vnmr.util.*;

//////////////////////////////////////////////////////////////////////////
//// Plot2D
/**

 **/
public class Plot2D extends PlotBox {

    /** Space for label at top of intensity scale. */
    final static private int LABEL_SPACE = 6;

    private int[] m_pixData = null;

    int width;
    int height;
    Image img;
    Image scale;
    double m_zSat = 2.5;    // Max value (saturation level)

    //protected static final int SCALE_WIDTH= 1;
    //protected static final int SCALE_HEIGHT= 134;
    
    double yFirst;
    double yLast;

    public Plot2D(int[] scalePixels){
        int width = 1;
        int height = scalePixels.length;
        setScale(scalePixels);
        scale = createImage(new MemoryImageSource(width, height, getScale(),
                                                  0, width));
    }

    protected synchronized void _drawPlot(Graphics graphics, boolean clearfirst, Rectangle drawRectangle) {

        if (img!=null){
            super.setImage(img, width, height);
            super._drawPlot(graphics, clearfirst, drawRectangle);
            drawScale(graphics);
            //drawYAxis(graphics);
        } else {
            super._drawPlot(graphics, clearfirst, drawRectangle);
            drawScale(graphics);
        }
        
    }

    public void setPlot2D(float[] vX, float[] vY, int[] vPixels){
        width= vX.length;
        height= vY.length;
        yFirst= (double) vY[0];
        yLast= (double) vY[height-1];
        img = createImage(new MemoryImageSource(width, height,
                                                vPixels, 0, width));
        //setFont (new Font ("Arial", Font.PLAIN, 10));
    }

    private void drawYAxis(Graphics graphics){
        String yF = (int) yFirst + " ";
        String yL = (int) yLast + " ";
        graphics.drawString(yF, _ulx-20, _lry);
        graphics.drawString(yL, _ulx-20, _uly);
    }

    public void setTopScale(double top) {
        m_zSat = top;
    }

    private void drawScale(Graphics graphics) {
        Rectangle legRect = super.getLegendRectangle();
        int w = legRect.width;
        int h = legRect.height - LABEL_SPACE;
        if (w < 0 || h < 0) {
            return;
        }
        int x = legRect.x;
        int y = legRect.y + LABEL_SPACE;
        // NB: Draw "scale" from bottom up, because it begins with low values
        graphics.drawImage(scale, x, y + h, w, -h, this);

        if (m_zSat <= 0) {
            return;
        }
        final int[] incr = {1, 2, 5, 10, 20, 50, 100};
        graphics.setFont(getLabelFont());
        double logTop = Math.log(m_zSat) / Math.log(10);
        double m = Math.pow(10, (int)Math.floor(logTop) - 1);
        double delta = m;
        for (int i = 0; i < incr.length; i++) {
            delta = m * incr[i];
            if (m_zSat / delta <= 5) {
                break;
            }
        }
        graphics.drawString("AU", x + 4, y - 13);
        for (double z = delta; z <= m_zSat; z += delta) {
            int py = y + (int)(h * (m_zSat - z) / m_zSat);
            graphics.drawString(Fmt.g(2, z), x, py - 1);
            graphics.drawLine(x, py, x + w - 1, py);
            //graphics.drawString("____", x, py);
        }
    }

    /**
     * Convert a y pixel position on the scale into an intensity.
     */
    public double zPixToData(int ypix) {
        // Top of the scale is at intensity = m_zSat, bottom is at 0.
        Rectangle legRect = getLegendRectangle();
        int top = legRect.y + LABEL_SPACE;
        int bottom = legRect.y + legRect.height;
        double intensity = (bottom - ypix) * m_zSat / (bottom - top);
        return intensity;
    }

    public double[] getZRange() {
        double[] rtn = {0, m_zSat};
        return rtn;
    }

    /**
     * TODO: zmin is currently ignored -- assume 0.
     */
    public void setZScaleRange(double zmin, double zmax) {
        m_zSat = zmax;
        repaint();
    }

    public boolean isInScaleRect(int x, int y) {
        Rectangle legRect = getLegendRectangle();
        return legRect.contains(x, y);
    }

    public void setScale(int[] pixels) {
        m_pixData = pixels;
    }
    
    private int[] getScale() {
        return m_pixData;
    }
}
