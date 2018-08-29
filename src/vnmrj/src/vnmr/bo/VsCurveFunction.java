/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.awt.geom.*;
import java.util.*;

import vnmr.util.*;

/**
 *
 * Defines a two parameter "vscale" function that includes power
 * functions, linear mapping, and an approximation to exponential
 * functions as special cases.  The functional form is:
 *
 *	f(t) = [t / (t - a * t + a)]**b
 *
 * Ref: Christophe Schlick, Graphics Gems IV, pg. 422
 *
 * The two parameters are defined by the position of one control
 * point.
 */

public class  VsCurveFunction extends VsFunction
{
    private double a;
    private double b;
    private java.util.List controlPoints;
    private int selectedKnot = -1;
    private double y0;
    private double x1;
    private double y1;
    private double y2;
    private double yctrl = 0.5;

    public VsCurveFunction(ColormapInfo cmi) {
	super(cmi);
	controlPoints = new ArrayList();
	controlPoints.add(new Point2D.Double(0.0, 0.0));
	controlPoints.add(new Point2D.Double(0.5, 0.5));
	controlPoints.add(new Point2D.Double(1.0, 1.0));
    }

    public VsCurveFunction(String function, ColormapInfo cmi) {
        super(cmi);
	controlPoints = new ArrayList();
	controlPoints.add(new Point2D.Double(0.0, 0.0));
	controlPoints.add(new Point2D.Double(0.5, 0.5));
	controlPoints.add(new Point2D.Double(1.0, 1.0));

        if (function == null) {
            return;
        }
        StringTokenizer toker = new StringTokenizer(function);
        while (toker.hasMoreTokens()) {
            String key = toker.nextToken();
            if (key.equals("curve")) {
                double x = getTokenDouble(toker, 0.5);
                double y = getTokenDouble(toker, 0.5);
                controlPoints.set(1, new Point2D.Double(x, y));
            } else if (key.equals("imin")) {
                double y = getTokenDouble(toker, 0);
                setBottomColorToUse(y);
                //controlPoints.set(0, new Point2D.Double(0, y));
            } else if (key.equals("imax")) {
                double y = getTokenDouble(toker, 1);
                setTopColorToUse(y);
                //controlPoints.set(2, new Point2D.Double(1, y));
            }
        }
    }

    private double getTokenDouble(StringTokenizer toker, double val) {
        if (toker.hasMoreTokens()) {
            try {
                val = Double.parseDouble(toker.nextToken());
            } catch (NumberFormatException nfe) {}
        }
        return val;
    }
            
    public java.util.List getControlPoints() {
	return controlPoints;
    }

    public void setControlPoints(java.util.List controlPoints) {
	setCltUpToDate(false);
	this.controlPoints = controlPoints;
    }

    public void setSelectedKnot(int index) {
	super.setSelectedKnot(index);
	java.util.List knotlist = getControlPoints();
	y0 = ((Point2D.Double)knotlist.get(0)).getY();
	x1 = ((Point2D.Double)knotlist.get(1)).getX();
	y1 = ((Point2D.Double)knotlist.get(1)).getY();
	y2 = ((Point2D.Double)knotlist.get(2)).getY();
	if (index == 0 || index == 2) {
	    if (Math.abs(y2 - y0) > 0.01) {
		// Remember position of control point relative
		// to y0, y2
		yctrl = (y1 - y0) / (y2 - y0);
	    }
	}
    }

    /**
     * Override the base class moveKnot method in order to
     * apply special constraints.
     * The second, control, point moves when an end point moves.
     * The second point has to stay between the end points in Y.
     */
    public void moveKnot(int index, double x, double y) {
	java.util.List knotlist = getControlPoints();
	if (index == 0 || index == 2) {
	    if (index == 0) {
		y0 = y;
	    } else {		// index == 2
		y2 = y;
	    }
            knotlist.set(index, new Point2D.Double(x, y));
	    y1 = y0 + yctrl * (y2 - y0);
	} else {		// index == 1
	    y = Math.min(y, Math.max(y0,y2));
	    y1 = Math.max(y, Math.min(y0,y2));
	    x1 = x;
	}
	super.moveKnot(1, x1, y1);
    }

