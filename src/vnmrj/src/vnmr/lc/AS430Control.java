/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.TreeMap;

import javax.swing.SwingUtilities;
import javax.swing.Timer;

import vnmr.ui.StatusManager;
import vnmr.util.*;

/**
 * This is the interface for communicating with the 430 autosampler for LC.
 * There are three Thread inner classes:
 * <p>
 * Sender - takes commands from a list and sends them in order.
 * This is so the caller never has to wait while the 430 is busy, it puts
 * the command in the queue and returns immediately.
 * <br>
 * Reader - continuously listens for responses from the 430. When it gets
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
public class AS430Control implements AutoSampler {
    protected static final int NULL = 0;
    protected static final int STX = 0x2;
    protected static final int ETX = 0x3;
    protected static final int ACK = 0x6;
    protected static final int NACK = 0x15;
    protected static final int NACK0 = 0x18;

    protected static final int RESPONSE_OK = -ACK;

    protected static final int AS430_OK = 0;
    protected static final int AS430_NIL = 1;
    protected static final int AS430_UNRESPONSIVE = 2;

    private static final int STATUS_PERIOD = 1000; // ms
    static private final int WAITTIME = 2000; // ms


    // Injection modes
    static private final int PARTIAL_LOOPFILL = 1;
    static private final int FLUSHED_LOOP = 2;
    static private final int MICROLITER_PICKUP = 3;

    // What to reset
    static private final int RESET_ALL = 1;

    // When to wash
    static private final int BETWEEN_SERIES = 1;
    static private final int BETWEEN_SAMPLES = 2;
    static private final int BETWEEN_INJECTIONS = 3;

    // Method types
    static private final int INJECTION_METHOD = 0x1;
    static private final int WASH_METHOD = 0x2;
    static private final int TIMEBASE_METHOD = 0x4;
    static private final int MIX_METHOD = 0x8;
    static private final int USERPROG_METHOD = 0x20;

    // Inject valve positions
    static private final int INJECT = 0;
    static private final int LOAD = 1;

    // MISC
    static private final int STOP = 0;
    static private final int START = 1;

    /**
     * Protocol Function Codes
     */
    static private final int ANALYSIS_TIME = 100;
    static private final int FIRST_VIAL = 108;
    static private final int LAST_VIAL = 109;
    static private final int FLUSH_VOLUME = 111;
    static private final int INJECTIONS_PER_VIAL = 112;
    static private final int INJECTION_MODE = 124;
    static private final int STATUS = 152;
    static private final int ERROR_CODE = 155;
    static private final int RESET_ERROR = 156;
    static private final int INJECTION_VOLUME = 210;
    static private final int WASH_BETWEEN = 500;
    static private final int SEND_PROGRAMMED_VALUE = 1000;
    static private final int SEND_ACTUAL_VALUE = 1001;
    static private final int START_METHOD = 5100;
    static private final int SWITCH_INJECTOR = 5105;
    static private final int INITIAL_WASH = 5130;

    // Special status values
    static private final int INJECT_MARKER = 55;


    private AS430Socket m_AS430Socket = null;
    private byte[] m_id = {(byte)'2', (byte)'1'}; // Default ID
    private byte[] m_ai = {(byte)'0', (byte)'1'}; // "Additional Information"
    private int[] m_rtrnMessage= new int[2];
    private ButtonIF m_vnmrIf= null;
    private String m_sVnmrMacro= null;
    private String m_sVnmrPrefix= null;
    private String m_sVnmrMiddle= null;
    private String m_sVnmrSuffix= null;
    private int m_iValueIndex= 0;
    private int m_retries = 4;  // # of times to send command if no response
    private StatusPoller m_statusPoller = null;
    private StatusManager m_statusManager;
    private LcInjectListener m_injectListener;
    private Map<Integer, String> m_statusMessages
        = new HashMap<Integer, String>(100);
    private Map<Integer, String> m_pfcNames = new HashMap<Integer, String>(100);
    private Sender m_sender = null;
    private Reader m_reader = null;
    private Map<Integer, Integer> m_resultTable
        = new TreeMap<Integer, Integer>();
    private Map<Integer, MacroFmt> m_macroTable
        = new TreeMap<Integer, MacroFmt>();
    private Map<String, String> m_statusMemory = new TreeMap<String, String>();
    protected int m_command;    // Last command sent
    protected int m_replyFailType = AS430_NIL;


    /**
     * Translation from status values to status text.
     */
    private final static Object[][] STATUS_VALUES = {
        {new Integer(0),   "Not running"},
        {new Integer(10),  "Running"},
        {new Integer(20),  "Searching vial"},
        {new Integer(30),  "Flushing"},
        {new Integer(40),  "Analysis timer running"},
        {new Integer(50),  "Filling sample loop"},
        {new Integer(51),  "Freeze active, holds injection valve from switching"},
        {new Integer(55),  "Inject marker"},
        {new Integer(60),  "Washing"},
        {new Integer(70),  "Segment missing"},
        {new Integer(71),  "Plate missing"},
        {new Integer(72),  "Change plate"},
        {new Integer(73),  "Plate still in clamp"},
        {new Integer(74),  "Moving right lift"},
        {new Integer(75),  "Moving left lift"},
        {new Integer(76),  "Wait for next right lift position"},
        {new Integer(77),  "Wait for next left lift position"},
        {new Integer(80),  "Vial missing"},
        {new Integer(90),  "Rinsing, first air segment, uL pick-up injection"},
        {new Integer(100), "Rinsing, first air segment, uL pick-up injection"},
        {new Integer(110), "Withdraw transport solvent, uL pick-up injection"},
        {new Integer(120), "Rinse buffer tubing"},
        {new Integer(130), "Dispense, Mixmethod and Userprog."},
        {new Integer(140), "Aspirate, Mixmethod and Userprog."},
        {new Integer(151), "Waiting for LOAD command"},
        {new Integer(152), "Waiting for next injection command"},
        {new Integer(159), "Waiting, Mixmethod and Userprog."},
        {new Integer(170), "Injection valve position: INJECT"},
        {new Integer(171), "Injection valve position: LOAD"},
        {new Integer(172), "Syringe valve position: Injection valve"},
        {new Integer(173), "Syringe LOAD"},
        {new Integer(174), "Syringe UNLOAD"},
        {new Integer(175), "Syringe HOME"},
        {new Integer(180), "Wait for input"},
        {new Integer(200), "Tray advancing"},
        {new Integer(300), "Running selftest"},
        {new Integer(310), "Initializing motors"},
        {new Integer(900), "Stop process"},
        {new Integer(910), "Initial wash"},
        {new Integer(920), "Prime solvent selection valve"},
        {new Integer(921), "Moving syringe to HOME"},
        {new Integer(922), "Moving syringe to END"},
        {new Integer(923), "Waiting for syringe HOME command"},
        {new Integer(924), "Moving plate to EXCHANGE position"},
        {new Integer(925), "Moving plate to HOME position"},
        {new Integer(926), "Waiting for plate HOME command"},
        {new Integer(927), "Check for plate in left lift"},
        {new Integer(930), "Needle unit moving to FRONT position"},
        {new Integer(931), "Needle unit moving to HOME position"},
        {new Integer(932), "Needle unit waiting for HOME command"},
    };

    private final static Object[][] INJECT_VALUES = {
        {new Integer(0),   "inject"},
        {new Integer(1),   "load"},
    };

    /**
     * Translation from PFC codes to function names.
     * (Currently a selection of the most used PFCs.)
     */
    private final static Object[][] FUNCTION_CODES = {
        {new Integer(100),   "Analysis Time"},
        {new Integer(107),   "Loop Volume"},
        {new Integer(108),   "First Vial"},
        {new Integer(109),   "Last Vial"},
        {new Integer(111),   "Flush Volume"},
        {new Integer(112),   "Injections per Vial"},
        {new Integer(117),   "Priority Vial Number"},
        {new Integer(122),   "Tray Cooling On/Off"},
        {new Integer(124),   "Injection Mode"},
        {new Integer(152),   "Status"},
        {new Integer(155),   "Error Code"},
        {new Integer(156),   "Reset Error"},
        {new Integer(210),   "Injection Volume"},
        {new Integer(500),   "Wash Between"},
        {new Integer(1000),  "Send Programmed Value"},
        {new Integer(1001),  "Send Actual Value"},
        {new Integer(5100),  "Start Method"},
        {new Integer(5105),  "Switch Injector"},
        {new Integer(5130),  "Initial Wash"},
    };

    public AS430Control(String host,
                        int port,
                        StatusManager statusManager,
                        LcInjectListener injectListener) {
        populateMessageTable(STATUS, STATUS_VALUES);
        populateMessageTable(SWITCH_INJECTOR, INJECT_VALUES);
        populateFunctionTable(FUNCTION_CODES);
        m_statusManager = statusManager;
        m_injectListener = injectListener;
        connect(host, port);
    }


    public static void main(String[] args) {

        if (args.length < 1) {
            System.out.println("usage: AS430Control <hostname> [<portnumber>]");
            System.exit(0);
        }
        String hostname = "";
        int port = 4000;

        if (args.length > 0) {
            hostname = args[0];
        }
        if (args.length > 1) {
            try {
                port = Integer.parseInt(args[1]);
            } catch (NumberFormatException nfe) {}
        }

        DebugOutput.addCategory("430Comm");

        AS430Control master = new AS430Control(null, 0, null, null);

        // Read and execute commands from stdin
        BufferedReader in
                = new BufferedReader(new InputStreamReader(System.in));
        String line;
        int[] rtrnMessage= new int[5];

        try {
            for (prompt("> ");
                 (line = in.readLine()) != null;
                 prompt("> "))
            {
                //System.out.println("got: \"" + line + "\"");
                StringTokenizer toker = new StringTokenizer(line, " :=");
                String cmd = "";
                if (toker.hasMoreTokens()) {
                    cmd = toker.nextToken();
                }

                String par = "";
                if (toker.hasMoreTokens()) {
                    if (cmd.equals("send")) {
                        par = toker.nextToken("").trim();
                    } else {
                        par = toker.nextToken();
                    }
                }

                String value = "";
                if (toker.hasMoreTokens()) {
                    value = toker.nextToken(); 
                }

                // Execute the command
                if (cmd.equals("connect")) {
                    master.connect(hostname, port);
                } else if (cmd.equals("disconnect")) {
                    master.disconnect();
                } else if (cmd.equals("send")) {
                    master.send(par);
                } else if (cmd.equals("readP")) {        //read programmed
                    if (par!=""){
                        master.sendCommand(SEND_PROGRAMMED_VALUE,
                                           Integer.parseInt(par));
                    } else {
                        System.out.println("Usage: read <code>");
                    }
                } else if (cmd.equals("readA")) {        //read actual
                    if (par!=""){
                        master.sendCommand(SEND_ACTUAL_VALUE,
                                           Integer.parseInt(par));
                    } else {
                        System.out.println("Usage: read <code>");
                    }
                } else if (cmd.equals("set")) {
                    if (par!="" && value!="") {
                        master.sendCommand(Integer.parseInt(par),
                                           Integer.parseInt(value));
                    } else {
                        System.out.println("Usage: set <code> <value>");
                    }
                } else if (cmd.equals("setID")) {
                    master.setASID(Integer.parseInt(par));
                } else if (cmd.equals("download")) {
                    if (master.downloadMethod(1000, 1, 1, 1, 3000, 1, null)) {
                        System.out.println("method loaded");
                    } else {
                        System.err.println("method failed!");
                    }
                } else if (cmd.equals("start")) {
                    if (master.startMethod()) {
                        System.out.println("Starting method");
                    } else {
                        System.out.println("method start failed");
                    }
                } else if (cmd.equals("ready")) {
                    if (!master.isReady()) {
                        System.out.println("NOT READY!");
                    }
                } else if (cmd.equals("wash")) {
                    if (master.wash()) {
                        System.out.println("Washing");
                    } else {
                        System.out.println("wash failed");
                    }
                } else if (cmd.equals("reset")) {
                    if (master.reset()) {
                        System.out.println("Reset");
                    } else {
                        System.out.println("reset failed");
                    }
                } else if (cmd.equals("readE")) {
                    if (master.readError()) {
                        System.out.println("Read Errors");
                    } else {
                        System.out.println("readE failed");
                    }
                } else if (cmd.equals("quit")) {
                    master.disconnect();
                    System.exit(0);
                } else if (cmd.length() > 0) {
                    System.out.println("Command not recognized: \""
                                       + cmd + "\"");
                }
            }
        } catch (IOException ioe) {
            System.err.println("Error: " + ioe);
        }
        System.out.println();

    } // End of main method

    private static void prompt(String s) {
        System.out.print(s);
    }

    private static String arrayToString(int[] vals) {
        if (vals == null) {
            return "null";
        } else {
            StringBuffer sb = new StringBuffer();
            for (int i = 0; i < vals.length; i++) {
                if (i == 0) {
                    sb.append("" + vals[i]);
                } else {
                    sb.append(", " + vals[i]);
                }
            }
            return sb.toString();
        }
    }

    private static String arrayToHexString(byte[] vals) {
        if (vals == null) {
            return "null";
        } else {
            StringBuffer sb = new StringBuffer();
            for (int i = 0; i < vals.length; i++) {
                if (i == 0) {
                    sb.append("" + Integer.toHexString(vals[i]));
                } else {
                    sb.append(", " + Integer.toHexString(vals[i]));
                }
            }
            sb.append(" (hex)");
            return sb.toString();
        }
    }

    private static String arrayToHexString(char[] vals) {
        if (vals == null) {
            return "null";
        } else {
            StringBuffer sb = new StringBuffer();
            for (int i = 0; i < vals.length; i++) {
                if (i == 0) {
                    sb.append("" + Integer.toHexString(vals[i]));
                } else {
                    sb.append(", " + Integer.toHexString(vals[i]));
                }
            }
            sb.append(" (hex)");
            return sb.toString();
        }
    }

    /* *****************Start of AutoSampler interface**************** */

    /**
     * Return the model number of this detector, "430".
     */
    public String getName(){
        return "430";
    }

    /** Open communication with the autosampler. */
    public void connect(String host, int port){
        Messages.postDebug("430Comm", "AS430Control.connect(" + host
                           + ", " + port + ")");
        m_AS430Socket = new AS430Socket(host, port);
        m_reader = new Reader(m_AS430Socket);
        m_reader.start();
        pollStatus(STATUS_PERIOD);
    }

    public void pollStatus(int period_ms) {
        if (m_statusPoller != null) {
            // Stop any running status poller
            m_statusPoller.quit();
        }
        if (period_ms > 0) {
            m_statusPoller = new StatusPoller(period_ms);
            m_statusPoller.start();
        }
    }

    public boolean wait(int time){
        try{
            Thread.sleep(time);
        } catch (Exception e){
            System.err.println("Cannot sleep");
            return false;
        } 
        return true;
    
    }
    
    /**See if the autosampler is ready to start a run. */
    // TODO: Fix this
    public boolean isReady(){
        //int[] returnMessage;
    
        //sendCommand(SEND_ACTUAL_VALUE, STATUS);
    
        //wait(WAITTIME);
        //returnMessage = getRtrnMessage();
        //if (returnMessage[1] != 0x00){
        //    return false;
        //}
        return true;
    }
    
    public boolean isConnected() {
        return m_AS430Socket != null && m_AS430Socket.isConnected();
    }

    /** Shut down communication with the autosampler. */
    public void disconnect() {
        Messages.postDebug("430Comm", "AS430Control.disconnect()");
        if (m_AS430Socket != null) {
            pollStatus(0);
            m_AS430Socket.disconnect();
        }
        if (m_sender != null) {
            m_sender.quit();
        }
        if (m_reader != null) {
            m_reader.quit();
        }
    }


    /**
     * Download an injection method to the autosampler.
     * @param injVol Injection volume 0000.10 to 1000.00 microLiters
     * in .10 uL increments. I.e., the value is a multiple of 10.
     * The legal range will be further constrained by the installed
     * inject loop size and injection mode.
     * @param tray Tray number to use; only 1 is supported for now.
     * @param row Row number to use, starting from 1.
     * @param col Column number to use, starting from 1.
     */
    public boolean downloadMethod(int injVol, int tray, int row, int col,
                                  int flushVol, int type, String cmd) {
        // This starts a separate thread to do the work because
        // it blocks after downloading each parameter until the reply
        // for that parameter is received.

        // AS430's position parameter:
        // 6 digits: "prrccc".
        // p= plate number; without feeder=0 or 1, with feeder=1-7.
        // rr = row number; first row is 0.
        // ccc = column number; first column is 1.
        int pos = tray * 100000 + (row - 1) * 1000 + col;
        Thread downloadMethodThread
                = new DownloadMethodThread(injVol, pos, flushVol, type, cmd);
        downloadMethodThread.start();
        return true;
    }

    private class DownloadMethodThread extends Thread {
        int mm_numInj = 1;
        int mm_flushVol;
        int mm_injVol;
        int mm_pos;
        int mm_type;
        String mm_cmd;

        private DownloadMethodThread(int injVol, int pos,
                                     int flushVol, int type, String cmd) {
            Messages.postDebug("lcInject", "DownloadMethodThread.<init>: "
                               + "cmd=\"" + cmd + "\"");
            
            mm_flushVol = flushVol;
            mm_injVol = injVol;
            mm_pos = pos;
            mm_type = type;
            mm_cmd = cmd;
        }

        public void run() {
            boolean ok = true;
            int result;

            if (ok) {
                clearConfirmation(INJECTION_MODE);
                sendCommand(INJECTION_MODE, mm_type);
                result = waitForConfirmation(INJECTION_MODE);
                if (result != RESPONSE_OK) {
                    ok = false;
                } else {
                    Messages.postDebug("430Comm",
                                       "Injection Mode result: " + result);
                }
            }

            if (ok) {
                clearConfirmation(ANALYSIS_TIME);
                sendCommand(ANALYSIS_TIME, 0);
                result = waitForConfirmation(ANALYSIS_TIME);
                if (result != RESPONSE_OK) {
                    ok = false;
                } else {
                    Messages.postDebug("430Comm",
                                       "Analysis Time result: " + result);
                }
            }

            if (ok && mm_type != MICROLITER_PICKUP) {
                clearConfirmation(FLUSH_VOLUME);
                sendCommand(FLUSH_VOLUME, mm_flushVol);
                result = waitForConfirmation(FLUSH_VOLUME);
                if (result != RESPONSE_OK) {
                    ok = false;
                } else {
                    Messages.postDebug("430Comm",
                                       "Flush Volume result: " + result);
                }
            }

            if (ok) {
                clearConfirmation(INJECTIONS_PER_VIAL);
                sendCommand(INJECTIONS_PER_VIAL, mm_numInj);
                result = waitForConfirmation(INJECTIONS_PER_VIAL);
                if (result != RESPONSE_OK) {
                    ok = false;
                } else {
                    Messages.postDebug("430Comm",
                                       "Injections per vial result: " + result);
                }
            }

            if (ok && mm_type != FLUSHED_LOOP) {
                clearConfirmation(INJECTION_VOLUME);
                sendCommand(INJECTION_VOLUME, mm_injVol);
                result = waitForConfirmation(INJECTION_VOLUME);
                if (result != RESPONSE_OK) {
                    ok = false;
                } else {
                    Messages.postDebug("430Comm",
                                       "Injection Volume result: " + result);
                }
            }

            if (ok) {
                clearConfirmation(FIRST_VIAL);
                sendCommand(FIRST_VIAL, mm_pos);
                result = waitForConfirmation(FIRST_VIAL);
                if (result != RESPONSE_OK) {
                    ok = false;
                } else {
                    Messages.postDebug("430Comm",
                                       "First Vial result: " + result);
                }
            }

            if (ok) {
                clearConfirmation(LAST_VIAL);
                sendCommand(LAST_VIAL, mm_pos);
                result = waitForConfirmation(LAST_VIAL);
                if (result != RESPONSE_OK) {
                    ok = false;
                } else {
                    Messages.postDebug("430Comm",
                                       "Last Vial result: " + result);
                }
            }

            if (ok) {
                if (mm_cmd != null) {
                    if (!mm_cmd.equals("y")) {
                        m_injectListener.setInjectionCommand(mm_cmd);
                    }
                    startMethod();
                    Messages.postInfo("AS 430 injection method started");
                } else {
                    Messages.postInfo("AS 430 injection method downloaded");
                }
            }
            Messages.postDebug("430Comm", "downloadMethod() result: " + ok);
        }
    }
    
    public boolean startMethod() {
        sendCommand(START_METHOD, INJECTION_METHOD);
        return true;
    }
 
    public boolean wash() {
        sendCommand(INITIAL_WASH, START);
        return true;
    }

    public boolean reset() {
        sendCommand(RESET_ERROR, RESET_ALL);
        return true;
    }


    public boolean readError() {
        sendCommand(SEND_ACTUAL_VALUE, ERROR_CODE);
        return true;
    }

    public byte[] getASID() {
        return m_id;
    }

    public boolean setASID(int id) {
        if (id < 20 || id > 29) {
            Messages.postError("AS430Control.setASID: "
                               + "ID must be 20-29; got" + id);
            return false;
        }
        m_id = Integer.toString(id).getBytes();
        return true;
    }

    /**
     * Called whenever a full reply has been received from the 430.
     * Also called with value=-1 if a timeout occurs waiting for a
     * reply or if a garbled reply is received.
     * @param what The parameter that the value refers to.  For one
     * byte replies (ACK, NACK, or NACK0) the command that produced
     * this reply.
     * @param value The value of the "what" parameter.  For one byte
     * replies, the <i>negated</i> value of that byte as an integer.
     */
    protected void processReply(int what, int value) {
        if (SwingUtilities.isEventDispatchThread()) {
            processReplyInEventThread(what, value);
        } else {
            SwingUtilities.invokeLater(new ReplyProcessor(what, value));
        }
    }

    /**
     * Does the work for processReply.
     * @see #processReply
     */
    private void processReplyInEventThread(int what, int value) {
        Messages.postDebug("430Comm", "AS430Control.processReply("
                           + what + ", " + value + ")");
        m_sender.gotReply(value);
        setConfirmation(what, value);
        sendToMacro(what, value);
        if (value == -NACK || value == -NACK0) {
            String msg = m_pfcNames.get(what);
            if (msg == null) {
                msg = "function #" + what;
            }
            // TODO: Workaround to avoid annoying error messages
            if (what != SEND_ACTUAL_VALUE && value != -NACK0) {
                Messages.postError("AS 430: error sending " + msg);
            } else {
                Messages.postDebug("AS 430: error " + -value
                                   + " sending " + msg);
            }
        } else {
            processStatus(what, value);
        }
    }
  
    /**
     * Send a command to the autosampler.
     */
    synchronized public boolean command(StringTokenizer toker) {
        int ntoks = toker.countTokens();
        if (ntoks == 0) {
            return false;
        }
        String cmd = toker.nextToken();
        ntoks--;

        boolean ok = false;
        if (cmd.equalsIgnoreCase("download")) {
            // Usage: download <row> <column> <vol_uL> <flushVol_uL> <mode>
            int row = 0;
            int col = 1;
            int tray = 1;
            int volume = 20;    // uL
            int flushVol = 20;    // uL
            int mode = 1;
            String token = null;
            String what = null;
            String command = null;
            try {
                if (ntoks > 0) {
                    what = "Row number";
                    token = toker.nextToken();
                    row = Integer.parseInt(token); // row is 1-8
                    ntoks--;
                }
                if (ntoks > 0) {
                    what = "Column number";
                    token = toker.nextToken();
                    col = Integer.parseInt(token); // col is 1-12
                    ntoks--;
                }
                if (ntoks > 0) {
                    what = "Inject volume";
                    token = toker.nextToken();
                    volume = 100 * (int)Double.parseDouble(token);
                    ntoks--;
                }
                if (ntoks > 0) {
                    what = "Flush volume";
                    token = toker.nextToken();
                    flushVol = 100 * (int)Double.parseDouble(token);
                    ntoks--;
                }
                if (ntoks > 0) {
                    what = "Injection mode";
                    token = toker.nextToken();
                    mode = Integer.parseInt(token);
                    ntoks--;
                }
                if (ntoks > 0) {
                    command = toker.nextToken("").trim(); // Remaining tokens
                    ntoks--;
                }
                ok = downloadMethod(volume, tray, row, col,
                                    flushVol, mode, command);
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad " + what + " in AS 430 method: "
                                   + token);
                ok = false;
            }
        } else if (cmd.equalsIgnoreCase("wash")) {
            ok = wash();
        } else if (cmd.equalsIgnoreCase("start")) {
            ok = startMethod();
        } else if (cmd.equalsIgnoreCase("inject")) {
            int val = INJECT;
            if (ntoks > 0) {
                String s = toker.nextToken().toLowerCase();
                val = (s.startsWith("y") || s.startsWith("i")) ? INJECT : LOAD;
            }
            ok = sendCommand(SWITCH_INJECTOR, val);
        } else if (cmd.equalsIgnoreCase("readActual")) {
            String token = "";
            try {
                token = toker.nextToken();
                int what = Integer.parseInt(token);
                ok = sendCommand(SEND_ACTUAL_VALUE, what);
            } catch (NoSuchElementException nsee) {
                Messages.postError("No 430 function code in readActual");
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad 430 function code in readActual: "
                                   + token);
            }
        } else if (cmd.equalsIgnoreCase("readProgrammed")) {
            String token = "";
            try {
                token = toker.nextToken();
                int what = Integer.parseInt(token);
                ok = sendCommand(SEND_PROGRAMMED_VALUE, what);
            } catch (NoSuchElementException nsee) {
                Messages.postError("No 430 function code in readProgrammed");
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad 430 function code in readProgrammed: "
                                   + token);
            }
        } else if (cmd.equalsIgnoreCase("set")) {
            String token = "";
            try {
                token = toker.nextToken();
                int what = Integer.parseInt(token);
                token = toker.nextToken();
                int value = Integer.parseInt(token);
                int extra = 1;
                if (ntoks > 2) {
                    token = toker.nextToken();
                    extra = Integer.parseInt(token);
                }
                ok = sendCommand(what, value, extra);
            } catch (NoSuchElementException nsee) {
                Messages.postError("Not enough arguments in 'autosampCmd set'");
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad argument in 'autosampCmd set': "
                                   + token);
            }
                
        }
        if (ok) {
            Messages.postDebug("430Comm", "AS 430 command \"" + cmd + "\" OK");
        } else {
            Messages.postError("AS 430 command \"" + cmd + "\" FAILED");
        }
        return ok;
    }

    public boolean sendCommand(int command, int value) {
        return sendCommand(command, value, 1);
    }

    public boolean sendCommand(int command, int value, int extra) {

        Messages.postDebug("430Comm",
                           "AS430Control.sendCommand(" + command
                           + ", " + value + ", " + extra + ")");

        // Construct the message byte array
        byte[] commandMessage = new byte[16];
    
        commandMessage[0] = 0x02;      //STX, start transmission

        commandMessage[1] = m_id[0];   // ID byte 1
        commandMessage[2] = m_id[1];   // ID byte 2

        // Turn int "extra" into array of 2 bytes (front-padded with zeros)
        byte[] extra_array = Fmt.d(2, extra, false, '0').getBytes();
        for (int i = 0; i < 2; i++) {
            commandMessage[3 + i] = extra_array[i];
        }

        // Turn int "command" into array of 4 bytes (front-padded with zeros)
        String scommand = Fmt.d(4, command, false, '0');
        byte[] command_array = scommand.getBytes();
        for (int i = 0; i < 4; i++) {
            commandMessage[5 + i] = command_array[i];
        }

        // Turn int "value" into array of 6 bytes (front-padded with zeros)
        String svalue = Fmt.d(6, value, false, '0');
        byte[] value_array = svalue.getBytes();
        for (int i = 0; i < 6; i++) {
            commandMessage[9 + i] = value_array[i];
        }

        commandMessage[15]= 0x03;    //ETX end transmission

        // Send off the message
        return sendToAutosamp(commandMessage, command);
    }
        

    /**
     * Send a command to the autosampler.
     * @param commandMessage The string to send to the 430, as a byte array.
     * @param command The "command" part of the message.  May be used
     * as part of the reply.
     * @return True if there is a connection to the 430.
     */
    synchronized public boolean sendToAutosamp(byte[] commandMessage,
                                               int command) {
        if (m_sender == null) {
            m_sender = new Sender(null, 3, this, 666);
            m_sender.start();
        }
        if (isConnected()) {
            m_sender.addCommand(commandMessage, command);
            // TODO: (Java 5) Release semaphore here
            //m_sender.interrupt(); // Wake up the run() method
            return true;
        } else {
            return false;
        }
    }
    
    /** send String directly to the autosampler
     * @param cmd as a string
     * @return boolean if sent correctly
     */
    public boolean send(String cmd) {

        byte[] cmd_array;
        byte[] value_array;
        byte[] send_array;
        int[] rtrnMessage= new int[2];
    
        if (this.isConnected()) {

            Messages.postDebug("430Comm",
                               "AS430Control.send(\"" + cmd + "\")");
            cmd_array = cmd.getBytes();
            int len = cmd_array.length;
            if (len != 14) {
                Messages.postWarning("send string for 430 has "
                                     + len + " bytes");
            }
            send_array = new byte[len + 2];
            send_array[0] = STX; // Start Transmission
            for (int i = 0; i < len; i++){
                send_array[i+1] = cmd_array[i];
            }
            send_array[len + 1] = ETX; // End Transmission
            return sendToAutosamp(send_array, Integer.parseInt(cmd));
        } else {
            return false;
        }
    }
    
    /**
     * Send the byte array to the autosampler.
     * @param par as byte array.
     * @return true if successful.
     */
    public boolean send(byte[] par) {
        if (this.isConnected()) {
            m_AS430Socket.write(par);
            Messages.postDebug("AS430+",
                               "Sent to AS430: " + arrayToHexString(par));
            return true;
        } else {
            return false;
        }
    }
  
    public void setMacro(ButtonIF vnmrIf, int id, String macro) {
        m_vnmrIf = vnmrIf;
        if (macro == null || macro.length() == 0) {
            m_macroTable.remove(id);
        } else {
            String pfx = macro;
            String sfx = "";
            boolean valFlag = false;
            int idx = macro.indexOf("$VALUE");
            if (idx >= 0) {
                valFlag = true;
                pfx = macro.substring(0, idx);
                idx += 6;           // Skip over "$VALUE"
                if (idx < macro.length()) {
                    sfx = macro.substring(idx);
                }
            }
            m_macroTable.put(id, new MacroFmt(pfx, sfx, valFlag));
        }
    }

    public void sendToMacro(int what, int value) {
        Messages.postDebug("430Comm",
                           "sendToMacro(" + what + ", " + value + ")");

        MacroFmt macro = (MacroFmt)m_macroTable.get(what);
        if (macro != null && m_vnmrIf != null) {
            String str = macro.prefix;
            if (macro.valueFlag) {
                str += value;
            }
            str += macro.suffix;
            Messages.postDebug("430Comm",
                               "sendToMacro: sending \"" + str + "\"");
            m_vnmrIf.sendToVnmr(str);
        }
    }

    /**
     * This method is called whenever a status message is received from
     * the 430.
     * @param iWhat An integer coding what the value is for (status=152).
     * @param iValue Is the actual value as an integer.
     */
    protected void processStatus(int iWhat, int iValue) {
        if (iValue < 0) {
            // This is just an acknowledge
            return;
        }
        int key = iWhat * 10000 + iValue;
        String msg = m_statusMessages.get(key);
        if (msg == null) {
            msg = "" + iValue;
        }
        String oldMsg;
        switch (iWhat) {
        case STATUS:
            oldMsg = m_statusMemory.get("asStatus");
            if (oldMsg == null || !oldMsg.equals(msg)) {
                if (m_statusManager != null) {
                    m_statusManager.processStatusData("asStatus " + msg);
                }
                m_statusMemory.put("asStatus", msg);
                Messages.postDebug("lcInject",
                                   "processStatus(): iValue=" + iValue
                                   + ", injectListener=" + m_injectListener);
                if (iValue == INJECT_MARKER && m_injectListener != null) {
                    m_injectListener.handleInjection();
                }
            }
            break;
        case SWITCH_INJECTOR:
            oldMsg = m_statusMemory.get("asInject");
            if (oldMsg == null || !oldMsg.equals(msg)) {
                if (m_statusManager != null) {
                    m_statusManager.processStatusData("asInject " + msg);
                }
                m_statusMemory.put("asInject", msg);
            }
            break;
        default:
            
            break;
        }
    }

    private void populateMessageTable(int type, Object[][] entries) {
        int size = entries.length;
        for (int i = 0; i < size; i++) {
            int key = 10000 * type + ((Integer)entries[i][0]).intValue();
            m_statusMessages.put(key, (String)entries[i][1]);
        }
    }

    private void populateFunctionTable(Object[][] entries) {
        int size = entries.length;
        for (int i = 0; i < size; i++) {
            int key = ((Integer)entries[i][0]).intValue();
            m_pfcNames.put((Integer)entries[i][0], (String)entries[i][1]);
        }
    }

    /**
     * Blocks until a message is received from the 430 with the
     * PFC set to "key".
     * @return The value in the response.
     */
    protected int waitForConfirmation(int key) {
        int result = -1;
        while (result == -1) {
            Integer nResult = m_resultTable.remove(key);
            if (nResult == null) {
                // TODO: (Java 5) Use semaphore
                try {
                    Thread.sleep(50);
                } catch (InterruptedException ie) {
                }
            } else {
                result = nResult;
            }
        }
        return result;
    }

    /**
     * Sets the confirmation value for a given key.
     * Overrides any previously set value for this key.
     * @param key The key.
     * @param value The value to set.
     */
    protected void setConfirmation(int key, int value) {
        m_resultTable.put(new Integer(key), new Integer(value));
    }

    /**
     * Clears out any old confirmations for the given key.
     * @param key The key to remove.
     */
    protected void clearConfirmation(int key) {
        m_resultTable.remove(new Integer(key));
    }


    /**
     * Interprets a reply. Done as a task in the Event Thread.
     */
    class ReplyProcessor implements Runnable {
        int mm_what;
        int mm_value;

        ReplyProcessor(int what, int value) {
            mm_what = what;
            mm_value = value;
        }

        public void run() {
            processReplyInEventThread(mm_what, mm_value);
        }
    }


    /**
     * This thread listens for messages from the socket port.
     */
    class Sender extends Thread {

        private byte[] mm_commandMessage;
        private int mm_numTries;
        private int mm_numTry;
        private AutoSampler mm_asListener;
        private List<Object[]> mm_commandList;
        private boolean mm_quit = false;
        private boolean mm_readyForCommand = true;
        private Timer mm_replyTimer;
        private int[] mm_rtnMsg = new int[2];
        private static final int REPLY_TIMEOUT = 1000; // ms


        public Sender(byte[] commandMessage, int numTries,
                      AutoSampler asListener, int command) {

            setName("AS430Control.Sender");
            mm_commandMessage = commandMessage;
            mm_numTries = numTries;
            mm_asListener = asListener;
            m_command = command;
            mm_commandList = new ArrayList<Object[]>();
            ActionListener replyTimeout = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    Messages.postDebug("AS430Control.replyTimeout");
                    checkForReply();
                }
            };
            mm_replyTimer = new Timer(REPLY_TIMEOUT, replyTimeout);
        }

        public void run() {

            while (!mm_quit) {
                if (mm_commandList.size() == 0 || !mm_readyForCommand) {
                    // TODO: (Java 5) Use java.util.concurrent.Semaphore
                    try {
                        Thread.sleep(50);
                    } catch (InterruptedException ie) {
                        Messages.postDebug("430Comm",
                                           "AS430Control.Sender interrupted!");
                    }
                } else if (mm_readyForCommand) {
                    execNextCommand();
                }
            }
        }

        public void quit() {
            mm_quit = true;
        }

        public void addCommand(byte[] msg, int cmd) {
            Object[] cmdMessage = new Object[2];
            cmdMessage[0] = new Integer(cmd);
            cmdMessage[1] = msg;
            mm_commandList.add(cmdMessage);
        }

        public void gotReply(int value) {
            mm_replyTimer.stop();
            if (value != -1 && m_replyFailType != AS430_OK) {
                Messages.postInfo("AS 430 connection OK");
                m_replyFailType = AS430_OK;
            }
            mm_readyForCommand = true;
            // TODO: (Java 5) Release semaphore here
            //interrupt();        // Wake up the run() method
        }

        private void execNextCommand() {
            Object[] cmdMessage = mm_commandList.remove(0);
            if (cmdMessage != null) {
                m_command = ((Integer)cmdMessage[0]).intValue();
                mm_commandMessage = (byte[])cmdMessage[1];
                Messages.postDebug("AS430++",
                                   "cmd= " + m_command + ": "
                                   + arrayToHexString(mm_commandMessage));
                mm_numTry = 1;
                mm_rtnMsg[0] = m_command;
                mm_rtnMsg[1] = -1;
                mm_readyForCommand = false;
                send(mm_commandMessage);
                mm_replyTimer.start();
            }
        }

        private synchronized void checkForReply() {
            mm_replyTimer.stop();
            if (mm_rtnMsg[1] == -1) {
                if (mm_numTry >= mm_numTries) {
                    // Hard failure
                    if (m_replyFailType != AS430_UNRESPONSIVE) {
                        Messages.postError("AS 430 is not responding");
                        m_replyFailType = AS430_UNRESPONSIVE;
                    }
                    processReply(mm_rtnMsg[0], -1);
                } else {
                    // Try again
                    Messages.postDebug("  ... resending command to AS 430");
                    mm_numTry++;
                    send(mm_commandMessage);
                    mm_replyTimer.start();
                }
            } else {
                // Got the reply
                //mm_asListener.newMessage(mm_rtnMsg);
                if (m_replyFailType != AS430_OK) {
                    Messages.postInfo("AS 430 connection OK");
                    m_replyFailType = AS430_OK;
                }
                processReply(mm_rtnMsg[0], mm_rtnMsg[1]);
            }
        }
    }        // End of Sender class


    /**
     * This thread monitors the input from the AS430 and assembles replies.
     * At the end of each reply calls processReply(int what, int value).
     * The following characters mark the beginning of a reply:
     * STX, ACK, NACK, NACK0.
     * These characters mark the end of a reply:
     * ETX, ACK, NACK, NACK0.
     */
    class Reader extends Thread {
        AS430Socket mm_socket;
        boolean mm_quit = false;

        public Reader(AS430Socket socket) {
            setName("AS430Control.Reader");
            mm_socket = socket;
        }

        public void quit() {
            mm_quit = true;
        }

        public void run() {
            final int BUFLEN = 16; // Messages are always 1 or 16 bytes
            byte[] buf = new byte[BUFLEN];
            int idx = 0;        // Which byte we're waiting for in buffer

            while (!mm_quit) {
                try {
                    int ch = mm_socket.read();
                    Messages.postDebug("AS430++", "AS430 sent 0x"
                                       + Integer.toHexString(ch));
                    switch (ch) {
                    case ACK:
                    case NACK:
                    case NACK0:
                        // This is the whole message
                        processReply(m_command, -ch);
                        idx = 0; // Wait for new message
                        break;
                    case STX:
                        // This starts a new message
                        idx = 0;
                        buf[idx++] = (byte)ch;
                        break;
                    default:
                        if (idx == 0) {
                            // Oops, this can't be first char of a message
                            // Ignore it
                        } else {
                            // Add char to message
                            buf[idx++] = (byte)ch;
                        }
                    }

                    // Check for end of message
                    if (ch == ETX) {
                        if (idx == BUFLEN) {
                            // Send off the result
                            int what = extractInt(buf, 5, 9);
                            int value = extractInt(buf, 12, 15);
                            idx = 0; // Ready for next message
                            processReply(what, value);
                        } else {
                            // Message was too short
                            Messages.postDebug("AS430Control: Reply too short: "
                                               + idx + " bytes");
                            idx = 0;
                            processReply(m_command, -1); // Indicate error
                        }
                    } else if (idx == BUFLEN) {
                        // Message is too long
                        Messages.postDebug("AS430Control: No ETX received at "
                                           + idx + " bytes");
                        idx = 0; // Wait for a new message
                    }
                } catch (IOException ioe) {
                    // Exit the thread
                    Messages.postDebug("AS430Control.Reader Quitting: " + ioe);
                    mm_quit = true;
                }
            }
        }

        /**
         * Parse a substring of chars in a byte array to an int value.
         * @param buf The byte array.
         * @param begin Index of first byte in substring.
         * @param end Index of byte after last in substring.
         * @return The int value, or -1 if there is a parsing error.
         */
        private int extractInt(byte[] buf, int begin, int end) {
            int value = -1;
            try {
                String strNum = new String(buf, begin, end - begin).trim();
                value = Integer.parseInt(strNum);
            } catch (NumberFormatException nfe) {
            } catch (IndexOutOfBoundsException ioobe) {
            }
            return value;
        }

    }
                        

    /**
     * This thread just sends status requests every so often.
     */
    class StatusPoller extends Thread {
        private boolean m_quit = false;
        private int m_rate_ms;

        public StatusPoller(int rate_ms) {
            setName("AS430Control.StatusPoller");
            m_rate_ms = rate_ms;
        }

        public void quit() {
            m_quit = true;
        }

        public synchronized void run() {
            while (!m_quit) {
                sendCommand(SEND_ACTUAL_VALUE, STATUS);
                sendCommand(SEND_ACTUAL_VALUE, SWITCH_INJECTOR);
                try {
                    Thread.sleep(m_rate_ms);
                } catch (InterruptedException ie) {}
            }
        }
    }


    /**
     * This class holds the spec for sending a reply value to VnmrBG.
     */
    class MacroFmt {
        public String prefix;
        public String suffix;
        public boolean valueFlag;

        public MacroFmt(String prefix, String suffix, boolean valueFlag) {
            this.prefix = prefix;
            this.suffix = suffix;
            this.valueFlag = valueFlag;
        }
    }

}
