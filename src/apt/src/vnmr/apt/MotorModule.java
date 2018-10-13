/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */


package vnmr.apt;

import java.io.*;
import java.net.*;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Random;
import java.util.StringTokenizer;

import vnmr.util.Fmt;
import static java.lang.Math.*;
import static vnmr.apt.ChannelInfo.Limits;
import static vnmr.apt.ChannelInfo.SwitchPosition;
import static vnmr.apt.ChannelInfo.TunePosition;
import static vnmr.apt.MotorControl.MAX_MOTORS;


/**
 * This class simulates a set of ProTune motor modules.
 */
public class MotorModule implements AptDefs {
    public static MotorModule m_motorModule = null;
    private final static int CONNECT_TIMEOUT = 0;
    private final static int SOCKET_READ_TIMEOUT = 0;

    private static String sm_vnmrSystemDir = "/vnmr";
    private static final String BASEDIR = ProbeTune.getVnmrSystemDir() + "/tmp";
    private static String sm_probeName;
    private static boolean sm_isAbort = false;

    private static String sm_capPath = BASEDIR + "/ptuneCapPositions";
    private static String sm_motorPath = BASEDIR + "/ptuneMotorPositions";
    private static String sm_limitsPath = BASEDIR + "/ptuneMotorLimits";

    private int m_port = 0;
    private ServerSocket m_ss = null;
    private Socket m_socket = null;
    private boolean m_ok = false;

