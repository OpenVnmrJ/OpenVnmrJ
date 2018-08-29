/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.awt.Frame;
import java.awt.event.*;
import java.io.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;
import java.text.*;


import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.*;
import vnmr.lc.jumbuck.JbControl;

import static vnmr.lc.LcDef.*;
import static vnmr.lc.LcCorbaClient.PUMP_READY;
import static vnmr.lc.LcCorbaClient.PUMP_RUNNING;;


public class LcControl
    implements DebugOutput.Listener, LcInjectListener {

    // General commands:
    private static final int ANALYZE_LC = 1;
    private static final int CHECK_SLIM_PORT = 2;
    private static final int CLEAR_EVENTS = 3;
    private static final int CORBA_CMD = 5;
    private static final int DOWNLOAD_METHOD = 6;
    private static final int EDIT_METHOD = 7;
    private static final int GPIB_MSG = 8;
    private static final int HIDE_LC_GRAPH = 9;
    private static final int HOLD_NOW = 10;
    private static final int OPEN_SLIM_PORT = 13;
    private static final int PRINT_HTML = 15;
    private static final int PRINT_LC_GRAPH = 16;
    private static final int PRINT_LOG = 17; // Not used
    private static final int PRINT_MS_GRAPH = 18;
    private static final int PRINT_SCHEDULE = 19;
    private static final int READ_DATA = 20;
    private static final int REPAINT_LC_GRAPH = 21;
    private static final int SET_DELAY = 22;
    private static final int SET_METHOD_VARIABLE = 23;
    private static final int SET_MS_ANALOG_OUT = 24;
    private static final int SET_MS_FILE = 25;
    private static final int SET_PAUSE_DURATION = 26;
    private static final int SET_PEAK_DET = 27;
    private static final int SET_PEAK_DET_TYPE = 28;
    private static final int SET_THRESH = 29;
    private static final int SHOW_LC_GRAPH = 30;
    private static final int SHOW_MS_GRAPH = 31;
    private static final int TEST = 32;
    private static final int TRANSLATE_DATA = 33;
    private static final int UPDATE_METHOD_PANEL = 35;
    private static final int UPDATE_MS_GRAPH = 36;
    private static final int UPDATE_PDA_GRAPH = 37;
    private static final int WAIT_FOR_PUMP = 38;
    private static final int WAIT_FOR_TIME = 39;
    private static final int TEST_PEAK_DETECT = 40;
    private static final int PDA_BASELINE = 41;
    private static final int PDA_RESET = 42;
    private static final int PDA_LAMP_ON = 43;
    private static final int PDA_LAMP_OFF = 44;
    private static final int PDA_LOAD_METHOD = 45;
    private static final int SET_TRANSFER_TIMES = 46;
    private static final int SERIAL_IO = 47;
    private static final int GET_PC_FILE = 48;
    private static final int PDA_VALIDATE = 49;
    private static final int PDA_D2RESET = 50;
    private static final int PDA_HGRESET = 51;
    private static final int PDA_SAVE_DATA = 52;
    private static final int PDA_IMAGE = 53;
    private static final int MS_PUMP_CONTROL = 54;
    private static final int PEAK_ACTION = 55;
    private static final int NMR_DONE_ACTION = 56;
    private static final int MANUAL_FLOW_CONTROL = 57;
    private static final int MS_STATUS = 58;
    private static final int PDA_START = 59;
    private static final int PDA_STOP = 60;
    private static final int CONNECT_UV = 61;
    private static final int CONNECT_PUMP = 62;
    private static final int CONNECT_MS = 63;
    private static final int SET_REFERENCE_TIME = 64;
    private static final int CHECK_METHOD_DOWNLOADS = 65;
    private static final int SET_FLOW_CELL = 66;
    private static final int ADC_TO_UV_STATUS = 67;
    private static final int CONNECT_AS= 68;
    private static final int AS_SETMACRO= 69;
    private static final int AS_CMD= 70;
    private static final int INJECT= 71;
    private static final int PRINT_PDA_GRAPH = 72;
    private static final int SHOW_PDA_SPECTRUM = 73;
    private static final int NMR_DONE = 74;
    private static final int WRITE_METHOD_TO_FILE = 75;
    private static final int READ_METHOD_FROM_FILE = 76;
    private static final int PRINT_METHOD = 77;
    private static final int LOAD_METHOD = 78;
    private static final int SAVE_CURRENT_METHOD = 79;
    private static final int DOWNLOAD_MS_METHOD = 80;

    // SLIM commands:
    private static final int FIRST_SLIM_CMD = 1000;
    private static final int STEP = 1000;
    private static final int STOP_RUN = 1001;
    private static final int PAUSE_RUN = 1002;
    private static final int STOP_FLOW_NMR = 1003;
    private static final int HOLD = 1004;
    private static final int START_RUN = 1005;
    private static final int RESTART_RUN = 1006;
    private static final int TO_COLLECTOR = 1007;
    private static final int TO_COLUMN = 1008;
    private static final int TO_NMR = 1009;
    private static final int TO_WASTE = 1010;
    private static final int ENABLE_TRIGGER = 1011;
    private static final int DISABLE_TRIGGER = 1012;
    private static final int ENABLE_HEARTBEAT = 1013;
    private static final int DISABLE_HEARTBEAT = 1014;
    private static final int COLLECT_LOOP = 1015;
    private static final int GOTO_LOOP = 1016;
    private static final int NMR_BYPASS_VALVE = 1017;
    private static final int RESTART_RUN_APPEND = 1018;
    private static final int GET_SLIM_VERSION = 1019;
    private static final int INIT = 1020;
    private static final int TIME_SLICE_NMR = 1021;
    private static final int MAKEUP_PUMP_OFF = 1022;
    private static final int MAKEUP_PUMP_ON = 1023;
    private static final int MAKEUP_PUMP_FLOW = 1024;
    private static final int MAKEUP_PUMP_PRESS = 1025;
    private static final int START_ADC = 1026;
    private static final int SLIM_CMD = 1027;
    private static final int START_CHROMATOGRAM = 1028;
    private static final int RESUME_CHROMATOGRAM = 1029;
    private static final int PAUSE_CHROMATOGRAM = 1030;
    private static final int STOP_CHROMATOGRAM = 1031;
    private static final int SLIM_PULSE = 1032;
    private static final int CLEAR_PLOTS = 1033;
    private static final int CONFIG_GRAPH_PANEL = 1034;
    private static final int AC_TO_NMR = 1035;
    private static final int AC_TO_WASTE = 1036;
    private static final int SF_STOP = 1037;
    private static final int SF_FLOW = 1038;

    private LcSlimIO slimIO;
    private String slimHostName = "V-Slim";
    private int slimPortNumber = 23;
    private int loopNumber;
    private int dataInterval;
    private int pointCount;
    private int NumberOfPoints, SaveInterval, NextSaveCount;
    private String m_baseFilePath = "";
    private boolean WatchInProgress, HoldInProgress;                  
    private String[] DataHeader;
    private boolean useEvents;
    private VLcPlot.LcPlot m_lcPlot;
    private VMsPlot m_vmsPlot = null;
    private Plot m_msPlot = null;
    private Plot m_pdaPlot = null;
    private VPdaPlot m_vpdaPlot = null;
    //private Plot2D m_pdaImage = null;
    private String StatusMessage;
    private short MaximumAdcValue;
    private NumberFormat nf;
    private SortedSet<LcEvent> eventQueue = new TreeSet<LcEvent>();
    private boolean[] ChannelActive, ShadowPeak;
    private boolean demoMode = false;
    private int demoPosition = 0;
    private ButtonIF m_vif = null;
    //private LcEventSchedule schedule;
    private HashMap<String, Integer> m_commandTable
        = new HashMap<String, Integer>();
    private double[] m_channelValues = null; // Latest SLIM ADC data
    private double m_uvOffset = 0;
    private String m_peakSortOrder = "chan time";
    private boolean m_haveLock = false;
    //private boolean m_useSlim = true;

    // Peak detection algorithm parameters
    public double[] m_peakDetectWidth_s = new double[MAX_TRACES];
    public double[] m_peakDetectSignalToNoise = new double[MAX_TRACES];
    public double[] m_peakDetectTestThresh = new double[MAX_TRACES];

    private LcCurrentMethod m_currentMethod = null;
    public LcMethodEditor m_methodEditor = null;/*CMP*/
    public LcConfiguration config = null;
    public MsCorbaClient msCorbaClient = null;
    public PdaCorbaClient pdaCorbaClient = null;
    public SortedSet<LcPeak> peakAnalysis = null;
    @SuppressWarnings("unchecked")
    private SortedSet<LcPeak>[] peakLists = new SortedSet[MAX_TRACES];
    private VPdaImage m_vpdaImage = null;
    private UvDetector m_uvDetector;
    private AS430Control m_autosampler= null;
    private String m_postInjectCommand= null;

    long waitTimeStarted;
    String waitTimerTickParameter;
    String waitTimerCommand;
    double waitTime;
    boolean waitTimeAborted;
    javax.swing.Timer waitTimer = null;
    

    private LcRun lcRun = null;
    private LcCorbaClient m_gpib = null;
    private LcGraphicsPanel m_graphPanel; // Holds the LC chromatogram/spectra
    private VContainer m_lcGraphPanel = null; // Parent of m_graphPanel

    // Serial Devices
    private Map<String, SerialSocketDeviceIO> m_deviceMap
        = new HashMap<String, SerialSocketDeviceIO>();
    private static Map<String, String> sm_solventTable =
        new HashMap<String, String>();


    public LcControl(ButtonIF vif, boolean haveLock) {
        Messages.postDebug("lcStartup", "LcControl.<init>");
        m_vif = vif;
        m_haveLock = haveLock;

        DebugOutput.addListener(this);

        writeLcDetectorInfo();
        writeLcDetectorMenu();

        //config = readConfiguration(); // TODO: Read LC configs
        if (config == null) {
            Messages.postDebug("lcStartup",
                               "LcControl.<init>: Make config panel");
            config = new LcConfiguration(m_vif);  // default config
        }

        Messages.postDebug("lcStartup", "LcControl.<init>: Make method editor");
        //schedule = new LcEventSchedule(); // TODO: Get correct schedule
        //m_methodEditor = new LcMethodEditor(m_vif, "LC Method Editor",
        //                                  schedule);

        // Set up the graphics panel
        Messages.postDebug("lcStartup", "LcControl.<init>: Make graph panels");
        m_lcGraphPanel = m_vif.getLcPanel();
        //showGraphPanel(false);
        m_lcGraphPanel.setLayout(new java.awt.BorderLayout());
        m_graphPanel = new LcGraphicsPanel(null, m_vif, "LcGraphicsPanel");
        m_lcGraphPanel.add(m_graphPanel, java.awt.BorderLayout.CENTER);
        m_lcGraphPanel.updateValue();

        m_vmsPlot = m_graphPanel.getMsPlot();
        if (m_vmsPlot != null) {
            m_msPlot = m_vmsPlot.getPlot();
        }

        m_vpdaImage = new VPdaImage(null, m_vif, "Pda Panel");
        //if (m_vpdaImage != null) {
        //    m_pdaImage = m_vpdaImage.getImage();
        //}

        m_vpdaPlot = m_graphPanel.getPdaPlot();
        if (m_vpdaPlot != null) {
            m_pdaPlot = m_vpdaPlot.getPlot();
        }

        m_lcPlot = m_graphPanel.getPlot();
        if (m_lcPlot == null) {
            // TODO: do something creative here?
            return;
        }
        m_lcPlot.setLcControl(this);

        makeCommandTable();

        String strUvOffset = System.getProperty("uvOffset");
        if (strUvOffset != null) {
            try {
                m_uvOffset = Double.parseDouble(strUvOffset);
            } catch (NumberFormatException nfe) {}
        }

        //m_useSlim = !DebugOutput.isSetFor("NoSlim");
        String slimName = System.getProperty("slimHost");
        if (slimName != null) {
            slimHostName = slimName;
        }
        String slimPort = System.getProperty("slimPort");
        if (slimPort != null) {
            try {
                slimPortNumber = Integer.parseInt(slimPort);
            } catch (NumberFormatException nfe) {
                Messages.postDebug("Bad slimPort specified: \"" + slimPort
                                   + "\"; using default: " + slimPortNumber);
            }
        }
        Messages.postDebug("lcStartup", "LcControl.<init>: Start SLIM comm");
        slimIO = new LcSlimIO(this);

        eventQueue = new TreeSet<LcEvent>();
        Solvents.init("INTERFACE/LcSolvents.txt");

        //m_gpib = new LcCorbaClient(this, vif);/*CMP*/
        //if (m_haveLock) {
        //    // We have permission to connect to LC modules
        //    Messages.postDebug("lcStartup",
        //                       "LcControl.<init>: Start GPIB CORBA comm");
        //    m_gpib = new LcCorbaClient(this, vif);
        //    Messages.postDebug("lcStartup",
        //                       "LcControl.<init>: Start MS CORBA comm");
        //    try{
        //        Messages.postDebug("lcStartup",
        //                           "LcControl.<init>: Start PDA CORBA comm");
        //        pdaCorbaClient = new PdaCorbaClient(this, vif, m_vpdaPlot);
        //    } catch(Exception e){
        //        Messages.postDebug("Cannot create PdaCorbaClient: " + e);
        //    }
        //}

        // TODO: Set PDA335 IP address
        //m_uvDetector = new vnmr.lc.jumbuck.JbControl(null,
        //                                             (StatusManager)m_vif);

        sendToVnmr("lcRunActive=0");

        // Update some values in VJ from VBG parameters
        sendToVnmr("lccmd('setPeakDetType', 1, "
                   + "lcPeakDetectWidth[1], lcPeakDetectS2N[1],"
                   + "lcPeakDetectTestThresh[1])");
        sendToVnmr("lccmd('setPeakDetType', 2, "
                   + "lcPeakDetectWidth[2], lcPeakDetectS2N[2],"
                   + "lcPeakDetectTestThresh[2])");
        sendToVnmr("lccmd('setPeakDetType', 3, "
                   + "lcPeakDetectWidth[3], lcPeakDetectS2N[3],"
                   + "lcPeakDetectTestThresh[3])");
        sendToVnmr("lccmd('setPeakDetType', 4, "
                   + "lcPeakDetectWidth[4], lcPeakDetectS2N[4],"
                   + "lcPeakDetectTestThresh[4])");
        sendToVnmr("lccmd('setPeakDetType', 5, "
                   + "lcPeakDetectWidth[5], lcPeakDetectS2N[5],"
                   + "lcPeakDetectTestThresh[5])");
        sendToVnmr("lcFlowCal('read')"); // Update the xfer times from system

        // Connect to configured LC modules
        sendToVnmr("lccmd('connectUv', lcDetector, lcDetectorAddress)");
        sendToVnmr("lccmd('connectPump', lcPump)");
        sendToVnmr("lccmd('connectMs', msSelection)");
        sendToVnmr("lccmd('connectAutosamp', lcAutoSampler, lcAutoSamplerAddress)");
        sendToVnmr("lccmd('configGraphPanel')");
        sendToVnmr("lccmd('loadMethod', lcMethodFile)");

        Messages.postDebug("lcStartup", "LcControl.<init>: DONE");
    }

    private void makeCommandTable() {
        m_commandTable.clear();
        for (int i = 0; i < CMD_TABLE.length; i++) {
            m_commandTable.put(((String)CMD_TABLE[i][0]).toLowerCase(),
                             (Integer)CMD_TABLE[i][1]);
        }
    }

    public boolean useSlim() {
        //return m_useSlim;
        return m_haveLock && !DebugOutput.isSetFor("NoSlim");
    }

    public boolean openSlimPort() {
        return openSlimPort(slimHostName, slimPortNumber);
    }

    private StringTokenizer openSlimPort(StringTokenizer toker) {
        openSlimPort();
        return toker;
    }

    public boolean openSlimPort(String hostName, int portNumber) {
        return openSlimPort(hostName, portNumber, false);
    }

    public boolean openSlimPort(String hostName,
                                int portNumber, boolean ifCheck) {
        Messages.postDebug("LcSlim", "LcControl.openSlimPort(): hostName="
                           + hostName);
        boolean ok = slimIO.openPort(hostName, portNumber, ifCheck);
        if (ok) {
            if (config.isAcHere()) {
                slimIO.getLoopPosition();
            }
        }
        return ok;
    }

    private void checkSlimPort(StringTokenizer toker) {
        openSlimPort(slimHostName, slimPortNumber, true);
    }

    public String getMyIpAddress() {
        return config.getMyIpAddress();
    }

    public int getLoop() {
        return loopNumber;
    }

    public void updateLoopPosition(int position) {
        if (DebugOutput.isSetFor("noAC")) {
            return;
        }
        sendToVnmr("lcLoopIndex=" + position);
        loopNumber = position;
    }

    public boolean execCommand(String str) {
        Messages.postDebug("lcCommands", "execCommand(" + str + ")");
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
        if (tok.hasMoreTokens()) {
            tok.nextToken();        // Eat the "lccmd" token
        }
        if (tok.hasMoreTokens()) {
            String cmd = tok.nextToken().trim().toLowerCase();
            Integer nCmd = m_commandTable.get(cmd);
            if (nCmd == null) {
                StringTokenizer toker2 = new StringTokenizer(str);
                if (toker2.countTokens() > 1) {
                    toker2.nextToken();        // Eat the "lccmd" token
                    str = toker2.nextToken("");
                }
                Messages.postError("Unknown \"lccmd\": " + str);
                return false;
            } else {
                return execCommand(cmd, nCmd, tok);
            }
        }
        return false;
    }

    public boolean execCommand(String cmd, int iCmd, StringTokenizer args) {
        SwingWorker worker;

        if (iCmd >= FIRST_SLIM_CMD) {
            if (useSlim() && !checkSlimCommand(cmd, args))
            {
                return false;
            }
        }
        boolean rtn = true;
        switch (iCmd) {
        case OPEN_SLIM_PORT:
            openSlimPort(args);
            break;
        case CHECK_SLIM_PORT:
            checkSlimPort(args);
            break;
        case EDIT_METHOD:
            editMethod(args);
            break;
        case LOAD_METHOD:
            loadMethod(args);
            break;
        case SAVE_CURRENT_METHOD:
            saveCurrentMethod(args);
            break;
        case PRINT_SCHEDULE:
            printSchedule(args);
            break;
        case SET_METHOD_VARIABLE:
            setMethodVariable(args);
            break;
        case REPAINT_LC_GRAPH:
            m_lcGraphPanel.repaint();
            break;
        case SHOW_LC_GRAPH:
            showGraphPanel(true);
            break;
        case PRINT_LC_GRAPH:
            printLcGraph(args);
            break;
        case PRINT_MS_GRAPH:
            printMsGraph(args);
            break;
        case PRINT_PDA_GRAPH:
            printPdaGraph(args);
            break;
        case SHOW_PDA_SPECTRUM:
            showPdaSpectrum(args);
            break;
        case PRINT_HTML:
            printHtml(args);
            break;
        case ANALYZE_LC:
            analyzeLc(args);
            break;
        case SHOW_MS_GRAPH:
            showMsPanel(args);
            break;
        case UPDATE_MS_GRAPH:
            setMsDataRate(args);
            break;
        case UPDATE_PDA_GRAPH:
            //uses callback, no longer valid
            //setPdaDataRate(args);
            break;
        case HIDE_LC_GRAPH:
            showGraphPanel(false);
            break;
        case UPDATE_METHOD_PANEL:
            updateMethodPanel();
            break;
        case READ_DATA:
            readData(args);
            break;
        case DOWNLOAD_METHOD:
            downloadMethod(args);
            break;
        case CHECK_METHOD_DOWNLOADS:
            checkMethodDownloads();
            break;
        case TEST:
            String argstr = "";
            if (args.hasMoreTokens()) {
                argstr = " " + args.nextToken("").trim();
            }
            Messages.postInfo("user.language="
                              + System.getProperty("user.language")
                              + ", user.country="
                              + System.getProperty("user.country")
                              + ", default locale=" + Locale.getDefault()
                              + " ("
                              + Locale.getDefault().getDisplayLanguage()
                              + ", "
                              + Locale.getDefault().getDisplayCountry()
                              + ")");
            break;
        case CORBA_CMD:
            corbaCommand(args);
            break;
        case SET_THRESH:
            setThreshold(args);
            break;
        case SET_PEAK_DET:
            setPeakDetect(args);
            break;
        case SET_PEAK_DET_TYPE:
            setPeakDetectType(args);
            break;
        case TEST_PEAK_DETECT:
            testPeakDetect(args);
            break;
        case SET_DELAY:
            setDelay(args);
            break;
        case GPIB_MSG:
            gpibMessage(args);
            break;
        case HOLD_NOW:
            holdNow();
            break;
        case TRANSLATE_DATA:
            translateData(args);
            break;
        case PDA_START:
            startPda();
            break;
        case PDA_STOP:
            stopPda();
            break;
        case PDA_BASELINE:
            baselinePda();
            break;
        case PDA_RESET:
            resetPda();
            break;
        case PDA_LAMP_ON:
            lampOnPda();
            break;
        case PDA_LAMP_OFF:
            lampOffPda();
            break;
        case PDA_LOAD_METHOD:
            loadMethodPda();
            break;
        case PDA_VALIDATE:
            validatePda();
            break;
        case PDA_D2RESET:
            resetD2Pda();
            break;
        case PDA_HGRESET:
            resetHgPda();
            break;
        case PDA_SAVE_DATA:
            saveDataPda(args);
            break;
        case PDA_IMAGE:
            getImagePda(args);
            break;
        case SET_TRANSFER_TIMES:
            setTransferTimes(args);
            break;
        case SET_REFERENCE_TIME:
            setReferenceTime(args);
            break;
        case DOWNLOAD_MS_METHOD:
            downloadMsMethod(args);
            break;
        case SET_MS_FILE:
            setMsFile(args);
            break;
        case MAKEUP_PUMP_OFF:
            setMakeupPumpOn(false, args);
            break;
        case MAKEUP_PUMP_ON:
            setMakeupPumpOn(true, args);
            break;
        case MAKEUP_PUMP_FLOW:
            getMakeupPumpFlow(args);
            break;
        case MAKEUP_PUMP_PRESS:
            getMakeupPumpPress(args);
            break;
        case WAIT_FOR_PUMP:
            waitForPump(args);
            break;
        case WAIT_FOR_TIME:
            waitForTime(args);
            break;
        case SET_PAUSE_DURATION:
            setPauseDuration();
            break;
        case CLEAR_EVENTS:
            clearEvents();
            break;
        case SET_MS_ANALOG_OUT:
            setMsAnalogOut(args);
            break;
        case SERIAL_IO:
            serialIO(args);
            break;
        case GET_PC_FILE:
            getPcFile(args);
            break;
        case MS_PUMP_CONTROL:
            msPumpControl(args);
            break;
        case PEAK_ACTION:
            peakAction(args);
            break;
        case NMR_DONE_ACTION:
            nmrDoneAction(args);
            break;
        case MANUAL_FLOW_CONTROL:
            manualFlowControl(args);
            break;
        case MS_STATUS:
            msStatus(args);
            break;
        case CONNECT_UV:
            worker = new SwingWorker(args) {
                    public Object construct() {
                        connectUv((StringTokenizer)parameter);
                        Messages.postDebug("connectUv() returned");
                        return null;
                    }
                };
            worker.start();
            //connectUv(args);
            break;
        case CONNECT_PUMP:
            worker = new SwingWorker(args) {
                    public Object construct() {
                        connectPump((StringTokenizer)parameter);
                        Messages.postDebug("connectPump() returned");
                        return null;
                    }
                };
            worker.start();
            //connectPump(args);
            break;
        case CONNECT_MS:
            connectMs(args);
            break;
        case SET_FLOW_CELL:
            setFlowCell(args);
            break;
        case ADC_TO_UV_STATUS:
            adcToUvStatus(args);
            break;
        case CONNECT_AS:
            connectAutosamp(args);
            break;
        case AS_SETMACRO:
            autosampSetMacro(args);
            break;
        case AS_CMD:
            autosampCmd(args);
            break;
        case INJECT:
            inject(args);
            break;

            /*
             * The following commands use the SLIM
             */
        case INIT:
            // No special action; we are now initialized
            break;
        case STEP:
            cmdStep_Click();
            break;
        case STOP_RUN:
            cmdStopRun_Click();
            break;
        case PAUSE_RUN:
            cmdPauseRun_Click();
            break;
        case STOP_FLOW_NMR:
            //Messages.postInfo("stopFlowNmr");/*CMP*/
            if (isActive(lcRun)) {
                lcRun.stopFlowNmr();
            }
            break;
        case TIME_SLICE_NMR:
            //Messages.postInfo(cmd);/*CMP*/
            timeSliceNmr(args);
            break;
        case HOLD:
            //
            break;
        case START_RUN:
            //Messages.postInfo("Start run.");/*CMP*/
            cmdStartRun_Click(args);
            break;
        case START_ADC:
            startAdc(args);
            break;
        case SLIM_CMD:
            slimCmd(args);
            break;
        case SLIM_PULSE:
            slimPulse(args);
            break;
        case NMR_DONE:
            nmrDone();
            break;
        case RESTART_RUN:
            //Messages.postInfo("Restart run.");/*CMP*/
            restartRun();
            break;
        case RESTART_RUN_APPEND:
            //Messages.postInfo("Restart run (append).");/*CMP*/
            restartRunAppend();
            break;
        case TO_COLLECTOR:
            optToCollectorWaste_Click();
            break;
        case TO_COLUMN:
            optToColumn_Click();
            break;
        case TO_NMR:
            optToNmr_Click();
            sfFlow();
            break;
        case TO_WASTE:
            optToWaste_Click();
            sfStop();
            break;
        case AC_TO_NMR:
            optToNmr_Click();
            break;
        case AC_TO_WASTE:
            optToNmr_Click();
            break;
        case SF_STOP:
            sfStop();
            break;
        case SF_FLOW:
            sfFlow();
            break;
        case NMR_BYPASS_VALVE:
            setNmrBypass(args);
            break;
        case ENABLE_TRIGGER:
            slimIO.enableNmrTriggerInput();
            break;
        case DISABLE_TRIGGER:
            slimIO.disableNmrTriggerInput();
            break;
        case ENABLE_HEARTBEAT:
            slimIO.enableHeartbeat();
            break;
        case DISABLE_HEARTBEAT:
            slimIO.disableHeartbeat();
            break;
        case COLLECT_LOOP:
            collectLoop(args);
            break;
        case GOTO_LOOP:
            gotoLoop(args);
            break;
        case GET_SLIM_VERSION:
            slimIO.getSlimVersion();
            break;
        case CLEAR_PLOTS:
            clearPlots();
            break;
        case CONFIG_GRAPH_PANEL:
            configGraphPanel(lcRun);
            break;
        case START_CHROMATOGRAM:
            //startChromatogram();
            break;
        case PAUSE_CHROMATOGRAM:
            pauseChromatogram();
            break;
        case RESUME_CHROMATOGRAM:
            resumeChromatogram();
            break;
        case STOP_CHROMATOGRAM:
            stopChromatogram();
            break;
        case WRITE_METHOD_TO_FILE:
            writeMethodToFile(args);
            break;
        case READ_METHOD_FROM_FILE:
            readMethodFromFile(args);
            break;
        case PRINT_METHOD:
            printMethod(args);
            break;
        default:
            rtn = false;
        }
        return rtn;
    }

    /**
     * Called through lccmd('AdcToUvStatus', < adc#1:3 >)
     */
    private void adcToUvStatus(StringTokenizer toker) {
        int iadc = 0;
        if(toker.hasMoreTokens()) {
            try {
                iadc = Integer.parseInt(toker.nextToken()) - 1;
            } catch (NumberFormatException nfe) {
            }
        }
        slimIO.setUvStatusManager(getStatusManager(), iadc);
    }
    
    /*
     * Autosampler Commands
     * See Endurance Manual for details 
     */
    
    /**
     * Called through lccmd('autosampSetMacro', <id>, <macro>)
     */
    private void autosampSetMacro(StringTokenizer toker){
        if (m_autosampler == null) {
            return;
        }
        int ntoks = toker.countTokens();
        if (ntoks == 0) {
            Messages.postError("LcControl.autosampSetMacro: no arguments");
        } else {
            String token = "";
            int id = 0;
            try {
                token = toker.nextToken();
                id = Integer.parseInt(token);
                String macro = "";
                if (ntoks > 1) {
                    macro = toker.nextToken("").trim();
                }
                m_autosampler.setMacro(m_vif, id, macro);
            } catch (NumberFormatException nfe) {
                Messages.postError("Non-numeric ID in autosampSetMacro: \""
                                   + token + "\"");
            }
        }
    }

    /**
     * Called through lccmd('autosampCmd', <string_command> [, < arg >, ...]).
     */
    private void autosampCmd(StringTokenizer args) {
        if (m_autosampler == null || !args.hasMoreTokens()) {
            return;
        }
        String strArgs = args.nextToken(""); // All the args as string
        args = new StringTokenizer(strArgs); // ... and the original tokenizer
        int ntoks = args.countTokens();
        String cmd = args.nextToken();
        if (cmd.equals("start") || (cmd.equals("download") && ntoks > 6)) {
            // This is an injection
            if (m_uvDetector != null
                && !m_uvDetector.isDownloaded(m_currentMethod))
            {
                Messages.postError("Download detector method before injecting");
                return;
            }
        }
        args = new StringTokenizer(strArgs); // The original tokenizer again
        m_autosampler.command(args);
    }
        
    /**
     * Send a status string to ExpPanel - as if from Infostat.
     * General format is:
     * <br>"Tag Value <units>"
     * <br>E.g.: "lklvl 82.3"
     * <br>or "spinval 0 Hz"
     * @param msg Status string to send.
     */
    public void sendStatusMessage(String msg) {
        ((ExpPanel)m_vif).processStatusData(msg);
    }

    public StatusManager getStatusManager() {
        return (StatusManager)m_vif;
    }

    /**
     * Execute a command for a socket connected serial device.
     * The first token is always the name of the device, and the
     * rest are the command followed by arguments.
     */
    private void serialIO(StringTokenizer toker) {
        String hostname = "";
        if (toker.hasMoreTokens()) {
            hostname = toker.nextToken();
        }
        SerialSocketDeviceIO device;
        device = m_deviceMap.get(hostname);
        if (device == null) {
            device = new SerialSocketDeviceIO(m_vif, hostname);
            m_deviceMap.put(hostname, device);
        }
        device.exec(toker);
    }

    private void getPcFile(  StringTokenizer toker) {
        if (msCorbaClient == null) {
            return;
        }
        // Args:
        // Src (remote) file path
        // Dst (local) file path
        // Local user name
        // Password
        // Local IP address
        String sSrcPath = "c:/xxx";
        String sDstPath = "/tmp/xxx";
        String sUser = "anon";
        String sPasswd = "anonpasswd";
        String sIpAddr = getMyIpAddress();

        if (toker.hasMoreTokens()) {
            QuotedStringTokenizer qtoker;
            qtoker = new QuotedStringTokenizer(toker.nextToken(""));
            if (qtoker.hasMoreTokens()) {
                sSrcPath = qtoker.nextToken();
            }
            if (qtoker.hasMoreTokens()) {
                sDstPath = qtoker.nextToken();
            }
            if (qtoker.hasMoreTokens()) {
                sUser = qtoker.nextToken();
            }
            if (qtoker.hasMoreTokens()) {
                sPasswd = qtoker.nextToken();
            }
            if (qtoker.hasMoreTokens()) {
                sIpAddr = qtoker.nextToken();
            }
        }
        msCorbaClient.getFile(sSrcPath, sDstPath, sUser, sPasswd, sIpAddr);
    }

    private void downloadMethod(StringTokenizer toker) {
        //m_currentMethod.sendMethodToPump();
    }

    private void checkMethodDownloads() {
        if (m_uvDetector != null) {
            boolean ok = m_uvDetector.isDownloaded(m_currentMethod);
            if (ok) {
                sendToVnmr("write('line3', 'UV method already downloaded')");
            } else {
                sendToVnmr("write('error', 'UV method is out-of-date')");
            }
        }
    }

    private void corbaCommand(StringTokenizer toker) {
        String module = null;
        String command = null;
        if (toker.hasMoreTokens()) {
            module = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            command = toker.nextToken("").trim();
        }
        //m_currentMethod.detector9050Cmd(command);
    }

    private void msPumpControl(StringTokenizer toker) {
        String sType = "y";
        boolean bType = true;
        if (toker.hasMoreTokens()) {
            sType = toker.nextToken();
        }
        if (sType.equals("n") || sType.equals("false")) {
            bType = false;
        }
        config.setMsPumpControl(bType);
    }

    private void setTransferTimes(StringTokenizer toker) {
        int n = toker.countTokens();
        double[] valArray = new double[n];
        for (int i = 0; i < n; ++i) {
            try {
                valArray[i] = Double.parseDouble(toker.nextToken());
            } catch (NumberFormatException nfe) {}
        }
        if (DebugOutput.isSetFor("lcTransferTimes")) {
            String msg = "setTransferTimes(): ";
            for (int i = 0; i < valArray.length; i++) {
                if (i > 0) {
                    msg += ", ";
                }
                msg += Fmt.f(2, valArray[i]);
            }
            Messages.postDebug(msg);
        }
        config.setTransferTimes(valArray);
    }

    private void setReferenceTime(StringTokenizer toker) {
        double value = 0;
        if (toker.hasMoreTokens()) {
            try {
                value = Double.parseDouble(toker.nextToken());
            } catch (NumberFormatException nfe) {}
        }
        Messages.postDebug("lcTransferTimes",
                           "setReferenceTime() : " + Fmt.f(2, value));
        config.setReferenceTime(value);
    }

    /**
     * Set the lcPeakDetect[1:n] parameter in VnmrBG.
     * <pre>
     * Usage 1: lccmd('setPeakDet {trace} {y|n}')
     * where {trace} is the trace number (from 1)
     * </pre>
     * If there is not an active run, peak detection is turned on/off for
     * the entire run.
     * If there is a run active, the peak detection is turned on/off for
     * the current time; any previously specified peak detection changes
     * remain in force.
     * 
     * <pre>
     * Usage 2: lccmd('setPeakDet lcPeakDetect{x}')
     * where {x} is the trace letter (A through E).
     * </pre>
     * Usage 2 is deprecated.<br>
     * Sets the parameter lcPeakDetectX to match the current state of peak
     * detection (y or n) on trace X. If there is no run active,
     * sets the state for t=0.
     */
    private void setPeakDetect(StringTokenizer toker) {
        String sChan = null;
        String sValue = null;
        if (toker.hasMoreTokens()) {
            sChan = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            sValue = toker.nextToken();
        }
        int iChan = 0;
        try {
            iChan = Integer.parseInt(sChan) - 1;
            m_currentMethod.setPeakEnabled(iChan, sValue, lcRun);
        } catch (NumberFormatException nfe) {
            m_currentMethod.setPeakEnabled(sChan, lcRun);
        }
    }

    private void testPeakDetect(StringTokenizer toker) {
        if (lcRun == null) {
            return;
        }
        String sChan = "0";
        int iChan = -1;
        if (toker.hasMoreTokens()) {
            sChan = toker.nextToken();
        }
        try {
            iChan = Integer.parseInt(sChan) - 1;
        } catch (NumberFormatException nfe) {}
        lcRun.testPeakDetect(iChan);
    }

    private void setPeakDetectType(StringTokenizer toker) {
        String sChan = "1";
        String sWidth = "5";
        String sStoN = "10";
        String sThresh = "0";
        int iChan = 0;
        double dWidth_s = 5;
        double dStoN = 10;
        double dThresh = 0;
        if (toker.hasMoreTokens()) {
            sChan = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            sWidth = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            sStoN = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            sThresh = toker.nextToken();
        }
        try {
            iChan = Integer.parseInt(sChan) - 1;
        } catch (NumberFormatException nfe) {}
        try {
            dWidth_s = Double.parseDouble(sWidth);
        } catch (NumberFormatException nfe) {}
        try {
            dStoN = Double.parseDouble(sStoN);
        } catch (NumberFormatException nfe) {}
        try {
            dThresh = Double.parseDouble(sThresh);
        } catch (NumberFormatException nfe) {}

        m_peakDetectWidth_s[iChan] = dWidth_s;
        m_peakDetectSignalToNoise[iChan] = dStoN;
        m_peakDetectTestThresh[iChan] = dThresh;
    }

    private void timeSliceNmr(StringTokenizer toker) {
        if (!isActive(lcRun)) {
            return;
        }
        double time_min = -1;
        String sValue = null;
        if (toker.hasMoreTokens()) {
            sValue = toker.nextToken();
        }
        if (sValue.equals("init")) {
            m_currentMethod.initTimeSlice(lcRun);
        } else if (sValue.equals("stop")) {
            m_currentMethod.stopTimeSlice(lcRun);
        } else {
            try {
                time_min = Double.parseDouble(sValue);
            } catch (NumberFormatException nfe) {}
            lcRun.nextTimeSlice(time_min);
        }
    }

    /** Not recommended */
    private void setDelay(StringTokenizer toker) {
        if (!isActive(lcRun)) {
            return;
        }
        String sChan = null;
        String sValue = null;
        if (toker.hasMoreTokens()) {
            sChan = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            sValue = toker.nextToken();
        }
        int iChan = 0;
        try {
            iChan = Integer.parseInt(sChan) - 1;
        } catch (NumberFormatException nfe) {}
        double dValue = 0;
        try {
            dValue = Double.parseDouble(sValue);
        } catch (NumberFormatException nfe) {}
        lcRun.setDelay(iChan, dValue);
    }

    private void gpibMessage(StringTokenizer toker) {
        String module = "";
        //String sAddr = null;
        String msg = null;
        if (toker.hasMoreTokens()) {
            module = toker.nextToken();
        }
        //if (toker.hasMoreTokens()) {
        //    sAddr = toker.nextToken();
        //}
        if (toker.hasMoreTokens()) {
            msg = toker.nextToken("").trim();
        }
        if (DebugOutput.isSetFor("lcPumpMethod")
            && "9012".equals(module)
            && msg != null
            && msg.toLowerCase().indexOf("downloadmethod") >= 0)

        {
            // This will print out the pump method as a side-effect
            LcCorbaClient.get9012Method(m_currentMethod);
        }
        //int iAddr = 0;
        //try {
        //    iAddr = Integer.parseInt(sAddr);
        //} catch (NumberFormatException nfe) {}
        //m_gpib.sendMessage(module, iAddr, msg);
        if (m_gpib == null) {
            return;
        }
        m_gpib.sendMessage(module, msg);
    }

    public void pumpStartRun() {
        Messages.postDebug("pumpCommands",
                           "m_gpib.sendMessage(\"9012\", \"start\")");
        if (m_gpib == null) {
            return;
        }
        m_gpib.sendMessage("9012", "start");
    }

    public void pumpStopRun() {
        Messages.postDebug("pumpCommands",
                           "m_gpib.sendMessage(\"9012\", \"stop\")");
        if (m_gpib == null) {
            return;
        }
        m_gpib.sendMessage("9012", "stop");
    }

    private void holdNow() {
        if (isActive(lcRun)) {
            lcRun.holdNow();
        }
    }

    private void setPauseDuration() {
        if (lcRun == null) {
            return;
        }
        lcRun.setPauseDuration();
    }

    private void clearEvents() {
        if (lcRun == null) {
            return;
        }
        lcRun.clearEvents();
    }

    private void translateData(StringTokenizer toker) {
        String infile = "";
        String outfile = "";
        if (toker.hasMoreTokens()) {
            infile = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            outfile = toker.nextToken();
        }
        LcRun.translateLcdata(infile, outfile);
    }

    /**
     * Usage: "setMsAnalogOut channel type scale mass command"
     * <br>channel = 0/1: first or second channel
     * <br>type = "off"/"tic"/"area"/"user"
     * <br>scale = float scale factor for "tic" and "area" modes
     * <br>mass = float mass for "area" mode
     * <br>command = any PML expression for "user" mode
     */
    private void setMsAnalogOut(StringTokenizer toker) {
        if (msCorbaClient == null) {
            return;
        }
        int channel = 0;
        String type = "off";
        double scale = 1.0e-9;
        double mass = 100;
        String command = "";
        if (toker.hasMoreTokens()) {
            String str = toker.nextToken();
            try {
                channel = Integer.parseInt(str);
            } catch (NumberFormatException nfe) {}
        }
        if (toker.hasMoreTokens()) {
            type = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            String str = toker.nextToken();
            try {
                scale = Double.parseDouble(str);
            } catch (NumberFormatException nfe) {}
        }
        if (toker.hasMoreTokens()) {
            String str = toker.nextToken();
            try {
                mass = Double.parseDouble(str);
            } catch (NumberFormatException nfe) {}
        }
        if (toker.hasMoreTokens()) {
            command = toker.nextToken(""); // Remainder of line is command
        }
        msCorbaClient.setAnalogOut(channel, type, command, mass, scale);
    }

    private void downloadMsMethod(StringTokenizer toker) {
        if (toker.hasMoreTokens()) {
            String methodName = toker.nextToken();
            m_currentMethod.setMsMethod(methodName, lcRun);
        }
    }

    private void setMsFile(StringTokenizer toker) {
        String file = "";
        if (toker.hasMoreTokens()) {
            file = toker.nextToken();
        }
        //(file);
    }

    private void setMsDataRate(StringTokenizer toker) {
        if (msCorbaClient == null) {
            return;
        }

        int rate = 0;
        if (toker.hasMoreTokens()) {
            String sRate = toker.nextToken();
            try {
                rate = Integer.parseInt(sRate);
            } catch (NumberFormatException nfe) {}
        }
        m_graphPanel.setMsDataRate(msCorbaClient, rate);
    }

    private void setPdaDataRate(StringTokenizer toker) {
        if (pdaCorbaClient == null) {
            return;
        }

        int rate = 0;
        if (toker.hasMoreTokens()) {
            String sRate = toker.nextToken();
            try {
                rate = Integer.parseInt(sRate);
            } catch (NumberFormatException nfe) {}
        }
        m_graphPanel.setPdaDataRate(pdaCorbaClient, rate);
    }

    public void setPdaPlotWavelengths(float[] wavelengths) {
        if (m_uvDetector != null && m_uvDetector.isPda() && m_vpdaPlot != null) {
            m_vpdaPlot.setXArray(wavelengths);
        }
    }

    public void setPdaPlotData(double time, float[] data) {
        if (m_uvDetector != null && m_uvDetector.isPda() && m_vpdaPlot != null) {
            m_pdaPlot.setTitle("Spectrum at " + Fmt.f(2, time));
            m_vpdaPlot.setYArray(data);
        }
    }

    /**
     * Connect to a particular UV detector, and start getting status.
     */
    private void connectUv(StringTokenizer toker) {
        if (m_haveLock) {
            //if (m_gpib == null) {
            //    m_gpib = LcCorbaClient.getClient(this, m_vif);
            //}
            String name = "";
            if (m_uvDetector != null) {
                name = m_uvDetector.getName();
                m_uvDetector.disconnect();
                m_uvDetector.setDisconnectedStatus();
                m_uvDetector = null;
            }
            if (toker.hasMoreTokens()) {
                name = toker.nextToken();
            }
            if (name.equals("9050") || name.equals("310")) {
                if (m_gpib == null) {
                    m_gpib = LcCorbaClient.getClient(this, m_vif);
                }
                if (m_gpib == null) {
                    LcMsg.postError("No connection to CORBA GPIB server");
                } else {
                    m_uvDetector = m_gpib.getDetector(name);
                }
            } else if (name.equals("330")) {
                try{
                    pdaCorbaClient = new PdaCorbaClient(this, m_vif, m_vpdaPlot);
                } catch(Exception e){
                    Messages.postDebug("Cannot create PdaCorbaClient: " + e);
                }
            } else if (name.equals("335")) {
                Integer ip = null;
                String strIP = "";
                if (toker.hasMoreTokens()) {
                    strIP = toker.nextToken();
                    ip = JbControl.intIpAddress(strIP);
                    if (ip == null) {
                        try {
                            InetAddress ina = InetAddress.getByName(strIP);
                            strIP = ina.getHostAddress();
                            ip = JbControl.intIpAddress(strIP);
                        } catch (UnknownHostException uhe) {
                            LcMsg.postDebug("Unknown host for PDA 335: "
                                            + strIP);
                        }
                    }
                }
                if (ip == null) {
                    LcMsg.postError("Bad address for PDA 335: " + strIP);
                    // TODO: Get IP from SLP?
                    return;
                }
                m_uvDetector = new JbControl(ip, (StatusManager)m_vif);
                int n = config.getFlowCell();
                double r = config.getFlowCellRatio();
                ((JbControl)m_uvDetector).setFlowCell(n, r);
            }
            configGraphPanel(lcRun);
        }
    }

    private void writeLcDetectorInfo() {
        PrintWriter file = null;
        String name = "USER/lc/lcDetectorInfo";
        try {
            // Open file and write header
            String path = FileUtil.savePath(name);
            if (path == null) {
                Messages.postError("Cannot open " + name + " for writing");
                return;
            }
            file = new PrintWriter(new BufferedWriter(new FileWriter(path)));
            file.println("#");
            file.println("# This file is made automatically by VnmrJ");
            file.println("# Do not edit");
            file.println("#");
            file.println("# Info on the UV detectors supported by VnmrJ-LC");
            file.println("# Used by the utility macro \"lcGetDetectorInfo\".");
            file.println("# Used by \"lcChannelMenu\" to construct menus.");
            file.println("#");
            file.println("# key PDA min max nLambda nAnalog");
            file.println("# Label");
            // Write line for 335 UV
            writeLcDetectorInfoLine(file, new JbControl());

            // Write line for 9050 UV
            writeLcDetectorInfoLine(file, new LcCorbaClient().getDetector());

            // Write line for 1200 MS
            file.println();
            file.println("1200 0 0 0 0 2");
            file.println("Label: 1200 MS");

        } catch (IOException ioe) {
            Messages.postError("Failed to write LcDetectorInfo");
        }

        if (file != null) {
            file.close();
        }
    }

    private void writeLcDetectorMenu() {
        PrintWriter file = null;
        String name = "USER/lc/lcDetectorMenu";
        try {
            // Open file and write header
            String path = FileUtil.savePath(name);
            if (path == null) {
                Messages.postError("Cannot open " + name + " for writing");
                return;
            }
            file = new PrintWriter(new BufferedWriter(new FileWriter(path)));
            file.println("#");
            file.println("# This file is made automatically by VnmrJ");
            file.println("# Do not edit");
            file.println("#");

            // Write line for 335
            writeLcDetectorMenuLine(file, new JbControl());

            // Write line for 9050
            writeLcDetectorMenuLine(file, new LcCorbaClient().getDetector());

            // Write line for "none"
            file.println("\"None\" \"0\"");

        } catch (IOException ioe) {
            Messages.postError("Failed to write LcDetectorMenu");
        }

        if (file != null) {
            file.close();
        }
    }

    private void writeLcDetectorMenuLine(PrintWriter file,
                                         UvDetector detector)
    {
        String label = detector.getIdString();
        String key = detector.getCanonicalIdString();
        file.println("\"" + label + "\" \"" + key + "\"");
    }

    private void writeLcDetectorInfoLine(PrintWriter file,
                                         UvDetector detector)
    {
        file.println();
        String key = detector.getCanonicalIdString();
        int pdaFlag = detector.isPda() ? 1 : 0;
        int[] range = detector.getMaxWavelengthRange();
        int nLambda = detector.getNumberOfMonitorWavelengths();
        int nAnalog = detector.getNumberOfAnalogChannels();
        file.println(key + " " + pdaFlag + " " + range[0] + " " + range[1]
                     + " " + nLambda + " " + nAnalog);
        String label = detector.getIdString();
        file.println("Label: " + label);
    }

    private void setFlowCell(StringTokenizer toker) {
        if (m_uvDetector instanceof JbControl) {
            int type = 3;
            double ratio = 0;
            if (toker.hasMoreTokens()) {
                try {
                    type = Integer.parseInt(toker.nextToken());
                } catch (NumberFormatException nfe) {}
            }
             if (toker.hasMoreTokens()) {
                try {
                    ratio = Double.parseDouble(toker.nextToken());
                } catch (NumberFormatException nfe) {}
            }
           ((JbControl)m_uvDetector).setFlowCell(type, ratio);
        }
    }

    /**
     * Set the IP parameters in the 335.
     */
    private void setIP335(StringTokenizer toker) {
        if (m_uvDetector instanceof JbControl) {
            Messages.postError("setIP335 not implemented");
            //jbControl.setIP(strIpAddr, strSubnet, strGateway, nBootpName);
        }
    }

    /**
     * Which lamp(s) to use for 335 PDA.
     * @return 0 for quartz-halogen, 1 for D2, 2 for both.
     */
    private int getLampSelection() {
        return config.getLampSelection();
    }

    // *********** UV / PDA Commands ***********

    public void startPda(){
        if (m_uvDetector == null) {
            return;
        }
        m_uvDetector.start();
    }

    public void stopPda(){
        if (m_uvDetector == null) {
            return;
        }
        m_uvDetector.stop();
    }

    public void pauseUv(){
        if (m_uvDetector == null) {
            return;
        }
        m_uvDetector.pause();
    }

    public void baselinePda(){
        if (m_uvDetector == null) {
            return;
        }
        m_uvDetector.autoZero();
    }

    public void resetPda(){
        if (m_uvDetector == null) {
            return;
        }
        m_uvDetector.reset();
    }

    public void lampOnPda(){
        if (m_uvDetector == null) {
            return;
        }
        if (m_uvDetector instanceof JbControl) {
            ((JbControl)m_uvDetector).selectLamp(getLampSelection());
        }
        m_uvDetector.lampOn();
    }

    public void lampOffPda(){
        if (m_uvDetector == null) {
            return;
        }
        m_uvDetector.lampOff();
    }

    public void loadMethodPda() {
        if (m_uvDetector == null || m_currentMethod == null) {
            return;
        }
        m_uvDetector.downloadMethod(m_currentMethod);
    }

    /**
     * @return Returns false if there was an error resulting in the
     * method not being saved even though it should have been.
     */
    public boolean saveCurrentMethod(StringTokenizer toker) {
        boolean rtn = true;
        if (m_currentMethod != null && toker.hasMoreTokens()) {
            String path = toker.nextToken();
            if (path.equals("MAYBE_SAVE_IF_CHANGED")
                && m_methodEditor != null)
            {
                try {
                    m_methodEditor.maybeSaveIfChanged();
                } catch (IOException mse) {
                    Messages.postError(mse.toString());
                    rtn = false;
                }
            } else {
                rtn = m_currentMethod.writeMethodToFile(path);
            }
        }
        return rtn;
    }

    private void validatePda(){
         if (pdaCorbaClient == null) {
            return;
         }
         pdaCorbaClient.validatePda();/* FIXME */
    }

    private void resetD2Pda(){
        if (pdaCorbaClient == null) {
            return;
         }
         pdaCorbaClient.resetD2Pda();/* FIXME */
    }

     private void resetHgPda(){
        if (pdaCorbaClient == null) {
            return;
         }
         pdaCorbaClient.resetHgPda();/* FIXME */
    }

    private void saveDataPda(StringTokenizer toker) {

         if (toker.hasMoreTokens()) {
            String directory = toker.nextToken();
            pdaCorbaClient.saveDataPda(directory);/* FIXME */
         }
    }

    private void getImagePda(StringTokenizer toker){
        if(m_vpdaImage == null){
            return;
        }
        if (toker.hasMoreTokens()) {
            String directory = toker.nextToken();
            File fDir = new File(directory);
            if (!fDir.isDirectory()) {
                directory = fDir.getParentFile().toString();
            }
            if (lcRun == null) {
                Messages.postWarning("The LC run is not loaded");
            } else {
                m_vpdaImage.setOffset((float)lcRun.getUvOffset_ms() / 60000f);
            }
            m_vpdaImage.setDirectory(directory);
            m_vpdaImage.showCurrentPdaImage();
         }
    }
    
    /**
     * Connect to the autosampler
     */
    private void connectAutosamp(StringTokenizer toker){
        if (m_haveLock){
            String name = "430";
            String host = "autosampler";
            int port= 4000;
                
            Messages.postDebug("430Comm", "LcControl.connectAutosamp()");
            if (m_autosampler != null) {
                Messages.postDebug("430Comm", "... calling disconnect");
                m_autosampler.disconnect();
                m_autosampler = null;
            }
            if (toker.hasMoreTokens()) {
                name= toker.nextToken();
            }
            if (toker.hasMoreTokens()) {
                host= toker.nextToken(" \t:"); // Allow "hostname:port"
            }
            if (toker.hasMoreTokens()) {
                port= Integer.parseInt(toker.nextToken());
            }
            if (name.equals("430")) {
                Messages.postDebug("430Comm", "... new autosamp: host=" + host
                                   + ", port=" + port);
                m_autosampler = new AS430Control(host,
                                                 port,
                                                 (StatusManager)m_vif,
                                                 this);
            }
        }
    }

    /**
     * Connect to the MS pump, and start getting status.
     */
    private void connectMs(StringTokenizer toker) {
        if (m_haveLock) {
            String name = "";
            if (msCorbaClient != null) {
                //name = "1200";  // Only possibility thus far (no getName())
                msCorbaClient.disconnect();
                msCorbaClient = null;
            }
            if (toker.hasMoreTokens()) {
                name = toker.nextToken();
            }
            if (name.equals("1200")) {
                msCorbaClient = new MsCorbaClient(m_vif);
            }
            configGraphPanel(lcRun);
        }
    }

    private void setMakeupPumpOn(boolean on, StringTokenizer toker) {
        double rate = 0;
        if (on && toker.hasMoreTokens()) {
            String sRate = toker.nextToken();
            try {
                rate = Double.parseDouble(sRate);
            } catch (NumberFormatException nfe) {}
        }
        slimIO.setMakeupFlow(rate);
    }

    private void getMakeupPumpFlow(StringTokenizer toker) {
        int rate = 0;           // ms between measurements
        if (toker.hasMoreTokens()) {
            String sRate = toker.nextToken();
            try {
                rate = Integer.parseInt(sRate);
            } catch (NumberFormatException nfe) {}
        }
        slimIO.setMakeupFlowQueryRate(rate);
    }

    private void getMakeupPumpPress(StringTokenizer toker) {
        int rate = 0;           // ms between measurements
        if (toker.hasMoreTokens()) {
            String sRate = toker.nextToken();
            try {
                rate = Integer.parseInt(sRate);
            } catch (NumberFormatException nfe) {}
        }
        slimIO.setMakeupPressQueryRate(rate);
    }

    /**
     * Connect to the LC pump, and start getting status.
     */
    private void connectPump(StringTokenizer toker) {
        if (m_haveLock) {
            String name = "";
            if (toker.hasMoreTokens()) {
                name = toker.nextToken();
            }
            if (name.equals("9012") || name.equals("230")) {
                if (m_gpib == null) {
                    m_gpib = LcCorbaClient.getClient(this, m_vif);
                }
                if (m_gpib == null) {
                    LcMsg.postError("No connection to CORBA GPIB server");
                } else {
                    m_gpib.getPump("9012");
                }
            }
        }
    }

    /**
     * See if the pump is ready for the method to start.
     * The pump must have the proper flow rate and solvent percentages.
     * @return True if the pump is ready.
     */
    private boolean isPumpReady() {
        Map<String,Object> state = m_gpib.getPumpState();
        Object oState = state.get("state");
        double dflow = m_currentMethod.getInitialFlow(); // mL/min
        Messages.postDebug("InitialFlow=" + dflow);
        Integer flow = (int)Math.round(1000 * dflow); // uL/min
        double[] pcts = m_currentMethod.getInitialPercents();
        Double[] percents = new Double[pcts.length];
        for (int i = 0; i < pcts.length; i++) {
            percents[i] = pcts[i];
        }
        Object pctA = state.get("percentA");
        Object pctB = state.get("percentB");
        Object pctC = state.get("percentC");
        if (DebugOutput.isSetFor("CheckPump")) {
            Messages.postDebug("isPumpReady:\n"
                               + "Item    Method    PumpState\n"
                               + "PctA " + percents[0] + " " + pctA + "\n"
                               + "PctB " + percents[1] + " " + pctB + "\n"
                               + "PctC " + percents[2] + " " + pctC + "\n"
                               + "Flow " + flow + " " + state.get("flow") + "\n"
                               + "Status ready(1)/run(2) " + oState
                               );
        }
        return (percents[0].equals(pctA)
                && percents[1].equals(pctB)
                && percents[2].equals(pctC)
                && flow.equals(state.get("flow"))
                && (oState.equals(PUMP_READY) || oState.equals(PUMP_RUNNING)));
    }

    private void waitForPump(StringTokenizer toker) {
        // Args: vnmr_command
        if (m_gpib == null) {
            return;
        }
        String command = "write('line3','Pump Ready')"; // Default cmd
        if (toker.hasMoreTokens()) {
            command = toker.nextToken(""); // Rest of string is command
        }
        if (command.trim().equals("cancel")) {
            m_gpib.clearPumpStatusListeners();
        } else {
            LcStatusListener listener
                    = new PumpListener(m_currentMethod.getInitialFlow(),
                                       m_currentMethod.getInitialPercents(),
                                       command);
            m_gpib.addPumpStatusListener(listener);
        }
    }

    private void waitForTime(StringTokenizer toker) {
        // Args: delay_time_minutes time_waited_parameter vnmr_command
        if (m_gpib == null) {
            return;
        }
        boolean cancel = false;
        double timeDelay_min = 0; // minutes
        String tickParameter = "null";
        String command = "write('line3','Time Expired')"; // Default cmd
        if (toker.hasMoreTokens()) {
            String sDelay = toker.nextToken();
            if (sDelay.equalsIgnoreCase("cancel")) {
                cancel = true;
            } else {
                try {
                    timeDelay_min = Double.parseDouble(sDelay);
                } catch (NumberFormatException nfe) {}
            }
        }
        if (toker.hasMoreTokens()) {
            tickParameter = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            command = toker.nextToken(""); // Rest of string is command
        }

        if (waitTimer != null && waitTimer.isRunning()
            && waitTimerTickParameter.equals(tickParameter))
        {
            // Abort a previous wait (and execute it's command)
            waitTimer.stop();
            sendToVnmr("write('line3','Aborting previous timed wait')");
            sendToVnmr(waitTimerCommand);
            if (!waitTimerTickParameter.equals("null")) {
                sendToVnmr(waitTimerTickParameter + "=0");
                sendToVnmr("vnmrjcmd('pnew', '1 " + waitTimerTickParameter
                           + " 0')");
            }
        }
        if (cancel) {
            return;
        }

        // Set up new handler for the timer ticks
        ActionListener waitTimeoutListener = new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                long now = new Date().getTime();
                double time = (now - waitTimeStarted) / 60000.0;
                if (time >= waitTime || waitTimeAborted) {
                    waitTimer.stop();
                    waitTimer = null;
                    time = 0;
                    sendToVnmr(waitTimerCommand);
                }
                if (!waitTimerTickParameter.equals("null")) {
                    sendToVnmr(waitTimerTickParameter + "=" + time);
                    sendToVnmr("vnmrjcmd('pnew', '1 "
                               + waitTimerTickParameter + " " + time + "')");
                }
            }
        };

        // Set up the timer parameters and start timer
        waitTimer = new javax.swing.Timer(250, waitTimeoutListener);
        waitTimeStarted = new Date().getTime();
        waitTimerTickParameter = tickParameter;
        waitTimerCommand = command;
        waitTime = timeDelay_min;
        waitTimeAborted = false;
        waitTimer.start();
    }

    private void setThreshold(StringTokenizer toker) {
        String sChan = null;
        String sThresh = null;
        if (toker.hasMoreTokens()) {
            sChan = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            sThresh = toker.nextToken();
        }
        int iChan = 0;
        double dThresh = 0;
        try {
            iChan = Integer.parseInt(sChan) - 1;
        } catch (NumberFormatException nfe) {}
        try {
            dThresh = Double.parseDouble(sThresh);
        } catch (NumberFormatException nfe) {}
        m_currentMethod.setThreshold(iChan, dThresh, lcRun);
    }

    private void updateMethodPanel() {
        //m_methodEditor.updatePanel();
    }

    private void readData(StringTokenizer tok) {
        if (tok.hasMoreTokens()) {
            String filename = tok.nextToken();
            runFromFile(filename);
        }
    }

    public String getRunPath() {
        if (lcRun != null) {
            return lcRun.getDirpath();
        } else {
            return "";
        }
    }

    private void printLcGraph(StringTokenizer tok) {
        boolean print = true;
        String path = "/tmp/LcTestPrint";
        int ptLeft = 36;
        int ptTop = 756;
        int ptWidth = 540;
        int ptHeight = 0;       // Scale by width, fixed aspect ratio

        String flag;
        boolean isFlag = true;
        while (isFlag && tok.hasMoreTokens()) {
            isFlag = false;
            flag = tok.nextToken();
            if (flag.equals("noprint")) {
                print = false;
                isFlag = true;
            } else {
                path = flag;
            }
        }
        if (!new File(path).isAbsolute()) {
            path = getRunPath() + File.separator + path;
        }
        try {
            if (tok.hasMoreTokens()) {
                ptLeft = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptTop = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptWidth = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptHeight = Integer.parseInt(tok.nextToken());
            }
        } catch (NumberFormatException nfe) {
            Messages.postError("Invalid print geometry");
        }
        String title = m_lcPlot.getTitle();
        if (title == null || title.trim().length() == 0) {
            title = "Chromatogram";
        }
        String date = Util.getStandardDateTimeString();
        m_lcPlot.printPS(path, ptLeft, ptTop, ptWidth, ptHeight,
                     "VnmrJ-LC", date, title);
        if (print) {
            sendToVnmr("lcPrint('" + path + "')");
            //sendToVnmr("shell(lcPrintCommand +' " + path + "')");
        }
        Messages.postInfo("EPS plot file in \"" + path + "\"");
    }

    private void printMsGraph(StringTokenizer tok) {
        if (m_msPlot == null) {
            Messages.postError("No MS graph to plot");
            return;
        }
        boolean print = true;
        String path = "/tmp/LcTestPrint";
        int ptLeft = 36;
        int ptTop = 756;
        int ptWidth = 540;
        int ptHeight = 0;       // Scale by width, fixed aspect ratio

        String flag;
        boolean isFlag = true;
        while (isFlag && tok.hasMoreTokens()) {
            isFlag = false;
            flag = tok.nextToken();
            if (flag.equals("noprint")) {
                print = false;
                isFlag = true;
            } else {
                path = flag;
            }
        }
        if (!new File(path).isAbsolute()) {
            path = getRunPath() + File.separator + path;
        }
        try {
            if (tok.hasMoreTokens()) {
                ptLeft = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptTop = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptWidth = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptHeight = Integer.parseInt(tok.nextToken());
            }
        } catch (NumberFormatException nfe) {
            Messages.postError("Invalid print geometry");
        }
        String title = m_lcPlot.getTitle();
        if (title == null || title.trim().length() == 0) {
            title = "Chromatogram";
        }
        String date = Util.getStandardDateTimeString();
        m_msPlot.printPS(path, ptLeft, ptTop, ptWidth, ptHeight,
                     "VnmrJ-MS", date, title);
        if (print) {
            sendToVnmr("lcPrint('" + path + "')");
            //sendToVnmr("shell(lcPrintCommand +' " + path + "')");
        }
        Messages.postInfo("EPS plot file in \"" + path + "\"");
    }

    private void printPdaGraph(StringTokenizer tok) {
        if (m_pdaPlot == null) {
            Messages.postError("No spectrum to plot");
            return;
        }
        boolean print = true;
        String path = "/tmp/LcTestPrint";
        int ptLeft = 36;
        int ptTop = 756;
        int ptWidth = 540;
        int ptHeight = 0;       // Scale by width, fixed aspect ratio

        String flag;
        boolean isFlag = true;
        while (isFlag && tok.hasMoreTokens()) {
            isFlag = false;
            flag = tok.nextToken();
            if (flag.equals("noprint")) {
                print = false;
                isFlag = true;
            } else {
                path = flag;
            }
        }
        if (!new File(path).isAbsolute()) {
            path = getRunPath() + File.separator + path;
        }
        try {
            if (tok.hasMoreTokens()) {
                ptLeft = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptTop = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptWidth = Integer.parseInt(tok.nextToken());
            }
            if (tok.hasMoreTokens()) {
                ptHeight = Integer.parseInt(tok.nextToken());
            }
        } catch (NumberFormatException nfe) {
            Messages.postError("Invalid print geometry");
        }
        String title = m_lcPlot.getTitle();
        if (title == null || title.trim().length() == 0) {
            title = "Spectrum";
        }
        String date = Util.getStandardDateTimeString();
        m_pdaPlot.printPS(path, ptLeft, ptTop, ptWidth, ptHeight,
                     "VnmrJ-Spectrum", date, title);
        if (print) {
            sendToVnmr("lcPrint('" + path + "')");
            //sendToVnmr("shell(lcPrintCommand +' " + path + "')");
        }
        Messages.postInfo("EPS plot file in \"" + path + "\"");
    }

    private void printHtml(StringTokenizer tok) {
        // Usage: lccmd('printHtml', 'filepath', 'printerName')
        String path = "/tmp/LcHtmlTest.html";
        String printer = "default";
        if (tok.hasMoreTokens()) {
            path = tok.nextToken();
        }
        if (tok.hasMoreTokens()) {
            printer = tok.nextToken();
        }
        JTextVista.printHtml(path, printer);
    }

    private void analyzeLc(StringTokenizer tok) {
        String filepath = "/tmp/LcDataTest";
        String args = "";
        if (tok.hasMoreTokens()) {
            filepath = tok.nextToken();
        }
        if (tok.hasMoreTokens()) {
            args = tok.nextToken("").trim(); // The rest of the line
        }
        LcAnalysis.analyzeData(peakLists, filepath, args);
        peakAnalysis = new TreeSet<LcPeak>();
        for (int i = 0; i < MAX_TRACES; i++) {
            if (peakLists[i] != null) {
                boolean addAll = peakAnalysis.addAll(peakLists[i]);
            }
        }
        m_lcPlot.repaint();
        if (lcRun != null) {
            lcRun.saveLog();
        }
 
        // Write results in a text file; same directory as data
        int pathend = filepath.lastIndexOf("/");
        if (pathend < 0) {
            pathend = 0;        // No path prefixed to name
        }
        int extstart = filepath.indexOf('.', pathend);
        String outFilepath;
        if (extstart >= 0) {
            outFilepath = filepath.substring(0, extstart);
        } else {
            outFilepath = filepath;
        }
        outFilepath += ".analysis.txt";

        // Open output file
        BufferedWriter fw = null;
        try {
            fw = new BufferedWriter(new FileWriter(outFilepath));
        } catch (IOException ioe) {
            Messages.postError("Cannot open LC Data output file: "
                               + outFilepath);
            return;
        }
        PrintWriter pw = new PrintWriter(fw);

        SortedSet<LcPeak> peakList;
        if (m_peakSortOrder.equals("time chan")) {
            // Print out peaks in "natural" order.
            peakList = peakAnalysis;
        } else {
            Comparator<LcPeak> peakComparator
                    = new PeakComparator(m_peakSortOrder);
            peakList = new TreeSet<LcPeak>(peakComparator);
            peakList.addAll(peakAnalysis);
        }
        Iterator iter = peakList.iterator();
        LcPeak peak;
        //pw.println("Threshold: " + threshold + "   Peak Width: " + peakWidth);
        pw.println("\n  ID     Time     Width    Height      Area\n");
        while(iter.hasNext()) {
            peak = (LcPeak)iter.next();
            String id = Fmt.d(3, peak.id + 1) + (char)('a' + peak.channel);
            // TODO: Format analysis output neatly (or HTML)
            pw.println(peak.getPeakIdString()
                       + " " + Fmt.f(9, 5, peak.time, false)
                       + " " + Fmt.f(9, 5, peak.fwhm, false)
                       + " " + Fmt.f(9, 5, peak.height, false)
                       + " " + Fmt.f(9, 5, peak.area, false));
        }
        pw.close();
        sendToVnmr("lcAnalysisFile='" + outFilepath + "'"
                   + " vnmrjcmd('pnew', 'lcAnalysisFile')");
    }

    private void sortAnalysis(StringTokenizer tok) {
        m_peakSortOrder = "chan time";
    }

    public void configGraphPanel(LcRun run) {
        Messages.postDebug("lcStartup", "LcControl.configGraphPanel()");
        boolean isPdaData;
        boolean isMsData;
        if (run == null || !(run instanceof LcFileRun)) {
            isPdaData = m_uvDetector != null && m_uvDetector.isPda();
            isMsData = msCorbaClient != null;
        } else {
            // Handle display of retrieved data (LcFileRun)
            LcFileRun fRun = (LcFileRun)run;
            isPdaData = fRun.isPdaData();
            isMsData = fRun.isMsData();
        }
        m_graphPanel.configure(isPdaData, isMsData);
        sendToVnmr("lcIsPdaData=" + (isPdaData ? "1" : "-1"));
        sendToVnmr("msIsMsData=" + (isMsData ? "1" : "-1"));
    }

    private void showGraphPanel(boolean isVisible) {
        m_lcGraphPanel.setVisible(isVisible);
        if (m_vmsPlot != null) {
            m_vmsPlot.setActive(isVisible);
        }
    }

    private void showMsPanel(StringTokenizer tok) {
        // 'showMsGraph' time_min "filename" "filename2"
        // (filename2 is a file to try if the first doesn't work)
        //if (msCorbaClient == null) {
        //    return;
        //}

        String sMinutes = null;
        double dMinutes = -1;
        String sFile = null;
        String sFile2 = null;

        showGraphPanel(true);

        if (tok.hasMoreTokens()) {
            sMinutes = tok.nextToken();
            try {
                dMinutes = Double.parseDouble(sMinutes);
            } catch (NumberFormatException nfe) {
                Messages.postError("LcControl.showMsPanel: bad time: "
                                   + sMinutes);
            }
        }
        if (tok.hasMoreTokens()) {
            // Allow quotes in remainder of string, for spaces in filename
            QuotedStringTokenizer qtoker;
            qtoker = new QuotedStringTokenizer(tok.nextToken(""));
            sFile = qtoker.nextToken();
            if (qtoker.hasMoreTokens()) {
                sFile2 = qtoker.nextToken();
            }
        }
        int time_ms = (int)(dMinutes * 60000);
        int tCorrected = time_ms;
        if (lcRun != null) {
            tCorrected = lcRun.correctMsRunTime(time_ms);
        }
        if (msCorbaClient == null) {
            return;
        }
        m_graphPanel.showMs(msCorbaClient, tCorrected, sFile, sFile2, time_ms);
    }

    private void showPdaSpectrum(StringTokenizer tok) {
        String sMinutes = null;
        double time_min = -1;
        String directory = null;

        showGraphPanel(true);

        if (lcRun != null && lcRun.isActive()) {
            return;             // Don't do it during a run
        }

        if (tok.hasMoreTokens()) {
            sMinutes = tok.nextToken();
            try {
                time_min = Double.parseDouble(sMinutes);
            } catch (NumberFormatException nfe) {
                Messages.postError("LcControl.showPdaSpectrum: bad time: "
                                   + sMinutes);
            }
        }
        if (tok.hasMoreTokens()) {
            // Allow quotes in remainder of string, for spaces in filename
            QuotedStringTokenizer qtoker;
            qtoker = new QuotedStringTokenizer(tok.nextToken(""));
            directory = qtoker.nextToken();
        }
        if (directory == null || directory.trim().length() == 0) {
            return;
        }
        File fDir = new File(directory);
        if (!fDir.isDirectory()) {
            directory = fDir.getParentFile().toString();
        }
        if (lcRun != null) {
            m_vpdaImage.setOffset((float)lcRun.getUvOffset_ms() / 60000f);
        }
        m_vpdaImage.setDirectory(directory);
        m_graphPanel.showPda(time_min, m_vpdaImage);
    }

    /**
     * Takes a command of the form
     * <pre>
     * name value
     * </pre>
     * or
     * <pre>
     * name value time
     * </pre>
     * The first sets the method variable "name" to a fixed value.
     * If it is currently in the method table it is removed.
     * The second puts the variable into the method table with whatever
     * its fixed value was at time 0, and with the value specified at
     * the specified time.  A row will be added to the table only if
     * a row for the specified time does not currently exist.
     */
    private void setMethodVariable(StringTokenizer tok) {
        if (!tok.hasMoreTokens()) {
            return;
        }
        String name = tok.nextToken();
        if (!tok.hasMoreTokens()) {
            return;
        }
        String value = tok.nextToken();
        if (!tok.hasMoreTokens()) {
            if (m_currentMethod != null) {
                m_currentMethod.setValue(name, value);
            }
        } else {
            String time = tok.nextToken();
            if (m_currentMethod != null) {
                m_currentMethod.setValue(name, value, time);
            }
        }
    }

    private void loadMethod(StringTokenizer toker) {
        Messages.postDebug("lccmdLoadMethod", "LcControl.loadMethod");
        String methodName = null;
        String dirPath = null;
        try {
            toker = new QuotedStringTokenizer(toker.nextToken(""));
            methodName = toker.nextToken();
            Messages.postDebug("lccmdLoadMethod", "LcControl.loadMethod: "
                               + "methodName=\"" + methodName + "\"");
        } catch (NoSuchElementException nsee) {
            // Do nothing if no filename given
            Messages.postError("loadMethod called with no filename");
            return;
        }
        if (toker.hasMoreTokens()) {
            toker = new QuotedStringTokenizer(toker.nextToken(""));
            dirPath = FileUtil.openPath(toker.nextToken());
        }
        if ("FILE_BROWSER".equals(methodName)) {
            // Pop up a FileChooser to select the real "methodName"
            methodName = LcMethodEditor
                    .chooseMethodToOpen("Choose LC Method to Load", dirPath);
        }
        if (methodName != null) {
            m_currentMethod = new LcCurrentMethod(m_vif, config, methodName);
            if (m_currentMethod.getName() == null) {
                // Failed to load
                Messages.postDebug("LcControl.loadMethod: "
                                   + "LcCurrentMethod failed to initialize");
                m_currentMethod = null;
            } else {
                Messages.postDebug("lccmdLoadMethod", "LcControl.loadMethod: "
                                   + "LcCurrentMethod initialized");
                m_currentMethod.sendTraceStringsToVnmr();
            }
        }
    }

    private void collectLoop(StringTokenizer tok) {
        if (!isActive(lcRun)) {
            return;
        }
        if (tok.hasMoreTokens()) {
            String token = tok.nextToken();
            if (token.equals("event")) {
                lcRun.collectLoopEvent();
            } else {
                try {
                    double time = Double.parseDouble(token);
                    lcRun.collectLoop(time);
                } catch (NumberFormatException nfe) {}
            }
        } else {
            lcRun.collectLoop();
        }
    }

    private void gotoLoop(StringTokenizer tok) {
        String sLoop = null;
        String cmd = null;
        if (tok.hasMoreTokens()) {
            sLoop = tok.nextToken();
        }
        if (tok.hasMoreTokens()) {
            cmd = tok.nextToken(""); // Rest of string is command
        }
        if (sLoop != null) {
            try {
                int iLoop = Integer.parseInt(sLoop);
                slimIO.gotoLoop(iLoop, cmd);
            } catch (NumberFormatException nfe) {}
        }
    }

    private void printSchedule(StringTokenizer tok) {
        /*
        int nrows = schedule.getRowCount();
        String name = "lcIndex";  // Because it's always available
        if (tok.hasMoreTokens()) {
            name = tok.nextToken();
        }
        if (m_currentMethod == null) {
            return;
        }
        for (int i=0; i<nrows; i++) {
            double t = schedule.getTime(i);
            System.out.println("Row " + (i + 1)
                               + " time=" + t
                               + " " + name + "="
                               + m_currentMethod.getValue(name, t));
        }
        */
    }

    /**
     * Start up the Method Editor on a given method file,
     * or, if none specified, the file for the current method,
     * or, if name is "FILE_BROWSER", pop up a file browser to select
     * the method.
     */
    private void editMethod(StringTokenizer toker) {
        String methodName = null;
        String dirPath = null;
        if (m_currentMethod != null) {
            // Default method name (but re-read current method from file)
            methodName = m_currentMethod.getName();
        }
        if (toker.hasMoreTokens()) {
            toker = new QuotedStringTokenizer(toker.nextToken(""));
            methodName = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            dirPath = toker.nextToken();
        }
        if ("FILE_BROWSER".equals(methodName)) {
            methodName = null;
        } else if ("METHOD_FROM_RUN".equals(methodName)) {
            if (dirPath == null) {
                if (lcRun != null) {
                    dirPath = lcRun.getDirpath();
                } else {
                    methodName = LcMethodEditor
                            .chooseMethodToOpen("Choose LC Method to Edit");
                    dirPath = null;
                }
            }
            if (dirPath != null) {
                // Find the parameter file within the dirPath directory
                boolean foundFile = false;
                File file = null;
                String[] files = new File(dirPath).list();
                if (files == null) {
                    Messages.postError("Invalid LC Run directory: " + dirPath);
                    return;
                } else {
                    for (String fname : files) {
                        if (fname.length() > 4) {
                            String suffix = fname.substring(fname.length() - 4);
                            if (".lcm".equalsIgnoreCase(suffix)) {
                                if (foundFile) {
                                    // Found more than one
                                    methodName
                                            = LcMethodEditor.chooseMethodToOpen
                                            ("Choose LC Method to Edit",
                                             dirPath);
                                    if (methodName == null) {
                                        return;
                                    }
                                    break; // Don't bring up any more choosers
                                } else {
                                    methodName = new File(dirPath, fname)
                                            .getPath();
                                    foundFile = true;
                                }
                            }
                        }
                    }
                }
                if (!foundFile) {
                    Messages.postError("No .lcm file found in " + dirPath);
                    return;
                }
            }
        }

        if ("LOADED_METHOD".equals(methodName)) {
            editCurrentMethod();
        } else {
            editMethod(methodName, dirPath);
        }
    }

    /**
     * Start up the Method Editor on the current, loaded method,
     */
    public boolean editCurrentMethod() {
        if (m_currentMethod == null) {
            Messages.postError("No LC method is currently loaded");
            return false;
        } else {
            editMethod(m_currentMethod);
            return true;
        }
    }

    /**
     * Start up the Method Editor on a given method,
     * @param method The method to edit.
     */
    private boolean editMethod(LcMethod method) {
        if (method != null) {
            if (m_methodEditor == null) {
                m_methodEditor = new LcMethodEditor(m_vif, method);
            } else {
                m_methodEditor.setMethod(method);
            }
            m_methodEditor.setState(Frame.NORMAL); // Uniconify if necessary
            m_methodEditor.setVisible(true);       // Bring to top or show
        }
        return method != null;
    }

    /**
     * Start up the Method Editor on a given method file,
     * or, if filepath is null, pop up a file browser to select
     * the method.
     * @param filepath The name of the method, maybe including a path.
     */
    private boolean editMethod(String filepath, String dirpath) {
        if (filepath == null || filepath.trim().length() == 0) {
            // Pop up a FileChooser to select the real "filepath"
            filepath = LcMethodEditor
                    .chooseMethodToOpen("Choose LC Method to Edit", dirpath);
        }
        if (filepath != null) {
            LcMethod method = null;
            //method = new LcCurrentMethod(m_vif, filepath);
            method = new LcMethod(config, filepath);
            if (method.getName() == null) {
                // Failed to get method
                method = null;
                Messages.postError("Cannot read method \"" + filepath + "\"");
                return false;
            }
            editMethod(method);
        }
        return filepath != null;
    }

    private void cmdPurgeNext_Click() {     
        // This removes the next thing that is to happen from the
        // eventQueue.  Don't know if it will be needed - not used now.
        Messages.postError("LcControl.cmdPurgeNext_Click() called."
                           + "  (Could be dangerous?)");
        eventQueue.remove(eventQueue.first());
    }

    private void nmrDone() {
        if (isActive(lcRun)) {
            lcRun.trigFromNmr();
        }
    }

    private void restartRun() {
        if (isActive(lcRun)) {
            lcRun.restartRun();
        } else {
            Messages.postError("Cannot re-start completed run");
        }
    }

    private void resumeChromatogram() {
        if (isActive(lcRun)) {
            lcRun.restartChromatogram();
        } else {
            Messages.postError("Cannot re-start completed run");
        }
    }

    private void restartRunAppend() {
        if (isActive(lcRun)) {
            lcRun.restartRunAppend();
        } else {
            Messages.postError("Cannot re-start completed run");
        }
    }

    private void startAdc(StringTokenizer tok) {
        int period_ms = (int)(m_currentMethod.getAdcPeriod() * 1000);
        slimIO.startAdc(period_ms);
    }
        
    private void slimCmd(StringTokenizer tok) {
        String cmd = "";
        if (tok.hasMoreTokens()) {
            cmd = tok.nextToken("").trim(); // Remainder of line
        }
        slimIO.sendCommand(cmd);
    }

    /**
     * To pulse bits #3 and #0, call "lccmd('slimPulse', 9)".
     */
    private void slimPulse(StringTokenizer tok) {
        int bitmask = 0;
        if (tok.hasMoreTokens()) {
            bitmask = Integer.parseInt(tok.nextToken());
        }
        slimIO.pulseBit(bitmask);
    }

    private void peakAction(StringTokenizer tok) {
        String cmd = null;
        if (tok.hasMoreTokens()) {
            cmd = tok.nextToken("").trim(); // Remainder of line
        }
        config.setPeakAction(cmd);
    }

    private void nmrDoneAction(StringTokenizer tok) {
        String cmd = null;
        if (tok.hasMoreTokens()) {
            cmd = tok.nextToken("").trim(); // Remainder of line
        }
        config.setNmrDoneAction(cmd);
    }

    private void manualFlowControl(StringTokenizer tok) {
        String cmd = "y";
        if (tok.hasMoreTokens()) {
            cmd = tok.nextToken();
        }
        boolean b = cmd.startsWith("y") || cmd.startsWith("t");
        config.setManualFlowControl(b);
    }

    private void msStatus(StringTokenizer tok) {
        if (msCorbaClient == null) {
            return;
        }
        String cmd = "off";
        if (tok.hasMoreTokens()) {
            cmd = tok.nextToken();
        }
        boolean b = cmd.equals("on");
        msCorbaClient.setMsStatusUpdating(b);
    }

    private void cmdStartRun_Click(StringTokenizer tok) {

        if (isActive(lcRun)) {
            Messages.postError("Cannot start new run; a run is already active");
            return;
        }

        if (!DebugOutput.isSetFor("DontCheckDetector")) {
            if (m_uvDetector == null
                || !m_uvDetector.isDownloaded(m_currentMethod))
            {
                Messages.postError("Download detector method before starting run");
                return;
            }
        }

        if (!DebugOutput.isSetFor("DontCheckPump")) {
            if (m_gpib == null || !m_gpib.isPumpDownloaded(m_currentMethod)) {
                Messages.postError("Download pump method before starting run");
                return;
            }

            if (!isPumpReady()) {
                Messages.postError("The pump is not ready for the run to start");
                return;
            }
        }

        // Get the run parameters
        String version = "Unknown Version";
        String revdate = "Unknown Rev Date";
        String filepath = "/tmp/VjLcData";
        String type = "";
        if (tok.hasMoreTokens()) {
            // Grab remainder of string by specifying no delimiters.
            QuotedStringTokenizer qtoker=
                    new QuotedStringTokenizer(tok.nextToken(""));
            if (qtoker.hasMoreTokens()) {
                version = qtoker.nextToken();
            }
            if (qtoker.hasMoreTokens()) {
                revdate = qtoker.nextToken();
            }
            if (qtoker.hasMoreTokens()) {
                filepath = qtoker.nextToken();
            }
            if (qtoker.hasMoreTokens()) {
                type = qtoker.nextToken();
            }
        }
        File cFilepath = new File(filepath);
        String path = cFilepath.getParent();
        String file = cFilepath.getName();

        double endTime = m_currentMethod.getEndTime() * 60.0;
        double dataRate = m_currentMethod.getAdcPeriod();
        if (endTime <= 0) {
            Messages.postError("Cannot start run: Specified run time is 0");
            return;
        }
        if (dataRate <= 0) {
            Messages.postError("Cannot start run: Specified data rate is 0");
            return;
        }
        /*
        if (schedule == null || schedule.getTime(0) < 0) {
            Messages.postError("Cannot start run; No method table");
            return;
        }
        */

        Messages.postInfo("Starting LC run: type=" + type
                          + ", Data dir=" + path);

        config.setPeakAction(null);
        config.setManualFlowControl(false);
        eventQueue.clear();
        m_currentMethod.preloadEventQueue(eventQueue);

        try {
            // Call the relevant experiment.  Note that loopNumber is acting
            // as a flag, based on the above check.  It is ok for loopNumber
            // to be 0 for the stop-flow base.
            if ( config.isAcHere() ) {
                slimIO.getLoopPosition();
            }
            /*TODO: reset AC if loopNumber = 0
              if ( loopNumber == 0 ) {
              }*/
            runLc(version, revdate, path, file, type);

        } catch (RuntimeException re) {
            Messages.postError("Runtime exception:");
            Messages.writeStackTrace(re);
        }
    }


    private void cmdStopRun_Click() {
        if (WatchInProgress) {
            WatchInProgress = false;
        }
        if (lcRun != null) {
            lcRun.stop();
            lcRun.runActive = false;
        } else {
            Messages.postWarning("No LC run to stop");
            //sendToVnmr("EndRun('VJ')");
        }
    }

    private void stopChromatogram() {
        if (isActive(lcRun)) {
            lcRun.stopChromatogram();
            lcRun.runActive = false;
        }
    }

    private void cmdPauseRun_Click() {
        if (isActive(lcRun)) {
            lcRun.pauseRun();
        }
    }

    private void pauseChromatogram() {
        if (isActive(lcRun)) {
            lcRun.pauseChromatogram();
        }
    }

    private void clearPlots() {
        if (m_lcPlot != null) {
            m_lcPlot.initialize(null, null);
        }
        if (m_pdaPlot != null) {
            m_pdaPlot.clear(false);
            m_pdaPlot.setTitle(null);
            m_pdaPlot.repaint();
        }
    }

    /**
     * Start an injection on the autosampler and then send the given
     * command to Vnmr when the injection happens.
     * @param toker Tokens representing the command(s) to send to Vnmr.
     */
    private void inject(StringTokenizer toker) {
        if (startInjection()) {
            m_postInjectCommand = null;
            if (toker.hasMoreTokens()) {
                m_postInjectCommand = toker.nextToken("").trim();
                Messages.postDebug("lcInject",
                                   "LcControl.inject: m_postInjectCommand="
                                   + m_postInjectCommand);
            }
        }
    }

    /**
     * Get some sample into the injection loop and switch the injection
     * valve.
     */
    public boolean startInjection() {
        if (m_autosampler == null || !m_autosampler.isConnected()) {
            Messages.postError("Auto-sampler not connected; "
                               + "cannot do injection");
            return false;
        } else {
            m_autosampler.startMethod();
            return true;
        }
    }

    public void setInjectionCommand(String cmd) {
        m_postInjectCommand = cmd;
    }

    public void handleInjection() {
        Messages.postDebug("lcInject", "LcControl.handleInjection()");
        if (m_postInjectCommand != null
            && m_postInjectCommand.trim().length() > 0)
        {
            sendToVnmr(m_postInjectCommand);
        } else if (lcRun != null) {
            lcRun.handleInjection(null);
        }
        m_postInjectCommand = null;
    }

    private boolean checkSlimCommand(String cmd, StringTokenizer toker) {
        if (slimIO.isPortOpen()) {
            // Just have the caller execute the command
            return true;
        } else {
            // Need to either open the port or give up
            if (slimIO.isPortCheckBad()) {
                // Last check failed to open port - give up
                return false;
            } else {
                // We have to have Vnmr figure out which port to open
                String vcmd = "lccmd('checkSlimPort')";
                Messages.postDebug("LcSlim", "vcmd=" + vcmd);
                sendToVnmr(vcmd);

                // Then have Vnmr redo this command
                String args = "";
                if (toker.hasMoreTokens()) {
                    args = toker.nextToken("").trim(); // Get rest of string
                }
                vcmd = cmd + " " + args;
                // Insert "\" in front of any "'"
                StringBuffer sb = new StringBuffer(vcmd);
                for (int i = 0; i < sb.length(); i++) {
                    if (sb.charAt(i) == '\'') {
                        if (i == 0 || sb.charAt(i - 1) != '\\')
                            sb.insert(i++, '\\');
                    }
                }

                vcmd = "lccmd('" + sb + "')";
                Messages.postDebug("LcSlim", "vcmd=" + vcmd);
                sendToVnmr(vcmd);
            }
            return false;
        }
    }        

    public void cmdStep_Click() {
            slimIO.loopIncr();
    }

    private void optToCollectorWaste_Click() {
        slimIO.leftCollWastePulse();
    }

    private void optToColumn_Click() {
        slimIO.leftColumnPulse();
    }

    private void optToNmr_Click() {
        slimIO.rightNmrPulse();
    }

    private void optToWaste_Click() {
        slimIO.rightWastePulse();
    }

    private void sfStop() {
        slimIO.SFStopPulse();
    }

    private void sfFlow() {
        slimIO.SFFlowPulse();
    }

    private void setNmrBypass(StringTokenizer toker) {
        String waste = "y";
        if (toker.hasMoreTokens()) {
            waste = toker.nextToken();
        }
        if (waste.equals("y")) {
            slimIO.rightWastePulse();
        } else {
            slimIO.rightNmrPulse();
        }
    }

    protected void startFlow() {
        optToNmr_Click();
        optToColumn_Click();
        //startPump();
    }

    protected void stopFlow() {
        optToWaste_Click();
        optToCollectorWaste_Click();
        //slimIO.pumpStopPulse();
        pumpStopRun();
        slimIO.triggerToNmr();
    }

    /**
     * Usage: lccmd('writeMethodToFile', 'filepath')
     */
    protected void writeMethodToFile(StringTokenizer toker) {
        String filepath = "/tmp/testMethod";
        if (toker.hasMoreTokens()) {
            filepath = toker.nextToken();
        }
        m_currentMethod.writeMethodToFile(filepath);
    }

    /**
     * Usage: lccmd('readMethodFromFile', 'filepath')
     */
    protected void readMethodFromFile(StringTokenizer toker) {
        String filepath = "/tmp/testMethod";
        if (toker.hasMoreTokens()) {
            filepath = toker.nextToken();
        }
        m_currentMethod.readMethodFromFile(filepath);
    }

    /**
     * Usage: lccmd('printMethod'<, 'HTMLfilepath'<, 'title'>>)
     * To send to printer also:
     *   lccmd('printMethod", '/tmp/method.html')
     *   lccmd('printHtml', '/tmp/method.html')
     */
    protected void printMethod(StringTokenizer toker) {
        String filepath = "/tmp/method.html";
        String title = "";
        if (toker.hasMoreTokens()) {
            filepath = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            title = toker.nextToken();
        }
        m_currentMethod.printMethod(filepath, title);
    }

    public void sendToVnmr(String str) {
        if (str != null) {
            m_vif.sendToVnmr(str);
        }
    }

    synchronized public void setWatchInProgress(boolean b) {
        WatchInProgress = b;
    }


    /**
     * Do a "run" that consists of reading data from a file
     */
    private void runFromFile(String filepath) {
        m_baseFilePath = filepath;
        peakAnalysis = null;    // Erase any previous analysis
        lcRun = new LcFileRun(this,
                              config,
                              eventQueue,
                              slimIO,
                              m_lcPlot,
                              filepath);
    }

    /**
     * Start an LC run.
     */
    private void runLc(String version, String revdate,
                       String path, String file, String type) {
        m_baseFilePath = path + File.separator + file;
        peakAnalysis = null;    // Erase any previous analysis
        lcRun = new LcRun(this,
                          config,
                          eventQueue,
                          m_currentMethod,
                          //schedule,
                          slimIO,
                          m_lcPlot,
                          version,
                          revdate,
                          path,
                          file,
                          type);
    }

    public boolean setUvDataListener(LcDataListener listener) {
        if (m_uvDetector != null) {
            return m_uvDetector.setDataListener(listener);
        }
        return false;
    }

    public boolean setUvChromatogramChannels(float[] lambdas, int[] traces,
                                        LcRun chromatogramManager) {
        if (m_uvDetector != null) {
            return m_uvDetector.setChromatogramChannels(lambdas, traces,
                                                        chromatogramManager);
        }
        return false;
    }

    public int getUvDataInterval() {
        if (m_uvDetector != null) {
            return m_uvDetector.getDataInterval();
        }
        return 0;
    }

    public LcRun getLcRun() {
        return lcRun;
    }

    public LcCurrentMethod getCurrentMethod() {
        return m_currentMethod;
    }


    /**
     * This class waits for the pump to be in a given state, and then
     * sends its command string to VnmrBG.
     */
    class PumpListener implements LcStatusListener {
        private Double[] m_percents; // desired concentrations
        private Integer m_flow; // Desired flow (uL / min)
        private String m_command; // Vnmr command to execute when ready

        /**
         * Construct a PumpListener that will wait for the pump to be
         * "Ready" or "Running" and with the given conditions.
         * Then, the specified command will be sent to VnmrBG.
         * @param flow The desired flow rate (L/minute).
         * @param percents The desired solvent percentages.
         * @param command The command to be executed.
         */
        public PumpListener(double flow, double[] percents, String command) {
            m_percents = new Double[3];
            for (int i = 0; i < 3; i++) {
                if (i < percents.length) {
                    m_percents[i] = percents[i];
                } else {
                    m_percents[i] = 0.0;
                }
            }
            m_flow = (int)Math.round(1000 * flow); // uL / min
            m_command = command;
        }

        public void detectorFileStatusChanged(int status, String message){}

        public void pumpFileStatusChanged(int status, String message){}

        /**
         * Called when the pump's state changes.
         * Executes the "Command" of this PumpListener if the pump is ready.
         * @param state The current pump state.
         */
        public void pumpStateChanged(Map<String, Object> state){
            Object oState = state.get("state");
            if (m_percents[0].equals(state.get("percentA"))
                && m_percents[1].equals(state.get("percentB"))
                && m_percents[2].equals(state.get("percentC"))
                && m_flow.equals(state.get("flow"))
                && (oState.equals(PUMP_READY) || oState.equals(PUMP_RUNNING)))
            {
                sendToVnmr(m_command);
                m_gpib.removePumpStatusListener(this);
            }
        }
    }

    /**
     * Select the UV absorption from among the ADC channel values and
     * correct it for offset.
     * NB: This only works if the UV is attached to some trace.
     */
    public double getUvAbsorption() {
        int chan = config.getUvChannel();
        if (chan >= 0
            && m_channelValues != null
            && chan < m_channelValues.length)
        {
            return m_channelValues[chan];
        } else {
            return 0;
        }
    }

    /**
     * Take a array of ADC channel values, and correct the UV channel
     * for offset.  A copy of the SLIM ADC values is kept here in
     * order to be able to set the correction (in setUvOffset()).
     * Set the (corrected) absorptions in the SLIM UV ADC channel.
     * NB: This only works if the UV is attached to some trace.
     */
    public double[] correctChannelValues(double[] channelValues) {
        int chan = config.getUvChannel();
        if (chan >= 0 && chan < channelValues.length) {
            channelValues[chan] -= m_uvOffset;
        }
        m_channelValues = channelValues;
        return channelValues;
    }

    /**
     * Called from LcCorbaClient when detector is being zeroed.
     * Set offset so that detected UV absorption is 0.
     */
    public void setUvOffset() {
        double val = getUvAbsorption();
        m_uvOffset += val;
    }


    /**
     * Sort peaks in other than time order.
     */
    class PeakComparator implements Comparator<LcPeak> {

        /**
         * Ignores its "order" argument for now.
         * Sorts by channel, then time.
         */
        public PeakComparator(String order) {
        }

        public int compare(LcPeak pk1, LcPeak pk2) {
            if (pk1.channel > pk2.channel) {
                return 1;
            } else if (pk1.channel < pk2.channel) {
                return -1;
            } else {
                if (pk1.time > pk2.time) {
                    return 1;
                } else if (pk1.time < pk2.time) {
                    return -1;
                } else {
                    return 0;
                }
            }
        }
    }

    public boolean isActive(LcRun run) {
        return run != null && run.isActive();
    }

    /**
     * Receive updates on the list of debug codes. Updates the Vnmr
     * parameter with the value of the debug string.
     * (For the DebugOutput.Listener interface.)
     */
    public void setDebugString(String str) {
        sendToVnmr("vjDebugString='" + str + "'");
    }


    public static class Solvents {

        /**
         * Do not call constructor -- static methods only.
         */
        private Solvents() {
        }

        /**
         * Return the name to be displayed to the user for a given
         * solvent parameter value.
         * @param key The solvent parameter value.
         * @return The solvent name to be displayed to the user.
         */
        public static String getDisplayName(String key) {
            return sm_solventTable.get(key);
        }

        /**
         * Initialize the mapping of solvent parameter value to solvent name.
         * @param filename A file containing the mapping.
         */
        public static void init(String filename) {
            String strPath=FileUtil.openPath(filename);
            if(strPath == null) {
                Messages.postError("LcControl.Solvents.init(): file not found: "
                                   + filename);
            }
            File mapFile = new File(strPath);
            FileReader fr;
            try {
                fr = new FileReader(strPath);
            } catch (java.io.FileNotFoundException e1) {
                Messages.postError("LcControl.Solvents.init(): file not found: "
                                   + strPath);
                return;
            }
            
            // Parse the file and extract key-value pairs.
            // NB: Value comes before key in the file.
            sm_solventTable.clear();
            BufferedReader text = new BufferedReader(fr);
            try {
                String line;
                while ((line = text.readLine()) != null) {
                    if (!line.startsWith("#")) {
                        StringTokenizer toker =
                                new QuotedStringTokenizer(line);
                        if (toker.countTokens() >= 2) {
                            String value = toker.nextToken();
                            String key = toker.nextToken();
                            sm_solventTable.put(key, value);
                        }
                    }
                }
            } catch(java.io.IOException e) {
                Messages.postDebug("LcControl.Solvents.init(): "
                                   + "error reading solvents file: " + strPath);
            }
            try {
                fr.close();
            } catch(IOException e) { }
        }

    }


    /**
     * List of possible LC commands and their Integer codes.
     */
    private static final Object[][] CMD_TABLE = {
        {"openSlimPort",        new Integer(OPEN_SLIM_PORT)},
        {"checkSlimPort",       new Integer(CHECK_SLIM_PORT)},
        {"editMethod",          new Integer(EDIT_METHOD)},
        {"loadMethod",          new Integer(LOAD_METHOD)},
        {"saveCurrentMethod",   new Integer(SAVE_CURRENT_METHOD)},
        {"printSchedule",       new Integer(PRINT_SCHEDULE)},
        {"setMethodVariable",   new Integer(SET_METHOD_VARIABLE)},
        {"repaintLcGraph",      new Integer(REPAINT_LC_GRAPH)},
        {"showLcGraph",         new Integer(SHOW_LC_GRAPH)},
        {"showMsGraph",         new Integer(SHOW_MS_GRAPH)},
        {"updateMsGraph",       new Integer(UPDATE_MS_GRAPH)},
        {"updatePdaGraph",      new Integer(UPDATE_PDA_GRAPH)},
        {"hideLcGraph",         new Integer(HIDE_LC_GRAPH)},
        {"printLcGraph",        new Integer(PRINT_LC_GRAPH)},
        {"printMsGraph",        new Integer(PRINT_MS_GRAPH)},
        {"printPdaGraph",       new Integer(PRINT_PDA_GRAPH)},
        {"showPdaSpectrum",     new Integer(SHOW_PDA_SPECTRUM)},
        {"printHtml",           new Integer(PRINT_HTML)},
        {"analyzeLc",           new Integer(ANALYZE_LC)},
        {"updateMethodPanel",   new Integer(UPDATE_METHOD_PANEL)},
        {"readData",            new Integer(READ_DATA)},
        {"downloadMethod",      new Integer(DOWNLOAD_METHOD)},
        {"checkMethodDownloads", new Integer(CHECK_METHOD_DOWNLOADS)},
        {"test",                new Integer(TEST)},
        {"corbaCmd",            new Integer(CORBA_CMD)},
        {"setThresh",           new Integer(SET_THRESH)},
        {"setPeakDet",          new Integer(SET_PEAK_DET)},
        {"setPeakDetType",      new Integer(SET_PEAK_DET_TYPE)},
        {"setTransferTimes",    new Integer(SET_TRANSFER_TIMES)},
        {"setReferenceTime",    new Integer(SET_REFERENCE_TIME)},
        {"testPeakDetect",      new Integer(TEST_PEAK_DETECT)},
        {"setDelay",            new Integer(SET_DELAY)},
        {"gpib",                new Integer(GPIB_MSG)},
        {"holdNow",             new Integer(HOLD_NOW)},
        {"translateData",       new Integer(TRANSLATE_DATA)},
        {"pdastart",            new Integer(PDA_START)},
        {"pdastop",             new Integer(PDA_STOP)},
        {"pdaautozero",         new Integer(PDA_BASELINE)},
        {"pdareset",            new Integer(PDA_RESET)},
        {"pdalampOn",           new Integer(PDA_LAMP_ON)},
        {"pdalampOff",          new Integer(PDA_LAMP_OFF)},
        {"pdadownloadMethod",   new Integer(PDA_LOAD_METHOD)},   
        {"pdaValidate",         new Integer(PDA_VALIDATE)},
        {"pdaD2Reset",          new Integer(PDA_D2RESET)},
        {"pdaHgReset",          new Integer(PDA_HGRESET)},
        {"pdaSaveData",         new Integer(PDA_SAVE_DATA)},
        {"pdaGetImage",         new Integer(PDA_IMAGE)},
        {"downloadMsMethod",    new Integer(DOWNLOAD_MS_METHOD)},
        {"setMsFile",           new Integer(SET_MS_FILE)},
        {"makeupPumpOn",        new Integer(MAKEUP_PUMP_ON)},
        {"makeupPumpOff",       new Integer(MAKEUP_PUMP_OFF)},
        {"makeupPumpGetFlow",   new Integer(MAKEUP_PUMP_FLOW)},
        {"makeupPumpGetPress",  new Integer(MAKEUP_PUMP_PRESS)},
        {"waitForPump",         new Integer(WAIT_FOR_PUMP)},
        {"waitForTime",         new Integer(WAIT_FOR_TIME)},
        {"setPauseDuration",    new Integer(SET_PAUSE_DURATION)},
        {"clearEvents",         new Integer(CLEAR_EVENTS)},
        {"setMsAnalogOut",      new Integer(SET_MS_ANALOG_OUT)},
        {"init",                new Integer(INIT)},
        {"step",                new Integer(STEP)},
        {"stopRun",             new Integer(STOP_RUN)},
        {"pauseRun",            new Integer(PAUSE_RUN)},
        {"stopFlowNmr",         new Integer(STOP_FLOW_NMR)},
        {"timeSliceNmr",        new Integer(TIME_SLICE_NMR)},
        {"holdDelayed",         new Integer(HOLD)},
        {"startRun",            new Integer(START_RUN)},
        {"startAdc",            new Integer(START_ADC)},
        {"slimCmd",             new Integer(SLIM_CMD)},
        {"restartRun",          new Integer(RESTART_RUN)},
        {"nmrDone",             new Integer(NMR_DONE)},
        {"restartRunAppend",    new Integer(RESTART_RUN_APPEND)},
        {"toCollector",         new Integer(TO_COLLECTOR)},
        {"toColumn",            new Integer(TO_COLUMN)},
        {"toNmr",               new Integer(TO_NMR)},
        {"toWaste",             new Integer(TO_WASTE)},
        {"setNmrBypass",        new Integer(NMR_BYPASS_VALVE)},
        {"enableTrigger",       new Integer(ENABLE_TRIGGER)},
        {"disableTrigger",      new Integer(DISABLE_TRIGGER)},
        {"enableHeartbeat",     new Integer(ENABLE_HEARTBEAT)},
        {"disableHeartbeat",    new Integer(DISABLE_HEARTBEAT)},
        {"collectLoop",         new Integer(COLLECT_LOOP)},
        {"gotoLoop",            new Integer(GOTO_LOOP)},
        {"getSlimVersion",      new Integer(GET_SLIM_VERSION)},
        {"serialIO",            new Integer(SERIAL_IO)},
        {"getPcFile",           new Integer(GET_PC_FILE)},
        {"msPumpControl",       new Integer(MS_PUMP_CONTROL)},
        {"peakAction",          new Integer(PEAK_ACTION)},
        {"nmrDoneAction",       new Integer(NMR_DONE_ACTION)},
        {"startChromatogram",   new Integer(START_CHROMATOGRAM)},
        {"pauseChromatogram",   new Integer(PAUSE_CHROMATOGRAM)},
        {"resumeChromatogram",  new Integer(RESUME_CHROMATOGRAM)},
        {"stopChromatogram",    new Integer(STOP_CHROMATOGRAM)},
        {"manualFlowControl",   new Integer(MANUAL_FLOW_CONTROL)},
        {"msStatus",            new Integer(MS_STATUS)},
        {"connectUv",           new Integer(CONNECT_UV)},
        {"connectPump",         new Integer(CONNECT_PUMP)},
        {"connectMs",           new Integer(CONNECT_MS)},
        {"setFlowCell",         new Integer(SET_FLOW_CELL)},
        {"adcToUvStatus",       new Integer(ADC_TO_UV_STATUS)},
        {"connectAutosamp",     new Integer(CONNECT_AS)},
        {"autosampSetMacro",    new Integer(AS_SETMACRO)},
        {"autosampCmd",         new Integer(AS_CMD)},
        {"slimPulse",           new Integer(SLIM_PULSE)},
        {"inject",              new Integer(INJECT)},
        {"clearPlots",          new Integer(CLEAR_PLOTS)},
        {"configGraphPanel",    new Integer(CONFIG_GRAPH_PANEL)},
        {"writeMethodToFile",   new Integer(WRITE_METHOD_TO_FILE)},
        {"readMethodFromFile",  new Integer(READ_METHOD_FROM_FILE)},
        {"printMethod",         new Integer(PRINT_METHOD)},
        {"acToNmr",             new Integer(AC_TO_NMR)},
        {"acToWaste",           new Integer(AC_TO_WASTE)},
        {"sfStop",              new Integer(SF_STOP)},
        {"sfFlow",              new Integer(SF_FLOW)},
    };

}
