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
import java.net.*;
import java.util.*;

import javax.swing.SwingUtilities;

import vnmr.util.*;
import vnmr.ui.*;


public class LcSlimIO implements LcDef {

    public static final int N_ADCS = 3;

    private final static int SOCKET_CONNECT_TIMEOUT = 10000; // ms

    // Define bits in SLIM I/O register
    private final static byte FAULT = 0x01;
    private final static byte PUMP_START = 0x02;
    private final static byte NMR_TRIG = 0x04;
    private final static byte SF_FLOW = 0x08;
    private final static byte SF_STOP = 0x10;
    private final static byte AUX_OUT = 0x20;
    private final static byte RESERVED_OUT = 0x40;

    private final static int PULSE_WIDTH = 200; // ms
    private final static int HEARTBEAT_DELAY = 2000;
    private final static int LOOP_STEP_DELAY = 2000;
    private final static int LOOP_READBACK_DELAY = 200;
    private final static int LOOP_RETRIES = 50;

    private final static int CMD_RA = ('R' << 8) + 'A';
    private final static int CMD_LN = ('L' << 8) + 'N';
    private final static int CMD_TL = ('T' << 8) + 'L';
    private final static int CMD_TN = ('T' << 8) + 'N';
    private final static int CMD_RH = ('R' << 8) + 'H';
    private final static int CMD_JV = ('J' << 8) + 'V';
    private final static int CMD_PF = ('P' << 8) + 'F';
    private final static int CMD_PP = ('P' << 8) + 'P';
    private final static int CMD_RV = ('R' << 8) + 'V';
    private final static int CMD_LV = ('L' << 8) + 'V';
    private final static int CMD_XG = ('X' << 8) + 'G';

    private static int outputByte = 0; // Shadow the SLIM's byte register
    private Socket m_socket = null;
    private SlimReader m_reader = null;
    private LcControl controller = null;
    private LcRun run = null;
    private BufferedReader inputStream = null;
    private PrintWriter outputStream = null;
    private SortedSet<PulseValue> pulseList = new TreeSet<PulseValue>();
    private javax.swing.Timer heartbeatTimer;
    private javax.swing.Timer loopTimer;
    private javax.swing.Timer msFlowTimer;
    private javax.swing.Timer msPressureTimer;
    private boolean expectHeartbeats = false;
    private boolean m_checkFailed = false;
    private String m_hostName = "";
    private int m_portNumber = 0;
    private int m_desiredLoop = 0;
    private String m_loopCommand = null;
    private long m_loopStepping = 0;
    private boolean m_isNewRunSegment = false;
    private long m_lastDataTime = Long.MAX_VALUE;
    private long m_adcStartTime = 0;
    private boolean m_adcRunning = false;
    private int m_adcRate_ms = 250;
    private long m_dbgTime = 0;
    private int m_loopCommFail = 0;
    private String m_slimVersion = "";
    private boolean m_gaveSlimWarning = false;
    private boolean m_loopOutOfRange = false;
    private PrintWriter m_dataWriter = null;
    private StatusManager m_statusManager = null;
    private int m_uvStatusChan;

    private int[] m_activeChannels = new int[0];

    // For timing test:
    private long m_timePrevPoint = 0;
    private int m_timePtCount = 0;
    private PrintWriter m_timeWriter = null;

