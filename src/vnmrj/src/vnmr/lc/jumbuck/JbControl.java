/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;


import java.io.*;
import java.util.*;

import vnmr.lc.LcDataListener;
import vnmr.lc.LcMethod;
import vnmr.lc.LcMsg;
import vnmr.lc.LcRun;
import vnmr.lc.UvDetector;
// import vnmr.lc.UvParams;
import vnmr.ui.StatusManager;
import vnmr.util.Fmt;


/**
 * Request the type, model and name of this instrument.
 * Note: This class can act as a StatusManager in stand-alone test mode.
 */
public class JbControl extends UvDetector
    implements JbDefs, JbReplyProcessor, StatusManager {

    /** The parameter cache for my values (driver values). */
    private JbParameters m_driverParameters;

    /** The parameter cache for detector values. */
    private JbParameters m_detectorParameters;

    /** This guy knows how to send commands */
    private JbCommands m_cmd;

    /** Table of who is listening for messages, by message code. */
    private Set<LcDataListener> m_dataListeners
        = new TreeSet<LcDataListener>();

    /** Thread to poll the instrument for data. */
    private DataPoller m_dataPoller = null;

    /** Counter to track whether we're expecting more data. */
    private int m_nDataRequests = 0;

    /** Thread to poll the instrument for status. */
    private StatusPoller m_statusPoller = null;

    /** The LC run that gets the chromatogram data. */
    private LcRun m_chromatogramManager;

    /** Which wavelengths to monitor. */
    private float[] m_chromatogramLambdas;

    /** Which data array indices correspond to the monitor wavelengths. */
    private int[] m_chromatogramIndices;

    /** Which traces plot the monitor data. */
    private int[] m_chromatogramTraces;

    /** The last-downloaded method parameters. */
    private MethodParams m_methodParams = null;

    /** Workaround for bad lamp control. */
    private boolean m_turnOffD2 = false;

    /** Track status updates. */
    private boolean m_firstStatus = true;

    /** Track state changes. */
    private int m_pdaState = -1;

    /** Track lamp state changes. */
    private int[] m_lampStat = {-1, -1};

    /** Track monitor wavelength changes. */
    private float[] m_lambda = {-1f, -1f};

    /** Track absorption changes. */
    private float[] m_abs = {-1f, -1f};

    /** Table of error message text. */
    private Map<Integer, String> m_errorTable = new HashMap<Integer, String>();

    /** Keep track of how much time we've spent stopped. */
    private double m_stopDuration_min = 0;

    /** Remember the last time we stopped. */
    private long m_dateStopped_ms = 0;

    /* Remember status values. */
    private boolean m_lampOnOk = false;
    private boolean m_lampOffOk = false;
    private boolean m_autoZeroOk = false;
    private boolean m_resetOk = false;
    private boolean m_downloadOk = false;
    private boolean m_startOk = false;
    private boolean m_stopOk = false;

    /** For diagnostic interface. */
    private boolean m_statusEchoingEnabled = false;

    /** Table of data rates (in ms) keyed by slit number */
    private static Map<String, String> sm_dataRateTable = null;

    /** Table of slit widths (in nm) keyed by bunching factor */
    private static Map<String, String> sm_slitTable = null;


    /**
     * Diagnostic user interface to the Jumbuck driver.
     */
    public static void main(String[] args) {

        LcMsg.addCategory("jbio");

        // Single allowed arg is IP address of PDA
        Integer ip = null;
        if (args.length > 0) {
            ip = intIpAddress(args[0]);
        }                

        JbControl master = new JbControl(ip, null);

        // Read and execute commands from stdin
        BufferedReader in
                = new BufferedReader(new InputStreamReader(System.in));
        String line;

        try {
            
            for (prompt("> ");
                 (line = in.readLine()) != null;
                 prompt("> "))
            {
                //System.out.println("got: \"" + line + "\"");
                StringTokenizer toker = new StringTokenizer(line, " =");
                String cmd = "";
                if (toker.hasMoreTokens()) {
                    cmd = toker.nextToken();
                }
                String par = "";
                if (toker.hasMoreTokens()) {
                    par = toker.nextToken().replace('-', ' ');
                }
                String value = null;
                if (toker.hasMoreTokens()) {
                    value = toker.nextToken("").trim(); // The rest of the line
                }

                // Execute the command
                if (cmd.equals("set")) {
                    master.setParameter(par, value);

                } else if (cmd.equals("get")) {
                    Object oValue = master.getParameter(par);
                    if (oValue instanceof float[]) {
                        float[] v = (float[])oValue;
                        System.out.print(par + " = float[" + v.length + "]");
                        if (v.length > 2) {
                            System.out.println(" = "
                                               + v[0] + ", "
                                               + v[1] + ", "
                                               + v[2] + ", ...");
                        } else {
                            System.out.println();
                        }
                    } else {
                        System.out.println(par + " = " + oValue);
                    }

                } else if (cmd.equals("connect")) {
                    master.connect();

                } else if (cmd.equals("status")) {
                    //master.stopStatus();
                    //if (par.equalsIgnoreCase("on")) {
                        //master.startStatus();
                    //}
                    master.setStatusEchoing(par.equalsIgnoreCase("on"));

                } else if (cmd.equals("cmd")) {
                    try {
                        // Is command a hex number?
                        String sCode = par;
                        // Strip any leading "0x"
                        int n = 1 + par.indexOf('x');
                        if (n > 0) {
                            sCode = par.substring(n);
                        }
                        int code = Integer.parseInt(sCode, 16);
                        master.sendCommand(code);
                    } catch (NumberFormatException nfe) {
                        // TODO: Interpret command by name
                        System.err.println("Invalid hex number: \""
                                           + par + "\"");
                    }

                } else if (cmd.equals("setip")) {
                    String ipAddr = "172.16.0.60";
                    String ipGateway; // Default defined below, based on ipAddr
                    String ipSubnet = "255.255.255.0";
                    int bootpId = 0;
                    // Reparse the line
                    StringTokenizer toke2 = new StringTokenizer(line, " =");
                    toke2.nextToken(); // Discard cmd
                    if (toke2.hasMoreTokens()) {
                        ipAddr = toke2.nextToken();
                    }
                    int lastPt = ipAddr.lastIndexOf('.');
                    ipGateway = ipAddr.substring(0, lastPt) + ".1";
                    if (toke2.hasMoreTokens()) {
                        ipGateway = toke2.nextToken();
                    }
                    if (toke2.hasMoreTokens()) {
                        ipSubnet = toke2.nextToken();
                    }
                    if (toke2.hasMoreTokens()) {
                        try {
                            bootpId = Integer.parseInt(toke2.nextToken());
                        } catch (NumberFormatException nfe) {
                        }
                    }
                    bootpId = (bootpId == 0) ? 0 : 1;
                    master.setIpAddress(ipAddr, ipGateway, ipSubnet, bootpId);

                } else if (cmd.equals("quit")) {
                    System.exit(0);

                } else if (cmd.length() > 0) {
                    System.out.println("Command not recognized: \""
                                       + cmd + "\"");
                }
                if (cmd.length() > 0) {
                    LcMsg.addCategory("jbio");
                } else {
                    LcMsg.removeCategory("jbio");
                }
            }
        } catch (IOException ioe) {
            System.err.println("Error: " + ioe);
        }
        System.out.println();
    }

    private static void prompt(String s) {
        System.out.print(s);
    }

    /**
     * Construct a "dead" version, just so that we can call methods
     * that could be static, if you could specify static methods
     * in an abstract class.
     */
    public JbControl() {
        super(null);
    }

    /**
     * Create JbControl with the default IP address for the 335 PDA.
     */
    public JbControl(StatusManager statusManager) {
        this(null, statusManager);
    }

    /**
     * Create JbControl with a specified IP address for the 335 PDA.
     */
    public JbControl(Integer ip, StatusManager statusManager) {
        super(statusManager);
        if (statusManager == null) {
            // Needed for creating from main() method:
            m_statusManager = this;
        }
        m_driverParameters = new JbParameters();
        m_detectorParameters = new JbParameters();
        m_cmd = new JbCommands(m_driverParameters, m_detectorParameters);
        m_cmd.setReplyProcessor(this);
        if (ip != null) {
            setParam("IP Address", ip);
        } else {
            // TODO: Look for a 335 with SLP?
        }
        // TODO: Scan a config file with initial parameter values?

        connect();
        startStatus();
    }

    /**
     * Set a fixed IP address in the detector.
     */
    public void setIpAddress(String addr, String gate, String mask, int bootp) {
        // Set values
        m_driverParameters.set("SetFixedIP", intIpAddress(addr));
        m_driverParameters.set("GatewayAddress", intIpAddress(gate));
        m_driverParameters.set("SetSubnetMask", intIpAddress(mask));
        m_driverParameters.set("BootPsname", bootp);
        sendCommand(ID_CMD_SET_IP_PARAMS);

        // Read back to check
        sendCommand(ID_CMD_GET_IP_PARAMS);
    }

    /**
     * Tell the detector what flow cell is installed.
     */
    public void setFlowCell(int n, double r) {
        setParam("CellType", n);
        setParam("CellRatio", (float)r);
        sendCommand(ID_CMD_SET_CELL_PARAMS);
    }

    /**
     * Select the lamp(s) to be controlled by ID_CMD_LAMP commands.
     */
    public void selectLamp(int n) {
        setParam("Lamp", n);
    }

    private static void initSlits() {
        sm_slitTable = new HashMap<String, String>();
        sm_slitTable.put(SLIT_WIDTH_1.toString(), "1 nm");
        sm_slitTable.put(SLIT_WIDTH_2.toString(), "2 nm");
        sm_slitTable.put(SLIT_WIDTH_4.toString(), "4 nm");
        sm_slitTable.put(SLIT_WIDTH_8.toString(), "8 nm");
        sm_slitTable.put(SLIT_WIDTH_16.toString(), "16 nm");
    }

    public static String getSlitLabel(String key) {
        if (sm_slitTable == null) {
            initSlits();
        }
        String value = sm_slitTable.get(key);
        return value == null ? key : value;
    }

    private static void initDataRates() {
        sm_dataRateTable = new HashMap<String, String>();
        sm_dataRateTable.put("400", "0.4 s");
        sm_dataRateTable.put("800", "0.8 s");
        sm_dataRateTable.put("1600", "1.6 s");
        sm_dataRateTable.put("3200", "3.2 s");
    }

    public static String getDataRateLabel(String key) {
        if (sm_dataRateTable == null) {
            initDataRates();
        }
        String value = sm_dataRateTable.get(key);
        return value == null ? key : value;
    }


    /* *************** Start of UvDetector interface *************** */

    /**
     * Return the name of this detector, "335".
     */
    public String getName() {
        return "335";
    }

    /**
     * Read the status from the detector periodically (NOT IMPLEMENTED).
     * This will include
     * the absorption at selected wavelengths, if possible.  These
     * wavelengths may be changed by downloadMethod().
     * @param period The time between samples in ms. If 0, read status once.
     */
    public boolean readStatus(int period) {
        return true;            // TODO: Implement readStatus()?
    }

    /** See if the detector is ready to start a run. */
    public boolean isReady() {
        return true;            // TODO: Implement isReady()
    }

    /**
     * Connect to the instrument.  If connection is successful,
     * all the error code strings are read.
     * @return Returns false if the connection cannot be made.
     */
    public boolean connect() {
        int ip = m_driverParameters.getInt("IP Address");
        boolean ok = m_cmd.connect(stringIpAddress(ip));
        if (ok) {
            setParam("ReadFlag", READ_FIRST);
            sendCommand(ID_CMD_GET_ERROR_CODE_STRINGS);
        }
        return ok;
    }

    /** Shut down communication with the detector. */
    public void disconnect() {
        if (m_dataPoller != null) {
            m_dataPoller.quit();
        }
        if (m_statusPoller != null) {
            m_statusPoller.quit();
        }
        m_cmd.disconnect();
    }

    /** Determine if detector communication is OK. */
    public boolean isConnected() {
        return m_cmd.isConnected();
    }

    /** Turn on the lamp(s). */
    public boolean lampOn() {
        setParam("Lamp Action", LAMP_ON);
        boolean ok = sendCommand(ID_CMD_LAMP);

        // Workaround to turn on only vis lamp
        m_turnOffD2 = false;
        int lamp = m_driverParameters.getInt("Lamp");
        if (ok && lamp == LAMP_VIS.intValue()) {
            m_turnOffD2 = true;
        }
        return ok;
    }

    /** Turn off the lamp(s). */
    public boolean lampOff() {
        setParam("Lamp", LAMP_BOTH);
        setParam("Lamp Action", LAMP_OFF);
        return sendCommand(ID_CMD_LAMP);
    }

    /** Make the current absorption the zero reference. */
    public boolean autoZero() {
        return sendCommand(ID_CMD_AUTO_ZERO);
    }

    /** Download a (constant) run method with maximum run time. */
    public boolean downloadMethod(LcMethod p) {
        MethodParams mp = null;
        if (p != null) {
            mp = new MethodParams(p);
            setParam("Param Wavelength 1", (float)mp.lambda1);
            setParam("Param Wavelength 2", (float)mp.lambda2);
            setParam("Min Wavelength", mp.lambdaMin);
            setParam("Max Wavelength", mp.lambdaMax);
            setParam("Slit Width", mp.slitNumber);
            setParam("Bunching Size", mp.bunchingFactor);
        }
        m_methodParams = null;
        boolean ok = sendCommand(ID_CMD_METHOD_HEADER_335);
        if (ok) {
            setParam("Run Time", ZERO);
            ok = sendCommand(ID_CMD_METHOD_LINE_ENTRY_335);
        }
        if (ok) {
            setParam("Run Time", MAX_RUN_TIME);
            ok = sendCommand(ID_CMD_METHOD_END_335);
        }
        if (ok) {
            m_methodParams = mp;
            //if (LcMsg.isSetFor("uvDownloadCheck")) {
                LcMsg.postDebug("JbControl.downloadMethod:\n" + mp);
            //}/*CMP*/
        }
        return ok;
    }

    /**
     * Returns true if the given method would download the same thing to
     * the 335 as what was last sent.
     */
    public boolean isDownloaded(LcMethod p) {
        MethodParams mp = new MethodParams(p);
        return mp.equals(m_methodParams);
    }

    /**
     * Start (or restart) a run.  If the 335 is in the STATE_STOPPED state,
     * restarts the current run, otherwise starts a new run.
     */
    public boolean start() {
        int state = getIntDetectorParameter("DetectorState");
        if (state == STATE_STOPPED) {
            return restart();
        }

        return start(false);
    }

    /** Restart a paused run. */
    public boolean restart() {
        return start(true);
    }

    /**
     * Start, or restart, a run, depending on the value of the restartFlag.
     * @param restartFlag If true, restarts the current run.
     */
    private boolean start(boolean restartFlag) {
        boolean ok;
        if (restartFlag) {
            setParam("Method Action", METHOD_ACTION_RESUME);
            ok = sendCommand(ID_CMD_METHOD_ACTION);
            long now = System.currentTimeMillis();
            if (m_dateStopped_ms > 0) {
                m_stopDuration_min += (now - m_dateStopped_ms) / 60000.0;
                LcMsg.postDebug("jbtime", "m_stopDuration_min="
                                + Fmt.f(4, m_stopDuration_min));
            }
        } else {
            startDataListeners();
            setParam("Method Action", METHOD_ACTION_START);
            ok = sendCommand(ID_CMD_METHOD_ACTION);
            m_stopDuration_min = 0;
        }
        m_dateStopped_ms = 0;
        m_nDataRequests = 0;
        setDataRate(1000);      // TODO: set data polling rate
        return ok;
    }

    /** Stop a run and save data, continue monitoring selected wavelength(s). */
    public boolean stop() {
        return pause();
    }

    /** Stop a run in such a way that it can be resumed. */
    public boolean pause() {
        // TODO: Close data file
        setParam("Method Action", METHOD_ACTION_STOP);
        boolean ok = sendCommand(ID_CMD_METHOD_ACTION);
        m_dateStopped_ms = System.currentTimeMillis();
        setDataRate(0);
        return ok;
    }

    /** Reset the detector state to be ready to start a new run. */
    public boolean reset() {
        setParam("Method Action", METHOD_ACTION_RESET);
        boolean ok = sendCommand(ID_CMD_METHOD_ACTION);
        return ok;
    }

    /**
     * Selects who deals with the data.
     * @return Returns true, indicating data will get sent.
     */
    public boolean setDataListener(LcDataListener dl) {
        m_dataListeners.clear();
        m_dataListeners.add(dl);
        return true;
    }

    /**
     * Selects channels and wavelengths to monitor for the chromatogram.
     * @param lambdas The wavelengths to monitor.
     * @param traces The corresponding traces to send them to.
     * @param chromatogramManager Where to send the data.
     */
    public boolean setChromatogramChannels(float[] lambdas, int[] traces,
                                           LcRun chromatogramManager) {
        return false;
        /*
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
        setChromatogramIndices();
        m_chromatogramManager = chromatogramManager;
        return true;
        */
    }

    /**
     * Helper method calculates which members of the data array
     * correspond to the desired chromatogram wavelengths.
     */
    private void setChromatogramIndices() {
        /*
         * NB: The wavelength increment between array elements is
         * always 1 nm.
         */
        if (m_methodParams == null) {
            return;             // No way to do it.
        }
        int n = m_chromatogramLambdas.length;
        if (m_chromatogramIndices == null || m_chromatogramIndices.length != n) {
            m_chromatogramIndices = new int[n];
        }
        for (int i = 0; i < n; i++) {
            if (m_chromatogramLambdas[i] < m_methodParams.lambdaMin) {
                m_chromatogramLambdas[i] = m_methodParams.lambdaMin;
            } else if (m_chromatogramLambdas[i] > m_methodParams.lambdaMax) {
                m_chromatogramLambdas[i] = m_methodParams.lambdaMax;
            }
            m_chromatogramIndices[i]
                    = (int)m_chromatogramLambdas[i] - m_methodParams.lambdaMin;
        }
    }

    /**
     * @return The expected interval between data points in ms.
     */
    public int getDataInterval() {
        return m_methodParams == null ? 0 : m_methodParams.dataInterval_ms;
    }

    /**
     * Get the string that uniquely identifies this detector type to the
     * software.
     * @return Returns "335".
     */
    public String getCanonicalIdString() {
        return "335";
    }

    /**
     * Get the label shown to the user to identify the detector type.
     * @return Returns "335 PDA".
     */
    public String getIdString() {
        return "335 PDA";
    }

    /**
     * Get the wavelength range this type of detector is good for.
     * @return Two ints, the min and max wavelengths, respectively, in nm.
     */
    public int[] getMaxWavelengthRange() {
        int[] rtn = {200, 900};
        return rtn;
    }

    /**
     * @return Returns true, since this detector can return spectra.
     */
    public boolean isPda() {
        return true;
    }

    /**
     * The number of "monitor" wavelengths this detector can support.
     * These may be monitored either from getting status or from
     * reading an analog-out value, but must be obtainable outside
     * of a run.
     * @return Returns 2.
     */
    public int getNumberOfMonitorWavelengths() {
        return 2;
    }

    /**
     * The number of "analog outputs" this detector supports, and
     * that we want the software to support.
     * @return Returns 0; the analog outputs are not supported.
     */
    public int getNumberOfAnalogChannels() {
        return 0;
    }

    /* *************** End of UvDetector interface *************** */


    public void startStatus() {
        m_statusPoller = new StatusPoller();
        m_statusPoller.start();
    }

    public void stopStatus() {
        if (m_statusPoller != null) {
            m_statusPoller.quit();
            m_statusPoller = null;
        }
    }

    private void startDataListeners() {
        if (m_dataListeners.size() > 0) {
            // Figure out what the wavelengths are (spacing is always 1 nm)
            int lambdaMin = m_driverParameters.getInt("Min Wavelength");
            int lambdaMax = m_driverParameters.getInt("Max Wavelength");
            int n = lambdaMax - lambdaMin + 1;
            float[] wavelengths = new float[n];
            for (int i = 0; i < n; i++) {
                wavelengths[i] = lambdaMin + i;
            }
            Iterator itr = m_dataListeners.iterator();
            while (itr.hasNext()) {
                LcDataListener dl = (LcDataListener)itr.next();
                dl.setUvWavelengths(wavelengths);
            }
        }
    }

    private void setParam(String name, Object value) {
        m_driverParameters.set(name, value);
    }

    /**
     * Used only for standalone debugging mode.
     */
    public void processStatusData(String s) {
        if (isStatusEchoing()) {
            System.out.println("STATUS: " + s);
        }
    }

    public void setStatusEchoing(boolean b) {
        m_statusEchoingEnabled = b;
    }

    public boolean isStatusEchoing() {
        return m_statusEchoingEnabled;
    }

    /**
     * Get the correct parameter table for a particular parameter.
     * If the parameter name starts with "detector " (case insensitive),
     * we use the detector table, otherwise the driver table.
     * The leading "detector " is stripped off the name.
     */
    public JbParameters getParameterTable(String name) {
        String key = name.toLowerCase();
        JbParameters table;
        if (key.startsWith("detector ")) {
            table = m_detectorParameters;
            name = name.substring(9);
        } else {
            table = m_driverParameters;
        }
        return table;
    }

    /**
     * Get the canonical name for a particular parameter.
     * If the parameter name starts with "detector " (case insensitive),
     * it is stripped off the name.
     */
    public String getCanonicalName(String name) {
        String key = name.toLowerCase();
        if (key.startsWith("detector ")) {
            name = name.substring(9);
        }
        return name;
    }

    /**
     * Set a parameter value.
     */
    public boolean setParameter(String name, String value) {
        JbParameters table = getParameterTable(name);
        name = getCanonicalName(name);
        return table.set(name, value);
    }

    /**
     * Get a parameter value.
     */
    public Object getParameter(String name) {
        JbParameters table = getParameterTable(name); // Look at the right table
        name = getCanonicalName(name);
        return table.getParameterValue(name);
    }

    /**
     * Get an int parameter value from the detector table.
     */
    public int getIntDetectorParameter(String name) {
        return m_detectorParameters.getInt(name);
    }

    /**
     * Get an int parameter value from the detector table.
     */
    public float getFloatDetectorParameter(String name) {
        return m_detectorParameters.getFloat(name);
    }

    /**
     * Get an float[] parameter value from the detector table.
     */
    public float[] getFloatArrayDetectorParameter(String name) {
        return m_detectorParameters.getFloatArray(name);
    }

    /**
     * Send a command to the instrument.
     * @return Returns true unless the connection is bad or the command
     * code is invalid.
     */
    public boolean sendCommand(int code) {
        return m_cmd.sendCommand(code);
    }

    /**
     * Send a command to the instrument.
     * @return Returns true unless the connection is bad or the command
     * code is invalid.
     */
    public boolean sendCommand(Integer code) {
        return m_cmd.sendCommand(code.intValue());
    }

    /**
     * Notes that a data packet has come in.
     */
    public void dataReceived() {
        m_nDataRequests--;
    }        

    /**
     * Does message specific processing of replies from the detector.
     * The parameters in the reply have already been set.
     */
    public void processReply(int code, int level) {
        int rtnCode = getIntDetectorParameter("ReturnCode");
        if (rtnCode != 0) {
            int severity = rtnCode >>> 14;
            rtnCode &= 0x3fff;
            // Error codes to avoid showing to user:
            boolean isOk = (rtnCode == 9008);
            String errStr = (String)m_errorTable.get(rtnCode);
            if (errStr == null) {
                if (isOk) {
                    LcMsg.postDebug("Unknown PDA error received: " + rtnCode);
                } else {
                    LcMsg.postError("Unknown PDA error received: " + rtnCode);
                }
            } else if (severity > 1 && !isOk) {
                LcMsg.postError("PDA Error #" + rtnCode + ": " + errStr);
            } else {
                LcMsg.postDebug("PDA Warning #" + rtnCode + ": " + errStr);
            }
            m_detectorParameters.set("ReturnCode", ZERO);
        }
        if ((code == ID_RSP_RETURN_SPECTRUM.intValue()
             || code == ID_RSP_RETURN_MULTIPLE_SPECTRA.intValue())
            && level > 0 && rtnCode == 0)
        {
            // Report data
            int rawTime = getIntDetectorParameter("RunTime"); // deci-secs
            /**
             * NB: The 335 timestamp on the data continues running when the
             * run is stopped.  Hence this correction.
             * (The RunTime returned in the status does stop.)
             */
            double time = (rawTime / 600.0) - m_stopDuration_min; // minutes
            LcMsg.postDebug("jbtime", "dataRunTime=" + Fmt.f(4, time) + " min");
            float[] spectrum = getFloatArrayDetectorParameter("AbsValue");

            // NB: Values above 2 AU are bogus. Clip the data at 2 AU.
            int specLength = spectrum.length;
            for (int i = 0; i < specLength; i++) {
                if (spectrum[i] > 2.0f) {
                    spectrum[i] = 2.0f;
                }
            }

            Iterator itr = m_dataListeners.iterator();
            while (rtnCode == 0 && itr.hasNext()) {
                LcDataListener dl = (LcDataListener)itr.next();
                dl.processUvData(time, spectrum);
            }
            if (m_dataPoller == null && m_nDataRequests == 0) {
                itr = m_dataListeners.iterator();
                while (itr.hasNext()) {
                    LcDataListener dl = (LcDataListener)itr.next();
                    dl.endUvData();
                }
            }

        } else if (code == ID_RSP_RETURN_STATUS.intValue()) {
            // Report status
            processStatus();

        } else if (code == ID_RSP_GET_ERROR_CODE_STRINGS.intValue()) {
            if (level == 0) {
                int moreFlag = getIntDetectorParameter("MoreFlag");
                if (moreFlag == MORE_DATA.intValue()) {
                    setParam("ReadFlag", READ_NEXT);
                    sendCommand(ID_CMD_GET_ERROR_CODE_STRINGS);
                } else {
                    LcMsg.postDebug("jbio", "Received all error strings");
                }
            } else {
                int errCode = 0x3fff & getIntDetectorParameter("ErrorCode");
                String errString
                        = m_detectorParameters.getString ("StringDescription");
                if (errString != null && errString.length() > 0) {
                    m_errorTable.put(errCode, errString);
                }
                LcMsg.postDebug("jberrors", "err #" + errCode
                                + ": " + errString);
            }
        } else if (code == ID_RSP_GET_IP_PARAMS.intValue()) {
            JbParameters p = m_detectorParameters;
            LcMsg.postDebug("\nThese IP parameters will take effect when the"
                            + " 335 is power-cycled:");
            String addr = stringIpAddress(p.getInt("SetFixedIP"));
            LcMsg.postDebug(" address: " + addr);
            String gate = stringIpAddress(p.getInt("GatewayAddress"));
            LcMsg.postDebug(" gateway: " + gate);
            String mask = stringIpAddress(p.getInt("SetSubnetMask"));
            LcMsg.postDebug(" subnet: " + mask);
            String idFlag = p.getInt("BootPsname") == 0 ? "no" : "yes";
            LcMsg.postDebug(" bootp ID? " + idFlag);
        }
    }

    /**
     * Extract the values at the chromatogram wavelengths from the data
     * and send them to the chromatogram.  Does nothing if the run is
     * inactive (paused or stopped).
     */
    /*
    private void sendMonitorData(double time, float[] data) {
        if (m_chromatogramManager != null && m_chromatogramManager.isActive()) {
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
            LcMsg.postDebug("jbMonitorTime", "Monitor time = " + time_ms);
            m_chromatogramManager.appendChannelValues(datums);
        }
    }
    */

    private void processStatus() {
        if (m_statusManager != null && isConnected()) {
            if (m_firstStatus) {
                m_statusManager.processStatusData("uvTitle PDA");
                m_statusManager.processStatusData("uvId 335 PDA");
            }

            String msg;
            int state = getIntDetectorParameter("DetectorState");
            if (state != m_pdaState) {
                m_pdaState = state;
                msg = "uvStatus ";
                switch (state) {
                case STATE_POWER_ON: msg += "Power-Up"; break;
                case STATE_LAMP_OFF: msg += "Lamp_Off"; break;
                case STATE_LAMP_ON: msg += "Disabled"; break;
                case STATE_INITIALIZING: msg += "Initialize"; break;
                case STATE_MONITOR: msg += "Monitor"; break;
                case STATE_READY: msg += "Ready"; break;
                case STATE_RUNNING: msg += "Run"; break;
                case STATE_STOPPED: msg += "Pause"; break;
                case STATE_DIAGNOSTIC: msg += "Diagnostic"; break;
                case STATE_CALIBRATE: msg += "Calibrate"; break;
                case STATE_HG_CALIBRATE: msg += "Hg_Cal"; break;
                default: msg += "# " + state;
                }
                m_statusManager.processStatusData(msg);
                if (state != STATE_RUNNING && state != STATE_STOPPED) {
                    // Clear any run-time status
                    m_statusManager.processStatusData("uvTime -");
                }
            }

            if (state == STATE_RUNNING) {
                double runTime = getIntDetectorParameter("RunTime") / 100.0;
                m_statusManager.processStatusData("uvTime " + Fmt.f(2, runTime)
                                                  + " min");
                LcMsg.postDebug("jbtime", "statusRunTime="
                                + Fmt.f(2, runTime) + " min");
            }

            int d2LampStat = getIntDetectorParameter("D2LampState");
            int visLampStat = getIntDetectorParameter("VisLampState");
            if (m_lampStat[0] != d2LampStat || m_lampStat[1] != visLampStat) {
                m_lampStat[0] = d2LampStat;
                m_lampStat[1] = visLampStat;
                msg = "uvLampStatus ";
                for (int i = 0; i < 2; i++) {
                    switch (m_lampStat[i]) {
                    case LAMP_STATE_OFF: msg += "Off"; break;
                    case LAMP_STATE_ON: msg += "On"; break;
                    case LAMP_STATE_WARMING: msg += "Wait"; break;
                    case LAMP_STATE_NOT_PRESENT: msg += "--"; break;
                    }
                    if (i == 0) {
                        msg += " / ";
                    }
                }
                m_statusManager.processStatusData(msg);
            }

            boolean ok = (state == STATE_LAMP_OFF);
            if (ok != m_lampOnOk) {
                m_lampOnOk = ok;
                m_statusManager.processStatusData("uvLampOnOk " + ok);
            }
            ok = (state == STATE_LAMP_ON
                  || state == STATE_READY
                  || state == STATE_STOPPED);
            if (m_firstStatus || ok != m_lampOffOk) {
                m_lampOffOk = ok;
                m_statusManager.processStatusData("uvLampOffOk " + ok);
            }
            ok = (state == STATE_READY);
            if (m_firstStatus || ok != m_autoZeroOk) {
                m_autoZeroOk = ok;
                m_statusManager.processStatusData("uvAutoZeroOk " + ok);
            }
            ok = true;          // NB: Reset always OK -- for now
            //ok = (state == STATE_READY
            //      || state == STATE_STOPPED);
            if (m_firstStatus || ok != m_resetOk) {
                m_resetOk = ok;
                m_statusManager.processStatusData("uvResetOk " + ok);
            }
            ok = (state == STATE_LAMP_OFF
                  || state == STATE_READY
                  || state == STATE_STOPPED);
            if (m_firstStatus || ok != m_downloadOk) {
                m_downloadOk = ok;
                m_statusManager.processStatusData("uvDownloadOk " + ok);
            }
            ok = (state == STATE_READY
                  || state == STATE_STOPPED);
            if (m_firstStatus || ok != m_startOk) {
                m_startOk = ok;
                m_statusManager.processStatusData("uvStartOk " + ok);
            }
            ok = (state == STATE_RUNNING);
            if (m_firstStatus || ok != m_stopOk) {
                m_stopOk = ok;
                m_statusManager.processStatusData("uvStopOk " + ok);
            }

            float lambda1 = getFloatDetectorParameter("Wavelength1");
            if (m_lambda[0] != lambda1) {
                m_lambda[0] = lambda1;
                msg = "uvLambda ";
                if (lambda1 > 0) {
                    msg += Math.round(lambda1) + " nm";
                }
                m_statusManager.processStatusData(msg);
            }
            float lambda2 = getFloatDetectorParameter("Wavelength2");
            if (m_lambda[1] != lambda2) {
                m_lambda[1] = lambda2;
                msg = "uvLambda2 ";
                if (lambda2 > 0) {
                    msg += Math.round(lambda2) + " nm";
                }
                m_statusManager.processStatusData(msg);
            }

            float abs1 = getFloatDetectorParameter("Absorbance1");
            if (m_abs[0] != abs1) {
                m_abs[0] = abs1;
                msg = "uvAttn " + Fmt.f(2, abs1) + " AU";
                m_statusManager.processStatusData(msg);
            }

            float abs2 = getFloatDetectorParameter("Absorbance2");
            if (m_abs[1] != abs2) {
                m_abs[1] = abs2;
                msg = "uvAttn2 " + Fmt.f(2, abs2) + " AU";
                m_statusManager.processStatusData(msg);
            }

            // Workaround for spurious D2 Lamp turn-on
            if (m_turnOffD2 && state == STATE_READY && d2LampStat != 0) {
                setParam("Lamp", LAMP_D2);
                setParam("Lamp Action", LAMP_OFF);
                sendCommand(ID_CMD_LAMP);
                setParam("Lamp", LAMP_VIS);
                m_turnOffD2 = false;
            }
            m_firstStatus = false;
        }
    }

    /**
     * Convert an Integer IP address into "dot format".  Note that the
     * IP address is stored with the last "dot" field in the most
     * significant byte!
     */
    public static Integer intIpAddress(String sAddr) {
        StringTokenizer toker = new StringTokenizer(sAddr, " .");
        int rtn = 0;
        try {
            while (toker.hasMoreTokens()) {
                rtn = (rtn >>> 8) + (Integer.parseInt(toker.nextToken()) << 24);
            }
        } catch (NumberFormatException nfe) {
            return null;
        }
        return rtn;
    }

    /**
     * Convert an integer (4 byte) IP address into a "dot" string.
     * Note that the IP address is stored with the last "dot" field in
     * the most significant byte!
     */
    public static String stringIpAddress(int ip) {
        String rtn = "";
        for (int i = 0; i < 4; i++) {
            rtn += Long.toString((ip >>> (8 * i)) & 0xff);
            if (i != 3) {
                rtn += ".";
            }
        }
        return rtn;
    }

    /**
     * 
     */
    public void setDataRate(int delay_ms) {
        if (delay_ms <= 0 && m_dataPoller != null) {
            m_dataPoller.quit();
        } else if (delay_ms >= 100) {
            if (m_dataPoller == null) {
                m_dataPoller = new DataPoller();
                m_dataPoller.setRate(delay_ms);
                m_dataPoller.start();
            } else {
                m_dataPoller.setRate(delay_ms);
            }
        } else {
            LcMsg.postError("JbControl.setDataRate(): Max data rate is 100 ms");
            LcMsg.writeStackTrace(new Exception("Bad data rate"));
        }
    }

    /**
     * 
     */
    public void addDataListener(LcDataListener dl) {
        m_dataListeners.add(dl);
    }


    class DataPoller extends Thread {

        private boolean mm_quit = false;
        private int mm_delay = 1000;

        public DataPoller() {
            setName("JbControl.DataPoller");
            LcMsg.postDebug("jbio", "JbControl.DataPoller created");
        }

        public void quit() {
            mm_quit = true;
            m_dataPoller = null;

            // One last data collection before quitting.
            sendCommand(ID_CMD_RETURN_MULTIPLE_SPECTRA);/*CMP*/
            m_nDataRequests++;
        }

        public void setRate(int delay) {
            mm_delay = delay;
        }

        public void run() {
            LcMsg.postDebug("jbio", "JbControl.DataPoller running");
            while (!mm_quit) {
                sendCommand(ID_CMD_RETURN_MULTIPLE_SPECTRA);/*CMP*/
                m_nDataRequests++;
                //if (m_nDataRequests > 1 && m_nDataRequests < 11) {
                //    LcMsg.postDebug("jbio", "***** nDataRequests = "
                //                    + m_nDataRequests + " *****");
                //}/*CMP*/
                try {
                    Thread.sleep(mm_delay);
                } catch (InterruptedException ie) {
                }
            }
        }
    }


    class StatusPoller extends Thread {

        private boolean mm_quit = false;
        private int mm_delay = 1000; // ms

        public StatusPoller() {
            setName("JbControl.StatusPoller");
            LcMsg.postDebug("jbio", "JbControl.StatusPoller created");
        }

        public void quit() {
            mm_quit = true;
        }

        public void setRate(int delay) {
            mm_delay = delay;
        }

        public void run() {
            LcMsg.postDebug("jbio", "JbControl.StatusPoller running");
            while (!mm_quit) {
                if (isConnected()) {
                    //if (m_nDataRequests > 0) {
                    //    sendCommand(ID_CMD_RETURN_MULTIPLE_SPECTRA);/*CMP*/
                    //} else {
                    sendCommand(ID_CMD_RETURN_STATUS);
                    //}
                } else {
                    connect();
                }
                try {
                    Thread.sleep(mm_delay);
                } catch (InterruptedException ie) {
                    LcMsg.postError("JbControl.StatusPoller: wait interrupted");
                }
            }
            m_statusPoller = null;
        }
    }

    /**
     * This class holds the parameters that affect the PDA's method.
     */
    class MethodParams {
        public int lambda1;
        public int lambda2;
        public int lambdaMin;
        public int lambdaMax;
        public int slitNumber;
        public int bunchingFactor;
        public int dataInterval_ms;

        public MethodParams(LcMethod p) {
            lambda1 = p.getLambda();
            lambda2 = p.getLambda2();
            lambdaMin = p.getLambdaMin();
            lambdaMax = p.getLambdaMax();

            // Calculate slit number from requested slit width
            int reqWidth = p.getBandwidth();
            slitNumber = 0;
            int limit = Math.min(reqWidth, 17);
            for (int width = 1; width < limit; slitNumber++, width *= 2);
                
            // Calculate bunching from data rate (round to slower rate)
            int rate_ms = p.getUvDataPeriod_ms();
            bunchingFactor = 4;   // Minimum bunching
            for (dataInterval_ms = 400;   // Minimum data interval
                 dataInterval_ms < rate_ms;
                 bunchingFactor *= 2, dataInterval_ms *= 2);
        }

        /**
         * Produces a string defining this MethodParams instance.
         */
        public String toString() {
            return ("lambda1=" + lambda1
                    + ", lambda2=" + lambda2
                    + ", lambdaMin=" + lambdaMin
                    + ", lambdaMax=" + lambdaMax
                    + ", slitNumber=" + slitNumber
                    + ", bunchingFactor=" + bunchingFactor);
        }

        /**
         * Returns true if the given Method would download the same thing
         * to the 335.
         */
        public boolean equals(MethodParams mp2) {
            //if (LcMsg.isSetFor("uvDownloadCheck")) {
                if (mp2 == null) {
                    LcMsg.postDebug("JbControl.MethodParams.equals: "
                                    + "mp2=" + mp2);
                } else if (lambda1 != mp2.lambda1) {
                    LcMsg.postDebug("MethodParams.equals: "
                                    + "lambda1=" + lambda1
                                    + ", mp2.lambda1=" + mp2.lambda1);
                } else if (lambda2 != mp2.lambda2) {
                    LcMsg.postDebug("MethodParams.equals: "
                                    + "lambda2=" + lambda2
                                    + ", mp2.lambda2=" + mp2.lambda2);
                } else if (lambdaMin != mp2.lambdaMin) {
                    LcMsg.postDebug("MethodParams.equals: "
                                    + "lambdaMin=" + lambdaMin
                                    + ", mp2.lambdaMin=" + mp2.lambdaMin);
                } else if (lambdaMax != mp2.lambdaMax) {
                    LcMsg.postDebug("MethodParams.equals: "
                                    + "lambdaMax=" + lambdaMax
                                    + ", mp2.lambdaMax=" + mp2.lambdaMax);
                } else if (slitNumber != mp2.slitNumber) {
                    LcMsg.postDebug("MethodParams.equals: "
                                    + "slitNumber=" + slitNumber
                                    + ", mp2.slitNumber=" + mp2.slitNumber);
                } else if (bunchingFactor != mp2.bunchingFactor) {
                    LcMsg.postDebug("MethodParams.equals: "
                                    + "bunchingFactor=" + bunchingFactor
                                    + ", mp2.bunchingFactor="
                                    + mp2.bunchingFactor);
                }
            //}/*CMP*/
                                       
            return (mp2 != null
                    && lambda1 == mp2.lambda1
                    && lambda2 == mp2.lambda2
                    && lambdaMin == mp2.lambdaMin
                    && lambdaMax == mp2.lambdaMax
                    && slitNumber == mp2.slitNumber
                    && bunchingFactor == mp2.bunchingFactor);
        }

        /**
         * Returns a hash code consistent with the equals() method.
         */
        public int hashCode() {
            return lambda1 + (lambda2 << 8)
                    + (lambdaMin << 16) + (lambdaMax << 20)
                    + (slitNumber << 4) + (bunchingFactor << 6);
        }
    }
}