    protected void updateLookupTable() {

	final double bmax = 10.0;
	final double amax = 1000.0;
	int i;

	setCltUpToDate(true);

	// Initialize control points
        double x0 = ((Point2D)controlPoints.get(0)).getX();
	double y0 = ((Point2D)controlPoints.get(0)).getY();
	double x1 = ((Point2D)controlPoints.get(1)).getX();
	double y1 = ((Point2D)controlPoints.get(1)).getY();
        double x2 = ((Point2D)controlPoints.get(2)).getX();
	double y2 = ((Point2D)controlPoints.get(2)).getY();

	// Normalize x1, y1
        x1 = x0 == x2 ? 0.5 : (x1 - x0) / (x2 - x0);
        x1 = x1 < 0 ? 0 : x1 > 1 ? 1 : x1; // Clip to interval [0, 1]
        y1 = y0 == y2 ? 0.5 : (y1 - y0) / (y2 - y0);
        y1 = y1 < 0 ? 0 : y1 > 1 ? 1 : y1; // Clip to interval [0, 1]

        colorLookupTable.setFunction("curve " + Fmt.fg(6, x1)
                                     + " " + Fmt.fg(6, y1)
                                     + " imin " + Fmt.fg(6, bottomFrac)
                                     + " imax " + Fmt.fg(6, topFrac));

        // Note that the action depends on which "quadrant" the control
        // point is in.  "Quadrants" are numbered like this:
        //
        //    1 +-----------+      
        //      | \   3   / |
        //      |   \   /   |
        //      | 2   X   4 |
        //      |   /   \   |
        //      | /   1   \ |
        //    0 +-----------+
        //      0           1
        //
	int quadrant;
	if (x1 + y1 <= 1) {
	    if (x1 <= y1) {
		quadrant = 2;		// Left quadrant
		double tmp = x1;
		x1 = y1;
		y1 = tmp;
	    } else {
		quadrant = 1;		// Bottom quadrant
	    }
	} else {
	    if (x1 < y1) {
		quadrant = 3;		// Top quadrant
		x1 = 1 - x1;
		y1 = 1 - y1;
	    } else {
		quadrant = 4;		// Right quadrant
		double tmp = x1;
		x1 = 1 - y1;
		y1 = 1 - tmp;
	    }
	}

	// Determine curve parameters
	double a;
	double b;
	if (x1 == 1 || y1 == 0) {
	    a = amax;
	    b = bmax;
	} else {
	    double d1 = x1 > 0.5 ? 1 - x1 : x1;
	    b = d1 / y1;
	    if (y1 < d1 * Math.pow(2, 1.0 - bmax)) {
		b = bmax;
	    } else {
		b = Math.log(2 * d1 / y1) / Math.log(2.0);
	    }
	    a = (x1 * Math.pow(y1, -1/b) - x1) / (1 - x1);
	}

	// Construct lookup table
	final int tabsize = 1024;
	int[] ctab = colorLookupTable.getTable();
	double icol = bottomColorToUse;
	double fcol = topColorToUse;
	double ncol1 = fcol - icol;
	if (cminfo.getNegative()) {
	    icol = fcol - ncol1 * y0;
	    fcol = fcol - ncol1 * y2;
	} else {
	    fcol = icol + ncol1 * y2;
	    icol = icol + ncol1 * y0;
	}
	double ncol = fcol - icol + 1;
	if (ctab == null || ctab.length != tabsize) {
	    ctab = new int[tabsize];
	    colorLookupTable.setTable(ctab);
	}
	for (i=0, x1=0; i<tabsize; i++, x1 += 1.0/(tabsize-1)) {
	    if (x1 == 0) {
		y1 = 0;
	    } else if (x1 == 1) {
		y1 = 1;
	    } else {
		switch (quadrant) {
		  case 2:
		    y1 = a * Math.pow(x1, 1/b)
			/ (1 + Math.pow(x1, 1/b) * (a - 1));
		    break;
		  case 1:
		    y1 = Math.pow(x1 / (x1 - a * x1 + a), b);
		    break;
		  case 4:
		    y1 = 1 - (a * Math.pow(1-x1, 1/b)
			     / (1 + Math.pow(1-x1, 1/b) * (a - 1)));
		    break;
		  case 3:
		    y1 = 1 - Math.pow((1-x1) / ((1-x1) - a * (1-x1) + a), b);
		    break;
		}
	    }
	    ctab[i] = (int)(icol + y1 * ncol);
	}
    }
}

