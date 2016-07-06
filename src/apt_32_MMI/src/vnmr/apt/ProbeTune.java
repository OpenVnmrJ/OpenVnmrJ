/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

package vnmr.apt;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;
import java.lang.management.ManagementFactory;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.TreeMap;
import java.util.concurrent.Semaphore;

import javax.swing.JOptionPane;

import vnmr.apt.ChannelInfo.TunePosition;
import vnmr.util.Fmt;
import vnmr.probeid.ProbeIdClient;

public class ProbeTune implements Executer, AptDefs {

    /*
     * Version string of the software. Update this string for every release!
     * Must be of the form "x.y...", where x, y, ... are the major,
     * minor, and even more minor version numbers.
     */
    static private final String VERSION = "1.7";

    static private final int STEP_SLACK = 2;
    static private final double STEP_RATIO = 5;
    static private final int MIN_STEP_REVERSES = 2;
    static private final int MAX_STEP_REVERSES = 5;

    /** use probe-resident tuning files **/
    private static boolean m_useProbeId = false;

    /** probe client interface -
     * declared static to provide external access without a handle on ProbeTune
     */
    private static AptProbeIdClient m_probeId = null;

    private static boolean m_useLock = true; // lock access to resources shared by multiple threads

    /**
     * The maximum number of independent tune motors per channel.
     * (One tune and one match.)
     */
    static private final int MAX_TUNE_MOTORS = 2; // Motors per channel

    private int m_nTuneMotors = MAX_TUNE_MOTORS;
    protected List<String> m_commandList = new ArrayList<String>();
    private CommandListenersThread m_listeners = null;
    private Map<Integer,ChannelInfo> m_chanInfos = null;
    private List<ChannelInfo> m_chanList = null;
    private MotorControl motorControl = null;
    private SweepControl sweepControl = null;
    private boolean m_rawDataMode = false;
    private boolean m_savedRawDataMode = m_rawDataMode;

    private int[] m_nReverses = new int[MAX_TUNE_MOTORS];

    private static boolean m_cancel = false;

    /** The frequency we currently want to tune to. */
    private static double m_targetFreq = Double.NaN;

    /**
     * The current tune criterion.
     */
    private TuneCriterion m_criterion = TuneCriterion.getCriterion("Fine");

    private String m_usrProbeName = "";
    private String m_probeName = "";
    private int m_channelNumber = 0;
    private static String m_modeName = null;
    private static double m_lowerCutoffFreq = Double.NaN;
    private static double m_upperCutoffFreq = Double.NaN;
    private static String m_sweepType = "mt";
    private static String m_sweepIpAddr = "agilentNA";
    private static String m_motorHostname = "V-Protune";
    private static String m_motorIpAddr = "172.16.0.245"; // Use if hostname bad
    private static String m_commands = null;
    private static double[] m_initialSweep = null;
    //private static double[] m_calSweep = new double[4];
    private static long m_1stSweepTimeout = 30000;
    private static boolean m_guiFlag = true;
    private static boolean m_autoFlag = false;
    private static String m_vnmrSystemDir = "/vnmr";
    private static String m_vnmrUserDir
            = System.getProperty("user.home") + "/vnmrsys";
    private static String m_sysTuneDir = null;
    private static boolean m_firstTime= true;
    private static boolean m_dispInfo= false;
    private static String m_cmdHostAndPort = "";
    //private static TuneCriterion[] m_tuneCriteria;

    /** What fraction of the way to move to the target in one step. */
    static private double m_stepDamping = 0.33;

    /** The status message for the latest tuning attempt. */
    private TuneResult m_tuneStatus = new TuneResult();

    /** List of all the tune results obtained in this run. */
    private ArrayList<TuneResult> m_tuneResults = new ArrayList<TuneResult>();

    private ReflectionData m_reflectionData  = null;
    private double m_padAdj = 1.0;

    private static String m_tuneUpdate = UPDATING_ON;
    private static boolean m_useRefPositions = true;
    private static boolean m_favorCurrentChannel = false;

    private static int m_maxStepSize = MAX_STEP_SIZE;

    private static boolean m_tuningAttempted = false;

    /**
     * m_debugLevel
     * Debug level setting. Current categories are:
     * 0 - No message
     * 1 - System hardware failure messages (unable to continue)
     * 2 - (1) + Tune related failure messages (unable to continue)
     * 3 - All error messages
     * 4 - All status and information messages
     * 5 - Special debug messages
     */
    private static int m_debugLevel = 0;        //Default no debug messages

    private static int m_lockPort = 4593;

    private static boolean m_probeNamePopup = false;

    private static boolean m_isVnmrConnection
        = TuneUtilities.getBooleanProperty("INFO2VJ", true);
    	//System.getProperty("INFO2VJ", Boolean.toString(true)).equals(Boolean.toString(true));

    /** The date this software version was compiled. */
    private static String m_date;

    //private static int m_matchCriterionIndex = -1;

    private boolean m_sysInitialized = false;

    public int m_nExecThreadsRunning = 0;

    /** Holds 5 calibration scans in order: open, load, short, probe, sigma. */
    private ReflectionData[] m_calData = new ReflectionData[5];

    private ExecTuneThread m_execThread = null;

    private String m_queuedCommands = "";

    private boolean m_isTorqueTool = false;

    /** Exit after executing command-line commands. */
    private static boolean m_autoExit = true;


    public static void main(String[] args) {
        String probeName = setup(args);
        if (m_probeNamePopup) {
            ProbeNamePopup.startProbeTune();
        } else {
            try {
                new ProbeTune(probeName, probeName);
            } catch (InterruptedException e) {
                return; // User aborted before initialization completed
            }
        }
    }

    /**
     * Parse arguments and configure static members.  It is a public member
     * in order to facilitate unit test setup.
     * @param args The command line arguments
     * @return probe name
     */
    public static String setup(String[] args) {
        Class<ProbeTune> myclass = ProbeTune.class;
        // NB: scons puts the compilation date in the Implementation Version
        String date = myclass.getPackage().getImplementationVersion();
        m_date = (date != null) ? date : "";

        //DebugOutput.addCategory("Initialization"); // Default messages
        DebugOutput.addCategory("TuningSummary"); // Default messages
        DebugOutput.addCategory("TuneAlgorithm"); // Default messages
        // NB: apt.debugCategories may be augmented later,
        // but enable debug output we know about as soon as possible.
        DebugOutput.addCategories(System.getProperty("apt.debugCategories"));

        Messages.postDebug("TuningSummary", "Starting ProTune");

        // This is for windows and non-windows system with new protune
        // script with the "sysdir" and "userdir" properties defined.
        String sysdir = System.getProperty("sysdir");
        if (sysdir != null) {
            m_vnmrSystemDir = sysdir;
        }
        String userdir = System.getProperty("userdir");
        if (userdir != null) {
            m_vnmrUserDir = userdir;
        }

        if (DebugOutput.isSetFor("Initialization")) {
            StringBuffer msg = new StringBuffer("Called with args:");
            for (String arg : args) {
                msg.append(" \"" + arg + "\"");
            }
            Messages.postDebug(msg.toString());
        }
        String probeName = "NoProbeName";
        int len = args.length;
        for (int i = 0; i < len; i++) {
            Messages.postDebug("Initialization", "Got command line flag: "
                               + args[i]);
            if (args[i].equalsIgnoreCase("-motorIP") && i + 1 < len) {
                m_motorIpAddr = m_motorHostname = args[++i];
                System.setProperty("apt.motorHostname", m_motorHostname);
            } else if (args[i].equalsIgnoreCase("-sweep") && i + 1 < len) {
                m_sweepType = args[++i];
                System.setProperty("apt.sweepType", m_sweepType);
            } else if (args[i].equalsIgnoreCase("-lowerCutoff") && i + 1 < len) {
                String lowerCutoff = args[++i];
                try {
                    m_lowerCutoffFreq = Double.parseDouble(lowerCutoff);
                    System.setProperty("apt.lowerCutoff", lowerCutoff);
                } catch (NumberFormatException nfe) {
                }
            } else if (args[i].equalsIgnoreCase("-upperCutoff") && i + 1 < len) {
                String upperCutoff = args[++i];
                try {
                    m_upperCutoffFreq = Double.parseDouble(upperCutoff);
                    System.setProperty("apt.upperCutoff", upperCutoff);
                } catch (NumberFormatException nfe) {
                }
            } else if (args[i].equalsIgnoreCase("-cmdSocket") && i + 1 < len) {
                m_cmdHostAndPort = args[++i];
                //m_autoFlag = true;      // No interactive messages
            } else if (args[i].equalsIgnoreCase("-sweepIP") && i + 1 < len) {
                m_sweepIpAddr = args[++i];
                System.setProperty("apt.sweepIpAddr", m_sweepIpAddr);
            } else if (args[i].equalsIgnoreCase("-vnmrsystem") && i + 1 < len) {
                m_vnmrSystemDir = args[++i];
            } else if (args[i].equalsIgnoreCase("-vnmruser") && i + 1 < len) {
                m_vnmrUserDir = args[++i];
            } else if (args[i].equalsIgnoreCase("-systunedir") && i + 1 < len) {
                m_sysTuneDir = args[++i];
            } else if (args[i].equalsIgnoreCase("-probe") && i + 1 < len) {
                probeName = args[++i];
                System.setProperty("apt.probeName", probeName);
                Messages.postDebug("TuningSummary",
                                   "ProbeTune: probeName=" + probeName);
            } else if (args[i].equalsIgnoreCase("-match") && i + 1 < len) {
                double match_db = Double.parseDouble(args[++i]);
                double match_pow = Math.pow(10, -Math.abs(match_db / 10));
                System.setProperty("apt.targetMatch", Fmt.f(6, match_pow));
            } else if (args[i].equalsIgnoreCase("-sweepTimeout") && i + 1 < len)
            {
                String firstSweepTimeout = args[++i];
                try {
                    m_1stSweepTimeout = Long.parseLong(firstSweepTimeout);
                    System.setProperty("apt.firstSweepTimeout",
                                       firstSweepTimeout);
                } catch (NumberFormatException nfe) {
                }
            } else if (args[i].equalsIgnoreCase("-initialSweep") && i+3 < len) {
                m_initialSweep = new double[3];
                String start = args[++i];
                String stop = args[++i];
                String np = args[++i];
                try {
                    m_initialSweep[0] = Double.parseDouble(start);
                    m_initialSweep[1] = Double.parseDouble(stop);
                    m_initialSweep[2] = Double.parseDouble(np);
                    System.setProperty("apt.initialStart", start);
                    System.setProperty("apt.initialStop", stop);
                    System.setProperty("apt.initialNp", np);
                } catch (NumberFormatException nfe) {
                    m_initialSweep = null;
                }
            } else if (args[i].equalsIgnoreCase("-lockPort") && ((i+1) < len)) {
                try {
                    m_lockPort = Integer.parseInt(args[++i]);
                } catch (NumberFormatException nfe) {
                }
                // System property "apt.lockPort" is set below
            } else if (args[i].equalsIgnoreCase("-exec") && i + 1 < len) {
                if (m_commands == null) {
                    m_commands = args[++i];
                } else {
                    m_commands += ";" + args[++i];
                }
            } else if (args[i].equalsIgnoreCase("-debug") && ((i+1) < len)) {
                try {
                    m_debugLevel = Integer.parseInt(args[++i]);
                } catch (NumberFormatException nfe) {
                    DebugOutput.addCategories(args[i]);
                }
            } else if (args[i].equalsIgnoreCase("-noGui")) {
                disableGui();
            } else if (args[i].equalsIgnoreCase("-simpleGui")) {
                System.setProperty("apt.simpleGui", "true");
            } else if (args[i].equalsIgnoreCase("-torque")) {
                System.setProperty("apt.torque", "true");
            } else if (args[i].equalsIgnoreCase("-auto")) {
                m_autoFlag = true;
            } else if (args[i].equalsIgnoreCase("-update") && i + 1 < len) {
                m_tuneUpdate = args[++i].trim().toLowerCase();
                //System.setProperty("apt.refPositionUpdating", m_tuneUpdate);
            }  else if (args[i].equalsIgnoreCase("-maxStep") && i + 1 < len) {
                try {
                    m_maxStepSize = Integer.parseInt(args[++i]);
                } catch (NumberFormatException nfe) {
                }
            } else if (args[i].equalsIgnoreCase("-dispInfo") ) {
            	m_dispInfo= true;
                disableGui();
            } else if (args[i].equalsIgnoreCase("-probeNamePopup") ) {
                m_probeNamePopup= true;
            } else if (args[i].equalsIgnoreCase("-probeid")) {
            	if (i+1 < len && !args[i+1].startsWith("-"))
                    m_useProbeId = Boolean.valueOf(args[++i]);
            	else
                    m_useProbeId = true;
            } else if (args[i].equalsIgnoreCase("-dontLock")) {
            	m_useLock = false;
            } else if (args[i].equalsIgnoreCase("+dontLock")) {
            	m_useLock = true;
            } else if (args[i].equalsIgnoreCase("-probeidiolock")) {
            	// use when ProbeID and ProbeTune are running in same Java VM
            	ProbeIdClient.io_synch = true;
            } else if (args[i].equalsIgnoreCase("-unittest")) {
            	System.setProperty("INFO2VJ", Boolean.toString(true));
            	if (args.length == 0) {
                    org.junit.runner.JUnitCore.main("vnmr.apt.ProbeTuneTest");
                    System.exit(0);
            	}
            } else if (args[i].equalsIgnoreCase("-mode") && i + 1 < len) {
                m_modeName = args[++i];
            } else if (args[i].equalsIgnoreCase("-noAutoExit") ) {
                m_autoExit = false;
            } else {
            	Messages.postDebugWarning("Bad command line flag: " + args[i]);
            }
        }
        System.setProperty("apt.lockPort", "" + m_lockPort);
        return probeName;
    }

    public static void disableGui() {
        m_guiFlag = false;
        m_autoFlag = true;      // No Gui implies "auto" since there
                                // should be no interactive message
    }

