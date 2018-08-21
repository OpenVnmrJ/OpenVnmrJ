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
import java.net.*;
import java.nio.ByteOrder;
import java.util.*;
import javax.imageio.stream.*;

import vnmr.lc.LcMsg;


/**
 * This class simulates a PDA335 instrument.
 */
public class JbInstrument implements JbDefs {

    private final static int CONNECT_TIMEOUT = 0;
    private final static int SOCKET_READ_TIMEOUT = 0;

    /** This guy knows what response to send for a command. */
    public static final Replies sm_REPLIES = new Replies();

    private int m_port = 0;
    private ServerSocket m_ss = null;
    private Socket m_socket = null;
    private boolean m_ok = false;

    private ArrayList m_messageList = new ArrayList();

    /** For reading from the host. */
    private JbInputStream m_in;

    /** For writing to the host. */
    private ImageOutputStream m_out;

    /** Parameter values from the host. */
    private JbParameters m_driverParms = new JbParameters();

    /** The instrument parameter values. */
    private JbParameters m_detectorParms = new JbParameters();

    /** This guy knows about the commands. */
    private JbCommands m_cmd = new JbCommands(m_driverParms, m_detectorParms);

    /** The thread that monitors the input stream. */
    private Reader m_reader = null;

    /** Time run was started. */
    private long m_runStart = System.currentTimeMillis();

    /** Time run was paused. */
    private long m_runPause = System.currentTimeMillis();

    /** Amount of time spent paused in this run. */
    private long m_timePaused_ms = 0;

    /** First wavelength in spectrum. */
    private int m_lambdaMin = 190;

    /** Last wavelength in spectrum. */
    private int m_lambdaMax = 399;

    /** First monitor wavelength. */
    private float m_lambda1 = 245;

    /** Second monitor wavelength. */
    private float m_lambda2 = 345;


    public static void main(String[] args) {
        int port = DEFAULT_PORT_NUMBER;

        for (int i = 0; i < args.length; i++) {
            LcMsg.addCategory(args[i]);
        }

        // Start up a virtual 335
        JbInstrument machine = new JbInstrument(port);
    }

    public JbInstrument(int port) {
        m_ok = true;
        m_port = port;
        
        try {
            LcMsg.postDebug("Pda335: opening server socket on port " + port);
            m_ss = new ServerSocket(port);
        } catch (IOException ioe) {
            LcMsg.postError("Pda335: IO exception 1: " + ioe);
            m_ok = false;
        } catch (SecurityException se) {
            LcMsg.postError("Pda335: Security exception");
            m_ok = false;
        }
        if (!m_ok) {
            return;
        }

        if (m_ss != null) {
            try {
                m_ss.setSoTimeout(CONNECT_TIMEOUT);
                m_socket = m_ss.accept();
            } catch (SocketTimeoutException ste) {
                LcMsg.postError("Pda335: \"accept\" timed out");
                m_ok = false;
            } catch (java.nio.channels.IllegalBlockingModeException ibme) {
                LcMsg.postError("Pda335: Illegal blocking");
                m_ok = false;
            } catch (IOException ioe) {
                LcMsg.postError("Pda335: IO exception 2");
                m_ok = false;
            } catch (SecurityException se) {
                LcMsg.postError("Pda335: Security exception");
                m_ok = false;
            }
        }

        if (!m_ok) {
            return;
        }
        LcMsg.postDebug("Pda335 is connected");

        try {
            m_socket.setSoTimeout(SOCKET_READ_TIMEOUT);
            OutputStream outputStream = m_socket.getOutputStream();
            m_out = new MemoryCacheImageOutputStream(outputStream);
            m_out.setByteOrder(ByteOrder.LITTLE_ENDIAN);
        } catch (SocketException se) {
            LcMsg.postDebug("Cannot set timeout on 335 communication");
        } catch (IOException ioe) {
            LcMsg.postError("Pda335: Couldn't get output stream"
                               + " for the connection.");
        }

        // Initialize detector state
        m_detectorParms.set("DetectorState", 1);

        m_reader = new Reader();
        m_reader.start();
    }

    protected Reader getReader() {
        return m_reader;
    }

    /**
     * Reads the top message in the list and sends a reply.
     */
    /*
    private synchronized void reply() {
        if (m_messageList.size() == 0) {
            // Nothing to do
            return;
        }
        JbMessage msg = (JbMessage)m_messageList.remove(0);
        try {
            msg.sendReply(m_out);
        } catch (IOException ioe) {
            LcMsg.postError("JbInstrument.reply: connection broken.");
        }
    }
    */

