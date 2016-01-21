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
import java.util.*;
import java.io.*;
import javax.swing.*;
import javax.swing.event.MouseInputAdapter;
import java.awt.geom.*;
import java.awt.event.*;
import java.beans.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.templates.*;


/**
 * Title:        VVsControl
 * Description:
 */

public class VVsControl extends VPlot {

    private VsFunction vsFunc;
    private ColormapInfo cminfo;
    private BkgHandler bkgHandler;
    private String bkgValExpr = null;
    private RangeHandler rangeHandler;
    private String rangeString;
    private String rangeValExpr;
    private String grayRampFile;
    private ArrayList grayRamp;
    private CltHandler cltHandler;
    private String cltFile = null;
    private String cltFileExpr = null;
    private boolean mouseDown = false;
    private boolean mouseIn = false;

    // The following track the size and position of the rectangle
    // bounding the Vs function within the canvas.
    private double xrect0;
    private double yrect0;
    private double xrect1;
    private double yrect1;

    // The following are set by updateScales()
    // xpix = xoffset + fx * xscale
    // xscale2 = xscale**2;
    private int[] canXRange;
    private int[] canYRange;
    private double xscale;
    private double xscale2;
    private double xoffset;
    private double yscale;
    private double yscale2;
    private double yoffset;
    private java.util.List knotlist;
    private int nknots;
    private int pixstx;
    private int pixsty;
    private int pixendx;
    private int pixendy;
    private int pixLasty;
    private int pixFirsty;

    /**
     * Constructor
     * @param sshare The session share object.
     * @param vif The button callback interface.
     * @param typ The type of the object.
     */
    public VVsControl(SessionShare sshare, ButtonIF vif, String typ)
    {
	super(sshare, vif, typ);

        m_plot = new VsControlPlot();
        m_plot.setOpaque(false);
        add(m_plot);

	cminfo = new ColormapInfo("Test cms", 0, 64);/*CMP*/
	vsFunc = new VsCurveFunction(cminfo);
        bkgHandler = new BkgHandler();
        rangeHandler = new RangeHandler();
        cltHandler = new CltHandler();
        grayRamp = new ArrayList(cminfo.getNumColors());
        setGrayLevels((String)null);

        xrect0 = 0;
        xrect1 = 1;
        yrect0 = 0;
        yrect1 = 1;
        updateScales();
        vsFunc.setTopColorToUse(1);
    }

    /**
     *  Returns the attributes for this graph.
     */
    public Object[][] getAttributes()
    {
	return attributes;
    }

    /**
     *  Attribute array for panel editor (ParamInfoPanel)
     */
    private final static Object[][] attributes = {
	{new Integer(VARIABLE),     "Vnmr Variables:"},
	{new Integer(SETVAL),       "Value of Item:"},
	{new Integer(SHOW),         "Enable Condition:"},
	{new Integer(CMD),          "Vnmr Command:"},
	{new Integer(GRAPHBGCOL),   "Background Color:", "color"},
	{new Integer(GRAPHFGCOL),   "Histogram Color:", "color"},
	//{new Integer(FILLHISTGM),   "Fill Histogram:", "radio", m_arrStrYesNo},
	{new Integer(LOGYAXIS),     "Log Y-axis:", "radio", m_arrStrYesNo},
	{new Integer(XAXISSHOW),    "Show X-axis:", "radio", m_arrStrYesNo},
	{new Integer(XLABELSHOW),   "Show X-axis label:", "radio",
         m_arrStrYesNo},
	{new Integer(YAXISSHOW),    "Show Y-axis:", "radio", m_arrStrYesNo},
	{new Integer(YLABELSHOW),   "Show Y-axis label:", "radio",
         m_arrStrYesNo},
	{new Integer(TICKCOLOR),    "Tick Color:", "color"},
	{new Integer(SHOWGRID),     "Show Grid:", "radio", m_arrStrYesNo},
	{new Integer(GRIDCOLOR),    "Grid Color:", "color"},
	{new Integer(RANGE),        "String Specifying the Function:"},
	{new Integer(PATH1),        "File for Color Lookup Table:"},
    };