    /**
     * Create and initialize the master, ProbeTune, object.
     * @param probeName The value of the Vnmr "probe" parameter.
     * @param usrProbeName The name of the user directory for the persistence
     * files.
     * @throws InterruptedException If tuning is aborted before
     * initialization completes.
     */
    public ProbeTune(String probeName, String usrProbeName)
                throws InterruptedException {

        ServerSocket locksocket;
        Messages.postDebug("Initialization", "ProbeTune<init>");
        locksocket = applicationRunning(this.getClass().getName(),
                                        m_lockPort);
        if (locksocket == null) {
            Messages.postError("Another instance of ProbeTune is running.");
            exit(-4, "failed - Another instance of ProbeTune is running");
        } else {
            // Listen for the "quitgui" command on the lock socket
            new CommandListener(m_lockPort, locksocket, this).start();
            writePid();
        }
        Messages.postDebug("Initialization", "ProbeTune<lock>");

        setProbe(probeName);
        m_usrProbeName = usrProbeName;
        Messages.postDebug("Initialization", "ProbeTune<probe>");
        setRequiredProperties();
        readPropertiesFromFile(getVnmrPropertiesFile());
        Messages.postDebug("Initialization", "ProbeTune<properties>");
        DebugOutput.addCategories(System.getProperty("apt.debugCategories"));
        if (!readPropertiesFromFile(getUserConfigFile())) {
            readPropertiesFromFile(getSystemConfigFile());
        }
        Messages.postDebug("Initialization", "ProbeTune<dbg-categories>");
        DebugOutput.addCategories(System.getProperty("apt.debugCategories"));
        if (!readPropertiesFromFile(getUserProbeConfigFile())) {
            readPropertiesFromFile(getSystemProbeConfigFile());
        }
        DebugOutput.addCategories(System.getProperty("apt.debugCategories"));

        Messages.postDebug("Initialization", "ProbeTune<debug>");

        // NB: We call these static initializations explicitly to make sure
        // they happen after all System Properties are set.
        // Initialize debug output first:
        setOptions();
        ReflectionData.setOptions();
        MotorControl.setOptions();
        ChannelInfo.setOptions();

        if (!checkProbeName(m_probeName, false)) {
            String msg = "No system tune directory found for probe \""
                + m_probeName + "\"";
            Messages.postError(msg);
            // NB: No longer exiting on this error
            //errorExit(msg);
        }
        //m_sweepType = sweepType;

        //m_tuneCriteria = TuneCriterion.getStandardCriteria();

        setDefaultTarget();

        m_listeners = new CommandListenersThread(this);
        m_listeners.start();

        // Check for channel and motor files
        //Messages.postDebug("Checking for configuration files");/*DBG*/
        if (!isProbeSupported()) {
            Messages.postDebugWarning("No system tune files for probe \""
                                      + m_probeName + "\"");
            exit("failed - no system tune files for probe \""
                 + m_probeName + "\"");
        }
        Messages.postDebug("Initialization", "ProbeTune<config>");

        // Start up GUI with communication
        m_isTorqueTool  = TuneUtilities.getBooleanProperty("apt.torque", false);
        boolean isSimpleGui;
        isSimpleGui = TuneUtilities.getBooleanProperty("apt.simpleGui", false);
        if (m_guiFlag) {
            try { // Don't fail just because we can't get a GUI!
                int port = m_listeners.getPort(20000);
                String console = System.getProperty("apt.console", "vnmrs");
                if (m_isTorqueTool) {
                    new TorqueGui(probeName, usrProbeName,
                                  getSysTuneDir(), getUsrTuneDir(),
                                  "localhost", port, console);
                    Messages.postDebug("Initialization", "Torque GUI");
                } else if (isSimpleGui) {
                    new SimpleGui(probeName, usrProbeName,
                                  getSysTuneDir(), getUsrTuneDir(),
                                  "localhost", port, console);
                    Messages.postDebug("Initialization", "Simple ProTune GUI");
                } else {
                    new ProbeTuneGui(probeName, usrProbeName, m_modeName,
                                     getSysTuneDir(), getUsrTuneDir(),
                                     "localhost", port, console);
                    Messages.postDebug("Initialization", "Started ProTune GUI");
                }
            } catch (Exception e) {
                Messages.postDebugWarning("Exception starting GUI: " + e);
                Messages.postStackTrace(e);
            } catch (java.lang.InternalError e) {
                Messages.postDebugWarning("Error starting GUI: "
                                          + e.getMessage());
            }
        }

        if (m_cmdHostAndPort != null && m_cmdHostAndPort.length() > 0) {
            String host = extractHost(m_cmdHostAndPort);
            int port = extractPort(m_cmdHostAndPort);
            if (port > 0) {
                try {
                    m_listeners.connectTo(host, port);
                } catch (Exception e) {}
            }
        }

        sendOptionsToGui();
        Messages.postDebug("Initialization", "Sent options to GUI");

        /*  NOTE:  In case of error, can NOT call function "exit()" above here
         *         because motorControl and m_listener must both exist otherwise
         *         exit() will give NullPointerException!
         */

        motorControl = new MotorControl(this, m_motorHostname);
        if (m_isTorqueTool) {
            motorControl.setTorqueDataDir(getUsrTuneDir());
        }
        Messages.postDebug("Initialization", "motorControl=" + motorControl);
        if (isMotorOk()) {
            sendToGui("motorOk yes");
        } else {
            //Messages.postError(" *** NO MOTOR CONTROL *** ");
            sendToGui("motorOk no");
//            if (m_autoFlag) {
//                Messages.postError("No motor control: " + m_motorHostname);
//                exit(-3, "failed - No motor control: "+ m_motorHostname);
//            }
        }

        Messages.postDebug("Initialization",
                           "Probe Tune: check if using network analyzer");
        String sweepType = System.getProperty("apt.sweepType");
        Messages.postDebug("Initialization",
                           "Probe Tune: apt.sweepType property: \"" + sweepType
                           + "\", m_sweepType=\"" + m_sweepType + "\"");
        if (sweepType != null) {
            m_sweepType = sweepType;
        }
        if (!m_isTorqueTool) {
            if (m_sweepType.equals("mt")) { // Use MTune (actually protune)
                Messages.postDebug("Initialization",
                                   "Probe Tune: using Vnmr data");
                // Make sure cal directory is ready
                MtuneControl.getCalDir(m_probeName);

                if (m_initialSweep != null) {
                    sweepControl = new MtuneControl(this, true,
                                                    m_initialSweep[0],
                                                    m_initialSweep[1],
                                                    (int)m_initialSweep[2]);
                } else {
                    sweepControl = new MtuneControl(this, true);
                }
                ((MtuneControl)sweepControl).setMotorController(motorControl);
            } else {
                // Use network analyzer
                Messages.postDebug("Initialization",
                "Probe Tune: using network analyzer");
                sweepControl = new NetworkAnalyzer(this, getSweepIpAddr());
            }
        }
        Messages.postDebug("Initialization", "sweepControl=" + sweepControl);
        if (isSweepOk()) {
            sendToGui("sweepOk yes");
        } else if (!m_isTorqueTool){
            Messages.postError(" *** NO SWEEP CONTROL *** ");
            sendToGui("sweepOk no");
            if (m_autoFlag) {
                Messages.postError("No sweep control: " + m_sweepType);
                exit(-2, "failed - No sweep control: "+m_sweepType);
            }
        }
        Messages.postDebug("Initialization", "ProbeTune<sweeptype>");

        setSysInitialized(true);
        Messages.postDebug("Initialization", "m_sysInitialized=true");
        Messages.postDebug("Initialization", "displayStatus: INITIALIZE");
        displayStatus(STATUS_INITIALIZE);

        //ChannelInfo chanInfo = getChannelInfo(m_channelNumber);
        //String chanName = chanInfo.getPersistenceFileName();
        //sendToGui("displayChannel " + chanName);
        sendToGui("displayMatchTolerance " + m_criterion.toString());
        Messages.postDebug("Initialization", "m_targetMatch="
                           + getTargetMatch());

        setTuneMode(1000000);

        // Set the channel number to the first available
        try {
            ChannelInfo ci = getChannelInfos().get(0);
            m_channelNumber = ci.getChannelNumber();
        } catch (Exception e) {
            exit("No tune channels are defined");
        }

        if (m_isTorqueTool) {
            setChannel(m_channelNumber, 0, false);
            sendToGui("sweepOk yes");
            displayStatus(STATUS_READY);
        } else if (m_commands == null) {
            // Default initial command
            setChannel(m_channelNumber, 0, true);
            ChannelInfo channelInfo = getChannelInfo(m_channelNumber);
            Messages.postDebug("Initialization", "channelInfo=" + channelInfo);
            measureTune(channelInfo, 00, true);
            displayStatus(STATUS_READY);
        } else {
            displayStatus(STATUS_READY);
            // Execute args from command line
            // Commands are separated by semi-colons
            StringTokenizer toker = new StringTokenizer(m_commands, ";");
            while (toker.hasMoreTokens()) {
                String cmd = toker.nextToken();
                displayCommand(cmd);
                exec(cmd);
            }
            if (m_autoExit ) {
                Messages.postDebug("Initialization", "ProbeTune<done>");
                exec("setTuneMode 0");
                exec("exit");
            }
        }

        // I don't believe this works, in general. -- ChrisP
        if(m_dispInfo){
            m_dispInfo=false;
            String popup;
            popup = displayInfo();
            popup = popup.replaceAll("<br>", NL);
            JOptionPane optPane= new JOptionPane();
            JOptionPane.showMessageDialog(optPane, popup, "Protune Info",
                                          JOptionPane.INFORMATION_MESSAGE);
            exit(0, "Protune info done");
        }
    }

    public void quit(int ms) throws InterruptedException {
        if (motorControl != null) motorControl.quit(ms);
        if (sweepControl != null)
            if (sweepControl instanceof MtuneControl)
                ((MtuneControl)sweepControl).stopTuneModeThread(ms);
        MtuneControl.close();
        m_listeners.close();
    }

    private void setProbe(String probeName) {
        m_probeName = probeName;
        if (m_useProbeId) {
            try {
                m_probeId = new AptProbeIdClient();
            } catch (FileNotFoundException e) {
                Messages.postError("error setting probe: "+e.getMessage());
                e.printStackTrace();
            }
        }
    }

    public static String getProbeFile(String file, String subdir, boolean system) {
        String tuneDir = subdir + File.separator + file;
        if (m_useProbeId) {
            File blob = m_probeId.blobLink(file, subdir, system, false);
            if (blob != null)
                return blob.getPath();
            else return null;
        } else {
            return (system ? getVnmrUserDir() : getVnmrSystemDir())
                    + File.separator + tuneDir;
        }
    }

//    private boolean isProbeSupported(String probeName) {
//        String dir = getProbeFile(probeName, "tune", false);
//        assert(m_useProbeId || dir.equals(getVnmrSystemDir() + "/tune/" + probeName));
//        boolean ok = ChannelInfo.isProbeSupported(dir);
//        ok &= MotorInfo.isProbeSupported(dir);
//        return ok;
//    }

    private boolean isProbeSupported() {
        String dir = getSysTuneDir();
        boolean ok = ChannelInfo.isProbeSupported(dir);
        ok &= MotorInfo.isProbeSupported(dir);
        return ok;
    }

    private String getSystemConfigFile() {
        return getVnmrSystemDir() + File.separator + "tune"
                + File.separator + "protuneConfig";
    }

    private String getUserConfigFile() {
        return getVnmrUserDir() + File.separator + "tune"
                + File.separator + "protuneConfig";
    }

    private String getSystemProbeConfigFile() {
        String file = getProbeFile("protuneConfig",
				   "tune"+File.separator+m_probeName, true);
        assert(m_useProbeId
               || file.equals(getVnmrSystemDir() + File.separator + "tune"
                              + File.separator + m_probeName
                              + File.separator + "protuneConfig"));
        return file;
    }

    private String getUserProbeConfigFile() {
        String file = getProbeFile("protuneConfig",
                                   "tune"+File.separator+m_usrProbeName, false);
        assert(m_useProbeId
               || file.equals(getVnmrUserDir() + File.separator + "tune"
                              + File.separator + m_usrProbeName
                              + File.separator + "protuneConfig"));
    	return file;
    }

    private String getVnmrPropertiesFile() {
        return getVnmrSystemDir() + File.separator + "tmp" + File.separator
                + "ptuneProperties";
    }

    /**
     * Extract the port number from a string of the form:
     * "<code>hostname:1234</code>".
     * @param hostAndPort The string to parse.
     * @return The port number, or 0 on failure.
     */
    private int extractPort(String hostAndPort) {
        int port = 0;
        String strPort = hostAndPort;
        int idx = hostAndPort.indexOf(":");
        if (idx > 0) {
            strPort = hostAndPort.substring(idx + 1);
        }
        try {
            port = Integer.parseInt(strPort);
        } catch (NumberFormatException nfe) {
            Messages.postDebugWarning("Bad cmd port number: " + hostAndPort);
        }
        return port;
    }

    /**
     * Extract the host name from a string of the form:
     * "<code>hostname:1234</code>".
     * @param hostAndPort The string to parse.
     * @return The port number, or "localhost" if there is no host
     * part in the input string.
     */
    private String extractHost(String hostAndPort) {
        int idx = hostAndPort.indexOf(":");
        if (idx > 0) {
            return hostAndPort.substring(0, idx);
        } else {
            return "localhost";
        }
    }

    public boolean isMotorOk() {
        return motorControl != null && motorControl.isConnected();
    }

    public boolean isSweepOk() {
        return (sweepControl != null && sweepControl.isConnected());
    }

    private void setDefaultTarget() {
        try {
            String matchThresh = System.getProperty("apt.targetMatch");
            setTargetMatch(Double.valueOf(matchThresh));
        } catch(Exception e) {
        }
        // NB: This will usually be overridden by a channel-specific value
        try {
            String targetFreq = System.getProperty("apt.targetFreq");
            m_targetFreq = Double.valueOf(targetFreq);
        } catch(Exception e) {
        }
    }

    /**
     * Make sure that all the System Properties we expect are actually there.
     */
    private void setRequiredProperties() {
    }

    private static void setOptions() {
        // TODO: Too late to set sysdir and userdir, we already used them
        // to read the property files.
        setStringFromProperty(m_vnmrSystemDir, "apt.vnmrSystemDir");
        setStringFromProperty(m_vnmrUserDir, "apt.vnmrUserDir");
        setStringFromProperty(m_tuneUpdate, "apt.refPositionUpdating");
        String key = "apt.useRefPositions";
        m_useRefPositions = TuneUtilities.getBooleanProperty(key, true);
        key = "apt.favorCurrentChannel";
        m_favorCurrentChannel  = TuneUtilities.getBooleanProperty(key, false);
        key = "apt.maxStepSize";
        m_maxStepSize = TuneUtilities.getIntProperty(key, m_maxStepSize);
        key = "apt.stepDamping";
        m_stepDamping = TuneUtilities.getDoubleProperty(key, m_stepDamping);
    }

    private static void setStringFromProperty(String var, String property) {
        String value = System.getProperty(property);
        if (value != null) {
            var = value;
        }
    }

    /**
     * Read properties from the ptuneProperties file, and set them
     * as System properties.
     * @param path The path to the properties file.
     * @return True if the file was found and was readable.
     */
    private static boolean readPropertiesFromFile(String path) {
        boolean ok = true;
        Messages.postDebug("Properties", "Reading properties from " + path);
        BufferedReader in = TuneUtilities.getReader(path);
        if (in == null) {
            ok = false;
        } else {
            try {
                String line;
                while ((line = in.readLine()) != null) {
                    // Split off key from the rest of the line
                    String[] tokens = line.trim().split("[ \t=:]+", 2);
                    if (tokens.length == 2) {
                        String key = "apt." + tokens[0];
                        System.setProperty(key, tokens[1].trim());
                        Messages.postDebug("Properties",
                                           "Set property " + key
                                           + " to \"" + tokens[1] + "\"");
                    }
                }
            } catch (IOException ioe) {
                Messages.postDebug("Properties",
                                   "readPropertiesFromFile: " + ioe);
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }
        }
        return ok;
    }

    private void sendOptionsToGui() {
        sendToGui("UseRefs " + (m_useRefPositions ? "1" : "0"));
    }

    private ServerSocket grabServerSocket(String appname, int port)
    throws InterruptedException {
        boolean app_running = true;     //Assume another instance is running
        ServerSocket serverSocket = null;
        Messages.postDebug("KillApp",
                           "Check for previous instance of " + appname);
        for (int i = 0; app_running && i < 5; i++) {
            try {
                // Try to create a server socket using the given port number
                Messages.postDebug("KillApp",
                                   " See if port " + port + " is busy ...");
                serverSocket = new ServerSocket(port);
                app_running = false;
                Messages.postDebug("KillApp", "  ... OK -- not busy");
            } catch (IOException ioe) {
                // Somehow the port is currently used, this is an indication
                // that another instance is running (although this is not
                // definitely true!)
                Messages.postDebug("KillApp", "  ... port is busy");

                // Connect to lock socket and send a quit command
                String host = "127.0.0.1";
                Socket socket = TuneUtilities.getSocket("Killer", host, port);
                PrintWriter cmdSender = null;
                if (socket != null) {
                    try {
                        cmdSender =
                            new PrintWriter(socket.getOutputStream(), true);
                        Messages.postDebug("KillApp",
                        "  ... killing old instance");
                        cmdSender.println("quitgui");
                        Thread.sleep(500);
                    } catch (IOException ioe2) {
                        Messages.postDebug("KillApp",
                                           "   Cannot talk to port " + port);
                    } finally {
                        cmdSender.close();
                    }
                }

            }
        }
        return serverSocket;
    }

    protected ServerSocket applicationRunning(String appname, int port)
        throws InterruptedException {

        ServerSocket serverSocket = grabServerSocket(appname, port);
        if (serverSocket == null) {
            // It's still running; use a "kill" command
            String pid = readPid();
            if (isUnix() && pid != null && pid.length() > 0) {
                StringTokenizer toker = new StringTokenizer(pid);
                if (toker.hasMoreTokens()) {
                    pid = toker.nextToken();
                    String[] command = {"sh", "-c", "kill -9 " + pid};
                    try {
                        Runtime.getRuntime().exec(command);
                    } catch (IOException e) {
                    }
                }
            }
            serverSocket = grabServerSocket(appname, port);
        }
        if (serverSocket == null) {
            Messages.postError("Failed to kill previous instance of ProTune");
        }
        return serverSocket;
    }

    private static void writePid() {
        String pid = getMyPid();
        String pidPath = getVnmrSystemDir() + "/tmp/ptunePid";
        new File(pidPath).delete();
        TuneUtilities.writeFile(pidPath, pid);
    }

    private static String getMyPid() {
        String pid = ManagementFactory.getRuntimeMXBean().getName();
        if (pid != null) {
            StringTokenizer toker = new StringTokenizer(pid, "@ \t\n");
            if (toker.hasMoreTokens()) {
                pid = toker.nextToken();
            }
        }
        return pid;
    }

    private String readPid() {
        String pidPath = getVnmrSystemDir() + "/tmp/ptunePid";
        String pid = TuneUtilities.readFile(pidPath, false);
        return pid;
    }

    private static boolean isUnix() {
        String os = System.getProperty("os.name").toLowerCase();
        //linux or unix
        return (os.indexOf( "nix") >= 0 || os.indexOf( "nux") >= 0);
    }

    public String getConsoleType() {
        return System.getProperty("apt.console", "vnmrs");
    }

    public static boolean isQuadPhaseProblematic(String console) {
        return "vnmrs".equalsIgnoreCase(console);
    }
    
    public static boolean isTuneModeSwitchable(String console) {
        return "inova".equalsIgnoreCase(console);
    }

    public static boolean isTuneBandSwitchable(String console) {
        return ("inova".equalsIgnoreCase(console)
                || "mercury".equalsIgnoreCase(console));
    }

    /**
     * Remove duplicate status messages from the given list and return
     * remaining messages as an array of Strings. The idea is to keep only
     * the last result for each tune frequency.
     * @param statusMsgs The full list of tune results.
     * @return The culled results as an array of String messages.
     */
    public static String[] cullTuneResults(ArrayList<TuneResult> statusMsgs) {
        ArrayList<TuneResult> culledList = new ArrayList<TuneResult>();
        int n = statusMsgs.size();
        // Go through list backwards; add only last msg for each frequency
        for (int i = n - 1; i >= 0; --i) {
            TuneResult tr = statusMsgs.get(i);
            // NB: TuneResult is "equal" iff frequency is the same
            if (!culledList.contains(tr)) {
                culledList.add(0, tr); // Add to front of list
            }
        }
        int len = culledList.size();
        String[] culledArray = new String[len];
        for (int i = 0; i < len; i++) {
            culledArray[i] = culledList.get(i).toString();
            Messages.postDebug("VnmrMessages",
                               "cullTuneResults: " + culledArray[i]);
        }
        return culledArray;
    }

    /**
     * Exit with a given single status message.
     * The exitCode will be 0 unless the message cannot be
     * sent to Vnmr, in which case it will be -1.
     * @param msg The message.
     */
    public void exit(String msg) {
        exit(-1, msg);
    }

    /**
     * Exit with a given exitCode and a given single status message.
     * The exitCode will be changed to -1 if the message cannot be
     * sent to Vnmr.
     * @param exitCode The exit code.
     * @param msg The message.
     */
    public void exit(int exitCode, TuneResult msg) {
        ArrayList<TuneResult> list = new ArrayList<TuneResult>(1);
        list.add(msg);
        exit(exitCode, list);
    }

    /**
     * Exit with a given exitCode and a given single status message.
     * The exitCode will be changed to -1 if the message cannot be
     * sent to Vnmr.
     * @param exitCode The exit code.
     * @param msg The message.
     */
    public void exit(int exitCode, String msg) {
        String[] list = {msg};
        exit(exitCode, list);
    }

    /**
     * Exit with a given exitCode and a given list of status messages.
     * Each status message is logged by Vnmr, and a summary status is
     * constructed sent as the "done" status of the ProTune run.
     * The exitCode will be 0 unless the messages cannot be
     * sent to Vnmr, in which case it will be -1.
     * @param statusMsgs The messages.
     */
    public void exit(ArrayList<TuneResult> statusMsgs) {
        exit(0, statusMsgs);
    }

    public void exit(int exitCode, ArrayList<TuneResult> statusMsgs) {
        if (statusMsgs == null) {
            statusMsgs = new ArrayList<TuneResult>();
        }
        String[] msgs = cullTuneResults(statusMsgs);
        exit(exitCode, msgs);
    }

