/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Component;
import java.awt.Cursor;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.MediaTracker;
import java.awt.Rectangle;
import java.awt.event.MouseEvent;
import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.SortedSet;

import javax.swing.SwingUtilities;

import vnmr.bo.VObjDef;
import vnmr.bo.VPlot;
import vnmr.lc.LcEvent.EventTrig;
import vnmr.lc.LcEvent.MarkType;
import vnmr.ui.SessionShare;
import vnmr.util.ButtonIF;
import vnmr.util.DisplayOptions;
import vnmr.util.Fmt;
import vnmr.util.Messages;
import vnmr.util.Util;
import vnmr.util.VCursor;

/**
 * This VObj object displays a plot of a chromatogram.
 */
public class VLcPlot extends VPlot implements LcDef {

    private final static int UNKNOWN_TYPE = 0;
    private final static int QUEUED_EVENT = 1;
    private final static int PAST_INCIDENT = 2;
    private final static int FIXED_EVENT = 3;

    private final static int NORMAL_COLOR = 0;
    private final static int HIGHLIGHT_COLOR = 1;
    private final static int MOVABLE_COLOR = 2;

    static protected Map<String, Image> refImages
        = new HashMap<String, Image>();

    protected Color queuedMarkColor = new Color(0x90BF62); // Green
    protected Color incidentMarkColor = new Color(0xF0B9B9); // Lighter red
    protected Color normalMarkColor = new Color(0xFF0000); // Red
    protected Color commandMarkColor = new Color(0x9696DA); // Blue
    protected Color elutionMarkColor = new Color(0xAD985D); // Brown
    protected Color injectOnColor = new Color(0x90BF62); // Green
    protected Color injectOffColor = new Color(0xE28585); // Dull red
    protected Color unknownMarkColor = Color.GRAY;
    protected Color inactiveMarkColor = Color.LIGHT_GRAY;

    private Color m_bgColor2 = Color.GRAY;
    private String m_strBGCol2 = "gray";

    public VLcPlot(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);

