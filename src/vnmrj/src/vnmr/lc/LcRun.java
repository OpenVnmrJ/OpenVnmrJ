/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.awt.event.*;
import java.io.*;
import java.util.*;

import vnmr.lc.LcEvent.EventTrig;
import vnmr.lc.LcEvent.MarkType;
import vnmr.util.*;


/**
 * This class handles all types of LC-NMR runs.  To execute a run, the
 * constructor is called with the parameters of the run.  The
 * constructor initializes the LC and NMR and starts the run, passing
 * its handle to LcSlimIO.  Events during the run are handled by
 * checking the event queue as each LC data point is acquired from
 * LcSlimIO.
 */
public class LcRun implements LcDef, LcDataListener {
    protected final static int FLOW_OFF = 0;
    protected final static int FLOW_ON = 1;
    protected final static int MAX_LAMBDAS = 2;

    protected double[] thresholds = new double[MAX_TRACES];
    protected LcControl lcCtl;
    protected LcConfiguration config;

    /** Events that we are waiting for. */
    protected SortedSet<LcEvent> eventQueue;

    /** Events that have happened. */
    protected ArrayList<LcEvent> incidentList = new ArrayList<LcEvent>();

    protected LcCurrentMethod method;
    //protected LcEventSchedule schedule;
    protected LcSlimIO slimIO;
    protected VLcPlot.LcPlot m_plot;
    protected LcData m_data;

    protected long[] nTransTime_ms = new long[MAX_TRACES];
    protected String[] channelName = new String[MAX_TRACES];
    protected String[] channelTag = new String[MAX_TRACES];
    protected String[] channelInfo = new String[MAX_TRACES];
    protected boolean holdInProgress = false;
    protected double[] m_lastPeakHeight = new double[MAX_TRACES];
    protected int[] m_lastPeakFound = new int[MAX_TRACES];
    protected int[] aboveThreshold = new int[MAX_TRACES];
    protected int[] belowThreshold = new int[MAX_TRACES];
    protected double[] m_maxYValue = new double[MAX_TRACES];
    protected int[] m_xOfMaxYValue = new int[MAX_TRACES];
    protected boolean[] m_detectorSaturated = new boolean[MAX_TRACES];
    protected long m_dataInterval_ms; // ms
    protected long m_endTime_ms; // Expected length of run
    protected long nextSaveCount = 0;
    protected int m_saveDataPtr = 0;
    protected int m_peakCount = 0; // Running count of peaks w/ NMR data
    protected double m_holdTime_min = 0;
    protected long flowVolume = 0; // nL (nanoliters)
    //protected boolean[] channelActive = new boolean[MAX_TRACES];
    protected boolean[] m_shadowPeak = new boolean[MAX_TRACES];
    protected boolean[] m_shadowTestPeak = new boolean[MAX_TRACES];
    protected double[] m_maxval = new double[MAX_TRACES];
    protected double[] m_minval = new double[MAX_TRACES];
    private float[] m_chromatogramLambdas = null;
    private int[] m_chromatogramTraces;
    private int[] m_chromatogramIndices;
    private boolean[] m_peakEnabled = new boolean[MAX_TRACES];

    protected int m_nEnabledChans;
    protected LcDataWriter m_dataWriter;
    protected String version;
    protected String revdate;
    protected String m_filepath;
    protected String fileFormatRev = "2.1";
    //protected PrintWriter m_pdaWriter = null;
    protected DataOutputStream m_pdaWriter = null;
    protected String m_pdaOutputLock = "";
    /**
     * The type of LC run.  Possibilities are:
     * <br> scout (On Flow NMR)
     * <br> iso (Isocratic On Flow NMR)
     * <br> stop (Stop Flow NMR)
     * <br> enter (Stop Flow NMR w/ selected experiments)
     * <br> analyte (Analyte Collection)
     * <br> lcalone (Test LC)
     * <br> enterA (Analyte Elution)
     */
    protected String m_type;
    protected String m_typeName;
    protected String runTitle;
    protected String m_methodName;
    protected String dateStamp;
    protected long adcStartTime = 0; // Clock time in ms
    protected long m_dataFlowTime_ms = 0; // Updated by appendChannelValues()
    protected long pauseStartTime = 0;
    //protected long timePaused = 0; // Time spent w/ flow off in this run
    protected long previousSegment_ms = 0;
    protected javax.swing.Timer restartTimer;
    protected ActionListener restartAction;
    protected javax.swing.Timer holdTimer;
    protected long holdStopTime = 0;
    protected ActionListener holdCountAction;
    protected double lastSliceTime = 0;
    protected long refTime_ms = 0;
    protected long m_uvOffset_ms = 0;
    protected boolean didEndRun = false;
    private boolean m_waitingForInjection = false;

    public boolean flowPaused = false;
    public boolean runActive = false;


    /**
     * Plain constructor needed by LcFileRun.
     */
    public LcRun() {
    }

