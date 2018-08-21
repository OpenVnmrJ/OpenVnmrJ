/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.io.*;
import java.util.*;

import vnmr.lc.LcEvent.EventTrig;
import vnmr.lc.LcEvent.MarkType;
import vnmr.util.*;


public class LcFileRun extends LcRun {

    private static final int LC2000_TRACES = 3;

    private int m_nTraces = MAX_TRACES;
    private boolean[] m_chanActive = new boolean[MAX_TRACES];
    private  double[] m_xferTime = new double[MAX_TRACES];
    //private boolean[] m_chanActive = null;
    //private double[] m_xferTime = null;
    private String m_fullfilename = "";

    private int[] m_columnToTrace = new int[MAX_TRACES];

    public LcFileRun(LcControl lcc,
                     LcConfiguration conf,
                     SortedSet<LcEvent> queue,
                     LcSlimIO io,
                     VLcPlot.LcPlot p,
                     String filepath) {
        //Messages.postInfo("LcFileRun starting");
        lcCtl = lcc;
        config = conf;
        eventQueue = queue;
        this.slimIO = io;
        m_plot = p;
        this.m_filepath = filepath;

        File cFilepath = new File(filepath);
        String path = cFilepath.getParent();
        sendToVnmr("lcRunLogFile='" + path + File.separator + "LcRun.html'");

        initPlot();
        readData();
        incidentList.addAll(eventQueue);
        eventQueue.clear();

        for (int i = 0; i < MAX_TRACES; i++) {
            channelInfo[i] = " ";
        }

        lcCtl.configGraphPanel(this);
    }

    public boolean isPdaData() {
        String path = m_fullfilename;
        File fPath = new File(path);
        if (!fPath.isDirectory()) {
            path = fPath.getParent();
        }
        File fPdaFile = new File(path + File.separator + "pdaData.dat");
        return fPdaFile.exists();
    }

    public boolean isMsData() {
        String path = m_fullfilename;
        File fPath = new File(path);
        if (!fPath.isDirectory()) {
            path = fPath.getParent();
        }
        File fMsFile = new File(path + File.separator + "msData.msd");
        return fMsFile.exists();
    }

    private double[] parseDoubles(String line, String msg, int nLine) {
        StringTokenizer toker = new StringTokenizer(line, " ,\t");
        int n = toker.countTokens();
        double[] d = new double[n];
        int i;
        for (i = 0; i < n; ++i) {
            String tok = toker.nextToken();
            try {
                d[i] = Double.parseDouble(tok);
            } catch (NumberFormatException nfe) {
                // Just assume this marks the end of the numbers
                break;
            }
        }
        if (i < n) {
            double[] dd = new double[i];
            for (int j = 0; j < i; j++) {
                dd[j] = d[j];
            }
            d = dd;
        }
        return d;
    }

    private void badFileMsg(String message, int line) {
        String postfix = "";
        if (line > 0) {
            postfix = "\n File: " + m_fullfilename + ", Line: " + line;
        }
        Messages.postError("Error reading Chromatogram: " + message
                           + postfix);
    }