    protected boolean processCommand(int code) {
        boolean processed = true;
        if (code == ID_RSP_RETURN_SPECTRUM.intValue()) {
            // Fake up a spectrum
            int npts = 1024;
            JbCommands.Command cmd = m_cmd.get(code);
            cmd.setSize(36 + npts * 4);
            m_detectorParms.set("ReturnCode", 0);
            m_detectorParms.set("InstActivityField", 0);
            m_detectorParms.set("InstErrorField", 0);
            m_detectorParms.set("CommsBufferLevelUsed", 0);
            m_detectorParms.set("Spare Int Parameter", 0);
            // RunTime unit is 100ms
            // NB: Data RunTime does not correct for the time spent paused.
            int time = (int)((System.currentTimeMillis() - m_runStart) / 100);
            LcMsg.postDebug("RtnSpec: runTime=" + time);
            m_detectorParms.set("RunTime", time);
            m_detectorParms.set("SigMod", 0);
            m_detectorParms.set("StartWL", 190);
            m_detectorParms.set("Count", npts);
            float[] data = new float[npts];
            double t = time - m_timePaused_ms / 100.0;
            double t_s = t / 10.0;
            for (int i = 0; i < npts; i++) {
                data[i] = (float)signal(t_s, m_lambdaMin + i);
            }
            m_detectorParms.set("AbsValue", data);
        } else if (code == ID_RSP_RETURN_MULTIPLE_SPECTRA.intValue()) {
            // Fake up one spectrum
            int npts = 210;
            int nspec = 1;
            int err = 0;
            JbCommands.Command cmd = m_cmd.get(code);
            int state = m_detectorParms.getInt("DetectorState");
            if (state != 6) {
                // No data -- not in run mode
                cmd.setSize(40);
                err = 9008 + 0x8000;
                nspec = 0;
            } else {
                err = 0;
                cmd.setSize(40 + npts * 4);
            }
            m_detectorParms.set("ReturnCode", err);
            m_detectorParms.set("InstActivityField", 0);
            m_detectorParms.set("InstErrorField", 0);
            m_detectorParms.set("CommsBufferLevelUsed", 0);
            m_detectorParms.set("NumberOfSpectraFollowing", nspec);
            m_detectorParms.set("Status", 0);
            // RunTime unit is 100ms
            // NB: Data RunTime does not correct for the time spent paused.
            int time = (int)((System.currentTimeMillis() - m_runStart) / 100);
            LcMsg.postDebug("RtnMultSpec: runTime=" + time + ", err=" + err);
            m_detectorParms.set("RunTime", time);
            m_detectorParms.set("SigMod", 0);
            m_detectorParms.set("StartWL", 190);
            m_detectorParms.set("Count", npts);
            float[] data = new float[npts];
            double t = time - m_timePaused_ms / 100.0;
            double t_s = t / 10.0;
            for (int i = 0; i < npts; i++) {
                data[i] = (float)signal(t_s, m_lambdaMin + i);
            }
            m_detectorParms.set("AbsValue", data);
        } else if (code == ID_RSP_METHOD_HEADER_335.intValue()) {
            m_lambdaMin = m_driverParms.getInt("Min Wavelength");
            m_lambdaMax = m_driverParms.getInt("Max Wavelength");
        } else if (code == ID_RSP_METHOD_LINE_ENTRY_335.intValue()) {
            m_lambda1 = m_driverParms.getFloat("Param Wavelength 1");
            m_detectorParms.set("Wavelength1", m_lambda1);
            m_lambda2 = m_driverParms.getFloat("Param Wavelength 2");
            m_detectorParms.set("Wavelength2", m_lambda2);
        } else if (code == ID_RSP_METHOD_ACTION.intValue()) {
            m_detectorParms.set("ReturnCode", 0);
            int state = 5;
            int action = m_driverParms.getInt("Method Action");
            switch (action) {
            case 1:             // Start
                m_runStart = System.currentTimeMillis();
                m_timePaused_ms = 0;
                //m_runStart = m_runPause = System.currentTimeMillis();
                state = 6;
                break;
            case 2:             // Resume
                m_timePaused_ms += System.currentTimeMillis() - m_runPause;
                //m_runStart = System.currentTimeMillis()
                //        - (m_runPause - m_runStart);
                state = 6;
                break;
            case 0:             // Stop
                m_runPause = System.currentTimeMillis();
                state = 7;
                break;
            case 3:             // Reset
            case 4:             // GoToReady
                state = 5;
                break;
            }
            m_detectorParms.set("DetectorState", state);
        } else if (code == ID_RSP_LAMP.intValue()) {
            m_detectorParms.set("ReturnCode", 0);
            int lamp = m_driverParms.getInt("Lamp");
            int action = m_driverParms.getInt("Lamp Action");
            switch (action) {
            case 0:             // Off
                if (lamp == 0 || lamp == 2) {
                    m_detectorParms.set("VisLampState", 0);
                }
                if (lamp == 1 || lamp == 2) {
                    m_detectorParms.set("D2LampState", 0);
                }
                m_detectorParms.set("DetectorState", 1);
                break;
            case 1:             // On
                if (lamp == 0 || lamp == 2) {
                    m_detectorParms.set("VisLampState", 1);
                }
                if (lamp == 1 || lamp == 2) {
                    m_detectorParms.set("D2LampState", 1);
                }
                m_detectorParms.set("DetectorState", 5);
                break;
            }
        } else if (code == ID_RSP_RETURN_STATUS.intValue()) {
            m_detectorParms.set("ReturnCode", 0);
            // RunTime unit is 0.01 minute
            int time = (int)((System.currentTimeMillis() - m_runStart) / 600);
            // NB: Status RunTime corrects for the time spent paused.
            time -= m_timePaused_ms / 600;
            m_detectorParms.set("RunTime", time);
            double time_s = time * 0.6;
            float au1 = (float)signal(time_s, m_lambda1);
            float au2 = (float)signal(time_s, m_lambda2);
            m_detectorParms.set("Absorbance1", new Float(au1));
            m_detectorParms.set("Absorbance2", new Float(au2));
        } else {
            processed = false;
        }
        return processed;
    }