    /**
     * Normal constructor to start a new LC run.
     */
    public LcRun(LcControl lcc,
                 LcConfiguration conf,
                 SortedSet<LcEvent> queue,
                 LcCurrentMethod method,
                 //LcEventSchedule schedule,
                 LcSlimIO io,
                 VLcPlot.LcPlot plot,
                 String version,
                 String revdate,
                 String path,
                 String basename,
                 String type) {

        Messages.postDebug("LcRun", "LcRun.<init> starting");

        lcCtl = lcc;
        config = conf;
        eventQueue = queue;
        this.method = method;
        //this.schedule = schedule;
        this.slimIO = io;
        this.version = version;
        this.revdate = revdate;
        this.m_filepath = path + File.separator + basename;
        this.m_type = type;
        m_typeName = getRunTypeString();
        sendToVnmr("lcDataFile='" + m_filepath + "'");
        sendToVnmr("lcRunLogFile='" + m_filepath + ".html'");

        // This timer is for timed holds
        restartAction = new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                restartRun();
            }
        };
        // Initialize w/ fake interval, actually set when started
        restartTimer = new javax.swing.Timer(5000, restartAction);
        restartTimer.setRepeats(false);

        // This timer updates the status during a hold
        holdCountAction = new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                long now = System.currentTimeMillis();
                double time = (now - holdStopTime) / 60000.0;
                if (!flowPaused || lcCtl.getLcRun() == null) {
                    holdTimer.stop();
                    time = 0;
                }
                Messages.postDebug("holdCounter",
                                   "LcRun.holdCountAction: incrementing time");
                sendToVnmr("lcHoldCount=" + time);
            }
        };
        final int delay_ms = 600; // Update every 0.01 minute
        holdTimer = new javax.swing.Timer(delay_ms, holdCountAction);
        holdTimer.setInitialDelay(0);

        m_dataInterval_ms = (int)(method.getAdcPeriod() * 1000);
        m_endTime_ms = (int)(method.getEndTime() * 60000);

        //
        // Set up the plotting
        //
        lcCtl.configGraphPanel(this);
        this.m_plot = plot;
        initPlot();
        m_plot.markEvents();
        double vol = conf.getNmrProbeVolume();
        if (m_type != null && m_type.equals("analyte")) {
            vol = conf.getLoopVolume();
        }
        m_plot.setHighlightVolume(vol);
        //m_plot.removeLegendButtons(); // New ones get set up below

        //
        // Initialize channel data.
        //
        for (int i = 0; i < MAX_TRACES; i++) {
            m_maxYValue[i] = 0; // For peak detection
            m_xOfMaxYValue[i] = 0; // For peak detection
            m_maxval[i] = 0;
            m_minval[i] = 0;
        }
        m_nEnabledChans = 0;
        double[] xferTimes_sec = config.getTransferTimes();
        if (DebugOutput.isSetFor("lcTiming")) {
            String msg = "TransferTimes={";
            for (int i = 0; i < xferTimes_sec.length; i++) {
                if (i > 0) {
                    msg += ", ";
                }
                msg += Fmt.f(2, xferTimes_sec[i]);
            }
            msg += "} sec";
            Messages.postDebug(msg);
        }
        refTime_ms = (long)(config.getReferenceTime() * 1000);
        Messages.postDebug("lcTiming",
                           "refTime=" + Fmt.f(2, (refTime_ms / 1000.0)) + " s");
        int[] activeAdcs = new int[MAX_TRACES];
        for (int i = 0; i < MAX_TRACES; i++) {
            activeAdcs[i] = -1; // Not active
        }
        int nActiveAdcs = 0;
        float[] monitorLambdas = new float[MAX_LAMBDAS];
        int[] monitorLambdaTraces = new int[MAX_LAMBDAS];
        for (int i = 0; i < MAX_LAMBDAS; i++) {
            monitorLambdas[i] = 0; // Not active
            monitorLambdaTraces[i] = -1; // Not active
        }
        for (int i = 0; i < MAX_TRACES; i++) {
            m_shadowPeak[i] = false;
            thresholds[i] = method.getThreshold(i, 0);
            nTransTime_ms[i] = (long)(xferTimes_sec[i] * 1000) - refTime_ms;
            Messages.postDebug("lcTransferTimes",
                               "LcRun.<init>: nTransTime_ms[" + i + "]="
                               + nTransTime_ms[i]
                               + ", threshold=" + Fmt.f(2, thresholds[i]));
            channelTag[i] = config.getChannelTag(i);
            String name = config.getChannelName(i);
            channelName[i] = name;
            Messages.postDebug("lcLabels", "Chan " + i + " name: " + name);
            String tag = config.getChannelTag(i);
            if (tag != null && tag.indexOf(":uv1") >= 0) {
                if (monitorLambdaTraces[0] >= 0) {
                    Messages.postError("Can not have 2 traces showing "
                                       + "the same data");
                    return;
                }
                monitorLambdas[0] = method.getLambda();
                monitorLambdaTraces[0] = i; // Get first lambda from trace i
                channelInfo[i] = (int)monitorLambdas[0] + " nm";
                m_uvOffset_ms = nTransTime_ms[i];
            } else if (tag != null && tag.indexOf(":uv2") >= 0) {
                if (monitorLambdaTraces[1] >= 0) {
                    Messages.postError("Can not have 2 traces showing "
                                       + "the same data");
                    return;
                }
                monitorLambdas[1] = method.getLambda2();
                monitorLambdaTraces[1] = i; // Get second lambda from trace i
                channelInfo[i] = (int)monitorLambdas[1] + " nm";
                m_uvOffset_ms = nTransTime_ms[i];
            } else {
                // NB: Zero length string may cause problems parsing data
                channelInfo[i] = " "; // TODO: channel info for MS, other
            }

            // Look for a ":adcN" tag
            int ix;
            if (tag != null && (ix = tag.indexOf(":adc")) >= 0) {
                int iadc = -1;
                try {
                    // The "tag" numbers ADCs from 1; we start at 0
                    iadc = Integer.parseInt(tag.substring(ix + 4, ix + 5)) - 1;
                } catch (NumberFormatException nfe) {
                }
                activeAdcs[i] = iadc; // i'th trace comes from ADC # iadc
                nActiveAdcs++;
            }
                
            Messages.postDebug("lcLabels",
                               "ChanInfo " + i + ": " + channelInfo[i] );
            if (config.isTraceEnabled(i)) {
                m_nEnabledChans++;
                try {
                    m_plot.addLegendButton(i, 2 * i, "" + (char)('1' + i),
                                           channelName[i]);
                    m_plot.addLegendButton(i, 2 * i + 1, null);
                } catch (Exception e) {
                    Messages.postDebug("Error setting legend button");
                    Messages.writeStackTrace(e);
                }
            }
        }

        // Initialize UV data collection
        lcCtl.setUvDataListener(this);
        if (lcCtl.setUvChromatogramChannels(monitorLambdas,
                                            monitorLambdaTraces,
                                            this)
            == false)
        {
            // This detector doesn't send separate chromatogram data.
            // We will extract it ourselves from any spectra we may get.
            setUvChromatogramChannels(monitorLambdas, monitorLambdaTraces);
        }

        // Initialize the SLIM data collection
        slimIO.setActiveChannels(activeAdcs);

        // Calculate the minimum time between data points
        int[] dt = new int[2];// Only two possible data sources so far
        dt[0] = (nActiveAdcs == 0) ? 0 : (int)m_dataInterval_ms;// ADC data rate
        dt[1] = lcCtl.getUvDataInterval();
        int dataInterval = 0;
        for (int i = 0; i < dt.length; i++) {
            if (dataInterval == 0 || (dt[i] > 0 && dataInterval > dt[i])) {
                dataInterval = dt[i];
            }
        }

        int[][] detTraces = {monitorLambdaTraces, };
        int[] traceTable = LcData.getTraceToColumnTable(detTraces, activeAdcs);
        int nActive = LcData.countActiveTraces(traceTable);
        m_data = new LcData(null, traceTable, null);

        // Start up the DataWriter
        String datapath = getFilepath() + ".data";
        m_dataWriter = new LcDataWriter(m_data, dataInterval, datapath);

        // Don't do auto-scaling on threshold lines.
        int[] scalingTraces = new int[m_nEnabledChans];
        for (int i = 0, j = 0; i < MAX_TRACES && j < m_nEnabledChans; i++) {
            if (config.isTraceEnabled(i)) {
                scalingTraces[j++] = 2 * i;
            }
        }
        m_plot.setScalingDataSets(scalingTraces);

        // Title uses the last component of the dirpath
        File fPath = new File(path);
        runTitle = fPath.getName();
        Messages.postDebug("LcRun", "runTitle=" + runTitle);

        dateStamp = Util.getStandardDateTimeString(System.currentTimeMillis());

        saveData();
        m_methodName = method.getName();
        setPlotTitle();

        flowPaused = true;
        initializeRunParameters();
        initializeHardware();
        restartRun();
    }

    public boolean isActive() {
        return runActive;
    }

    protected void initPlot() {
        m_plot.initialize(eventQueue, incidentList);
    }

    /** Not recommended */
    public void setDelay(int chan, double delay_s) {
        nTransTime_ms[chan] = (long)(delay_s * 1000);
    }

    public String getDayDateStamp() {
        String str = dateStamp;
        int i = dateStamp.indexOf(' ');
        if (i > 0) {
            str = str.substring(0, i);
        }
        return str;
    }

    public void setPlotTitle() {
        if (m_plot != null) {
            m_plot.setTitle(getDayDateStamp()
                          + "  " + runTitle
                          + "  \"" + m_methodName + "\"");
        }
    }

    public String getFilepath() {
        return m_filepath;
    }

    public String getDirpath() {
        String dir = new File(m_filepath).getParent();
        return (dir == null) ? "." : dir;
    }

    /**
     * Return the first channel that is connected to Mass Spec.
     * @return Channel number (starting at 0).  Returns -1 if no
     * Mass Spec channel is found.
     */
    public int getMsChannel() {
        for (int i = 0; i < MAX_TRACES; i++) {
            if (channelName[i] != null
                && (channelName[i].toLowerCase().indexOf("ms") >= 0)) {
                return i;
            }
        }
        return -1;
    }        

    /**
     * This glues a new run with a new method to the current method,
     * starting at the current flowTime.
     * Modules start new runs, so the event times for module methods
     * differ from the event times for us by the current flowTime.
     */
    public void restartRunAppend() {
        method.preloadEventQueue(eventQueue, getDataFlowTime());
        m_plot.markEvents();
        initializeRunParameters();
        initializeHardware();
        m_endTime_ms = m_dataFlowTime_ms + (int)(method.getEndTime() * 60000);
        restartRun();
    }

    protected void initializeHardware() {
        String msMethod = method.getInitialMsMethod();
        if (msMethod != null
            && (msMethod = msMethod.trim()).length() != 0
            && !msMethod.equals("null"))
        {
            sendToVnmr("msSetMethod('init', 'save', '" + msMethod + "')");
        }
    }

    protected void initializeRunParameters() {
        sendToVnmr("lcNumberOfIncidents=0");
        for (int i = 0; i < MAX_TRACES; i++) {
            double thresh = method.getThreshold(i, 0);
            sendToVnmr("lcThreshold[" + (i + 1) + "]=" + thresh);
            boolean enb = (config.isTraceEnabled(i)
                           && method.isPeakEnabled(i, 0));
            String sEnb = enb ? "'y'" : "'n'";
            sendToVnmr("lcPeakDetect[" + (i + 1) + "]=" + sEnb);
        }
        sendToVnmr("lcRunActive=1");
    }

    /**
     * Stop the current run and store data.
     */
    public void stop() {
        Messages.postDebug("LcRun", "LcRun.stop(): runActive=" + runActive);
        //Messages.writeStackTrace(new Exception("DEBUG"));/*DBG*/
        if (runActive) {
            // Do standard EndRun command, if not done before in method
            if (!didEndRun) {
                sendToVnmr("EndRun('VJ')");
                didEndRun = true;
            }
            stopChromatogram(); // Turns off runActive
        } else {
            Messages.postWarning("No LC run is active");
        }
    }

    /**
     * Stop the current run and store data.
     */
    public void stopChromatogram() {
        Messages.postDebug("LcRun", "LcRun.stopChromatogram(): runActive="
                           + runActive);
        if (runActive) {
            runActive = false;
            previousSegment_ms += System.currentTimeMillis() - adcStartTime;
            restartTimer.stop();
            holdTimer.stop();
            sendToVnmr("lcHoldCount=" + 0);
            lcCtl.stopPda();
            lcCtl.resetPda();
            if (DebugOutput.isSetFor("fakeChromatogram")) {
                slimIO.stopAdc();
            }
            slimIO.closeDataWriter();
            saveData();
            writeLcdataFile();

            m_plot.repaint();
            Messages.postInfo("Run stopped: " + m_data.getSize(0)
                              + " SLIM points saved");

            sendToVnmr("lcRunActive=0");
            // Do special end-of-run stuff, if requested
            String cmd = method.getPostRunCommand();
            if (cmd != null && (cmd = cmd.trim()).length() > 0) {
                sendToVnmr(cmd);
            }
        }
    }

    /**
     * Stop the current run and pump and store data. Can be restarted later.
     */
    public void pauseRun() {
        if (flowPaused) {
            return;
        }
        setValves(FLOW_OFF, m_type);
        sendToVnmr("pmlcmd('collect_pause')");
        pauseChromatogram();
        //Messages.postInfo("Run paused: " + data.size() + " points, so far");
    }

    /**
     * Stop the current run and store data. Can be restarted later.
     */
    public void pauseChromatogram() {
        if (flowPaused) {
            return;
        }
        flowPaused = true;
        lcCtl.pauseUv();
        if (DebugOutput.isSetFor("fakeChromatogram")) {
            slimIO.stopAdc();
        }
        previousSegment_ms += System.currentTimeMillis() - adcStartTime;
        Messages.postDebug("lcTiming", "pauseRun(): previousSegment_ms="
                           + previousSegment_ms);
        saveData();
    }

    /**
     * Gets how long pump has been pumping (in ms).  This is based on the
     * time of the current data being processed, not on the actual time.
     */
    private long getDataFlowTime_ms() {
        return m_dataFlowTime_ms;
    }

    /**
     * Gets how long pump has been pumping (in minutes).   This is based on the
     * time of the current data being processed, not on the actual time.
     */
    private double getDataFlowTime() {
        return m_dataFlowTime_ms / 60000.0;
    }

    /**
     * Gets how long pump has been pumping (in ms).  This is
     * based on the actual current position of the solvent, not the
     * time of the current data being processed.
     */
    public long getFlowTime_ms() {
        long t = previousSegment_ms;;
        if (!flowPaused) {
            long now = System.currentTimeMillis();
            t += now - adcStartTime;
        }
        return t;
    }

    /**
     * Gets how long pump has been pumping (in minutes).  This is
     * based on the actual current position of the solvent, not the
     * time of the current data being processed.
     */
    public double getFlowTime() {
        return getFlowTime_ms() / 60000.0;
    }

    /**
     * Gets location of NMR on chromatogram (in ms).  This is
     * based on the actual current position of the solvent, not the
     * time of the current data being processed.
     * (In loop collection, the location of the loop.)
     */
    public long getNmrFlowTime_ms() {
        long t = previousSegment_ms;;
        if (!flowPaused && runActive) {
            long now = System.currentTimeMillis();
            t += now - adcStartTime;
        }
        return t - refTime_ms;
    }

    /**
     * Gets location of NMR on chromatogram (in minutes).  This is
     * based on the actual current position of the solvent, not the
     * time of the current data being processed.
     * (In loop collection, the location of the loop.)
     */
    public double getNmrFlowTime() {
        return getNmrFlowTime_ms() / 60000.0;
    }

    /**
     * Get the offset of the UV/PDA channel in ms. This is the time
     * displayed on the chromatogram minus the time in the data set.
     */
    public long getUvOffset_ms() {
        return m_uvOffset_ms;
    }

    public void holdNow() {
        long time_ms = getNmrFlowTime_ms();
        double time_min = time_ms / 60000.0;
        //String cmd = getPeakAction(); // Default action for run type
        //Object oCmd = method.getValue("lcPeakCommand", time_min);
        //if (oCmd instanceof String) {
        //    String sCmd = (String)oCmd;
        //    if (sCmd != null && sCmd.trim().length() > 0) {
        //        cmd = sCmd;     // Method action overrides default
        //    }
        //}
        String cmd = getPeakAction(time_min);
        Messages.postDebug("LcEvent", "holdNow: cmd=" + cmd);
        LcEvent lcEvent = new LcEvent(time_ms, 0, 0,
                                      lcCtl.getLoop(),
                                      EventTrig.USER_HOLD, MarkType.HOLD,
                                      cmd);
        lcEvent.peakNumber = -1;
        eventQueue.add(lcEvent);
        m_plot.addMark(time_min, HOLD_Y_POSITION, lcEvent);
        m_plot.repaint();
    }

    /**
     * Deals with data from the UV spectrometer.
     * This method is responsible for:
     *   (1) appending data to the data array;
     *   (2) signaling updates to the chart;
     *   (3) checking the event table;
     *   (4) tracking the maximum ADC value.
     * @param datums An array of LcDatum values.  For now, assume all
     * have the same run time.
     */
    public void appendChannelValues(LcDatum[] datums) {
        int nChans = datums.length;
        if (nChans == 0 || datums[0] == null) {
            return;
        }

        // GMT time of this data.
        long clockTime_ms = System.currentTimeMillis();

        Messages.postDebug("lcTiming",
                           "appendChannelValues(): previousSegment="
                           + Fmt.f(3, previousSegment_ms / 60000.0) + " min");

        // TODO: Handle events in separate thread?
        // Handle any events for the current time.
        // NB:  Assume all times the same
        long segment_ms = m_dataFlowTime_ms = datums[0].getRunTime();
        //m_dataFlowTime_ms = segment_ms + previousSegment_ms;
        checkEvents(getNmrFlowTime_ms());

        double tMinutes = m_dataFlowTime_ms / 60000.0;
        //m_plot.setFlowTime(tMinutes);
        Messages.postDebug("lcTiming",
                           "appendChannelValues(): segment_ms=" + segment_ms
                           + ", flowTime=" + Fmt.f(4, tMinutes) + " min"
                           + " = " + m_dataFlowTime_ms + " ms");

        //
        // Plot the data
        //
        for (int j = 0; j < nChans; j++) {
            int trace = datums[j].getChannel();
            if (config.isTraceEnabled(trace)) {
                double chanTime = tMinutes + nTransTime_ms[trace] / 60000.0;
                double val = datums[j].getValue();
                m_plot.addPoint(2 * trace, chanTime, val, true, false);

                // NB: thresh == -1 iff peak-finding is turned off
                double thresh = method.getThreshold(trace, tMinutes);
                // See if this point is peakEnabled.
                boolean peakEnb = (config.isTraceEnabled(trace)
                                   && method.isPeakEnabled(trace, tMinutes));
                // See if previous point was also peakEnabled.
                boolean connect = peakEnb && m_peakEnabled[trace];
                // Threshold point is added, but will not be visible unless
                // it is connected to the previous point (there is no "mark").
                m_plot.addPoint(2*trace + 1, chanTime, thresh, connect, false);
                m_peakEnabled[trace] = peakEnb;
            }
        }
        m_plot.repaint();

        m_data.add(datums);
        m_dataWriter.appendData(datums);

        //
        // Check the tail of these channels for peaks.
        //
        for (int i = 0; i < datums.length; i++) {
            int c = datums[i].getChannel();
            m_detectorSaturated[c] = fitForPeak(c, m_data.getSize(c), true,
                                                m_data.getMax(c),
                                                m_detectorSaturated[c]);
        }

        // A second check, in case we just put in a peak it's time to do.
        checkEvents(getNmrFlowTime_ms());

        if ( m_dataFlowTime_ms >= m_endTime_ms ) {
            stop();
        }
    }


    // ************ LcDataListener Interface *************

    /**
     * Helper method opens or reopens the data file.
     * Opens a DataOutputStream, which writes the binary data in a
     * machine-independent way (BigEndian, IEEE floats).
     */
    private DataOutputStream openPdaDataFile(boolean append) {
        synchronized (m_pdaOutputLock) {
            DataOutputStream dos = null;
            File file = new File(getDirpath() + "/pdaData.dat");
            if (!append) {
                file.delete();
            }
            try {
                Messages.postDebug("PdaWriter", "openPdaDataFile(): " + file
                                   + ", append=" + append);
                dos = new DataOutputStream(new FileOutputStream(file , append));
            } catch (FileNotFoundException fnfe) {
                Messages.postError("Cannot write PDA Data file: "
                                   + file.getPath());
                dos = null;
            }
            return dos;
        }
    }

    /**
     * Opens the PDA data file and writes out the wavelengths line.
     * Called before any data is sent to define the wavelenths that
     * apply to all the subsequent data.  This is essentially also
     * the signal that a new data set is coming.
     * <p> Format of output file is binary data (in network byte order).
     * <br> int Number_of_wavelengths
     * <br> float Wavelength (repeated Number_of_wavelengths times)
     * <br><i> The following are repeated for each spectrum: </i>
     * <br> long Time_ms
     * <br> float Absorption (repeated Number_of_wavelengths times)
     *
     * @param wavelengths An array equal in length to the number of
     * points in each spectrum, giving the wavelength in nm of each point.
     */
    public void setUvWavelengths(float[] wavelengths) {
        if (m_chromatogramLambdas != null) {
            setChromatogramIndices(wavelengths);
        }
        lcCtl.setPdaPlotWavelengths(wavelengths);
        synchronized (m_pdaOutputLock) {
            boolean append = false;
            Messages.postDebug("PdaWriter",
                               "setUvWavelengths(): opening pdaWriter");
            m_pdaWriter = openPdaDataFile(append);
            try {
                if (m_pdaWriter != null) {
                    m_pdaWriter.writeInt(wavelengths.length);
                    for (int i = 0; i < wavelengths.length; i++) {
                        m_pdaWriter.writeFloat(wavelengths[i]);
                    }
                }
                m_pdaWriter.flush();
            } catch (IOException ioe) {
                Messages.postError("LcRun.setUvWavelengths(): "
                                   + "cannot write data file");
            }
        }
    }

    /**
     * Writes a spectrum into the PDA data file.
     * Called whenever a new spectrum is available.
     * @param time The run time in minutes of the spectrum.
     * @param spectrum An array equal in length to the number of
     * wavelengths sent earlier, giving the absorption in AU at
     * each wavelength.
     */
    public void processUvData(double time, float[] spectrum) {
        if (m_chromatogramLambdas != null) {
            // We need to extract the absorption at these lambdas
            // for the chromatogram.
            extractChromatogramData(time, spectrum);
        }

        // Show the current spectrum (t = current UV time on chromatogram)
        double uvTime = time + getUvOffset_ms() / 60000.0;
        lcCtl.setPdaPlotData(uvTime, spectrum);

        // Write out the data (t = 0 at first point)
        synchronized (m_pdaOutputLock) {
            if (m_pdaWriter == null) {
                boolean append = true;
                Messages.postDebug("PdaWriter",
                                   "processUvData(): opening pdaWriter");
                m_pdaWriter = openPdaDataFile(append);
            }
            float run_min = (float)time;
            try {
                if (m_pdaWriter != null) {
                    // NB: Times in the file are in minutes
                    m_pdaWriter.writeFloat(run_min);
                    int n = spectrum.length;
                    for (int i = 0; i < n; i++) {
                        m_pdaWriter.writeFloat(spectrum[i]);
                    }
                    Messages.postDebug("PdaWriter", "processUvData(): "
                                       + "wrote " + n + " points");
                }
                m_pdaWriter.flush();
            } catch (IOException ioe) {
                Messages.postError("LcRun.processUvData(): "
                                   + "cannot write data file");
            }
        }
    }

    /**
     * Called after the last spectrum has been sent.
     * Closes the data file.
     */
    public void endUvData() {
        synchronized (m_pdaOutputLock) {
            Messages.postDebug("PdaWriter", "endUvData(): closing pdaWriter");
            try {
                m_pdaWriter.close();
            } catch (IOException ioe) {
            }                
            m_pdaWriter = null;
        }
    }

    // ************ END LcDataListener Interface *************
    

    /**
     * Calculates which data in a spectrum
     * correspond to the desired chromatogram wavelengths.
     * @param spectrumLambdas The wavelength of each element in a spectrum.
     */
    private void setChromatogramIndices(float[] spectrumLambdas) {
        int n = m_chromatogramLambdas.length;
        int m = spectrumLambdas.length;
        m_chromatogramIndices = new int[n];
        for (int i = 0; i < n; i++) {
            m_chromatogramIndices[i]
                    = Util.getNearest(spectrumLambdas, m_chromatogramLambdas[i]);
        }
    }

    /**
     * Selects channels and wavelengths to monitor for the chromatogram.
     * @param lambdas The wavelengths to monitor.
     * @param traces The corresponding traces to send them to.
     */
    public boolean setUvChromatogramChannels(float[] lambdas, int[] traces) {
        // Count number of valid lambdas
        int nLambdas = 0;
        for (int i = 0; i < lambdas.length; i++) {
            if (lambdas[i] > 0) {
                nLambdas++;
            }
        }
        if (nLambdas != lambdas.length || nLambdas != traces.length) {
            m_chromatogramLambdas = new float[nLambdas];
            m_chromatogramTraces = new int[nLambdas];
            int j = 0;
            for (int i = 0; i < lambdas.length; i++) {
                if (lambdas[i] > 0) {
                    m_chromatogramLambdas[j] = lambdas[i];
                    m_chromatogramTraces[j] = traces[i];
                    j++;
                }
            }
        } else {
            m_chromatogramLambdas = lambdas;
            m_chromatogramTraces = traces;
        }
        return true;
    }

    /**
     * Extract the values at the chromatogram wavelengths from PDA data
     * and send them to the chromatogram.  Does nothing if the run is
     * inactive (paused or stopped).
     */
    private void extractChromatogramData(double time, float[] data) {
        if (isActive()) {       // Don't do it if run is over!
            long time_ms = (long)(time * 60000);
            long now = System.currentTimeMillis();
            int n = Math.min(m_chromatogramIndices.length,
                             m_chromatogramTraces.length);
            LcDatum[] datums = new LcDatum[n];
            for (int i = 0; i < n; i++) {
                datums[i] = new LcDatum(m_chromatogramTraces[i],
                                        time_ms,
                                        now,
                                        data[m_chromatogramIndices[i]]);
            }
            appendChannelValues(datums);
        }
    }

    /**
     * Contatenate the .hdr file and the .data file to make an lcdata file.
     */
    public void writeLcdataFile() {
        String hdr = getFilepath() + ".hdr";
        String data = getFilepath() + ".data";
        String[] src = {hdr, data};
        String lcdata = getDirpath() + "/lcdata";
        FileUtil.cat(src, lcdata);
        sendToVnmr("shell('chmod 444 " + lcdata + "')"); // Don't mess with it
    }

    /**
     * Get whether a given trace number is enabled in the current method.
     * The answer is returned as a string suitable for the LC data file
     * header.
     * This is not allowed to change during the run.
     * @param trace The trace number, starting from 0.
     * @return The string "#TRUE#" or "#FALSE#".
     */
    private String isTraceEnabledString(int trace) {
        return config.isTraceEnabled(trace) ? "#TRUE#" : "#FALSE#";
    }

    /**
     * Saves the Chromatogram data in a file.
     */
    synchronized protected void saveData() { // TODO: Obsolete saveData()
        //String s = "Saving data: ";
        //s += Calendar.getInstance().getTime().toString();
        //s += " in " + m_filepath + ".hdr";
        //Messages.postInfo(s);

        // Open file
        BufferedWriter fw = null;
        try {
            fw = new BufferedWriter(new FileWriter(m_filepath + ".hdr"));
        } catch (IOException ioe) {
            Messages.postError("Cannot open LC Data file: "
                               + m_filepath + ".hdr");
            return;
        }
        PrintWriter pw = new PrintWriter(fw);

        // Write header
        pw.println("; File format version: LCD " + fileFormatRev);
        pw.println("; Run start time:\n" + dateStamp);
        pw.println("; Program version:\n" + version);
        pw.println("; Program date:\n" + revdate);
        pw.println("; Run Title:\n" + runTitle);
        pw.println("; Method:\n" + m_methodName);
        pw.println("; Star text file name:\n" + ""); // TODO: file name
        pw.println("; Run type:\n" + m_typeName);
        pw.println("; flow rate, ml/min:\n" + Fmt.f(4, method.getInitialFlow(),
                                                    false));
        pw.println("; Number of channels:\n" + m_nEnabledChans);
 
        pw.println("; Trace numbers:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(",");
                }
                pw.print("" + (1 + i));
                j++;
            }
        }
        pw.println();

        // NB: All channels with data are active, so write all "trues"!
        pw.println("; Active channels:");
        for (int i = 0; i < m_nEnabledChans; i++) {
            if (i > 0) {
                pw.print(",");
            }
            pw.print("#TRUE#");
        }
        pw.println();

        pw.println("; Detection thresholds:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(",");
                }
                pw.print(Fmt.f(4, method.getThreshold(i, 0), false));
                j++;
            }
        }
        pw.println();

        pw.println("; Transfer times:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(",");
                }
                pw.print(Fmt.f(4, (refTime_ms + nTransTime_ms[i]) / 60000.0,
                            false));
                j++;
            }
        }
        pw.println();

        pw.println("; Reference time:\n"
                   + Fmt.f(4, refTime_ms / 60000.0, false));

        pw.println("; Channel info:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(", ");
                }
                pw.print(channelInfo[i]);
                j++;
            }
        }
        pw.println();

        pw.println("; Channel names:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(", ");
                }
                pw.print(channelName[i]);
                j++;
            }
        }
        pw.println();

        pw.println("; Channel tags:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(", ");
                }
                pw.print(channelTag[i]);
                j++;
            }
        }
        pw.println();

        pw.println("; Channel maximum values:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(",");
                }
                pw.print(Fmt.f(4, m_data.getMax(i), false));
                j++;
            }
        }
        pw.println();

        pw.println("; Channel minimum values:");
        for (int i = 0, j = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                if (j > 0) {
                    pw.print(",");
                }
                pw.print(Fmt.f(4, m_data.getMin(i), false));
                j++;
            }
        }
        pw.println();

        pw.println("; delta time:\n"
                   + (m_dataWriter.getDataInterval() / 60000.0));
        pw.println("; Chart title:\n"
                   + "");         // TODO: get user-specified chart title.
        pw.println("; Run notes:"); // TODO: get run-notes??
        for (int i = 0; i < 10; ++i) {
            pw.println("|");
        }
        //synchronized(incidentList) {
            pw.println("; Incident table:\n" + incidentList.size());
            for (LcEvent event: incidentList) {
                pw.println(event.channel + ","
                           + Fmt.f(4, event.time_ms / 60000.0, false) + ","
                           + Fmt.f(4, event.delay_ms / 60000.0, false) + ","
                           + Fmt.f(4, event.actual_ms / 60000.0, false) + ","
                           + Fmt.f(4, event.length_ms / 60000.0, false) + ","
                           + event.loop + ","
                           + event.trigger.label() + ","
                           + event.action);
            }
        //}
        pw.println("; label table:\n" + 0); // TODO: Implement plot labels

        // Tag for data - data actually written below
        pw.println("; data:");

        // Close file
        pw.close();

        // Put in the actual data too!
        m_saveDataPtr = 0;
        saveLog();
    }

    /**
     * Convert from flow time to time in MS data file.
     * Flow time is relative to the chosen reference channel.
     * Correct this for:
     * 1) Time flow is paused (MS clock keeps running).
     * 2) Delay between command sent and start of MS data collection.
     * 3) Offset between reference channel and MS channel.
     */
    public int correctMsRunTime(int time_ms) {

        // Calcualte channel offset
        int iChan = getMsChannel();
        int msToRef_ms = 0;
        int msToNmr_ms = 0;
        if (iChan >= 0) {
            Messages.postDebug("msRunTime",
                               "Correct MS offset: time=" + time_ms
                               + ", msTrans=" + nTransTime_ms[iChan]
                               + ", refTime=" + refTime_ms);
            msToRef_ms = (int)nTransTime_ms[iChan];
            msToNmr_ms = (int)(msToRef_ms + refTime_ms);
        }

        // Calculate extra time for pauses
        int n = incidentList.size();
        int pause_ms = 0;
        Messages.postDebug("msRunTime", "LcRun.correctMsRunTime: "
                           + "number incidents = " + incidentList.size());
        for (LcEvent event: incidentList) {
            Messages.postDebug("msRunTime", "LcRun.correctMsRunTime: "
                               + "event time=" + event.actual_ms
                               + ", length=" + event.length_ms);
            // time_ms must be later than eventTime by msToNmr xfer time
            if (event.actual_ms > (time_ms - msToNmr_ms)) {
                break;
            } else {
                pause_ms += event.length_ms;
            }
        }

        // Start delay for MS
        int startDelay_ms = (int)(config.getMsSyncDelay() * 1000);

        // Total correction
        int msTime_ms = time_ms + pause_ms - startDelay_ms - msToRef_ms;

        // Diagnostic output
        double msTime = (msTime_ms) / 60000.0;
        double dispTime = (time_ms) / 60000.0;
        Messages.postDebug("msRunTime",
                           "Correct MS offset: time=" + Fmt.f(4,dispTime)
                           + ", msTime=" + Fmt.f(4,msTime));
        Messages.postDebug("msRunTime", "LcRun.correctMsRunTime("
                           + time_ms + "): pause_ms=" + pause_ms);

        return msTime_ms;
    }

    public void saveLog() {
        // Open file
        BufferedWriter fw = null;
        try {
            fw = new BufferedWriter(new FileWriter(m_filepath + ".html"));
        } catch (IOException ioe) {
            Messages.postError("Cannot open LC Log file: "
                               + m_filepath + ".html");
            return;
        }
        PrintWriter pw = new PrintWriter(fw);

        pw.println("<html>");
        pw.println("");
        pw.println("<title>" + "Run Log for " + runTitle + "</title>");
        pw.println("<h3 align=center>" + "Run Log for " + runTitle + "</h3>");

        double flow = method.getInitialFlow();
        pw.println("<p style=\"line-height: 105%\">");
        pw.println("Run Started: " + dateStamp);
        pw.println("<br>Type of Run: " + m_typeName);
        pw.println("<br>Method: " + m_methodName);
        pw.println("<br>Flow Rate: " + Fmt.f(3, flow) + " mL/minute");
        pw.println("</p>");

        // Detector attributes
        pw.println("<table border=1 cellpadding=0 cellspacing=0>");
        pw.println("  <caption align=top>");
        pw.println("    <h3>Active Channel Table</h3>");
        pw.println("  </caption>");
        pw.println("  <tr>");
        pw.println("    <th> Trace # </th>");
        pw.println("    <th> Signal </th>");
        pw.println("    <th> Threshold Level </th>");
        pw.println("    <th> Time Offset </th>");
        pw.println("    <th> Note</th>");
        pw.println("  </tr>");
        for (int i = 0; i < MAX_TRACES; i++) {
            if (config.isTraceEnabled(i)) {
                String name = config.getChannelName(i);
                /*Messages.postDebug("thresholds="+thresholds);/*CMP*/
                String thresh = Fmt.f(4, thresholds[i], false);
                String xfer = Fmt.f(4, nTransTime_ms[i] / 60000.0, false);
                pw.println("  <tr>");
                pw.println("    <td nowrap>" + (i + 1) + "</td>");
                pw.println("    <td nowrap>" + name + "</td>");
                pw.println("    <td nowrap>" + thresh + "</td>");
                pw.println("    <td nowrap>" + xfer + "</td>");
                pw.println("    <td nowrap>" + channelInfo[i] + "</td>");
                pw.println("  </tr>");
            }
        }
        String xfer = Fmt.f(4, -refTime_ms / 60000.0, false);
        pw.println("  <tr>");
        pw.println("    <td nowrap>" + "---" + "</td>");
        pw.println("    <td nowrap>" + "NMR Flow Cell" + "</td>");
        pw.println("    <td nowrap>" + " " + "</td>");
        pw.println("    <td nowrap>" + xfer + "</td>");
        pw.println("    <td nowrap>" + " " + "</td>");
        pw.println("  </tr>");
        pw.println("</table>");

        // Incident table
        pw.println("<br><table border=1 cellpadding=0 cellspacing=0>");
        pw.println("  <caption align=top>");
        pw.println("    <h3>Incident Table</h3>");
        pw.println("  </caption>");
        pw.println("  <tr>");
        pw.println("    <th><nobr> Scheduled </nobr></th>");
        pw.println("    <th><nobr> Actual </nobr></th>");
        pw.println("    <th><nobr> Duration </nobr></th>");
        pw.println("    <th><nobr> Channel </nobr></th>");
        pw.println("    <th><nobr> Threshold </nobr></th>");
        pw.println("    <th><nobr> Loop </nobr></th>");
        pw.println("    <th><nobr> Trigger </nobr></th>");
        pw.println("    <th><nobr> Action </nobr></th>");
        pw.println("  </tr>");
        LcEvent[] events = incidentList.toArray(new LcEvent[0]);
        for (int i = 0; i < events.length; i++) {
            LcEvent event = events[i];
            int chan = event.channel; // Starts at 1
            String sChan = chan == 0 ? "n/a" : Integer.toString(chan);
            double time = event.time_ms / 60000.0;

            double thresh = method.getThreshold(chan - 1, time);
            String sThresh = thresh <= 0 ? "n/a" : Fmt.f(4, thresh, false);
            /*System.out.println("** time=" + time
                                   + ", thresh " + chan + "=" + thresh);/*CMP*/
            String eventTime = Fmt.f(4, event.time_ms / 60000.0, false);
            String actionTime = Fmt.f(4, event.actual_ms / 60000.0, false);
            String duration = event.length_ms == 0 ? ""
                    : Fmt.f(4, event.length_ms / 60000.0, false);
            int loop = event.loop;
            String sLoop = loop == 0 ? "n/a" : Integer.toString(loop);
            pw.println("  <tr>");
            pw.println("    <td nowrap>" + eventTime + "</td>");
            pw.println("    <td nowrap>" + actionTime + "</td>");
            pw.println("    <td nowrap>" + duration + "</td>");
            pw.println("    <td nowrap>" + sChan + "</td>");
            pw.println("    <td nowrap>" + sThresh + "</td>");
            pw.println("    <td nowrap>" + sLoop + "</td>");
            String sTrig = event.trigger == null ? "-" : event.trigger.label();
            pw.println("    <td nowrap>" + sTrig + "</td>");
            pw.println("    <td nowrap>" + event.action + "</td>");
            pw.println("  </tr>");
        }
        pw.println("</table>");

        // Peak table from "Galaxie" analysis
        SortedSet<LcPeak> peakAnalysis = lcCtl.peakAnalysis;
        if (peakAnalysis != null) {
            pw.println("<br><table border=1 cellpadding=0 cellspacing=0>");
            pw.println("  <caption align=top>");
            pw.println("    <h3>Peak Table</h3>");
            pw.println("  </caption>");
            pw.println("  <tr>");
            pw.println("    <th><nobr> Peak # </th>");
            pw.println("    <th><nobr> Time </nobr></th>");
            pw.println("    <th><nobr> FWHM </nobr></th>");
            pw.println("    <th><nobr> Area </nobr></th>");
            pw.println("    <th><nobr> Height </nobr></th>");
            pw.println("  </tr>");
            LcPeak[] peaks = peakAnalysis.toArray(new LcPeak[0]);
            for (int i = 0; i < peaks.length; i++) {
                LcPeak pk = peaks[i];
                pw.println("  <tr>");
                pw.println("    <td nowrap>" + pk.getPeakIdString() + "</td>");
                pw.println("    <td nowrap>" + Fmt.f(4, pk.time) + "</td>");
                pw.println("    <td nowrap>" + Fmt.f(4, pk.fwhm) + "</td>");
                pw.println("    <td nowrap>" + Fmt.f(4, pk.area) + "</td>");
                pw.println("    <td nowrap>" + Fmt.f(4, pk.height)+"</td>");
                pw.println("  </tr>");
            }
            pw.println("</table>");
        }

        pw.println("All times are in minutes.");

        // Close file
        pw.println("</html>");
        pw.close();
    }

    /**
     * Get a bitmask indicating the dectector channels currently
     * doing peak detection.
     * @return Bit mask of enabled channels.  Bit 0 for 1st channel,
     * etc.
     */
    public int getPeakDetectChannels() {
        int rtn = 0;
        for (int channel = 0; channel < MAX_TRACES; channel++) {
            if (config.isTraceEnabled(channel)
                && method.isPeakEnabled(channel, getDataFlowTime()))
            {
                rtn |= 1 << channel;
            }
        }
        return rtn;
    }

    private void addEvent(int iChannel,
                          double time_min, EventTrig type, String cmd) {
        Messages.postDebug("LcEvent",
                           "addEvent: event time=" + Fmt.f(3,time_min));
        LcEvent lcEvent = new LcEvent((long)(time_min * 60000 + 0.5),
                                      nTransTime_ms[iChannel],
                                      iChannel + 1,
                                      lcCtl.getLoop(),
                                      type,
                                      cmd);
        lcEvent.peakNumber = -1;
        eventQueue.add(lcEvent);
        //gui.setStatus4("Status: Peak queued");
        //Messages.postInfo("Peak queued: time=" + time_min);
        /*Messages.postDebug("lcPeakDetect", "y0 time = "
                      + (t2 + nTransTime_ms[iChan] / 60000.0)
                      + ", peak time = " + peakTime);/*CMP*/
        m_plot.addMark(time_min,
                     m_plot.getDataValueAt(time_min, 2 * iChannel),
                     lcEvent);
    }

    public void testPeakDetect(int ichan) {
        Messages.postDebug("TestPeakDetect",
                           "testPeakDetect(" + ichan + ") ...");
        clearTestMarks();
        if (ichan >= 0 && ichan < MAX_TRACES) {
            m_shadowTestPeak[ichan] = false;
            m_xOfMaxYValue[ichan] = 0;
            int last = m_data.getSize(ichan);
            double ymax = m_data.getValue(ichan, 0);
            boolean isSaturated = false;
            for (int i = 0; i < last; i++) {
                if (ymax < m_data.getValue(ichan, i)) {
                    ymax = m_data.getValue(ichan, i);
                }
                isSaturated = fitForPeak(ichan, i+1, false, ymax, isSaturated);
            }
        }
        Messages.postDebug("TestPeakDetect"," ... testPeakDetect() done.");
    }

    public void clearTestMarks() {
        m_plot.clear(TEMP_DATASET);
    }

    /**
     * Check for a peak detection on one point of one channel.
     * @param iChan The channel number (from 0).
     * @param npts The current number of points, i.e., look for a peak
     * only over the fit region ending at n-1;
     * @param inRun True if this is a fit for a run in progress. Events
     * will not be inserted if this is false.
     * @param yMax The current maximum value on this channel.
     * @param isSaturated Whether the channel was considered saturated
     * on the previous point.
     */
    private boolean fitForPeak(int iChan, int npts, boolean inRun,
                               double yMax, boolean isSaturated) {
        final int MIN_POINTS = 5;
        long time_ms;
        double peakTime;

        if (npts < MIN_POINTS) {
            return isSaturated;
        }

        // Find how many data points to fit
        // First point in fit will have time <= start of desired time interval
        double dPkWidth_ms = lcCtl.m_peakDetectWidth_s[iChan] * 1000;
        double tLast_ms = m_data.getFlowTime(iChan, npts - 1);
        double tFirst_ms = tLast_ms - dPkWidth_ms; // Desired first time in fit
        int n;                  // Number of points in fit
        double t1 = tLast_ms;   // Will be the time of first point in fit
        for (n = MIN_POINTS;
             n <= npts && (t1 = m_data.getFlowTime(iChan, npts-n)) > tFirst_ms;
             n++);
        
        if (t1 > tFirst_ms) {
            return isSaturated; // Don't have enough points to do a fit yet
        }
        double t2 = (t1 + tLast_ms) / (2 * 60000.0); // Middle of fit (minutes)

        boolean[] shadowPeak;
        if (inRun) {
            shadowPeak = m_shadowPeak;
        } else {
            shadowPeak = m_shadowTestPeak;
        }
        if (!inRun
            || (config.isTraceEnabled(iChan)
                && method.isPeakEnabled(iChan, t2)) )
        {
            // Load recent data into a 1D array
            double[] x = new double[n];
            double[] y = new double[n];
            for (int i = npts - n, j = 0; i < npts; i++, j++) {
                y[j] = m_data.getValue(iChan, i);
                // NB: Shift x=0 to middle of fit region
                x[j] = m_data.getFlowTime(iChan, i) / 60000.0 - t2;
            }
            double xMaxForPeak =  (x[0] + x[n - 1]) / 2;

            double threshold = inRun
                    ? method.getThreshold(iChan, t2)
                    : lcCtl.m_peakDetectTestThresh[iChan];
            if (y[n-1] >= threshold) {
                aboveThreshold[iChan] = npts;
            } else {
                belowThreshold[iChan] = npts;
            }

            // Check for saturated peak
            double satLevel = yMax * 0.999;// Allow for a little "noise"
            Messages.postDebug("lcPeaks",
                              "satLevel=" + satLevel + ", ylast=" + y[n-1]);
            if (y[n-1] < satLevel) {
                // We're now below saturation
                if (isSaturated) {
                    // ... but previous point was saturated
                    if (satLevel > threshold) {
                        // Saturation is over threshold; found a saturated peak
                        Messages.postDebug("lcPeaks",
                                           "Saturated Peak found: iChan="
                                           + iChan
                                           + ", saturation level="
                                           + satLevel);
                        m_lastPeakHeight[iChan] = satLevel;
                        m_lastPeakFound[iChan] = npts - 1;
                        shadowPeak[iChan] = true;
                        // Just coming out of saturation - mark pk at middle
                        int pt = (npts + m_xOfMaxYValue[iChan]) / 2;
                        time_ms = m_data.getFlowTime(iChan, pt - 1)
                                + nTransTime_ms[iChan];
                        peakTime = time_ms / 60000.0;
                        if (inRun) {
                            // NB: Put peak in for current time if time we
                            // already determined is in the past.
                            double flowNow = getNmrFlowTime();
                            if (peakTime < flowNow) {
                                peakTime = flowNow;
                            }
                            addEvent(iChan,
                                     peakTime,
                                     EventTrig.PEAK_DETECT,
                                     getPeakAction(peakTime));
                        } else {
                            double z;
                            z = m_plot.getDataValueAt(peakTime, 2 * iChan);
                            m_plot.addPoint(TEMP_DATASET, peakTime, z,
                                          false, true, true);
                            m_plot.addPoint(TEMP_DATASET, peakTime, 0,
                                          true, true, true);
                        }
                        m_plot.repaint();
                    }
                }
                m_xOfMaxYValue[iChan] = npts; // Last point not sated
                isSaturated = false;
            } else if (!isSaturated) {
                // We're at max value seen and were not saturated before...
                // Are we saturated now? ... Look at the last few points.
                int i = Math.min(n - MIN_POINTS, n / 2);
                double ymin = y[i];
                double ymax = y[i];
                for (i++; i < n; i++) {
                    if (ymin > y[i]) {
                        ymin = y[i];
                    } else if (ymax < y[i]) {
                        ymax = y[i];
                    }
                }
                if (Math.abs(1 - ymax / ymin) < 0.001) {
                    // We've had a flat line for a while
                    isSaturated = true;
                } else if (n > 1 && y[n-1] > y[n-2]) {
                    // If we are going up, previous point was not saturated.
                    m_xOfMaxYValue[iChan] = npts -1; // Prev point not sated
                }

            }

            // Now check for a normal peak
            int xdist = npts - m_lastPeakFound[iChan];
            double ydist = y[n - 1] - m_lastPeakHeight[iChan];
            if (shadowPeak[iChan]
                && (xdist > n || (xdist > n / 2 && ydist > 0)))
            {
                //
                // Turn off peak shadow.
                // Gone a long way since the last peak, or going up higher.
                //
                Messages.postDebug("lcPeaks",
                                   "Peak shadowing turned off (distance): "
                                   + "(xdist>n)=" + (xdist > n)
                                   + ", (xdist>n/2 && ydist>0)="
                                   + (xdist > n / 2 && ydist > 0));
                shadowPeak[iChan] = false;
            } else {
                //
                // See if we have either a peak or a valley here.
                // (Valley can turn off shadowing.)
                // Successful test here is a peak.
                //
                double s2n = lcCtl.m_peakDetectSignalToNoise[iChan];
                double[] check = peak(x, y, s2n);
                if (!isSaturated
                    && !shadowPeak[iChan]
                    && check[0] < 0
                    && check[1] <= xMaxForPeak
                    && check[1] > x[0]
                    && threshold < check[4]
                    )
                {
                    //
                    // We have passed the center of a peak
                    //
                    Messages.postDebug("lcPeaks",
                                       "Peak found:"
                                       + " t2=" + Fmt.f(3, t2)
                                       + ", iChan=" + iChan
                                       + ", n=" + n
                                       + ", chi=" + Fmt.f(2, check[2])
                                       + ", range=" + Fmt.f(2, check[3])
                                       + ", curve=" + Fmt.f(4, check[0])
                                       + ", xPk=" + Fmt.f(3, check[1])
                                       + ", yPk=" + Fmt.f(3, check[4])
                                       );
                    peakTime = t2 + check[1];
                    m_lastPeakHeight[iChan] = check[4];
                    m_lastPeakFound[iChan] = npts;
                    shadowPeak[iChan] = true;
                    time_ms = (long)((peakTime * 60000 + 0.5)
                                     + nTransTime_ms[iChan]);
                    peakTime = time_ms / 60000.0;

                    if (inRun) {
                        addEvent(iChan,
                                 peakTime,
                                 EventTrig.PEAK_DETECT,
                                 getPeakAction(peakTime));
                    } else {
                        double toffset = t2 + nTransTime_ms[iChan] / 60000.0;
                        double[] parabola = {check[5], check[6], check[7]};
                        int nplot = 3 * (n - 1) + 1;
                        m_plot.addPoly(TEMP_DATASET, nplot, x[0], x[n - 1],
                                       toffset, parabola);
                        double z;
                        z = m_plot.getDataValueAt(peakTime, 2 * iChan);
                        m_plot.addPoint(TEMP_DATASET, peakTime, z,
                                      false, true, true);
                        m_plot.addPoint(TEMP_DATASET, peakTime, 0,
                                      true, true, true);
                    }
                    m_plot.repaint();
                } else if (shadowPeak[iChan]
                           && check[0] > 0 // A minimum
                           && check[1] <= 0
                           && check[1] > x[0])
                {
                    //
                    // Went through a valley
                    //
                    Messages.postDebug("lcPeaks",
                                       "Peak shadowing turned off (valley): "
                                       + "iChan=" + iChan);
                    shadowPeak[iChan] = false;
                } else {
                    //
                    // Nothing - here's the usual case
                    //
                    Messages.postDebug("lcPeaks",
                                       "No peak: iChan=" + iChan
                                       + ", unsaturated="
                                       + !isSaturated
                                       + ", unshadowed="
                                       + !shadowPeak[iChan]
                                       + ", (curvature<0)="
                                       + (check[0] < 0)
                                       + ", (xPeak<="
                                       + Fmt.f(2, xMaxForPeak) + ")="
                                       + (check[1] <= xMaxForPeak)
                                       + ", (xPeak>" + Fmt.f(2, x[0]) + ")="
                                       + (check[1] > x[0])
                                       + ", (thresh<yPk)="
                                       + (threshold < check[4])
                                       );
                }
            }
        }
        return isSaturated;
    }

    /**
     * Find the peak defined by some data points.  Fits a parabola
     * through the points by least squares and finds the max of the
     * parabola.
     * <p>
     * Determines whether a peak (or valley) was reliably detected
     * by computing the ratio of the range of the y-coordinate of the
     * parabola over the fit region to the RMS residual of the fit.
     * The ratio must be larger than the specified signal-to-noise to
     * have a peak detection.
     *
     * @param x An array of x values--must be monotonicly increasing.
     * @param y An array of data values--same length as x.
     * @param s2n The signal-to-noise ratio used to define a successful peak
     * detection.
     * @return Array of length 8 containing the following elements.
     * <br> [0] The curvature of the fit: negative for a peak,
     * positive for a valley, or 0 for no definitive extremum.
     * <br> [1] The x coordinate of the extremum.
     * <br> [2] The square-root of the Chi-square statistic of the fit.
     * <br> [3] The range in Y of the fit over the interval.
     * <br> [4] The Y value at the peak.
     * <br> [5] The a0 coefficient of the fit.
     * <br> [6] The a1 coefficient of the fit.
     * <br> [7] The a2 coefficient of the fit.
     */
    protected double[] peak(double[] x, double[] y, double s2n) {
        double[] rtn = {0, 0, 0, 0, 0, 0, 0, 0};
        int len = y.length;

        // Get the fit
        LinFit.PolyBasisFunctions poly = new LinFit.PolyBasisFunctions(2);
        LinFit.FitResult ans = LinFit.svdFit(x, y, null, poly);
        double[] a = ans.getCoeffs();
        double chi = Math.sqrt(ans.getChisq());

        double xpk = Double.NaN;
        // NB: If xpk remains NaN, ypk and range will be NaN and rtn[0] = 0.
        if (a[2] != 0) {
            xpk = -a[1] / (2 * a[2]);
        }
        double ypk = a[0] + xpk * (a[1] + xpk * a[2]);

        // Calculate the y-range of the parabola over the x-range of the fit
        double xend = (Math.abs(xpk - x[0]) > Math.abs(xpk - x[len - 1]))
                ? x[0] : x[len - 1];
        double range = Math.abs(a[0] + xend * (a[1] + xend * a[2]) - ypk);

        /*Messages.postMessage(Messages.LOG,
                             "range=" + Fmt.f(2, range)
                             + ", chi=" + Fmt.f(2, chi)
                             + ", ratio=" + Fmt.f(2, range / chi)
                             + ", xpk=" + Fmt.f(2, xpk));/*CMP*/

        // A bunch of conditions that mean no peak detected
        // Need this?: (ymin == 0 || !(range / ymin > 0.001)) // Too flat
        if (
            (chi > 0 && a[2] < 0 && range / chi < s2n) // Peak too noisy
            || (chi > 0 && a[2] > 0 && range / chi < 2) // Valley too noisy
            || (xpk < x[0] || xpk > x[len - 1]) // Peak out of x-range
            )
        {
            rtn[0] = 0;         // No peak detected
        } else {
            rtn[0] = a[2];
        }
        rtn[1] = xpk;
        rtn[2] = chi;
        rtn[3] = range;
        rtn[4] = ypk;
        rtn[5] = a[0];
        rtn[6] = a[1];
        rtn[7] = a[2];

        return rtn;
    }

    public void stopFlowNmr() {
        pauseRun();
        slimIO.triggerToNmr();
        slimIO.enableNmrTriggerInput();
        pauseStartTime = System.currentTimeMillis();
        Messages.postDebug("lcTiming",
                           "stopFlowNmr(): pauseStartTime="
                           + pauseStartTime);

        m_holdTime_min = method.getHoldTime(m_dataFlowTime_ms);
        String holdType = method.getHoldType(m_dataFlowTime_ms);
        if (m_holdTime_min > 0 && "fixed".equals(holdType)) {
            // Arrange to restart after a fixed time
            restartRunLater((int)(m_holdTime_min * 60000));
        }
        startHoldCount();
    }

    /**
     * Do a stop flow for time-slice, and , if time-slice is still
     * enabled, insert the next time-slice event.
     */
    public void nextTimeSlice(double eventTime_min) {
        if (eventTime_min < 0) {
            eventTime_min = m_dataFlowTime_ms / 60000.0;
        }
        Messages.postDebug("TimeSlice", "nextTimeSlice: t=" + eventTime_min);
        double dt_min = method.getTimeSlicePeriod(eventTime_min) / 60;
        double nextTime_min = eventTime_min + dt_min;
        // See if time slice is enabled just BEFORE next scheduled slice.
        // Avoids missing last slice because of rounding.
        double EnbTestTime_min = nextTime_min - dt_min / 20;
        Messages.postDebug("TimeSlice",
                           "dt_min=" + dt_min + ", isTimeSlice="
                           + method.isTimeSlice(EnbTestTime_min));
        if (dt_min > 0 && method.isTimeSlice(EnbTestTime_min)) {
            // Insert next timeSlice event in queue
            insertTimeSlice(nextTime_min, EventTrig.TIME_SLICE);
        }
        stopFlowNmr();
    }

    /**
     * Insert a time-slice event into the eventQueue for a given time.
     */
    public void insertTimeSlice(double time_min, EventTrig source) {
        String cmd = ("timeSliceNmr('y', "
                      + Fmt.f(3, time_min, false) + ", $PEAK)");
        long time_ms = (long)(time_min * 60000);
        LcEvent lcEvent = new LcEvent(time_ms, 0, 0,
                                      lcCtl.getLoop(),
                                      source,
                                      MarkType.SLICE,
                                      cmd);
        //lcEvent.markType = MarkType.SLICE;
        eventQueue.add(lcEvent);
        m_plot.addMark(time_min,
                     SLICE_Y_POSITION, lcEvent);
        m_plot.repaint();
    }

    /**
     * Remove the next time slice event from the queue (that is not
     * the first in a time-slice sequence).
     */
    public void removeNextTimeSlice() {
        LcEvent[] events = eventQueue.toArray(new LcEvent[0]);
        for (int i = 0; i < events.length; i++) {
            LcEvent e = events[i];
            if (e.markType == MarkType.SLICE
                && e.trigger == EventTrig.TIME_SLICE)
            {
                Messages.postDebug("LcRun",
                                   "removeTimeSlice at "
                                   + (e.time_ms / 60000.0));
                eventQueue.remove(e);
                m_plot.unmarkEvent(e);
                break;
            }
        }
    }

    /**
     * Create a collectLoop event and put it in the incident table
     */
    public void collectLoopEvent() {
        double t_min = getNmrFlowTime();
        int iLoop = lcCtl.getLoop();
        long t_ms = getNmrFlowTime_ms();
        LcEvent lcEvent = new LcEvent(t_ms,
                                      0, 0,
                                      iLoop,
                                      EventTrig.USER_ACTION,
                                      MarkType.COLLECTION,
                                      "collectLoop");
        lcEvent.markType = MarkType.PEAK;
        lcEvent.actual_ms = t_ms;
        incidentList.add(lcEvent);
        m_plot.addMark(t_min, COLLECTION_Y_POSITION, lcEvent);
        m_plot.repaint();
    }

    public void collectLoop() {
        double t_min = getNmrFlowTime();
        collectLoop(t_min);
    }

    public void collectLoop(double time_min) {
        int iLoop = lcCtl.getLoop();
        int nLoops = config.getNumberOfLoops();
        String strLoop = "" + iLoop;
        if (DebugOutput.isSetFor("noAC")) {
            strLoop = "lcLoopIndex";
        }
        // Step to next loop
        slimIO.loopIncr();

        // Set solvent pcts for this loop (before step)
        if (iLoop <= nLoops) {
            double pumpToLoop_min = config.getPumpToLoopTransferTime() / 60.0;
            Messages.postDebug("loopCollect",
                               "time_min=" + time_min
                               + ", pumpToLoop_min=" + pumpToLoop_min);
            double[] pcts = method.getPercents(time_min - pumpToLoop_min);
            char bottle = 'A';
            for (int i = 0; i < pcts.length; i++, bottle++) {
                String cmd = "lcLoopPercent" + bottle + "[" + strLoop + "]"
                        + "=" + Fmt.fg(2, pcts[i]);
                Messages.postDebug("LcRun", "sendToVnmr: " + cmd);
                sendToVnmr(cmd);
            }
            sendToVnmr("collectLoop(" + strLoop + ")");
        }
    }

    private void restartRunLater(int delay_ms) {
        restartTimer.setInitialDelay(delay_ms);
        restartTimer.start();
    }

    private void startHoldCount() {
        if (!holdTimer.isRunning()) {
            holdStopTime = System.currentTimeMillis();
            holdTimer.start();
        }
    }

    private void setValves(int flow, String type) {
        boolean manual = config.isManualFlowControl();
        Messages.postDebug("flowControl", "manual=" + manual);
        if (manual) {
            return;
        } else if (flow == FLOW_OFF) {
            slimIO.setValvesStopped(true);
            slimIO.SFStopPulse(); // Set stop-flow valve
            //slimIO.leftCollWastePulse();
            //slimIO.rightWastePulse();
            //slimIO.pumpStopPulse();
            lcCtl.pumpStopRun();
            if (config.getMsPumpControl()) {
                slimIO.setMakeupFlow(0); // MS pump off
                sendToVnmr("pmlcmd('lc_valve=0')");
            }
        } else {
            slimIO.SFFlowPulse(); // Set stop-flow valve
            if (type == null) {
                Messages.postError("LcRun.setValves(): null run type");
                return;
            } else if (type.equals("scout")
                       || type.equals("iso")
                       || type.equals("stop")
                       || type.equals("enter"))
            {
                // Through column to NMR
                slimIO.leftColumnPulse();
                //slimIO.rightNmrPulse();
            } else if (type.equals("enterA")) {
                // From analyte collector to NMR - bypass column
                slimIO.leftCollWastePulse();
                //slimIO.rightNmrPulse();
            } else if (type.equals("analyte") || type.equals("lcalone")) {
                // Through column to analyte collector - bypass NMR
                slimIO.leftColumnPulse();
                //slimIO.rightWastePulse();
            } else {
                Messages.postError("LcRun.setValves(): unknown run type: "
                                   + type);
                return;
            }
            // TODO: Is getFlowTime() correct here? or need clock based time?
            Object oBypass = method.getValue("lcNmrBypass", getDataFlowTime());
            boolean bBypass = ((Boolean)oBypass).booleanValue();
            if (bBypass) {
                slimIO.rightWastePulse();
            } else {
                slimIO.rightNmrPulse();
            }
            //slimIO.pumpStartPulse();
            lcCtl.pumpStartRun();
            if (config.getMsPumpControl()) {
                double rate = config.getMsPumpFlow();
                slimIO.setMakeupFlow(rate); // MS pump on
                sendToVnmr("pmlcmd('lc_valve=1')");
            }
        }
    }

    public String getRunType() {
        return m_type;
    }

    public String getRunTypeString() {
        return getRunTypeString(m_type);
    }

    public static String getRunTypeString(String type) {
        if (type == null) {
            return "null";
        } else if (type.equals("scout")) {
            return "On Flow NMR";
        } else if (type.equals("iso")) {
            return "Isocratic On Flow NMR";
        } else if (type.equals("stop")) {
            return "Stop Flow NMR";
        } else if (type.equals("enter")) {
            return "Stop Flow NMR";
        } else if (type.equals("analyte")) {
            return "Analyte Collection";
        } else if (type.equals("enterA")) {
            return "Analyte Elution";
        } else if (type.equals("lcalone")) {
            return "Test LC";
        } else {
            return type;
        }
    }

    public String getPeakAction(double time) {
        String customCmd = config.getPeakAction();
        if (customCmd != null) {
            return customCmd;
        } else {
            return method.getPeakAction(time, m_type);
        }
    }

    public long getAdcStartTime_ms(){
        return adcStartTime;
    }

    public long getPrevSegTime_ms(){
        return  previousSegment_ms;
    }

    public void setPauseDuration() {
        // Set the time paused
        long now = System.currentTimeMillis();
        LcEvent lce = null;
        LcEvent[] incidentArray = new LcEvent[0];
        incidentArray = incidentList.toArray(incidentArray);
        for (int i = incidentArray.length - 1; i >= 0; --i) {
            lce = incidentArray[i];
            // NB: Special case for correcting pause time in loop elution
            // TODO: lce.stopsFlow() is a stub that always returns true
            if (lce.stopsFlow() && lce.actual_ms != 0
                && pauseStartTime > 0
                && (lce.length_ms == 0 || m_type.equals("enterA")))
            {
                lce.length_ms = now - pauseStartTime;
                Messages.postDebug("lcTiming",
                                   "setPauseDuration(): pauseTime="
                                   + (now - pauseStartTime));
                break;
            }
        }
        saveLog();           // Update run log file
    }

    public void trigFromNmr() {
        if ("nmr".equals(method.getHoldType(0))) {  // Only in wait-for-nmr mode
            String customCmd = config.getNmrDoneAction();
            if (customCmd != null) {
                sendToVnmr(customCmd);
            } else if (m_type.equals("enterA")) {
                sendToVnmr("eluteLoop('next')");
            } else {
                restartRun();
            }
        }
    }

    /**
     * Start an LC run that has been previously initialized.
     * This starts the flow, notifies the MS (through VnmrBG), and
     * calls restartChromatogram().
     */
    public boolean restartRun() {
        if (!flowPaused) {
            return false;
        }
        setValves(FLOW_ON, m_type);
        sendToVnmr("pmlcmd('collect_resume')");
        // Listener must be set before starting PDA
        return restartChromatogram();
    }

    /**
     * Start collecting and saving chromatogram data.  Data may be
     * collected outside of runs, but not saved.
     */
    public boolean restartChromatogram() {
        if (!flowPaused) {
            return false;
        }
        lcCtl.startPda();
        slimIO.startRun(this, (int)m_dataInterval_ms);
        //slimIO.openDataWriter(m_filepath);
        //slimIO.startAdc(this, (int)m_dataInterval_ms);
        adcStartTime = System.currentTimeMillis();
        Messages.postDebug("lcTiming",
                           "restartRun(): adcStartTime=" + adcStartTime);
        setPauseDuration();
        flowPaused = false;
        runActive = true;
        saveLog();
        sendToVnmr("lcNumberOfIncidents=lcNumberOfIncidents + 0.01");
        return true;
    }

    public void clearEvents() {
        m_plot.unmarkEvents();
        eventQueue.clear();
    }

    public boolean checkEvents(long time) {
        boolean gotEvent = false;
        LcEvent e;
        Messages.postDebug("LcEvent+",
                           "checkEvents: time=" + time + " ms");
        for (e = eventQueue.size() <= 0 ? null : eventQueue.first();
             e != null && e.time_ms <= time;
             e = eventQueue.size() <= 0 ? null : eventQueue.first())
        {
            Messages.postDebug("LcEvent", "checkEvents: got event: checkTime="
                               + Fmt.f(3, time / 60000.0) + " min");
            // Time to do next event
            eventQueue.remove(e);
            e.actual_ms = time;
            //if (e.loop != 0) {
            e.loop = lcCtl.getLoop(); // Set loop to value before action
            //}
            if (e.action.equals("timedElution")) {
                e.peakNumber = -1;
            }
            if (e.peakNumber < 0) {
                e.peakNumber = ++m_peakCount;
            }
            // Maybe insert peak number and time in command
            //System.err.println("action='" + e.action + "'");/*CMP*/
            e.action = e.action.replaceAll("\\$PEAK", Fmt.d(1, m_peakCount));
            String strTime = Fmt.f(3, time / 60000.0, false);
            e.action = e.action.replaceAll("\\$TIME", strTime);
            String vcmd = e.action;
            //System.err.println("vcmd=" + vcmd);/*CMP*/
            if ((vcmd.startsWith("timeSlice") && vcmd.indexOf("'n'") < 0)) {
                pauseRun(); // Stop pump w/o waiting to hear from Vnmr
                if (e.trigger == EventTrig.TIME_SLICE) {
                    method.undisableTimeSlice(getDataFlowTime());
                }
            } else if (vcmd.startsWith("stopFlow")) {
                pauseRun(); // Stop pump w/o waiting to hear from Vnmr
            }
            if (vcmd.indexOf("EndRun('VJ')") >= 0) {
                didEndRun = true;
            }
            if (vcmd.equals("stopFlowInject")) {
                // TODO: set pauseStartTime in pauseRun instead of stopFlowNmr?
                // TODO: pauseStartTime needed only for loop elution?
                pauseStartTime = System.currentTimeMillis();

                m_waitingForInjection = true;
                lcCtl.startInjection();
            } else {
                // NB: stopFlowInject command is not sent to Vnmr
                Messages.postDebug("LcRun", "sendToVnmr: " + vcmd);
                sendToVnmr(vcmd);
            }
            incidentList.add(e);
            m_plot.validateMarks(false);
            gotEvent = true;
        }
        if (gotEvent) {
            saveLog();
            sendToVnmr("lcNumberOfIncidents=" + incidentList.size());
        }
        return gotEvent;
    }

    /**
     *
     */
    public void handleInjection(String cmd) {
        Messages.postDebug("lcInject", "LcRun.handleInjection(" + cmd + ")");
        if (runActive && m_waitingForInjection) {
            if (flowPaused) {
                restartRun();
            } else {
                restartChromatogram();
            }
        }
        if (cmd != null) {
            Messages.postDebug("lcInject", "...Call sendToVnmr(" + cmd + ")");
            sendToVnmr(cmd);
        }
        m_waitingForInjection = false;
    }

    public void sendToVnmr(String str) {
        lcCtl.sendToVnmr(str);
    }

    static public boolean translateLcdata(String inpath, String outpath) {
        final int bufSize = 32 * (1 << 10); // 32K
        BufferedReader in = null;
        int nLine = 0;
        try {
            String line;
            in = new BufferedReader(new FileReader(inpath), bufSize);
            in.mark(bufSize);
            nLine = 1;
            if ((line = in.readLine()) != null) {
                if (line.startsWith("Run start time:")) {
                    // Lcnmr2000 format -- do nothing
                    in.close();
                    return false;
                } else if (line.startsWith("; File format version: LCD 1.")) {
                    // VJ data format -- process below
                } else {
                    Messages.postError("translateLcdata(): "
                                       + " Unrecognized Format");
                    in.close();
                    return false;
                }
            } else {
                Messages.postError("translateLcdata(): "
                                   + " Empty File");
                in.close();
                return false;
            }
        } catch (FileNotFoundException fnfe) {
            Messages.postError("Cannot find LC data file: " + inpath);
            return false;
        } catch (IOException ioe) {
            Messages.postError("translateLcdata(): "
                               + ioe.getLocalizedMessage());
            Messages.writeStackTrace(ioe);
            try { in.close(); } catch (IOException ioe2) {}
            return false;
        }

        int fileSize = (int)(new File(inpath).length());

        // Open output file
        BufferedWriter fw = null;
        try {
            fw = new BufferedWriter(new FileWriter(outpath));
        } catch (IOException ioe) {
            Messages.postError("Cannot open LC Data output file: " + outpath);
            return false;
        }
        PrintWriter pw = new PrintWriter(fw);

        try {
            String line;

            // Dummy first line
            pw.println("run000.lcd");

            while (!(line=in.readLine()).startsWith("; Run start time"));
            line = in.readLine(); // Read time
            pw.println("Run start time: " + line);

            pw.println("2000,0,9,1"); // Version #

            while (!(line=in.readLine()).startsWith("; Run Title"));
            line = in.readLine(); // Read title
            pw.println(line); // Run title

            pw.println("; Star text file name:"); // Dummy Star file name
            pw.println("");

            while (!(line=in.readLine()).startsWith("; Run type"));
            line = in.readLine();
            pw.println("; Experiment type:");
            pw.println(line); // Run type

            while (!(line=in.readLine()).startsWith("; flow rate"));
            pw.println("; flow rate, ml/min:");
            line = in.readLine();
            pw.println(line);

            while (!(line=in.readLine()).startsWith("; active channels"));
            pw.println("; active channels:");
            line = in.readLine();
            pw.println(line);

            while (!(line=in.readLine()).startsWith("; detection thresholds"));
            pw.println("; detection thresholds:");
            line = in.readLine();
            pw.println(line);

            while (!(line=in.readLine()).startsWith("; Transfer times"));
            line = in.readLine(); // Decode transfer times
            int[] xfer = new int[3];
            if (line != null) {
                try {
                    StringTokenizer toker = new StringTokenizer(line, ",");
                    for (int i = 0; i < 3 && toker.hasMoreTokens(); i++) {
                        double x = Double.parseDouble(toker.nextToken());
                        xfer[i] = (int)(60 * x);
                    }
                } catch (NumberFormatException nfe) {}
            }
            pw.println("; detector-to-probe times, stop-flow expt, "
                       + "chans A, B, C:");
            pw.println(xfer[0] + "," + xfer[1] + "," + xfer[2]);
            pw.println("; detector-to-loop times, collection expt, "
                       + "chans A, B, C:");
            pw.println(xfer[0] + "," + xfer[1] + "," + xfer[2]);
            pw.println("; detector-to-probe times, detected elution expt, "
                       + "chans A, B, C:");
            pw.println(xfer[0] + "," + xfer[1] + "," + xfer[2]);
            pw.println("; detector-to-probe times, fixed elution expt, "
                       + "chans A, B, C:");
            pw.println(xfer[0] + "," + xfer[1] + "," + xfer[2]);

            pw.println("; chart title:");
            pw.println("");

            while (!(line=in.readLine()).startsWith("; Run notes"));
            while ((line=in.readLine()).startsWith("|"));
            pw.println("; run notes:");
            pw.println("|");
            pw.println("|");
            pw.println("|");
            pw.println("|");
            pw.println("|");
            pw.println("|");
            pw.println("|");
            pw.println("|");
            pw.println("|");
            pw.println("|");

            // Get the incident info for later printing (after we have data rate)
            for ( ; !line.startsWith("; Incident table"); line = in.readLine());
            line = in.readLine(); // Get # of incidents
            int nIncidents = 0;
            if (line != null) {
                try {
                    nIncidents = Integer.parseInt(line);
                } catch (NumberFormatException nfe) {}
            }
            int j = 0;
            int[] chan = new int[nIncidents];
            double[] tEvent = new double[nIncidents];
            double[] tDelay = new double[nIncidents];
            double[] tActual = new double[nIncidents];
            double[] tStopped = new double[nIncidents];
            int[] loop = new int[nIncidents];
            String trig = "";
            for (int i = 0; i < nIncidents; i++) {
                /*Messages.postDebug("i=" + i + ", j=" + j);/*CMP*/
                line = in.readLine(); // Get incident
                if (line != null) {
                    try {
                        StringTokenizer toker = new StringTokenizer(line, ",");
                        if (toker.hasMoreTokens()) {
                            chan[j] = Integer.parseInt(toker.nextToken());
                        }
                        if (toker.hasMoreTokens()) {
                            tEvent[j] = Double.parseDouble(toker.nextToken());
                        }
                        if (toker.hasMoreTokens()) {
                            tDelay[j] = Double.parseDouble(toker.nextToken());
                        }
                        if (toker.hasMoreTokens()) {
                            tActual[j] = Double.parseDouble(toker.nextToken());
                        }
                        if (toker.hasMoreTokens()) {
                            tStopped[j] = Double.parseDouble(toker.nextToken());
                        }
                        if (toker.hasMoreTokens()) {
                            loop[j] = Integer.parseInt(toker.nextToken());
                        }
                        if (toker.hasMoreTokens()) {
                            trig = toker.nextToken();
                        }
                    } catch (NumberFormatException nfe) {}
                }
                if (trig.equals("Peak Detect")
                    || trig.equals("Click on Plot")
                    || trig.equals("User Hold"))
                {
                    j++;
                }
            }

            while (!(line=in.readLine()).startsWith("; data"));
            int nDats = 0;
            StringBuffer sbuf = new StringBuffer(fileSize);
            line = in.readLine(); // # data lines or first data
            // Read in all the data to a buffer
            if (line != null & line.indexOf(",") >= 0) {
                nDats++;
                sbuf.append(line);
                sbuf.append("\n");
            }
            String prevline = "";
            while ((line = in.readLine()) != null) {
                nDats++;
                sbuf.append(line);
                sbuf.append("\n");
                prevline = line;
            }
            // Calc data rate
            double dataRate = 60;// pts per minute
            try {
                StringTokenizer toker = new StringTokenizer(prevline,"");
                if (toker.hasMoreTokens()) {
                    double t = Double.parseDouble(toker.nextToken());
                    if (t != 0) {
                        dataRate = (nDats - 1) / t;
                    }
                }
            } catch (NumberFormatException nfe) {}

            // Print new incident table
            pw.println("; incident table:");
            pw.println(j);
            for (int i = 0; i < j; i++) {
                /*Messages.postDebug("chan[i]=" + chan[i]);/*CMP*/
                double dt = chan[i] == 0 ? 0 : xfer[chan[i] - 1];
                double tt = tEvent[i] - dt;
                pw.println(chan[i] + ","
                           + (int)(tEvent[i] * dataRate) + ","
                           + tEvent[i] + ","
                           + (int)(tt * dataRate) + ","
                           + tt + ","
                           + tStopped[i] + ","
                           + loop[i]);
            }

            pw.println("; label table:");
            pw.println("0");

            pw.println("; data:");
            pw.println(nDats);
            pw.print(sbuf.toString());

        } catch (IOException ioe) {
            Messages.postError("translateLcdata(): "
                               + ioe.getLocalizedMessage());
            Messages.writeStackTrace(ioe);
            try { in.close(); pw.close(); } catch (IOException ioe2) {}
            return false;
        }

        try { in.close(); pw.close(); } catch (IOException ioe2) {}
        return true;
    }
}