    /**
     * Reads all formats of data that it can.  Calls the appropriate
     * routine to do most of the reading.
     */
    private void readData() {
        final int BUFSIZE = 32 * (1 << 10); // 32K
        BufferedReader in = null;
        int nLine = 0;
        /*System.out.println("readData(" + m_filepath + ")");/*CMP*/
        try {
            String line;
            String line2;

            // We only pay attention to the directory part of the path
            String path = m_filepath;
            String givenName = "";
            File fPath = new File(path);
            if (!fPath.isDirectory()) {
                path = fPath.getParent();
                givenName = fPath.getName();
            }

            // Look for the data in this directory
            String name = "lcdata";
            String basename = "LcRun";
            File fPathName = new File(path + File.separator + name);
            if (!fPathName.isFile()) {
                name = "LcRun.hdr";
                fPathName = new File(path + File.separator + name);
            }
            if (!fPathName.isFile()) {
                name = "LcRun001.lcd";
                basename = "LcRun001";
                fPathName = new File(path + File.separator + name);
            }
            if (!fPathName.isFile()) {
                basename = givenName;
                name = basename + ".lcd";
                fPathName = new File(path + File.separator + name);
            }
            m_fullfilename = fPathName.getAbsolutePath();
            m_filepath = path + File.separator + basename;

            try {
                in = new BufferedReader(new FileReader(fPathName), BUFSIZE);
            } catch (FileNotFoundException fnfe) {
                Messages.postError("LcFileRun.readData(): "
                                   + "Cannot open file: " + m_fullfilename);
                return;
            }
            in.mark(BUFSIZE);
            nLine = 1;
            if ((line = in.readLine()) != null
                && (line2 = in.readLine()) != null) {
                if (line.startsWith("Run start time:")
                    || line2.startsWith("Run start time:"))
                {
                    // Lcnmr2000 format
                    in.reset();
                    readLcnmr2000Data(in);
                    m_plot.fillPlot();
                } else if (line.startsWith("; Run start time:")) {
                    // Version 1.0
                    in.reset();
                    readLcVjData(in);
                    m_plot.fillPlot();
                } else if (line.startsWith("; File format version:")) {
                    line = line.substring(22);
                    StringTokenizer toker = new StringTokenizer(line);
                    if (!toker.hasMoreTokens()) {
                        badFileMsg("Unrecognized LcVnmrJ Format", 1);
                    } else {
                        String ver = toker.nextToken();
                        in.reset();
                        readLcVjData(in);
                    }
                    m_plot.fillPlot();
                } else {
                    badFileMsg("Unrecognized Format", 1);
                }
            } else {
                badFileMsg("Empty File", 1);
            }
        } catch (FileNotFoundException fnfe) {
            Messages.postError("Cannot find LC data file: " + m_fullfilename);
        } catch (IOException ioe) {
            badFileMsg(ioe.getLocalizedMessage(), nLine);
            Messages.writeStackTrace(ioe);
        }
        if (m_data != null) {
            m_dataInterval_ms = m_data.getInterval(0);
            Messages.postDebug("LcFileRun",
                               "LcFileRun: dataInterval=" + m_dataInterval_ms);
        }
    }