    /**
     * Exit with a given exitCode and a given list of status messages.
     * Each status message is logged by Vnmr, and a summary status is
     * constructed and sent as the "done" status of the ProTune run.
     * The exitCode will be changed to -1 if the messages cannot be
     * sent to Vnmr.
     * @param exitCode The exit code.
     * @param statusMsgs The messages.
     */
    public void exit(int exitCode, String[] statusMsgs) {
        int n = statusMsgs.length;
        Messages.postDebug("TuningSummary", "Exiting ProTune with "
                           + n + " results");


        try {
            setTuneMode(0);

            // Keep only last result for each frequency

            String msg = "";
            String summaryStatus = (exitCode == 0) ? "ok" : "failed";
            for (String status : statusMsgs) {
                if (status.startsWith("failed")) {
                    summaryStatus = "failed";
                    msg += VJ_ERR_MSG;
                } else if (!"failed".equals(summaryStatus)
                           && status.startsWith("Warning"))
                {
                    summaryStatus = "warning";
                    msg += VJ_WARN_MSG;
                } else {
                    msg += VJ_INFO_MSG;
                }
                // ProTune reports (prints) each individual status
                msg += ("protune('print', '" + status + "')\n");
            }
            if (statusMsgs.length == 0 && m_tuningAttempted) {
                // Nothing tuned
                summaryStatus = "failed - tuning did not complete";
            }
            // Overall ProTune result is the summaryStatus
            Thread.sleep(100);
            String pfx = summaryStatus.startsWith("ok")
                    ? VJ_INFO_MSG
                    : (summaryStatus.startsWith("warning") ? VJ_WARN_MSG
                       : VJ_ERR_MSG);
            msg += (pfx + "protune('done', '" + summaryStatus + "')");
            Messages.postDebug("TuningSummary",
                               "Summary done status=" + summaryStatus);
            queueToVnmr(msg);
        } catch (Exception e) {
            errorExit("Problem reporting results: " + e);
        }
        if (exitCode < 0) {
            Messages.postError(statusMsgs[0]);
        }
        System.exit(exitCode);
    }

    public void errorExit(String msg) {
        msg = VJ_ERR_MSG + "protune('done', 'failed - " + msg + "')";
        queueToVnmr(msg);
        exit();
    }

    /**
     * Forced exit with no messages.
     * The exit code will be -1.
     */
    public void exit() {
        System.exit(-1);
    }

    public void displayTuneStatus() {
        //Currently GUI is not supporting this command
        //sendToGui("displayTuneStatus " + m_tuneStatus.toString("db"));
    }

    /*
     * Abort any pending and current command if possible.
     * The routine will be executed in the calling thread. This
     * may be a problem in some case. It would be better to modify
     * it to run only in main thread in the future.
     */
    public void abort(String msg) {
        Messages.postWarning(msg);
        m_commandList.clear();
        setCancel(true);
        try {
            motorControl.abort();
        } catch (Exception e) {
        }
        return;
    }

    public void exec(String cmd) {
        String lcmd = cmd.toLowerCase();
        Messages.postDebug("exec", "exec(\"" + cmd + "\")");
        if (lcmd.startsWith("abort")) {
            if (m_execThread != null) {
                m_execThread.interrupt();
            }
            String msg = null;
            if (!lcmd.contains("quiet")) {
                msg = "ProTune ABORT";
            }
            abort(msg);
            return;
        } else if (lcmd.equals("quitgui")) {
            /*
             *  Need to handle "quitgui" here, otherwise it will never
             * get executed if the previous command is stuck.
             */
            abort("ProbeTune.exec: QUITGUI!");
            exit();
            return;
        }
        setCancel(false);
        m_commandList.add(cmd);
        if (m_nExecThreadsRunning < 1) {
            m_execThread  = new ExecTuneThread();
            m_execThread.start();
        }
    }

    /**
     * Execute a command synchronously (support for unit testing)
     * @param cmd The command to execute.
     * @throws InterruptedException If command is aborted.
     */
    public void execTest(String cmd) throws InterruptedException {
    	m_commandList.add(cmd);
    	execute();
    }

