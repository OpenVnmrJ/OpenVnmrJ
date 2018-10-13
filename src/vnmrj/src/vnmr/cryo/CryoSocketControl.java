/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.cryo;


import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.*;
import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.NumberFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.SimpleTimeZone;
import java.util.StringTokenizer;
import java.util.TimeZone;
import java.util.TreeMap;

import javax.swing.*;
import java.awt.*;

import vnmr.ui.StatusManager;
import vnmr.util.Fmt;

import static vnmr.cryo.CryoConstants.*;
import static vnmr.cryo.CryoPort.*;
import static vnmr.cryo.Priority.*;


/**
 * NOTE: This javadoc may be out of date
 * <p>
 * This is the interface for communicating with the CryoBay.
 * There are three Thread inner classes:
 * <p>
 * Sender - takes commands from a list and sends them in order.
 * This is so the caller never has to wait while the CryoBay is busy, it puts
 * the command in the queue and returns immediately.
 * <br>
 * Reader - continuously listens for responses from the CryoBay. When it gets
 * a complete response it calls processReply(what, value), which deals
 * with notifying anyone who needs to know about it.
 * <br>
 * StatusPoller - sends requests for status every so often.
 * <p>
 * The basic strategy to execute a command and is:
 * <br>
 *
 *
 */
public class CryoSocketControl implements CryoBay {

    protected static final int CRYO_OK = 0;
    protected static final int CRYO_NIL = 1;
    protected static final int CRYO_UNRESPONSIVE = 2;

    private static final int STATUS_PERIOD = 5000; // ms

    static private final NumberFormat excelFmt
    = new DecimalFormat("00000.00000");
    static private final NumberFormat excelFmtPadded
        = new DecimalFormat("00000.00000        ");
    static private final NumberFormat i10FmtPadded
        = new DecimalFormat("0000000000         ");
    static private final TimeZone tz = new SimpleTimeZone(0, "GMT");

    static private Calendar gmtCalendar = Calendar.getInstance(tz);
    static private Calendar localCalendar = Calendar.getInstance();
    static private DateFormat date_TimeFmt; // w/ space between date and time
    static private DateFormat timeZoneFmt;
    static private DateFormat date_TimeFmtGmt; // w/ space between date and time
    static private DateFormat timeZoneFmtGmt;
    static private DateFormat dateTTimeFmt; // w/ "T" between date and time
    static private DateFormat dateMonthFmt;
    static private String blankDate = "                   ";
    //static private String dateTTimeFmtStr = "yyyy-MM-dd'T'HH:mm:ss";
    static {
        date_TimeFmt = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        date_TimeFmt.setCalendar(localCalendar);
        date_TimeFmtGmt = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        date_TimeFmtGmt.setCalendar(gmtCalendar);
        timeZoneFmt = new SimpleDateFormat("z");
        timeZoneFmt.setCalendar(localCalendar);
        timeZoneFmtGmt = new SimpleDateFormat("z");
        timeZoneFmtGmt.setCalendar(gmtCalendar);
        dateTTimeFmt = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss");
        dateTTimeFmt.setCalendar(localCalendar);
        dateMonthFmt = new SimpleDateFormat("yyyy-MM");
        dateMonthFmt.setCalendar(localCalendar);
    }

    /** Day offset from 1900 to 1970 (+2) */
    static private final long UNIX_TO_EXCEL = 25569;

    /** Seconds offset from 1980 to 1970 */
    static private final long DOS_TO_UNIX_DATE_OFFSET = 3652 * 24 * 60 *60;

    private CryoSocket m_CryoSocket = null;
    private int m_retries = 3;  // # of times to send command if no response
    public StatusPoller m_statusPoller = null;
    private StatusManager m_statusManager;
    private Map<Integer, String> m_statusNames
            = new HashMap<Integer, String>(10);
    private Sender m_sender = null;
    private Reader m_reader = null;
    private Map<String, Boolean> m_resultTable
        = new TreeMap<String, Boolean>();
    protected String m_command;    // Last command sent
    protected int m_replyFailType = CRYO_NIL;
    private String status;
    private ArrayList<String> m_currentUpload;
    private Timer m_uploadTimer = null;
    private long m_uploading = 0;
    private CryoPort m_socketType;
    private String m_uploadDataPath = null;
    private boolean m_versionKnown = false;

    private Map<String, String> m_commandMap = new TreeMap<String, String>();
    private MessageIF m_cryoMsg;
    private String m_dataDir;
    private String m_logDir;
    private String m_uploadFilePath;
    private String m_dateFormat = STRING_DATES_LOCAL;
    private long m_date_ms = 0;
    protected long m_dateSyncTime/* = System.currentTimeMillis()*/;
    private int m_uploadCount;
    private boolean m_isConnectError = false;

    static private String m_calibration = null;
    static private boolean m_uploadRequestFlag = false;
    static private int m_probeType= 0;	//type of probe PLUTO=0, PRIUS=1
    static private boolean getStatus;
    static private String m_firmwareVN = new String("");
    /** A table tracking which dialogs are showing. */
    static private Map<String, JDialog> m_dialogsShowing
            = new TreeMap<String, JDialog>();
    static private boolean m_requestedVersionPopup = false;

    /**
     * List of possible Cryo commands and their Integer codes.
     */
    private static final Object[][] STATUS_CODES = {
        {new Integer(0), 	"IDLE"},
        {new Integer(1), 	"VACUUM PUMPING"},
        {new Integer(2), 	"COOLING"},
        {new Integer(3), 	"OPERATING"},
        {new Integer(4), 	"WARMING"},
        {new Integer(5), 	"FAULT"},
        {new Integer(6), 	"SAFE"},
        {new Integer(7), 	"PURGE"},		//never used
        {new Integer(8), 	"THERMAL CYCLE"},
    };


    /**
     * A regular expression that matches commands that have arguments
     * embedded in their names.
     * These are valid commands for all firmware versions.
     */
    private static final String m_specialCmdRegex =
        "AN[0-9]"
        + "|AI[0-9]"
        + "|AO[0-9]"
        + "|DO[0-1]?[0-9]"
        + "|DI[0-2]?[0-9]"
        + "|DH[0-9]"
        + "|U_[0-9]"
        + "|C_FAST[01]\\?"
        ;

    /**
     * A map giving the firmware version needed to support a given command,
     * keyed by the command name.
     * The command is OK for a given version if the given version is
     * lexicographically greater than or equal to the version in the map.
     * The actual version will usually have month and day after the year.
     * We don't include month and day here unless there are 2 versions in
     * the same year. (JV2007 and JV2008 use letters for the month.)
     * The general version format is "JVyyyy-mm-dd ..."
     * A blank version means that any version is OK.
     */
    private static final String[][] COMMAND_MAP =
    {
        {"C_1", ""},
        {"C_2", ""},
        {"C_3", ""},
        {"C_4", ""},
        {"C_CAL", ""},
        {"C_COLD", ""},
        {"C_FAST", ""},
        {"C_HEAT", ""},
        {"C_PAUSE", ""},
        {"C_RANGE", ""},
        {"C_RESUME", ""},
        {"C_STAT", ""},
        {"C_WARM", ""},
        {"CALI", ""},
        {"CALRESET", ""},
        {"CALSET1", ""},
        {"CALSET2", ""},
        {"DATARATE", ""},
        {"GETDATE", "JV2010"},
        {"GETFAULT", ""},
        {"GETSTATE", ""},
        {"GM_HOURS", ""},
        {"GM_NEW", ""},
        {"GM_OFF", ""},
        {"GM_ON", ""},
        {"GM_PRESS", ""},
        {"GM_RESET", ""},
        {"GM_TEMP", ""},
        {"IP", ""},
        {"LIMIT_GET", ""},
        {"LIMIT_SET", ""},
        {"P_IN", ""},
        {"P_NUM", ""},
        {"P_OUT", ""},
        {"P_PURGE", ""},
        {"PROBETYPE", "JV2008"},
        {"READY", ""},
        {"REBOOT", ""},
        {"REMOVE", ""},
        {"RESET", ""},
        {"RESTART", ""},
        {"SAVE", ""},
        {"SETDATE", "JV2010"},
        {"SETSTATE", ""},
        {"SETWARM", ""},
        {"START", ""},
        {"STOP", ""},
        {"T_COMM", ""},
        {"T_DAYS", ""},
        {"T_ERR", ""},
        {"T_OFF", ""},
        {"T_ON", ""},
        {"T_REM", ""},
        {"T_STAT", ""},
        {"T_TEST", ""},
        {"T_TIME", ""},
        {"THERMALCYCLE", "JV2008"},
        {"THERMALCYCLETIMEOUT", "JV2012"},
        {"TIMER_GET", ""},
        {"TIMER_SET", ""},
        {"U_SHUTDOWN", ""},
        {"U_SILENCE", ""},
        {"U_STATUS", ""},
        {"UPLOAD", ""},
        {"V_PROBE", ""},
        {"V_PURGE", ""},
        {"VN", ""},
        {"W_CCC1", ""},
        {"W_CCC2", ""},
        {"W_PROBE", ""},
        {"XERASELOG", "JV2010"},
        {"XFAULT", "JV2010"},
        {"XMAGIC", "JV2010"},
    };