    /**
     * Reads VJ LC data
     */
    private void readLcVjData(BufferedReader in) {
        Messages.postDebug("readLcVjData(): VJ style LC data");
        try {
            int ndats;
            double[] d = null;
            int nLine = 0;
            String line;
            while ((line = in.readLine()) != null) {
                nLine++;
                // NB: MAKE KEYWORDS CASE INSENSITIVE
                line = line.trim().toLowerCase();
                Messages.postDebug("LcFileRun", "Header line=\"" + line + "\"");

                if (line.startsWith("; run start time")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        dateStamp = line.trim();
                    }
                } else if (line.startsWith("; run title")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        runTitle = line.trim();
                    }
                } else if (line.startsWith("; method")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        m_methodName = line.trim();
                    }
                } else if (line.startsWith("; run type")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        m_typeName = line.trim();
                    }

                } else if (line.startsWith("; number of channels")) {
                    // NB: Does not occur in version 1.x data
                    // In Version 2.x data, this is the first
                    // header line that implies the number of traces.
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        d = parseDoubles(line, "# of channels", nLine);
                        if (d.length >= 1) {
                            m_nTraces = m_nEnabledChans = (int)d[0];
                            Messages.postDebug("ReadLcData",
                                               "readLcVjData: # chans="
                                               + m_nEnabledChans);
                        } else {
                            badFileMsg("Problem with # of channels", nLine);
                        }
                    }
                } else if (line.startsWith("; trace numbers")) {
                    // NB: Does not occur in version 1.x data
                    // Column # to trace # conversion
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        d = parseDoubles(line, "trace numbers", nLine);
                        if (d.length != m_nTraces) {
                            badFileMsg("Wrong number of trace numbers", nLine);
                        } else {
                            m_columnToTrace = new int[m_nTraces];
                            for (int i = 0; i < m_nTraces; i++) {
                                m_columnToTrace[i] = (int)d[i] - 1;
                            }
                        }
                    }
                    
                } else if (line.startsWith("; active channels")) {
                    // NB: In Version 1.x data, this is the first
                    // header line that implies the number of traces.
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        StringTokenizer toker
                                = new StringTokenizer(line, "# ,\t");
                        int n = toker.countTokens();
                        if (n <= MAX_TRACES) {
                            m_nTraces = n;
                        } else {
                            m_nTraces = MAX_TRACES;
                            badFileMsg("Too many channels", nLine);
                        }
                        m_nEnabledChans = 0;
                        m_chanActive = new boolean[m_nTraces];
                        for (int i = 0; i < n && i < m_nTraces; ++i) {
                            String token = toker.nextToken();
                            if (token.equals("TRUE")) {
                                m_chanActive[i] = true;
                                m_nEnabledChans++;
                            } else {
                                m_chanActive[i] = false;
                            }
                        }
                    }

                } else if (line.startsWith("; detection thresholds")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        d = parseDoubles(line, "threshold levels", nLine);
                        if (d.length == m_nTraces) {
                            for (int i = 0; i < m_nTraces; i++) {
                                thresholds[m_columnToTrace[i]] = d[i];
                            }
                        } else {
                            badFileMsg("Problem with threshold levels", nLine);
                        }
                    }

                } else if (line.startsWith("; transfer times")) {
                    // Transfer times for this exp type
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        // NB: Xfer time to NMR == 0
                        d = parseDoubles(line, "xfer times", nLine);
                        if (d.length != m_nTraces) {
                            badFileMsg("Wrong number of xfer times", nLine);
                        } else {
                            for (int i = 0; i < m_nTraces; i++) {
                                m_xferTime[m_columnToTrace[i]] = d[i];
                            }
                        }
                    }

                } else if (line.startsWith("; reference time")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        d = parseDoubles(line, "ref time", nLine);
                        if (d.length >= 1) {
                            refTime_ms = (long)(d[0] * 60000);
                            Messages.postDebug("ReadLcData",
                                               "readLcVjData: refTime_ms="
                                               + refTime_ms);
                        } else {
                            badFileMsg("Problem with reference time", nLine);
                        }
                    }
                    
                } else if (line.startsWith("; channel info")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        StringTokenizer toker = new StringTokenizer(line, ",");
                        int n = toker.countTokens();
                        if (n != m_nTraces) {
                            badFileMsg("Bad 'channel info' line", nLine);
                        }
                        for (int i = 0; i < n && i < m_nTraces; ++i) {
                            String token = " ";
                            if (toker.hasMoreTokens()) {
                                token = toker.nextToken();
                            }
                            channelInfo[m_columnToTrace[i]] = token;
                        }
                    }
                } else if (line.startsWith("; channel names")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        StringTokenizer toker = new StringTokenizer(line, ",");
                        int n = toker.countTokens();
                        if (n != m_nTraces) {
                            badFileMsg("Bad 'channel name' line", nLine);
                        }
                        for (int i = 0; i < n && i < m_nTraces; ++i) {
                            String token = " ";
                            if (toker.hasMoreTokens()) {
                                token = toker.nextToken().trim();
                            }
                            channelName[m_columnToTrace[i]] = token;
                        }
                    }
                } else if (line.startsWith("; channel tags")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        nLine++;
                        StringTokenizer toker = new StringTokenizer(line, ",");
                        int n = toker.countTokens();
                        if (n != m_nTraces) {
                            badFileMsg("Bad 'channel tag' line", nLine);
                        }
                        for (int i = 0; i < n && i < m_nTraces; ++i) {
                            String token = " ";
                            if (toker.hasMoreTokens()) {
                                token = toker.nextToken().trim();
                            }
                            int j = m_columnToTrace[i];
                            channelTag[j] = token;
                            if (token.startsWith(":uv")) {
                                m_uvOffset_ms = (long)(m_xferTime[j] * 60000)
                                        - refTime_ms;
                            }
                        }
                    }
                } else if (line.startsWith("; incident table:")) {
                    Messages.postDebug("ReadLcData",
                                       "Reading incident table");
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    }
                    nLine++;
                    boolean ok = true;
                    ndats = 0;
                    try {
                        ndats = Integer.parseInt(line.trim());
                    } catch (NumberFormatException nfe) {
                        badFileMsg("Problem getting number of incidents",
                                   nLine);
                        ok = false;
                    }
                    Messages.postDebug("ReadLcData",
                                       "Number of incidents=" + ndats);
                    int i;
                    for (i = 0;
                         ok && i < ndats && (line = in.readLine()) != null;
                         ++i)
                    {
                        Messages.postDebug("ReadLcData",
                                           "Incident line: " + line);
                        nLine++;
                        // Read incident
                        d = parseDoubles(line, "incident", nLine);
                        if (d.length < 5) {
                            badFileMsg("Problem reading incident", nLine);
                            ok = false;
                        }
                        int chan = (int)d[0] - 1;
                        long pk_ms = (long)(d[1] * 60000);
                        long xfer_ms = (long)(d[2] * 60000);
                        long actual_ms = (long)(d[3] * 60000);
                        long length_ms = (long)(d[4] * 60000);
                        int loop = (int)d[5];
                        Messages.postDebug("ReadLcData",
                                           "Incident " + (i + 1)
                                           + ", pk_ms=" + pk_ms
                                           + ", actual_ms=" + actual_ms
                                           + ", length_ms=" + length_ms);/*CMP*/

                        EventTrig trigger = EventTrig.UNKNOWN;
                        String command = "unknown";
                        StringTokenizer toker = new StringTokenizer(line, ",");
                        for (int j = 0; j < d.length; j++) {
                            toker.nextToken();
                        }
                        if (toker.hasMoreTokens()) {
                            String trigLabel = toker.nextToken().trim();
                            trigger = EventTrig.getType(trigLabel);
                        }
                        if (toker.hasMoreTokens()) {
                            command = toker.nextToken("").trim();
                            command = command.substring(1); // Strip ","
                        }

                        LcEvent lce = new LcEvent(pk_ms,
                                                  xfer_ms,
                                                  chan + 1,
                                                  loop,
                                                  trigger,
                                                  command);
                        if (command.startsWith("timeSlice")
                            && command.indexOf("'n'") < 0
                            && command.indexOf("'off'") < 0)
                        {
                            lce.markType = MarkType.SLICE;
                        } else if (command.startsWith("stopFlow")) {
                            if (EventTrig.USER_HOLD.equals(trigger)) {
                                lce.markType = MarkType.HOLD;
                            } else {
                                lce.markType = MarkType.PEAK;
                            }
                        } else if (EventTrig.PEAK_DETECT.equals(trigger)) {
                            lce.markType = MarkType.PEAK;
                        } else if (command.indexOf("Inject y") >= 0) {
                            lce.markType = MarkType.INJECT_ON;
                        } else if (command.indexOf("Inject n") >= 0) {
                            lce.markType = MarkType.INJECT_OFF;
                        } else {
                            lce.markType = MarkType.COMMAND;
                        }                          
                        lce.length_ms = length_ms;
                        lce.actual_ms = actual_ms;
                        lce.peakNumber = i + 1;
                        eventQueue.add(lce);
                        /*Messages.postDebug("Incident " + i
                                           + ", chan " + chan
                                           + ", pk " + pk_ms
                                           + ", xfer " + xfer_ms
                                           + ", actual " + actual_ms
                                           + ", length " + length_ms);/*CMP*/
                        /*Messages.postDebug("eventQueue length="
                                           + eventQueue.size());/*CMP*/
                    }
                    if (ok && i < ndats) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    }

                } else if (line.startsWith("; data:")) {
                    // Make LcData with correct size and trace numbers
                    m_data = new LcData(m_columnToTrace, null, null);

                    if ((line = in.readLine()) == null
                        || line.trim().length() == 0)
                    {
                        // Read data from a separate file
                        try {
                            in.close();
                        } catch (IOException ioe) {}
                        m_fullfilename = m_filepath+".data";
                        in = new BufferedReader
                                (new FileReader(m_fullfilename));
                        readDataOnly(in, nLine);
                        
                        //badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        Messages.postDebug("readLcVjData(): data is in-line");
                        double refTime = refTime_ms / 60000.0;
                        nTransTime_ms = new long[MAX_TRACES];
                        for (int i = 0; i < MAX_TRACES; i++) {
                            nTransTime_ms[i] = (long)(m_xferTime[i] * 60000);
                            nTransTime_ms[i] -= refTime_ms;
                        }
                        m_plot.removeLegendButtons();
                        int nEnabledChans = 0;
                        for (int c = 0; c < m_nTraces; ++c) {
                            if (m_chanActive[c]) {
                                int trace = m_columnToTrace[c];
                                nEnabledChans++;
                                m_plot.addLegendButton(trace, 2 * trace,
                                                       "" + (char)('1' + trace),
                                                       channelName[trace]);
                                m_plot.addLegendButton(trace, 2 * trace + 1,
                                                       null);
                            }
                        }

                        // Don't do auto-scaling on threshold lines.
                        int[] scalingTraces = new int[nEnabledChans];
                        for (int i = 0, j = 0; i < m_nTraces; i++) {
                            //int trace = m_columnToTrace[i];
                            if (m_chanActive[i]) {
                                scalingTraces[j++] = 2 * m_columnToTrace[i];
                            }
                        }
                        m_plot.setScalingDataSets(scalingTraces);

                        nLine++;
                        ndats = 0;
                        int i = 0;
                        try {
                            ndats = Integer.parseInt(line);
                            //m_data = new LcData(ndats, m_nTraces);
                        } catch (NumberFormatException nfe) {
                            // "ndats" line is not required
                            // Assume it's data
                            //m_data = new LcData(ndats, m_nTraces);
                            ++i;
                            d = parseDoubles(line, "chromatograph data", nLine);
                            if (d.length < m_nTraces + 1) {
                                badFileMsg("Problem reading data", nLine);
                                break;
                            }
                            m_data.add(d);
                            for (int c = 0; c < m_nTraces; ++c) {
                                if (m_chanActive[c]) {
                                    double chanTime;
                                    int trace = m_columnToTrace[c];
                                    chanTime = d[0]
                                            + nTransTime_ms[trace] / 60000.0;
                                    m_plot.addPoint(2 * trace, chanTime,
                                                  d[c+1],
                                                  true, false);
                                    // NB: Disable threshold lines
                                    //m_plot.addPoint(2 * trace + 1, chanTime,
                                    //              thresholds[c],
                                    //              true, false);
                                }
                            }
                        }
                        for ( ; (line = in.readLine()) != null; ++i) {
                            nLine++;
                            d = parseDoubles(line, "chromatograph data", nLine);
                            if (d.length < m_nTraces + 1) {
                                badFileMsg("Problem reading data", nLine);
                                break;
                            }
                            m_data.add(d);
                            m_dataFlowTime_ms = (long)(d[0] * 60000);
                            for (int c = 0; c < m_nTraces; ++c) {
                                if (m_chanActive[c]) {
                                    double chanTime;
                                    int trace = m_columnToTrace[c];
                                    chanTime = d[0]
                                            + nTransTime_ms[trace] / 60000.0;
                                    m_plot.addPoint(2 * trace, chanTime,
                                                  d[c+1],
                                                  true, false);
                                    // NB: Disable threshold lines
                                    //m_plot.addPoint(2 * trace + 1, chanTime,
                                    //              thresholds[c],
                                    //              true, false);
                                }
                            }
                        }

                        // Add event markers
                        for (LcEvent event: eventQueue) {
                            int iChan = event.channel - 1;
                            double x = event.time_ms / 60000.0;
                            double y = DEFAULT_Y_POSITION;
                            if (event.markType == MarkType.SLICE) {
                                y = SLICE_Y_POSITION;
                            } else if (event.markType == MarkType.HOLD
                                       || event.markType == MarkType.PEAK)
                            {
                                if (iChan < 0) {
                                    y = HOLD_Y_POSITION;
                                } else {
                                    y = m_plot.getDataValueAt(x, 2 * iChan);
                                }
                            }
                            m_plot.addMark(x, y, event);
                        }
                    }
                } else {
                    Messages.postDebug("lcFile",
                                       "Uninterpreted line in: "
                                       + m_fullfilename + ": " + nLine
                                       + ": \"" + line + "\"");
                }
            }
        } catch (IOException ioe) {
            Messages.postError("Error reading Cromatogram: " + m_fullfilename);
            return;
        }

        try {
            in.close();
        } catch (IOException ioe) {}


        // Draw the plot

        setPlotTitle();
        m_plot.repaint();
        try {
            in.close();
        } catch (IOException ioe) {}
    }

    /**
     * @param in The BufferedReader to use to read the data.
     * @param nLine The line # of the first line read (for error messages).
     */
    private void readDataOnly(BufferedReader in, int nLine) {
        Messages.postDebug("readDataOnly(): data is in separate file");
        double refTime = refTime_ms / 60000.0;
        for (int i = 0; i < MAX_TRACES; i++) {
            nTransTime_ms[i] = (long)(m_xferTime[i] * 60000);
            nTransTime_ms[i] -= refTime_ms;
        }
        m_plot.removeLegendButtons();
        int nEnabledChans = 0;
        for (int c = 0; c < m_nTraces; ++c) {
            if (m_chanActive[c]) {
                nEnabledChans++;
                int trace = m_columnToTrace[c];
                m_plot.addLegendButton(trace, 2 * trace,
                                       "" + (char)('1' + trace),
                                       channelName[trace]);
                m_plot.addLegendButton(trace, 2 * trace + 1, null);
            }
        }

        // Don't do auto-scaling on threshold lines.
        int[] scalingTraces = new int[nEnabledChans];
        for (int i = 0, j = 0; i < m_nTraces; i++) {
            //int trace = m_columnToTrace[i];
            if (m_chanActive[i]) {
                scalingTraces[j++] = 2 * m_columnToTrace[i];
            }
        }
        m_plot.setScalingDataSets(scalingTraces);

        nLine++;
        int ndats = 0;
        double[] d = null;
        //m_data = new LcData(ndats, m_nTraces);
        String line;
        try {
            for (int i = 0 ; (line = in.readLine()) != null; ++i) {
                nLine++;
                d = parseDoubles(line, "chromatogram data", nLine);
                if (d.length < m_nTraces + 1) {
                    badFileMsg("Problem reading data", nLine);
                    break;
                }
                m_data.add(d);
                m_dataFlowTime_ms = (long)(d[0] * 60000);

                boolean pkDetectOn = false;
                for (int c = 0; c < m_nTraces; ++c) {
                    if (m_chanActive[c]) {
                        double chanTime;
                        int trace = m_columnToTrace[c];
                        chanTime = d[0] + nTransTime_ms[trace] / 60000.0;
                        m_plot.addPoint(2 * trace, chanTime,
                                        d[c+1],
                                        true, false);
                        if (thresholds[c] >= 0) {
                            // NB: Disable threshold lines
                            //m_plot.addPoint(2 * trace + 1, chanTime,
                            //                thresholds[c],
                            //                pkDetectOn, false);
                            pkDetectOn = true;
                        } else {
                            pkDetectOn = false;
                        }
                    }
                }
            }
        } catch (IOException ioe) {
            Messages.postError("Error reading Cromatogram: "
                               + m_filepath+"...");
            return;
        }

        // Add event markers
        for (LcEvent event: eventQueue) {
            int iChan = event.channel - 1;
            double x = event.time_ms / 60000.0;
            double y = DEFAULT_Y_POSITION;
            if (event.markType == MarkType.SLICE) {
                y = SLICE_Y_POSITION;
            } else if (event.markType == MarkType.HOLD
                       || event.markType == MarkType.PEAK)
            {
                if (iChan < 0) {
                    y = HOLD_Y_POSITION;
                } else {
                    y = m_data.getValueAt(iChan,
                                          event.time_ms - nTransTime_ms[iChan]);
                }
            }
            m_plot.addMark(x, y, event);
        }
    }
        


    /**
     * Reads old format data (Lcnmr2000)
     */
    private void readLcnmr2000Data(BufferedReader in) {
        Messages.postDebug("readLcnmr2000Data(): VB style LC data");
        try {
            int ndats;
            String line;
            double[] d = null;
            double[] xferTime = new double[LC2000_TRACES];
            boolean[] chanActive = new boolean[LC2000_TRACES];
            int nLine = 0;
            m_typeName="stop-flow";
            m_plot.removeLegendButtons();
            while ((line = in.readLine()) != null) {
                nLine++;
                line = line.trim();
                if (line.startsWith("Run start time: ")) {
                    dateStamp = line.substring(16);
                } else if (line.equalsIgnoreCase("; Experiment type:")) {
                    if ((line = in.readLine()) != null) {
                        nLine++;
                        m_typeName = line.trim();
                    }
                    if (m_typeName.equals("stop flow")) {
                        m_typeName = "stop-flow";
                    }
                } else if (line.indexOf(m_typeName) >= 0) {
                    // Transfer times for this exp type
                    if ((line = in.readLine()) != null) {
                        nLine++;
                        d = parseDoubles(line, "xfer times", nLine);
                        if (d.length >= LC2000_TRACES) {
                            xferTime = d;
                        }
                    }
                } else if (line.equalsIgnoreCase("; active channels:")) {
                    if ((line = in.readLine()) != null) {
                        nLine++;
                        StringTokenizer toker
                                = new StringTokenizer(line, "# ,\t");
                        int n = toker.countTokens();
                        for (int i = 0; i < n && i < LC2000_TRACES; ++i) {
                            String token = toker.nextToken();
                            chanActive[i] = token.equalsIgnoreCase("TRUE");
                        }
                    }
                } else if (line.equalsIgnoreCase("; incident table:")) {
                    if ((line = in.readLine()) == null) {
                        break;
                    }
                    nLine++;
                    try {
                        ndats = Integer.parseInt(line.trim());
                    } catch (NumberFormatException nfe) {
                        Messages.postError("Bad Chromatograph file: "
                                           + m_fullfilename
                                           + "\n Number of incidents = "
                                           + "\"" + line + "\"");
                        break;
                    }
                    for (int i = 0;
                         i < ndats && (line = in.readLine()) != null;
                         ++i)
                    {
                        nLine++;
                        // Read incident
                        d = parseDoubles(line, "incident", nLine);
                        if (d.length < 7) {
                            Messages.postError("Bad Chromatograph file: "
                                               + m_fullfilename
                                               + "\n Incident = "
                                               + "\"" + line + "\"");
                            break;
                        }
                        int chan = (int)d[0] - 1;
                        long pk_ms = (long)(d[2] * 60000);
                        long xfer_ms = (long)(xferTime[chan] * 1000);
                        int loop = (int)d[6];
                        LcEvent lce = new LcEvent(pk_ms,
                                                  xfer_ms,
                                                  chan + 1,
                                                  loop,
                                                  EventTrig.PEAK_DETECT,
                                                  "");
                        eventQueue.add(lce);
                    }
                } else if (line.equalsIgnoreCase("; detection thresholds:")) {
                    if ((line = in.readLine()) != null) {
                        nLine++;
                        d = parseDoubles(line, "threshold levels", nLine);
                        if (d.length >= LC2000_TRACES) {
                            thresholds = d;
                            for (int i = 0; i < LC2000_TRACES; i++) {
                                thresholds[i] = d[i];
                            }
                        }
                    }
                } else if (line.equalsIgnoreCase("; data:")) {
                    if ((line = in.readLine()) == null) {
                        badFileMsg("File ends prematurely", nLine);
                        break;
                    } else {
                        double refTime = refTime_ms / 60000.0;
                        for (int c = 0; c < LC2000_TRACES; ++c) {
                            if (chanActive[c]) {
                                m_plot.addLegendButton(c, 2 * c,
                                                     "" + (char)('1' + c),
                                                       channelName[c]);
                                m_plot.addLegendButton(c, 2 * c + 1, null);
                            }
                        }

                        nLine++;
                        try {
                            ndats = Integer.parseInt(line.trim());
                        } catch (NumberFormatException nfe) {
                            badFileMsg("Problem reading number of data points",
                                       nLine);
                            break;
                        }
                        m_data = new LcData(ndats, LC2000_TRACES);
                        for (int i = 0;
                             i < ndats && (line = in.readLine()) != null;
                             ++i)
                        {
                            nLine++;
                            d = parseDoubles(line, "chromatograph data", nLine);
                            if (d.length < LC2000_TRACES + 1) {
                                badFileMsg("Problem reading data", nLine);
                                break;
                            }
                            m_data.add(d);
                            m_dataFlowTime_ms = (long)(d[0] * 60000);
                            for (int c = 0; c < LC2000_TRACES; ++c) {
                                if (chanActive[c]) {
                                    double chanTime = d[0] + xferTime[c] / 60;
                                    m_plot.addPoint(2 * c, chanTime,
                                                  d[c+1],
                                                  true, false);
                                    // NB: Disable threshold lines
                                    //m_plot.addPoint(2 * c + 1, chanTime,
                                    //              thresholds[c],
                                    //              true, false);
                                }
                            }
                        }

                        // Add event markers
                        try {
                            for (LcEvent event = eventQueue.first();
                                 event != null;
                                 event = eventQueue.first())
                            {
                                eventQueue.remove(event);
                                event.actual_ms = m_dataFlowTime_ms;
                                int iChan = event.channel - 1;
                                double x = event.time_ms / 60000.0;
                                double y = m_plot.getDataValueAt(x, 2 * iChan);
                                m_plot.addMark(x, y, event);
                            }
                        } catch (NoSuchElementException nsee) {
                            break; // Out of file reader
                        }
                    }
                } else {
                    Messages.postDebug("lcFile",
                                       "Uninterpreted line in: "
                                       + m_fullfilename + ": " + nLine
                                       + ": \"" + line + "\"");
                }
            }
        } catch (IOException ioe) {
            Messages.postError("Error reading Cromatogram: " + m_fullfilename);
        }

        // Draw the plot
        m_plot.setTitle(null);
        m_plot.repaint();
        try {
            in.close();
        } catch (IOException ioe) {}
    }
}