    /**
     *  Sets the value for various attributes.
     *  @param attr attribute to be set.
     *  @param c    value of the attribute.
     */
    public void setAttribute(int attr, String c) {
	if (c != null) {
	    c = c.trim();
	    if (c.length() == 0) {c = null;}
	}

	switch (attr) {
          case PATH1:
            cltFileExpr = c;
            break;
          case RANGE:
            rangeValExpr = c;
            break;
          default:
            super.setAttribute(attr, c);
            break;
	}
    }
            
    /**
     *  Gets that value of the specified attribute.
     *  @param attr attribute whose value should be returned.
     */
    public String getAttribute(int attr) {
	String strAttr;
	switch (attr) {
          case PATH1:
            strAttr = cltFileExpr;
            break;
          case RANGE:
            strAttr = rangeValExpr;
            break;
          default:
            strAttr = super.getAttribute(attr);
            break;
        }
        return strAttr;
    }

    public void updateValue() {
        super.updateValue();
        bkgHandler.updateValue();
        rangeHandler.updateValue();
        cltHandler.updateValue();
    }

    public boolean setValue(String key, String keyval) {
        boolean change = false;
        if (key.equals("vsRange")) {
            setVsRange(keyval);
        } else {
            change = super.setValue(key, keyval);
        }
        return change;
    }

    public void setValue(ParamIF pf) {
        if (pf != null)
        {
            super.setValue(pf);
            vsFunc.setColormapInfo(cminfo);
        }
    }

    public void updateGraph(Graphics2D g2) {
        if ( nknots < 1) {
            return;
        }
        updateScales();

        // Draw line
        Polyline graph = vsFunc.getGraph(pixstx,
                                         canYRange[0],
                                         pixendx - pixstx,
                                         canYRange[1] - canYRange[0]);
        g2.setColor(Color.red);
        g2.drawPolyline(graph.x, graph.y, graph.length);

        // Draw knots
        int sel = vsFunc.getSelectedKnot();
        for (int i=0; i<nknots; i++) {
            // Draw unselected knots
            if (i != sel) {
                drawKnot(g2, i, Color.red);
            }
        }
        if (sel >= 0 && sel < nknots) {
            // Draw selected knot
            drawKnot(g2, sel, Color.green);
        }
        
    }

    protected void setGrayLevels(String path) {
        grayRamp.clear();
        int nc = cminfo.getNumColors();

        if (path == null) {
            for (int i=0; i<nc; ++i) {
                int gray = (i * 255) / nc;
                grayRamp.add(new Color(gray, gray, gray));
            }
        } else {
            try {
                BufferedReader in = new BufferedReader(new FileReader(path));
                String line;
                for (int i=0; i<nc && (line=in.readLine()) != null; ++i) {
                    StringTokenizer tok = new StringTokenizer(line);
                    try {
                        int c1, c2, c3;
                        if (tok.hasMoreTokens()) {
                            c1 = Integer.parseInt(tok.nextToken());
                            if (tok.hasMoreTokens()) {
                                c2 = Integer.parseInt(tok.nextToken());
                                if (tok.hasMoreTokens()) {
                                    c3 = Integer.parseInt(tok.nextToken());
                                    grayRamp.add(new Color(c1, c2, c3));
                                }
                            }
                        }
                    } catch (NumberFormatException nfe) {}
                }
                in.close();
            } catch (IOException e) {
                Messages.writeStackTrace(e);
            }
        }
    }