    /**
     * Execute a command.  This is called in a separate thread that is
     * just for this command.  Synchronized, so that only one command
     * runs at a time.  Only the "abort" command bypasses this; abort
     * runs in the main thread.
     * @throws InterruptedException if this command is aborted.
     */
    synchronized public void execute() throws InterruptedException {
        if (m_cancel) {
            Messages.postDebug("Exec",
                               "Quitting: " + Thread.currentThread().getName());
            return;             // Canceled while we were queued up
        }
        if (m_commandList.size() == 0) {
            // Nothing to do
            return;
        }
        String cmd = m_commandList.remove(0);
        Thread.currentThread().setName(cmd); // Set name for the debugger
        Messages.postDebug("Exec", "ProbeTune: executing \"" + cmd + "\"");
        StringTokenizer toker = new StringTokenizer(cmd);
        //if (toker.hasMoreTokens()) {
        //    toker.nextToken();  // Discard label for now
        //}

        String key = "";
        if (toker.hasMoreTokens()) {
            key = toker.nextToken();
        }

        if (key.equalsIgnoreCase("getData")) {
            if (isSweepOk()) {
                sendToGui("sweepOk yes");
                // TODO: Wait until sweep is set
                int delay = 0;
                ChannelInfo chInfo = getChannelInfo(m_channelNumber);
                if (toker.hasMoreTokens()) {
                    String sDelay = toker.nextToken();
                    try {
                        delay = Integer.parseInt(sDelay);
                    } catch (NumberFormatException nfe) {
                        Messages.postError("Bad delay count in getData cmd: \""
                                           + sDelay + "\"");
                    }
                }
                ReflectionData data = sweepControl.getData(chInfo, delay);
                if (data != null) {
                    m_reflectionData = data;
                    displayData(data);

                    /*double reflection = data.getDipValue();
                    //double complexReflection = data.getPhasedReflection();
                    //complexReflection *= complexReflection;
                    printlnDebugMessage(5, "DipData: " + (data.getDipFreq()/1e6)
                                       + " " + reflection
                                       //+ " " + complexReflection
                                       );/*DBG*/
                }
            }
        } else if (key.equalsIgnoreCase("displayData")) {
            displayData(m_reflectionData);
        } else if (key.equalsIgnoreCase("clearCal")) {
            sweepControl.clearCal();
        } else if (key.equalsIgnoreCase("savePhase")) {
            saveQuadPhase();
        } else if (key.equalsIgnoreCase("setPhase")) {
            int phase = -1;
            try {
                phase = Integer.parseInt(toker.nextToken());
                if (Math.abs(phase) > 3) {
                    phase = (int)(Math.round(phase / 90.0));
                }
                phase = phase % 4;
            } catch (NumberFormatException nfe) {
            } catch (NoSuchElementException nsee) {
            }
            setQuadPhase(phase);
        } else if (key.equalsIgnoreCase("setProbeDelay")) {
            double delay_ns = 0;
            try {
                delay_ns = Double.valueOf(toker.nextToken());
            } catch (NumberFormatException nfe) {
            } catch (NoSuchElementException nsee) {
            }
            setProbeDelay(delay_ns);
        } else if (key.equalsIgnoreCase("setSweep")) {
            if (toker.countTokens() == 4) {
                String sCenter = toker.nextToken();
                String sSpan = toker.nextToken();
                String sNp = toker.nextToken();
                String sRfchan = toker.nextToken();
                try {
                    double center = Double.parseDouble(sCenter);
                    double span = Double.parseDouble(sSpan);
                    int np = Integer.parseInt(sNp);
                    int rfchan = Integer.parseInt(sRfchan);
                    if (rfchan == 0) {
                        rfchan = getChannel().getRfchan();
                    }
                    setSweep(center, span, np, rfchan, true);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad argument in setSweep cmd: \""
                                       + cmd + "\"");
                }
            }
        } else if (key.equalsIgnoreCase("setCenter")) {
            if (toker.hasMoreTokens()) {
                String sCenter = toker.nextToken();
                try {
                    double center = Double.parseDouble(sCenter);
                    setCenter(center);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad frequency in setCenter cmd: \""
                                       + sCenter + "\"");
                }
            }
        } else if (key.equalsIgnoreCase("setNP")) {
            if (toker.hasMoreTokens()) {
                String sNp = toker.nextToken();
                try {
                    setNp(Integer.parseInt(sNp));
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad number of points in setNP cmd: \""
                                       + sNp + "\"");
                }
            }
        } else if (key.equalsIgnoreCase("setCalSweep")) {
            if (toker.countTokens() < 2) {
                Messages.postError("Need at least 2 arguments to setCalSweep");
            } else {
                String sFreq0 = toker.nextToken();
                String sFreq1 = toker.nextToken();
                int rfchan = 0;
                try {
                    if (toker.hasMoreTokens()) {
                        rfchan = Integer.parseInt(toker.nextToken());
                    }
                    double freq0 = Double.parseDouble(sFreq0) * 1e6;
                    double freq1 = Double.parseDouble(sFreq1) * 1e6;
                    setCalSweep(freq0, freq1, rfchan);
                    trackTune(1);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad number in \"setCalSweep\" command");
                }
            }
        } else if (key.equalsIgnoreCase("trackTune")) {
            int nSecs = 1;
            if (toker.hasMoreTokens()) {
                String sSecs = toker.nextToken();
                try {
                    nSecs = Integer.parseInt(sSecs);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad time in trackTune cmd: \""
                                       + sSecs + "\"");
                }
            }
            trackTune(nSecs);
        } else if (key.equalsIgnoreCase("step")) {
            if (isMotorOk()) {
                sendToGui("motorOk yes");
            }
            //displayStatus(STATUS_RUNNING);
            int cmi = 0;     // Which motor on this channel
            int nSteps = 0;
            if (toker.hasMoreTokens()) {
                String sMotor = toker.nextToken();
                try {
                    cmi = Integer.parseInt(sMotor);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad motor number in step cmd: \""
                                       + sMotor + "\"");
                }
            }
            if (toker.hasMoreTokens()) {
                String sSteps = toker.nextToken();
                try {
                    nSteps = Integer.parseInt(sSteps);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad number of steps in step cmd: \""
                                       + sSteps + "\"");
                }
            }
            ChannelInfo ci = getChannelInfo(m_channelNumber);
            ci.step(cmi, nSteps);
            //displayStatus(STATUS_READY);
        } else if (key.equalsIgnoreCase("stepGMI")) {
            if (isMotorOk()) {
                sendToGui("motorOk yes");
            }
            //displayStatus(STATUS_RUNNING);
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int nSteps = Integer.parseInt(toker.nextToken());
                ChannelInfo ci = getChannelInfo(m_channelNumber);
                int cmi = ci.getChannelMotorNumber(gmi);
                if (cmi >= 0) {
                    // This is one of the channel motors, do a channel step.
                    ci.step(cmi, nSteps);
                } else if (gmi >= 0) {
                    getMotorControl().step(gmi, nSteps);
                    trackTune(1);
                }
           } catch (NumberFormatException nfe) {
               postCommandError(cmd);
           } catch (NoSuchElementException nsee) {
               postCommandError(cmd);
           }
           //displayStatus(STATUS_READY);
        } else if (key.equalsIgnoreCase("gotoGMI")) {
            //displayStatus(STATUS_RUNNING);
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int position = Integer.parseInt(toker.nextToken());
                ChannelInfo ci = getChannelInfo(m_channelNumber);
                int tolerance = 3;
                int cmi = ci.getChannelMotorNumber(gmi);
                if (cmi >= 0) {
                    // This is Tune or Match.
                    // Use ChannelInfo so that slaves get moved as well.
                    ci.moveMotorToPosition(gmi, position, tolerance);
                } else if (gmi >= 0) {
                    MotorControl mc = getMotorControl();
                    mc.moveMotorToPosition(gmi, position, tolerance);
//                    MotorInfo mi = mc.getMotorInfo(gmi);
//                    int curPosition = mi.getPosition();
//                    int nSteps = position - curPosition;
//                    mc.step(gmi, nSteps);
//                    trackTune(1);
                }
           } catch (NumberFormatException nfe) {
               postCommandError(cmd);
           } catch (NoSuchElementException nsee) {
               postCommandError(cmd);
           }
            //displayStatus(STATUS_READY);
        } else if (key.equalsIgnoreCase("displayTargetFreq")) {
            if (toker.hasMoreTokens()) {
                String sFreq = toker.nextToken();
                try {
                    m_targetFreq = Double.parseDouble(sFreq);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad freq in displayTargetFreq cmd: \""
                                       + sFreq + "\"");
                }
            }
            displayTargetFreq(m_targetFreq, m_channelNumber);

        } else if (key.equalsIgnoreCase("goToRef")) {
            // goTo <freq>
            double freq = Double.parseDouble(toker.nextToken());
            if (freq < 1e4) {
                freq *= 1e6; // Deal with freqs given in MHz
            }
            goToRef(freq);
        } else if (key.equalsIgnoreCase("tuneTo")) {
            // tuneTo <freq> [noref|useref] [<criterion>] [<rfchan>] [<nucleus>]
            // First arg must be frequency
            // remaining (optional) args may be in any order
            m_tuningAttempted = true;
            double freq = Double.parseDouble(toker.nextToken());
            if (freq < 1e4) {
                freq *= 1e6; // Deal with freqs given in MHz
            }
            TuneTarget target = new TuneTarget(freq, m_criterion);
            boolean useref = m_useRefPositions;
            while (toker.hasMoreTokens()) {
                String token = toker.nextToken();
                if (token.toLowerCase().equals("noref")) {
                    useref = false;
                } else if (token.toLowerCase().equals("useref")) {
                    useref = true;
                } else if (TuneCriterion.isStandardCriterion(token)) {
                    m_criterion = TuneCriterion.getCriterion(token);
                    target.setCriterion(m_criterion);
                } else if (token.matches("[0-9]+")) {
                    // Looks like an integer
                    target.setRfchan(Integer.parseInt(token));
                } else if (token.matches("[0-9]+\\.[0-9]+")) {
                    // Looks like a float
                    double thresh_pct = Double.parseDouble(token);
                    double thresh_db = TuneCriterion.percentToDb(thresh_pct);
                    m_criterion = new TuneCriterion(thresh_db);
                    target.setCriterion(m_criterion);
                } else {
                    // It better be a nucleus name
                    target.setNucleus(token);
                }
            }
            TuneResult tuneResult = tune(target, useref);
            m_tuneResults.add(tuneResult);
            Messages.postDebug("TuneStatus", "ProbeTune.execute(setTuneFreq): "
                               + "Tune status #" + m_tuneResults.size()
                               + ": " + tuneResult);
        } else if (key.equalsIgnoreCase("setTuneFrequency")) {
            //displayStatus(STATUS_RUNNING);
            m_tuningAttempted = true;
            if (toker.countTokens() > 1) {
                // Read channel number, but we don't actually use it any more
                String sChan = toker.nextToken();
                try {
                    Integer.parseInt(sChan);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad channel # in setTuneFreq cmd: \""
                                       + sChan + "\"");
                }
            }
            double freq = 0;
            if (toker.hasMoreTokens()) {
                String sFreq = toker.nextToken();
                try {
                    freq = Double.parseDouble(sFreq);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad frequency in setTuneFreq cmd: \""
                                       + sFreq + "\"");
                }
            }
            TuneTarget target = new TuneTarget(freq, m_criterion);

            // NB: Choose channel automatically
            TuneResult tuneResult = tune(target, m_useRefPositions);

            m_tuneResults.add(tuneResult);
            Messages.postDebug("TuneStatus", "ProbeTune.execute(setTuneFreq): "
                               + "Tune status #" + m_tuneResults.size()
                               + ": " + tuneResult);
        } else if (key.equalsIgnoreCase("setPredefinedTune")
                   || key.equalsIgnoreCase("getPredefinedTune"))
        {
            // getPredefinedTune goes to the correct channel for the
            //   predefined position but does not alter tune and match motors.
            // setPredefinedTune also sets the tune/match and reports result.
            Messages.postDebug("PredefinedTune", key);
            boolean setTuning = key.equalsIgnoreCase("setPredefinedTune");

            if (toker.hasMoreTokens()){
                String sampleName= toker.nextToken();
                if(!sampleName.startsWith("samp")){
                    sampleName= "samp" + sampleName;
                }
                Messages.postDebug("PredefinedTune",
                                   "Got samplename: " + sampleName
                                   + ", current channel: " + m_channelNumber);

                List<ChannelInfo> chanInfos = getChannelInfos();
                for (ChannelInfo ci : chanInfos) {
                    TunePosition pdtPsn= ci.getPositionAt(sampleName);
                    if(pdtPsn!=null){
                        int iChan = ci.getChannelNumber();
                        if (ci != getChannel()) {
                            //change channel
                            Messages.postDebug("PredefinedTune",
                                               "Switch to channel " + iChan);
                            String chanName = ci.getPersistenceFileNames(true);
                            sendToGui("displayChannel " + chanName);
                            setChannel(iChan, 0, false);
                        }
                        double freq = pdtPsn.getFreq();
                        Messages.postDebug("PredefinedTune",
                                           "Got predefined position: "
                                           + pdtPsn.getSampleName()
                                           + "; Set sweep to show "
                                           + Fmt.f(3, freq / 1e6));
                        displayTargetFreq(pdtPsn.getFreq(), iChan);
                        setSweepToShow(freq, freq);
                        if (setTuning) {
                            Messages.postDebug("PredefinedTune", "Set tuning");
                            ci.moveMotorToSavedPosition(pdtPsn, freq, 1);
                            measureTune(ci);
                            TuneTarget tgt = new TuneTarget(freq, m_criterion);
                            double matchPwr = ci.getMatchAtRequestedFreq(tgt);
                            double matchDb = 10 * Math.log10(matchPwr);
                            String isOk = matchDb <= tgt.getAccept_db()
                                ? "ok" : "failed";
                            m_tuneStatus = new TuneResult(isOk);
                            String msg = "Set to predefined position \""
                                + sampleName + "\"";
                            m_tuneStatus.setNote(msg);
                            m_tuneStatus.setTarget(tgt);
                            m_tuneStatus.setActualMatch_db(matchDb);
                            m_tuneResults.add(m_tuneStatus);
                            Messages.postDebug("TuneStatus",
                                               "Tune status #"
                                               + m_tuneResults.size()
                                               + ": " + m_tuneStatus);
                        } else {
                            measureTune(ci);
                        }
                        break;
                    }
                }
            }
        }  else if (key.equalsIgnoreCase("setChannel")) {
            if (toker.hasMoreTokens()) {
                setChannel(toker.nextToken(), true);
            }
        } else if (key.equalsIgnoreCase("setTuneMode")) {
            if (toker.hasMoreTokens()) {
                String sMode = toker.nextToken();
                try {
                    int mode = Integer.parseInt(sMode);
                    setTuneMode(mode);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad mode # in setTuneMode cmd: \""
                                       + sMode + "\"");
                }
            }
        } else if (key.equalsIgnoreCase("setRawDataMode")) {
            if (toker.hasMoreTokens()) {
                String sMode = toker.nextToken();
                try {
                    int mode = Integer.parseInt(sMode);
                    setRawDataMode(mode != 0);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad mode # in setRawDataMode cmd: \""
                                       + sMode + "\"");
                }
            }
        } else if (key.equalsIgnoreCase("saveRawDataMode")) {
            saveRawDataMode();
        } else if (key.equalsIgnoreCase("restoreRawDataMode")) {
            restoreRawDataMode();
        } else if (key.equalsIgnoreCase("setMatch")) {
            // Eg: setMatch <target_db>
            if (toker.hasMoreTokens()) {
                String sThresh = toker.nextToken();
                try {
                    double match = Double.parseDouble(sThresh);
                    setTargetMatch(Math.pow(10, -Math.abs(match / 10)));
                } catch (NumberFormatException nfe) {
                    if (TuneCriterion.isStandardCriterion(sThresh)) {
                        m_criterion = TuneCriterion.getCriterion(sThresh);
                    } else {
                        Messages.postError("Bad value in setMatch cmd: \""
                                           + sThresh + "\"");
                    }
                }
            }
        } else if (key.equalsIgnoreCase("setMatchMargin")) {
            try {
                double margin_db = Double.parseDouble(toker.nextToken());
                TuneCriterion.setDefaultMargin_db(margin_db);
                m_criterion = m_criterion.setMargin(margin_db);
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad value in setMatchMargin cmd: \""
                                   + cmd + "\"");
            } catch (NoSuchElementException nsee) {
                Messages.postError("Bad \"setMatchMargin\" command: \""
                                   + cmd + "\"");
            }
        } else if (key.equalsIgnoreCase("setMatchThresh")) {
            if (toker.countTokens() > 1) {
                String sChan = toker.nextToken();
                try {
                    Integer.parseInt(sChan);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad channel # in setMatchThresh cmd: \""
                                       + sChan + "\"");
                }
            }
            if (toker.hasMoreTokens()) {
                String sThresh = toker.nextToken();
                try {
                    double thresh = Double.parseDouble(sThresh);
                    m_criterion = new TuneCriterion(thresh);
                } catch (NumberFormatException nfe) {
                    if (TuneCriterion.isStandardCriterion(sThresh)) {
                        m_criterion = TuneCriterion.getCriterion(sThresh);
                    } else {
                        Messages.postError("Bad value in setMatchThresh cmd: \""
                                           + sThresh + "\"");
                    }
                }
            }

        } else if (key.equalsIgnoreCase("Initialize")) {
            //displayStatus(STATUS_RUNNING);
            ChannelInfo chanInfo = getChannelInfo(m_channelNumber);
            //chanInfo.initializeChannelFromScratch();
            chanInfo.initializeChannel();
            //displayStatus(STATUS_READY);
        } else if (key.equalsIgnoreCase("Index")) {
            if (toker.hasMoreTokens()) {
                //displayStatus(STATUS_RUNNING);
                String smotor = toker.nextToken();
                if (smotor.equalsIgnoreCase("all")) {
                    for (int gmi = 0; gmi < MotorControl.MAX_MOTORS; gmi++) {
                        motorControl.indexMotor(gmi);
                    }
                } else {
                    try {
                        int gmi = Integer.parseInt(smotor);
                        motorControl.indexMotor(gmi);
                    } catch (NumberFormatException nfe) {
                        Messages.postError("Bad motor number in Index cmd: \""
                                           + smotor + "\"");
                    }
                }
                //displayStatus(STATUS_READY);
            }
        } else if (key.equalsIgnoreCase("Motor")) {
          if (toker.hasMoreTokens()) {
              //displayStatus(STATUS_RUNNING);
              String command = toker.nextToken("").trim();
              motorControl.sendToMotor(command);
              //displayStatus(STATUS_READY);
          }
        } else if (key.equalsIgnoreCase("MotorTest")) {
           motorTest();
        } else if (key.equalsIgnoreCase("SendToVnmr")) {
           if (toker.hasMoreTokens()) {
               sendToVnmr(toker.nextToken("").trim()); // Send rest of line
           }
        } else if (key.equalsIgnoreCase("exit")) {
            exit(m_tuneResults);
        } else if (key.equalsIgnoreCase("exitN")) {
            exit(m_tuneResults);
        } else if (key.equalsIgnoreCase("quitgui")) {
            exit();
        } else if (key.equalsIgnoreCase("findDip")) {
            if (m_reflectionData == null) {
                Messages.postError("ProbeTune.execute(" + cmd + "): no data");
            } else {
                m_reflectionData.findNextResonance(0);
            }
//        } else if (key.equalsIgnoreCase("RestorePFiles")) {
//            if (toker.hasMoreTokens()) {
//                String file= toker.nextToken();
//                file = file.toLowerCase();
//                int motorNum;
//                if(file.startsWith("chan")){
//                    //restore Chan# file
//                    motorNum = Integer.parseInt(file.substring(5));
//                    ChannelInfo chanInfo = getChannelInfo(motorNum);
//                    chanInfo.restorePersistence(motorNum);
//                    setReceived("RestorePFiles chan#" + motorNum);
//                } else {
//                    //restore Motor# file
//                    motorNum = Integer.parseInt(file.substring(6));
//                    MotorInfo motorInfo = getMotorInfo(motorNum);
//                    motorInfo.restorePersistence(motorNum);
//                    setReceived("RestorePFiles motor#" + motorNum);
//                }
//                Messages.postDebug("Persistence",
//                                   "RestorePersistence " + motorNum);
//            } else {
//                //restore all files
//                File sysFile = new File(getSysTuneDir());
//                File userFile = new File(ProbeTune.getVnmrUserDir()
//                                         + "/tune/" + getUsrProbeName());
//                try{
//                    TuneUtilities.copyDir(sysFile, userFile);
//                } catch (Exception e){
//                    Messages.postError("RestorePFiles failed: " + e);
//                }
//            }
        } else if (key.equalsIgnoreCase("TuneUpdate")) {
            if (toker.hasMoreTokens()) {
                m_tuneUpdate = toker.nextToken();
                //System.setProperty("apt.refPositionUpdating", m_tuneUpdate);
                setReceived("TuneUpdate " + m_tuneUpdate);
                Messages.postDebug("TuneUpdate", "Tune Update " + m_tuneUpdate);
            }
//            else {
//                m_tuneUpdate = System.getProperty("apt.refPositionUpdating");
//                Messages.postDebug("TuneUpdate", "Tune Update " + m_tuneUpdate);
//                setReceived("TuneUpdate " + m_tuneUpdate);
//            }
        }  else if (key.equalsIgnoreCase("saveTune")) {
            if (toker.hasMoreTokens()) {
                String sampleName= toker.nextToken();
                Messages.postDebug("PredefinedTune",
                                   "SampleName=" + sampleName);
                ChannelInfo ci = getChannelInfo(m_channelNumber);
                ci.saveCurrentTunePosition("samp" + sampleName);
                setReceived("saveTune " + sampleName);
                //}
            } else {
                Messages.postError("Usage: saveTune <sampleName>");
            }
        } else if (key.equalsIgnoreCase("maxStepSize")) {
            if (toker.hasMoreTokens()) {
                int maxStep= Integer.parseInt(toker.nextToken());
                m_maxStepSize= maxStep;
                setReceived("maxStepSize " + m_maxStepSize);
                Messages.postDebug("MaxStep", "Max Step=" + m_maxStepSize);
            }
        } else if (key.equalsIgnoreCase("execFile")) {
            try {
                String filepath = toker.nextToken("").trim();
                execFile(filepath);
            } catch (NoSuchElementException nsee) {
                Messages.postError("\"execFile\" command requires a "
                                   + "filepath argument");
            }
        } else if (key.equalsIgnoreCase("CollectCalData")) {
            try {
                String type = toker.nextToken().toLowerCase();
                collectCalData(type);
            } catch (NoSuchElementException nsee) {
                Messages.postError("Command requires a data-type argument: "
                                   + cmd);
            }
        } else if (key.equalsIgnoreCase("SaveAllCalData")) {
            saveCalData(toker.nextToken("").trim());
        } else if (key.equalsIgnoreCase("UseRefs")) {
            m_useRefPositions = !toker.nextToken().startsWith("0");
            sendOptionsToGui();
        } else if (key.equalsIgnoreCase("DisplayInfo")) {
            displayInfo();
        } else if (key.equalsIgnoreCase("calcSigma")) {
            String path = "data";
            try {
                path = toker.nextToken();
            } catch (NoSuchElementException e) {
            }
            TuneUtilities.calculateSigma(path);
        } else if (key.equalsIgnoreCase("debug")) {
            try {
                String category = "";
                String action = toker.nextToken().toLowerCase();
                if (toker.hasMoreTokens()) {
                    category = toker.nextToken();
                }
                if ("add".equals(action)) {
                    if (DebugOutput.addCategory(category)) {
                        Messages.postInfo("Debug category \"" + category
                                          + "\" added");
                    } else {
                        Messages.postWarning("Debug category \"" + category
                                          + "\" already active");
                    }
                } else if ("remove".equals(action)) {
                    if (DebugOutput.removeCategory(category)) {
                        Messages.postInfo("Debug category \"" + category
                                          + "\" removed");
                    } else {
                        Messages.postWarning("Debug category \"" + category
                                          + "\" was not active");
                    }
                } else if ("list".equals(action)) {
                    String categories = DebugOutput.getCategories();
                    Messages.postInfo("Active Debug Categories: " + NL
                                      + categories + "-----------");
                }
            } catch (NoSuchElementException nsee) {
                Messages.postError("Bad 'debug' command: \"" + cmd + "\"");
            }
        } else {
            Messages.postError("ProTune: unknown command \"" + cmd + "\"");
        }

        ChannelInfo chanInfo = getChannelInfo(m_channelNumber);
        Messages.postDebug("Persistence",
                           "About to write Persistence. Check chan info");
        if (chanInfo != null) {
            if (ChannelInfo.isAutoUpdateOk()) {
                chanInfo.writePersistence();
            }
            if (MotorControl.isAutoUpdateOk()) {
                motorControl.writePersistence();
            }
        } else {
            Messages.postError("Could not Write Persistence.");
        }

        Messages.postDebug("Exec", "ProbeTune: executed \"" + cmd + "\"");
    }

    private void collectCalData(String type) throws InterruptedException {
        if (!(sweepControl instanceof MtuneControl)) {
            // Network Analyzer doesn't do this
            return;
        }
        MtuneControl mtuneControl = (MtuneControl)sweepControl;
        String key = "apt.nCalsAveraged";
        int nScans = (int)TuneUtilities.getDoubleProperty(key, 9);
        if ("probe".equals(type)) {
            nScans = 1;
        }
        ChannelInfo chInfo = getChannelInfo(m_channelNumber);

        if (getCalDataIndex(type) == 0) {
            // First calibration type; optimize power and gain
            doAutogain(mtuneControl, chInfo);
        }

        ReflectionData[] data = new ReflectionData[nScans];
        try {
            for (int i = 0; i < nScans; i++) {
                data[i] = mtuneControl.getData(chInfo, -1);
                m_reflectionData = ReflectionData.average(data, i + 1);
                displayData(m_reflectionData);
            }
            setCalData(type, m_reflectionData);
            if ("open".equals(type)) {
                // Calculate and save the relative S/N for this scan.
                setCalData("sigma", ReflectionData.getRelSigma(data, nScans));
            }
            sendToGui("calDataSet");
        } catch (Exception e) {
            sendToGui("calDataSet failed");
        }
    }

    private void doAutogain(MtuneControl sweep, ChannelInfo chInfo)
        throws InterruptedException {

        //ArrayList<GainInfo> gainInfos = new ArrayList<GainInfo>(0);
        ReflectionData data = sweep.getData(chInfo, -1);
        int power = sweep.getCurrentPower();
        int gain = sweep.getCurrentGain();
        int rfchan = data.getRfchan();
//        if (sweep instanceof MtuneControl) {
//            rfchan = ((MtuneControl)sweep).getRfchan();
//        }
        double maxVal = data.getMaxAbsval();
        //gainInfos.add(new GainInfo(power, gain, maxVal));
        int maxPower = PowerLimits.getMaxPower(rfchan);
        int minPower = PowerLimits.getMinPower(rfchan);
        int stepPower = Math.max(1, PowerLimits.getStepPower(rfchan));
        int maxGain = PowerLimits.getMaxGain();
        int minGain = PowerLimits.getMinGain();
        int stepGain = Math.max(1, PowerLimits.getStepGain());
        if (power > maxPower || power < minPower
            || gain > maxGain || gain < minGain)
        {
            // Power or gain limits have changed since last calibration
            power = Math.max(minPower, Math.min(maxPower, power));
            gain = Math.max(minGain, Math.min(maxGain, gain));
            sweep.setPowerAndGain(power, gain, rfchan);
            data = sweep.getData(chInfo, -1);
            displayData(data);
            maxVal = data.getMaxAbsval();
        }
        double maxSig = PowerLimits.getMaxOkSignal();
        double minSig = PowerLimits.getMinOkSignal();
        Messages.postDebugInfo("doAutogain: minOKSig=" + minSig
                               + ", maxOKSig=" + maxSig);
        Messages.postDebugInfo("doAutogain: pwr=" + power + ", gain=" + gain);
        for (int i = 0; i < 9 && maxVal < minSig || maxVal > maxSig; i++) {
            double opt = (minSig + maxSig) / 2;
            // How much we have to change, in dB:
            int dbDiff = (int)Math.round(20 * Math.log10(opt / maxVal));
            Messages.postDebugInfo("doAutogain: pwr=" + power
                                   + ", gain=" + gain
                                   + ", maxVal=" + (int)maxVal
                                   + ", dbDiff=" + dbDiff);
            if (dbDiff == 0) {
                // Don't expect this, but if minSig ~= maxSig ...
                break;
            } else if (dbDiff > 0) {
                // Need more power or gain
                if (power / stepPower == maxPower / stepPower
                    && gain / stepGain == maxGain / stepGain)
                {
                    // Already as close as we can get
                    Messages.postDebugInfo("doAutogain: signal below optimum"
                                           + " by < 1 gain/power step");
                    break;
                }
                // Prefer to turn up power, before gain
                int dpwr = Math.min(maxPower - power, dbDiff);
                dpwr = stepPower * (dpwr / stepPower);
                dbDiff -= dpwr;
                power += dpwr;
                if (power == maxPower) {
                    int dgain = Math.min(maxGain - gain, dbDiff);
                    dgain = stepGain * (dgain / stepGain);
                    dbDiff -= dgain;
                    gain += dgain;
                }
            } else { // dbDiff < 0
                // Need less power or gain
                if (power / stepPower == minPower / stepPower
                    && gain / stepGain == minGain / stepGain)
                {
                    // Already as close as we can get
                    Messages.postDebugInfo("doAutogain: signal above optimum"
                                           + " by < 1 gain/power step");
                    break;
                }
                // Prefer to turn down gain, rather than power
                int dgain = Math.max(minGain - gain, dbDiff);
                dgain = stepGain * (dgain / stepGain);
                dbDiff -= dgain;
                gain += dgain;
                if (gain == minGain) {
                    int dpwr = Math.max(minPower - power, dbDiff);
                    dpwr = stepPower * (dpwr / stepPower);
                    dbDiff -= dpwr;
                    power += dpwr;
                }
            }
            sweep.setPowerAndGain(power, gain, rfchan);
            data = sweep.getData(chInfo, -1);
            displayData(data);

            power = sweep.getCurrentPower();
            gain = sweep.getCurrentGain();
            maxVal = data.getMaxAbsval();
        }
        Messages.postDebugInfo("doAutogain done: pwr=" + power
                               + ", gain=" + gain + ", maxVal=" + (int)maxVal);
    }

    private static int getCalDataIndex(String type) {
        int n = -1;
        if ("open".equals(type)) {
            n = 0;
        } else if ("load".equals(type)) {
            n = 1;
        } else if ("short".equals(type)) {
            n = 2;
        } else if ("probe".equals(type)) {
            n = 3;
        } else if ("sigma".equals(type)) {
            n = 4;
        }
        return n;
    }

    private void setCalData(String type, ReflectionData data) {
        m_calData[getCalDataIndex(type)] = data;
    }

    private void saveCalData(String spec) throws InterruptedException {
        if (sweepControl instanceof MtuneControl) {
            MtuneControl mtControl = (MtuneControl)sweepControl;
            resetChannelPhases();
            ReflectionData data = mtControl.measureProbeDelay(m_calData);
            double delay = data.getProbeDelay();
            Interval exclude = new Interval(-1, -1);
            ReflectionData.Dip dip = data.getDips()[0];
            if (dip != null) {
                Messages.postDebug("RFCal", "saveCalData: "
                                   +" dip in probe reference data at "
                                   + Fmt.f(2, dip.getFrequency() / 1e6)
                                   + " MHz");
                exclude = dip.getPerturbedInterval();
            }
            Messages.postDebug("RFCal", "saveCalData:"
                               + " delay=" + Fmt.f(3, delay)
                               + ", exclude=" + exclude
                               + ", spec=" + spec
                               );
            mtControl.writeCalData(m_calData, spec, delay, exclude);
        }
    }

    private void execFile(String filepath) {
        BufferedReader in = TuneUtilities.getReader(filepath);
        if (in != null) {
            try {
                String line = null;
                while ((line = in.readLine()) != null) {
                    line = line.trim();
                    if (!line.startsWith("#")) {
                        displayCommand(line);
                        exec(line);
                    }
                }
            } catch (IOException ioe) {
                Messages.postError("Problem parsing command file: \""
                                   + filepath + "\"");
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }
        }
    }

    private void postCommandError(String cmd) {
        Messages.postError("Bad command to ProbeTune: " + cmd);
    }

    public static boolean isCancel() {
        return m_cancel;
    }

    private static void setCancel(boolean b) {
        m_cancel = b;
    }

    private void motorTest() throws InterruptedException {
        int gmi = 0;
        int dir = -500;
        while (!isCancel()) {
            if (gmi == 0) {
                dir *= -1;
            }
            if (motorControl.isValidMotor(gmi)) {
                motorControl.sendToMotor("step " + gmi + " " + dir);
                Thread.sleep(1000);
            }
            gmi = (gmi + 1) % motorControl.getNumberOfMotors();
        }
    }

    /**
     * Finds the appropriate channel for the given frequency.
     * A centering parameter is calculated that tells how close the
     * given frequency is to the center of the channel. A value of 0
     * means the frequency is in the center of the channel, and -1 or +1
     * mean it is at the lower or upper boundary, respectively.
     * If the current channel includes the given frequency
     * (abs(centering parameter) <= 1)
     * and m_favorCurrentChannel is true, 
     * the current channel is returned.
     * Otherwise, chooses the channel with the smallest centering parameter.
     * If the absolute value of the returned centering parameter
     * is greater than 1, we cannot tune to the given frequency.
     * @param freq Frequency to tune to (Hz).
     * @return The channel number to use and it's centering parameter.
     */
    private ChannelAlignment findChannel(double freq)
        throws InterruptedException {
        ChannelInfo ci;
        int bestChan = -1;
        double bestCentering = 99;

        if (m_favorCurrentChannel) {
            // See if current channel is OK
            ci = getChannelInfo(m_channelNumber);
            if (ci != null) {
                double center = ci.getCenterTuneFreq();
                double span = ci.getSpanTuneFreq();
                bestCentering = (freq - center) / (span / 2);
                bestChan = m_channelNumber;
            }
        }

        if (Math.abs(bestCentering) > 1) {
            // Current channel no good, look for a channel to use
            for (ChannelInfo cii : getChannelInfos()) {
                if (cii != null) {
                    double center = cii.getCenterTuneFreq();
                    double span = cii.getSpanTuneFreq();
                    double centering = (freq - center) / (span / 2);
                    if (Math.abs(bestCentering) > Math.abs(centering)) {
                        bestCentering = centering;
                        bestChan = cii.getChannelNumber();
                    }
                } else {
                    break;
                }
            }
        }
        return new ChannelAlignment(bestChan, bestCentering);
    }

    /**
     * Move motors to a remembered reference position for the given frequency.
     *
     * @param freq The desired frequency (Hz).
     * @return True if motors are now at an appropriate reference position.
     * @throws InterruptedException If command is cancelled.
     */
    private boolean goToRef(double freq) throws InterruptedException {
        boolean ok = false;
        ChannelAlignment bestChannelAlignment = findChannel(freq);
        if (Math.abs(bestChannelAlignment.centeringParameter) <= 1) {
            int channel = bestChannelAlignment.iChannel;
            if (channel >= 0) {
                ChannelInfo chanInfo = getChannelInfo(channel);
                if (chanInfo != null) {
                    double freqTol = 2e6; // Hz
                    ok = chanInfo.goToSavedPosition(freq, freqTol);
                } else {
                    Messages.postError("Channel " + channel + " not present");
                }
            }
        }
        return ok;
    }

    /**
     * Tune to a given frequency.
     * First finds which channel to use and switches to that channel.
     * @param target The target to tune to.
     * @param useref If true, use any available reference motor positions.
     */
    private TuneResult tune(TuneTarget target, boolean useref)
                            throws InterruptedException {
        double match_dB = Math.log10(getTargetMatch()) * 10;
        Messages.postDebug("TuningSummary",
                           "Tuning to " + target.getFreq_str()
                           + " with match < " + Fmt.f(1, match_dB) + " dB"
                           + ", rfchan=" + target.getRfchan()
                           + ", nucleus=" + target.getNucleus());

        ChannelAlignment bestChannelAlignment = findChannel(target.getFreq_hz());

        long startTime = System.currentTimeMillis();
        boolean tuned = false;
        if (Math.abs(bestChannelAlignment.centeringParameter) <= 1) {
            int ichan = bestChannelAlignment.iChannel;
            m_padAdj = 1; // Standard step damping
            int metaTries;
            for (metaTries = 0; !tuned && metaTries < 4; metaTries++) {
                tuned = tune(ichan, target, useref);
                useref = false; // Only go to ref position once!
                if (isCancel()) {
                    Messages.postDebug("TuningSummary", "Tuning ABORTED");
                    String msg = "Tuning was aborted by the user";
                    m_tuneStatus = new TuneResult("warning", msg);
                    break;
                }
                if (!tuned) {
                    Messages.postDebug("TuneAlgorithm", "Meta-try #"
                                       + (metaTries + 1) + " failed");
                }
            }
            double time = (System.currentTimeMillis() - startTime) / 1000.0;
            String try_ies = metaTries > 1 ? " tries" : " try";
            Messages.postDebug("TuningSummary", "ProbeTune.tune: " + metaTries
                               + try_ies + ", Total time " + Fmt.f(1, time)
                               + " s" + NL + "    ... Result: "
                               + m_tuneStatus.toString("db"));
        } else {
            Messages.postError("Frequency not in range of any channel: "
                               + target.getFreq_str());
            String msg = target.getFrequencyName()
                         + " is not tunable on this probe";
            m_tuneStatus = new TuneResult("warning", msg);
            tuned = false;
        }
        displayTuneStatus();
        /*System.err.println("ProbeTune.tune: DONE: " + m_tuneStatus);/*DBG*/
        return m_tuneStatus;
        //return m_tuneResult;
    }

    private int dampStep(double calcSteps) {
        int limit = 10;
        // TODO: Use SDV values of sensitivities to choose how far we step
        // towards the target.
        double damping = m_stepDamping * m_padAdj;
        if (Math.abs(calcSteps) < limit) {
            // If we're closer than this limit - more damping
            damping = 1 - (1 - damping) * Math.abs(calcSteps) / limit;
        }
        int nSteps = (int)Math.round(damping * calcSteps);
        int absNSteps = Math.abs(nSteps);
        if (absNSteps > m_maxStepSize) {
            nSteps = nSteps > 0 ?  m_maxStepSize : -m_maxStepSize;
        } else if (absNSteps < limit) {
            // If we're below the limit now - still more damping
            // TODO: Combine this with above "< limit" adjustment?
            double pad = absNSteps / (double)limit;
            nSteps = (int)Math.round(pad * nSteps);
        }
        if (nSteps == 0 && Math.abs(calcSteps) > 0.5) {
            nSteps = (int)Math.copySign(1, calcSteps);
        }
        Messages.postDebug("DampStep","dampStep: calcSteps="
                           + Fmt.f(1, calcSteps) + ", nSteps=" + nSteps);
        return nSteps;
    }

    /**
     * Tune to a given frequency on a given tune channel.
     * @param channel The tune channel number to use.
     * @param target The target to tune to.
     * @param useref If true, use any available reference motor positions.
     * @return True if we are done tuning.
     */
    private boolean tune(int channel, TuneTarget target, boolean useref)
                         throws InterruptedException {

        long time0 = System.currentTimeMillis();
        boolean done = false;
        double freq = target.getFreq_hz();
        boolean isNewChannel = channel != m_channelNumber;
        setChannel(channel, 0, true); // TODO: Why is rfchan 0, rather than target.getRfchan()?
        ChannelInfo chanInfo = getChannelInfo(channel);
        m_nTuneMotors = chanInfo.getNumberOfMotors();
        chanInfo.setTuneStepDistance(Double.NaN);

        m_tuneStatus = new TuneResult("failed");

        Messages.postDebug("AllTuneAlgorithm", "ProbeTune.tune(chan="
                           + channel + ", freq="
                           + target.getFreq_str() + ")"
                           + "   np=" + getNp()
                           + ", isNewChannel=" + isNewChannel);

        Messages.postDebug("AllTuneAlgorithm", "ProbeTune.tune: "
                           + "change channel to " + channel);
        if (!chanInfo.isChannelReady()) {
            Messages.postError("ProbeTune.tune: channel " + channel
                               + " not ready.");
            String msg = "tune channel #" + channel + " ("
            + target.getFreq_str() + ") is not ready";
            m_tuneStatus = new TuneResult("failed", msg);
            return false;
        }
        Messages.postDebug("TuneAlgorithm",
                           "Switching to channel " + channel
                           + " and setting sweep range");
        setChannel(channel, target.getRfchan(), true); // Must change sweep

        if (freq < chanInfo.getMinTuneFreq()
            || freq > chanInfo.getMaxTuneFreq())
        {
            Messages.postError("Requested frequency, "
                               + target.getFreq_str() + ", out of range: "
                               + Fmt.f(3, chanInfo.getMinFreq() / 1e6) + " to "
                               + Fmt.f(3, chanInfo.getMaxFreq() / 1e6));
            String msg = target.getFreq_str()
                         + " is out of the range of tune channel #"
                         + channel;
            m_tuneStatus = new TuneResult("failed", msg);
            return false;
        }

        m_targetFreq = freq;
        displayTargetFreq(freq, channel);
        //chanInfo.initializeChannel(false);

        chanInfo.setSwitchToFreq(freq);

        //TuneInfo desiredTune = new TuneInfo(freq, target.getMatch_refl());

        TuneInfo ti = chanInfo.getFreshTunePosition(0, false);

        if (!chanInfo.isSignalDetected()) {
            exit("failed - RF signal is too weak on tune chan#" + channel);
            return false;
        }

        if (!getMotorControl().isConnected()
            && !chanInfo.isMatchBetterThan(target))
        {
            exit("failed - No communication with tune motors");
            return false;
        }

//        if (ti == null) {
//            String msg = "cannot get position of tuning resonance in channel #"
//                         + channel;
//            m_tuneStatus = new TuneResult("failed", msg);
//            return false;
//        }
        double refl = (ti == null) ? 1.0 : ti.getReflection();

        measureTune(chanInfo, 00, false);
        if (!Double.isNaN(m_lowerCutoffFreq)
            || !Double.isNaN(m_upperCutoffFreq))
        {
            // NB: This is NOT a normal case
            Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                               + "A CUTOFF FREQ IS SET...");
            if (!chanInfo.isMatchBetterThan(target)) {
                boolean atRef = false;
                if (useref) {
                    atRef = chanInfo.goToSavedPosition(freq,
                                                       ti,
                                                       m_lowerCutoffFreq,
                                                       m_upperCutoffFreq,
                                                       200e3,
                                                       3);
                }
                ti = chanInfo.getMeasuredTunePosition();

                if (atRef && ti != null && chanInfo.isDipDetected()) {
                    Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                                       + "Prepare for tuning--"
                                       + "Went to reference position");
                } else if (ti != null && chanInfo.isDipDetected()) {
                    // Set sweep to show from current position to dst
                    Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                                       + "Prepare for tuning--"
                                       + "Set sweep to show "
                                       + Fmt.f(3, freq / 1e6));
                    setSweepToShow(ti.getFrequency(), freq);
                } else {
                    Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                                       + "Prepare for tuning--"
                                       + "No dip detected--Set full sweep");
                    setFullSweep();
                }
            }
        } else if (ti == null
                   || !chanInfo.isDipDetected()
                   || Math.abs(ti.getFrequency() - freq) > 10e6
                   || Math.abs(refl) > 0.3)
        {
            // We're far from tuned, or we don't see a dip
            boolean atRef = false;
            if (useref) {
                atRef = chanInfo.goToSavedPosition(freq,
                                                   ti,
                                                   m_lowerCutoffFreq,
                                                   m_upperCutoffFreq,
                                                   200e3,
                                                   3);
            }
            ti = chanInfo.getMeasuredTunePosition();
            if (atRef && ti != null && chanInfo.isDipDetected()){
                Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                                   + "Prepare for tuning--"
                                   + "Went to reference position");
            } else if (ti != null && chanInfo.isDipDetected()) {
                // Set sweep to show from current position to dst
                Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                                   + "Prepare for tuning--"
                                   + "Set sweep to show "
                                   + Fmt.f(3, freq / 1e6));
                setSweepToShow(ti.getFrequency(), freq);
            } else {
                // We're in an unknown position
                Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                                   + "Prepare for tuning--"
                                   + "Freq is unknown--Set full sweep");
                chanInfo.setFullSweep();
            }
        }
        measureTune(chanInfo, 00, true);

        // Make sure we see the dip before trying to tune iteratively
        if (!chanInfo.searchForDip(target)) {
            m_tuneStatus = new TuneResult("failed", "cannot find a dip");
            return false;
        }

        ti = chanInfo.getMeasuredTunePosition();
        refl = ti.getReflection();

        // Set sweep to show from current position to dst
        Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                           + "Set sweep to show "
                           + Fmt.f(3, freq / 1e6));
        setSweepToShow(ti.getFrequency(), freq);

        //double reflectTol = Math.sqrt(matchTol);
        //if (Math.abs(match) > reflectTol) {
        //    // Step the match motor to confirm the sign
        //    chanInfo.measureMatchSign(reflectTol);
        //}

        for (int i = 0; i < m_nTuneMotors; i++) {
            m_nReverses[i] = 0;
        }

        // Go into the iterative tuning loop
        chanInfo.clearStuckMotors();
        chanInfo.clearEotMotors();
        Messages.postDebug("AllTuneAlgorithm", "ProbeTune.tune: "
                           + "Enter top level tuning loop");
        int totalItrs = 0;
        final int MAX_META_TRIES = 2;
        int itrs2;
        for (itrs2 = 0; itrs2 < MAX_META_TRIES; ) {
            itrs2++; // Increment here so count is ok if we break out

            // Check if we lost sweep
            if (sweepControl instanceof MtuneControl
                && !((MtuneControl)sweepControl).isSweepOk())
            {
                done = true;
                m_tuneStatus = new TuneResult("failed",
                                              "lost sweep communication");
                break;
            }

            // Check if we lost motor
            if (!(motorControl.isConnected())) {
                done = true;
                m_tuneStatus = new TuneResult("failed",
                                              "lost motor communication");
                break;
            }


            if (itrs2 > 1) {
                // After a failure, reset channel sensitivities
                //chanInfo.initializeChannel(true);
                Messages.postDebug("TuneAlgorithm", "ProbeTune.tune: "
                                   + "RESETTING SENSITIVITIES to "
                                   + "system values; adding damping.");
                chanInfo.resetSensitivity();
                m_padAdj = 0.5; // Increased step damping
            }

            int itrs;
            Messages.postDebug("AllTuneAlgorithm",
                                "ProbeTune.tune: Enter tuneStep() loop");
            // Step towards target until we get there or go past it:
            for (itrs = 0; tuneStep(chanInfo, target, itrs); itrs++) {}
            Messages.postDebug("TuneAlgorithm",
                               "ProbeTune.tune: tuneStep() did "
                               + itrs + " iterations");

            if (chanInfo.getStuckMotorNumber() >= 0) {
                int n = chanInfo.getMotorNumber(chanInfo.getStuckMotorNumber());
                Messages.postError("MOTOR # " + n
                                   + " DOES NOT SEEM TO BE DOING ANYTHING");
                // NB: only fatal if we're on the last try
                // NB: itrs2 is incremented at top of this loop
                if (itrs2 == MAX_META_TRIES) {
                    done = true;
                    String msg = "motor #" + n + " having no effect";
                    m_tuneStatus = new TuneResult("failed", msg);
                }
                break;
            }
//            if (chanInfo.getEotMotorNumber() >= 0) {
//                int n = chanInfo.getMotorNumber(chanInfo.getEotMotorNumber());
//                if (n == chanInfo.getMatchMotor()) {
//                    ProbeTune.printErrorMessage(2, "MATCH ");
//                    m_tuneStatus = "failed - match motor ("+n+") at end of travel";
//                } else {
//                    ProbeTune.printErrorMessage(2, "TUNE ");
//                    m_tuneStatus = "failed - tune motor ("+n+") at end of travel";
//                }
//                printlnErrorMessage(2, "MOTOR IS AT THE END OF ITS TRAVEL");
//                break;
//            }
            totalItrs += 1 + itrs;
            if (chanInfo.isMatchBetterThan(target)) {
                done = true;
                break;
            } else if (chanInfo.getNumberOfMovableMotors() == 0) {
                done = true;
//                m_tuneStatus = "failed -- All motors at end of travel?";
//                m_allMotorsAtLimits = true;
                break;
            } else if (chanInfo.getNumberOfMovableMotors() == 1) {
                // Either we have only one motor to begin with or one
                // motor is at the EOT.
                // With one motor, can't both tune and match, so we just
                // optimize the one motor. (In this case, we just go for
                // the best possible match, or any match better than the spec.
                // if we happen to see that.)
                Messages.postDebug("TuneAlgorithm",
                                    "Single motor check: itrs=" + itrs
                                    + ", totalItrs=" + totalItrs
                                    + ", reverses=" + m_nReverses[0]
                                    + ", tuneDistance="
                                    + chanInfo.getTuneStepDistance()
                                    );
                if (chanInfo.getTuneStepDistance() < 1) {
                    done = true;
                    break;
                }
            }
        }

        // Get the match result
        double matchPwr = chanInfo.getMatchAtRequestedFreq(target);
        double matchDb = 10 * Math.log10(matchPwr);

        if (done) {
            // Set the status message
            boolean ok = matchPwr < target.getCriterion().getAccept_pwr();
            if (ok) {
                // If we're done and the match is OK, status is "ok", even
                // if there were other serious errors.
                m_tuneStatus = new TuneResult("ok");
                /*System.out.println("ProbeTune.tune: "
                + "call saveCurrentTunePosition");/*DBG*/
                // Save this position for future use
//                m_tuneUpdate = System.getProperty("apt.refPositionUpdating");
                if (m_tuneUpdate.equalsIgnoreCase(UPDATING_ON)) {
                    chanInfo.saveCurrentTunePosition(null);
                }
            }
        } else {

            //if tuning failed to find a dip put back to Ref line
            if (!chanInfo.isDipDetected()){
                Messages.postDebug("TuneAlgorithm",
                                   "Could not tune.  Sending to Ref line.");
                chanInfo.goToSavedPosition(freq,
                                           ti = isNewChannel ? null : ti,
                                           m_lowerCutoffFreq,
                                           m_upperCutoffFreq,
                                           200e3, 1, false);
            }

            if (!chanInfo.isDipDetected()) {
                if (!chanInfo.retrieveDip()) {
                    chanInfo.searchForDip(target);
                }
            }

            // ... We may try some more - this message may be overwritten.
            String msg = "tuning " + Fmt.f(3, freq/1e6)
                         + " MHz: no convergence after " + totalItrs
                         + " iterations";
            m_tuneStatus = new TuneResult("failed", msg);
            m_tuneStatus.setTarget(target);

            if (!chanInfo.isDipDetected()) {
                Messages.postError("ProTune has lost the dip");
                return(true);
            }
        }

        // Put the match in the status message
        m_tuneStatus.setTarget(target);
        m_tuneStatus.setActualMatch_db(matchDb);

        // Add any EOT messages to the status message
        double[] steps = chanInfo.calcStepsToTune(target.getFreq_hz());
        chanInfo.setEotValues(steps);
        for (int cmi = 0; cmi < m_nTuneMotors; cmi++) {
            if (chanInfo.isAtLimit(cmi)) {
                int gmi = chanInfo.getMotorNumber(cmi);
                Messages.postDebug("TuneAlgorithm",
                                   "Motor " + gmi + " is at End Of Travel");
                m_tuneStatus.setEotMotor(gmi);
            }
        }

        // Print results to debug only - not necessarily the final result
        double time = (System.currentTimeMillis() - time0) / 1000.0;
        Messages.postDebug("AllTuneAlgorithm",
                           "Meta-step time: " + Fmt.f(0, time) + "s"
                           + ", " + itrs2 + " tries (" + totalItrs
                           + " \"tuneStep\" iterations)");

        return done;
    }

    private void trackTune(int nSeconds) throws InterruptedException {
        //displayStatus(STATUS_RUNNING);
        ChannelInfo ci = getChannelInfo(m_channelNumber);
        for (int i = 0; i < nSeconds; i++) {
            TuneInfo ti = measureTune(ci, -1, true); // Measure new data
            if (ti != null) {
                ti.getFrequency();
            }
            if (isCancel()) {
                //displayStatus(STATUS_READY);
                return;
            }
            Thread.sleep(100); /*TRACK*/
        }
        //displayStatus(STATUS_READY);
    }

    private void setCalSweep(double freq0, double freq1, int rfchan)
                            throws InterruptedException {
        //displayStatus(STATUS_RUNNING);
        if (freq0 >= freq1) {
            return;
        }
        if (sweepControl instanceof MtuneControl) {
            ((MtuneControl)sweepControl).initializeForChannel(null, rfchan);
        }
        freq0 = MtuneControl.adjustCalSweepStart(freq0);
        freq1 = MtuneControl.adjustCalSweepStop(freq1);
        int np = MtuneControl.getCalSweepNp(freq0, freq1);
        double center = (freq1 + freq0) / 2;
        double span = freq1 - freq0;
        sweepControl.setExactSweep(center, span, np, rfchan);
        //displayStatus(STATUS_READY);
    }

