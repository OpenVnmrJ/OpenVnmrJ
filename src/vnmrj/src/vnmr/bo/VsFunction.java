/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.geom.*;
import java.util.*;

import vnmr.util.*;

public abstract class  VsFunction {
    protected int selectedKnot = -1;

    protected ColormapInfo cminfo;
    protected ColorLookupTable colorLookupTable;
    protected int bottomColorToUse;
    protected int topColorToUse;
    public double bottomFrac;
    public double topFrac;
    private boolean cltUpToDate = false;
    private Polyline curve;
    private int graphX = -1;
    private int graphY = -1;
    private int graphWidth = -1;
    private int graphHeight = -1;

    public VsFunction(ColormapInfo cmi) {
	this.cminfo = cmi;
	colorLookupTable = new ColorLookupTable(0.0, 0.009);
	setCltUpToDate(false);
    }

    static public VsFunction getFunction(String function, ColormapInfo cmi) {
        if (function == null) {
            return null;
        }
        VsFunction vsFunc = null;
        if (function.startsWith("curve")) {
            vsFunc = new VsCurveFunction(function, cmi);
        }
        return vsFunc;
    }

    public abstract java.util.List getControlPoints();
    public abstract void setControlPoints(java.util.List controlPoints);
    protected abstract void updateLookupTable();

    public void setBottomColorToUse(double frac) {
        bottomFrac = frac;
        int fc = cminfo.getFirstColor();
        int lc = cminfo.getLastColor();
        bottomColorToUse = (int)(fc + frac * (lc - fc) + 0.5);
    }
    
    public void setTopColorToUse(double frac) {
        topFrac = frac;
        int fc = cminfo.getFirstColor();
        int lc = cminfo.getLastColor();
        topColorToUse = (int)(fc + frac * (lc - fc) + 0.5);
    }
    
    public void setUflowColor(double frac) {
        int fc = cminfo.getFirstColor();
        int lc = cminfo.getLastColor();
        int color = (int)(fc + frac * (lc - fc) + 0.5);
        cminfo.setUflowColor(color);
        colorLookupTable.setUflowColor(color);
    }
    
    public void setOflowColor(double frac) {
        int fc = cminfo.getFirstColor();
        int lc = cminfo.getLastColor();
        int color = (int)(fc + frac * (lc - fc) + 0.5);
        cminfo.setOflowColor(color);
        colorLookupTable.setOflowColor(color);
    }
    
    public void setSelectedKnot(int index) {
	selectedKnot = index;
    }
    public int getSelectedKnot() { return selectedKnot; }

    public void setMinData(double minval) {
        colorLookupTable.minData = minval;
    }

    public void setMaxData(double maxval) {
        colorLookupTable.maxData = maxval;
    }

    public double getMinData() {
        return colorLookupTable.minData;
    }

    public double getMaxData() {
        return colorLookupTable.maxData;
    }

    public void moveKnot(int index, double x, double y) {
	java.util.List knotlist = getControlPoints();
	knotlist.set(index, new Point2D.Double(x, y));
	updateLookupTable();
    }
	

    protected boolean getCltUpToDate() { return cltUpToDate; }
    protected void setCltUpToDate(boolean b) {
	cltUpToDate = b;
	// New CLT means the current graph is out of date
	graphWidth = -1;
	graphHeight = -1;
    }

    public ColorLookupTable getLookupTable() {
	if (!getCltUpToDate()) {
	    updateLookupTable();
	}
	return colorLookupTable;
    }

    public void setColormapInfo(ColormapInfo cmi) {
	setCltUpToDate(false);
	this.cminfo = cmi;
    }

    /**
     * Construct a Polyline that represents a graph of the Vs function.
     * @param x0 The X-coordinate of the left side of the graph (pixels).
     * @param y0 The Y-coordinate of the top of the graph (pixels).
     * @param width The width of the graph (pixels).
     * @param height The height of the graph (pixels).
     */
    public Polyline getGraph(int x0, int y0, int width, int height) {
	if (!getCltUpToDate()) {
	    updateLookupTable();
	}
	//if (graphX == x0 && graphY == y0 &&
        //    graphWidth == width && graphHeight == height)
        //{
	//    return curve;
	//}
	int i;
	int j;
	double ficol5 = cminfo.getFirstColor() - 0.5;
	int ncols = cminfo.getNumColors();
	double fncols = ncols;
	double fh1 = height - 1;
	double fw1 = width - 1;
	int[] tbl = colorLookupTable.getTable();
	int tsize = tbl.length;
	double ftsize = tsize;

	// Need 2 pts on each color.  In worst case, lookup
	// table could change color with every table entry.
	Polyline curve = new Polyline(tsize * 2);
	int[] x = curve.x;
	int[] y = curve.y;

	int col = tbl[0];
	x[0] = x0;
	y[0] = y0 + (int)(fh1 - fh1 * (col - ficol5) / fncols + 0.5);
	for (i=j=1; i<tsize; i++) {
	    if (tbl[i] != col) {
		x[j] = x0 + (int)( (fw1 * i) / ftsize + 0.5);
		y[j] = y[j-1];
		j++;
		col = tbl[i];
		x[j] = x[j-1];
		y[j] = y0 + (int)(fh1 - fh1 * (col - ficol5) / fncols + 0.5);
		j++;
	    }
	}
	x[j] = x0 + width - 1;
	y[j] = y[j-1];
	curve.length = j + 1;
	if (cminfo.getNegative()) {
	    for (i=0; i<curve.length; i++) {
		y[i] = height - y[i];
	    }
	}
	return curve;
    }

}