    private void updateSelection(int x, int y) {
        final double aperture = 7;
        updateScales();
        double fx = (x - xoffset) / xscale;
        double fy = (y - yoffset) / yscale;

        // Are we near an existing point?
        int knot = -1;	// Index of selected knot
        double tst = aperture * aperture;
        for (int i=0; i<nknots; ++i) {
            Point2D.Double pt = (Point2D.Double)knotlist.get(i);
            double fx0 = pt.getX();
            double fy0 = pt.getY();
            double d2 = (fx-fx0)*(fx-fx0)*xscale2 + (fy-fy0)*(fy-fy0)*yscale2;
            if (d2 <= tst) {
                knot = i;
                tst = d2;	// Keep looking for a closer one
            }
        }

        if (knot != vsFunc.getSelectedKnot()) {
            // Knot selection changed
            vsFunc.setSelectedKnot(knot);
            repaint();          // TODO: Only need to redraw knots.
        }
    }

    private void moveKnot(int knot, int x, int y) {
        if (x < canXRange[0]) { x = canXRange[0]; }
        if (x > canXRange[1]) { x = canXRange[1]; }
        if (y < canYRange[0]) { y = canYRange[0]; }
        if (y > canYRange[1]) { y = canYRange[1]; }
        knotlist = vsFunc.getControlPoints();
        nknots = knotlist.size();

        double fx;
        double fy;
        if (knot == 0 || knot == nknots-1) {
            // NB: Don't let 1st and last knots be at same Y position
            if (knot == 0) {
                // Keep first knot to left of last knot
                x = pixstx = (x >= pixendx) ? pixendx - 1 : x;
                fx = 0;
                fy = ((Point2D)knotlist.get(0)).getY();
                if (y < pixLasty) {
                    fy = 1;
                    pixendy = pixLasty;
                    pixFirsty = pixsty = y;
                } else if (y > pixLasty) {
                    fy = 0;
                    pixsty = pixLasty;
                    pixFirsty = pixendy = y;
                }
                //fy = y <= pixLasty ? 1 : 0;
                vsFunc.moveKnot(nknots-1, 1, fy != 0 ? 0 : 1);
            } else {            // if (knot == nknots-1)
                // Keep last knot to right of first knot
                x = pixendx = x <= pixstx ? pixstx + 1 : x;
                fx = 1;
                fy = ((Point2D)knotlist.get(nknots-1)).getY();
                if (y > pixFirsty) {
                    fy = 0;
                    pixsty = pixFirsty;
                    pixLasty = pixendy = y;
                } else if (y < pixFirsty) {
                    fy = 1;
                    pixendy = pixFirsty;
                    pixLasty = pixsty = y;
                }
                //fy = y >= pixFirsty ? 0 : 1;
                vsFunc.moveKnot(0, 0, fy != 0 ? 0 : 1);
            }
            vsFunc.moveKnot(knot, fx, fy);
            updateRect();
            vsFunc.setBottomColorToUse(1 - yrect1);
            vsFunc.setTopColorToUse(1 - yrect0);
            //updateScales();
        } else if (knot > 0 && knot < nknots-1) {
            // Convert to normalized coords (0 < val < 1)
            updateScales();
            fx = (x - xoffset) / xscale;
            fy = (y - yoffset) / yscale;

            // Apply constraints to adjustment of knots
            if (fy < 0) {
                fy = 0;             // Don't stray off the bottom
            } else if (fy > 1) {
                fy = 1;             // ... or top
            }
            // Knots cannot move past neighbors in x direction
            double x0 = ((Point2D)knotlist.get(knot-1)).getX(); // Knot to left
            double x1 = ((Point2D)knotlist.get(knot+1)).getX(); // ... and right
            if (fx <= x0) {
                fx = x0 + 1.0 / xscale; // One pixel to the right
                if (fx > (x0 + x1) / 2) {
                    fx = (x0 + x1) / 2;
                }
            } else if (fx >= x1) {
                fx = x1 - 1.0 / xscale; // One pixel to the left
                if (fx < (x0 + x1) / 2) {
                    fx = (x0 + x1) / 2;
                }
            }
            vsFunc.moveKnot(knot, fx, fy);
        }
        repaint();
    }