    /**
     * Initialize I/O controller to SLIM, but do not connect.
     * @param lcControl The "master" for LC control.
     */
    public LcSlimIO(LcControl lcControl) {
        controller = lcControl;

        // Set up heartbeat monitor
        ActionListener heartbeatFail = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                Messages.postDebug("slimIO", "Heartbeat failed");
                if (expectHeartbeats) {
                    Messages.postError("No heartbeat from SLIM.  Close port.");
                    closePort();
                }
            }
        };
        heartbeatTimer = new javax.swing.Timer(HEARTBEAT_DELAY, heartbeatFail);
        heartbeatTimer.setRepeats(false);

        ActionListener loopCheck = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                Messages.postDebug("slimIO",
                                   "Didn't get expected loop postition");
                if (m_loopCommFail < 0) {
                    return;     // Got a reply already
                }
                if (++m_loopCommFail > LOOP_RETRIES) {
                    Messages.postError("No communication with Analyte Collector");
                    m_loopCommFail = 0;
                } else {
                    // Try to get going again
                    getLoopPosition();
                    /*Messages.postDebug("Started loop timer");/*CMP*/
                }
            }
        };
        loopTimer = new javax.swing.Timer(LOOP_STEP_DELAY, loopCheck);
        loopTimer.setRepeats(false);

        // TODO: Make update rate much slower if ms pump is not responding.
        ActionListener msFlowStatus = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                readMakeupFlow();
            }
        };
        msFlowTimer = new javax.swing.Timer(1000, msFlowStatus);
        msFlowTimer.setInitialDelay(0);
        msFlowTimer.setRepeats(true);
        
        ActionListener msPressureStatus = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                readMakeupPressure();
            }
        };
        msPressureTimer = new javax.swing.Timer(1000, msPressureStatus);
        msPressureTimer.setInitialDelay(0);
        msPressureTimer.setRepeats(true);
    }

    /**
     * Open a file to save the SLIM data in.
     */
    private boolean openDataWriter(String basePath) {
        closeDataWriter();
        String outpath = basePath + ".slimData";
        BufferedWriter bw = null;
        try {
            boolean append = true;
            bw = new BufferedWriter(new FileWriter(outpath, append));
        } catch (IOException ioe) {
            Messages.postError("Cannot open SLIM Data output file: " + outpath);
            return false;
        }
        m_dataWriter = new PrintWriter(bw);
        synchronized (m_dataWriter) {
            if (m_dataWriter != null) {
                if (DebugOutput.isSetFor("SlimTiming")) {
                    // Init timing test
                    m_timePrevPoint = m_timePtCount = 0;
                    outpath = "/tmp/lcTimingSlim";
                    try {
                        bw = new BufferedWriter(new FileWriter(outpath));
                        m_timeWriter = new PrintWriter(bw);
                    } catch (IOException ioe) {
                        Messages.postError("Cannot open SLIM Timing file: "
                                           + outpath);
                    }
                }
            }
        }
        return true;
    }

    /**
     * Write out one line of SLIM data.
     */
    private void writeData(long time_ms, double[] data) {
        if (m_dataWriter == null) {
            return;
        }
        synchronized (m_dataWriter) {
            // In case run ended and m_dataWriter was nulled while we waited:
            if (m_dataWriter != null) {
                double t_minutes = time_ms / 60000.0;
                m_dataWriter.print(Fmt.f(5, t_minutes, false));
                int len = data == null ? 0 : data.length;
                for (int i = 0; i < len; i++) {
                    m_dataWriter.print("," + Fmt.f(5, data[i], false));
                }
                m_dataWriter.println();
                m_dataWriter.flush();   // Save data in real time.
                if (DebugOutput.isSetFor("SlimTiming")) {
                    // Timing test output
                    if (m_timePrevPoint == 0) {
                        m_timePrevPoint = System.currentTimeMillis();
                        m_timePtCount++;
                    } else {
                        long now = System.currentTimeMillis();
                        if (m_timeWriter != null) {
                            m_timeWriter.println(m_timePtCount + " "
                                                 + (now - m_timePrevPoint));
                        }
                        m_timePrevPoint = now;
                        m_timePtCount++;
                    }
                }
            }
        }
    }

    /**
     * Close the data file.
     */
    public void closeDataWriter() {
        if (m_dataWriter != null) {
            synchronized (m_dataWriter) {
                if (m_dataWriter != null) {
                    Messages.postDebug("slimIO", "*** Closing data writer ***");
                    m_dataWriter.close();
                    m_dataWriter = null;
                }
            }
        }
        if (m_timeWriter != null) {
            m_timeWriter.close();
            m_timeWriter = null;
        }
    }

    /**
     * Returns the number of traces that need data from an ADC.
     */
    public int countActiveChannels() {
        int count = 0;
        for (int i = 0; i < m_activeChannels.length; i++) {
            if (m_activeChannels[i] >= 0) {
                count++;
            }
        }
        return count;
    }

    /**
     * Selects which channels we pay attention to.
     */
    public void setActiveChannels(int[] activeChannels) {
        m_activeChannels = activeChannels;
        if (countActiveChannels() == 0 && m_adcRunning) {
            stopAdc();
        } else if (!m_adcRunning) {
            startAdc(m_adcRate_ms);
        }
    }

    private boolean checkPort() {
        if (m_socket == null) {
            // Only happens if it has never tried to connect,
            // or has closed the connection.
            return controller.openSlimPort();
        } else {
            return true;
        }
    }

    private void println(String msg) {
        if (DebugOutput.isSetFor("slimIO") || DebugOutput.isSetFor("slimIO+")) {
            boolean common
                    = msg.equals("MR_FLOWRATE") || msg.equals("MR_PRESSURE");
            if (!common || DebugOutput.isSetFor("slimIO+")) {
                Messages.postDebug("LcSlimIO.println(" + msg + ")");
            }
        }
        if (outputStream == null) {
            if (!m_gaveSlimWarning && controller.useSlim()) {
                Messages.postDebug("Cannot send message - no SLIM connection");
                m_gaveSlimWarning = true;
            }
        } else  {
            outputStream.println(msg);
        }
    }

    public boolean openPort(String hostName, int portNumber) {
        return openPort(hostName, portNumber, false);
    }

    /**
     * Open a Socket to communicate with the SLIM box.
     * @param hostName The host name of the SLIM (should be in /etc/hosts)
     * @param portNumber Which port to use for communication
     * @param ifCheck If true, only open port if it is currently closed.
     */
    public boolean openPort(String hostName, int portNumber, boolean ifCheck) {
        m_hostName = hostName;
        m_portNumber = portNumber;

        if (!controller.useSlim()) {
            // Don't even try to connect
            if (!m_gaveSlimWarning) {
                Messages.postWarning("SLIM COMMUNICATION DISABLED: "
                                     + "\"NoSlim\" debug flag is set");
            }
            m_checkFailed = true;
            m_gaveSlimWarning = true;
            return false;
        }
        m_gaveSlimWarning = false;
        if (ifCheck && isPortOpen() && m_hostName.equals(hostName)) {
            return true;        // Port checks OK
        } else if (isPortOpen()) {
            closePort();
        }
        m_checkFailed = false;

        Messages.postDebug("slimIO", "LcSlimIO.openPort(): hostName="
                           + hostName);

        try {
            Messages.postDebug("slimIO",
                               "SlimIO.openPort(): creating socket ...");
            m_socket = new Socket();
            SocketAddress socketAddr
                    = new InetSocketAddress(hostName, portNumber);
            m_socket.connect(socketAddr, SOCKET_CONNECT_TIMEOUT);
            Messages.postDebug("slimIO",
                               "SlimIO.openPort(): ... got socket");
            try {
                m_socket.setSoTimeout(10000);
            } catch (SocketException se) {
                Messages.postDebug("Cannot set timeout on SLIM communication");
            }
            outputStream = new PrintWriter(m_socket.getOutputStream(), true);
            inputStream = new BufferedReader
                    (new InputStreamReader(m_socket.getInputStream()));
            m_reader = new SlimReader(inputStream);
            m_reader.start();
        } catch (UnknownHostException uhe) {
            Messages.postError("Don't know about host: " + hostName);
        } catch (IOException ioe) {
            Messages.postError("Couldn't get socket for "
                               + "the connection to: " + hostName
                               + "\n" + ioe);
        }

        Messages.postDebug("slimIO",
                           "openPort(): m_socket = " + m_socket);


        boolean ok = (m_socket != null);
        if (ifCheck && !ok) {
            m_checkFailed = true;
        }
        if (ok) {
            //stopAdc();
            startAdc(250);
            setMakeupFlowQueryRate(1000);
            setMakeupPressQueryRate(1000);
        }
        return ok;
    }

    public void closePort() {
        if (m_socket != null) {
            heartbeatTimer.stop();
            try {
                m_socket.close();
            } catch (IOException ioe) {}
            m_socket = null;
            inputStream = null;
            outputStream = null;
            m_checkFailed = false;
        }
        Messages.postDebug("slimIO", "m_socket closed");
    }

    public boolean isPortOpen() {
        return (m_socket != null);
    }

    public boolean isPortCheckBad() {
        return m_checkFailed;
    }

    /**
     * Encodes 7-bit ASCII string into an int for using in switch statements.
     * Only good for strings <= 4 characters!
     */
    private int convertAsciiToInt(String str) {
        int rtn = 0;
        for (int i = 0; i < str.length(); i++) {
            rtn = (rtn << 8) + str.charAt(i);
        }
        return rtn;
    }

    /**
     * Decode a String from the SLIM box and take appropriate action.
     */
    synchronized public void readMessage(String strMsg) {
        if (DebugOutput.isSetFor("slimIO") || DebugOutput.isSetFor("slimIO+")) {
            boolean common = strMsg.startsWith("RA ")
                    || strMsg.startsWith("PP ")
                    || strMsg.startsWith("PF ");
            if (!common || DebugOutput.isSetFor("slimIO+")) {
                Messages.postDebug("Message from SLIM"
                                   + ": time=" + System.currentTimeMillis()
                                   + ", msg=\"" + strMsg + "\"");
            }
        }

        // First token is the command name
        StringTokenizer toker = new StringTokenizer(strMsg);
        String cmd = "";
        int nTokens = toker.countTokens();
        if (nTokens > 0) {
            cmd = toker.nextToken();
            --nTokens;
        }

        int iCmd = convertAsciiToInt(cmd);
        switch (iCmd) {
        case CMD_RA:
            {
                // Got ADC data: "RA time dat1 dat2 ..."
                /*
                 * NB: All ADC data is converted to "AU", which
                 * has the range -2 to +2.  This is good for
                 * the 9050 UV detector, but may not be so great
                 * for anything else.
                 */
                {
                    long slimTime = 0;  // Flow time according to SLIM 
                    if (nTokens > 0) {
                        slimTime = Long.parseLong(toker.nextToken());
                        --nTokens;
                    }

                    double[] dat = new double[nTokens];
                    for (int i = 0; i < nTokens; ++i) {
                        int j = i * 2;
                        dat[i] = Double.parseDouble(toker.nextToken());
                        dat[i] /= 3.2768; // ADC to mV conversion
                        dat[i] *= 0.0002; // mV to AU conversion
                        // TODO: getMvToAu() not functional
                        //dat[i] *= getMvToAu(i); // mV to AU conversion
                    }
                    // Correct the offset on the UV channel.
                    dat = controller.correctChannelValues(dat);

                    // Send data to current run.
                    // We use the SLIM time here instead of Java time because
                    // of possible of jitter in the interrupt latency.
                    // TODO: Correct SLIM time for drift?
                    long flowTime = slimTime;
                    if (run != null && run.runActive && !run.flowPaused) {
                        // Don't use pre-pause data just after we start the ADC
                        if (!m_isNewRunSegment || slimTime < m_lastDataTime) {
                            // NB: Correct SLIM time for previous pauses!
                            flowTime += run.getPrevSegTime_ms();
                            appendChannelValues(flowTime, dat);
                            m_isNewRunSegment = false;
                        }
                    }
                    m_lastDataTime = slimTime;

                    updateStatus(dat);

                    if (DebugOutput.isSetFor("lcTiming")) {
                        long time2; // Flow time according to Java
                        time2 = System.currentTimeMillis() - m_adcStartTime;
                        if (run != null) {
                            time2 = run.getFlowTime_ms();
                        }
                        Messages.postDebug("slimIO.readMessage(): "
                                           + "clock - SLIM = "
                                           + (time2 - flowTime));
                        Messages.postDebug("... readMessage() done");
                    }
                }
            }
            break;
        case CMD_LN:
            {
                // Got the loop number from analyte collector: "LN loop#"
                loopTimer.stop();
                m_loopCommFail = -1;
                m_loopStepping = 0;
                int position = 0;
                if (toker.hasMoreTokens()) {
                    position = Integer.parseInt(toker.nextToken());
                }
                int maxLoop = controller.config.getNumberOfLoops();
                if (position <= 0 || position > maxLoop) {
                    if (! m_loopOutOfRange) {
                        Messages.postError("Got the illegal loop number \""
                                           + position
                                           + "\" from Analyte Collector\n"
                                           + "  ...Max loop number is "
                                           + maxLoop);
                    }
                    m_loopOutOfRange = true;
                    position = 0;
                } else {
                    m_loopOutOfRange = false;
                }
                controller.updateLoopPosition(position);
                if (m_desiredLoop != 0 && m_desiredLoop != position) {
                    // We are going to a requested loop number
                    // This could just be from a random LN query
                } else if (m_desiredLoop != 0) {
                    m_desiredLoop = 0;
                    /*Messages.postDebug("serialEvent: sendToVnmr("
                      + m_loopCommand + ")");/*CMP*/

                    // Do what was requested on reaching the loop
                    controller.sendToVnmr(m_loopCommand);
                    m_loopCommand = null;
                }
            }
            break;
        case CMD_TL:
            {
                // Got a trigger - meaning time to start LC
                // Don't need this with VJ
            }
            break;
        case CMD_TN:
            {
                // Got a trigger from NMR - meaning NMR has finished
                // TODO: Eliminate this - have Vnmr perform action directly
                Messages.postDebug("nmrComm", "Trig From NMR: " + strMsg);
                if (run != null) {
                    run.trigFromNmr();
                }
            }
            break;
        case CMD_RH:
            {
                // Got a heartbeat
                /*System.out.println("Got heartbeat: timer running="
                  + heartbeatTimer.isRunning());/*CMP*/
                heartbeatTimer.restart(); // So we don't time out
            }
            break;
        case CMD_JV:
            {
                // Got a version string
                m_slimVersion = "";
                if (toker.hasMoreTokens()) {
                    m_slimVersion = toker.nextToken();
                }
                Messages.postInfo("SLIM version: " + m_slimVersion);
            }
            break;
        case CMD_PF:
            {
                Messages.postDebug("makeupPump", "Msg from SLIM: " + strMsg);
                if (toker.hasMoreTokens()) {
                    String val = toker.nextToken();
                    double flow = 0;
                    try {
                        // Eliminate spurious precision (and scale it)
                        flow = Double.parseDouble(val) / 20;
                        val = Fmt.f(2, flow, false);
                    } catch (NumberFormatException nfe) {}
                    controller.sendStatusMessage("makeupFlow "
                                                 + val + " mL/min");
                    if (flow > 0) {
                        controller.sendStatusMessage("makeupStatus on");
                    } else {
                        controller.sendStatusMessage("makeupStatus off");
                    }
                }
            }
            break;
        case CMD_PP:
            {
                Messages.postDebug("makeupPump", "Msg from SLIM: " + strMsg);
                if (toker.hasMoreTokens()) {
                    String val = toker.nextToken();
                    try {
                        // Eliminate spurious precision
                        //val = Fmt.f(0, Double.parseDouble(val), false);
                        double dval = Double.parseDouble(val);
                        dval = 10 * Math.round(dval / 10);
                        val = Fmt.f(0, dval, false);
                    } catch (NumberFormatException nfe) {}
                    controller.sendStatusMessage("makeupPressure "
                                                 + val + " PSI");
                }
            }
            break;
        case CMD_RV:
            {
                if (toker.hasMoreTokens()) {
                    int val = toker.nextToken().equals("A") ? 0 : 1;
                    controller.sendToVnmr("lcNmrValve=" + val);
                }
            }
            break;
        case CMD_LV:
            {
                if (toker.hasMoreTokens()) {
                    int val = toker.nextToken().equals("A") ? 1 : 0;
                    controller.sendToVnmr("lcColumnValve=" + val);
                }
            }
            break;
        case CMD_XG:
            {
                if (toker.hasMoreTokens()) {
                    try {
                        int val = Integer.parseInt(toker.nextToken());
                        int maxLoop = controller.config.getNumberOfLoops();
                        if (val != maxLoop) {
                            println("XL " + maxLoop);
                            Messages.postInfo("SLIM reprogrammed for " + maxLoop
                                              + " loop analyte collector");
                        }
                    } catch (NumberFormatException nfe) {
                    }
                }
            }
            break;
        default:
            {
                Messages.postDebug("Strange message from LC SLIM box: \""
                                   + strMsg + "\"");
            }
            break;
        }
    }

    // TODO: Rework getMvToAu() to take user specified scaling
    public double getMvToAu(int chan) {
        double rtn = 0.0002;    // Default scaling for UV... or unknown
        String cname = controller.config.getChannelName(chan);
        if(controller.config.isTraceEnabled(chan)) {
            if (cname != null) {
                if (cname.startsWith("PDA")){
                    rtn = 0.002;
                } else if (cname.startsWith("3Quad")) {
                    rtn = 0.0004;
                }
            }
        } else {
            rtn = 0;            // Null data for disabled channels
        }
        Messages.postDebug("lcAdc", "getMvToAu(" + chan + ")=" + rtn
                           + ", cname=" + cname);
        return rtn;
    }

    /**
     * @param m Where to send status messages.
     * @param iAdc Which ADC's data to send.
     */
    public void setUvStatusManager(StatusManager m, int iAdc) {
        m_statusManager = m;
        m_uvStatusChan = iAdc;
    }

    private void updateStatus(double[] datums) {
        if (m_statusManager != null
            && m_uvStatusChan >= 0 && m_uvStatusChan < datums.length)
        {
            // Send UV absorption to status manager
            String msg = "uvAttn " + Fmt.f(4, datums[m_uvStatusChan]) + " AU";
            m_statusManager.processStatusData(msg);
        }
    }

    public void sendCommand(String cmd) {
        println(cmd);
    }

    /* *********** BEGIN TEST STUFF **************************
    * ... to acquire fake data
    ******************************************************/
    javax.swing.Timer updateTimer = null;
    private int nFakes = 0;
    private BufferedReader fakeIn = null;
    private long fakeT0 = 0;
    private long fakeTimeHold;


    private double[] parseDoubles(String line) {
        if (line == null) {
            return new double[4];
        }
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

    private void initFakeData() {
        String path = FileUtil.usrdir() + "/lc/lcdata.lcd";
        final int bufSize = 32 * (1 << 10); // 32K
        System.out.println("readFakeData(" + path + ")");
        try {
            String line;
            fakeIn = new BufferedReader(new FileReader(path), bufSize);
            while ((line = fakeIn.readLine()) != null) {
                line = line.trim().toLowerCase();
                if (line.startsWith("; data:")) {
                    line = fakeIn.readLine(); // Swallow possibly junk line
                    break;
                }
            }
        } catch (FileNotFoundException fnfe) {
            Messages.postError("Cannot find LC data file: " + path);
        } catch (IOException ioe) {
            Messages.postError("IO Exception initializing fake data: " + path);
            Messages.writeStackTrace(ioe);
        }
        nFakes = 0;
    }

    private void updateFakeData() {
        if (fakeIn == null) {
            Messages.postDebug("updateFakeData(): no fake data file");
            return;
        }
        try {
            String line;
            if ((line = fakeIn.readLine()) == null) {
                fakeIn.close();
                fakeIn = null;
                controller.sendToVnmr("lccmd('stopRun')");
            } else {
                double[] d = parseDoubles(line);
                Messages.postDebug("debugFakeData",
                                   "line=" + line
                                   + ", len=" + d.length
                                   + ", run=" + run);
                if (d.length >= 4 && run != null && !run.flowPaused) {
                    long time = nFakes * m_adcRate_ms;
                    if (run != null) {
                        time = run.getFlowTime_ms();
                    }
                    Messages.postDebug("debugFakeData",
                                       "time=" + (time / 60000.0)
                                       + " minutes");

                    double[] data = new double[d.length - 1];
                    for (int i = 1; i < d.length; i++) {
                        data[i - 1] = d[i];
                    }
                    appendChannelValues(time, data);
                    nFakes++;
                }
            }
        } catch (IOException ioe) {
            Messages.postError("IO Exception reading fake data");
            Messages.writeStackTrace(ioe);
        }
    }
    /* *********** END TEST STUFF **************************/
    /******************************************************/

    /**
     * Send current data to the run; also append it to the SLIM data file.
     * Checks which ADC channels are being used, and only sends data
     * for the active channels.
     * @param time The flow time of the data in ms.
     * @param data The data values for all ADCs (active or not).
     */
    private void appendChannelValues(long time, double[] data) {
        long now = System.currentTimeMillis();
        int n = countActiveChannels();
        if (n > 0) {
            LcDatum[] datums = new LcDatum[n];
            int k = 0;
            for (int itrace = 0; itrace < m_activeChannels.length; itrace++) {
                int iadc = m_activeChannels[itrace];
                if (iadc >= 0) {
                    datums[k++] = new LcDatum(itrace, time, now, data[iadc]);
                }
            }
            run.appendChannelValues(datums);
            writeData(time, data);
        }
    }

    /**
     * Start an LC run; i.e., start saving data.
     */
    public boolean startRun(LcRun r, int period_ms) {
        String filepath = r.getFilepath();
        boolean ok = openDataWriter(filepath);
        if (ok) {
            startAdc(r, period_ms);
        }
        return ok;
    }

    /**
     * Tell the SLIM to collect and send ADC values.  A value
     * will be sent every "period_ms" milliseconds.  The format
     * is an ASCII byte string: "RA<10><t0>...<t3>< a0>< a1><b0><b1><c0><c1>"
     * Where <ti> are the time bytes, and < ai>, <bi>, <ci> the bytes
     * for the 3 channels.
     */
    public void startAdc(LcRun r, int period_ms) {

        /******************************************************
         * Debug stuff
         */
        if (DebugOutput.isSetFor("fakeChromatogram")) {
            if (fakeIn == null) {
                initFakeData();
            }
            //Messages.postDebug("Generating fake ADC data");
            ActionListener updateTimeout = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                    updateFakeData();
                }
            };
            run = r;
            updateTimer = new javax.swing.Timer(1000, updateTimeout);
            updateTimer.setRepeats(true);
            updateTimer.start();
        }
        /*
         * End Debug Stuff
         *****************************************************/

        //Messages.postInfo("startAdc");
        m_adcRate_ms = period_ms;
        m_adcRunning = true;
        m_isNewRunSegment = true;
        run = r;
        println("PA " + period_ms); // Set Rate
        println("RA+"); // Start ADC
        m_adcStartTime = System.currentTimeMillis();
        controller.sendToVnmr("lcAdcStartTime=" + m_adcStartTime);
        String date = Util.getStandardDateTimeString(m_adcStartTime);
        controller.sendToVnmr("lcAdcStartDate='" + date + "'");
    }

    public void startAdc(int period_ms) {
        m_adcRate_ms = period_ms;
        m_adcRunning = true;
        m_isNewRunSegment = true;
        println("PA " + period_ms); // Set Rate
        println("RA+"); // Start ADC
    }

    public void stopRun() {
        stopAdc();
        closeDataWriter();
    }

    /**
     * Tell the SLIM to stop sending ADC values.
     */
    public void stopAdc() {

        Messages.postDebug("slimIO", "stopAdc");

        /******************************************************
         * Debug stuff
         */
        if (DebugOutput.isSetFor("fakeChromatogram")) {
            if (updateTimer != null) {
                updateTimer.stop();
            }
            fakeTimeHold = System.currentTimeMillis();
            if (fakeIn != null && !run.runActive) {
                try {
                    fakeIn.close();
                } catch (IOException ioe) {
                    Messages.postError("IO Exception closing fake data");
                    Messages.writeStackTrace(ioe);
                }
                fakeIn = null;
            }
            return;
        }
        /*
         * End Debug Stuff
         *****************************************************/

        if (!checkPort()) {
            return;
        }

        m_adcRunning = false;
        m_lastDataTime = Long.MAX_VALUE;
        println("RA-");
    }

    public void stopAllThreads() {
        stopAdc();
        //  stopLcWatch();
        //  stopNmrWatch();
    }


    public void watchdogFailure() {
        String msg = "LC Read Failure (watchdog).  Check cabling, power.";
        stopAllThreads();
        Messages.postError(msg);
    }

    // This method is intended to deal with situations where sub-threads
    // throw exceptions.  The exception cannot be propagated back up to
    // the calling thread, but a message can be sent to here.
    public void threadExceptionHandler (boolean closePort, Exception e) {
        String msg = "Exception thrown in subthread: " + e.getMessage();

        stopAllThreads();

        closePort();

        Messages.postError(msg);
        Messages.writeStackTrace(e);
    }

    // need to close the ports when the object is destroyed
    protected void finalize() throws Throwable {
        closePort();
        super.finalize();
    }

    public void setMakeupFlow(double mlPerMin) {
        /*Messages.postDebug("setMakeupFlow(" + mlPerMin + ")");/*CMP*/
        println("MW_LOCK");
        if (mlPerMin <= 0) {
            println("MW_STOP");
        } else {
            /*
             * Max flow is 5 mL/min
             * Set flow in units of 1/1000 % of max (millipercents)
             * NB: Makeup pump needs number as 6 digit string
             *     (i.e., padded on left with zeros)
             */
            long mPct = Math.round(1e5 * mlPerMin / 5);
            mPct = Math.min(mPct, 100000);
            String strPct = Fmt.d(6, mPct, false, '0');
            /*Messages.postDebug("setMakeupFlow(): strPct=\"" + strPct + "\"");/*CMP*/
            println("MW_FLWSET " + strPct);
        }
        println("MW_UNLOCK");
        //Messages.postDebug("setMakeupFlow() ... DONE");/*CMP*/
    }

    public void setMakeupFlowQueryRate(int rate_ms) {
        if (rate_ms > 0) {
            msFlowTimer.setDelay(rate_ms);
            msFlowTimer.start();
        } else {
            readMakeupFlow();
            msFlowTimer.stop();
        }
    }

    public void readMakeupFlow() {
        println("MR_FLOWRATE");
    }

    public void setMakeupPressQueryRate(int rate_ms) {
        if (rate_ms > 0) {
            msPressureTimer.setDelay(rate_ms);
            msPressureTimer.start();
        } else {
            readMakeupPressure();
            msPressureTimer.stop();
        }
    }

    public void readMakeupPressure() {
        println("MR_PRESSURE");
    }

    public void getSlimVersion() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("getVersion()");
        println("VN");
    }


    /**
     * Tell the SLIM box to send a "heartbeat" signal about
     * once a second.
     * A "heartbeat" is the ASCII byte string "RH\000".
     */
    public void enableHeartbeat() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("enableHeartbeat()");
        println("HR+");
        expectHeartbeats = true;
        if (!heartbeatTimer.isRunning()) {
            heartbeatTimer.start();
        }
    }

    /**
     * Tell the SLIM box to stop sending "heartbeats".
     */
    public void disableHeartbeat() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("disableHeartbeat()");
        println("HR-");
        expectHeartbeats = false;
        if (heartbeatTimer.isRunning()) {
            heartbeatTimer.stop();
        }
    }

    /**
     * Tell the SLIM box to start listening for LC triggers.
     * After one trigger occurs, the SLIM is automatically
     * disabled.  Call this method again to listen for more
     * triggers.  The SLIM sends the ASCII byte string
     * "TL\000" when it gets a trigger.
     */
    public void enableLcTriggerInput() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("enableLcTriggerInput()");
        println("AL+");
    }

    /**
     * Tell the SLIM box to ignore LC trigger pulses.
     */
    public void disableLcTriggerInput() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("disableLcTriggerInput()");
        println("AL-");
    }

    /**
     * Tell the SLIM box to start listening for NMR triggers.
     * After one trigger occurs, the SLIM is automatically
     * disabled.  Call this method again to listen for more
     * triggers.  The SLIM sends the ASCII byte string
     * "TN\000" when it gets a trigger.
     */
    public void enableNmrTriggerInput() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("enableNmrTriggerInput()");
        println("AN+");
    }

    /**
     * Tell the SLIM box to ignore trigger pulses from NMR.
     */
    public void disableNmrTriggerInput() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("disableNmrTriggerInput()");
        println("AN-");
    }

    /**
     * Pulse the bit connected to the "fault" line going
     * to the LC pump.  The pulse should be at least 100 ms long.
     */
    public void pumpStopPulse() {
        Messages.postError("LcSlimIO.pumpStopPulse() called");
        pulseBit(FAULT);
    }

    /**
     * Pulse the bit connected to the "start" line going
     * to the LC pump.  The pulse should be at least 100 ms long.
     */
    public void pumpStartPulse() {
        Messages.postError("LcSlimIO.pumpStartPulse() called");
        pulseBit(PUMP_START);
    }

    /**
     * Sends a trigger pulse to the NMR console.  The pulse should be
     * 100 ms long.  The SLIM box turns off the pulse.
     */
    public void triggerToNmr() {
        Messages.postDebug("nmrComm", "triggerToNmr() --->");
        pulseBit(NMR_TRIG);
    }

    public void setValvesStopped(boolean stop) {
        if (stop) {
            leftCollWastePulse();
            rightWastePulse();
        }
    }

    /**
     * Switches the left-side valve on the analyte collector to the
     * "TO COLLECTOR/TO WASTE" position.
     */
    public void leftCollWastePulse() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("leftCollWastePulse()");
        println("C+");
        controller.sendToVnmr("lcColumnValve=0");
    } 

    /**
     * Switches the left-side valve on the analyte collector to the
     * "TO COLUMN" position.
     */
    public void leftColumnPulse() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("leftCollPulse()");
        println("C-");
        controller.sendToVnmr("lcColumnValve=1");
    }

    synchronized private boolean loopStepping() {
        if (m_loopStepping != 0
            && System.currentTimeMillis() - m_loopStepping < 10000)
        {
            return true;
        } else {
            m_loopStepping = 0;
            return false;
        }
    }

    /**
     * Increments the analyte collector loop valve.  After it has
     * reached the new position, the SLIM will send the new position
     * with an ASCII byte string "LN\001\<loop#>.
     */
    synchronized public void loopIncr() {
        if (!checkPort()) {
            return;
        }
        m_desiredLoop = 0;      // Stop any gotoLoop we may be doing
        m_loopCommFail = 0;     // Prepare to detect failure
        println("IL");
        m_loopStepping = System.currentTimeMillis();
        loopTimer.setInitialDelay(LOOP_STEP_DELAY);
        //loopTimer.start();/*CMP*/
        Messages.postDebug("slimIO","Loop Incremented");
    }

    /**
     * Go to the specified loop number, and execute a command when it
     * gets there.
     */
    synchronized public void gotoLoop(int nLoop, String cmd) {
        if (!checkPort()) {
            return;
        }
        m_loopCommand = cmd;
        int maxLoop = controller.config.getNumberOfLoops();
        if (nLoop <= 0 || nLoop > maxLoop) {
            Messages.postError("Cannot go to loop " + nLoop + ";"
                               + " loop numbers are 1 - " + maxLoop + ".");
            return;
        }
        m_desiredLoop = nLoop;
        println("FL+ " + m_desiredLoop);
        Messages.postDebug("slimIO",
                           "Sent goto loop " + m_desiredLoop + " command");
    }

    /**
     * Tell the SLIM to read and send back the analyte collector
     * loop position.  It will be sent as an ASCII byte string:
     * "LN\001\<loop#>.
     */
    public void getLoopPosition() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("LcSlimIO.getLoopPosition()");
        Messages.postDebug("slimIO", "Sending loop position request");
        println("LN");
        loopTimer.setInitialDelay(LOOP_READBACK_DELAY);
        //loopTimer.start();/*CMP*/
    }

    /**
     * Switches the right-side valve on the analyte collector to the
     * "TO NMR" position.
     */
    public void rightNmrPulse() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("rightNmrPulse()");
        println("N+");
        controller.sendToVnmr("lcNmrValve=1");
    }

    /**
     * Switches the right-side valve on the analyte collector to the
     * "TO WASTE" position.
     */
    public void rightWastePulse() {
        if (!checkPort()) {
            return;
        }
        //Messages.postInfo("rightWastePulse()");
        println("N-");
        controller.sendToVnmr("lcNmrValve=0");
    }

    /**
     * Switches the stop-flow valve to flow.
     * This is used only on systems without an analyte collector.
     */
    public void SFFlowPulse() {
        //Messages.postInfo("SFFlowPulse()");
        pulseBit(SF_FLOW);
    }

    /**
     * Switches the stop-flow valve to stop.
     * This is used only on systems without an analyte collector.
     */
    public void SFStopPulse() {
        //Messages.postInfo("SFStopPulse()");
        pulseBit(SF_STOP);
    }

    /**
     * Pulses some collection of bits in the outputByte on the SLIM box.
     * @param value Bit mask indicating which bit(s) to pulse.
     */
    public void pulseBit(int value) {
        if (value == 0) {
            return;
        }
        if (!checkPort()) {
            return;
        }
        outputByte |= value;
        //Messages.postInfo("pulseBit():"
        //                  + new String(cmd, 0, cmd.length - 1)
        //                  + (cmd[cmd.length - 1] & 0xff));
        println("SO " + outputByte);
        ActionListener unPulse = new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                unPulseBit();
            }
        };
        long time = System.currentTimeMillis() + PULSE_WIDTH;
        pushPulseValue(time, value);
        javax.swing.Timer timer = new javax.swing.Timer(PULSE_WIDTH, unPulse);
        timer.setRepeats(false);
        timer.start();
    }

    /**
     * Turns off the SLIM pulse started by "pulseBit()"
     */
    private void unPulseBit() {
        if (!checkPort()) {
            return;
        }
        int value;
        while ((value = popPulseValue()) != 0) {
            outputByte &= ~value;
            //Messages.postInfo("unPulseBit():"
            //                  + new String(cmd, 0, cmd.length - 1)
            //                  + (cmd[cmd.length - 1] & 0xff));
            println("SO " + outputByte);
        }
    }

    private void pushPulseValue(long time, int bitval) {
        pulseList.add(new PulseValue(time, bitval));
    }

    private int popPulseValue() {
        if (pulseList.isEmpty()) {
            return 0;
        }
        long now = System.currentTimeMillis();
        PulseValue pv = pulseList.first();
        if (pv.time <= now) {
            pulseList.remove(pv);
            return pv.value;
        } else {
            return 0;
        }
    }

    class PulseValue implements Comparable{
        public long time;
        public int value;

        public PulseValue(long time, int value) {
            this.time = time;
            this.value = value;
        }

        public int compareTo(Object ob2) {
            PulseValue pv2 = (PulseValue)ob2;
            if (time > pv2.time) {
                return 1;
            } else if (time < pv2.time) {
                return -1;
            } else if (value > pv2.value) {
                return 1;
            } else if (value < pv2.value) {
                return -1;
            } else {
                return 0;
            }
        }

        public boolean equals(Object ob2) {
            PulseValue pv2 = (PulseValue)ob2;
            return (time == pv2.time && value == pv2.value);
        }
    }


    /**
     * This class listens for messages from the SLIM
     */
    class SlimReader extends Thread {
        private BufferedReader m_inputStream = null;
        private boolean m_quit = false;

        SlimReader(BufferedReader inputStream) {
            setName("LcSlimReader");
            m_inputStream = inputStream;
        }

        public void quit() {
            m_quit = true;
        }

        public synchronized void run() {
            String msg = "";
            while (!m_quit && msg != null) {
                try {
                    msg = m_inputStream.readLine();
                    SwingUtilities.invokeLater(new MessageReader(msg));
                } catch (SocketTimeoutException stoe) {
                    Messages.postDebug("slimIO+",
                                       "LcSlimIO: Socket read timed out");
                } catch (IOException ioe) {
                    Messages.postDebug("LcSlimIO: Socket read failed");
                    m_quit = true;
                }
            }
        }
    }

    /**
     * Interprets a message. Done as a task in the Event Thread.
     */
    class MessageReader implements Runnable {
        String mm_msg;

        MessageReader(String msg) {
            mm_msg = msg;
        }

        public void run() {
            readMessage(mm_msg);
        }
    }
}