//    public ReflectionData getData(ChannelInfo ci, int delay_ms)
//        throws InterruptedException {
//
//        //displayStatus(STATUS_RUNNING);
//        ReflectionData d =sweepControl.getData(ci, delay_ms);
//        //displayStatus(STATUS_READY);
//        return d;
//    }

    public boolean setSweepToShow(double f0, double f1)
            throws InterruptedException {
        final double MIN_SWEEP = 10e6;

        boolean changed = false;
        double center = getCenter();
        double span = getSpan();
        double start = center - span / 2;
        double stop = center + span / 2;
        double fmin = Math.min(f0, f1);
        double fmax = Math.max(f0, f1);

        // Don't sweep a ridiculously small range
        if (fmax - fmin < MIN_SWEEP) {
            center = (fmax + fmin) / 2;
            fmax = center + MIN_SWEEP / 2;
            fmin = center - MIN_SWEEP / 2;
        }

        if (fmin - start < span / 10 || stop - fmax < span / 10) {
            center = (fmax + fmin) / 2;
            span = (fmax - fmin) * 1.4;
            int rfchan = getChannelInfo(m_channelNumber).getRfchan();
            setSweep(center, span, 0, rfchan);
            //setCenter(center);
            //setSpan(span);
            changed = true;
            //sweepControl.getData(center, span, np);
        }
        return changed;
    }

    /**
     * Do one iteration of stepping towards the desired tune point.
     * May (or may not) move all the motors that affect the specified
     * channel.
     * @return True if tuning is still in progress (more to do).
     * Returns false if tuning is done (may have failed).
     */
    private boolean tuneStep(ChannelInfo chanInfo, TuneTarget target, int itr)
        throws InterruptedException {

        double freq = target.getFreq_hz();
        boolean motion = false;
        if (isCancel()) {
            return motion;
        }
        Messages.postDebug("AllTuneAlgorithm", "ProbeTune.tuneStep() starting");

        // Go through all the motors
        for (int cmi = 0; cmi < chanInfo.getNumberOfMotors(); cmi++) {
            MotorInfo mInfo = chanInfo.getMotorInfo(cmi);
            boolean isMatchMotor = chanInfo.isMatchMotor(cmi);

            // Measure current position
            ReflectionData data = sweepControl.getData(chanInfo, 00, freq);
            if (data == null) {
                break;// TODO: Need failure message?
            }

            // Quit right now if we're good enough
            if (chanInfo.isMatchBetterThan(target)) {
                Messages.postDebug("TuningAlgorithm",
                                   "ProbeTune.tuneStep: done -- match good");
                return false;
            }

            setMeasuredTune(chanInfo, data);
            TuneInfo tuneInfo = chanInfo.getMeasuredTunePosition();
            double dipRefl = tuneInfo.getReflection();

            //Special check to take care of cutoff frequencies. This will
            // force the motor to go back to a specific reference location and
            // start again if somehow the dip is dropped outside the cutoffs
            if ( !Double.isNaN(m_upperCutoffFreq) ) {
                // Expanded the tolerence from 25kHz to 125Hz. This
                // will cause the motor easier to detect a
                // close-to-cutoff situation, thus move out more
                // often. Although it might seems to have negative
                // effect of resetting the start position more often,
                // the second condition of current reflection must
                // larger than 3 times the target reflection will
                // provide some safety pad to avoid too often move
                // back. In addition this expansion may take care of
                // not enough resolution problem when the dip gets
                // close to the cutoff.
                double targetRefl = target.getCriterion().getTarget_refl();
                if (((m_upperCutoffFreq - tuneInfo.getFrequency()) < 125e3)
                    && (tuneInfo.getReflection() > (targetRefl * 3)) )
                {
                    chanInfo.goToSavedPosition(freq,
                                               tuneInfo,
                                               m_lowerCutoffFreq,
                                               m_upperCutoffFreq,
                                               200e3, 1, false);
                    Messages.postDebug("TuneAlgorithm",
                                       "ProbeTune.tuneStep: "
                                       + "moved to reference position");
                    return true;
                }
            }

            if (!chanInfo.isDipDetected()) {
                Messages.postDebug("TuneAlgorithm", "Lost the dip");
                if (!chanInfo.retrieveDip()) {
                    Messages.postDebug("TuneAlgorithm",
                                       "Failed to retrieve dip");
                    return false; // quit
                }
            }

            // Calculate steps to reach the target
            double[] steps = chanInfo.calcStepsToTune(target.getFreq_hz());
            if (steps == null) {
                Messages.postError("ProbeTune.tuneStep: step calculation failed"
                                   + " - reseting sensitivities to sys values");
                chanInfo.resetSensitivity();
                return true; // Keep going
            } else {
                Messages.postDebug("AllTuneAlgorithm", "cmi=" + cmi
                                   + ", steps="
                                   + Fmt.f(2, steps, false, false));
            }

            // If all (double)steps are zero, either we are already there
            // or all the motors are at their limits.
            boolean allStepsZero = true;
            for (int i = 0; i < steps.length; i++) {
                if (steps[i] != 0) {
                    allStepsZero = false;
                    break;
                }
            }
            if (allStepsZero) {
                m_tuneStatus.setNote("as close as we can get");
                return false;
            }

            // See if we're asking for the impossible (motor past EOT)
            chanInfo.setAtLimit(cmi, mInfo.isAtLimit(steps[cmi]));
            if (chanInfo.isAtLimit(cmi)) {
                Messages.postDebug("TuneAlgorithm",
                                   "Motor " + chanInfo.getMotorNumber(cmi)
                                   + " is at end of travel");
                // NB: We may be able to get there with just the other motor
                if (chanInfo.getNumberOfMovableMotors() == 0) {
                    // It's hopeless -- all motors at limits of travel
                    Messages.postWarning("All tuning motors at end of travel");
                    // Don't add this message: already have 1 or 2 EOT messages
                    //m_tuneStatus += " -- all motors at limits of travel";
                    return false; // We're done
                }
            }

            // If another motor has to go much farther, skip this one.
            int pad = Math.min(5, Math.max(2, itr / 2));
            double targetRefl = getTargetReflection();
            double maxStep = TuneUtilities.getMaxAbs(steps);
            int matchMotor = chanInfo.getMatchMotorChannelIndex();
            double matchStep = matchMotor >= 0 ? steps[matchMotor] : 0;
            boolean skipMatch = Math.abs(dipRefl) < targetRefl / pad
                    && chanInfo.getNumberOfMovableMotors() > 1
                    && Math.abs(matchStep) < maxStep;
            if (skipMatch && !isMatchMotor) {
                // Recalculate tune and match steps based on fixed match motor
                steps[chanInfo.getMatchMotorChannelIndex()] = 0; // TODO: ???
                steps[cmi] = chanInfo.calcSingleMotorStepsToTune(cmi,
                                                                 target.getFreq_hz());
            }

            double thisStep = Math.abs(steps[cmi]);
            boolean shortDist = (maxStep / thisStep > STEP_RATIO
                                 && chanInfo.getNumberOfMovableMotors() > 1);

            if (Math.round(thisStep) == 0) {
                Messages.postDebug("TuneAlgorithm", "ProbeTune.tuneStep: "
                                   + "Skip motor "
                                   + chanInfo.getMotorNumber(cmi)
                                   + ": step is 0");
                if (m_nTuneMotors < 2) {
                    // We must be done.
                    Messages.postDebug("TuneAlgorithm", "ProbeTune.tuneStep: "
                                        + "tuneStep rtn false: "
                                        + "Zero step with single motor");
                    return false;
                }
            } else if (shortDist && (isMatchMotor || !skipMatch)) {
                Messages.postDebug("TuneAlgorithm", "ProbeTune.tuneStep: "
                                   + "Skip motor "
                                   + chanInfo.getMotorNumber(cmi)
                                   + ": other motor has much farther to go: "
                                   + Math.round(maxStep) + "/"
                                   + Math.round(thisStep));

            //} else if (unimptStep) {
            //    Messages.postDebug("TuneAlgorithm", "Skip motor " + cmi
            //                       + ": other step is way big: " + maxStep);

            } else if (skipMatch && isMatchMotor) {
                Messages.postDebug("TuneAlgorithm", "ProbeTune.tuneStep: "
                                   + "Skip motor "
                                   + chanInfo.getMotorNumber(cmi)
                                   + ": match is OK");
            } else if (!chanInfo.isMatchBetterThan(target)) {
                // We want to move this motor
                // ...unless we have backlash to worry about.
                int nSteps = dampStep(steps[cmi]);
                Messages.postDebug("AllTuneAlgorithm", "ProbeTune.tuneStep: "
                                   + "isSameDirection(" + nSteps + ")="
                                   + mInfo.isSameDirection(nSteps));

                if (!mInfo.isSameDirection(nSteps)
                    && nSteps != 0
                    && (
                           (m_nReverses[cmi] < MAX_STEP_REVERSES
                            && Math.abs(nSteps) > STEP_SLACK)
                        || (m_nReverses[cmi] < MIN_STEP_REVERSES // FIXME???
                                && Math.abs(nSteps) > STEP_SLACK)
                        || (nSteps != 0 && m_nReverses[cmi] == 0)
                        )
                    )
                {
                    // We need to back up.
                    // (Either we're just getting started, or we went too far.)
                    m_nReverses[cmi]++;
                    double currentFrequency = tuneInfo.getFrequency();
                    nSteps = reverseMotor(cmi, chanInfo, nSteps,
                                          currentFrequency, steps[cmi]);
                    measureTune(chanInfo); // Measure current position

                    // Recalculate all steps.
                    steps = chanInfo.calcStepsToTune(target.getFreq_hz());

                    Messages.postDebug("TuneAlgorithm", "ProbeTune.tuneStep: "
                                       + "Reversed motor "
                                       + chanInfo.getMotorNumber(cmi)
                                       + ": steps="
                                       + Fmt.f(2, steps, false, false));
                    if (steps != null) {
                        nSteps = dampStep(steps[cmi]);
                    }
                }

                if (mInfo.isSameDirection(nSteps)) {
                    if (Math.abs(nSteps) > m_maxStepSize) {
                        nSteps = nSteps > 0 ?  m_maxStepSize : -m_maxStepSize;
                    }
                    // Go towards target
                    Messages.postDebug("TuneAlgorithm", "ProbeTune.tuneStep: "
                                       + "Step motor "
                                       + chanInfo.getMotorNumber(cmi) + " "
                                       + nSteps + " towards target.");
                    motion = true;
                    chanInfo.step(cmi, nSteps);
                } else {
                    // This should never happen
                    Messages.postDebug("TuneAlgorithm",
                                       "Could not take up backlash in motor "
                                       + chanInfo.getMotorNumber(cmi));
                }
            }
        }
        if (chanInfo.getStuckMotorNumber() >= 0) {
            motion = false;
        }
        Messages.postDebug("AllTuneAlgorithm", "ProbeTune.tuneStep: "
                           + "returning from this tuning step with done="
                           + !motion);
        return motion;
    }

    /**
     * Get ready to reverse a motor by taking up the backlash.
     * If the distance is short, back up and approach again from the
     * same direction.
     * If the distance is far, just take up the backlash in the
     * requested (new) direction.
     * @param cmi Which motor to turn.
     * @param chanInfo Which channel to measure.
     * @param reqSteps Where we have requested to go.
     * @param currentFrequency The current dip position (Hz).
     * @param estSteps How far we think it is to the target.
     * @return A new estimate of "reqSteps".
     */
    private int reverseMotor(int cmi, ChannelInfo chanInfo, int reqSteps,
                             double currentFrequency, double estSteps)
        throws InterruptedException {

        Messages.postDebug("ReverseMotor",
                           "reverseMotor: reqSteps=" + reqSteps);
        int newEstSteps = (int)Math.round(estSteps);
        double backlash = chanInfo.getTotalBacklash(cmi);
        // NB: graylash is the uncertainty in the backlash
        double graylash = chanInfo.getTotalGraylash(cmi);
        int reqDir = sign(reqSteps); // = +/- 1
        int absNSteps = Math.abs(reqSteps);
        if (Math.abs(estSteps) > backlash + graylash) {
            // Target is far away - just take up the backlash
            int n = (int)Math.round(reqDir * backlash);
            Messages.postDebug("TuneAlgorithm", "ProbeTune.reverseMotor: "
                               + "Go towards target to take up backlash: step "
                               + chanInfo.getMotorNumber(cmi) + " " + n);
            chanInfo.step(cmi, n);
            // NB: just took up the estimated backlash, dip nominally unmoved
        } else {
            // Choose direction to back off based on which end of the
            // channel we are on.
            int dir = chanInfo.getSafeTuneDirection(cmi, currentFrequency);
            if (dir * reqDir > 0) {
                // Back up and try again from previous direction
                // TODO: If we are almost tuned: remember both motor positions,
                // back up, and bring this motor slowly up to position without
                // moving the other motor until we are almost tuned again.
                double backoff = backlash + 2 * graylash + Math.abs(estSteps);
                int n = (int)Math.round(reqDir * backoff);
                Messages.postDebug("TuneAlgorithm", "ProbeTune.reverseMotor: "
                                   + "Back off past backlash: step "
                                   + chanInfo.getMotorNumber(cmi) + " " + n);
                chanInfo.step(cmi, n, false);
                newEstSteps -= Math.round(reqDir * (backoff - backlash));
                // Now take up the backlash going the other way
                n = (int)Math.round(-reqDir * (backlash + graylash));
                Messages.postDebug("TuneAlgorithm", "ProbeTune.reverseMotor: "
                                   + "...and take up backlash going fwd: step "
                                   + chanInfo.getMotorNumber(cmi) + " " + n);
                chanInfo.step(cmi, n);
                newEstSteps -= Math.round(-reqDir * (graylash));

            } else {
                // Go forward (in previous direction), then take up
                // backlash in requested direction
                double backoff = backlash + 2 * graylash - absNSteps;
                int n = (int)Math.round(-reqDir * backoff);
                Messages.postDebug("TuneAlgorithm", "ProbeTune.reverseMotor: "
                                   + "Go forward past backlash: step "
                                   + chanInfo.getMotorNumber(cmi) + " " + n);
                chanInfo.step(cmi, n, false);
                newEstSteps -= n;
                // Now take up the backlash going the requested direction
                n = (int)Math.round(reqDir * (backlash + graylash));
                Messages.postDebug("TuneAlgorithm", "ProbeTune.reverseMotor: "
                                   + "...and take up backlash going back: step "
                                   + chanInfo.getMotorNumber(cmi) + " " + n);
                chanInfo.step(cmi, n);
                newEstSteps -= Math.round(reqDir * (graylash));
            }
        }
        // Now we should going right direction with all backlash taken up
        newEstSteps /= 2; // Damping in new default step
        Messages.postDebug("ReverseMotor",
                           "reverseMotor: returning reqSteps=" + newEstSteps);
        return newEstSteps;
    }

    private int sign(double d) {
        return d >= 0 ? 1 : -1;
    }

    /**
     * Get ready to reverse a motor by taking up the backlash.
     * @param cmi Which motor to turn.
     * @param chanInfo Which channel to measure.
     * @param nSteps The direction we want to end up stepping.  We go the
     * opposite way, initially, to measure backlash.
     */
    // TODO; Should probably call this somewhere to check the backlash
    protected void getBacklash(int cmi, ChannelInfo chanInfo, int nSteps)
        throws InterruptedException {

        Messages.postDebug("Backlash", "ProbeTune.getBacklash: "
                           + "*** Taking up & measuring backlash ***");

        // Initial, target tune
        TuneInfo t0 = chanInfo.measureTune(0, false);// Don't display
        //double f0 = getFrequency(0);

        // Take a big step in the opposite direction to get past backlash
        int bigStep = chanInfo.getMaxBacklash(cmi);
        int backlash = -bigStep;
        if (nSteps > 0) {
            bigStep *= -1;
        }
        chanInfo.step(cmi, bigStep);

        chanInfo.measureTune(00, true);
        double[] tuneError = chanInfo.calcStepsToTune(t0.getFrequency());
        double direction = tuneError[cmi];
        int iStep = dampStep(tuneError[cmi]);
        while (iStep * direction > 0) {
            chanInfo.step(cmi, iStep);
            backlash += Math.abs(iStep);
            chanInfo.measureTune(00, true);
            tuneError = chanInfo.calcStepsToTune(t0.getFrequency());
            iStep = dampStep(tuneError[cmi]);
        }
        chanInfo.getMotorInfo(cmi).setBacklash(backlash);

        int tuneCmi = chanInfo.getTuneMotorChannelIndex();
        String motor = (cmi == tuneCmi) ? "tune" : "match";
        displayBacklash(backlash, motor);
    }

    /**
     * Measure the current tune position on a given channel.
     * The result is stored as the Measured Tune Position, as well
     * as being returned to the caller.
     * @param ci Which channel to measure.
     * @return The TuneInfo for the measured tune position.
     * @throws InterruptedException if this command is aborted.
     */
    private TuneInfo measureTune(ChannelInfo ci) throws InterruptedException {
        return measureTune(ci, 00, true);
    }

    /**
     * Measure the current tune position on this channel.
     * @param chan Which channel to measure.
     * @param when Clock time in ms of oldest acceptable data. Use 0
     * to accept any data, or -1 to accept any data newer than the data
     * last read.
     * @param display If true, the sweep for the measurement is
     * sent to the GUI.
     * @return The measured tune position.
     * @throws InterruptedException if this command is aborted.
     */
    public TuneInfo measureTune(ChannelInfo chan, long when, boolean display)
        throws InterruptedException {

        if (sweepControl == null) {
            return null;
        }
        ReflectionData data = sweepControl.getData(chan, when);
        if (data == null) {
            Messages.postError("ProbeTune.measureTune: No sweep data");
            return null;
        }
        TuneInfo tuneInfo = new TuneInfo(data);
        chan.setMeasuredTunePosition(tuneInfo);
        if (display) {
            displayData(data);
        }
        return tuneInfo;
    }

    /**
     * Save the given measured reflection data in the given channel.
     * @param chanInfo The channel the data applies to.
     * @param data The data.
     */
    public void setMeasuredTune(ChannelInfo chanInfo, ReflectionData data) {
        TuneInfo tuneInfo = new TuneInfo(data);
        chanInfo.setMeasuredTunePosition(tuneInfo);
        displayData(data);
    }

    //public boolean isMatchOk(ChannelInfo chanInfo, TuneInfo dst) {
    //    return chanInfo.isMatchBetterThan(dst);
    //}

    //public boolean isMatchBetterThan(double level,
    //                                 ReflectionData data, TuneInfo dst) {
    //    double freq = dst.getFrequency();
    //    double targetRefl2 = data.getMatchAtFreq(freq);
    //    return Math.abs(targetRefl2) <= level;
    //}

    /**
     * Get an ordered list of the available ChannelInfos.
     * @return The list, ordered by channel number.
     */
    public List<ChannelInfo> getChannelInfos() {
        if (m_chanList == null) {
            if (m_chanInfos == null) {
                m_chanInfos = ChannelInfo.getChanInfos(this, m_modeName);
            }
            // Make sure these are in order
            // NB: Optimized for few gaps in the channel numbering
            m_chanList = new ArrayList<ChannelInfo>();
            int n = m_chanInfos.size();
            for (int i = 0, j = 0; j < n; i++) {
                ChannelInfo ci = m_chanInfos.get(i);
                if (ci != null) {
                    m_chanList.add(ci);
                    j++;
                }
            }
        }
        return m_chanList;
    }

    /**
     * Get the ChannelInfo instance for the channel with the given index
     * number.
     * @param iChan The channel index number (starting at 0).
     * @return The ChannelInfo object, or null if the channel is unknown.
     * @throws InterruptedException if this command is aborted.
     */
    public ChannelInfo getChannelInfo(int iChan) throws InterruptedException {
        if (m_chanInfos == null) {
            m_chanInfos = ChannelInfo.getChanInfos(this);
        }
        ChannelInfo chanInfo = m_chanInfos.get(iChan);;
        if (chanInfo == null) {
            String chanName = "chan#" + iChan;
                exit("No system channel info found for \""
                     + m_probeName + "/" + chanName + "\"");
        }
        return chanInfo;
    }

    /**
     * Get the ChannelInfo instance for the channel with the given index
     * number.
     * If the channel does not exist or has not been initialized, null
     * is returned.
     * @param channel The channel index number (starting at 0).
     * @return The ChannelInfo object, or null.
     */
    public ChannelInfo getExistingChannelInfo(int channel) {
        ChannelInfo chanInfo = null;
        try {
            chanInfo = m_chanInfos.get(channel);
        } catch (Exception e) {}
        return chanInfo;
    }

    public void displayCalibration() {
        sendToGui("showCal");
    }

    /*
    public void sendToGui(String s) {
        if (guiWriter != null) {
            guiWriter.println(s);
        } else {
            printlnErrorMessage(4, "guiWriter=" + guiWriter);
        }
    }
    */

    public void sendToGui(String s) {
        Messages.postDebug("SendToGui", "To GUI> " + s);
        m_listeners.write(s);
    }

    public void displayHzPerStep(double value, String motor) {
        sendToGui("setHzPerStep " + value + " " + motor);
    }

    public void displayMatchPerStep(double value, String motor) {
        sendToGui("setMatchPerStep " + value + " " + motor);
    }

    public void displayBacklash(double value, String motor) {
        sendToGui("setBacklash " + value + " " + motor);
    }

    public void displayTargetFreq(double value, int chan)
    throws InterruptedException {
        sendToGui("setTargetFreq " + value + " " + chan);
        sendToGui("displayMatchTolerance " + m_criterion.toString());
        ChannelInfo ci = getChannelInfo(m_channelNumber);
        if (ci != null) {
            ci.setTargetFreq(value);
        }
    }

    public void displayLastStep(int gmi, int step) {
        sendToGui("setLastStep " + gmi + " " + step);
    }

    public void displayData(ReflectionData data) {
        if (data != null) {
            m_reflectionData = data;
            String prefix = "setData";
            if (data.m_calFailed) {
                prefix += " calfailed";
            }
            String strData = data.getStringData(prefix, " ");
            sendToGui(strData);
            data.calculateDip();
            ReflectionData.Dip[] dips = data.getDips();
            boolean gotDip = dips != null && dips.length > 0 && dips[0] != null;
            if (gotDip) {
                for (int i = 0; i < dips.length && dips[i] != null; i++) {
                    ReflectionData.Dip dip = dips[i];
                    double[] arc = dip.getCircleArc();
                    displayFitCircle(arc[0], arc[1], arc[2], arc[3], arc[4]);
                    double[] reflect = dip.getReflectionAt(m_targetFreq);
                    if (reflect != null) {
                        displayReflectionAt(m_targetFreq,
                                            reflect[0], reflect[1]);
                    }
                    double dipReflect = dip.getReflection();
                    double dipFreq = dip.getFrequency();
                    displayVertex(dipFreq, dipReflect);
                    Messages.postDebug("DisplayData",
                                       "dip freq=" + Fmt.f(3, dipFreq/1e6)
                                       + ", sdv="
                                       + Fmt.f(4, dip.getFrequencySdv()/1e6)
                                       );
                }
            } else {
                displayVertex(Double.NaN, Double.NaN);
                double[] reflect = data.getComplexReflectionAtFreq(m_targetFreq);
                if (reflect != null) {
                    displayReflectionAt(m_targetFreq, reflect[0], reflect[1]);
                }
            }
            //displayVertex(data.getDipFreq(), Math.sqrt(data.getDipValue()));
            //displayVertex(m_targetFreq, m_targetMatch);
        }
        Messages.postDebug("DisplayData", "displayData() done");
    }

    public void displayFitCircle(double x, double y, double r,
                                 double theta0, double deltaTheta) {
        sendToGui("displayFitCircle " + x + " " + y + " " + r
                  + " " + theta0 + " " + deltaTheta);
    }

    private void displayReflectionAt(double freq, double real, double imag) {
        sendToGui("displayReflectionAt " + freq + " " + real + " " + imag);
    }

    public void displayVertex(double x, double y) {
        sendToGui("setVertex " + x + " " + y);
    }

    public void displayRefl(double refl) {
        sendToGui("setRefl " + refl);
    }

    public void displayCommand(String cmd) {
        sendToGui("displayCommand " + cmd);
    }

    public void displayStatus(int status) {
        sendToGui("setStatus " + status);
    }

    public void displayMLStatus(int status) {
        sendToGui("setMLStatus " + status);
    }

    public void displayBandSwitch(int band) {
        sendToGui("displayBandSwitch " + band);
    }

    public void displayPhase(int quadPhase) {
        displayPhase(quadPhase, false);
    }

    public void displayPhase(int quadPhase, boolean isFixedValue) {
        String cmd = "displayPhase " + quadPhase;
        if (isFixedValue) {
            cmd += " fixed";
        }
        sendToGui(cmd);
    }

    public void displayProbeDelay(double delay_ns) {
        sendToGui("displayProbeDelay " + delay_ns);
    }

    public void setReceived(String received) { // TODO: purge this method
        sendToGui("setReceived " + received);
    }

    public MotorControl getMotorControl() {
        return motorControl;
    }

    /**
     * @return Returns the sweepControl.
     */
    public SweepControl getSweepControl() {
        return sweepControl;
    }
    public MotorInfo getMotorInfo(int gmi) throws InterruptedException {
        return motorControl.getMotorInfo(gmi);
    }

    public double getCenter() {
        return sweepControl.getCenter();
        //return (m_stopFreq + m_startFreq) / 2;
    }

    public double getSpan() {
        return sweepControl.getSpan();
        //return m_stopFreq - m_startFreq;
    }

    public double getTargetFreq() {
        return m_targetFreq;
    }

    /**
     * @return Returns the m_1stSweepTimeout.
     */
    public static long get1stSweepTimeout() {
        return m_1stSweepTimeout;
    }

    public void setTargetFreq(double freq) throws InterruptedException {
         m_targetFreq = freq;
         displayTargetFreq(freq, m_channelNumber);
    }

    /**
     * Get the current channel.
     * @return The channel.
     */
    public ChannelInfo getChannel() {
        return m_chanInfos.get(m_channelNumber);
    }

    //public void step(int gmi, int nSteps) {
    //    motorControl.step(gmi, nSteps);
    //}

    // In general, these "setXxxx" methods should update the GUI on
    // what's happening.

    /**
     * Set channel (and probe) name.
     * The name must be of the form "PPPPP/chan#N", where "PPPPP" is
     * the probe ID string (any length, the characters "/" and "\" not
     * allowed), and "N" is an integer giving the channel number.
     * This is the name of the file containing the channel config.
     * @param name The probe/channel name.
     * @param setSweepFlag If true, set sweep to full range of this channel.
     * @throws InterruptedException if this command is aborted.
     */
    public void setChannel(String name, boolean setSweepFlag)
        throws InterruptedException {

        int n = Math.max(name.lastIndexOf('/'), name.lastIndexOf('\\'));
        if (n <= 0) {
            Messages.postError("No probe in string: \"" + name + "\"");
        } else {
            String probeName = name.substring(0, n);
            n = 1 + name.lastIndexOf('#');
            if (n <= 0) {
                Messages.postError("No channel number in string: \""
                                   + name + "\"");
            } else {
                try {
                    int iChan = Integer.parseInt(name.substring(n));
                    // Have a legal probe name and channel #
                    setChannel(probeName, iChan, 0, setSweepFlag);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Illegal channel number: \""
                                       + name.substring(n) + "\"");
                }
            }
        }
    }

    /**
     * Set the channel number, assuming the current probe.
     * @param iChan Which channel to set.
     * @param rfchan Which RF channel to use, or 0 for automatic.
     * @param setSweepFlag If true, set sweep to full range of this channel.
     * @return True if channel is set and initialized.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean setChannel(int iChan, int rfchan, boolean setSweepFlag)
        throws InterruptedException {

        return setChannel(m_probeName, iChan, rfchan, setSweepFlag);
    }

    /**
     * Set the channel number and probe name.
     * @param probeName The probe name (Vnmr "probe" parameeter.
     * @param iChan Channel number, starting at 0.
     * @param rfchan Which RF channel to use, or 0 for automatic.
     * @param setSweepFlag If true, set sweep to full range of this channel.
     * @return True if channel is set and initialized.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean setChannel(String probeName, int iChan, int rfchan,
                              boolean setSweepFlag)
    throws InterruptedException {

        if (probeName == null) {
            Messages.postError("ProbeTune.setChannel(): Null probe name");
            return false;
        }
        if (!probeName.equals(m_probeName)) {
            // Probe changed - dump old channel info.
            m_chanInfos = new TreeMap<Integer,ChannelInfo>();
            setProbe(probeName);
            ChannelInfo.initializeMotorToChannelMap(this);
        }

        boolean ok = false;
        m_channelNumber = iChan;
        ChannelInfo ci = getChannelInfo(iChan);
        if (ci != null) {
            ci.setRfchan(rfchan);
            displayAllChannelInfo();
            if (sweepControl instanceof MtuneControl) {
                ok = ((MtuneControl)sweepControl).initializeForChannel(null, 0);
            }
            ci.setChannel(setSweepFlag);
            if (sweepControl instanceof MtuneControl) {
                ok = ((MtuneControl)sweepControl).initializeForChannel(ci,
                                                                       rfchan);
            }
            if (setSweepFlag) {
                ci.setFullSweep();
            }
            setTargetFreq(ci.getTargetFreq());
        }
        return ok;
    }

    public String displayInfo() throws InterruptedException {
        // HTML abbrevs:
        final String BDL = "<tr><td>"; // Begin Data Left
        final String EDL = "</td>"; // End Data Left
        final String BDR = "<td>"; // Begin Data Right
        final String EDR = "</td></tr>"; // End Data Right
        final String EBD = EDL + BDR; // End (left) Begin (right) Data

        String title = MotorControl.isPZT()
                ? "PZT_Information" : "ProTune_Information";
        String msg = "<html><table>";
        String verDate = getSWVersion() + "  " + getSWDate();
        msg += BDL + "Software Version" + EBD + verDate + EDR;
        if (getMotorName() != null && !getMotorName().equals(getMotorIpAddr())){
            msg += BDL + "Motor Hostname" + EBD + getMotorName() + EDR;
        }
        msg += BDL + "Motor IP Address" + EBD + getMotorIpAddr() + EDR;
        if (MotorControl.isPZT()) {
            // NB: isPZT implies there is a valid Firmware Version
            String moduleId = MotorControl.getModuleId();
            msg += BDL + "PZT Module ID" + EBD + moduleId + EDR;
            String ver = MotorControl.getFirmwareVersion();
            msg += BDL + "Firmware" + EBD + ver + EDR;
        } else {
            boolean gotMotor = false;
            for (int gmi = 0; gmi < MotorControl.MAX_MOTORS; gmi++) {
                if (motorControl.isValidMotor(gmi)) {
                    gotMotor = true;
                    String ver = getMotorInfo(gmi).getFirmwareVersion();
                    msg += BDL + "Motor " + gmi + " Firmware" + EBD + ver + EDR;
                }
            }
            if (!gotMotor) {
                msg += BDL + "Firmware" + EBD + "NO COMMUNICATION" + EDR;
            }
        }
        msg += BDL + "Probe Name" + EBD + getProbeName() + EDR;
        msg += BDL + "User Tune Directory" + EBD + getUsrTuneDir() + EDR;
        msg += BDL + "System Tune Directory" + EBD + getSysTuneDir() + EDR;
        String cpath = (getSweepControl() instanceof MtuneControl)
                ? MtuneControl.getCalDir(getProbeName())
                : "None";
        msg += BDL + "RF Calibration Directory" + EBD + cpath + EDR;
        msg += "</table>";

        sendToGui("popup info title " + title + " msg " + msg);

        return msg;
    };

    /**
     * Send info about this channel to the GUI.
     * @throws InterruptedException if this command is aborted.
     */
    public void displayAllChannelInfo() throws InterruptedException {
        sendToGui("displayChannel " + m_probeName + "/chan#" + m_channelNumber);
        motorControl.displayAllMotorLimits();
        motorControl.readAllMotorPositions(); // Updates position in GUI also
        displayAllMotorNames();
        ChannelInfo ci = getChannelInfo(m_channelNumber);
        ci.displayAllBacklashes();
        ci.displaySensitivities();
    }

    protected void displayAllMotorNames() throws InterruptedException {
        ChannelInfo ci = getChannelInfo(m_channelNumber);
        int maxMotors = motorControl.getNumberOfMotors();
        for (int gmi = 0; gmi < maxMotors; gmi++) {
            String name = "";
            boolean emphasized = false;
            boolean isUsed = motorControl.isValidMotor(gmi);
            int cmi = ci.getChannelMotorNumber(gmi);
            if (cmi >= 0) {
                // This is either Tune or Match.
                emphasized = true;
                if (cmi == ci.getMatchMotor()) {
                    name = "Match";

                } else {
                    // Must be the tune motor
                    name = "Tune";
                }
            } else if (gmi >= 0) {
                int j;
                if ((j = ci.getMasterOf(gmi)) >= 0) {
                    name = "Slave of " + j;
                } else {
                    // Maybe it's a switch
                    j = 0;
                    for (int i = 0; j >= 0; i++) {
                        j = ci.getSwitchMotorNumber(i);
                        if (j == gmi) {
                            name = "Switch";
                        }
                    }
                }
            }
            sendToGui("displayMotorName " + gmi + " " + name);
            sendToGui("displayMotorEmphasis " + gmi + " " + emphasized);
            sendToGui("displayMotorPresent " + gmi + " " + isUsed);
        }
    }

    public void saveQuadPhase() {
        if (sweepControl != null && sweepControl instanceof MtuneControl) {
            ((MtuneControl)sweepControl).saveQuadPhase();
        }
    }

    public void setQuadPhase(int quadPhase) {
        if (sweepControl != null && sweepControl instanceof MtuneControl) {
            ((MtuneControl)sweepControl).setQuadPhase(quadPhase,
                                                      quadPhase >= 0);
        }
    }

    public void resetChannelPhases() throws InterruptedException {
        for (ChannelInfo ci : getChannelInfos()) {
            ci.setQuadPhase(null);
        }
    }

    public void setProbeDelay(double delay_ns) throws InterruptedException {
        if (m_channelNumber >= 0) {
            ChannelInfo chanInfo = getChannelInfo(m_channelNumber);
            if (chanInfo != null) {
                chanInfo.setProbeDelay(delay_ns);
            }
        }
    }

    public void setSweep(double center, double span, int rfchan) {
        /*System.out.println("ProbeTune.setSweep(" + Fmt.f(3, center / 1e6)
                           + ", " + Fmt.f(3, span / 1e6) + ")");/*DBG*/
        setSweep(center, span, sweepControl.getNp(), rfchan);
    }

    public void setSweep(double center, double span, int np, int rfchan) {
        setSweep(center, span, np, rfchan, false);
    }

    public void setSweep(double center, double span, int np, int rfchan, boolean force) {
        sweepControl.setSweep(center, span, np, rfchan, force);
    }

    public void setFullSweep() throws InterruptedException {
        getChannelInfo(m_channelNumber).setFullSweep();
    }


    public void setSweepLimits(double minFreq, double maxFreq) {
        if (sweepControl != null) {
            sweepControl.setLimits(minFreq, maxFreq);
        }
    }

    public void setCenter(double center) {
        if (sweepControl != null) {
            sweepControl.setCenter(center);
        }
    }

    public void setSpan(double span) {
        if (sweepControl != null) {
            sweepControl.setSpan(span);
        }
    }

    public void setNp(int np) {
        if (sweepControl != null) {
            sweepControl.setNp(np);
        }
    }

    /**
     * @return Returns the m_sysInitialized.
     */
    public boolean isSysInitialized() {
        return m_sysInitialized;
    }

    public void setSysInitialized(boolean b) {
        m_sysInitialized = b;
        sendToGui("setSysInitialized " + (b ? 1 : 0));
    }

    public int getNp() {
        return sweepControl.getNp();
    }

    public String getProbeName() {
        return m_probeName;
    }

    public String getUsrProbeName() {
        return m_usrProbeName;
    }

    public String getSysTuneDir() {
        if (m_sysTuneDir == null) {
            m_sysTuneDir = m_vnmrSystemDir + "/tune/" + getProbeName();
        }
        return m_sysTuneDir;
    }

    public String getUsrTuneDir() {
        return getVnmrUserDir() + "/tune/" + getUsrProbeName();
    }

    public static String getVnmrSystemDir() {
        if (m_vnmrSystemDir == null) {
            m_vnmrSystemDir = "/vnmr";
        }
        return m_vnmrSystemDir;
    }

    public static String getVnmrUserDir() {
        return m_vnmrUserDir;
    }

    /**
     * @return Returns the m_debugLevel.
     */
    public int getDebugLevel() {
        return m_debugLevel;
    }

    public static String getSWVersion() {
        return VERSION;
    }

    public static String getSWDate() {
        return m_date;
    }

    public static boolean isSwVersionAtLeast(String reqVersion) {
        StringTokenizer tok1 = new StringTokenizer(VERSION, ".");
        StringTokenizer tok2 = new StringTokenizer(reqVersion, ".");
        boolean rtn = true;
        while (tok2.hasMoreTokens()) {
            if (!tok1.hasMoreTokens()) {
                rtn = false;
                break;
            } else {
                int v1 = Integer.parseInt(tok1.nextToken());
                int v2 = Integer.parseInt(tok2.nextToken());
                if (v1 < v2) {
                    rtn = false; // Current version too small
                    break;
                } else if (v1 > v2) {
                    break;     // Current version bigger than required
                }
            }
        }
        return rtn;
    }

    /**
     * Set the network host name for the motor module.
     * @param name The host name.
     */
    public static void setMotorName(String name) {
        m_motorHostname = name;
    }

    /**
     * Get the network host name for the motor module.
     * @return The host name.
     */
    public static String getMotorName() {
        return m_motorHostname;
    }

    /**
     * Set the IP address of the motor module.
     * @param address The IP address as a "dot string".
     */
    public static void setMotorAddress(String address) {
        m_motorIpAddr = address;
    }

    /**
     * @return Returns the IP address of the motor module as a "dot string".
     */
    public static String getMotorIpAddr() {
        return m_motorIpAddr;
    }

    /**
     * @return Returns the m_sweepIpAddr.
     */
    public static String getSweepIpAddr() {
        m_sweepIpAddr = TuneUtilities.getProperty("apt.sweepIpAddr",
                                                  m_sweepIpAddr);
        return m_sweepIpAddr;
    }

    public void setTuneMode(int mode) throws InterruptedException {
        if (motorControl != null) motorControl.sendToMotor("tune " + mode);
        sendToGui("displayTuneMode " + mode);
    }

    public void setRawDataMode(boolean b) {
        m_rawDataMode = b;
        displayRawDataMode(b);
    }

    public void saveRawDataMode() {
        m_savedRawDataMode = m_rawDataMode;
    }

    public void restoreRawDataMode() {
        m_rawDataMode = m_savedRawDataMode;
        displayRawDataMode(m_rawDataMode);
    }

    public boolean isRawDataMode() {
        return m_rawDataMode;
    }

    public void displayMotorBacklash(int gmi) throws InterruptedException {
        int backlash = 0;
        ChannelInfo ci = getExistingChannelInfo(m_channelNumber);
        if (ci != null) {
            int cmi = ci.getChannelMotorNumber(gmi);
            if (cmi >= 0) {
                backlash = ci.getTotalBacklash(cmi);
            }
        }
        if (backlash == 0) {
            backlash = (int)Math.round(getMotorInfo(gmi).getBacklash());
        }
        sendToGui("displayMotorBacklash " + gmi + " " + backlash);
    }

    private void displayRawDataMode(boolean b) {
        int mode = b ? 1 : 0;
        sendToGui("displayRawDataMode " + mode);
    }

    /**
     * Put commands in a file to be executed by Vnmr when it gets
     * around to looking at the file - maybe at blocksize.
     * @param command The command to send.
     * @return True if successful.
     */
    public boolean queueToVnmr(String command) {
        final String COMMANDPATH = getVnmrSystemDir()+"/tmp/ptuneCmds";

        Messages.postDebug("VnmrMessages|QueueToVnmr",
                           "ProbeTune.queueToVnmr: \"" + command + "\"");
        // See if there is stuff already queued up
        File cmdfile = new File(COMMANDPATH);
        if (cmdfile.delete()) {
            // There was stuff there, append these commands to previous ones.
            if (m_queuedCommands.length() > 0 && !m_queuedCommands.endsWith(NL)) {
                m_queuedCommands += NL;
            }
            m_queuedCommands += command;
        } else {
            m_queuedCommands = command;
        }

        // First, write to a temp file
        String tmpout = COMMANDPATH + "tmp";
        File tmpfile = new File(tmpout);
        PrintWriter out = null;
        try {
            // NB: Only send ASCII characters
            FileOutputStream fout = new FileOutputStream(tmpfile);
            OutputStreamWriter osw = new OutputStreamWriter(fout, "US-ASCII");
            out = new PrintWriter(new BufferedWriter(osw));

            out.println(m_queuedCommands);
            out.close();

            // Now, move the temp file to the right place.
            if (!tmpfile.renameTo(cmdfile)) {
                cmdfile.delete();
                if (!tmpfile.renameTo(cmdfile)) {
                    Messages.postError("Unable to rename "+tmpfile.getName());
                }
            }
        } catch (IOException ioe) {
            Messages.postError("Can't write Vnmr cmds in " + tmpfile.getName()
                               + ": " + ioe);
        } catch (NullPointerException npe){
            Messages.postError("Can't write Vnmr cmds in " + tmpfile.getName()
                               + ": " + npe);
        } finally {
            try { out.close(); } catch (Exception e){};
        }
        return true;
    }

    /**
     * Send a command to be executed by Vnmr.
     * @param command Command to send.
     * @return True if command was sent.
     */
    static synchronized public boolean sendToVnmr(String command) {
        Messages.postDebug("SendToVnmr", "cmd=\"" + command + "\"");

        if (!m_isVnmrConnection) {
            return false;
        }

        String reason = "";
        String infoPath = getVnmrUserDir() + "/persistence/.talk2j";
        if (m_isVnmrConnection && !new File(infoPath).canRead()) {
            m_isVnmrConnection = false;
            reason = "File \"" + infoPath + "\" is not readable";
        }
        String execPath = getVnmrSystemDir() + "/bin/send2Vnmr";
        if (m_isVnmrConnection && !new File(execPath).canExecute()) {
            m_isVnmrConnection = false;
            reason = "File \"" + execPath + "\" is not executable";
        }
        if (m_isVnmrConnection) {
            try {
                // Only send ASCII characters
                command = new String(command.getBytes("US-ASCII"));
            } catch (UnsupportedEncodingException e1) {
                // Will never happen
            }
            Messages.postDebug("SendToVnmr", "talk file = \""
                               + infoPath + "\", cmd=\"" + command + "\"");
            String[] cmdArray = new String[3];
            cmdArray[0] = execPath;
            cmdArray[1] = infoPath;
            cmdArray[2] = command;
            try {
                Process process = Runtime.getRuntime().exec(cmdArray);
                int status = 1;
                for (int i = 0; i < 100; i++) {
                    try {
                        status = process.exitValue();
                        Messages.postDebug("sendToVnmr",
                                           "send2Vnmr status=" + status
                                           + ", tries =" + i);
                        break;
                    } catch (IllegalThreadStateException itse) {
                        // process is still running
                        try {
                            Thread.sleep(1);
                        } catch (InterruptedException e) {
                            // If interrupted, can try again later
                            process.destroy();
                            status = 0;
                            break;
                        }
                    }
                }
                if (status != 0) {
                    process.destroy();
                    if (m_isVnmrConnection) {
                        m_isVnmrConnection = false;
                        Messages.postDebugWarning("send2Vnmr timed out");
                    }
                }
            } catch (IOException e) {
                m_isVnmrConnection = false;
                reason = "send2Vnmr shell command failed: " + e;
            }
        }
        if (!m_isVnmrConnection) {
            Messages.postError("Cannot send message to Vnmr: " + reason);
        }
        return m_isVnmrConnection;
    }

    /**
     * Set the tolerance on the match.
     * @param thresh The threshold in units of the
     * square of the reflection coefficient.
     */
    public void setTargetMatch(double thresh) {
        double target_db = 10 * Math.log10(thresh);
        m_criterion = new TuneCriterion(target_db);
    }

    /**
     * Get the tolerance on the match.
     * @return The threshold in units of the
     * square of the reflection coefficient.
     */
    public double getTargetMatch() {
        return m_criterion.getTarget_pwr();
    }

    /**
     * Get the tolerance on the match.
     * @return The threshold in units of dB.
     */
    public double getTargetMatch_db() {
        return m_criterion.getTarget_db();
    }

    /**
     * Get the tolerance on the reflection amplitude.
     * The absolute value of the reflection coefficient.
     * @return The absolute value of the biggest tolerated reflection.
     */
    public double getTargetReflection() {
        return m_criterion.getTarget_refl();
    }

    public static AptProbeIdClient getProbe() {
    	return m_probeId;
    }

    public static boolean useProbeId() {
    	return m_useProbeId;
    }

    /**
     * See if the system has tune information for the given probe.
     * @param name The probe name, normally the Vnmr "probe" parameter.
     * @param sysFlag If true, look only in system directory.
     * @return True if "/vnmr/tune/[probename]" exists.
     */
    public boolean checkProbeName(String name, boolean sysFlag) {
        return getProbeFile(name, sysFlag) != null;
    }