    private void setVsRange(String str) {
        StringTokenizer strtok = new StringTokenizer(str, " ,");
        double mindat = vsFunc.getMinData();
        double maxdat = vsFunc.getMaxData();
        double val1 = mindat;
        double val2 = maxdat;
        try {
            if (strtok.hasMoreTokens()) {
                val1 = Double.parseDouble(strtok.nextToken());
                if (strtok.hasMoreTokens()) {
                    val2 = Double.parseDouble(strtok.nextToken());
                }
            }
            /*if (mindat != val1 || maxdat != val2)*/ {
                setVsRange(val1, val2);
            }
        } catch (NumberFormatException nfe) {}
    }

    private void setVsRange(double val1, double val2) {
        vsFunc.setMinData(val1);
        vsFunc.setMaxData(val2);
        updateRectFromDataRange(val1, val2);
        repaint();
    }        

    private void setVsFunction(String func) {
        int selectedKnotNumber = vsFunc.getSelectedKnot();
        VsFunction vsf = VsFunction.getFunction(func, cminfo);
        if (vsf != null) {
            vsFunc = vsf;
            vsFunc.setSelectedKnot(selectedKnotNumber);
        }
        double val1 = vsFunc.getMinData();
        double val2 = vsFunc.getMaxData();
        StringTokenizer toker = new StringTokenizer(func);
        while (toker.hasMoreTokens()) {
            String key = toker.nextToken();
            if (key.equals("dmin")) {
                val1 = getTokenDouble(toker, val1);
            } else if (key.equals("dmax")) {
                val2 = getTokenDouble(toker, val2);
            }
        }
        yrect1 = 1 - vsFunc.bottomFrac;
        yrect0 = 1 - vsFunc.topFrac;
        updateScales();
        setVsRange(val1, val2);
    }

    private double getTokenDouble(StringTokenizer toker, double val) {
        if (toker.hasMoreTokens()) {
            try {
                val = Double.parseDouble(toker.nextToken());
            } catch (NumberFormatException nfe) {}
        }
        return val;
    }

    private void sendVsInfo() {
        // Set min and max data limits for curve
        knotlist = vsFunc.getControlPoints();
        nknots = knotlist.size();
        double[] xRange = m_plot.getXRange();
        double xlen = xRange[1] - xRange[0];
        double minDat = xRange[0] + xrect0 * xlen;
        double maxDat = xRange[0]  + xrect1 * xlen;
        vsFunc.setMinData(minDat);
        vsFunc.setMaxData(maxDat);
        if (((Point2D)knotlist.get(0)).getY() == 0) {
            vsFunc.setUflowColor(1 - yrect1);
            vsFunc.setOflowColor(1 - yrect0);
        } else {
            vsFunc.setUflowColor(1 - yrect0);
            vsFunc.setOflowColor(1 - yrect1);
        }

        // Notify client of new settings
        try {
            if (cltFile != null) {
                FileWriter out = new FileWriter(cltFile);
                out.write(vsFunc.getLookupTable().toString());
                out.close();
            }
        } catch(IOException ioe) {}
        sendVnmrCmd();	// Send command to VBG
    }        

    private void updateScales() {
        knotlist = vsFunc.getControlPoints();
        nknots = knotlist.size();
        canXRange = m_plot.getXPixelRange();
        canYRange = m_plot.getYCanvasRange();
        //canWidth1 = xrange[1] - xrange[0];
        //canHeight1 = yrange[1] - yrange[0];
        int canWd = canXRange[1] - canXRange[0];
        int canHt = canYRange[1] - canYRange[0];
        xscale = canWd * (xrect1 - xrect0);
        yscale = -canHt * (yrect1 - yrect0);
        if (xscale == 0.0)
            xscale = 1.0;
        if (yscale == 0.0)
            yscale = 1.0;
        xscale2 = xscale * xscale;
        yscale2 = yscale * yscale;
        xoffset = canXRange[0] + xrect0 * canWd;
        yoffset = canYRange[0] + yrect1 * canHt; // Bottom of canvas
        pixstx = (int)(xoffset + 0.5);
        pixendx = canXRange[0] + (int)(xrect1 * canWd + 0.5);
        pixsty = canYRange[0] + (int)(yrect0 * canHt + 0.5);
        pixendy = (int)(yoffset + 0.5);
        if (nknots >= 2) {
            pixFirsty = ((Point2D)knotlist.get(0)).getY() != 0
                ? pixsty : pixendy;
            pixLasty = ((Point2D)knotlist.get(nknots-1)).getY() != 0
                ? pixsty : pixendy;
        } else {
            pixFirsty = canYRange[1];
            pixLasty = canYRange[0];
        }
    }

