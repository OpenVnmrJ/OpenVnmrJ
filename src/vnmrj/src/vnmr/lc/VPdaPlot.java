/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;


/**
 * This VObj object displays a plot of a PDA spectrum.
 */
public class VPdaPlot extends VPlot {

    private PdaCorbaClient m_corbaClient = null;
    private boolean firsttime = true;
    private double m_xFirst = -1;
    private double m_xLast = -1;
    private int m_itr = 0;      // For testing
    //private VContainer m_lcGraphPanel;

    PdaScan m_scan= new PdaScan();

    public VPdaPlot(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);

        try {
            m_plot.setXAxis(true);
            m_plot.setXLabel("Wavelength (nm)");
            m_plot.setYAxis(true);
            m_plot.setYLabel("Absorption (AU)");
            m_plot.setTitle("UV Spectrum");
            //m_plot.setGrid(false);
            //m_plot.setImpulses(true);
            //m_lcGraphPanel= (vif).getLcPanel();
            
        
        } catch (Exception e) {
            Messages.postError("Error creating PDA plot");
            Messages.writeStackTrace(e);
        }
    }
    
    private void drawPlot(int[] data, Plot plot) {
        
        int nPts = data.length / 2;

        plot.clear(false);

        for (int i = 0; i < nPts; i++) {
            plot.addPoint(0, (double) data[2 * i] / 256.0,
                          (double) data[2 * i + 1], false);
        }
    }


    public void showCurrentPdaData(Plot plot, PdaScan scan) {
        //plot.clear(false);
        //plot.setShowZeroY(true);
        //plot.repaint();
        if (firsttime) {
            firsttime = false;
            //corbaClient.setScanOn(true);
        }
        //Messages.postDebug("PdaData",
        //                   "showCurrentPdaData(): ...got Data");

        int npts = scan.x.length;
        if (scan == null) {
            //Messages.postDebug("Null PDA scan returned");
            return;
        }
        int len = scan.y.length;
        if (len != npts) {
            Messages.postDebug("showCurrentPdaData: "
                               + "have " + len + " data points for "
                               + npts + " wavelengths");
            len = Math.min(len, npts);
        }
        /*if (len < npts) {
            Messages.postDebug("showCurrentPdaData: "
                               + "Not enough data in scan array; have "
                               + len + ", need >=" + npts);
            return;
            }*/
        Messages.postDebug("PdaData", "PDA Data: len=" + len);
        float firstLambda = scan.x[0];
        float lastLambda = scan.x[npts - 1];
        double xFirst = firstLambda;
        double xLast = lastLambda;

        plot.setXFillRange(xFirst, xLast);
        if (xFirst != m_xFirst || xLast != m_xLast) {
            plot.setBars(false);
            plot.setConnected(true);
            plot.setXRange(xFirst, xLast);
            m_xFirst = xFirst;
            m_xLast = xLast;
        }

        // NB: This steps through the data set with each call
        //int idx = npts * m_itr;
        //if (idx + npts - 1 > len) {
        //    idx = m_itr = 0;
        //}
        //++m_itr;
        //for (int i = idx; i < idx + npts; i++) {
        //    Messages.postDebug("PdaData+",
        //                       "lambda[" + i + "]=" + scan.x[i]
        //                       + ", inten=" + scan.y[i]);
        //    plot.addPoint(0, scan.x[i], scan.y[i]*1000, true);
        //}
        plot.setPoints(0, m_scan.dX, m_scan.dY, len, true, true);
        //plot.repaint();

    }

    public void setTitle(String title) {
        m_plot.setTitle(title);
    }

    public void setYArray(float y[]){
         m_scan.setY(y);
         showCurrentPdaData(m_plot, m_scan);
    }

    public void setXArray(float x[]){
        m_scan.setX(x);
    }


    public class PdaScan{

        public float x[]= null;
        public float y[]= null;
        public double dX[]= null;
        public double dY[]= new double[0];

        public PdaScan(){

        }

        public void setX(float[] newX){
            x = newX;
            int len = x.length;
            dX = new double[len];
            for (int i = 0; i < len; i++) {
                dX[i] = x[i];
            }
        }

        public void setY(float[] newY){
            y = newY;
            int len = y.length;
            if (dY.length != len) {
                dY = new double[len];
            }
            for (int i = 0; i < len; i++) {
                dY[i] = y[i];
            }
        }
    }
    
}