    /**
     * Calculate the signal as a function of run time and wavelength.
     */
    private double signal(double time_s, double lambda) {
        double m = lambda - m_lambdaMin + 1;
        //return m * (1 - Math.cos(time_s / 6)) / 200.0; // Always non-negative
        return m * Math.cos(time_s / 6) / 50.0; // Test over/underflows
    }

    public static int getReplyFor(int command) {
        Integer itmp = sm_REPLIES.get(command);
        return (itmp == null) ? ID_RSP_UNKNOWN_COMMAND : itmp;
    }


    /**
     * Thread to monitor the input stream for commands.
     */
    public class Reader extends Thread {

        private boolean mm_quit = false;

        public Reader() {
            LcMsg.postDebug("Instrument.Reader created");
        }

        public void quit() {
            mm_quit = true;
        }

        public void run() {
            LcMsg.postDebug("Instrument.Reader running");
            try {
                InputStream inputStream = m_socket.getInputStream();
                m_in = new JbInputStream(inputStream);
                //m_in.setByteOrder(ByteOrder.LITTLE_ENDIAN);
            } catch (SocketException se) {
                LcMsg.postError("Cannot set timeout on 335 communication");
            } catch (IOException ioe) {
                LcMsg.postError("335.Reader: Couldn't get input stream"
                                   + " for the connection.");
            }

            while (!mm_quit) {
                int code = 0;
                try {
                    //
                    // Receive the message
                    //
                    code = m_in.readShort();
                    LcMsg.postDebug("335.Reader got code: 0x"
                                    + Integer.toHexString(code));
                    m_cmd.receiveCommand(code, m_in);
                } catch (SocketTimeoutException stoe) {
                    LcMsg.postError("335.Reader: Socket read timed out");
                    mm_quit = true;
                } catch (IOException ioe) {
                    LcMsg.postError("335.Reader: Socket read failed: " + ioe);
                    mm_quit = true;
                }

                if (!mm_quit) {
                    int replyCode = getReplyFor(code);

                    // Process this command
                    if (!processCommand(replyCode)) {
                        // No special processing, just update parameters
                        m_cmd.copyPars(code, m_driverParms, m_detectorParms);
                    }
                    
                    //
                    // Send the reply
                    //
                    m_cmd.sendReply(replyCode, m_out);
                }
            }
            LcMsg.postDebug("335.Reader: quitting");
            try {
                m_socket.close();
                m_ss.close();
            } catch (IOException ioe) {
            }
            JbInstrument machine = new JbInstrument(m_port);
        }
    }


    /**
     * Holds a map that looks up the appropriate reply for a command.
     */
    static class Replies extends HashMap<Integer, Integer> {

        /**
         * Initialize the map of command-reply pairs.
         */
        Replies() {
            for (int i = 0; i < mm_REPLIES.length; i++) {
                put(mm_REPLIES[i][0], mm_REPLIES[i][1]);
            }
        }