        m_plot = new LcPlot();
        m_plot.setOpaque(false);
        m_plot.setBackground(null);
        add(m_plot);
        offsetDragEnabled = true;
    }

    public LcPlot getLcPlot() {
        return (LcPlot)m_plot;
    }

    /**
     * Override method in VPlot in order to make each threshold color
     * match the data color for that channel.
     */
    protected void setPlotOptions() {
        super.setPlotOptions();
        for (int i = MAX_TRACES - 1; i >= 0; --i) {
            Color color = m_plot.getMarkColor(i);
            m_plot.setMarkColor(color, 2 * i);
            m_plot.setMarkColor(color, 2 * i + 1);
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

        switch (attr) {
        case GRAPHBGCOL2:
            m_strBGCol2 = c;
            if (c != null) {
                m_bgColor2 = DisplayOptions.getColor(c);
                //m_plot.repaint();
            }
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
        switch (attr) {
        case GRAPHBGCOL2:
            strAttr = m_strBGCol2;
            break;
        default:
            strAttr = super.getAttribute(attr);
            break;
        }
        return strAttr;
    }

    public void setColors() {
        super.setColors();
        setShadowColor();
    }

    public void setShadowColor() {
        String strColor = getAttribute(VObjDef.GRAPHBGCOL2);
        if (strColor != null) {
            m_strBGCol2 = strColor;
            m_bgColor2 = DisplayOptions.getColor(strColor);
        }
    }


    public class LcPlot extends VPlot.CursedPlot {

        private Collection<Marker> markerList;
        private Marker minMark = null;
        private Marker prevMinMark = null;
        private int minMarkType = UNKNOWN_TYPE;
        private boolean minMarkMoved = false;
        private int xOffset = 0;
        private int yOffset = 0;
        private SortedSet<LcEvent> eventQueue = null;
        private ArrayList incidentList = null;
        private LcControl lcCtl = null;
        private double highlightVolume = 0;
        //private int PointCount;
        //private short[][] Data;
        //private int PlotStart, PlotEnd;       // indices of data to plot
        //private int Yspan;            // generally = (full scale) - (offset)
        //private short Lower, Upper;   // lower, upper limits of data in plot
        //private short MaximumAdcValue;

        public LcPlot() {
            super();
            //Data = null;
            //PointCount = 0;
            //PlotStart = PlotEnd = 0;
            //Yspan = 1;
            setSize(600,400);
            setYRange(0, 2);
            setXRange(0, 3);
            setWalking(true, 3); // Default width of window in minutes
            setGrid(true);
            setXAxis(true);
            setXLabel("Retention Time (minutes)");
            setYAxis(true);
            setYLabel("Absorption (AU)");
            setMarkColor(Color.blue, 0);
            setFillButton(true);
            setTailButton(true);

            removeMouseListeners();
            MouseInputListener mouseListener = new MouseInputListener();
            addMouseMotionListener(mouseListener);
            addMouseListener(mouseListener);

            markerList = new HashSet<Marker>();
        }

        public void initialize(SortedSet<LcEvent> eventQueue,
                               ArrayList incidentList) {
            clearMarkers();
            setEventQueue(eventQueue);
            setIncidentList(incidentList);
            setMarkColor(Color.BLACK, TEMP_DATASET);
            super.initialize();
        }

        public void clearMarkers() {
            markerList.clear();
        }

        public void markEvents() {
            if (eventQueue == null) {
                return;
            }
            LcEvent[] eventArray = null;
            eventArray = eventQueue.toArray(new LcEvent[0]);
            for (int i = 0; i < eventArray.length; i++) {
                LcEvent e = eventArray[i];
                double y = 0;
                String iconName = null;
                switch (e.markType) {
                case SLICE:
                    y = SLICE_Y_POSITION;
                    iconName = "TimeSlice";
                    break;
                case ELUTION:
                case COLLECTION:
                    y = ELUTION_Y_POSITION;
                    iconName = "EluteLoop";
                    break;
                case PEAK:
                case HOLD:
                    iconName = "StopFlow";
                    break;
                case COMMAND:
                    y = DEFAULT_Y_POSITION;
                    iconName = "LcCommand";
                    break;
                default:
                    y = DEFAULT_Y_POSITION;
                }
                if (e.markType != MarkType.NONE
                    || e.markType != MarkType.SLICEOFF) // No mark for now
                {
                    addMark(e.time_ms / 60000.0, y, iconName, e);
                }
            }
            repaint();
        }

        public void unmarkEvent(LcEvent event) {
            Marker[] marks = markerList.toArray(new Marker[0]);
            for (int i=0; i < marks.length; i++) {
                if (event == marks[i].getEvent()) {
                    removeMark(marks[i]);
                    break;
                }
            }
            repaint();
        }

        public void unmarkEvents() {
            if (eventQueue == null) {
                return;
            }
            Marker[] marks = markerList.toArray(new Marker[0]);
            for (int i=0; i < marks.length; i++) {
                if (eventQueue.contains(marks[i].getEvent())) {
                    removeMark(marks[i]);
                }
            }
            repaint();
        }

        public void markPeaks(Graphics graphics, SortedSet peaks) {
            Iterator iter = peaks.iterator();
            LcPeak peak;
            double xmin = getFullXRange()[0];
            double xmax = getFullXRange()[1];
            Color[] colors = new Color[MAX_TRACES];
            for (int i = 0; i < MAX_TRACES; i++) {
                Color color = _colors[(i * 2) % _colors.length];
                colors[i] = color.darker();
            }
            /*System.out.println("Mark Peaks: "
              + "Line#, Time, FWHM, Area, Height"
              + ", PkStart, PkEnd, BaseStart(x,y)"
              + ", BaseEnd(x.y)");/*CMP*/
            final float dash1[] = {5};
            final BasicStroke dashed = new BasicStroke(2,
                                                       BasicStroke.CAP_BUTT,
                                                       BasicStroke.JOIN_MITER,
                                                       5, dash1, 0);
            ((Graphics2D)graphics).setStroke(dashed);

            while(iter.hasNext()) {
                peak = (LcPeak)iter.next();
                if (peak.blStartTime > xmax) {
                    break;
                }

                // NB: Several peaks can have the same baseline,
                // so some baselines will get drawn multiple times.
                if (peak.blEndTime > xmin) {
                    /*String id = "" + peak.id + (char)('a' + peak.channel);
                      System.out.println(id
                      + ", " + peak.time
                      + ", " + peak.fwhm
                      + ", " + peak.area
                      + ", " + peak.height
                      + ", " + peak.startTime
                      + ", " + peak.endTime
                      + ", (" + peak.blStartTime
                      + ", " + peak.blStartVal
                      + "), (" + peak.blEndTime
                      + ", " + peak.blEndVal + ")");/*CMP*/
                    int dataset = (peak.channel * 2);
                    if (isShown(dataset)) {
                        graphics.setColor(colors[peak.channel % colors.length]);

                        // Draw baseline
                        int x0 = xDataToPix(peak.blStartTime);
                        int x1 = xDataToPix(peak.blEndTime);
                        double blStartValue = peak.blStartVal;
                        //= getDataValueAt(peak.blStartTime, dataset);
                        int y0 = yDataToPix(blStartValue);
                        double blEndValue = peak.blEndVal;
                        //= getDataValueAt(peak.blEndTime, dataset);
                        int y1 = yDataToPix(blEndValue);
                        graphics.drawLine(x0, y0, x1, y1);

                        // Draw peak center and height
                        int xPeak = xDataToPix(peak.time);
                        double y = peak.blStartVal
                                + (peak.blEndVal - peak.blStartVal)
                                * (peak.time - peak.blStartTime)
                                / (peak.blEndTime - peak.blStartTime);
                        int yTop = yDataToPix(y + peak.height);

                        //int yBottom = yDataToPix(y);
                        //graphics.drawLine(xPeak, yBottom, xPeak, yTop);

                        // Draw peak "sides"
                        double yPkLeft = blStartValue
                                + (blEndValue - blStartValue)
                                * (peak.startTime - peak.blStartTime)
                                / (peak.blEndTime - peak.blStartTime);
                        double yPkRight = blStartValue
                                + (blEndValue - blStartValue)
                                * (peak.endTime - peak.blStartTime)
                                / (peak.blEndTime - peak.blStartTime);
                        x0 = xDataToPix(peak.startTime);
                        y0 = yDataToPix(yPkLeft);
                        graphics.drawLine(xPeak, yTop, x0, y0);
                        x1 = xDataToPix(peak.endTime);
                        y1 = yDataToPix(yPkRight);
                        graphics.drawLine(xPeak, yTop, x1, y1);
                    }
                }
            }

        }

        public void setLcControl(LcControl lcc) {
            lcCtl = lcc;
        }

        public void setEventQueue(SortedSet<LcEvent> q) {
            eventQueue = q;
        }

        public void setIncidentList(ArrayList list) {
            incidentList = list;
        }

        public void setHighlightVolume(double v) {
            highlightVolume = v;
        }

        public Marker addMark(double x, double y, LcEvent e) {
            String iconName = "null";
            MarkType markType = e.markType;
            switch (e.markType) {
            case SLICE:
                iconName = "TimeSlice";
                break;
            case ELUTION:
            case COLLECTION:
                iconName = "EluteLoop";
                break;
            case PEAK:
            case HOLD:
                iconName = "StopFlow";
                break;
            case COMMAND:
                iconName = "LcCommand";
                break;
            default:
                y = DEFAULT_Y_POSITION;
                break;
            }
            return addMark(x, y, iconName, e);
        }

        public Marker addMark(double x, double y, String icon, LcEvent e) {
            //Messages.postInfo("addMark(" + x + ", " + y + ")");
            Marker marker = new Marker(x, y, icon, e);
            markerList.add(marker);
            return marker;
        }

        public void removeMark(Marker mark) {
            markerList.remove(mark);
        }

        /**
         * Checks that the status of markers is consistent with
         * their being still pending or already happened.
         */
        public void validateMarks(boolean fullRepaint) {
            Marker[] marks = markerList.toArray(new Marker[0]);
            Cursor curs = null;
            for (int i=0; i < marks.length; i++) {
                if (marks[i].isShown()) {
                    boolean change = false;
                    if (marks[i] == minMark) {
                        // How it is shown on cursor rollover
                        if (minMarkType == QUEUED_EVENT) {
                            // Indicate that it can be moved
                            change = marks[i].setColor(MOVABLE_COLOR);
                            curs = VCursor.getCursor("horizontalMove");
                        } else if (minMarkType == PAST_INCIDENT) {
                            // Show that it's clickable
                            change = marks[i].setColor(HIGHLIGHT_COLOR);
                        } else if (minMarkType == FIXED_EVENT) {
                            change = marks[i].setColor(NORMAL_COLOR);
                        } else {
                            // Does this ever happen?
                            change = marks[i].setColor(NORMAL_COLOR);
                        }
                    } else {
                        // How it is normally shown
                        change = marks[i].setColor(NORMAL_COLOR);
                    }
                    if (change && !fullRepaint) {
                        Graphics g = getGraphics();
                        g.setClip(_ulx, _uly, _lrx - _ulx, _lry - _uly);
                        marks[i].paintMark(m_plot, g,
                                           xDataToPix(marks[i].getX()),
                                           yDataToPix(marks[i].getYPlotted()));
                    }
                }
            }
            m_plot.setDefaultCursor(curs);
            if (fullRepaint) {
                repaint();
            }
        }

        /**
         * Override method in VPlot to ignore threshold lines.
         */
        //protected boolean isScalingDataSet(int index) {
        //    return (index >= 0 && index < getNumDataSets() && (index & 1) == 0);
        //}

        /**
         * Override method in VPlot to ignore threshold lines.
         */
        //protected int getNextScalingDataSet(int index) {
        //    if ((index < 0)) {
        //        index = -2;
        //    }
        //    index += 2;
        //    if (index < getNumDataSets()) {
        //        return index;
        //    }
        //    return -1;
        //}

        public void drawPoint(int chan, double x, double y) {
            addPoint(chan * 2, x, y, true);
            repaint();          // TODO: Only if NMR band changed?
        }

        /**
         * Draw event markers on the plot.
         * First calls super._drawPlot() to draw the rest of the plot.
         */
        protected synchronized void _drawPlot(Graphics graphics,
                                              boolean clearfirst,
                                              Rectangle drawRectangle) {

            super._drawPlot(graphics, clearfirst, drawRectangle);

            double[] xr = getFullXRange();
            Marker[] marks = markerList.toArray(new Marker[0]);
            for (int i=0; i < marks.length; i++) {
                //LcEvent event = marks[i].getEvent();
                //if (event != null && isShown(2 * (event.channel - 1))) {
                if (marks[i].isShown()) {
                    double x = marks[i].getX();
                    if (x >= xr[0] && x <= xr[1]) {
                        // Need to draw this mark
                        //double y = getDataValueAt(x, marks[i].getDataset());
                        double y = marks[i].getY();
                        //double[] yr = getYRange();
                        int yPix = yDataToPix(y);
                        if (yPix < _uly + marks[i].getHeight() + 2) {
                            yPix = _uly + marks[i].getHeight() + 2;
                        }
                        else if (yPix > _lry) {
                            yPix = _lry;
                        }
                        //y = Math.min(Math.max(y, yr[0]), yr[1]);
                        marks[i].setYPlotted(yPixToData(yPix));
                        marks[i].paintMark(this, graphics, xDataToPix(x),
                                           yDataToPix(marks[i].getYPlotted()));
                    }
                }
            }

            // Draw the peak measurements, if we have them
            if (lcCtl.peakAnalysis != null) {
                markPeaks(graphics, lcCtl.peakAnalysis);
            }

            // Put up legend
            //clearLegends();
            //for (int i = 0; i < _points.size(); i += 2) {
            //    if (((ArrayList)_points.get(i)).size() > 0) {
            //        addLegendButton(i, " " + (char)('A' +  (i / 2)));
            //    }
            //}
        }

        /**
         * Draw the background for the plot.  We override the standard
         * method so we can put a vertical band for the present NMR
         * probe position.
         */
        protected void _drawCanvas(Graphics g, int x, int y, int wd, int ht) {
            super._drawCanvas(g, x, y, wd, ht); // Draw normal background
            LcRun run = lcCtl.getLcRun();
            if (run != null && run.isActive()) {
                // Draw probe position
                double flowRate = lcCtl.getCurrentMethod().getFlow(0);
                double probeWidth = highlightVolume / flowRate; // In minutes
                double flowTime = run.getNmrFlowTime();
                double diff = System.currentTimeMillis() / 60000.0 - flowTime;
                Messages.postDebug("NmrFlowTime",
                                   "VLcPlot._drawCanvas: nmrFlowTime="
                                   + Fmt.f(4, flowTime)
                                   + ", now-flow=" + Fmt.f(4, diff));
                double probeStart = flowTime - probeWidth / 2;
                int left = xDataToPix(probeStart);
                int width = (int)(probeWidth * _xscale);
                if (width < 3) {
                    width = 3;  // Make sure it's visible
                    left -= 1;
                }
                if (left + width > _ulx && left < _lrx) {
                    if (left + width >= _lrx) {
                        width = _lrx - left - 1;
                    }
                    if (left <= _ulx) {
                        width -= _ulx - left;
                        left = _ulx + 1;
                    }
                    g.setColor(m_bgColor2);
                    g.fillRect(left, _uly + 1, width, _lry - _uly - 1);
                }
            }
        }


        private class MouseInputListener
            extends VPlot.CursedPlot.MouseInputListener {

            public void mouseMoved(MouseEvent me) {
                //if (mouseOffCanvas(me) != 0) {
                //    super.mouseMoved(me);
                //}
                double min = 99 * 99;
                int mx = me.getX();
                int my = me.getY();
                Marker[] marks = markerList.toArray(new Marker[0]);

                // Go through the marks to find out which is closest
                minMark = null;
                for (int i=0; i < marks.length; i++) {
                    // Only consider marks that are displayed
                    if (marks[i].isShown()) {
                        Rectangle rect = marks[i].getIconRect();
                        if (mx >= rect.x && mx <= rect.x + rect.width
                            && my >= rect.y && my <= rect.y + rect.height)
                        {
                            // We are on the icon, want the closest
                            int x = rect.x + rect.width / 2;
                            int y = rect.y + rect.height / 2;
                            double dx = x - mx;
                            double dy = y - my;
                            double d2 = dx * dx + dy * dy;
                            if (min > d2) {
                                min = d2;
                                minMark = marks[i];
                            }
                        }
                    }
                }

                if (prevMinMark != minMark) {
                    //Messages.postDebug("plotScale", "minMark=" + minMark);
                    prevMinMark = minMark;
                }

                // Find out what kind of mark it is.
                LcRun run = lcCtl.getLcRun();
                if (minMark != null) {
                    LcEvent event = minMark.getEvent();
                    if (event != null
                        && (event.markType != MarkType.HOLD
                            && event.markType != MarkType.PEAK
                            && event.markType != MarkType.SLICE
                            ))
                    {
                        minMarkType = FIXED_EVENT;
                    } else if (run != null  && run.isActive()
                               && eventQueue != null
                               && eventQueue.contains(event))
                    {
                        minMarkType = QUEUED_EVENT;
                    } else if (incidentList != null
                               && incidentList.contains(event))
                    {
                        minMarkType = PAST_INCIDENT;
                    } else {
                        minMarkType = UNKNOWN_TYPE;
                    }

                    // Set tooltip text
                    String cmd = event.action;
                    /*
                    if (cmd != null) {
                        // Ad-hoc editing to clean up text
                        if (cmd.startsWith("timeSliceNmr('y")) {
                            cmd = "timeSliceNmr";
                        } else if (cmd.startsWith("timeSliceNmr('n")) {
                            cmd = "timeSliceNmr off";
                        }
                    }
                    */
                    String strPkLabel = "";
                    if (event != null && event.peakNumber > 0
                        && (event.markType == MarkType.HOLD
                            || event.markType == MarkType.PEAK
                            || event.markType == MarkType.SLICE))
                    {
                        double time_min = event.actual_ms / 60000.0;
                        strPkLabel = "Stop #" + event.peakNumber
                                + " at " + Fmt.f(2, time_min) + " min<br>";
                    }
                    setToolTipText("<html>" + strPkLabel
                                   + event.trigger.label()
                                   + "<br> " + cmd);
                } else {
                    setToolTipText(null);
                }

                validateMarks(false);
                super.mouseMoved(me);
            }

            public void mousePressed(MouseEvent me) {
                if (mouseOffCanvas(me) != 0) {
                    super.mousePressed(me);
                    return;
                }
                boolean left = SwingUtilities.isLeftMouseButton(me);
                boolean right = SwingUtilities.isRightMouseButton(me);
                if (me.isShiftDown() && left) {
                    LcRun run = lcCtl.getLcRun();
                    if (eventQueue != null && run != null && run.isActive()) {
                        // Add a mark here
                        double x = xPixToData(me.getX());
                        double y = yPixToData(me.getY());

                        if (run != null) {
                            // Don't create an event in the past!
                            x = Math.max(x, run.getNmrFlowTime());
                        }

                        // NB: Make an event for fake channel -1
                        int loop = lcCtl.getLoop();
                        String cmd = "stopFlowNmr"; // Default
                        LcMethod method = lcCtl.getCurrentMethod();
                        if (method != null) {
                            cmd = method.getPeakAction(x, run.getRunType());
                        }
                        LcEvent lce = new LcEvent((int)(x * 60000 + 0.5),
                                                  0, // Delay
                                                  0, // Channel
                                                  loop,
                                                  EventTrig.CLICK_PLOT,
                                                  cmd);
                        lce.peakNumber = -1;
                        eventQueue.add(lce);
                        minMark = addMark(x, y, "StopFlow", lce);
                        minMarkType = QUEUED_EVENT;
                        validateMarks(true);
                        //m_plot.repaint(); // Redo the whole plot!
                    }
                } else if (minMark != null && left) {
                    if (minMarkType == QUEUED_EVENT) {
                        // Remove event from queue while moving mark
                        eventQueue.remove(minMark.getEvent());
                        xOffset = xDataToPix(minMark.getX()) - me.getX();
                        yOffset = yDataToPix(minMark.getY()) - me.getY();
                        minMarkMoved = true;
                    } else if (minMarkType == PAST_INCIDENT) {
                        LcEvent e = minMark.getEvent();
                        double time = e.actual_ms / 60000.0; // Minutes
                        lcCtl.sendToVnmr("lcShowIncident('"
                                         + Fmt.f(3, time, false) + "','"
                                         + e.peakNumber + "')");
                    }
                } else if (minMark != null && right) {
                    if (minMarkType == QUEUED_EVENT) {
                        removeMark(minMark);
                        boolean b = eventQueue.remove(minMark.getEvent());
                        minMark = null;
                        mouseMoved(me); // Possibly select new active mark
                        //validateMarks(true);
                        m_plot.repaint(); // Redo the whole plot!
                    }
                } else {
                    super.mousePressed(me);
                }
            }

            public void mouseReleased(MouseEvent me) {
                if (minMark != null && minMarkType == QUEUED_EVENT
                    && minMarkMoved)
                {
                    LcEvent event = minMark.getEvent();
                    double t = minMark.getX();
                    LcRun run = lcCtl.getLcRun();
                    double t0 = t;
                    if (run != null) {
                        // Don't release an event into the past!
                        t0 = run.getNmrFlowTime();
                        if (t < t0) {
                            t = t0;
                            minMark.setX(t0);
                            m_plot.repaint(); // Redo the whole plot!
                        }
                    }

                    // Finished moving -- reinsert event
                    event.time_ms = (long)(t * 60000);
                    eventQueue.add(event);
                }
                minMarkMoved = false;
                if (minMark == null) {
                    super.mouseReleased(me);
                }
            }

            public void mouseDragged(MouseEvent me) {
                 if (minMark == null || minMarkType != QUEUED_EVENT) {
                    super.mouseDragged(me);
                } else {
                    int mx = me.getX() + xOffset;
                    int my = me.getY() + yOffset;
                    int x = xDataToPix(minMark.getX());
                    int y = yDataToPix(minMark.getY());

                    LcRun run = lcCtl.getLcRun();
                    if (run != null) {
                        // Don't move an event into the past!
                        mx = Math.max(mx, xDataToPix(run.getNmrFlowTime()));
                    }

                    boolean changed = false;
                    if (x != mx) {
                        minMark.setX(xPixToData(mx));
                        minMarkMoved = true;
                        changed = true;
                    }
                    if (y != my && !Double.isInfinite(minMark.getY())) {
                        minMark.setY(yPixToData(my));
                        minMarkMoved = true;
                        changed = true;
                    }
                    if (changed) {
                        m_plot.repaint(); // Redo the whole plot!
                    }
                }
           }
        }
    }


    public class Marker {
        private int width = 9;
        private int height = 8;
        private int xicon = -99;
        private int yicon = -99;
        private double m_x = 0;
        private double m_y = 0;
        private double yPlotted = 0;
        private Color m_color = Color.WHITE;
        private LcEvent event;
        private Color mm_normalColor;
        private Color mm_highlightColor;

        private String imageName = "";
        private Image myImage = null;
        private Image refImage = null;
        private BufferedImage refBufImg = null;
        private int[] refRGBArray;
        private int refGray;

        public Marker(double x, double y, LcEvent lce) {
            this.m_x = x;
            this.m_y = y;
            this.event = lce;
            MarkType type = getEvent().markType;
            boolean clickable = true;
            switch (type) {
            case ELUTION:
            case COLLECTION:
                mm_normalColor = elutionMarkColor;
                clickable = false;
                break;
            case COMMAND:
                mm_normalColor = commandMarkColor;
                clickable = false;
                break;
            case INJECT_ON:
                mm_normalColor = injectOnColor;
                clickable = false;
                break;
            case INJECT_OFF:
                mm_normalColor = injectOffColor;
                clickable = false;
                break;
            default:
                mm_normalColor = normalMarkColor;
                break;
            }
            if (clickable) {
                mm_highlightColor = Util.changeBrightness(mm_normalColor, 20);
            } else {
                mm_highlightColor = mm_normalColor;
            }
            m_color = mm_normalColor;
        }

        public Marker(double x, double y, String imageName, LcEvent lce) {
            this(x, y, lce);
            if (imageName == null || imageName.startsWith("null")) {
                return;
            }
            this.imageName = imageName;
            refImage = refImages.get(imageName);
            if (refImage == null) {
                refImage = Util.getImage(imageName);
                if (refImage == null) {
                    Messages.postError("VLcPlot.Marker: "
                                       + "Cannot load image " + imageName);
                }

                // TODO: Determine if this is needed.
                try {
                    MediaTracker tracker = new MediaTracker(m_plot);
                    tracker.addImage(refImage, 0);
                    tracker.waitForID(0);
                } catch ( Exception e ) {}

                refImages.put(imageName, refImage);
            }
            if (refImage != null) {
                int w = refImage.getWidth(m_plot);
                int h = refImage.getHeight(m_plot);
                if (w <= 0 || h <= 0) {
                    Messages.postError("VLcPlot.Marker: image "
                                       + imageName + " not found");
                    refImage = null;
                    return;
                }
                width = w;
                height = h;
                refBufImg = new BufferedImage(width, height,
                                              BufferedImage.TYPE_INT_ARGB_PRE);
                Graphics2D big = refBufImg.createGraphics();
                big.drawImage(refImage, 0, 0, m_plot);
                refRGBArray = refBufImg.getRGB(0, 0, width, height,
                                               null, 0, width);
                int iMiddle = width / 2 + width * (height / 2);
                Color refBkg = new Color(refRGBArray[iMiddle]);
                refGray = refBkg.getBlue();
                if (refGray < 10 || refGray > 245) {
                    refGray = 128;
                }
                makeNewImage(mm_normalColor);
            }
        }

        private Image makeNewImage(Color color) {
            if (refRGBArray == null) {
                return null;
            }
            Image img = null;
            String strColor = color.toString();
            img = refImages.get(imageName + strColor);
            if (img == null) {
                BufferedImage bufimg = new BufferedImage
                        (width, height, BufferedImage.TYPE_INT_ARGB_PRE);
                //biGraphics = img.createGraphics();
                int len = refRGBArray.length;
                int[] bkgRGBArray = new int[len];
                for (int i = 0; i < len; i++) {
                    int alpha = refRGBArray[i] & 0xff000000;
                    Color thisRefColor = new Color(refRGBArray[i]);
                    int thisGray = refRGBArray[i] & 0xff;
                    int brightness = (100 * (thisGray - refGray)) / refGray;
                    Color thisBkg = Util.changeBrightness(color, brightness);
                    bkgRGBArray[i] = (thisBkg.getRGB() & 0xffffff) | alpha;
                }
                bufimg.setRGB(0, 0, width, height,
                              bkgRGBArray, 0, width);
                refImages.put(strColor, bufimg);
                img = bufimg;
            }
            myImage = img;
            return img;
        }

        /**
         * Get the X value of the marker.
         * @return X coordinate in minutes.
         */
        public double getX() { return m_x; }

        public double getY() { return m_y; }

        public double getYPlotted() { return yPlotted; }

        public LcEvent getEvent() { return event; }

        public void setX(double x) { this.m_x = x; }

        public void setY(double y) { this.m_y = y; }

        public void setYPlotted(double y) { this.yPlotted = y; }

        public int getWidth() { return width;}

        public int getHeight() { return height;}

        public Rectangle getIconRect() {
            return new Rectangle(xicon, yicon, width, height);
        }


        /**
         * Set the mark color.
         * @param c The new color.
         * @return True iff the new color is different from the old color.
         */
        public boolean setColor(Color c) {
            if (m_color != c) {
                m_color = c;
                makeNewImage(c);
                return true;
            } else {
                return false;
            }
        }

        /**
         * Set the mark color.
         * @param type The new type of color.
         * @return True iff the new color is different from the old color.
         */
        public boolean setColor(int type) {
            Color c;
            switch (type) {
            case MOVABLE_COLOR:
                c = queuedMarkColor;
                break;
            case HIGHLIGHT_COLOR:
                c = mm_highlightColor;
                break;
            default:
                c = mm_normalColor;
                break;
            }
            if (m_color != c) {
                m_color = c;
                makeNewImage(c);
                return true;
            } else {
                return false;
            }
        }

        public int getMarkWidth() {
            return 9;           // TODO: user's Mark size?
        }

        public int getMarkHeight() {
            return 9;           // TODO: user's Mark size?
        }

        public boolean isShown() {
            int chan = event.channel - 1;
            if (chan < 0) {
                return true;    // Not associated w/ a channel, always shown
            } else {
                return event != null && m_plot.isShown(2 * chan);
            }
        }

        public void paintMark(Component c, Graphics g, int x, int y) {
            if (event.markType == MarkType.NONE) {
                return;
            }
            g.setColor(m_color);

            xicon = x - width / 2;
            yicon = y - height;
            if (myImage != null) {
                g.drawImage(myImage, xicon, yicon, width, height, m_plot);
            } else {
                // Draw a nabla
                int[] xPoints = new int[3];
                int[] yPoints = new int[3];
                xPoints[0] = x;
                yPoints[0] = y;
                xPoints[1] = x + (width / 2);
                yPoints[1] = y - height + 1;
                xPoints[2] = x - (width / 2);
                yPoints[2] = y - height + 1;
                if (event.markType == MarkType.SLICE) {
                    g.drawPolygon(xPoints, yPoints, 3);
                } else {
                    g.fillPolygon(xPoints, yPoints, 3);
                    // "fill" alone makes a ragged, unsymmetrical outline, so...
                    g.drawPolygon(xPoints, yPoints, 3);
                }
            }
            // Put the peak number (if there is one) on the icon
            // (Need to set the font and center the string on the icon)
            //if (event != null && event.peakNumber > 0) {
            //    g.setColor(Color.BLACK);
            //    g.drawString("" + event.peakNumber, x, y);
            //}
        }
    }
}