    private void updateRectFromDataRange(double dmin, double dmax) {
        double[] plotRange = m_plot.getXRange();
        //double[] curveRange = new double[2];
        //curveRange[0] = vsFunc.getMinData();
        //curveRange[1] = vsFunc.getMaxData();
        if (dmin < plotRange[0] || dmax > plotRange[1]) {
            m_plot.setXRange(Math.min(dmin, plotRange[0]),
                             Math.max(dmax, plotRange[1]));
            plotRange = m_plot.getXRange();
        }
        xrect0 = (dmin - plotRange[0]) / (plotRange[1] - plotRange[0]);
        xrect1 = (dmax - plotRange[0]) / (plotRange[1] - plotRange[0]);
    }

    private void updateRect() {
        canXRange = m_plot.getXPixelRange();
        canYRange = m_plot.getYCanvasRange();
        int canWd = canXRange[1] - canXRange[0];
        int canHt = canYRange[1] - canYRange[0];
        xrect0 = (double)(pixstx - canXRange[0]) / canWd;
        xrect1 = (double)(pixendx - canXRange[0]) / canWd;
        yrect0 = (double)(pixsty - canYRange[0])/ canHt;
        yrect1 = (double)(pixendy - canYRange[0]) / canHt;
    }

    private void drawKnot(Graphics2D g2, int knot, Color color) {
        Point2D.Double pt = (Point2D.Double)knotlist.get(knot);
        int x = (int)(xoffset + pt.getX() * xscale + 0.5);
        int y = (int)(yoffset + pt.getY() * yscale + 0.5);
        g2.setColor(color);
        g2.fillOval(x-3, y-3, 7, 7);
    }

    class VsControlPlot extends VPlot.CursedPlot {
        protected MouseMotionListener ptMouseMotionListener = null;

        public VsControlPlot() {
            super(false);      // Don't install base mouse listeners
            MouseInputListener mouseListener = new MouseInputListener();
            addMouseMotionListener(mouseListener);
            addMouseListener(mouseListener);
        }

        protected void _drawCanvas(Graphics g, int x, int y, int wd, int ht) {
            int nc = 64;
            int h = 1 + (ht - 1) / nc;
            for (int i=0; i<nc; ++i) {
                int yy = y + (i * (ht - 1)) / nc;
                //g.setColor((Color)grayRamp.get(nc - i - 1));
                int j = 255 - (i * 256 / nc);
                g.setColor(new Color(j, j, j));
                g.fillRect(x, yy, wd, h);
            }

            int[] xrange = m_plot.getXPixelRange();
            int[] yrange = m_plot.getYPixelRange();
        }

        public void paintComponent(Graphics g) {
            m_bLeftCursorOn = m_bRightCursorOn = false;
            super.paintComponent(g);  //paint background
            //updateGraph((Graphics2D)g);
        }

        protected synchronized void _drawPlot(Graphics graphics,
                                              boolean clearfirst,
                                              Rectangle drawRectangle) {
            super._drawPlot(graphics, clearfirst, drawRectangle);
            updateGraph((Graphics2D)graphics);
        }
            
       public void setCursor(int nCursor, String strValue) {
        }

        public void setCursor(int nCursor, double value) {
        }

        class MouseInputListener extends VPlot.CursedPlot.MouseInputListener {

            private boolean otherCursorChanged;

            public void mouseMoved(MouseEvent e) {
                mouseIn = true;
                updateSelection(e.getX(), e.getY());
            }