    static private double sm_stepsPerRev;
    static private long[] sm_motorFlags = new long[MAX_MOTORS];
    static private long[] sm_odometer = new long[MAX_MOTORS];
    static private int[] sm_ranges = new int[MAX_MOTORS];
    static private String sm_serialNumber = "0000000000";
    static private String sm_eeVersion = "A0.0";
    static private boolean sm_tuneMode = true;
    static private int sm_tuneBand = 0; // NB: Reply refers to this as "mode"!
    static private String sm_swVersion = "PTMM 1.6.sim";
    static private int sm_relayLogic = 0;
    static private int sm_magLegStatus = 0; // 0 ==> OK
    static private String sm_module_id = "1234567";
    static private int[] sm_OneProbeBacklash = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0,
                                                1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                11,12,13,14,15,16,17,18,19,20,
                                                19,18,17,16,15,14,13,12,11,10,
                                                9, 8, 7, 6, 5, 4, 3, 2, 1};
    static private int[] m_motorStatus = new int[MAX_MOTORS];

    /** For reading from the host. */
    static private BufferedReader m_in;

    /** For writing to the host. */
    static private PrintWriter m_out;

    /** The thread that monitors the input stream. */
    private Reader m_reader = null;
    private boolean m_forceQuit = false;

    static {
        for (int i = 0; i < MAX_MOTORS; i++) {
            sm_motorFlags[i] = System.currentTimeMillis() / 1000;
        }
    }



    public static void main(String[] args) {
        boolean initMotorPositions = false;

        int len = args.length;
        for (int i = 0; i < len; i++) {
            if (args[i].equalsIgnoreCase("-vnmrsystem") && i + 1 < len) {
                sm_vnmrSystemDir = args[++i];
            } else if (args[i].equalsIgnoreCase("-probe") && i + 1 < len) {
                sm_probeName = args[++i];
            } else if (args[i].equalsIgnoreCase("-init")) {
                initMotorPositions = true;
            }
        }
        if (MotorModule.isOptima(sm_probeName)) {
            MotorControl.setFirmwareVersion("PTOPT");
        } else if (MotorModule.isPZT(sm_probeName)) {
            MotorControl.setFirmwareVersion("PZT");
        } else {
            MotorControl.setFirmwareVersion("PTMM");
        }
        sm_stepsPerRev = isOptima() ? 720 : 200;
        int port = DEBUG_MOTOR_PORT;
        if (isPZT()) {
            sm_swVersion = sm_swVersion.replace("PTMM", "PZT");
        } else if (isOptima()) {
            sm_swVersion = sm_swVersion.replace("PTMM", "PTOPT");
        }
        sm_swVersion += new SimpleDateFormat(" yyyy-MM-dd").format(new Date());
        setGlobalPaths();

        // Start up a virtual motor module
        m_motorModule = new MotorModule(port, initMotorPositions);
    }

    private static boolean isOptima() {
        return isOptima(sm_probeName);
    }

    public static boolean isOptima(String probeName) {
        return probeName != null && probeName.toLowerCase().contains("optima");
    }

    private static boolean isPZT() {
        return isPZT(sm_probeName);
    }

    public static boolean isPZT(String probeName) {
        return probeName != null && probeName.toLowerCase().contains("pzt");
    }

    private static void setGlobalPaths() {
        String basedir = sm_vnmrSystemDir + "/tmp";
        sm_capPath = basedir + "/ptuneCapPositions";
        sm_motorPath = basedir + "/ptuneMotorPositions";
        sm_limitsPath = basedir + "/ptuneMotorLimits";
        System.out.println("basedir=" + basedir);
    }

    public MotorModule(int port, boolean initMotorPositions) {
        m_ok = true;
        m_port = port;

        initializeMotors();
        if (initMotorPositions) {
            setMotorsToDefaultPositions();
        }

        try {
            Messages.postDebug("MotorModule: opening server socket on port "
                               + port);
            m_ss = new ServerSocket(port);
        } catch (IOException ioe) {
            Messages.postError("MotorModule: IO exception getting socket: "
                               + ioe);
            m_ok = false;
        } catch (SecurityException se) {
            Messages.postError("MotorModule: Security exception");
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
                Messages.postError("MotorModule: \"accept\" timed out");
                m_ok = false;
            } catch (java.nio.channels.IllegalBlockingModeException ibme) {
                Messages.postError("MotorModule: Illegal blocking");
                m_ok = false;
            } catch (IOException ioe) {
                Messages.postError("MotorModule: IO exception starting socket: "
                                + ioe);
                m_ok = false;
            } catch (SecurityException se) {
                Messages.postError("MotorModule: Security exception");
                 m_ok = false;
            }
        }

        if (!m_ok) {
            return;
        }
        Messages.postDebug("MotorModule is connected");

        try {
            m_socket.setSoTimeout(SOCKET_READ_TIMEOUT);
            OutputStream outputStream = m_socket.getOutputStream();
            m_out = new PrintWriter(outputStream, true);
        } catch (SocketException se) {
            Messages.postDebug("MotorModule: Cannot set timeout "
                               + "on MotorModule communication");
        } catch (IOException ioe) {
            Messages.postError("MotorModule: Couldn't get output stream"
                               + " for the connection.");
        }

        m_reader = new Reader();
        m_reader.start();
    }

    /**
     * Sets all the motors to default positions from the "chan#N" files. For
     * each channel file, in reverse numerical order, sets the motors for the
     * switch positions and for the first "Ref" line in the file.
     */
    private void setMotorsToDefaultPositions() {
        // Set motor positions for all channels
        String persistDir = sm_vnmrSystemDir + "/tune/" + sm_probeName;
        File[] chanFiles = null;
        // Use all the channels in the directory
        chanFiles = new File(persistDir).listFiles(new ChanFileFilter());
        if (chanFiles == null) {
            System.err.println("Cannot read directory \""
                               + persistDir + "\"");
        } else if (chanFiles.length == 0) {
            System.err.println("No channel files in \""
                               + persistDir + "\"");
        }

        Arrays.sort(chanFiles, new PFileNameComparator<File>());

        for (int i = chanFiles.length - 1; i >= 0; --i) {
            File chanFile = chanFiles[i];
            ChannelInfo ci = new ChannelInfo(chanFile.getPath());

            // Set the fixed-position switches
            SwitchPosition[] splist = ci.getFixedSwitches();
            for (SwitchPosition sp : splist) {
                Motor motor = sm_motors[sp.getMotor()];
                motor.setPosition(sp.getPosition(), false);
            }

            // Set motors for first tune position
            TunePosition tp = ci.getPositionAt(Double.NaN,
                                               null, 0, 0, 0, true);
            if (tp != null) {
                // Set freq-dependent switches
                splist = ci.getFrequencyDependentSwitches();
                for (SwitchPosition sp : splist) {
                    Motor motor = sm_motors[sp.getMotor()];
                    motor.setPosition(sp.getPosition(tp.getFreq()), false);
                }

                // Set tune and match and any slaves
                int n = tp.getNumberOfMotors();
                for (int cmi = 0; cmi < n; cmi++) {
                    int gmi = ci.getMotorNumber(cmi);
                    if (gmi >= 0) {
                        int posn = tp.getPosition(cmi);
                        Motor motor = sm_motors[gmi];
                        motor.setPosition(posn, false);
                        while ((gmi = ci.getSlaveOf(gmi)) >= 0) {
                            motor = sm_motors[gmi];
                            motor.setPosition(posn, false);
                        }
                    }
                }
            }
        }
        writeMotorPositions();
    }

    /**
     * Sets the properties of all the motors from their "motor#N" files.
     * Sets their positions from the MotorPositions file.
     * Writes out the MotorLimits file.
     * Does nothing if the "probe name" has not been set.
     * Motors without corresponding motor files are given default properties.
     */
    protected void initializeMotors() {
        if (sm_probeName == null) {
            return; // use legacy mode
        }
        int[] positions = readMotorPositions();
        String persistenceDir = sm_vnmrSystemDir + "/tune/" + sm_probeName;
        for (int gmi = 0; gmi < sm_motors.length; gmi++) {
            MotorInfo mi = new MotorInfo(gmi, persistenceDir,
                                         (int)sm_stepsPerRev);
            int resonance = 0; // Legacy
            String label = mi.getMotorName(); // Legacy

            sm_motors[gmi] = new Motor(label, resonance, gmi);
            int backlash = (int)mi.getBacklash();
            sm_motors[gmi].setBacklash(backlash);
            sm_motors[gmi].setLimits(mi.getMinlimit(), mi.getMaxlimit());
            sm_motors[gmi].setPosition(positions[gmi], false);
            if (gmi == 0 && sm_probeName.contains("One")) {
                sm_motors[gmi].setBacklashFunction(sm_OneProbeBacklash);
            }
        }
        writeMotorLimits();
    }

    /**
     * Write out the limits for each motor in the MotorLimits file.
     * There is one line for each motor (in order of gmi) and the
     * format of each line is "min_position:max_position".
     */
    static private void writeMotorLimits() {
        StringBuffer sbuf = new StringBuffer();
        for (int gmi = 0; gmi < sm_motors.length; gmi++) {
            Limits limits = sm_motors[gmi].getCapLimits(); // Includes backlash
            sbuf.append(limits.toString()).append(NL);
        }
        TuneUtilities.writeFileSafely(sm_limitsPath, sbuf.toString());
    }

    static public void quit(int ms) throws InterruptedException, IOException {
        m_motorModule.m_reader.quit(ms);
        m_motorModule.m_ss.close();
        //-->m_motorModule.m_ss.socket().close();
        m_out.close();
    }

    protected Reader getReader() {
        return m_reader;
    }

    static protected int getMotorPosition(int motorIdx) {
        Motor motor = getMotor(motorIdx);
        if (motor != null) {
            return motor.getMotorPosition();
        } else {
            return 0;
        }
    }

    static protected int getCapPosition(int motorIdx) {
        Motor motor = getMotor(motorIdx);
        if (motor != null) {
            return motor.getCapPosition();
        } else {
            return 0;
        }
    }

    static protected boolean gotoMotorPosition(int motorIdx, int position) {
        boolean ok = false;
        Motor motor = getMotor(motorIdx);
        if (motor != null) {
            ok = motor.gotoMotorPosition(position);
        }
        return ok;
    }

    static protected void setMotorPosition(int motorIdx, int position) {
        Motor motor = getMotor(motorIdx);
        if (motor != null) {
            motor.setPosition(position, true);
        }
    }

    static protected Motor getMotor(int motorIdx) {
        if (motorIdx < 0 || motorIdx >= sm_motors.length) {
            return null;
        } else {
            return sm_motors[motorIdx];
        }
    }

    protected String processCommand(String msg) {
        String reply = "Motor got invalid command: \"" + msg + "\"";
        String[] tokens = msg.split("[ \\t]+");
        int nTokens = tokens.length; // # of tokens, including command name
        String cmd = tokens[0].toLowerCase();
        //System.out.println("cmd=" + msg);

        if (cmd.equals("step")) {
            sm_isAbort = false;
            if (nTokens == 3) {
                Messages.postDebug(msg);
                int motor = Integer.parseInt(tokens[1]);
                int nsteps = Integer.parseInt(tokens[2]);
                boolean isIndexing = Math.abs(nsteps) == 200000;
                nsteps = applyNoiseToMotorMotion(nsteps);
                int p0 = getMotorPosition(motor);
                int position = p0 + nsteps;
                if (isIndexing) {
                    position = (int)((random() - 0.5) * 40);
                    if (nsteps > 0) {
                        position += (int)(sm_motors[motor].getRange().getMax());
                    }
                };
                boolean eot = (gotoMotorPosition(motor, position) == false);
                sm_odometer[motor] += Math.abs(getMotorPosition(motor) - p0);
                if (sm_probeName == null) {
                    setCapPositions(); // Legacy mode
                }
                writeMotorPositions();
                //setCapPositions(sm_motorPositions);
                String status = eot || isIndexing ? " eot " : " ok ";
                if (sm_isAbort) {
                    sm_isAbort = false;
                    status = " a ";
                }
                reply = "Step " + motor + status + getMotorPosition(motor);
                Messages.postDebug(reply);
                Messages.postDebug("capPosition[" + motor + "]="
                                   + getCapPosition(motor));
            }

        } else if (cmd.equals("index")) {
            if (nTokens == 2) {
                int motor = Integer.parseInt(tokens[1]);
                int p0 = getMotorPosition(motor);
                sm_ranges[motor] = (int)(sm_motors[motor].getRange().getMax());
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) { }
                int delta = (int)((random() - 0.5) * 40);
                sm_odometer[motor] += p0 - delta;
                m_out.println("Reindex " + motor + " " + delta);
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) { }
                sm_odometer[motor] += sm_ranges[motor];
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) { }
                sm_odometer[motor] += sm_ranges[motor] - getMotorPosition(motor);
                //sm_odometer[motor] += 2 * sm_ranges[motor] - delta;
                m_motorStatus[motor] |= 4;
                reply = "Index " + motor + " ok " + getMotorPosition(motor); 
            }

        } else if (cmd.equals("range")) {
            if (nTokens == 2) {
                int motor = Integer.parseInt(tokens[1]);
                reply = "Range " + motor + " " + sm_ranges[motor];
            }

        } else if (cmd.equals("position")) {
            int motor = 0;
            int position = 0;
            if (nTokens >= 2) {
                motor = Integer.parseInt(tokens[1]);
            }
            if (nTokens == 2) {
                position = getMotorPosition(motor);
                reply = "Position " + motor + " " + position;
                if (!isOptima()) {
                    reply += " ";
                    reply += Fmt.f(4, Math.sin((Math.PI * position) / 200));
                }
            }
            if (nTokens == 3 && isOptima()) {
                position = Integer.parseInt(tokens[2]);
                setMotorPosition(motor, position);
                reply = "Position " + motor + " " + position;
            }

        } else if (cmd.equals("setposition")) {
            if (nTokens == 3) {
                int motor = Integer.parseInt(tokens[1]);
                int position = Integer.parseInt(tokens[2]);
                setMotorPosition(motor, position);
                reply = null;
                if (isOptima()) {
                    reply = "SetPosition " + motor + " " + position;
                }
            }

        } else if (cmd.equals("abort")) {
            sm_isAbort = true;
            if (isOptima()) {
                reply = "Abort ok";
            } else {
                reply = "#\"abort\" command not implemented";
            }

        } else if (cmd.equals("setflag")) {
            if (nTokens == 3) {
                int motor = Integer.parseInt(tokens[1]);
                int value = Integer.parseInt(tokens[2]);
                sm_motorFlags[motor] = value;
                reply = null;
            }

        } else if (cmd.equals("getflag")) {
            if (nTokens == 2) {
                int motor = Integer.parseInt(tokens[1]);
                reply = "flag " + motor + " " + sm_motorFlags[motor];
            }

        } else if (cmd.equals("tune")) {
            if (nTokens == 1) {
                reply = "tune " + (sm_tuneMode == false ? 0 : 1);
            } else if (nTokens == 2) {
                int tuneTimeout = Integer.parseInt(tokens[1]);
                sm_tuneMode = (tuneTimeout == 0 ? false : true);
                // TODO: Turn off tune mode after "timeout" seconds
                reply = null;
            }

        } else if (cmd.equals("htune")
                   || cmd.equals("xtune")
                   || cmd.equals("dcpl"))
        {
            if (nTokens == 1) {
                if (cmd.equals("htune")) {
                    sm_tuneBand = 0;
                } else if (cmd.equals("xtune")) {
                    sm_tuneBand = 1;
                } else if (cmd.equals("dcpl")) {
                    sm_tuneBand = 2;
                } else {
                    sm_tuneBand = -1;
                }
                reply = "tune mode: " + sm_tuneBand;
            }

        } else if (cmd.equals("version")) {
            if (isOptima()) {
                reply = "Version " + sm_swVersion;
            } else {
                int motor = 0;
                try {
                    motor = Integer.parseInt(tokens[1]);
                } catch (ArrayIndexOutOfBoundsException aioob){
                }
                reply = "Motor " + motor + " firmware version " + sm_swVersion;
            }

        } else if (cmd.equals("reverse")) {
            if (nTokens == 1) {
                sm_relayLogic ^= 1;
                reply = "tune logic reversed?:  " + sm_relayLogic;
            }

        } else if (cmd.equals("getmlstat")) {
            reply = "mlstat " + sm_magLegStatus;

        } else if (cmd.equals("module_id")) {
            reply = "Module ID: " + sm_module_id;

        } else if (cmd.equals("status")) {
            if (nTokens == 2) {
                int mtr = Integer.parseInt(tokens[1]);
                reply = "status " + mtr + " " + m_motorStatus [mtr];
            }
        } else if (cmd.equals("calibrate") && isOptima()) {
            if (nTokens == 4) {
                int motor = Integer.parseInt(tokens[1]);
                m_motorStatus[motor] |= 1;
                reply = "Calibrate " + motor + " ok "
                        + tokens[3] + " " + tokens[3];
            }
        } else if (cmd.equals("odometer") && (isOptima() || isPZT())) {
            if (nTokens == 2) {
                int mtr = Integer.parseInt(tokens[1]);
                reply = ("Odometer " + mtr + " "
                        + (int)Math.rint(sm_odometer[mtr] / sm_stepsPerRev ));
            }
        } else if (cmd.equals("magneticfield") && (isOptima())) {
            if (nTokens == 1) {
                reply = "MagneticField 0";
            }
        } else if (cmd.equals("resetodometer") && isOptima()) {
            if (nTokens == 2) {
                int mtr = Integer.parseInt(tokens[1]);
                sm_odometer[mtr] = 0;
                reply = "ResetOdometer " + mtr + " ok";
            }
        } else if (cmd.equals("serialnumber") && isOptima()) {
            if (nTokens == 2) {
                if (tokens[1].length() <= 10) {
                    sm_serialNumber = tokens[1];
                }
            }
            if (nTokens <= 2) {
                reply = "SerialNumber " + sm_serialNumber;
            }
        } else if (cmd.equals("eeversion") && isOptima()) {
            if (nTokens == 2) {
                if (tokens[1].length() <= 4) {
                    sm_eeVersion = tokens[1];
                }
            }
            if (nTokens <= 2) {
                reply = "EEVersion " + sm_eeVersion;
            }
        } else if (cmd.equals("setparameter") && isOptima()) {
            if (nTokens == 1) {
                reply = "Parameters reset";
            }
        }

        return reply;
    }


    private int applyNoiseToMotorMotion(int nsteps) {
        int absSteps = Math.abs(nsteps);
        int noise = (int)Math.round(Math.random() * 3 - 2); // -2 <= noise <= 1
        absSteps += noise;
        absSteps = Math.max(absSteps, 0);
        nsteps = (nsteps > 0) ? absSteps : -absSteps;
        return nsteps;
    }


    static private int nResonances = 2; // Legacy
    static private Motor[] sm_motors = new Motor[MAX_MOTORS];
    static protected Mtune.CapPositions[] sm_capPositions
        = new Mtune.CapPositions[nResonances]; // Legacy

    /**
     * Calculate the positions of all the capacitors, based on all the
     * motor positions, and write the capacitor tweaks to a file.
     */
    static private void setCapPositions() {
        // Set all capacitor positions to their nominal values
        //sm_capPositions = new Mtune.CapPositions[nResonances];
        for (int i = 0; i < sm_capPositions.length; i++) {
            if (sm_capPositions[i] != null) {
                sm_capPositions[i].clear();
            }
        }

        // Create the motors, if necessary
        for (int i = 0; i < sm_motors.length; i++) {
            if (sm_motors[i] == null) {
                int resonance = i / 2;
                String label = i % 2 == 0 ? "tune" : "match";

                // Simulate motor #2 as a slave motor to #0
                if (i == 2) {
                    resonance = 0;
                    label = "tune";
                }

                sm_motors[i] = new Motor(label, resonance, i);
                sm_motors[i].setBacklash(100 + 10 * i);
            }
        }

        // Add the effect of each motor position to each capacitor position
        for (int i = 0; i < sm_motors.length; i++) {
            sm_capPositions = sm_motors[i].adjustCaps(sm_capPositions,
                                                      getCapPosition(i));
        }

        // Write the capacitor adjustments to file
        StringBuffer sbuf = new StringBuffer();
        for (int i = 0; i < sm_capPositions.length; i++) {
            sbuf.append("tune\t").append(i).append("\t");
            sbuf.append(sm_capPositions[i].tune).append(NL);

            sbuf.append("match\t").append(i).append("\t");
            sbuf.append(sm_capPositions[i].match).append(NL);
        }
        TuneUtilities.writeFileSafely(sm_capPath, sbuf.toString());
    }

    static private int[] readMotorPositions() {
        int[] posns = new int[sm_motors.length];
        String buf = TuneUtilities.readFile(sm_motorPath, true);
        StringTokenizer toker = new StringTokenizer(buf);
        for (int i = 0; i < sm_motors.length; i++) {
            if (toker.hasMoreTokens()) {
                String line = toker.nextToken();
                StringTokenizer toker2 = new StringTokenizer(line, ": ");
                if (toker2.countTokens() == 1) {
                    try {
                        posns[i] = Integer.parseInt(toker2.nextToken());
                    } catch (NumberFormatException nfe) {
                        posns[i] = 0;
                    }
                }
            } else {
                posns[i] = 0;
            }
        }
        return posns;
    }

    static private void writeMotorPositions() {
        StringBuffer sbuf = new StringBuffer();
        for (Motor motor : sm_motors) {
            sbuf.append(motor.getCapPosition()).append(NL);
        }
        TuneUtilities.writeFileSafely(sm_motorPath, sbuf.toString());
    }

    /**
     * This class holds the mapping of motor position to capacitor
     * tweaks for one motor. It also holds the motor position,
     * the backlash, and enough of the history of the motor motion
     * to calculate the capacitor position. (The capacitor position
     * is the motor position adjusted to simulate the effect of backlash.)
     * Moving a motor may move more than one resonance, e.g., both the
     * F and H resonances on an HFX probe.
     */
    static public class Motor {

        /** Limits of travel for this motor -- taken from motor#X file. */
        private int mm_motorMin = 0;
        private int mm_motorMax = 20000;
        private int mm_backlash;

        private int mm_motorPosition = (mm_motorMax + mm_motorMin) / 2;

        /** Motor position with simulated perfect backlash */
        private int mm_cleanCapPosition = mm_motorPosition;

        /** Motor position with simulated sloppy backlash. */
        private int mm_capPosition = mm_motorPosition;

        /** Random number generator to put noise into the cap positions. */
        private Random mm_random = new Random(0);

        /** The current offset in the backlash (noise). */
        private int mm_backlashOffset;

        /** The current offset in the step position (noise). */
        private int mm_stepOffset;

        /** The standard deviation in the step offset. */
        private double mm_stepNoise = 1;

        /** The standard deviation in the backlash offset. */
        private double mm_backNoise = 4;

        /** An array of any length giving the behavior of the backlash. */
        private int[] mm_backlashFunction = {0};

        /** Resonance numbers to tweak. */
        private int[] resonanceIndex;

        /** Whether the tweak is to the "tune" or "match" capacitor. */
        private String[] resonanceLabel;

        /** Minimum tweak values. */
        private double[] tweakMin;

        /** Maximum tweak values. */
        private double[] tweakMax;

        /** Motor position to capacitor tweak parameter: intercept. */
        private double[] mm_t0;

        /** Motor position to capacitor tweak parameter: intercept. */
        private double[] mm_dtdp;
        private int mm_gmi;
        //private int mm_prevStep;


        /**
         * Create a Motor for a motor affecting one resonance in
         * either the "tune" or "match" dimension.
         * @param label Either "tune" or "match", depending on which
         * virtual capacitor this motor controls.
         * @param resonance Which virtual resonance this Motor affects (from 0).
         * @param gmi The Global Motor Index (gmi) of this motor (from 0).
         */
        public Motor(String label, int resonance, int gmi) {
            mm_gmi = gmi;
            resonanceLabel = new String[1];
            resonanceLabel[0] = label;
            resonanceIndex = new int[1];
            resonanceIndex[0] = resonance;
            tweakMin = new double[1];
            tweakMin[0] = -0.8;
            tweakMax = new double[1];
            tweakMax[0] = 1;
            initialize();
        }

        public void setBacklash(int backlash) {
            mm_backlash = backlash;
        }

        public void setBacklashFunction(int[] function) {
            mm_backlashFunction = function;
        }

        private void initialize() {
            int len = resonanceIndex.length;
            mm_dtdp = new double[len];
            mm_t0 = new double[len];
            for (int i = 0; i < len; i++) {
                mm_dtdp[i] = ((tweakMax[i] - tweakMin[i])
                              / (mm_motorMax - mm_motorMin));
                mm_t0[i] = tweakMin[i] - mm_motorMin * mm_dtdp[i];
            }
            initializeMotorPosition();
        }

        public int getMotorPosition() {
            return mm_motorPosition;
        }

        /**
         * Get the capacitor position for this motor.
         * This is just the motor position, modified by the backlash.
         * @return The position.
         */
        public int getCapPosition() {
            return mm_capPosition;
        }

        /**
         * Set the motor and capacitor positions both to the given value.
         * The value is clipped to be in the range of this motor.
         * @param position The given position.
         * @param force If true, the given position is never clipped.
         */
        public void setPosition(int position, boolean force) {
            if (!force) {
                position = (int)getCapLimits().clip(position);
            }
            mm_motorPosition = mm_cleanCapPosition = mm_capPosition = position;
        }

        /**
         * Set the range of possible positions for this motor.
         * Does not include allowance for any backlash.
         * @param minPosition Minimum legal motor position.
         * @param maxPosition Maximum legal motor position.
         */
        public void setLimits(int minPosition, int maxPosition) {
            mm_motorMin = minPosition;
            mm_motorMax = maxPosition;
        }

        /**
         * Get the hard-stop limits for this motor as deduced from the
         * motor file.
         * @return The "hard" limits on the motor position.
         */
        public Limits getRange() {
            return new Limits(0, mm_motorMax + mm_motorMin);
        }

        /**
         * Get the limits on the capacitor position that may be legally set
         * for this motor. This is the same as the limit on motor positions,
         * except that the minimum position is increased by the backlash.
         * That is because we would have to move the motor to "backlash" steps
         * below its minimum value to get the capacitor to that same minimum
         * value.
         * @return The limits on the capacitor position.
         */
        public Limits getCapLimits() {
            int mincap = mm_motorMin + mm_backlash;
            int maxcap = mm_motorMax;
            return new Limits(mincap, maxcap);
        }

        /**
         * Move the motor slowly to the requested position, or to the minimum
         * or maximum legal position.
         * The time in this method approximates the time taken to move a
         * real motor.
         * Position updates are sent as the motor is "moving" into position.
         * The "capacitor position" for this motor is also updated, taking
         * the backlash into account.
         * @param position The position to move the motor to.
         * @return True if the motor was moved to the requested position.
         */
        public boolean gotoMotorPosition(int position) {
            int pos = Math.max(mm_motorMin, Math.min(mm_motorMax, position));
            if (pos == mm_motorPosition) {
                return true;
            }
            calculateCapPosition(pos);

            final int SETUP_PAUSE = 200; // Overhead: initial pause (ms)
            final int STEP = 60; // Steps between real-time updates
            final int OPTIMA_STEP = 12; // Optima steps between updates
            // Time between real-time updates:
            final int PAUSE = (int)Math.round(2.0 * STEP); // ms
            final int OPTIMA_PAUSE = 10; // ms to go "OPTIMA_STEPS" steps
            
            int step = isOptima() ? OPTIMA_STEP : STEP;
            int pause = isOptima() ? OPTIMA_PAUSE : PAUSE;
            int delta = (pos - mm_motorPosition > 0) ? step : -step;
            int n = Math.abs(pos - mm_motorPosition) / step;
            try {
                Thread.sleep(SETUP_PAUSE);
            } catch (InterruptedException e) {
            }
            for (int i = mm_motorPosition, j = 0; j < n; i += delta, j++)
            {
                if (i != mm_motorPosition) {
                    // Send real-time position update
                    String reply = "RTPosition " + mm_gmi + " " + i;
                    //if (!isOptima()) {
                        // Add a torque value emulating that sent by the PZT
                        // bench-test setup:
                        double torque = 1 + (mm_gmi + sin((PI * i) / 200)) / 30;
                        reply += " " + Fmt.f(5, torque);
                    //}
                    m_out.println(reply);
                }
                if (sm_isAbort) {
                    pos = i;
                    break;
                }
                try {
                    Thread.sleep(pause);
                } catch (InterruptedException e) {
                }
                System.out.print(".");
            }
            mm_motorPosition = pos;
            return (pos == position);
        }

        /**
         * Calculate the new virtual capacitor position that would result from
         * moving the motor to the specified position, allowing for backlash.
         * @param pos The new motor position.
         */
        private void calculateCapPosition(int pos) {
            // Backlash calculation:
            // The cap position is always between (motorPosition) and
            // (motorPosision + backlash).
            // Usually, the cap position is (motorPosition) when we are
            // stepping in a positive direction,
            // and (motorPosision + backlash) when we are stepping in a
            // negative direction.
            // However, after a reversal, we may be somewhere in between.
            // So we compare the new position (meaning the position from
            // the "usual" cap position calculation) with the
            // previous cap position. If the new position is farther in the
            // direction of the current motion than the previous position,
            // the new position becomes the current position; otherwise
            // the current position is left unchanged.
            int prevPosition = getMotorPosition();
            if (pos == prevPosition) {
                return; // No motion; nothing to do
            }
            int thisDir = (pos - prevPosition > 0 ? 1 : -1);
            int backlashCorrection = (thisDir > 0) ? 0 : mm_backlash;
            int trialCapPosition = pos + backlashCorrection;
            int x = (trialCapPosition - mm_cleanCapPosition) * thisDir;
            if (x > 0) {
                // Backlash has all been taken up
                randomizeStepOffset();
                mm_cleanCapPosition = trialCapPosition;
                mm_capPosition = mm_cleanCapPosition + getOffset();
            } else { // x < 0
                // We're in the backlash region; cleanCapPosition won't change
                randomizeBacklashOffset();
                mm_capPosition = mm_cleanCapPosition;
                mm_capPosition += thisDir * getBacklashValue(x);
            }
        }

        /**
         * Get the offset due to backlash as a function of position within
         * the backlash region.
         * @param x The (negative) distance from the end of the backlash region.
         * @return The
         */
        private int getBacklashValue(int x) {
            // Normalize argument to a positive value between
            // 0 and the size of the table specifying the backlash function.
            int len = mm_backlashFunction.length - 1;
            x = (int)(max(0.0, mm_backlash + x) * len / mm_backlash);
            return getOffset() - mm_backlashFunction[x];
        }

        /**
         * Set the offset in the backlash correction to a small random integer.
         */
        private void randomizeStepOffset() {
            mm_stepOffset = (int)(mm_random.nextGaussian() * mm_stepNoise);
        }

        /**
         * Set the offset in the backlash correction to a small random integer.
         */
        private void randomizeBacklashOffset() {
            mm_backlashOffset = (int)(mm_random.nextGaussian() * mm_backNoise);
        }

        /**
         * Get the current value of the backlash offset + step offset.
         * @return The offset.
         */
        private int getOffset() {
            return mm_backlashOffset + mm_stepOffset;
        }

        public void initializeMotorPosition() {
            mm_motorPosition = (int)Math.round(-mm_t0[0] / mm_dtdp[0]);
        }

        /**
         * Adjust the capacitor positions for this motor.
         * @param capPosns The initial capacitor positions.
         * @param motorPosition The current position of this motor.
         * @return The (modified) capacitor positions.
         *
         */
        public Mtune.CapPositions[] adjustCaps(Mtune.CapPositions[] capPosns,
                                               int motorPosition) { // Legacy

            if (capPosns == null) {
                capPosns = new Mtune.CapPositions[nResonances];
            }

            for (int j = 0; j < resonanceIndex.length; j++) {
                int idx = resonanceIndex[j];
                if (idx >= nResonances) {
                    Messages.postDebug("MotorModule",
                                       "MotorModule.adjustCaps: "
                                       + "cannot track resonance " + idx);
                } else {
                    if (capPosns[idx] == null) {
                        capPosns[idx] = new Mtune.CapPositions();
                    }
                    double tweak = mm_t0[j] + mm_dtdp[j] * motorPosition;
                    if ("tune".equals(resonanceLabel[j])) {
                        capPosns[idx].tune += tweak;
                    } else if ("match".equals(resonanceLabel[j])) {
                        capPosns[idx].match += tweak;
                    }
                }
            }
            return capPosns;
        }
    }


    /**
     * Thread to monitor the input stream for commands.
     */
    public class Reader extends Thread {

        private volatile boolean mm_quit = false;

        public Reader() {
            Messages.postDebug("Instrument.Reader created");
        }

        public void quit(int ms) throws InterruptedException, IOException {
            //m_reader.quit(ms);
            m_forceQuit = true;
            quit();
            //->m_ss.socket().close();
            m_ss.close();
            interrupt();
            join(ms);
        }

        public void quit() { // TODO: rename this "reset"?
            mm_quit = true;
        }

        public void run() {
	    setName("MotorModule.Reader");
            Messages.postDebug("Instrument.Reader running");
            try {
                InputStream inputStream = m_socket.getInputStream();
                m_in = new BufferedReader(new InputStreamReader(inputStream));
            } catch (SocketException se) {
                Messages.postError("Cannot set timeout on MotorModule comm");
            } catch (IOException ioe) {
                Messages.postError("MotorModule.Reader: Couldn't get input "
                                   + "stream for the connection.");
            }

            while (!mm_quit) {
                String cmd = "";
                try {
                    //
                    // Receive the message
                    //
                    cmd = m_in.readLine();
                    if (cmd == null) {
                        quit();
                    } else if ("quit".equals(cmd)) {
                        quit(); 
                        m_forceQuit = true;
                    } else {
                        Messages.postDebug("MotorModule",
                                           "Got command: \"" + cmd + "\"");
                    }
                } catch (SocketTimeoutException stoe) {
                    Messages.postError("MotorModule.Reader: "
                                       + "Socket read timeout");
                    quit();
                } catch (IOException ioe) {
                    Messages.postError("MotorModule.Reader: "
                                       + "Socket read failed: " + ioe);
                    quit();
                }

                if (!mm_quit) {

                    new CommandProcessor(cmd).start();
//                    String reply = processCommand(cmd);
//
//                    // Send the reply
//                    if (reply != null) {
//                        Messages.postDebug("MotorModule",
//                                           "Send reply: \"" + reply + "\"");
//                        m_out.println(reply);
//                    }
                }
            }
            Messages.postDebug("MotorModule.Reader: quitting");
            try {
                m_socket.close();
            } catch (Exception e) {}
            try {
                m_ss.close();
            } catch (Exception e) {}
            if (!m_forceQuit) {
                new MotorModule(m_port, false);
            }
        }
    }

    class CommandProcessor extends Thread {
        private String mm_command;
        
        public CommandProcessor(String cmd) {
            mm_command = cmd;
        }
        
        public void run() {
            String reply = processCommand(mm_command);

            // Send the reply
            if (reply != null) {
                Messages.postDebug("MotorModule",
                                   "Send reply: \"" + reply + "\"");
                m_out.println(reply);
            }
        }
    }

}