//    public File getProbeObject(String name, String obj, boolean sysFlag) {
//        return new File(obj, getProbeFile(name, sysFlag).getPath());
//    }

//    public static File getProbeUserFile(String name) {
//        if (m_useProbeId) {
//            return m_probeId.blobLink(name, "tune", false, true);
//        }
//        return new File(getVnmrUserDir() + "/tune/" + name);
//    }

    public File getProbeFile(String name, boolean sysFlag) {
        // get a link to the probe if Probe ID is enabled
        File file = null;
        if (m_useProbeId) {
            file = m_probeId.blobLink(name, "tune", sysFlag, false);
        } else {
            // otherwise try user directory first
            file = new File(getUsrTuneDir());
            if (sysFlag || !file.exists()) {
                // No user file (or ignoring it); try system directory
                file = new File(getSysTuneDir());
                if (!file.exists()) {
                    return null;    // No file; give up
                }
            }
        }
        return file;
    }

//    public BufferedReader getPersistenceReader(String name, boolean sysFlag) {
//        // Look for a persistence file
//        // Try user directory first
//        File file = getProbeFile(name, sysFlag);
//        if (file == null) return null;
//        BufferedReader in = null;
//        try {
//            in = new BufferedReader(new FileReader(file));
//        } catch (FileNotFoundException fnfe) {
//        }
//        Messages.postDebug("Persistence",
//                           "ProbeTune.getPersistenceReader: "
//                           + "Reading \"" + file.getPath() + "\"");
//        return in;
//    }