            public void mouseDragged(MouseEvent e){
                int knot;
                if ((knot=vsFunc.getSelectedKnot()) < 0) {
                    return;
                }
                moveKnot(knot, e.getX(), e.getY());
                //sendVsInfo();
            }

            public void mouseReleased(MouseEvent e){
                if (vsFunc.getSelectedKnot() < 0) {
                    return;
                }
                sendVsInfo();
                mouseDown = false;
                if (!mouseIn) {
                    vsFunc.setSelectedKnot(-1);
                    repaint();
                } else {
                    updateSelection(e.getX(), e.getY());
                }
            }

            public void mousePressed(MouseEvent e) {
                mouseDown = true;
            }

            public void mouseClicked(MouseEvent e){}

            public void mouseExited(MouseEvent e) {
                mouseIn = false;
                if (!mouseDown && vsFunc.getSelectedKnot() >= 0) {
                    vsFunc.setSelectedKnot(-1);
                    repaint();
                }
            }

            public void mouseEntered(MouseEvent e) {
                mouseIn = true;
            }

            protected void mousePosition(MouseEvent e, int nMouseAction) {
                Point2D.Double point;
                if (SwingUtilities.isLeftMouseButton(e)) {
                    movePointToMouse(e, leftPoint);
                    m_strLCValue = Double.toString(leftPoint.x);
                } else {
                    return;
                }
                repaint();
            }

            /**
             * Send the appropriate command to Vnmr for a mouse event.
             * Changing one cursor may cause a command to be sent for
             * the other cursor also.  The logic is rather ad hoc; it
             * is designed for VnmrCmds that follow certain rules.
             * The MOUSE_PRESS command should activate the cursor if
             * it is not already active, The MOUSE_RELEASE command
             * should be equivalent to MOUSE_PRESS except for not
             * changing the active status.  If MOUSE_DRAG is defined,
             * it should also not change the active status.  The
             * "other" cursor never gets sent a MOUSE_PRESS, so its
             * active status should not change, but it should change
             * position to be consistent with the first cursor.
             */
            public void sendCursorCmds(MouseEvent e, int nMouseAction) {
                if (SwingUtilities.isLeftMouseButton(e)) {
                    m_objMin.sendVnmrCmd(nMouseAction);
                }
            }
                    
        }
    }

    class RangeHandler extends VObjAdapter {

        public void updateValue() {
	    if (vnmrIf == null) {
		return;
            }
            if (rangeValExpr != null) {
	        vnmrIf.asyncQueryParam(this, rangeValExpr);
            }
        }

        public void setValue(ParamIF pf) {
	    if (pf != null)
	    {
                setVsFunction(pf.value);
	    }
	}
    }

    class BkgHandler extends VObjAdapter /*implements VGraphHelperIF*/ {

        /*
        public void updateGraph(Graphics2D g2) {
            System.out.println("VVsControl.BkgHandler.updateGraph()");
            updateScales();
            int nc = grayRamp.size();
            if (nc <= 0) { nc = 1; }
            int h = 1 + canHeight1 / nc;
            for (int i=0; i<nc; ++i) {
                int y = (i * canHeight1) / nc;
                g2.setColor((Color)grayRamp.get(nc - i - 1));
                g2.fillRect(0, y, canWidth1+1, h);
            }
        }
        */

        public void updateValue() {
	    if (vnmrIf == null) {
		return;
            }
            if (bkgValExpr != null) {
	        vnmrIf.asyncQueryParam(this, bkgValExpr);
            }
        }

        public void setValue(ParamIF pf) {
	    if (pf != null)
	    {
		grayRampFile = pf.value;
                setGrayLevels(grayRampFile);
		repaintPanels();
	    }
	}
    }

    class CltHandler extends VObjAdapter {

        public void updateValue() {
	    if (vnmrIf == null) {
		return;
            }
            if (cltFileExpr != null) {
	        vnmrIf.asyncQueryParam(this, cltFileExpr);
            }
        }

        public void setValue(ParamIF pf) {
	    if (pf != null)
	    {
		cltFile = pf.value;
	    }
	}
    }
}
