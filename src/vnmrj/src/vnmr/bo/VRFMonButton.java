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
import java.awt.event.*;
import java.beans.*;
import javax.swing.*;
import java.util.*;
import vnmr.ui.*;
import vnmr.util.*;

/**
 * A two-line button that shows RF Monitor data passed by Infostat.
 */
public class VRFMonButton extends VStatusButton 
    implements PropertyChangeListener, MouseListener {

    private static final String[] POWER = {"10s_obspower", "5m_obspower",
                                           "10s_decpower", "5m_decpower",
                                           "10s_ch3power", "5m_ch3power",
                                           "10s_ch4power", "5m_ch4power"};

    private static final String[] LIMIT = {"10s_obslimit", "5m_obslimit",
                                           "10s_declimit", "5m_declimit",
                                           "10s_ch3limit", "5m_ch3limit",
                                           "10s_ch4limit", "5m_ch4limit"};

    private static final int NCHANS = POWER.length;
    private static final int NAVGS = 2; // Nbr of averaging times
    private static final int MAX_POINTS = 5000; // Max data to keep
    private static final int RESOLUTION = 500; // ms
    private static final int PLOT_REFRESH_DELAY = 15000; // ms

    //private static long m_time0 = new Date().getTime();
    private double[] m_power = new double[NCHANS];
    private double[] m_limit = new double[NCHANS];
    private int m_iChan = 0;

    private PowerLog m_powerLog = null;
    private VRFPlot m_vrfplot = null;
    private VPlot.CursedPlot m_vcplot = null;
    private javax.swing.Timer m_plotTimer;
    private Popup m_popupMenu;
    private JButton m_fullButton;
    private JButton m_tailButton;
    private int     showrfmon = 0;

    /**
     * Constructor (for LayoutBuilder)
     */
    public  VRFMonButton(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        m_vrfplot = new VRFPlot(sshare,vif,"chart");
        m_vrfplot.setAttribute(FONT_STYLE, "Menu1");
        m_vrfplot.setAttribute(GRAPHBGCOL, "GraphBackground");
        m_vrfplot.setAttribute(GRAPHFGCOL, "GraphForeground");
        m_vrfplot.setAttribute(GRAPHFGCOL2, "GraphForeground2");
        m_vrfplot.setAttribute(GRAPHFGCOL3, "GraphForeground3");
        m_vrfplot.setAttribute(COLOR4, "GraphForeground4");
        m_vrfplot.setAttribute(GRIDCOLOR, "GraphGrid");
        m_vrfplot.setAttribute(TICKCOLOR, "GraphTick");
        m_vrfplot.setPreferredSize(new Dimension(500, 300));
        m_vcplot = m_vrfplot.getPlot();
        //m_vcplot.setMarksStyle("points");
        m_vcplot.addLegend(0, "10s Avg");
        m_vcplot.addLegend(2, "5m Avg");
        long now = new Date().getTime();
        m_vcplot.setXRange(now - 300000, now); // Range is last 5 minutes
        m_vcplot.addPoint(0, now, 0, false); // Initial point to help scaling
        m_vcplot.setXTimeOfDay(true);
        m_vcplot.setWalking(true, 300000); // Default window width in ms
        m_vcplot.setPointsPersistence(MAX_POINTS); // Keep this many points, max
        m_vcplot.setFillButton(true);
        m_vcplot.setTailButton(true);
        int[] scaley = {0, 2};  // Scale on power only (not limits)
        m_vcplot.setScalingDataSets(scaley);

        m_powerLog = new PowerLog(NCHANS, MAX_POINTS);

        m_popupMenu = new Popup();
        addMouseListener(this);
        
        ActionListener plotTimeout = new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                setValueString();
            }
        };
        m_plotTimer = new javax.swing.Timer(PLOT_REFRESH_DELAY, plotTimeout);
        m_plotTimer.start();

        setToolTipText("Right click selects channel");

        /*
         * For debug
         */
        if (DebugOutput.isSetFor("RFMonData")) {
            Messages.postDebug("Generating fake RF monitor data.");
            ActionListener updateTimeout = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                    updateFakeData();
                }
            };
            updateTimer = new javax.swing.Timer(UPDATE_DELAY, updateTimeout);
            updateTimer.start();
        }
    }

    protected void makeChartFrame() {
        chart_frame = new ChartFrame(m_vrfplot);
        chart_frame.getContentPane().setBackground(Util.getBgColor());
        m_vrfplot.setBackground(Util.getBgColor());
    }

    protected String getPersistenceName() {
        return "RfMonitor";
    }

    /**************************************
     * FOR DEBUG
     * Send fake status data to ExpPanel
     **************************************/
    javax.swing.Timer updateTimer = null;
    private final int UPDATE_DELAY = 1000; // ms
    private double[] pwr10 = new double[4];
    private double[] pwr300 = new double[4];
    private double[] avg10 = new double[10];
    private double[] avg300 = new double[30];
    private int nFakes = 0;
    private int n10 = 0;

    private String pwrStr(double p) {
        return (p < 20)
                ? Fmt.f(1, p)
                : Integer.toString((int)(0.5 + p));
    }

    private void updateFakeData() {
        if (nFakes == 0) {
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[0] + " " + 10);
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[1] + " " + 2);
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[2] + " " + 20);
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[3] + " " + 4);
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[4] + " " + 30);
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[5] + " " + 6);
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[6] + " " + 40);
            ((ExpPanel)vnmrIf).processStatusData(LIMIT[7] + " " + 8);
        }
        int idx10 = nFakes % 10;

        pwr10[0] -= avg10[idx10];
        avg10[idx10] = (nFakes % 120) < 50 ? 0.36 : 0;
        pwr10[0] += avg10[idx10];
        double p = pwr10[0];
        ((ExpPanel)vnmrIf).processStatusData(POWER[0] + " " + pwrStr(p));
        p *= 2;
        ((ExpPanel)vnmrIf).processStatusData(POWER[2] + " " + pwrStr(p));
        p *= 3.0 / 2;
        ((ExpPanel)vnmrIf).processStatusData(POWER[4] + " " + pwrStr(p));
        p *= 4.0 / 3;
        ((ExpPanel)vnmrIf).processStatusData(POWER[6] + " " + pwrStr(p));

        if (nFakes % 10 == 0) {
            // Process 5 minute avg
            int idx30 = n10 % 30;
            pwr300[0] -= avg300[idx30];
            avg300[idx30] = pwr10[0];
            pwr300[0] += avg300[idx30];
            p = pwr300[0] / 30;
            ((ExpPanel)vnmrIf).processStatusData(POWER[1] + " " + pwrStr(p));
            p *= 2;
            ((ExpPanel)vnmrIf).processStatusData(POWER[3] + " " + pwrStr(p));
            p *= 3.0 / 2;
            ((ExpPanel)vnmrIf).processStatusData(POWER[5] + " " + pwrStr(p));
            p *= 4.0 / 3;
            ((ExpPanel)vnmrIf).processStatusData(POWER[7] + " " + pwrStr(p));

            n10++;
        }
        nFakes++;
    }
    /************ END TEST STUFF **************************/
    /******************************************************/

    public void updateStatus(String msg) {
        if (msg == null) 
            return;

        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String key = tok.nextToken();
            String val = "";
            if (tok.hasMoreTokens()) {
                val = tok.nextToken("").trim(); // Get remainder of msg
            }
            //if (key.equals(statkey)) {
            //    state = getState(val);
            //    setState(state);
            //}
            boolean setValue = false;
            // TODO: Could optimize these comparisons
            for (int i = 0; !setValue && i < NCHANS; i++) {
                if (key.equals(POWER[i])) {
                    try {
                        m_power[i] = Double.parseDouble(val);
                        setValue = true;
                    } catch (NumberFormatException nfe) {
                        Messages.postError("Illegal RF Monitor power: " + val);
                    }
                }
            }
            for (int i = 0; !setValue && i < NCHANS; i++) {
                if (key.equals(LIMIT[i])) {
                    try {
                        StringTokenizer toker = new StringTokenizer(val);
                        String slimit = toker.nextToken();
                        m_limit[i] = Double.parseDouble(slimit);
                        if ( (i==1) && slimit.startsWith("-0.0") && 
                             (showrfmon>=0) ) {
                           ExpPanel.setShowRFmon(-1);
                           showrfmon=-1;
                        }
                        if ( (i==1) && ! slimit.startsWith("-0.0") &&
                             (showrfmon<=0) ) {
                           ExpPanel.setShowRFmon(1);
                           showrfmon=1;
                        }
                        setValue = true;
                    } catch (NumberFormatException nfe) {
                        Messages.postError("Illegal RF Monitor limit: " + val);
                    }
                }
            }
            if (setValue) {
                setValueString();
            }
            //if (key.equals(statset)) {
            //    if (val !=null && !val.equals("-")) {
            //        setval = val;
            //        setState(state);
            //    }
            //}
        }
    }

    protected void setTitleString(String s){
        setLine1(s);
        m_vcplot.setTitle(s);
        //chart_frame.setTitle(s);
    }

    /**
     * Set the appearance of the RF Monitor for the current state.
     */
    protected void setState(int s) {
        setValueColor(colors[s]);
        setValueFont(fonts[s]);
        setBackground(Global.BGCOLOR);
        showset=true;
    }

    private void updateChart() {
        double[] time = m_powerLog.getTime();
        int j = 2 * m_iChan;
        m_vcplot.setPoints(0, time, m_powerLog.getPower(j), true, true);
        m_vcplot.setPoints(1, time, m_powerLog.getLimit(j), true, false);
        j++;
        m_vcplot.setPoints(2, time, m_powerLog.getPower(j), true, false);
        m_vcplot.setPoints(3, time, m_powerLog.getLimit(j), true, false);
        m_vcplot.repaint();
    }

    private void setChannel(String chan, boolean on) {
        int iChan = 0;
        try {
            iChan = Integer.parseInt(chan);
        } catch (NumberFormatException nfe) {
            Messages.postDebug("VRFMonButton.setChannel(): Illegal channel: "
                               + chan);
        }
        m_iChan = iChan;
        String title = "RF Watts: Channel " + (iChan + 1);
        setTitleString(title);
        setValueString();
        repaint();
        updateChart();
        if (chart_frame.isVisible()) {
            // If it's trying to be visible, put it on top.
            chart_frame.setVisible(true);
        }
    }

    protected void setValueString() {
        String[] spwr = new String[NAVGS]; // 
        int curState = READYID;
        double time = new Date().getTime();

        m_powerLog.add(time, m_power, m_limit);
        updateChart();
        for (int i = 0; i < NCHANS; i++) {
            if (m_limit[i] > 0 && m_power[i] / m_limit[i] > 0.8) {
                curState = INTERACTIVEID; // Warning font/color
            }
            if (i / NAVGS == m_iChan) {
                int j = i % NAVGS;
                spwr[j] = (m_power[i] < 20)
                        ? Fmt.f(1, m_power[i])
                        : Integer.toString((int)(0.5 + m_power[i]));
                if (m_limit[i] > 0 && m_power[i] >= m_limit[i]) {
                    spwr[i] = "TRIP";
                }
            }
        }
        m_plotTimer.restart();
        if (state != curState) {
            state = curState;
            setState(state);
        }
        setLine2("10s: " + spwr[0] + "   5min: " + spwr[1]);
    }

    /**
     * Get state display string.
     */
    protected String getStateString(int s) {
        return Util.getLabel("sON","On");
    }

    //protected void setToolTip() { }

    public Point getToolTipLocation(MouseEvent event) {
        return new Point(0,0);
        //Point pt = event.getPoint();
        //pt.y = 0;
        //return pt;
    }

    public void mouseClicked(MouseEvent me) {}
    public void mouseEntered(MouseEvent me) {}
    public void mouseExited(MouseEvent me) {}
    public void mousePressed(MouseEvent me) { popUpMenu(me); }
    public void mouseReleased(MouseEvent me) { popUpMenu(me); }

    public void popUpMenu(MouseEvent me) {
        if (me.isPopupTrigger()) {
            int width = m_popupMenu.getWidth();
            int height = m_popupMenu.getHeight();
            if (width <= 0) {
                width = 100;
            }
            if (height <= 0) {
                height = 115;
            }
            m_popupMenu.show(me.getComponent(),
                             me.getX() - width, me.getY() - height);
        }
    }


    class VRFPlot extends VPlot {

        public VRFPlot(SessionShare sshare, ButtonIF vif, String typ) {
            super(sshare, vif, typ);
        }

        /**
         * Override this to make limit lines same color as
         * avg power lines.
         */
        protected void setPlotOptions() {
            super.setPlotOptions();
            int max = 2;        // Number of averaging times
            for (int i = max - 1; i >= 0; --i) {
                Color color = m_plot.getMarkColor(i);
                m_plot.setMarkColor(color, 2 * i);
                m_plot.setMarkColor(color, 2 * i + 1);
            }
        }
    }


    class ChartLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        /**
         * Do the layout
         * @param target component to be laid out
         */
        public void layoutContainer(Container target) {
            final int PAD = 3;
            synchronized (target.getTreeLock()) {
                Dimension targetSize = target.getSize();
                Insets insets = target.getInsets();

                int width = targetSize.width - insets.left - insets.right;
                int height = targetSize.height - insets.top - insets.bottom;
                int x1 = insets.left;
                int y1 = insets.top;

                m_vrfplot.setBounds(x1, y1, width, height);

                Dimension buttonDim = m_fullButton.getPreferredSize();
                int fWd = buttonDim.width;
                int fHt = buttonDim.height;
                int x = x1 + width - fWd - 4;
                int y = y1 + height - fHt - 20;
                m_fullButton.setBounds(x, y, fWd, fHt);

                buttonDim = m_tailButton.getPreferredSize();
                fWd = buttonDim.width;
                fHt = buttonDim.height;
                //x = x1 + width - fWd;
                y -= fHt + 4;
                m_tailButton.setBounds(x, y, fWd, fHt);
            }
        }
    }


    class Popup extends JPopupMenu {
        ActionListener menuListener;
        public Popup() {
            String string;
            //readPersistence();
            menuListener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    JMenuItem item = (JMenuItem)evt.getSource();
                    String s = evt.getActionCommand();
                    setChannel(s, item.isSelected());
                    //writePersistence();
                }
            };

            setLabel("Select Channel");
            JMenuItem menuItem;
            ButtonGroup group = new ButtonGroup();

            menuItem = new JRadioButtonMenuItem("Channel 1");
            menuItem.setActionCommand("0");
            menuItem.addActionListener(menuListener);
            add(menuItem);
            group.add(menuItem);

            menuItem = new JRadioButtonMenuItem("Channel 2");
            menuItem.setActionCommand("1");
            menuItem.addActionListener(menuListener);
            add(menuItem);
            group.add(menuItem);

            menuItem = new JRadioButtonMenuItem("Channel 3");
            menuItem.setActionCommand("2");
            menuItem.addActionListener(menuListener);
            add(menuItem);
            group.add(menuItem);

            menuItem = new JRadioButtonMenuItem("Channel 4");
            menuItem.setActionCommand("3");
            menuItem.addActionListener(menuListener);
            add(menuItem);
            group.add(menuItem);
        }
    }


    class PowerLog {

        private int m_length;
        private int m_nchans;
        private double[] m_time;  // Times of data
        private double[][] m_power; // Power levels vs. channel and time
        private double[][] m_limit; // Limits vs. channel and time
        private int m_i0;       // The first element
        private int m_i1;       // One after the last element (next to write)
        private boolean m_dup = false; // Last two entries are the same.
        private boolean m_empty = true;

        /**
         * @param nchans Number of RF channels.
         * @param length Maximum number of points to keep in the log.
         */
        public PowerLog(int nchans, int length) {
            m_length = length;
            m_nchans = nchans;
            m_time = new double[length];
            m_power = new double[nchans][length];
            m_limit = new double[nchans][length];
            m_empty = true;
            m_i0 = m_i1 = 0;
        }

        public double[] getTime() {
            int size = getSize();
            double[] time = new double[size];
            int idx = m_i0;
            for (int i = 0; i < size; i++, idx = (idx + 1) % m_length) {
                time[i] = m_time[idx];
            }
            return time;
        }

        public double[] getPower(int ichan) {
            int size = getSize();
            double[] power = new double[size];
            int idx = m_i0;
            for (int i = 0; i < size; i++, idx = (idx + 1) % m_length) {
                power[i] = m_power[ichan][idx];
            }
            return power;
        }

        public double[] getLimit(int ichan) {
            int size = getSize();
            double[] limit = new double[size];
            int idx = m_i0;
            for (int i = 0; i < size; i++, idx = (idx + 1) % m_length) {
                limit[i] = m_limit[ichan][idx];
            }
            return limit;
        }

        private boolean isDup(int p1, int p2) {
            boolean dup = true;
            for (int i = 0; dup && i < m_nchans; i++) {
                if (m_power[i][p1] != m_power[i][p2]
                    || m_limit[i][p1] != m_limit[i][p2])
                {
                    dup = false;
                }
            }
            return dup;
        }

        private void compactData(int idx1) {
            int idx2 = getPreviousIndex(idx1);
            int idx3 = getPreviousIndex(idx2);
            if (idx3 >= 0) {
                if (isDup(idx1, idx2) && isDup(idx2, idx3)) {
                    // Put the time of the later one on the earlier one
                    m_time[idx2] = m_time[idx1];
                    // Decrement end of data and copy latest point to idx1
                    m_i1 = getPreviousIndex(m_i1);
                    m_time[idx1] = m_time[m_i1];
                    for (int i = 0; i < m_nchans; i++) {
                        m_power[i][idx1] = m_power[i][m_i1];
                        m_limit[i][idx1] = m_limit[i][m_i1];
                    }
                }
            }
        }

        /**
         * Add data point to the end of the record.
         */
        public void add(double time, double[] power, double[] limit) {
            if (time == 0 || limit[0] == 0) {
                return;         // Not good data
            }

            m_empty = false;
            int prev = getPreviousIndex(m_i1);
            if (prev >= 0 && time - m_time[prev] < RESOLUTION) {
                // Previous point is very recent; just change its data
                for (int i = 0; i < m_nchans; i++) {
                    m_power[i][prev] = power[i];
                    m_limit[i][prev] = limit[i];
                }
            } else {
                // Store new data
                m_time[m_i1] = time;
                for (int i = 0; i < m_nchans; i++) {
                    m_power[i][m_i1] = power[i];
                    m_limit[i][m_i1] = limit[i];
                }
                // Advance end pointer
                m_i1 = (m_i1 + 1) % m_length;
                if (m_i1 == m_i0) {
                    // Out of space, delete first point
                    m_i0 = (m_i0 + 1) % m_length;
                }
                // Previous data is complete, so see if it can be
                // combined with earlier data.
                compactData(prev);
            }
            Messages.postDebug("RFMon",
                               "Data size=" + getSize()
                               + ", power=" + power[0]
                               + ", " + power[1]
                               + ", " + power[2]
                               + ", " + power[3]
                               + ", " + power[4]
                               + ", " + power[5]
                               + ", " + power[6]
                               + ", " + power[7]
                               );/*CMP*/
        }
        /*
        public void add(double time, double[] power, double[] limit) {
            if (time == 0) {
                return;         // Not real data
            }
            // Check if this duplicates the last 2 points
            int prev = getPreviousIndex(m_i1);
            boolean dup = prev >= 0;
            int prev2 = getPreviousIndex(prev);
            m_dup = prev2 >= 0 && isDup(prev, prev2);
            for (int i = 0; dup && i < m_nchans; i++) {
                if (m_power[i][prev] != power[i]
                    || m_limit[i][prev] != limit[i])
                {
                    dup = false;
                }
            }
            if (m_dup && dup) {
                // Data is same as last 2 points; change time on last point.
                m_time[prev] = time;
            } else if (!dup && prev >= 0 && time - m_time[prev] < RESOLUTION) {
                // Previous point is very recent; just change its data
                for (int i = 0; i < m_nchans; i++) {
                    m_power[i][prev] = power[i];
                    m_limit[i][prev] = limit[i];
                }
                m_dup = false;  // (Probably--the safe assumption)
            } else {
                // Need to store new data
                if (!m_empty && m_i0 == m_i1) {
                    m_i0 = (m_i0 + 1) % m_length; // Buf full; drop first point
                }
                // Put in new data
                m_time[m_i1] = time;
                for (int i = 0; i < m_nchans; i++) {
                    m_power[i][m_i1] = power[i];
                    m_limit[i][m_i1] = limit[i];
                }
                m_i1 = (m_i1 + 1) % m_length;
                m_dup = dup;
            }
            m_empty = false;
        }
        */

        public int getSize() {
            if (m_empty) {
                return 0;
            }
            int size = m_i1 - m_i0;
            if (size <= 0) {
                size += m_length;
            }
            return size;
        }

        private int getPreviousIndex(int i) {
            if (i == m_i0 || i < 0) {
                return -1;
            } else {
                return (i + MAX_POINTS - 1) % MAX_POINTS;
            }
        }

        /* For test */
        public String vals(double[] d) {
            int len = d.length;
            String str = "";
            for (int i = 0; i < len; i++) {
                str = str + Fmt.f(2, d[i]) + " ";
            }
            return str;
        }
    }
}

