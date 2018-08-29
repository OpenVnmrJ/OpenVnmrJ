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
 * Title:        VPlot
 * Description:
 */

public class VPlot extends VObj
    implements VEditIF, PropertyChangeListener, VStatusChartIF
{
    /** String that corresponds to the choice of fill/unfill histogram. */
    protected String m_strFillHistgm;

    /** String that corresponds to the choice of showing the X-axis. */
    protected String m_xaxisshow;

    /** String that corresponds to the choice of showing the Y-axis. */
    protected String m_yaxisshow;

    /** If this is "Yes" plot the X values on a log scale. */
    protected String m_logXaxis;

    /** If this is "Yes" plot the Y values on a log scale. */
    protected String m_logYaxis;

    /** If this is "Yes" put a grid on the canvas. */
    protected String m_showGrid;

    /** String that corresponds for joining points for scatter plot */
    protected String m_strJoinPts;

    /** Size for the points drawn for the scatter plot. */
    protected String m_strPtSize;

    /** Set Value param for the left cursor of the histogram. */
    protected String m_strLCSetVal;

    /** Press Button command for the left cursor of the histogram.  */
    protected String m_strLCCmd;

    /** Release button command for the left cursor of the histogram. */
    protected String m_strLCCmd2;

    /** Drag button command for the left cursor of the histogram.  */
    protected String m_strLCCmd3;

    /** Set value param for the right cursor of the histogram. */
    protected String m_strRCSetVal;

    /** Press button command for the right cursor of the histogram.  */
    protected String m_strRCCmd;

    /** Release button command for the right cursor of the histogram.  */
    protected String m_strRCCmd2;

    /** Drag button command for the right cursor of the hisogram.  */
    protected String m_strRCCmd3;

    protected boolean m_bLeftCursorOn = false;
    protected boolean m_bRightCursorOn = false;
    protected boolean m_bTopCursorOn = false;
    protected boolean m_bBotCursorOn = false;
    protected String m_strLCValue = "0";
    protected String m_strRCValue = "0";
    protected String m_strLCShow;
    protected String m_strRCShow;
    protected String m_strLCColor;
    protected String m_strRCColor;
    protected String m_strGridColor;
    protected String m_strTickColor;
    protected Color m_lcColor = Color.red;
    protected Color m_rcColor = Color.red;
    protected Color m_fgColor = Color.yellow;
    protected Color m_fgColor2 = Color.green;
    protected Color m_fgColor3 = Color.blue;
    protected Color m_fgColor4 = Color.black;
    protected Color m_fgColor5 = Color.black;
    protected Color m_fgColor6 = Color.black;
    protected Color m_gridColor = Color.gray;
    protected Color m_tickColor = Color.yellow;

    protected String m_strBGCol;

    protected String m_strFGCol;
    protected String m_strFGCol2;
    protected String m_strFGCol3;
    protected String m_strFGCol4;
    protected String m_strFGCol5;
    protected String m_strFGCol6;

    protected boolean vsDifferent = false;
    protected String plotFile = null; // File plot data is in
    protected String plotFileId = "";
    protected long plotFileDate = 0; // When file last modified
    protected boolean plotFileModified = false;
    protected boolean offsetDragEnabled = true;
    protected boolean showZeroY = false;

    /** String Array that contains option for fill/unfill histogram,
        and join lines for scatter plot. */
    protected static final String[] m_arrStrYesNo = {"Yes", "No"};

    /** String Array that contains menu for the size of the points
        for the scatter plot. */
    protected static final String[] m_arrStrPtSz = { "None", "Small", "Large"};

    protected Max m_objMax = null;

    protected Min m_objMin = null;

    protected VPlot m_graph;

    protected CursedPlot m_plot;
    //protected Overlay m_panel;

    protected String m_orientationExpr = null;
    protected String m_orientation = null;
    protected Orientation m_objOrientation = new Orientation();

    /** Constant Defintions */
    protected static final int NUMELMS      = 2;
    protected static final int FIRSTELEM    = 0;
    protected static final int SECONDELEM   = 1;

    protected static final int MOUSE_PRESS = 0;
    protected static final int MOUSE_DRAG = 1;
    protected static final int MOUSE_RELEASE = 2;

    protected static final int LEFT_CURSOR = 0;
    protected static final int RIGHT_CURSOR = 1;


    /**
     * Dummy constructor for test programs.
     */
    public VPlot() {
    }

    /**
     * Normal constructor.
     * @param sshare    The session share object.
     * @param vif       The button callback interface.
     * @param typ The type of the object.
     */
    public VPlot(SessionShare sshare, ButtonIF vif, String typ)
    {
        super(sshare, vif, typ);

        setBackground(Util.getBgColor());
        m_plot = new CursedPlot();
        m_plot.setFillButton(true);
        m_plot.setOpaque(true);
        m_plot.setBackground(null);
        add(m_plot);
        m_graph = this;
        m_objMax = new Max();
        m_objMin = new Min();
        m_xaxisshow = "Yes";
        m_yaxisshow = "Yes";
        m_logXaxis = "No";
        m_logYaxis = "No";
        m_showGrid = "Yes";
        setLayout(new MyLayout());
        initialize();
    }

    public CursedPlot getPlot() {
        return m_plot;
    }

    public void propertyChange(PropertyChangeEvent evt) {
        super.propertyChange(evt);
        setColors();
        setPlotOptions();
    }

    public void setColors() {
        setBackground();
        setCanvasColor();
        setDataColors();
        setLeftCursorColor();
        setRightCursorColor();
        setGridColor();
        setTickColor();
    }

    public void setBackground() {
        String attr = getAttribute(BGCOLOR);
        String colorName = DisplayOptions.getOption(DisplayOptions.COLOR, attr);
        if (colorName != null) {
            Color color = DisplayOptions.getColor(colorName);
            m_plot.setBackground(color);
            setBackground(color);
        }
    }

    public void setCanvasColor() {
        String strColor = getAttribute(GRAPHBGCOL);
        if (strColor != null) {
            m_plot.setPlotBackground(DisplayOptions.getColor(strColor));
        }
    }

    public void setDataColors() {
        String strColor = getAttribute(GRAPHFGCOL);
        if (strColor != null) {
            m_strFGCol = strColor;
            m_fgColor = DisplayOptions.getColor(strColor);
            m_plot.setMarkColor(m_fgColor, 0);
        }
        strColor = getAttribute(GRAPHFGCOL2);
        if (strColor != null) {
            m_strFGCol2 = strColor;
            m_fgColor2 = DisplayOptions.getColor(strColor);
            m_plot.setMarkColor(m_fgColor2, 1);
        }
        strColor = getAttribute(GRAPHFGCOL3);
        if (strColor != null) {
            m_strFGCol3 = strColor;
            m_fgColor3 = DisplayOptions.getColor(strColor);
            m_plot.setMarkColor(m_fgColor3, 2);
        }
        strColor = getAttribute(COLOR4);
        if (strColor != null) {
            m_strFGCol4 = strColor;
            m_fgColor4 = DisplayOptions.getColor(strColor);
            m_plot.setMarkColor(m_fgColor4, 3);
        }
        strColor = getAttribute(COLOR5);
        if (strColor != null) {
            m_strFGCol5 = strColor;
            m_fgColor5 = DisplayOptions.getColor(strColor);
            m_plot.setMarkColor(m_fgColor5, 4);
        }
        strColor = getAttribute(COLOR6);
        if (strColor != null) {
            m_strFGCol6 = strColor;
            m_fgColor6 = DisplayOptions.getColor(strColor);
            m_plot.setMarkColor(m_fgColor6, 5);
        }
    }

    public void setLeftCursorColor() {
        String strColor = getAttribute(LCCOLOR);
        if (strColor != null) {
            m_lcColor = DisplayOptions.getColor(strColor);
        }
    }

    public void setRightCursorColor() {
        String strColor = getAttribute(VObjDef.RCCOLOR);
        if (strColor != null) {
            m_rcColor = DisplayOptions.getColor(strColor);
        }
    }

    public void setGridColor() {
        String strColor = getAttribute(GRIDCOLOR);
        if (strColor != null) {
            m_gridColor = DisplayOptions.getColor(strColor);
            m_plot.setGridColor(m_gridColor);
        }
    }

    public void setTickColor() {
        String strColor = getAttribute(TICKCOLOR);
        if (strColor != null) {
            m_tickColor = DisplayOptions.getColor(strColor);
            m_plot.setTickColor(m_tickColor);
        }
    }

    public void setForeground(Color color) {
        if (m_plot != null) {
            m_plot.setForeground(color);
        }
    }

    public void changeFont() {
        String strFont = getAttribute(VObjDef.FONT_STYLE);
        Font font = DisplayOptions.getFont(strFont);

        if (font != null && m_plot != null) {
            /**/m_plot.setLabelFont(font);
            float size = font.getSize2D();
            /*Messages.postDebug("Title font size=" + size);/*CMP*/
            m_plot.setTitleFont(font.deriveFont(Font.BOLD, size));
            m_plot.repaint();
        }
    }

    /**
     * Initializes the UI components.
     */
    protected void initialize()
    {
    }

    public boolean inEditMode()
    {
        return inEditMode;
    }

    /**
     * Updates the value.
     */
    public void updateValue()
    {
        if (vnmrIf == null)
            return;
        m_objMax.updateValue();
        m_objMin.updateValue();
        m_objOrientation.updateValue();
        if (showExpr != null)
        {
            vnmrIf.asyncQueryShow(this, showExpr);
        }
        else if (valueExpr != null)
        {
            vnmrIf.asyncQueryParam(this, valueExpr, precision);
        }

        String strFont = getAttribute(VObjDef.FONT_STYLE);
        Font font = DisplayOptions.getFont(strFont);
    }

    /**
     * Dummy method required by VStatusChartIF
     */
    public void setValue(String s){
        Messages.postError("This dummy method should never be called.");
        Messages.writeStackTrace(new Exception("Stack Trace"));
    }

    /**
     * Dummy method required by VStatusChartIF
     */
    public void updateValue(Vector v){
        Messages.postError("This dummy method should never be called.");
        Messages.writeStackTrace(new Exception("Stack Trace"));
    }

    /**
     * Set value of a parameter.
     * @param key The key name of the parameter.
     * @param keyval The value of the parameter.
     * @return Returns true if the value is different from the old value.
     */
    public boolean setValue(String key, String keyval) {
        boolean change = false;
        if (key.equals("file")) {
            change = !keyval.equals(plotFile);
            plotFile = keyval;
            if (!change) {
                long mod = (new File(keyval)).lastModified();
                change = (mod != plotFileDate);
                plotFileDate = mod;
            }
            if (change) {
                plotFileModified = true;
            } else {
                Messages.postDebug("VPlot",
                                   "VPlot.setValue(): Plot file not modified");
            }
        } else if (key.equals("range")) {
            StringTokenizer toker = new StringTokenizer(keyval, ",");
            if (toker.countTokens() >= 2) {
                try {
                    double minx = Double.parseDouble(toker.nextToken());
                    double maxx = Double.parseDouble(toker.nextToken());
                    m_plot.setXRange(minx, maxx);
                } catch  (NumberFormatException nfe) {}
            }
        }
        return change;
    }

    /**
     * Sets the value of the parameter.
     */
    public void setValue(ParamIF pf)
    {
        if (pf != null && pf.value != null) {
            StringTokenizer toker = new StringTokenizer(pf.value);
            while (toker.countTokens() >= 2) {
                String key = toker.nextToken();
                String keyval = toker.nextToken();
                setValue(key, keyval);
            }
            if (toker.hasMoreTokens()) {
                plotFile = toker.nextToken();
            }
            repaintPanels();
        }
    }

    public void sendVnmrCmd()
    {
        if (vnmrIf == null)
            return;
        if (vnmrCmd != null)
        {
            vnmrIf.sendVnmrCmd(this, vnmrCmd);
        }
    }

    /**
     *  Sets the value for various attributes.
     *  @param attr attribute to be set.
     *  @param c    value of the attribute.
     */
    public void setAttribute(int attr, String c) {
        if (c != null)
        {
            c = c.trim();
            if (c.length() == 0) {c = null;}
        }

        switch (attr)
        {
            case ORIENTATION:
                    m_orientationExpr = c;
                    break;
            case JOINPTS:
                    m_strJoinPts = c;
                    break;
            case FILLHISTGM:
                    m_strFillHistgm = c;
                    break;
            case XAXISSHOW:
                    m_xaxisshow = c;
                    break;
            case YAXISSHOW:
                    m_yaxisshow = c;
                    break;
                    //case LOGXAXIS:
                    //        m_logXaxis = c;
                    //        break;
            case LOGYAXIS:
                    m_logYaxis = c;
                    break;
            case SHOWGRID:
                    m_showGrid = c;
                    break;
            case POINTSIZE:
                    m_strPtSize = c;
                    break;
            case LCSETVAL:
                    m_strLCSetVal = c;
                    if (c != null && m_objMin != null && m_strLCValue != null) {
                         vnmrIf.asyncQueryParam(m_objMin, m_strLCSetVal);
                    }
                    break;
            case LCSHOW:
                    m_strLCShow = c;
                    break;
            case LCCMD:
                    m_strLCCmd = c;
                    break;
            case LCCMD2:
                    m_strLCCmd2 = c;
                    break;
            case LCCMD3:
                    m_strLCCmd3 = c;
                    break;
            case LCCOLOR:
                    m_strLCColor = c;
                    setLeftCursorColor();
                    break;
            case RCSETVAL:
                    m_strRCSetVal = c;
                    if (c != null && m_objMax != null && m_strRCValue != null) {
                         vnmrIf.asyncQueryParam(m_objMax, m_strRCSetVal);
                    }
                    break;
            case RCSHOW:
                    m_strRCShow = c;
                    if (m_strRCShow != null && m_objMax != null)
                        vnmrIf.asyncQueryShow(m_objMax, m_strRCShow);
                    break;
            case RCCMD:
                    m_strRCCmd = c;
                    break;
            case RCCMD2:
                    m_strRCCmd2 = c;
                    break;
            case RCCMD3:
                    m_strRCCmd3 = c;
                    break;
            case LCVAL:
                    m_strLCValue = c;
                    if (c != null && m_objMin != null && m_strLCSetVal != null)
                    {
                        vnmrIf.asyncQueryParam(m_objMin, m_strLCSetVal);
                    }
                    break;
            case RCVAL:
                    m_strRCValue = c;
                    if (c != null && m_objMax != null && m_strRCSetVal != null)
                    {
                        vnmrIf.asyncQueryParam(m_objMax,m_strRCSetVal);
                    }
                    break;
            case RCCOLOR:
                    m_strRCColor = c;
                    setRightCursorColor();
                    break;
            case GRAPHBGCOL:
                    m_strBGCol = c;
                    setCanvasColor();
                    break;
            case GRAPHFGCOL:
                    m_strFGCol = c;
                    if (c != null) {
                        m_fgColor = DisplayOptions.getColor(c);
                        m_plot.setMarkColor(m_fgColor, 0);
                    }
                    break;
            case GRAPHFGCOL2:
                    m_strFGCol2 = c;
                    if (c != null) {
                        m_fgColor2 = DisplayOptions.getColor(c);
                        m_plot.setMarkColor(m_fgColor2, 1);
                    }
                    break;
            case GRAPHFGCOL3:
                    m_strFGCol3 = c;
                    if (c != null) {
                        m_fgColor3 = DisplayOptions.getColor(c);
                        m_plot.setMarkColor(m_fgColor3, 2);
                    }
                    break;
            case COLOR4:
                    m_strFGCol4 = c;
                    if (c != null) {
                        m_fgColor4 = DisplayOptions.getColor(c);
                        m_plot.setMarkColor(m_fgColor4, 3);
                    }
                    break;
            case COLOR5:
                    m_strFGCol5 = c;
                    if (c != null) {
                        m_fgColor5 = DisplayOptions.getColor(c);
                        m_plot.setMarkColor(m_fgColor5, 4);
                    }
                    break;
            case COLOR6:
                    m_strFGCol6 = c;
                    if (c != null) {
                        m_fgColor6 = DisplayOptions.getColor(c);
                        m_plot.setMarkColor(m_fgColor6, 5);
                    }
                    break;
            case GRIDCOLOR:
                    m_strGridColor = c;
                    setGridColor();
                    break;
            case TICKCOLOR:
                    m_strTickColor = c;
                    setTickColor();
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
    public String getAttribute(int attr)
    {
        String strAttr;
        switch (attr)
        {
            case ORIENTATION:
                    strAttr = m_orientationExpr;
                    break;
            case JOINPTS:
                    strAttr = m_strJoinPts;
                    break;
            case FILLHISTGM:
                    strAttr = m_strFillHistgm;
                    break;
            case XAXISSHOW:
                    strAttr = m_xaxisshow;
                    break;
            case YAXISSHOW:
                    strAttr = m_yaxisshow;
                    break;
                    //case LOGXAXIS:
                    //    strAttr = m_logXaxis;
                    //    break;
            case LOGYAXIS:
                    strAttr = m_logYaxis;
                    break;
            case SHOWGRID:
                    strAttr = m_showGrid;
                    break;
            case POINTSIZE:
                    strAttr = m_strPtSize;
                    break;
            case LCSETVAL:
                    strAttr = m_strLCSetVal;
                    break;
            case LCSHOW:
                    strAttr = m_strLCShow;
                    break;
            case LCCMD:
                    strAttr = m_strLCCmd;
                    break;
            case LCCMD2:
                    strAttr = m_strLCCmd2;
                    break;
            case LCCMD3:
                    strAttr = m_strLCCmd3;
                    break;
            case LCCOLOR:
                    strAttr = m_strLCColor;
                    break;
            case RCSETVAL:
                    strAttr = m_strRCSetVal;
                    break;
            case RCSHOW:
                    strAttr = m_strRCShow;
                    break;
            case RCCMD:
                    strAttr = m_strRCCmd;
                    break;
            case RCCMD2:
                    strAttr = m_strRCCmd2;
                    break;
            case RCCMD3:
                    strAttr = m_strRCCmd3;
                    break;
            case LCVAL:
                    /*
                    if (m_strLCValue == null && m_strLCSetVal != null
                        && m_objMin != null && vnmrIf != null)
                    {
                        vnmrIf.asyncQueryParam(m_objMin, m_strLCSetVal);
                    }
                    */
                    strAttr = m_strLCValue;
                    break;
            case RCVAL:
                    /*
                    if (m_strRCValue == null && m_strRCSetVal != null
                        && m_objMax != null && vnmrIf != null)
                    {
                        vnmrIf.asyncQueryParam(m_objMax, m_strRCSetVal);
                    }
                    */
                    strAttr = m_strRCValue;
                    break;
            case RCCOLOR:
                    strAttr = m_strRCColor;
                    break;
            case GRAPHBGCOL:
                    strAttr = m_strBGCol;
                    break;
            case GRAPHFGCOL:
                    strAttr = m_strFGCol;
                    break;
            case GRAPHFGCOL2:
                    strAttr = m_strFGCol2;
                    break;
            case GRAPHFGCOL3:
                    strAttr = m_strFGCol3;
                    break;
            case COLOR4:
                    strAttr = m_strFGCol4;
                    break;
            case COLOR5:
                    strAttr = m_strFGCol5;
                    break;
            case COLOR6:
                    strAttr = m_strFGCol6;
                    break;
            case GRIDCOLOR:
                    strAttr = m_strGridColor;
                    break;
            case TICKCOLOR:
                    strAttr = m_strTickColor;
                    break;
            default:
                    strAttr = super.getAttribute(attr);
                    break;
        }
        return strAttr;
    }

    public String getOrientation() {
        return m_orientation;
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
        {new Integer(VARIABLE),     "Vnmr variables:"},
        {new Integer(SETVAL),       "Value of item:"},
        {new Integer(SHOW),         "Enable condition:"},
        {new Integer(CMD),          "Vnmr command:"},
        {new Integer(GRAPHBGCOL),   "Graph Background Color:", "color"},
        {new Integer(GRAPHFGCOL),   "First Mark Color:", "color"},
        {new Integer(GRAPHFGCOL2),  "Second Mark Color:", "color"},
        {new Integer(GRAPHFGCOL3),  "Third Mark Color:", "color"},
        {new Integer(POINTSIZE),    "Point Size for Scatter Plot:", "menu",
         m_arrStrPtSz},
        {new Integer(JOINPTS),      "Connect Points in ScatterPlot:", "radio",
         m_arrStrYesNo},
        //{new Integer(FILLHISTGM),   "Fill Histogram:", "radio", m_arrStrYesNo},
        {new Integer(LOGYAXIS),     "Log Y-axis:", "radio", m_arrStrYesNo},
        {new Integer(XAXISSHOW),    "Show X-axis:", "radio", m_arrStrYesNo},
        {new Integer(XLABELSHOW),   "Show X-axis label:", "radio",
         m_arrStrYesNo},
        {new Integer(YAXISSHOW),    "Show Y-axis:", "radio", m_arrStrYesNo},
        {new Integer(YLABELSHOW),   "Show Y-axis label:", "radio",
         m_arrStrYesNo},
        {new Integer(TICKCOLOR),    "Tick color:", "color"},
        {new Integer(SHOWGRID),     "Show grid:", "radio", m_arrStrYesNo},
        {new Integer(GRIDCOLOR),    "Grid color:", "color"},
        {new Integer(LCSETVAL),     "Value of item for left cursor:"},
        {new Integer(LCSHOW),       "Enable left cursor condition:"},
        {new Integer(LCCMD),        "Vnmr cmd for left cursor button release:"},
        {new Integer(LCCMD2),       "Vnmr cmd for left cursor button press:"},
        {new Integer(LCCMD3),       "Vnmr cmd for left cursor button drag:"},
        {new Integer(LCCOLOR),      "Color for left cursor:", "color"},
        {new Integer(RCSETVAL),     "Value of item for right cursor:"},
        {new Integer(RCSHOW),       "Enable right cursor condition:"},
        {new Integer(RCCMD),       "Vnmr cmd for right cursor button release:"},
        {new Integer(RCCMD2),       "Vnmr cmd for right cursor button press:"},
        {new Integer(RCCMD3),       "Vnmr cmd for right cursor button drag:"},
        {new Integer(RCCOLOR),      "Color for right cursor:", "color"}

    };

    /**
     *  Whether we want to show any axis
     */
    public boolean getShowAxis() {
        return ((m_xaxisshow != null && m_xaxisshow.equalsIgnoreCase("Yes")) ||
                (m_yaxisshow != null && m_yaxisshow.equalsIgnoreCase("Yes")));
    }

    /**
     *  Whether we want to show the X-axis
     */
    public boolean getShowXAxis() {
        return m_xaxisshow != null && m_xaxisshow.equalsIgnoreCase("Yes");
    }

    /**
     *  Whether we want to show the Y-axis
     */
    public boolean getShowYAxis() {
        return m_yaxisshow != null && m_yaxisshow.equalsIgnoreCase("Yes");
    }

    /**
     * Whether we want to show the Y-data on a log scale
     */
    public boolean getLogYAxis() {
        return m_logYaxis != null && m_logYaxis.equalsIgnoreCase("Yes");
    }

    /**
     * Whether we want to show a grid on the canvas
     */
    public boolean getShowGrid() {
        return m_showGrid != null && m_showGrid.equalsIgnoreCase("Yes");
    }

    /**
     *  Sets the foreground color of the graph including the canvas,
     *  the axes, and the labels.
     *  @param g2   the graphics2D object whose color is being set.
     */
    /*
    public void setFGColor(Graphics2D g2)
    {
        String strColor = (getAttribute(GRAPHFGCOL) == null) ? "yellow"
                                : getAttribute(GRAPHFGCOL);
        g2.setColor(DisplayOptions.getColor(strColor));
    }
    */

    /**
     * Repaints the panels.
     */
    protected void repaintPanels() {
        plotFileModified = false;
        if (plotFile == null) {
            return;             // No data to display
        }

        FileReader fr;
        try {
            fr = new FileReader(plotFile);
        } catch (IOException ioe) {
            return;
        }
        BufferedReader in = new BufferedReader(fr);

        String strLine = "";
        boolean headerFlag = true;
        boolean histogramFlag = false;
        boolean doingHistogramData = false;
        boolean doingScatterplotData = false;
        //int nbins = 0;
        int npts = 0;
        int npExpect = 0;
        double xMin = 0;
        double xMax = 0;
        double yMin = 0;
        double yMax = 0;
        boolean xMinFlag = false;
        boolean xMaxFlag = false;
        boolean yMinFlag = false;
        boolean yMaxFlag = false;
        double xLeft = 0;
        double barWidth = 0;
        double[] xv = null;
        double[] yv = null;
        boolean skipPlot = false;

        showZeroY = false; // Default

        // Parse the file
        while (true) {
            try {
                strLine = in.readLine();
            } catch (IOException ioe) {
                ioe.printStackTrace();
                break;
            }
            if (strLine ==  null) {
                break;
            }
            if (headerFlag) {
                // Reading header stuff
                StringTokenizer strTok = new StringTokenizer(strLine);
                String id = "";
                String val = "";
                if (strTok.hasMoreTokens()) {
                    id = strTok.nextToken();
                }
                if (strTok.hasMoreTokens()) {
                    // Grab remainder of line
                    val = strTok.nextToken("").trim();
                }
                if (id.equalsIgnoreCase("Histogram")) {
                    histogramFlag = true;
                    try {
                        npExpect = Integer.parseInt(val);
                    } catch (NumberFormatException nfe) {}
                } else if (id.equalsIgnoreCase("ID")) {
                    if (plotFileId.equals(val)) {
                        skipPlot = true;
                        break;  // We've already plotted this file
                    }
                    plotFileId = val;
                } else if (id.equalsIgnoreCase("XLabel")) {
                    m_plot.setXLabel(val);
                } else if (id.equalsIgnoreCase("YLabel")) {
                    m_plot.setYLabel(val);
                } else if (id.equalsIgnoreCase("NPoints")) {
                    try {
                        npExpect = Integer.parseInt(val);
                        xv = new double[npExpect];
                        yv = new double[npExpect];
                    } catch (NumberFormatException nfe) {}
                } else if (id.equalsIgnoreCase("XMin")) {
                    try {
                        xMinFlag = true;
                        xMin = Double.parseDouble(val);
                    } catch (NumberFormatException nfe) {}
                } else if (id.equalsIgnoreCase("XMax")) {
                    try {
                        xMaxFlag = true;
                        xMax = Double.parseDouble(val);
                    } catch (NumberFormatException nfe) {}
                } else if (id.equalsIgnoreCase("YMin")) {
                    try {
                        yMinFlag = true;
                        yMin = Double.parseDouble(val);
                    } catch (NumberFormatException nfe) {}
                } else if (id.equalsIgnoreCase("YMax")) {
                    try {
                        yMaxFlag = true;
                        yMax = Double.parseDouble(val);
                    } catch (NumberFormatException nfe) {}
                } else if (id.equalsIgnoreCase("ShowZeroY")) {
                    showZeroY = true;
                } else if (id.equalsIgnoreCase("Data")) {
                    if (npExpect < 0) {
                        Messages.postDebug("Negative number of points in "
                                           + "VPlot.repaintPanels: "
                                           + npExpect);
                        break;
                    }
                    if (xMinFlag && xMaxFlag) {
                        m_plot.setXRange(xMin, xMax);
                    }
                    if (yMinFlag && yMaxFlag) {
                        m_plot.setYRange(yMin, yMax);
                    }
                    if (histogramFlag) {
                        barWidth = (xMax - xMin) / npExpect;
                        xLeft = xMin + barWidth / 2;
                    }
                    headerFlag = false;
                }
            } else if (histogramFlag) {
                // Doing histogram data
                if (xv == null || yv == null) {
                    xv = new double[npExpect];
                    yv = new double[npExpect];
                }
                if (npts >= npExpect) {
                    break;  // Prevent overrunning our data arrays
                }
                xv[npts] = xLeft + npts * barWidth;
                try {
                    yv[npts] = Double.parseDouble(strLine);
                    ++npts;
                } catch (NumberFormatException nfe) {}
            } else {
                // Doing scatterplot data
                if (xv == null || yv == null) {
                    xv = new double[npExpect];
                    yv = new double[npExpect];
                }
                if (npts >= npExpect) {
                    break;  // Prevent overrunning our data arrays
                }
                StringTokenizer strTok = new StringTokenizer(strLine);
                double x = 0;
                double y = 0;
                try {
                    if (strTok.hasMoreTokens()) {
                        xv[npts] = Double.parseDouble(strTok.nextToken());
                    }
                    if (strTok.hasMoreTokens()) {
                        yv[npts] = Double.parseDouble(strTok.nextToken());
                    }
                    ++npts;
                } catch (NumberFormatException nfe) {}
            }
        }
        try {
            in.close();
        } catch (IOException ioe) {}

        // Plot the data
        if (npts == npExpect && !skipPlot) {
            // Set common options
            setPlotOptions();   // TODO: just set stuff needed for setPoints()

            if (histogramFlag) {
                // Set histogram options
                m_plot.setBars(barWidth, 0);
                m_plot.setConnected(false);
                m_plot.setMarksStyle("none");
            } else {
                // Set scatterplot options
                m_plot.setBars(false);
                if (m_strJoinPts != null &&
                    m_strJoinPts.equalsIgnoreCase("yes"))
                {
                    m_plot.setConnected(true);
                } else {
                    m_plot.setConnected(false);
                }
                if (m_strPtSize == null) {
                    m_plot.setMarksStyle("points");
                } else {
                    if (m_strPtSize.equalsIgnoreCase("None")) {
                        m_plot.setMarksStyle("none");
                    } else if (m_strPtSize.equalsIgnoreCase("Small")) {
                        m_plot.setMarksStyle("points");
                    } else {
                        m_plot.setMarksStyle("dots");
                    }
                }

            }
            m_plot.setPoints(0, xv, yv, true);
        }
    }

    private boolean isValTrue(float fValue, int nMax)
    {
        return (nMax > 0 ? (fValue < nMax) : (fValue > nMax));
    }

    protected void setPlotOptions() {
        if (getLogYAxis()) {
            m_plot.setYLog(true, -0.3);
        } else {
            m_plot.setYLog(false);
        }
        m_plot.setGrid(getShowGrid());
        m_plot.setShowZeroY(showZeroY);
        m_plot.setXAxis(getShowXAxis());
        m_plot.setYAxis(getShowYAxis());
        m_plot.setMarkColor(m_fgColor, 0);
        m_plot.setMarkColor(m_fgColor2, 1);
        m_plot.setMarkColor(m_fgColor3, 2);
        m_plot.setMarkColor(m_fgColor4, 3);
        m_plot.setMarkColor(m_fgColor5, 4);
        m_plot.setMarkColor(m_fgColor6, 5);
    }

    protected class Orientation extends VObjAdapter {
        /**
         * Updates the value.
         */
        public void updateValue() {
            if (vnmrIf == null) {
                return;
            }
            if (m_orientationExpr != null) {
                vnmrIf.asyncQueryParam(this, m_orientationExpr);
            }
        }

        // Gets "setValue" messages for left cursor
        public void setValue(ParamIF pf) {
            if (pf != null) {
                if (m_orientation != pf.value) {
                    m_orientation = pf.value;
                    m_plot.invalidate();
                    m_plot.validate();
                    repaint();
                }
            }
        }

        public String getAttribute(int nAttr) {
            return m_graph.getAttribute(nAttr);
        }

        public void setAttribute(int nAttr, String strAttr) {
            m_graph.setAttribute(nAttr, strAttr);
        }
    }

    protected class Min extends VObjAdapter {
        /**
         * Updates the value.
         */
        public void updateValue() {
            if (vnmrIf == null) {
                return;
            }
            if (m_strLCShow != null) {
                vnmrIf.asyncQueryShow(this, m_strLCShow);
            }
            if (m_strLCSetVal != null) {
                vnmrIf.asyncQueryParam(this, m_strLCSetVal);
            }
        }

        // Gets "setValue" messages for left cursor
        public void setValue(ParamIF pf) {
            if (pf != null) {
                if (m_strLCValue != pf.value) {
                    m_strLCValue = pf.value;
                    m_plot.moveCursor(LEFT_CURSOR, m_strLCValue);
                }
            }
        }

        public void setShowValue(ParamIF pf) {
            if (pf != null && pf.value != null) {
                String  s = pf.value.trim();
                int nActive = Integer.parseInt(s);
                if (nActive > 0) {
                    m_bLeftCursorOn = true;
                    if (m_strLCSetVal != null) {
                        vnmrIf.asyncQueryParam(this, m_strLCSetVal, precision);
                    }
                } else {
                    m_bLeftCursorOn = false;
                }
                repaint();
            }
        }

        public void sendVnmrCmd(int nMouseAction) {
            if (vnmrIf == null) {
                return;
            }
            if (nMouseAction == MOUSE_PRESS && m_strLCCmd2 != null) {
                 vnmrIf.sendVnmrCmd(this, m_strLCCmd2, LCVAL);
            }
            if (nMouseAction == MOUSE_DRAG && m_strLCCmd3 != null) {
                vnmrIf.sendVnmrCmd(this, m_strLCCmd3, LCVAL);
            }
            if (nMouseAction == MOUSE_RELEASE && m_strLCCmd != null) {
                vnmrIf.sendVnmrCmd(this, m_strLCCmd, LCVAL);
            }
        }

        public String getAttribute(int nAttr) {
            return m_graph.getAttribute(nAttr);
        }

        public void setAttribute(int nAttr, String strAttr) {
            m_graph.setAttribute(nAttr, strAttr);
        }
    }

    protected class Max extends VObjAdapter {
        /**
         * Updates the value.
         */
        public void updateValue() {
            if (vnmrIf == null) {
                return;
            }
            if (m_strRCShow != null) {
                vnmrIf.asyncQueryShow(this, m_strRCShow);
            }
            if (m_strRCSetVal != null) {
                vnmrIf.asyncQueryParam(this, m_strRCSetVal);
            }
        }

        // Gets "setValue" messages for right cursor
        public void setValue(ParamIF pf) {
            if (pf != null) {
                if (m_strRCValue != pf.value) {
                    m_strRCValue = pf.value;
                    m_plot.moveCursor(RIGHT_CURSOR, m_strRCValue);
                }
            }
        }

        public void setShowValue(ParamIF pf) {
            if (pf != null && pf.value != null) {
                String  s = pf.value.trim();
                int nActive = Integer.parseInt(s);
                if (nActive > 0) {
                    m_bRightCursorOn = true;
                    if (m_strRCSetVal != null) {
                        vnmrIf.asyncQueryParam(this, m_strRCSetVal, precision);
                    }
                } else {
                    m_bRightCursorOn = false;
                }
                repaint();
            }
        }

        public void sendVnmrCmd(int nMouseAction) {
            if (vnmrIf == null) {
                return;
            }
            if (nMouseAction == MOUSE_RELEASE && m_strRCCmd != null) {
                vnmrIf.sendVnmrCmd(this, m_strRCCmd, RCVAL);
            } else if (nMouseAction == MOUSE_PRESS && m_strRCCmd2 != null) {
                 vnmrIf.sendVnmrCmd(this, m_strRCCmd2, RCVAL);
            } else if (nMouseAction == MOUSE_DRAG && m_strRCCmd3 != null) {
                vnmrIf.sendVnmrCmd(this, m_strRCCmd3, RCVAL);
            }
        }

        public String getAttribute(int nAttr) {
            return m_graph.getAttribute(nAttr);
        }

        public void setAttribute(int nAttr, String strAttr) {
            m_graph.setAttribute(nAttr, strAttr);
        }
    }

    /**
     * Layout for the canvas component.
     */
    protected class MyLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}
        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        } // unused
        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        } // unused

        /**
         * do the layout
         * @param target component to be laid out
         */
        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock())
            {
                Dimension targetSize = target.getSize();
                Insets insets = target.getInsets();
                int width = targetSize.width - insets.left - insets.right;
                int height = targetSize.height - insets.top - insets.bottom;
                m_plot.setBounds(insets.left, insets.top, width, height);
            }
        }
    }

    protected class CursedPlot extends Plot {

        protected static final int ABOVE = 1;
        protected static final int LEFT = 2;
        protected static final int BELOW = 4;
        protected static final int RIGHT = 8;
        protected static final int POSITION = 0xf;
        protected static final int BUTTON1 = 16;
        protected static final int BUTTON2 = 32;
        protected static final int BUTTON3 = 64;
        protected static final int BUTTON = 0x70;
        protected static final int EXTERNAL = 0x10000;

        protected int cursorType = 0; // LEFT, BELOW, or 0
        protected int actionMode = 0; // OR of LEFT, BELOW, etc.

        protected Point2D.Double leftPoint;
        protected Point2D.Double rightPoint;
        protected MouseMotionListener ptMouseMotionListener = null;
        protected Cursor m_cursor
            = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);

        public CursedPlot() {
            init(true);
        }

        public CursedPlot(boolean installListeners) {
            init(installListeners);
        }

        protected void init(boolean installListeners) {
            setBackground(Util.getBgColor());
            leftPoint = new Point2D.Double();
            rightPoint = new Point2D.Double();
            removeMouseListeners();

            if (installListeners) {
                MouseInputListener mouseListener = new MouseInputListener();
                addMouseMotionListener(mouseListener);
                addMouseListener(mouseListener);
            }
        }

        public void initialize() {
            clear(false);
            setTitle(null);
            removeLegendButtons();
            repaint();
        }

        public void setDefaultCursor(Cursor curs) {
            if (curs != null) {
                cursorType = EXTERNAL;
                if (m_cursor != curs) {
                    m_cursor = curs;
                    setCursor(curs);
                }
            } else {
                if (cursorType == EXTERNAL) {
                    m_cursor = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
                    setCursor(m_cursor);
                    cursorType = 0;
                }
            }
        }

        protected void removeMouseListeners() {
            MouseMotionListener[] list = (MouseMotionListener[])
                (getListeners(MouseMotionListener.class));
            if (list.length > 0) {
                ptMouseMotionListener = list[0];
                removeMouseMotionListener(ptMouseMotionListener);
            }
            MouseListener[] list2 = (MouseListener[])
                (getListeners(MouseListener.class));
            if (list2.length > 0) {
                removeMouseListener(list2[0]);
            }
        }

        /**
         * Draw the cursors on the plot.
         * First calls super._drawPlot() to draw the rest of the plot.
         */
        protected synchronized void _drawPlot(Graphics graphics,
                                              boolean clearfirst,
                                              Rectangle drawRectangle) {
            setPlotOptions();   // TODO: just do last-minute settings
            super._drawPlot(graphics, clearfirst, drawRectangle);

            // Draw cursors
            if (m_bLeftCursorOn) {
                int leftPix = _ulx + (int)((leftPoint.x  - _xMin) * _xscale);
                if (leftPix > _ulx && leftPix < _lrx) {
                    graphics.setColor(m_lcColor);
                    graphics.drawLine(leftPix, _uly + 1, leftPix, _lry - 1);
                }
            }
            if (m_bRightCursorOn) {
                int rightPix = _ulx + (int)((rightPoint.x  - _xMin) * _xscale);
                if (rightPix > _ulx && rightPix < _lrx) {
                    graphics.setColor(m_rcColor);
                    graphics.drawLine(rightPix, _uly + 1, rightPix, _lry - 1);
                }
            }
        }

        public void moveCursor(int nCursor, String strValue) {
            try {
                double value = Double.parseDouble(strValue);
                moveCursor(nCursor, value);
            } catch (NumberFormatException nfe) {}
        }

        public void moveCursor(int nCursor, double value) {
            double left = leftPoint.x;
            double right = rightPoint.x;
            if (nCursor == LEFT_CURSOR) {
                leftPoint.x = value;
                m_strLCValue = Double.toString(leftPoint.x);
                if (rightPoint.x < leftPoint.x) {
                    rightPoint.x = leftPoint.x;
                    m_strRCValue = Double.toString(rightPoint.x);
                }
            } else if (nCursor == RIGHT_CURSOR) {
                rightPoint.x = value;
                m_strRCValue = Double.toString(rightPoint.x);
                if (leftPoint.x > rightPoint.x) {
                    leftPoint.x = rightPoint.x;
                    m_strLCValue = Double.toString(leftPoint.x);
                }
            } else {
                return;
            }
            if (left != leftPoint.x || right != rightPoint.x) {
                repaint();
            }
        }

        protected class MouseInputListener extends MouseInputAdapter {
            protected boolean otherCursorChanged;
            protected boolean movingCursors = false;
            protected double cursorXOnData;
            protected double cursorYOnData;
            protected double xDataRange[];
            protected double yDataRange[];

            public void mouseMoved(MouseEvent e) {
                checkCursorIcon(e);
            }

            private void checkCursorIcon(MouseEvent e) {
                if (cursorType == EXTERNAL) {
                    return;
                }
                int off = mouseOffCanvas(e);
                //int but = mouseButton(e);
                actionMode = off;
                if (!offsetDragEnabled) {
                    return;
                }
                Cursor curs;
                if (off == LEFT) {
                    /*
                     * This if-else stuff is rather sloppy.
                     * When (say) the left button goes up, we get
                     * isLeftMouseButton() == true, but we want the
                     * SouthCursor to go off at this point.  It will,
                     * only because we have already set
                     * cursorType == (LEFT | BUTTON1),
                     * so the (cursorType != LEFT) clause gets executed.
                     * If this method got called on drag, it wouldn't work.
                     */
                    if (SwingUtilities.isRightMouseButton(e)
                        && cursorType != (LEFT | BUTTON3))
                    {
                        curs = VCursor.getCursor("northResize");
                        m_plot.setCursor(curs);
                        cursorType = LEFT | BUTTON3;
                    } else if (SwingUtilities.isLeftMouseButton(e)
                        && cursorType != (LEFT | BUTTON1))
                    {
                        curs = VCursor.getCursor("southResize");
                        m_plot.setCursor(curs);
                        cursorType = LEFT | BUTTON1;
                    } else if (cursorType != LEFT) {
                        curs = VCursor.getCursor("verticalMove");
                        m_plot.setCursor(curs);
                        cursorType = LEFT;
                    }
                } else if (off == BELOW) {
                    if (SwingUtilities.isRightMouseButton(e)
                        && cursorType != (BELOW | BUTTON3))
                    {
                        curs = VCursor.getCursor("eastResize");
                        m_plot.setCursor(curs);
                        cursorType = BELOW | BUTTON3;
                    } else if (SwingUtilities.isLeftMouseButton(e)
                        && cursorType != (BELOW | BUTTON1))
                    {
                        curs = VCursor.getCursor("westResize");
                        m_plot.setCursor(curs);
                        cursorType = BELOW | BUTTON1;
                    } else if (cursorType != BELOW) {
                        curs = VCursor.getCursor("horizontalMove");
                        m_plot.setCursor(curs);
                        cursorType = BELOW;
                    }
                } else if (cursorType != 0) {
                    curs = m_cursor;
                    m_plot.setCursor(curs);
                    cursorType = 0;
                }
            }

            public void mousePressed(MouseEvent e) {
                checkCursorIcon(e);
                if (actionMode == 0) {
                    otherCursorChanged = false;
                    mousePosition(e, MOUSE_PRESS);
                    sendCursorCmds(e, MOUSE_PRESS);
                } else if (actionMode == LEFT) {
                    setYOffset(e);
                } else if (actionMode == BELOW) {
                    setXOffset(e);
                }
            }

            private void setXOffset(MouseEvent e) {
                cursorXOnData = xPixToData(e.getX());
                xDataRange = m_plot.getXRange();
            }

            private void setYOffset(MouseEvent e) {
                cursorYOnData = yPixToData(e.getY());
                yDataRange = m_plot.getYRange();
            }

            public void mouseDragged(MouseEvent e) {
                if (actionMode == 0) {
                    mousePosition(e, MOUSE_DRAG);
                    sendCursorCmds(e, MOUSE_DRAG);
                } else if (actionMode == LEFT) {
                    moveYOffset(e);
                } else if (actionMode == BELOW) {
                    moveXOffset(e);
                }
            }

            private void moveXOffset(MouseEvent e) {
                if (xDataRange == null) {
                    Messages.postDebug("moveXOffset(): xDataRange=null");
                    return;
                }
                double x = xPixToData(e.getX());
                double r[] = getXRange();
                if (SwingUtilities.isRightMouseButton(e)) {
                    double xmax = r[0] + (cursorXOnData - r[0])
                            * (r[1] - r[0]) / (x - r[0]);
                    double factor = (xmax - r[0]) / (xDataRange[1] - r[0]);
                    if (x < r[0] || factor > 10) {
                        xmax = r[0] + 10 * (xDataRange[1] - r[0]);
                    } else if (factor < 0.1) {
                        xmax = r[0] + 0.1 * (xDataRange[1] - r[0]);
                    }
                    if (cursorXOnData > r[0]) {
                        m_plot.setXRange(r[0], xmax);
                    }
                } else if (SwingUtilities.isLeftMouseButton(e)) {
                    double xmin = r[1] - (r[1] - cursorXOnData)
                            * (r[1] - r[0]) / (r[1] - x);
                    double factor = (r[1] - xmin) / (r[1] - xDataRange[0]);
                    if (x > r[1] || factor > 10) {
                        xmin = r[1] - 10 * (r[1] - xDataRange[0]);
                    } else if (factor < 0.1) {
                        xmin = r[1] - 0.1 * (r[1] - xDataRange[0]);
                    }
                    if (cursorXOnData < r[1]) {
                        m_plot.setXRange(xmin, r[1]);
                    }
                } else {
                    x += xDataRange[0] - r[0];
                    double delta = x - cursorXOnData;
                    m_plot.setXRange(xDataRange[0] - delta,
                                     xDataRange[1] - delta);
                }
                m_plot.repaint();
            }

            private void moveYOffset(MouseEvent e) {
                if (yDataRange == null) {
                    Messages.postDebug("moveYOffset(): yDataRange=null");
                    return;
                }
                double y = yPixToData(e.getY());
                double r[] = getYRange();
                if (SwingUtilities.isRightMouseButton(e)) {
                    double ymax = r[0] + (cursorYOnData - r[0])
                            * (r[1] - r[0]) / (y - r[0]);
                    double factor = (ymax - r[0]) / (yDataRange[1] - r[0]);
                    if (y < r[0] || factor > 10) {
                        ymax = r[0] + 10 * (yDataRange[1] - r[0]);
                    } else if (factor < 0.1) {
                        ymax = r[0] + 0.1 * (yDataRange[1] - r[0]);
                    }
                    if (cursorYOnData > r[0]) {
                        m_plot.setYRange(r[0], ymax);
                    }
                } else if (SwingUtilities.isLeftMouseButton(e)) {
                    double ymin = r[1] - (r[1] - cursorYOnData)
                            * (r[1] - r[0]) / (r[1] - y);
                    double factor = (r[1] - ymin) / (r[1] - yDataRange[0]);
                    if (y > r[1] || factor > 10) {
                        ymin = r[1] - 10 * (r[1] - yDataRange[0]);
                    } else if (factor < 0.1) {
                        ymin = r[1] - 0.1 * (r[1] - yDataRange[0]);
                    }
                    if (cursorYOnData < r[1]) {
                        m_plot.setYRange(ymin, r[1]);
                    }
                } else {
                    y += yDataRange[0] - r[0];
                    double delta = y - cursorYOnData;
                    m_plot.setYRange(yDataRange[0] - delta,
                                     yDataRange[1] - delta);
                }
                m_plot.repaint();
            }

            public void mouseReleased(MouseEvent e) {
                if (actionMode == 0) {
                    sendCursorCmds(e, MOUSE_RELEASE);
                    otherCursorChanged = false;
                }
                checkCursorIcon(e);
            }

            public void mouseClicked(MouseEvent me) {
                if (me.getClickCount() == 2
                    && (SwingUtilities.isLeftMouseButton(me)
                        || SwingUtilities.isRightMouseButton(me)))
                {
                    int off = mouseOffCanvas(me);
                    if (off == 0) {
                        expand();
                    }
                    repaint();
                }
            }

            private void expand() {
                setXRange(leftPoint.x, rightPoint.x);
            }

            protected void mousePosition(MouseEvent e, int nMouseAction) {
                Point2D.Double point;
                if (SwingUtilities.isLeftMouseButton(e)) {
                    movePointToMouse(e, leftPoint);
                    m_strLCValue = Double.toString(leftPoint.x);
                    if (rightPoint.x < leftPoint.x) {
                        rightPoint.x = leftPoint.x;
                        m_strRCValue = Double.toString(rightPoint.x);
                        otherCursorChanged = true;
                    }
                } else if (SwingUtilities.isRightMouseButton(e)) {
                    movePointToMouse(e, rightPoint);
                    m_strRCValue = Double.toString(rightPoint.x);
                    if (leftPoint.x > rightPoint.x) {
                        leftPoint.x = rightPoint.x;
                        m_strLCValue = Double.toString(leftPoint.x);
                        otherCursorChanged = true;
                    }
                } else if (SwingUtilities.isMiddleMouseButton(e)) {
                    double ydat = 0;
                    double ydatMax = 0;
                    for (int idat = getNextScalingDataSet(-1);
                         idat >= 0;
                         idat = getNextScalingDataSet(idat))
                    {
                        ydat = getDataValueAt(xPixToData(e.getX()), idat);
                        ydat = Math.abs(ydat);
                        if (ydatMax < ydat) {
                            ydatMax = ydat;
                        }
                    }
                    double y = yPixToData(e.getY());
                    double sf = 0.5;
                    if (y > 0 && ydatMax != 0) {
                        sf = y / ydatMax;
                    }
                    double r[] = getYRange();
                    double r0 = Math.abs(r[0] / sf);
                    double r1 = Math.abs(r[1] / sf);
                    if (r0 < 1e30 && r1 < 1e30 && r1 > r0) {
                        setYRange(r[0] / sf, r[1] / sf);
                    }
                } else {
                    return;
                }
                repaint();
            }

            protected void movePointToMouse(MouseEvent e,
                                            Point2D.Double point) {
                int x = e.getX();
                int y = e.getY();
                if (x <= _ulx) {
                    x = _ulx;
                } else if (x >= _lrx) {
                    x = _lrx;
                }

                // Convert from pixel to user coordinates.
                point.x = _xMin + (x + 0.5 - _ulx) / _xscale;
                point.y = _yMin + (y + 0.5 - _uly) / _yscale;
            }

            /**
             * Returns a bitmapped integer indicating where the
             * MouseEvent is relative to the canvas.
             * Returns 0 if the event is on the canvas, otherwise
             * returns a bitwise OR of the following values:
             * ABOVE, LEFT, BELOW, RIGHT.
             */
            protected int mouseOffCanvas(MouseEvent e) {
                int rtn = 0;
                int x = e.getX();
                int y = e.getY();

                if (x < _ulx) {
                    rtn |= LEFT;
                } else if (x > _lrx) {
                    rtn |= RIGHT;
                }
                if (y < _uly) {
                    rtn |= ABOVE;
                } else if (y > _lry) {
                    rtn |= BELOW;
                }
                return rtn;
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
                    if (otherCursorChanged) {
                        int cmd = nMouseAction;
                        if (cmd == MOUSE_PRESS) {
                            cmd = MOUSE_RELEASE;
                        } else if (cmd == MOUSE_RELEASE) {
                            otherCursorChanged = false;
                        }
                        m_objMax.sendVnmrCmd(cmd);
                    }
                } else if (SwingUtilities.isRightMouseButton(e)) {
                    m_objMax.sendVnmrCmd(nMouseAction);
                    if (otherCursorChanged) {
                        int cmd = nMouseAction;
                        if (cmd == MOUSE_PRESS) {
                            cmd = MOUSE_RELEASE;
                        } else if (cmd == MOUSE_RELEASE) {
                            otherCursorChanged = false;
                        }
                        m_objMin.sendVnmrCmd(cmd);
                    }
                }
            }

        }
    }
}