    private static final char NL = '\n';
    private static final String DATA_HDR
    = "TAG\tTIME            \tTZ\tSTATE\tINLET\tEXHAUST\tMASSFLO\tVACUUM  \tIONPUMP\tCLI\tPROBE\tCCC1\tCCC2\tPREAMP\tHEATER";
    private static final String LEGACY_DATA_HDR
    = "UPLOAD TIME     \tTZ\tTAG\tMINUTES\tSTATE\tINLET\tEXHAUST\tMASSFLO\tVACUUM\tIONPUMP\tCLI\tPROBE\tCCC1\tCCC2\tHEATER";
    private static final long TIME_BETWEEN_DATA_LINES = 5000;


    public CryoSocketControl(String host,
                             CryoPort socketType,
                             MessageIF cryoMsg,
                             String dataDir,
                             String logDir,
                             StatusManager statusManager) {
        m_dataDir = dataDir;
        m_logDir = logDir;
        m_cryoMsg = cryoMsg;
        m_socketType = socketType;
        populateStatusTable(STATUS_CODES);
        populateCommandMap(COMMAND_MAP);
        m_statusManager = statusManager;
        connect(host, socketType.getPort());
        if (socketType == CRYOBAY) {
            pollStatus(STATUS_PERIOD);
            getStatus= true;
        }
        String dateFmt = getDefaultDateFormat();
        m_cryoMsg.setDateFormat(dateFmt); // Set default date format
        //setDateFormat(STRING_DATES);
        m_cryoMsg.setUploadCount(0);
    }

    /**
     * Append a given string to a file, prepending a header line iff
     * the file is non-existent or empty, and appending a line separator.
     * @param path Pathname of the file.
     * @param buffer The string to write.
     * @param header The header line.
     * @return True if the write succeeded.
     */
    public static boolean appendLog(String path, String buffer, String header) {
        File file = new File(path);
        if (header != null && (!file.canWrite() || file.length() == 0)) {
            // Prepend the header to the stuff to write
            buffer = header + NL + buffer;
        }
        return appendLog(path, buffer);
    }

    /**
     * Append the given string to a file, appending a line separator.
     * Thread safe.
     * @param path Pathname of the file.
     * @param buffer The string to write.
     * @return True if the write succeeded.
     */
    public static synchronized boolean appendLog(String path, String buffer) {
        // Open a file and append the buffer
        File file = new File(path);
        if (!file.canWrite()) {
            file.delete();
        }
        PrintWriter out = null;
        boolean rtn = true;
        boolean append = true;
        try {
            out = new PrintWriter
            (new BufferedWriter(new FileWriter(file, append)));
            if (buffer.length() > 0) {
                if (buffer.charAt(buffer.length() - 1) != NL) {
                    out.print(buffer + NL);
                } else {
                    out.print(buffer);
                }
            }
        } catch (IOException ioe) {
            //m_cryoMsg.postError("TuneUtilities.appendLog: Error writing "
            //                    + path + ": " + ioe);
            rtn = false;
        } finally {
            try {
                out.close();
            } catch (Exception e) {}
        }
        return rtn;
    }

    /**
     * Read the contents of a file and return it as a list of lines.
     * @param filepath The filepath to read from.
     * @return The contents of the file; maybe empty, but never null.
     */
    public static List<String> readFileLines(String filepath) {
        ArrayList<String> lines = new ArrayList<String>();
        BufferedReader in = null;
        try {
            in = getReader(filepath, false);
            String line = null;
            while ((line = in.readLine()) != null) {
                lines.add(line);
            }
        } catch (Exception e) {
            // FIXME: m_cryoMsg.postError("Cannot read file: " + filepath);
        } finally {
            try { in.close(); } catch (Exception e) {}
        }
        return lines;
    }

    public static BufferedReader getReader(String filepath,
                                           boolean wantErrorMsg) {
        BufferedReader in = null;
        String errMsg = "(NULL file path)";
        if (filepath != null) {
            try {
                in = new BufferedReader(new FileReader(filepath));
            } catch (IOException ioe) {
                errMsg = ioe.toString();
            }
        }
        if (in == null && wantErrorMsg) {
            /*FIXME:*/ System.out.println("Cannot read the file \""
                               + filepath + "\": " + errMsg);
        }
        return in;
    }


    /* *****************Start of AutoSampler interface**************** */

//    /**
//     * Return the model number of the cryobay.
//     */
//
//    public String getName(){
//        return "CRYO";
//    }

    /** Open communication with the cryobay. 
     * @param host The host name or IP of the Cryobay.
     * @param port The port number to use.
     */
    public void connect(String host, int port){
        m_cryoMsg.postDebug("cryoinit",
                            "CryoSocketControl.connect(" + host
                            + ", " + port + ")");
        m_CryoSocket = new CryoSocket(host, port, m_cryoMsg);
        m_reader = new Reader(m_CryoSocket);
        m_reader.start();
    }