//    public PrintWriter getPersistenceWriter(String name) {
//        PrintWriter out = null;
//    	if (m_useProbeId) {
//            // this is in the user directory, which may be different than
//            // the original source of the data in the system directory and
//            // may, in fact, not exist on the probe yet.
//            out = m_probeId.getPrintWriter(name, "tune", false, false);
//    	} else {
//            // Can only write to user directory
//            String path = getVnmrUserDir() + "/tune/" + name;
//            File file = new File(path);
//
//            // Make sure we have a directory to write into
//            File dir = file.getParentFile();
//            if (!dir.isDirectory()) {
//                if (!dir.mkdirs()) {
//                    return null;    // Can't get that directory
//                }
//            }
//
//            try {
//                out = new PrintWriter(new BufferedWriter(new FileWriter(path)));
//                Messages.postDebug("Persistence",
//                                   "ProbeTune.getPersistenceWriter: "
//                                   + "writing \"" + path + "\"");
//            } catch (IOException ioe) {
//                Messages.postError("Cannot write persistence file: \""
//                                   + path + "\": " + ioe);
//            }
//    	}
//    	return out;
//    }

    public void writePortNumber(int portNum) {
        String path = getVnmrSystemDir() + "/acqqueue/ptunePort";
        File file = new File(path);
        file.delete();
        PrintWriter out = null;
        try {
            out = new PrintWriter(new BufferedWriter(new FileWriter(file)));
            out.println(InetAddress.getLocalHost().getHostName()
                        + " " + portNum);
        } catch (IOException ioe) {
        } finally {
            try { out.close(); } catch (Exception e) {}
        }
    }


    class ExecTuneThread extends Thread {

        ExecTuneThread() {
        }

        public void run() {
            m_nExecThreadsRunning++;
            String cmd = "";
            try {
                while (m_commandList.size() > 0) {
                    displayStatus(STATUS_RUNNING);
                    try {
                        cmd = m_commandList.get(0);
                    } catch (IndexOutOfBoundsException ioobe) {
                        Messages.postDebug("Exec",
                                           "Exception getting next command: "
                                           + ioobe);
                    }
                    setName("exec " + cmd); // Set thread name for the debugger
                    try {
                        Messages.postDebug("Exec", "Call execute for: " + cmd);
                        execute();
                    } catch (InterruptedException ie) {
                        Messages.postWarning("Command \"" + cmd + "\" aborted");
                        setCancel(false);
                    }
                    if (isCancel()) {
                        Messages.postWarning("Command \"" + cmd + "\" was aborted");
                        setCancel(false);
                    }
                    displayStatus(STATUS_READY);
                }
            } catch (Exception e) {
                Messages.postError("Exception executing \"" + cmd + "\": " + e);
                Messages.postStackTrace(e);
            } finally {
                displayStatus(STATUS_READY);
                m_nExecThreadsRunning--;
                if (m_nExecThreadsRunning < 0) {
                    Messages.postError("Error: m_nExecThreadsRunning="
                                       + m_nExecThreadsRunning);
                    m_nExecThreadsRunning = 0;
                }
            }
        }
    }


    /**
     * Maintain a list of listeners, all waiting for commands from
     * different sockets.  One listener is created when the run()
     * method is called.  When a connection is made on the socket,
     * CommandListener calls listenerConnected() and another socket is
     * created; thus, there is always a socket available for a new
     * connection.  The host name and port number of the unconnected
     * socket are kept in a well known file: /vnmr/acqqueue/ptunePort.
     */
    class CommandListenersThread extends Thread implements ListenerManager {
    	private Semaphore m_lock = new Semaphore(1);
        private Executer mm_master;
        private ArrayList<CommandListener> mm_listeners
                = new ArrayList<CommandListener>();

        CommandListenersThread(Executer master) {
            mm_master = master;
        }

        private void lock() {
       	    if (m_useLock) try {
       	        m_lock.acquire();
       	    } catch (InterruptedException e) {
       	        e.printStackTrace();
       	    }
        }

        private void release() {
            if (m_useLock) m_lock.release();
        }

        /**
         * Send a message to all connected listeners.
         * @param msg The message to send.
         */
        public void write(String msg) {
	    lock();
            int nWritten = 0;
            int n = mm_listeners.size();
            for (int i = 0; i < n; i++) {
                CommandListener listener = mm_listeners.get(i);
                if (listener.isConnected()) {
                    PrintWriter writer = listener.getWriter();
                    writer.println(msg);
                    nWritten++;
                }
            }
            release();
            if (nWritten == 0) {
                if(m_firstTime){
                    Messages.postDebug("CommandListeners",
                                       "No listeners connected");
                    m_firstTime= false;
                }
            }
        }

        /** used for cleanup in unit test framework */
        public synchronized void close() {
            ArrayList<CommandListener> listeners = new ArrayList<CommandListener>();
            lock();
            listeners.addAll(mm_listeners);
            release();
            for (CommandListener listener : listeners) {
                listener.close();
            }
        }

        public void listenerDisconnected(CommandListener listener) {
            Messages.postDebug("CommandListeners",
                               "listenerDisconnected: " + listener);
            lock();
            mm_listeners.remove(listener);
            release();
        }

        public void listenerConnected(CommandListener listener) {
            Messages.postDebug("CommandListeners",
                               "listenerConnected: " + listener);
            makeListener();
        }

        /**
         * Return the number of an unconnected port.
         * @param timeout How long to keep trying (ms).
         * @return The port number, or 0 on failure.
         * @throws InterruptedException If user aborts.
         */
        public int getPort(int timeout) throws InterruptedException {
            boolean gotit = false;
            long t0 = System.currentTimeMillis();
            long wait = 0;
            while (!gotit && wait < timeout) {
                int n = mm_listeners.size();
                for (int i = 0; i < n; i++) {
                    CommandListener listener = mm_listeners.get(i);
                    if (!listener.isConnected()) {
                        return listener.getPort();
                    }
                }
                Thread.sleep(100);
                wait = System.currentTimeMillis() - t0;
            }
            Messages.postError("getPort: All ports in use!");
            return 0;
        }

        private void makeListener() {
            CommandListener listener = new CommandListener(mm_master, this);
            int port = listener.getPort();
            writePortNumber(port);   // Write port number to file
            listener.start();
            lock();
            mm_listeners.add(listener);
            release();
        }

        /**
         * Create a listener attached to a client socket at the
         * given host and port.
         * @param host The host name or IP.
         * @param port The port number.
         * @throws IOException If there is an I/O error in connecting.
         * @throws UnknownHostException If the host cannot be found.
         */
        public void connectTo(String host, int port)
                              throws UnknownHostException, IOException {
            CommandListener listener = new CommandListener(mm_master,
                                                           host, port, this);
            listener.start();
            lock();
            mm_listeners.add(listener);
            release();
        }

        public void run() {
            makeListener();
        }
    }


    /**
     * Container class to hold a channel number and its centering parameter.
     */
    public class ChannelAlignment {
        public double centeringParameter;
        public int iChannel;

        public ChannelAlignment(int channelNumber, double centering){
            centeringParameter = centering;
            iChannel = channelNumber;
        }
    }
}