        /** Initial data -- command/reply pairs. */
        private Integer[][] mm_REPLIES = {
            {ID_CMD_GET_INST_IDENTITY, ID_RSP_GET_INST_IDENTITY},
            {ID_CMD_METHOD_ACTION, ID_RSP_METHOD_ACTION},

            {ID_CMD_METHOD_CHANGE_END_TIME, ID_RSP_METHOD_CHANGE_END_TIME},
            {ID_CMD_METHOD_CLEAR, ID_RSP_METHOD_CLEAR},
            {ID_CMD_SET_CELL_PARAMS, ID_RSP_SET_CELL_PARAMS},
            {ID_CMD_GET_CELL_PARAMS, ID_RSP_GET_CELL_PARAMS},
            {ID_CMD_SET_GLOBALS, ID_RSP_SET_GLOBALS},
            {ID_CMD_AUTO_ZERO, ID_RSP_AUTO_ZERO},
            {ID_CMD_LAMP, ID_RSP_LAMP},
            {ID_CMD_SET_CONFIG_OPTIONS, ID_RSP_SET_CONFIG_OPTIONS},
            {ID_CMD_SET_PROTO_PARAMS, ID_RSP_SET_PROTO_PARAMS},
            {ID_CMD_GET_PROTO_PARAMS, ID_RSP_GET_PROTO_PARAMS},
            {ID_CMD_SET_IP_PARAMS, ID_RSP_SET_IP_PARAMS},
            {ID_CMD_GET_IP_PARAMS, ID_RSP_GET_IP_PARAMS},
            {ID_CMD_RETURN_STATUS, ID_RSP_RETURN_STATUS},

            {ID_CMD_RETURN_RUN_LOG,ID_RSP_RETURN_RUN_LOG },
            {ID_CMD_RETURN_ERROR_LOG, ID_RSP_RETURN_ERROR_LOG},
            {ID_CMD_CLEAR_LOG, ID_RSP_CLEAR_LOG},
            {ID_CMD_GET_ERROR_CODE_STRINGS, ID_RSP_GET_ERROR_CODE_STRINGS},
            {ID_CMD_GET_RUN_LOG_STRINGS, ID_RSP_GET_RUN_LOG_STRINGS},

            {ID_CMD_GET_GLOBALS, ID_RSP_GET_GLOBALS},
            {ID_CMD_RETURN_METHOD_HEADER, ID_RSP_RETURN_METHOD_HEADER},
            {ID_CMD_RETURN_ALL_METHOD_LINES, ID_RSP_RETURN_ALL_METHOD_LINES},

            //{ID_CMD_NON_METHOD_ACTION, },
            {ID_CMD_EXTERNAL_MONITOR_CNTRL, ID_RSP_EXTERNAL_MONITOR_CNTRL},
            {ID_CMD_GET_EXT_MONITOR_CNTRL, ID_RSP_GET_EXT_MONITOR_CNTRL},
            {ID_CMD_EXTERNAL_MONITOR_END, ID_RSP_EXTERNAL_MONITOR_END},

            {ID_CMD_SET_DATE_TIME, ID_RSP_SET_DATE_TIME},
            {ID_CMD_GET_DATE_TIME, ID_RSP_GET_DATE_TIME},
            {ID_CMD_SET_LAMP_ALARM_TABLE, ID_RSP_SET_LAMP_ALARM_TABLE},
            {ID_CMD_GET_LAMP_ALARM_TABLE, ID_RSP_GET_LAMP_ALARM_TABLE},
            {ID_CMD_WRITE_STORAGE, ID_RSP_WRITE_STORAGE},
            {ID_CMD_READ_STORAGE, ID_RSP_READ_STORAGE},

            {ID_CMD_METHOD_HEADER_335, ID_RSP_METHOD_HEADER_335},
            {ID_CMD_METHOD_LINE_ENTRY_335, ID_RSP_METHOD_LINE_ENTRY_335},
            {ID_CMD_METHOD_END_335, ID_RSP_METHOD_LINE_ENTRY_335},/*SPECIAL*/
            {ID_CMD_GET_CAL_DATA_335, ID_RSP_GET_CAL_DATA_335},
            {ID_CMD_RETURN_SPECTRUM_335, ID_RSP_RETURN_SPECTRUM},
            {ID_CMD_RETURN_MULTIPLE_SPECTRA, ID_RSP_RETURN_MULTIPLE_SPECTRA},
        };
    }
}