    public void pollStatus(int period_ms) {

        m_cryoMsg.postDebug("cryoinit",
                            "CryoSocketControl.pollStatus " + period_ms
                            + " ms");

        if (m_statusPoller != null) {
            // Stop any running status poller
            m_statusPoller.quit();
            m_cryoMsg.postDebug("cryoinit", "Quitting status poller");
        } else {
            m_cryoMsg.postDebug("cryoinit", "About to start status poller");
        }
        if (period_ms > 0) {
            m_statusPoller = new StatusPoller(period_ms);
            sendCommand("<VN>", PRIORITY3);
            m_statusPoller.start();
            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
            }
            sendCommand("<C_CAL>", PRIORITY3);
            m_cryoMsg.postDebug("cryoinit",
                                "Started status poller " + period_ms + " ms");
        } else {
            m_cryoMsg.postDebug("cryoinit",
                                "Status period was less than zero");
        }
    }

    public boolean wait(int time){
        try{
            Thread.sleep(time);
        } catch (Exception e){
            m_cryoMsg.postError("Cannot sleep");
            return false;
        }
        return true;
    }

    public boolean isConnected() {
        return m_CryoSocket != null && m_CryoSocket.isConnected();
    }

    /** Shut down communication with the autosampler. */
    public void disconnect() {
        m_cryoMsg.postDebug("cryoinit",
                            "CryoSocketControl.disconnect " + m_socketType);
        if (m_CryoSocket != null) {
            pollStatus(0);
            m_CryoSocket.disconnect();
        }
        if (m_sender != null) {
            m_sender.quit();
        }
        if (m_reader != null) {
            m_reader.quit();
        }
    }

    public String getState(){
    	if(status!=null) return status;
    	else return "NULL";
    }

    public boolean start() {
        sendCommand("<READY>");
        if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Sending: <READY>\n");
        return true;
    }

    public boolean stop() {
        sendCommand("<STOP>");
        if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Sending: <STOP>\n");
        return true;
    }

    public boolean detach() {
        sendCommand("<REMOVE>");
        if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Sending: <REMOVE>\n");
        return true;
    }

    public boolean thermalCycle() {
        sendCommand("<THERMALCYCLE>");
        if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Sending: <THERMALCYCLE>\n");
        return true;
    }

    public void stopSystem() {
        if(status.startsWith("IDLE") && getStatus)
            popupMsg("Air Off", "Turning off Cryobay eject air.", true);
        else if(status.startsWith("IDLE") && !getStatus)
            popupMsg("Turbo Pump",
                     "Turning off Turbo and closing valves.", true);
        else if( status.startsWith("THERM") )
            popupMsg("Stop Thermal Cycle",
                     "Do you want to stop and cool the probe?", true);
        else
            popupMsg("Warning!",
                     "Are you sure you want to warm the system?", true);
    }

    public void checkThermCycle() {
        popupMsg("Thermal Cycle",
                 "Are you sure you want to thermal cycle the probe?", true);
    }

    /**
     * Called whenever a full reply has been received from the CryoBay.
     * @param what The message string.
     * @param value True if the message needs to be processed;
     * set to false if this is just an acknowledgment.
     */
    protected void processReply(String what, boolean value) {
        m_cryoMsg.postDebug("cryomsg", "processReply: " + what);
        if (SwingUtilities.isEventDispatchThread()) {
            processReplyInEventThread(what, value);
        } else {
            SwingUtilities.invokeLater(new ReplyProcessor(what, value));
        }
    }

    /**
     * Does the work for processReply.
     * Called whenever a full reply has been received from the CryoBay.
     * @param what The message string.
     * @param value True if the message needs to be processed;
     * set to false if this is just an acknowledgment.
     * @see #processReply
     */
    private void processReplyInEventThread(String what, boolean value) {
        m_cryoMsg.postDebug("cryomsg", "CryoSocketControl.processReply("
                            + what + ", " + value + ")");
        if (what != null) {
            if (m_socketType == DATA){
                writeDataToFile(what);
            } else {
                if (!isNewFirmware() && what.startsWith("1")) {
                    writeDataToFile(what);
                }
                if(m_sender!=null)
                    m_sender.gotReply(value);
                processStatus(what, value);
            }
        }
    }

    public boolean sendCommand(String command) {
        return sendToCryoBay(command, PRIORITY2);
    }

    public boolean sendCommand(String command, Priority priority) {
        return sendToCryoBay(command, priority);
    }

    /**
     * Send a command to the cryobay.
     * @param command The string to send to the cryobay.
     * @param priority The priority level of the command.
     * @return True if successful.
     */
    synchronized public boolean sendToCryoBay(String command,
                                              Priority priority) {
        m_cryoMsg.postDebug("cryocmd",
                            "CryoSocketControl.sendCommand: " + command);
        if (m_sender == null) {
            m_sender = new Sender(null, m_retries);
            m_sender.start();
        }
        if (isConnected()) {
            m_sender.addCommand(command, priority);
                // TODO: (Java 5) Release semaphore here
                //m_sender.interrupt(); // Wake up the run() method
            if (m_isConnectError) {
                m_cryoMsg.postInfo(m_socketType + " connection is OK");
                m_isConnectError = false;
            }
            return true;
        } else {
            if (!m_isConnectError) {
                m_cryoMsg.postInfo( m_socketType + " is not connected");
                m_isConnectError = true;
            }
            return false;
        }
    }

    public boolean isVersionKnown() {
        return m_firmwareVN != null && m_firmwareVN.length() > 0;
    }

    public void setVersionKnown(boolean b) {
        m_versionKnown = b;
    }

    /**
     * Determine if the given message is legal to send.
     * Null messages will be rejected.
     * If this is for the CryoBay port,
     * also checks a list of allowed messages for the detected
     * version of the firmware.
     * @param msg The message to check.
     * @return True if the message is OK to send.
     */
    public boolean isMessageAllowed(String msg) {
        boolean ok = (msg != null);
        if (m_socketType == CRYOBAY) {
            String cmd = null;
            if (ok) {
                StringTokenizer toker = new StringTokenizer(msg, " <>");
                if (toker.hasMoreTokens()) {
                    cmd = toker.nextToken();
                }
            }
            if (ok && cmd != null) {
                ok = isCommandAllowedFor(cmd, m_firmwareVN);
            }
            m_cryoMsg.postDebug("cryocmd",
                                "isMessageAllowed(" + cmd + ") = " + ok);
        }
        if (!ok) {
            m_cryoMsg.postWarning("COMMAND REJECTED: " + msg);
        }
        return ok;
    }

    /**
     * Determine if the given message is legal to send to the CryoBay.
     * Checks a list of allowed messages for the given
     * version of the firmware.
     * @param cmd The message to check.
     * @param version The firmware version to check against.
     * @return True if the message is OK to send.
     */
    private boolean isCommandAllowedFor(String cmd, String version) {
        boolean ok = false;
        if (cmd != null) {
            // Check for special cases (good for all firmware versions)
            ok = cmd.matches(m_specialCmdRegex);

            if (!ok) {
                // Check if it's in the list
                String requiredVersion = m_commandMap.get(cmd);
                if (requiredVersion == null) {
                    // It's not in the list at all
                    ok = false;
                } else {
                    // It's in the list, but...
                    // check if our firmware is up to required version
                    ok = version.compareTo(requiredVersion) >= 0;
                }
            }
        }
        return ok;
    }

    /**
     * Check the firmware version to see if the firmware sends the date as a
     * Unix time-stamp in log messages.
     * The version string is in the form
     * <CODE>
     * JVyyyyMMMdd
     * </CODE>
     * or
     * <CODE>
     * JVyyyy-mm-dd
     * </CODE>
     * Where yyyy is the year, MMM is the month abbreviation, mm is the month
     * number, and dd is the day.
     * Versions after JV2010... use a Unix time-stamp.
     * Earlier versions have a buggy minute counter.
     * @return True if there is a reliable Unix time-stamp.
     */
    private boolean isNewFirmware() {
        return m_firmwareVN.compareTo("JV2010") >= 0;
    }

    /** send String directly to the cryobay
     * @param cmd as a string
     * @return boolean if sent correctly
     */
    public boolean send(String cmd) {
        if (this.isConnected()) {
            if (m_isConnectError) {
                m_cryoMsg.postInfo(m_socketType + " connection OK");
                m_isConnectError = false;
            }
            m_cryoMsg.postDebug("cryocmd",
                                "CryoSocketControl.send(\"" + cmd + "\")");
            m_CryoSocket.write(cmd.getBytes());
            return true;
        } else {
            if (!m_isConnectError) {
                m_cryoMsg.postError(m_socketType + " not connected");
                m_isConnectError = true;
            }
            return false;
        }
    }

    /**
     * Send the byte array to the CryoBay.
     * @param par as byte array.
     * @return true if successful.
     */
    public boolean send(byte[] par) {
        if (this.isConnected()) {
            m_CryoSocket.write(par);
            //m_cryoMsg.postDebug("cryocmd",
            //                    "Sent to CryoBay: " + arrayToHexString(par));
            return true;
        } else {
            return false;
        }
    }



    /**
     * This method is called whenever a message is received from
     * the data port.
     * @param msg The message.
     */
    protected void writeDataToFile(String msg) {
        m_cryoMsg.postDebug("cryodata",
                            "writeDataToFile: " + msg);
        if (msg.startsWith("1")) {
            msg = "FAULT: 0 " + msg;
        }
        if (msg.startsWith("UPLOAD")) {
            m_uploadDataPath  = getDataPath(true);
            m_cryoMsg.postDebug("cryodata",
                                "m_uploadDataPath=" + m_uploadDataPath);
            if (!isNewFirmware()) {
                startUploadTimer();
                //m_uploading  = System.currentTimeMillis();
                m_currentUpload = new ArrayList<String>();
            }
        } else if (msg.matches(".?DATA.*") || msg.matches(".?FAULT.*")) {
            m_cryoMsg.postDebug("cryodata", "Matching msg=" + msg);
            // NB: Old firmware (pre 2010) uploads identical messages for
            // the periodic uploads and the explicitly requested uploads.
            // However, it precedes a series of uploaded messages with an
            // "UPLOAD" acknowledgment reply that we use to start the upload
            // timer and guess that messages that come in soon after are
            // explicit uploads.
            // Later firmware (2010 and later) prepends a "U" to explicitly
            // uploaded messages and does not send the "UPLOAD" reply.
            // (That is, it doesn't send it to the "data" socket.)
            boolean isUpload = msg.startsWith("U") || isUploadTimerRunning();

            String cleanMsg = prettifyMessage(msg, isUpload);

            if (isUploadTimerRunning()) {
                // Support for old firmware -- uploading w/o "U" flag
                m_cryoMsg.postDebug("cryodata",
                                    "Uploading: delta="
                                    + (CryoUtils.timeMs() - m_uploading));
                if (m_currentUpload == null) {
                    m_currentUpload = new ArrayList<String>();
                }
                startUploadTimer();
                m_cryoMsg.setUploadCount(++m_uploadCount);
                m_currentUpload.add(cleanMsg);
            } else if (isVersionKnown()){
                writeCurrentUpload();
                String hdr = isNewFirmware() ? DATA_HDR : LEGACY_DATA_HDR;
                String path = isUpload ? m_uploadDataPath : getDataPath(false);
                m_cryoMsg.setUploadCount(++m_uploadCount);
                appendLog(path, cleanMsg, hdr);
            }
        }
    }

    private String getDataPath(boolean isUpload) {
        String path = m_logDir +  "/" + dateMonthFmt.format(new Date());
        if (isUpload && !isUploadRequested()) {
            // This is an automatic upload triggered by a fault (not requested)
            path += "_fault";
        } else if (isUpload) {
            // Check for user defined location
            path = getUploadFilePath();
            if (path == null) {
                // No user-defined file path; make up a name in user dir
                String name = File.separator + dateTTimeFmt.format(new Date());
                path = m_dataDir + name;
                appendLog(path, ""); // Try to create the file
                if (!new File(path).exists()) {
                    // Couldn't make that file; probably an illegal name ...
                    // (Windows won't allow ":" in a file name.)
                    // Can't do this to the whole path, as Windows directory
                    // requires a ":" in it.
                    name = name.replace(':', '.');
                    path = m_dataDir + name;
                }
            }
        }
        return path;
    }

    private String prettifyMessage(String msg, boolean isUpload) {
        if (isNewFirmware() || msg.startsWith("U")) {
            return prettifyNewMessage(msg, isUpload);
        } else {
            return prettifyOldMessage(msg, isUpload);
        }
    }

    private String prettifyNewMessage(String msg, boolean isUpload) {
        boolean isFault = msg.matches(".?FAULT.*");
        if (msg.startsWith("U")) {
            msg = msg.substring(1);
        }
        String[] tokens = msg.split("[\t ]+");
        long t_sec = Long.parseLong(tokens[1]);
        // Correct for any wrap-around of the SBC's 32-bit date
        long now = CryoUtils.timeMs();
        t_sec = CryoUtils.fixDateOverflow(t_sec, now);

        // If we're not manually uploading, force string dates.
        String dateFormat = isUpload ? null : STRING_DATES_LOCAL;
        // Change the token that has the time stamp:
        tokens[1] = formatDateStamp(t_sec, dateFormat, "\t");
        String prettyMsg = "";
        for (int i = 0; i < tokens.length; i++) {
            prettyMsg += tokens[i];
            if (i < tokens.length - 1) {
                if (isFault && i > 1) {
                    prettyMsg += " ";
                } else {
                    prettyMsg += "\t";
                }
            }
        }
        return prettyMsg;
    }

    /**
     * Produce a formatted date string based on a Unix time stamp.
     * A time zone code is included at the end, separated from the date/time
     * part of the string by the specified separator.
     * The reqFormat must be one of:
     * <br>STRING_DATES
     * <br>STRING_DATES_LOCAL
     * <br>EXCEL_DATES
     * <br>EXCEL_DATES_LOCAL
     * <br>UNIX_DATES
     * <br>
     * @param t_sec The time stamp (seconds since beginning of 1970).
     * @param reqFormat The string code for the desired DateFormat. 
     * @param separator The separator to use between the date/time and the
     * time zone code.
     * @return The formatted date string.
     */
    private String formatDateStamp(long t_sec,
                                   String reqFormat, String separator) {
        if (t_sec == 0) {
            return blankDate + "\t   ";
        }
        String strDate;
        if (reqFormat == null) {
            reqFormat = getDateFormat();
        }
        Date date = new Date(t_sec * 1000);
        if (reqFormat.equals(STRING_DATES_LOCAL)) {
            strDate = date_TimeFmt.format(date);
        } else if (reqFormat.equals(STRING_DATES)) {
            strDate = date_TimeFmtGmt.format(date);
        } else if (reqFormat.equals(EXCEL_DATES)) {
            long dosTime = t_sec - DOS_TO_UNIX_DATE_OFFSET;
            strDate = getExcelDay(dosToMillis(dosTime), true);
        } else if (reqFormat.equals(EXCEL_DATES_LOCAL)) {
            long dosTime = t_sec - DOS_TO_UNIX_DATE_OFFSET;
            long tms = t_sec * 1000;
            int offset = localCalendar.getTimeZone().getOffset(tms);
            dosTime += offset / 1000;
            strDate = getExcelDay(dosToMillis(dosTime), true);
        } else {
            strDate = i10FmtPadded.format(t_sec);
        }
        strDate += separator;
        if (reqFormat.equals(STRING_DATES_LOCAL)
            || reqFormat.equals(EXCEL_DATES_LOCAL))
        {
            strDate += timeZoneFmt.format(date);
        } else {
            strDate += timeZoneFmtGmt.format(date);
        }
        return strDate;
    }

    public String getDefaultDateFormat() {
        String fmt = STRING_DATES_LOCAL;
        List<String> lines = readFileLines(getDateFormatPath());
        if (lines.size() > 0 && lines.get(0).length() > 0) {
            fmt = lines.get(0);
        }
        return fmt;
    }

    private void setDefaultDateFormat(String fmt) {
        String path = getDateFormatPath();
        new File(path).delete();
        appendLog(path, fmt);
        hideFile(path);
    }

    private void hideFile(String path) {
        // TODO: in Java 7 use File.toPath().setAttribute("dos:hidden", true)
        if (!isUnix()) {
            try {
                Runtime.getRuntime().exec("attrib +h " + path);
            } catch (IOException e) {
            }
        }
    }

    private static boolean isUnix() {
        String os = System.getProperty("os.name").toLowerCase();
        //linux or unix
        return (os.indexOf( "nix") >= 0 || os.indexOf( "nux") >= 0);
    }

    private String getDateFormatPath() {
        return m_dataDir + File.separator + ".DateFormat";
    }

    private String getDateFormat() {
        return m_dateFormat;
    }

    public void setDateFormat(String fmt) {
        if (!fmt.equals(m_dateFormat)) {
            m_dateFormat = fmt;
            setDefaultDateFormat(fmt);
            m_cryoMsg.setDateFormat(fmt);
        }
    }

    private String prettifyOldMessage(String msg, boolean isUpload) {
        boolean isFault = msg.matches("FAULT.*");
        String[] tokens = msg.split("[ ]+");
        long t_sec = CryoUtils.timeMs() / 1000;
        if (isFault) {
            // NB: Overflowed fault times are the negative of the actual time!
            // Overflowed data times are 0.
            // (A compiler bug in Dynamic C 9.21)
            try {
                float time = Float.parseFloat(tokens[1]);
                time = Math.abs(time);
                tokens[1] = Fmt.f(1, time, false);
            } catch (NumberFormatException nfe) {
            }
        }
        String prettyMsg;
        // If we're not manually uploading, force string dates.
        String fmt = isUploadRequested() ? null : STRING_DATES_LOCAL;
        String uploadDate = formatDateStamp(t_sec, fmt, "\t");
        String separator = "\t";
        prettyMsg = uploadDate + separator; // Our new date tag for upload time
        prettyMsg += tokens[0] + separator; // "DATA" or "FAULT"
        prettyMsg += tokens[1] + separator; // "Minutes" date from firmware
        if (isFault) {
            separator = " ";
            tokens[2] = tokens[2].replace(":", ": ");
        }
        for (int i = 2; i < tokens.length; i++) {
            prettyMsg += tokens[i];
            if (i < tokens.length - 1) {
                prettyMsg += separator;
            }
        }
        return prettyMsg;
    }

    private void startUploadTimer() {
        if (m_uploadTimer == null) {
            ActionListener upload = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    m_cryoMsg.postDebug("CryoSocketControl.replyTimeout");
                    writeCurrentUpload();
                    setUploadRequested(false);
                }
            };
            m_uploadTimer = new Timer((int)TIME_BETWEEN_DATA_LINES, upload);
            m_uploadTimer.setRepeats(false);
        }
        if (m_uploadTimer.isRunning()) {
            m_cryoMsg.postDebug("cryodata", "Restarting uploadTimer");
            m_uploadTimer.restart();
        } else {
            m_cryoMsg.postDebug("cryodata", "Starting uploadTimer");
            m_uploadTimer.start();
        }
    }

    private void stopUploadTimer() {
        if (m_uploadTimer != null) {
            m_cryoMsg.postDebug("cryodata", "Stopping uploadTimer: isRunning="
                                + m_uploadTimer.isRunning());
            m_uploadTimer.stop();
            setUploadRequested(false);
        }
    }

    private boolean isUploadTimerRunning() {
        return (m_uploadTimer != null && m_uploadTimer.isRunning());
    }

    synchronized private void writeCurrentUpload() {
        m_cryoMsg.postDebug("cryodata", "writeCurrentUpload:"
                            + " m_uploadDataPath=" + m_uploadDataPath
                            + ", m_currentUpload=" + m_currentUpload
                            );
        if (m_currentUpload == null || m_uploadDataPath == null) {
            return;
        }
        stopUploadTimer();

        // Read old upload into array if there is a recent one
        long oldTime = new File(m_uploadDataPath).lastModified();
        List<String> oldUpload = null;
        if (CryoUtils.timeMs() - oldTime < 60000 * 60 * 24 * 5) {
            oldUpload = readFileLines(m_uploadDataPath);
        }

        // Cull out the lines that are already recorded
        cullDuplicates(m_currentUpload, oldUpload);

        StringBuffer newLines = new StringBuffer();
        for (String dataline : m_currentUpload) {
            newLines.append(dataline).append("\n");
        }
        appendLog(m_uploadDataPath, newLines.toString(), LEGACY_DATA_HDR);
        m_currentUpload = null;
    }

    /**
     * Remove lines from the "newLines" list that are already in the
     * "oldLines" list.
     * Both lists are assumed chronological.
     * We remove the maximum initial lines from the "newLines" list that
     * match the same number of trailing lines in the "oldLines" list.
     * The initial two (tab delimited) tokens are ignored when comparing lines.
     * @param newLines The list of "newLines".
     * @param oldLines The list of "oldLines".
     */
    private void cullDuplicates(List<String> newLines, List<String> oldLines) {
        // Look for a new line that matches the last old line
        if (oldLines == null) {
            return;
        }
        int oldLen = oldLines.size();
        if (oldLen > 0) {
            String oldLine = oldLines.get(oldLen - 1);
            int iMatch = newLines.size();
            while ((iMatch= findMatchingLine(oldLine, newLines, iMatch)) >= 0) {
                if (isGoodMatch(iMatch, newLines, oldLines)) {
                    break;
                }
            }
            newLines.subList(0, iMatch + 1).clear();
        }
    }

    /**
     * Given that newLines[iMatch] matches the last line in the oldLines list,
     * checks if all the lines 0 through (iMatch - 1) match the corresponding
     * lines in the oldLines list.
     * The first two (tab delimited) tokens are ignored when comparing lines.
     * @param iMatch The index of the matching line in the newLines list.
     * @param newLines The list of new lines.
     * @param oldLines The list of old lines, the tail of which may contain
     * some initial number of lines in the new list.
     * @return True iff all the lines that should match do in fact match.
     */
    private boolean isGoodMatch(int iMatch, List<String> newLines,
                                List<String> oldLines) {
        boolean ok = true;
        int i = iMatch - 1; // Assume newLines[iMatch] matches last oldLines
        int j = oldLines.size() - 2; // Start checking at next-to-last oldLines
        if (i > j) {
            // Require all new lines (below iMatch) to have a match in old list
            ok = false;
        }
        for ( ; ok && i >= 0; --i, --j) {
            String line1 = newLines.get(i);
            String[] toks1 = line1.split("\t", 3);
            if (toks1.length != 3) {
                ok = false;
            } else {
                String line2 = oldLines.get(j);
                String[] toks2 = line2.split("\t", 3);
                if (toks2.length != 3) {
                    ok = false;
                } else {
                    ok = toks1[2].equals(toks2[2]);
                }
            }
        }
        return ok;
    }

    /**
     * Searches for a given line in list of lines.
     * The initial two (tab delimited) tokens are ignored when comparing lines.
     * @param line The line to search for.
     * @param lines The list of lines to search.
     * @param idx Search downwards in "lines" from idx-1.
     * @return The index of the first match to "line", or -1.
     */
    private int findMatchingLine(String line, List<String> lines, int idx) {
        int rtn = -1;
        String[] toks = line.split("\t", 3);
        if (toks.length == 3) {
            String oldBaseLine = toks[2];
            for (int i = idx - 1; i >= 0; --i) {
                String[] newToks = lines.get(i).split("\t", 3);
                if (newToks.length == 3 && newToks[2].equals(oldBaseLine)) {
                    // Lines are identical, ignoring first 2 tokens
                    rtn = i;
                    break;
                }
            }
        }
        m_cryoMsg.postDebug("cryodata", "findMatchingLine returning " + rtn);
        return rtn;
    }

    private long dosToMillis(long dosSeconds) {
        long millis = 1000 * (dosSeconds + DOS_TO_UNIX_DATE_OFFSET);
        return millis;
    }

    private String getExcelDay(long time, boolean isPadded) {
        double excelDate = UNIX_TO_EXCEL + (double)time / (1000 * 60 * 60 * 24);
        if (isPadded) {
            return excelFmtPadded.format(excelDate);
        } else {
            return excelFmt.format(excelDate);
        }
    }

    /**
     * This method is called whenever a status message is received from
     * the CryoBay.
     * @param msg The status message to process.
     * @param iValue If false, the message is not processed.
     */
    protected void processStatus(String msg, boolean iValue) {
        StringTokenizer toker;
        float temp;


        if (!iValue) {
            // This is just an acknowledge
            return;
        }
        //String key = new String(iWhat);

        if(msg!=null)
            toker = new StringTokenizer(msg, " \t>");
        else return;

        m_cryoMsg.postDebug("cryocmd",
                            "CryoSocketControl got from cryobay: " + msg);

        String cmd;
        if (toker.hasMoreTokens()){
            cmd= toker.nextToken();
        }else{
            cmd= " ";
        }
        m_cryoMsg.postDebug("cryomsg", "CryoSocketControl cmd: " + cmd);

        final int MAX_VALUES = 3;
        int nTokens = toker.countTokens();
        String[] values = new String[Math.max(MAX_VALUES, nTokens)];
        for (int i = 0; i < MAX_VALUES; i++) {
            if (i < nTokens) {
                values[i] = toker.nextToken();
                m_cryoMsg.postDebug("cryomsg",
                                    "CryoSocketControl values[" + i + "]: "
                                    + values[i]);
            } else {
                values[i] = " ";
            }
        }

        if (cmd.startsWith("<GETSTATE")) {
            status = m_statusNames.get(Integer.parseInt(values[0]));
            m_cryoMsg.postDebug("cryomsg",
                                "CryoSocketControl status: " + status);
            if(getStatus) {
                m_statusManager.processStatusData("cryostatus " + status);
            }
            setEnableButtons(status);
        } else if (cmd.startsWith("<C_1")) {
            temp = Float.parseFloat(values[0]);
            if(temp<=0) values[0]= "N/A";
            m_statusManager.processStatusData("cryotemp " + values[0]);
        } else if (cmd.startsWith("<C_3")) {
            temp = Float.parseFloat(values[0]);
            if(temp<=0) values[0]= "N/A";
            m_statusManager.processStatusData("cryoccc1 " + values[0]);
        } else if (cmd.startsWith("<C_4")) {
            temp = Float.parseFloat(values[0]);
            if(temp<=0) values[0]= "N/A";
            m_statusManager.processStatusData("cryoccc2 " + values[0]);
        } else if (cmd.startsWith("<C_HEAT")) {
            temp = Float.parseFloat(values[0]);
            if(temp<0) values[0]= "N/A";
            m_statusManager.processStatusData("cryoheater " + values[0]);
        } else if (cmd.startsWith("<C_COLD")) {
            m_statusManager.processStatusData("cryosetpoint " + values[0]);
        } else if (cmd.startsWith("<AN0")) {
            m_statusManager.processStatusData("cryoinlet " + values[0]);
        } else if (cmd.startsWith("<AN1")) {
            m_statusManager.processStatusData("cryoexhaust " + values[0]);
        } else if (cmd.startsWith("<AN2")) {
            m_statusManager.processStatusData("cryomass " + values[0]);
        } else if (cmd.startsWith("<AN3")) {
            m_statusManager.processStatusData("cryovacuum " + values[0]);
        //}  else if (cmd.startsWith("<AN4")) {
            //m_statusManager.processStatusData("cryocli " + values[0]);
        } else if (cmd.startsWith("<AN5")) {
            m_statusManager.processStatusData("cryocli " + values[0]);
        //}  else if (cmd.startsWith("<C_WARM")) {
            //SDM Setpoint m_statusManager.processStatusData("cryosetpoint "
                                                             //+ values[0]);
        } else if (cmd.startsWith("<NMR")) {
            m_cryoMsg.postDebug("cryomsg", "Got NMR: " + cmd);
            if(values[0].startsWith("1")){
                m_cryoMsg.postDebug("cryomsg", "Stopping NMR...");
                if(m_cryoMsg!= null) m_cryoMsg.stopNMR();
            }
        } else if (cmd.startsWith("<READY")) {
            if(m_cryoMsg!= null) {
                m_cryoMsg.writeAdvanced("Received: " + msg + "\n");
            }
            if(values[0].startsWith("ACK")){
                m_statusManager.processStatusData("cryostatus VACUUM PUMPING");
            }else if (values[0].startsWith("NACK")){
                popupMsg("Start", "Cannot start. System not warm", false);
            }
        } else if (cmd.startsWith("<VN")) {
            m_cryoMsg.postDebug("cryomsg",
                                "CryoSocketControl version: " + values[0]);
            if(m_cryoMsg!= null) {
                m_cryoMsg.writeAdvanced("Received: " + msg + "\n");
            }
            if (!m_versionKnown) {
                // First time getting version: do initialization
                initVersion(values[0]);
            }
            if (m_requestedVersionPopup) {
                requestVersionPopup(false);
                popupMsg("About CryoBay",
                         "CryoBay firmware version: " + values[0], false);
            }
        } else if (cmd.startsWith("<GM_HOURS")) {
            m_cryoMsg.postDebug("cryomsg",
                                "CryoSocketControl gm hours: " + values[0]);
            popupMsg("Service Counter",
                     "Counter Hours since last reset: " + values[0], false);
            if(m_cryoMsg!= null) {
                m_cryoMsg.writeAdvanced("Received: " + msg + "\n");
            }
        } else if (cmd.startsWith("<C_CAL")) {
            m_calibration  = (values[0] + values[1] + values[2]).trim();
            m_cryoMsg.postDebug("cryomsg",
                                "CryoSocketControl cryocalibration: "
                                + m_calibration);
            m_statusManager.processStatusData("cryocal " + m_calibration);
            if(m_cryoMsg!= null) {
                m_cryoMsg.writeAdvanced("Received: " + msg + "\n");
            }
        } else if (cmd.startsWith("<PROBETYPE")) {
            m_cryoMsg.postDebug("cryocmd",
                                "CryoSocketControl cryocalibration: "
                                + values[0]);
            //m_statusManager.processStatusData("probetype "
                                                //+ values[0] + values[1]
                                                //+ value3);
            if(m_cryoMsg != null) m_cryoMsg.writeAdvanced("Received: " + msg
                                                          + "\n");
            m_probeType= Integer.parseInt(values[0]);
        } else if (cmd.startsWith("1") && m_socketType != TEMPCTRL) {
            if(m_cryoMsg != null) {
                m_cryoMsg.writeAdvanced("Received: " + msg + "\n");
                m_cryoMsg.postError("\n" + msg);
            }
        } else if (cmd.startsWith("2") && m_socketType != TEMPCTRL) {
            if(m_cryoMsg != null) {
                m_cryoMsg.writeAdvanced("Received: " + msg + "\n");
                if(m_cryoMsg != null)
                    m_cryoMsg.postError(msg); // Should this be a popup?
            }
        } else if (cmd.startsWith("<REMOVE")) {
            if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Received: " + msg
                                                         + "\n");
            if(values[0].startsWith("ACK")) {
                getStatus=true;
                m_statusManager.processStatusData("cryostatus IDLE");
                popupMsg("Detach probe", "OKay to Detach Probe", false);
            } else if(values[0].startsWith("...")){
                m_statusManager.processStatusData("cryostatus DETACHING PROBE");
                getStatus=false;
            } else {
                getStatus=true;
                m_statusManager.processStatusData("cryostatus IDLE");
                popupMsg("Detach probe", "Detach Probe Failed!", false);
            }
        } else if (cmd.startsWith("<V_PURGE")) {
            if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Received: " + msg
                                                         + "\n");
            if(values[0].startsWith("ACK")) {
                getStatus=true;
                m_statusManager.processStatusData("cryostatus IDLE");
                popupMsg("Vacuum Purge", "Vacuum Purge Complete", false);
            } else if(values[0].startsWith("...")){
                m_statusManager.processStatusData("cryostatus VACUUM PURGING");
                getStatus=false;
            } else {
                getStatus=true;
                m_statusManager.processStatusData("cryostatus IDLE");
                popupMsg("Vacuum Purge", "Vacuum Purge Failed!", false);
            }
        } else if (cmd.startsWith("<V_PROBE")) {
            if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Received: " + msg
                                                         + "\n");
            if(values[0].startsWith("ACK")) {
                getStatus=true;
                m_statusManager.processStatusData("cryostatus IDLE");
                popupMsg("Pumping probe", "Probe Pumping Complete", false);
            } else if(values[0].startsWith("...")){
                m_statusManager.processStatusData("cryostatus PROBE PUMPING");
                getStatus=false;
            } else {
                getStatus=true;
                m_statusManager.processStatusData("cryostatus IDLE");
                popupMsg("Pumping probe", "Probe Pumping Failed!", false);
            }
        } else if (cmd.startsWith("<THERMALCYCLE")) {
            if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Received: " + msg
                                                         + "\n");
            if(values[0].startsWith("ACK")){
                m_statusManager.processStatusData("cryostatus THERMAL CYCLE");
            }else if (values[0].startsWith("NACK")){
                popupMsg("Thermal Cycle",
                         "Must be OPERATING with Xsense probe.", false);
            }
        } else if (cmd.startsWith("<STOP")) {
            if(m_cryoMsg!= null) m_cryoMsg.writeAdvanced("Received: " + msg
                                                         + "\n");
            //if(values[0].startsWith("ACK")) popupMsg("Warming",
                                                       //"Starting Warm Up",
                                                       //false);
            //else if(values[0].startsWith("NACK")) popupMsg("Warming",
                                            //"Cannot warm system.", false);
            if(values[0].startsWith("ACK")){
                if(status.startsWith("THERM")){
                    m_statusManager.processStatusData("cryostatus COOLING");
                } else{
                    m_statusManager.processStatusData("cryostatus WARMING");
                }
            } else if(values[0].startsWith("PUMPING")){
                getStatus=true;
                m_statusManager.processStatusData("cryostatus IDLE");
            }
        } else {
            if (cmd.startsWith("<GETDATE")) {
                long date_s  = Long.parseLong(values[0]);
                date_s = CryoUtils.fixDateOverflow(date_s, CryoUtils.timeMs());
                m_date_ms = date_s * 1000;
                Date date = new Date(m_date_ms);
                String strDate = date_TimeFmt.format(date);
                String tz = timeZoneFmt.format(date);
                msg = "<GETDATE " + strDate + " " + tz + ">";
            }
            if(m_cryoMsg!= null) {
                String source = "";
                if (m_socketType != CRYOBAY) {
                    source = " from " + m_socketType;
                }
                m_cryoMsg.writeAdvanced("Received: " + msg + source);
            }
        }
    }

    /**
     * Save the firmware version and
     * perform any initialization that requires knowledge of the version.
     * Viz:
     * <UL>
     * <LI>Get the probe ID.
     * </UL>
     * @param version The version string from the firmware.
     */
    private void initVersion(String version) {
        m_firmwareVN = version;
        m_versionKnown  = true;
        String cmd = "<PROBETYPE>";
        if (isMessageAllowed(cmd)) {
            sendToCryoBay(cmd, PRIORITY2);
        }
        if (isCommandAllowedFor("SETDATE", m_firmwareVN)) {
            new DateSetter().start();
        }
    }

    private void setEnableButtons(String status){

        boolean send = m_cryoMsg != null && m_cryoMsg.getCryobay() != null;
        m_cryoMsg.setCryoSendEnabled(send);
        m_cryoMsg.setThermSendEnabled(send);

        if(status.startsWith("IDLE")){
            m_cryoMsg.postDebug("cryostatus",
            "status is idle, setting buttons");
            if(m_cryoMsg!=null){
                m_cryoMsg.setStopEnabled(true);
                m_cryoMsg.setStartEnabled(getStatus);
                m_cryoMsg.setDetachEnabled(getStatus);
                m_cryoMsg.setVacPurgeEnabled(getStatus);
                m_cryoMsg.setPumpProbeEnabled(getStatus);
                m_cryoMsg.setThermCycleEnabled(false);
            }
        } else if(status.startsWith("VACUUM PUMPING")){
            if(m_cryoMsg!=null){
                m_cryoMsg.setStopEnabled(true);
                m_cryoMsg.setStartEnabled(false);
                m_cryoMsg.setDetachEnabled(false);
                m_cryoMsg.setVacPurgeEnabled(false);
                m_cryoMsg.setPumpProbeEnabled(false);
                m_cryoMsg.setThermCycleEnabled(false);
            }
        } else if(status.startsWith("COOLING")){
            if(m_cryoMsg!=null){
                m_cryoMsg.setStopEnabled(true);
                m_cryoMsg.setStartEnabled(false);
                m_cryoMsg.setDetachEnabled(false);
                m_cryoMsg.setVacPurgeEnabled(false);
                m_cryoMsg.setPumpProbeEnabled(false);
                m_cryoMsg.setThermCycleEnabled(false);
            }
        } else if(status.startsWith("OPERATING")){
            if(m_cryoMsg!=null){
                m_cryoMsg.setStopEnabled(true);
                m_cryoMsg.setStartEnabled(false);
                m_cryoMsg.setDetachEnabled(false);
                m_cryoMsg.setVacPurgeEnabled(false);
                m_cryoMsg.setPumpProbeEnabled(false);
                if(m_probeType==1)
                    m_cryoMsg.setThermCycleEnabled(true);
                else
                    m_cryoMsg.setThermCycleEnabled(false);
            }
        } else if(status.startsWith("THERMAL CYCLE")){
            if(m_cryoMsg!=null){
                m_cryoMsg.setStopEnabled(true);
                m_cryoMsg.setStartEnabled(false);
                m_cryoMsg.setDetachEnabled(false);
                m_cryoMsg.setVacPurgeEnabled(false);
                m_cryoMsg.setPumpProbeEnabled(false);
                m_cryoMsg.setThermCycleEnabled(false);
            }
        } else if(status.startsWith("WARMING")){
            if(m_cryoMsg!=null){
                m_cryoMsg.setStopEnabled(false);
                m_cryoMsg.setStartEnabled(false);
                m_cryoMsg.setDetachEnabled(false);
                m_cryoMsg.setVacPurgeEnabled(false);
                m_cryoMsg.setPumpProbeEnabled(false);
                m_cryoMsg.setThermCycleEnabled(false);
            }
        } else if(status.startsWith("SAFE")){
            if(m_cryoMsg!=null){
                m_cryoMsg.setStopEnabled(false);
                m_cryoMsg.setStartEnabled(false);
                m_cryoMsg.setDetachEnabled(false);
                m_cryoMsg.setVacPurgeEnabled(false);
                m_cryoMsg.setPumpProbeEnabled(false);
                m_cryoMsg.setThermCycleEnabled(false);
            }
        }
    }

    private void populateStatusTable(Object[][] entries) {
        int size = entries.length;
        for (int i = 0; i < size; i++) {
            m_statusNames.put((Integer)entries[i][0], (String)entries[i][1]);
        }
    }

    private void populateCommandMap(String[][] entries) {
        for (String[] entry : entries) {
            m_commandMap.put(entry[0], entry[1]);
        }
    }

    /**
     * Display a modal popup dialog with given title and message.
     * An "OK" button dismisses the dialog.
     * @param title The title of the popup dialog.
     * @param msg The message string displayed in the popup.
     * @param isCancelable If true, a "Cancel" button is displayed, as well
     * as the "OK" button.
     */
    public void popupMsg(String title, String msg, boolean isCancelable) {
        final JDialog warn;
        JButton ok, cancel;
        JLabel message;

        Window frame = m_cryoMsg.getFrame();
        warn = new JDialog(frame, title);
        warn.getContentPane().setLayout(new BorderLayout());

        String tag = msg;
        if (title.equalsIgnoreCase("Service Counter")) {
            tag = title;
        }
        JDialog prevDialog = m_dialogsShowing.remove(tag);
        if (prevDialog != null) {
            prevDialog.dispose();
        }
        m_dialogsShowing.put(tag, warn);

        message = new JLabel(msg);
        message.setBorder(BorderFactory.createMatteBorder(10, 10, 10, 10,
                                                          (Color)null));
        ok = new JButton("OK");
        cancel = new JButton("Cancel");

        if (isCancelable) {
            if (title.startsWith("Thermal Cycle")){
                ok.setActionCommand("THERMCYCLE");
            } else {
                ok.setActionCommand("STOP");
            }
        } else {
            ok.setActionCommand("");
        }
        cancel.setActionCommand("Cancel");

        warn.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent ev) {
                warn.dispose();
            }
        });

        ok.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                String cmd = ae.getActionCommand();
                warn.dispose();
                if (cmd.startsWith("THERMCYCLE")) {
                    thermalCycle();
                } else if (cmd.startsWith("STOP")) {
                    stop();
                }
            }
        } );

        cancel.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                warn.dispose();
            }
        } );


        warn.getContentPane().add(message, BorderLayout.CENTER);
        JPanel southPanel = new JPanel();
        warn.getContentPane().add(southPanel, BorderLayout.SOUTH);
        southPanel.add(ok);
        if (isCancelable){
            southPanel.add(cancel);
        }

        //warn.setSize(300, 100);
        warn.setUndecorated(true);
        warn.getRootPane().setWindowDecorationStyle(JRootPane.WARNING_DIALOG);
        warn.setResizable(false);
        warn.setVisible(true);
        warn.pack();
        warn.setLocationRelativeTo(m_cryoMsg.getFrame());
    }

    /**
     * 
     * @param msg The message to display.
     * @param title The title string for the dialog.
     * @param optType An integer disignating the options available on the
     * dialog: YES_NO_OPTION, YES_NO_CANCEL_OPTION, or OK_CANCEL_OPTION.
     * @param msgType An integer designating the kind of message this is;
     * primarily used to determine the icon from the pluggable Look and Feel:
     * ERROR_MESSAGE, INFORMATION_MESSAGE, WARNING_MESSAGE, QUESTION_MESSAGE,
     * or PLAIN_MESSAGE.
     * @return An integer indicating the option selected by the user.
     */
    private int showConfirmDialog(String msg,
                                  String title,
                                  int optType,
                                  int msgType) {
        Window f = m_cryoMsg.getFrame();
        return JOptionPane.showConfirmDialog(f, msg, title, optType, msgType);
    }

    /**
     * Blocks until a message is received from the Cryobay with the
     * PFC set to "key".
     * @return The value in the response.
     */
    protected boolean waitForConfirmation(int key) {
        boolean result = false;
        while (!result) {
            boolean nResult = m_resultTable.remove(key);
            /*if (nResult == null) {
                // TODO: (Java 5) Use semaphore
                try {
                    Thread.sleep(50);
                } catch (InterruptedException ie) {
                }
            } else {*/
                result = nResult;
           // }
        }
        return result;
    }

    private String lastMinuteEdits(String cmd) {
        if (m_socketType == CRYOBAY) {
            m_cryoMsg.postDebug("editcryocmd",
                                "Edit: \"" + cmd + "\"");
            if (cmd != null) {
                if (cmd.matches("<UPLOAD.*")) {
                    cmd = editUploadCommand(cmd);
                    setUploadRequested(true);
                } else if (cmd.matches("<SETDATE>")) {
                    // Append current timestamp -- seconds since 1970
                    long now_s = CryoUtils.timeMs() / 1000;
                    now_s = CryoUtils.truncateTimeStamp(now_s);
                    cmd = "<SETDATE " + now_s + ">";
                }
            }
            m_cryoMsg.postDebug("editcryocmd", "  ---> \"" + cmd + "\"");
        }
        return cmd;
    }

    private void setUploadRequested(boolean b) {
        m_uploadRequestFlag = b;
    }

    private boolean isUploadRequested() {
        return m_uploadRequestFlag;
    }

    /**
     * Examines all the (whitespace delimited) tokens in the given string and
     * converts every token that can be parsed as a date into a Unix time stamp.
     * If a time is included in the date, it must be separated from the date
     * part by a 'T' -- no spaces. The time is 0 - 23 hours.
     * The following formats are accepted:<br>
     * yyyy-mm-dd<br>
     * yyyy-mm-dd'T'hh<br>
     * yyyy-mm-dd'T'hh:mm<br>
     * yyyy-mm-dd'T'hh:mm:ss<br>
     * 
     * @param str The given string.
     * @return The Unix time stamp -- seconds since 1970, truncated to 32 bits.
     */
    private String editUploadCommand(String str) {
        StringBuffer sb = new StringBuffer();
        StringTokenizer toker = new StringTokenizer(str, " \t<>");
        String lastToken = "";
        int ndates = 0;
        while (toker.hasMoreTokens()) {
            String token = toker.nextToken();
            Date date = null;
            String strDate = token;
            for (int i = 0; date == null && i < 3; i++) {
                try {
                    date = dateTTimeFmt.parse(strDate);
                } catch (ParseException e) {
                }
                if (date == null) {
                    if (!strDate.contains("T")) {
                        strDate += "T00:00:00";
                    } else {
                        strDate += ":00";
                    }
                }
            }
            if (sb.length() == 0) {
                sb.append("<");
            } else {
                sb.append(" ");
            }
            if (date == null) {
                sb.append(token);
                if (lastToken.equalsIgnoreCase("FROM")
                    || lastToken.equalsIgnoreCase("TO"))
                {
                    ndates++;
                }
            } else {
                if (ndates == 0) {
                    if (!lastToken.equalsIgnoreCase("FROM")) {
                        sb.append("FROM ");
                    }
                } else if (ndates == 1) {
                    if (!lastToken.equalsIgnoreCase("TO")) {
                        sb.append("TO ");
                    }
                }
                // Force time stamp to wrap at 32 bits
                sb.append(CryoUtils.truncateTimeStamp(date.getTime() / 1000));
                ndates++;
            }
            lastToken = token;
        }
        sb.append(">");
        return sb.toString();
    }

    /**
     * Sets the confirmation value for a given key.
     * Overrides any previously set value for this key.
     * @param key The key.
     * @param value The value to set.
     */
    protected void setConfirmation(String key, boolean value) {
        m_resultTable.put(new String(key), new Boolean(value));
    }

    /**
     * Clears out any old confirmations for the given key.
     * @param key The key to remove.
     */
    protected void clearConfirmation(String key) {
        m_resultTable.remove(new String(key));
    }

    public void setUploadFilePath(String path) {
        m_uploadFilePath = path;
    }

    private String getUploadFilePath() {
        return m_uploadFilePath;
    }

    private void requestVersionPopup(boolean b) {
        m_requestedVersionPopup  = b;
    }

    public void popupFirmwareVersion() {
        if (isVersionKnown()) {
            popupMsg("About CryoBay",
                     "CryoBay firmware version: " + m_firmwareVN, false);
        } else {
            sendToCryoBay("<VN>", PRIORITY1);
            requestVersionPopup(true);
        }
    }


    /**
     * Interprets a reply. Done as a task in the Event Thread.
     */
    class ReplyProcessor implements Runnable {
        String mm_what;
        boolean mm_value;

        ReplyProcessor(String what, boolean value) {
            mm_what = what;
            mm_value = value;
        }

        public void run() {
            processReplyInEventThread(mm_what, mm_value);
        }
    }


    /**
     * This thread waits for the port to be ready and sends the next
     * command in the queue.
     */
    class Sender extends Thread {

        private int mm_numTries;
        private int mm_numTry;
        private List<String> mm_commandList;
        private List<String> mm_priorityCommandList;
        private boolean mm_quit = false;
        private boolean mm_readyForCommand = true;
        private Timer mm_replyTimer;
        private String mm_rtnMsg = new String();
        private boolean mm_rtnFlag = false;
        private static final int REPLY_TIMEOUT = 60000; // ms


        public Sender(String command, int numTries) {

            setName(m_socketType + " Sender");
            //mm_commandMessage = commandMessage;
            mm_numTries = numTries;
            m_command = command;
            mm_commandList
            = Collections.synchronizedList(new ArrayList<String>());
            mm_priorityCommandList
            = Collections.synchronizedList(new ArrayList<String>());
            ActionListener replyTimeout = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    m_cryoMsg.postDebug("CryoSocketControl.replyTimeout");
                    checkForReply();
                }
            };
            mm_replyTimer = new Timer(REPLY_TIMEOUT, replyTimeout);
        }

        public void run() {
            while (!mm_quit) {
                if ((mm_commandList.size() == 0
                     && mm_priorityCommandList.size() == 0)
                    || !mm_readyForCommand)
                {
                    // TODO: (Java 5) Use java.util.concurrent.Semaphore
                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException ie) {
                        m_cryoMsg.postDebug("cryocmd",
                                            "CryoSocketControl.Sender "
                                            + "interrupted!");
                    }
                } else if (mm_readyForCommand) {
                    execNextCommand();
                }
            }
            mm_replyTimer.stop();
        }

        public void quit() {
            mm_quit = true;
        }

        public void addCommand(String cmd, Priority priority) {
            if (priority == PRIORITY1) {
                mm_priorityCommandList.add(cmd);
            } else if (priority == PRIORITY2 || !mm_commandList.contains(cmd)) {
                mm_commandList.add(cmd);
            }
        }

        public void gotReply(boolean value) {
            mm_replyTimer.stop();
            if (value != false && m_replyFailType != CRYO_OK) {
                m_cryoMsg.postInfo("Cryobay connection OK");
                m_replyFailType = CRYO_OK;
            }
            mm_readyForCommand = true;
            // TODO: (Java 5) Release semaphore here
            //interrupt();        // Wake up the run() method
        }

        private void execNextCommand() {
            String cmdMessage = null;
            synchronized (mm_priorityCommandList) {
                if (mm_priorityCommandList.size() > 0) {
                    cmdMessage = mm_priorityCommandList.remove(0);
                    m_cryoMsg.postDebug("cryocmd",
                                        "PRIORITY COMMAND: " + cmdMessage);
                }
            }
            if (cmdMessage == null) {
                synchronized (mm_commandList) {
                    if (mm_commandList.size() > 0) {
                        cmdMessage = mm_commandList.remove(0);
                    }
                }
            }

            //m_cryoMsg.postDebug("Sending: " + cmdMessage + "  on " + m_socketType);
            cmdMessage = lastMinuteEdits(cmdMessage);
            if (isMessageAllowed(cmdMessage)) {
                m_command = (cmdMessage);
                //mm_commandMessage = (byte[])cmdMessage[1];
                m_cryoMsg.postDebug("cryocmd",
                                    "Executing next cmd= " + m_command);
                mm_numTry = 1;
                mm_rtnMsg = m_command;
                mm_rtnFlag = false;
                mm_readyForCommand = false;
                send(m_command);
                mm_replyTimer.start();
            }
        }

        private synchronized void checkForReply() {
            mm_replyTimer.stop();
            m_cryoMsg.postDebug("cryoreply",
                                "checkForReply: " + m_socketType
                                + ", mm_rtnFlag=" + mm_rtnFlag);
            if (!mm_rtnFlag) {
                if (mm_numTry >= mm_numTries) {
                    // Hard failure
                    m_cryoMsg.postError("No reply on " + m_socketType
                                        + "; cmd= " + m_command.trim());
                    if (m_replyFailType != CRYO_UNRESPONSIVE) {
                        if(m_socketType == TEMPCTRL){
                            //SDM FIXME
                            //m_cryoMsg.postError("Temp Ctrl not responding");
                        }else {
                            //SDM FIXME
                            //m_cryoMsg.postError("CryoBay is not responding");
                        }
                        m_replyFailType = CRYO_UNRESPONSIVE;
                    }
                    processReply(mm_rtnMsg, false);
                } else {
                    // Try again
                    m_cryoMsg.postInfo("Resend cmd to " + m_socketType
                                       + ": " + m_command.trim());
                    mm_numTry++;
                    send(m_command);
                    mm_replyTimer.start();
                }
            } else {
                // Got the reply
                //mm_asListener.newMessage(mm_rtnMsg);
                if (m_replyFailType != CRYO_OK) {
                    if(m_socketType == TEMPCTRL){
                        m_cryoMsg.postInfo("Temp Controller connection OK");
                    } else {
                        m_cryoMsg.postInfo("CryoBay connection OK");
                    }
                    m_replyFailType = CRYO_OK;
                }
                processReply(mm_rtnMsg, mm_rtnFlag);
            }
        }
    }        // End of Sender class


    /**
     * This thread monitors the input from the CryoBay.
     * Messages end with a '\n' character.
     * At the end of each reply calls processReply(String reply, true).
     */
    class Reader extends Thread {
        CryoSocket mm_socket;
        boolean mm_quit = false;

        public Reader(CryoSocket socket) {
            setName(m_socketType + " Reader");
            mm_socket = socket;
        }

        public void quit() {
            mm_quit = true;
        }

        public void run() {
            final int BUFLEN = 256;
            char[] buf = new char[BUFLEN];
            int idx = 0;        // Which byte we're waiting for in buffer

            while (!mm_quit) {
                try {
                    char ch = (char)mm_socket.read();
                    m_cryoMsg.postDebug("allCryoCmd",
                                        "From " + m_socketType + ": 0x"
                                        + Integer.toHexString(ch));
                    if (ch != 0) {
                        buf[idx++] = ch;
                    }

                    // Check for end of message
                    if (ch == '\n') {
                        if (m_isConnectError) {
                            m_isConnectError = false;
                            m_cryoMsg.postInfo("Connection to " + m_socketType
                                               + " is OK");
                        }
                        processReply(String.valueOf(buf).trim(), true);
                        idx = 0; // Ready for next message
                        buf = new char[BUFLEN];
                    } else if (idx == BUFLEN) {
                        // Message is too long
                        if (!m_isConnectError) {
                            m_cryoMsg.postDebug("No <LF> after " + idx
                                                + " bytes from "
                                                + m_socketType);
                            m_isConnectError = true;
                        }
                        idx = 0; // Wait for a new message*/
                    }
                } catch (IOException ioe) {
                    // Exit the thread
                    m_cryoMsg.postDebug("CryoSocketControl.Reader Quitting: "
                                        + m_socketType
                                        + ": " + ioe);
                    mm_quit = true;
                }
            }
        }
    }


    /**
     * This thread just sends status requests every so often.
     */
    public class StatusPoller extends Thread {
        private boolean m_quit = false;
        private int m_rate_ms;

        public StatusPoller(int rate_ms) {
            setName(m_socketType + " StatusPoller");
            m_rate_ms = rate_ms;
        }

        public void quit() {
            m_cryoMsg.postDebug("cryopoll", "QUITTING status poller");
            m_quit = true;
        }

        public synchronized void run() {
            while (!m_quit) {
                m_cryoMsg.postDebug("cryopoll", "StatusPoller sending queries");
                sendCommand("<GETSTATE>", PRIORITY3);
                sendCommand("<C_1>", PRIORITY3);
                sendCommand("<C_3>", PRIORITY3);
                sendCommand("<C_4>", PRIORITY3);
                sendCommand("<C_HEAT>", PRIORITY3);
                sendCommand("<C_COLD>", PRIORITY3);
                //sendCommand("<C_WARM>", PRIORITY3);
                sendCommand("<AN0>", PRIORITY3);
                sendCommand("<AN1>", PRIORITY3);
                sendCommand("<AN2>", PRIORITY3);
                sendCommand("<AN3>", PRIORITY3);
                sendCommand("<AN5>", PRIORITY3);
                try {
                    Thread.sleep(m_rate_ms);
                } catch (InterruptedException ie) {}
            }
        }
    }


    class DateSetter extends Thread {
        /**
         * Maximum clock skew that we will correct without warning
         * (in seconds).
         */
        private static final int MAX_CLOCK_SKEW_S = 300; // seconds

        /**
         * How often we check the clock (in ms).
         */
        private static final long SYNC_INTERVAL = 1000 * 60 * 60 * 24; // ms


        public void run() {
            while (true) {
                setName("Cryo Date Synchronizer");
                m_date_ms = 0;
                sendCommand("<GETDATE>", PRIORITY1);
                try {
                    // NB: Reader thread sets m_data_ms when reply comes in
                    while (m_date_ms == 0) {
                        Thread.sleep(100);
                    }
                } catch (InterruptedException ie) {
                    m_cryoMsg.postWarning("Date setting command aborted");
                    continue;
                }
                int now_s = (int)(CryoUtils.timeMs() / 1000);
                int date_s = (int)(m_date_ms / 1000);
                int delta_s = date_s - now_s;
                int ok = JOptionPane.YES_OPTION;
                if (Math.abs(delta_s) >= MAX_CLOCK_SKEW_S) {
                    String msg = makeClockSkewMsg(date_s, now_s);
                    ok = showConfirmDialog(msg,
                                           "Clock Skew Warning",
                                           JOptionPane.YES_NO_OPTION,
                                           JOptionPane.WARNING_MESSAGE);
                }
                if (ok == JOptionPane.YES_OPTION) {
                    if (Math.abs(delta_s) > 1) {
                        sendCommand("<SETDATE>", PRIORITY1);
                    }
                }

                try {
                    Thread.sleep(SYNC_INTERVAL);
                } catch (InterruptedException ie) {
                }
            }
        }

        private String makeClockSkewMsg(int date_s, int now_s) {
            String msg =
                "The CryoBay clock differs significantly from the host clock:";
            long now = CryoUtils.timeMs();
            long ldate = CryoUtils.fixDateOverflow(date_s, now);
            Date date = new Date(ldate * 1000);
            String strDate = date_TimeFmt.format(date);
            String tz = timeZoneFmt.format(date);
            msg += "\nCryoBay date: " + strDate + " " + tz;
            date = new Date(now);
            strDate = date_TimeFmt.format(date);
            tz = timeZoneFmt.format(date);
            msg += "\nHost date: " + strDate + " " + tz;
            msg += "\nShould the CryoBay's clock be synchronized to the Host?";
            return msg;
        }
    }

}
