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

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


public class MotorControl implements AptDefs {

    public static final int MOTOR_SOCKET_TIMEOUT = 10000; // ms
    public static final int MOTOR_NO_POSITION = -999999; // Invalid position
    public static final int MAX_MOTORS = 10;

    /** PZT status bit: motor calibrated */
    static public final int PZTSTAT_CALIBRATED = 0x01;

    /** PZT status bit: indexed (single sided) */
    static public final int PZTSTAT_SINGLE_INDEXED = 0x02;

    /** PZT status bit: indexed (double-sided) */
    static public final int PZTSTAT_DOUBLE_INDEXED = 0x04;

    private static String m_motorFileUpdating;


    private ProbeTune m_master = null;
    private String m_hostname = "";
    private boolean m_ok = false; // Whether control is operational
    private Socket m_motorSocket = null;
    private InputStream m_inputStream = null;
    private PrintWriter m_motorWriter; // May be null for GUI-only mode
    private MotorInfo[] m_motorList = null;
    private Flag m_waitForStep = new Flag(); // Waiting for step cmd to finish
    private Flag m_waitForPosition = new Flag(); // Wait for a position reply
    private Flag m_waitForFlag = new Flag(); // Waiting for a flag reply
    private Flag m_waitForStatus = new Flag(); // Waiting for a status reply
    private Flag m_waitForRange = new Flag(); // Waiting for a PZT range reply
    private Flag m_waitForModuleId = new Flag(); // Waiting for a PZT ID reply
    private boolean m_checkData = true; // Check data after moving motor
    protected boolean m_gotMotorComm = false; // Set true after we hear from motor

    // Workaround for sticky probe knob / clutch interaction
    private int m_eotList[] = null;

    private boolean m_lastStepError = false;

    // Watchdog counter for detecting lose ethernet cable and motor box power off
    private int m_watchDogCounter = 0;
    private Set<String> m_badMotorMessagesList = null;
    private long m_lastMessageTime = 0;
    protected boolean m_isIndexing = false;
    private Object m_motorCommLock = new Object();
    private int[] m_reindexing = new int[MAX_MOTORS];
    private static String m_firmwareVersion;
    private static String m_moduleId = null;
    protected static Object m_readerLock = new Object();

    protected ExecutorService m_execPool = Executors.newFixedThreadPool(10);
    private TorqueData m_torqueData = null;

    private MotorReadThread m_readThread = null;

    private static final int MAX_WD_COUNT = 3;

    private static final int TIMEOUT = 5000;

    //private static final String[] MOTOR_CODES = {"%", "@"};
    //private static final String PLUS_STEP = "C";
    //private static final String MINUS_STEP = "W";

    private static final int MAX_SETTLE_TIME = 1000; // milliseconds
    private static final int MOTOR_DIGEST_TIME = 150; // milliseconds
    private static final int MOTOR_REPLY_TIME = 1000 + MOTOR_DIGEST_TIME;
    private static final int MAX_STEP_TIME = 1000000;


    public MotorControl(ProbeTune master, String hostname)
        throws InterruptedException {

        m_master = master;
        m_hostname = hostname;
        initializeSocket();

        // Ready lists of motor info
        // Actual motor infos are created when needed
        m_motorList = new MotorInfo[MAX_MOTORS];
        m_eotList = new int[MAX_MOTORS];
    }

    public void quit(int ms) throws InterruptedException {
    	if (m_execPool != null) {
    		m_execPool.shutdownNow();
    	}
    	if (m_readThread != null) {
    		m_readThread.quit(ms);
     	}
    }

    public static void setOptions() {
        // Initialize permission for updating motor files.
        String key = "apt.motorFileUpdating";
        m_motorFileUpdating  = TuneUtilities.getProperty(key, UPDATING_ON);
    }

    protected void initializeSocket() throws InterruptedException {
        m_motorSocket = null;

        boolean noErrors = true;
        InetAddress inetAddr = null;
        try {
            inetAddr = InetAddress.getByName(m_hostname);
            ProbeTune.setMotorAddress(inetAddr.getHostAddress());
        } catch (UnknownHostException uhe) {
            Messages.postDebug("MotorComm",
                               "No IP address found for Motor module \""
                               + m_hostname + "\"" + NL
                               + " ... Using fall-back IP address \""
                               + ProbeTune.getMotorIpAddr() + "\"");
            try {
                inetAddr = InetAddress.getByName(ProbeTune.getMotorIpAddr());
                ProbeTune.setMotorName("");
            } catch (UnknownHostException uhe2) {
                Messages.postError("Bad fall-back motor IP address \""
                                   + ProbeTune.getMotorIpAddr() + "\"");
                noErrors = false;
            }
        }

        if (noErrors) {
            int port = TELNET_PORT;
            if (inetAddr.getHostAddress().equals("127.0.0.1")) {
                Messages.postWarning("Using local (simulated) motor socket");
                port = DEBUG_MOTOR_PORT;
            }
            try {
                Messages.postDebug("MotorComm", "MotorControl: Connect to "
                                   + inetAddr.getHostAddress() + ":" + port
                                   + " (timeout=" + (TIMEOUT / 1000.0) + " s)");
                InetSocketAddress inetSocketAddr
                        = new InetSocketAddress(inetAddr, port);
                m_motorSocket = new Socket();
                m_motorSocket.connect(inetSocketAddr, TIMEOUT);
            } catch (IOException ioe) {
                if (ioe instanceof SocketTimeoutException) {
                    Messages.postError("Timeout connecting to Motor socket \""
                                       + ProbeTune.getMotorIpAddr() + "\"");
                    m_motorSocket = null;
                } else {
                    Messages.postError("MotorControl:"
                                       + "IOException connecting to \""
                                       + ProbeTune.getMotorName() + "\" ("
                                       + ProbeTune.getMotorIpAddr() + ")");
                }
                noErrors = false;
            }
        }

        if (noErrors) {
            Messages.postDebug("MotorComm", "MotorControl: Getting address");
            SocketAddress socketAddr = m_motorSocket.getRemoteSocketAddress();

            if (socketAddr == null) {
                Messages.postError("MotorControl: Null socket address");
                noErrors = false;
            } else {
                Messages.postDebug("MotorComm",
                                   "MotorControl: Socket address = "
                                   + socketAddr);
            }

            /*
             *  Set the timeout for read() function so the MotorReadThread will
             *  not block forever
             */
            try {
                m_motorSocket.setSoTimeout(MOTOR_SOCKET_TIMEOUT);
            } catch (SocketException se) {
                Messages.postError("MotorControl: Can't set socket timeout");
            }

        }

        if (noErrors) {
            try {
                m_motorWriter =
                        new PrintWriter(m_motorSocket.getOutputStream(), true);
                m_inputStream = m_motorSocket.getInputStream();
                BufferedReader motorReader = new BufferedReader
                        (new InputStreamReader(m_inputStream));
                m_readThread = new MotorReadThread(motorReader);
                m_readThread.start();
            } catch (IOException ioe) {
                Messages.postError("MotorControl: IOException: " + ioe);
                noErrors = false;
            }
        }

        if (noErrors) {
            sendToMotor("version"); // Get the firmware version
        }

        m_ok = noErrors;
    }

    public void connectionReset(boolean forceQuit) {
    	if (forceQuit) return; // do nothing if we're being forced to quit

        // Cancel any pending and current commands
        m_master.abort("MotorControl: Lost communication, abort!");

        // Try close the socket to prevent any future problem
        if (m_motorSocket != null) {
            try {
                m_ok = false;
                m_motorSocket.close();
            } catch (IOException ioe) {
            }
        }

        while (!m_ok) {
            try {
                initializeSocket();
            } catch (InterruptedException e) { }
            displayMotorCommunication();
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) { }

            if (!m_gotMotorComm) {
                String msg = "Cannot contact the motor module.<br>"
                    + "Verify the motor IP address.<br>";
                String title = "No_Motor_Communication";
                m_master.sendToGui("popup error title " + title + " msg " + msg);
                m_master.exit("failed - No motor communication");
            }
        }
    }

    protected void sendToGui(String msg) {
        m_master.sendToGui(msg);
    }

    private void displayMotorCommunication() {
        String msg = m_ok ? "motorOk yes" : "motorOk no";
        sendToGui(msg);
    }

    //public void step(int motor, int steps) {
    //    if (motor >= 0 && motor < MOTOR_CODES.length) {
    //        step(MOTOR_CODES[motor], steps);
    //    }
    //}

    /**
     * Check if a given motor number is valid.
     * Assumed valid if it appears in a channel file for the current probe.
     * @param gmi The Global Motor Index to test (from 0)
     * @return True if this is a valid motor number.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean isValidMotor(int gmi) throws InterruptedException {
        return ChannelInfo.getChannelForMotor(m_master, gmi) >= 0;
    }

    /**
     * Get the positions of all the motors from the motor module.
     * Used for initialization of the interface.
     * @throws InterruptedException if this command is aborted.
     */
    public void readAllMotorPositions() throws InterruptedException {
        for (int i = 0; i < MAX_MOTORS; i++) {
            if (isValidMotor(i)) {
                readPosition(i);
            }
        }
    }

    /**
     * Tell the GUI what the motion limits are for all the motors.
     * @throws InterruptedException if this command is aborted.
     */
    public void displayAllMotorLimits() throws InterruptedException {
        for (int i = 0; i < MAX_MOTORS; i++) {
            displayMotorLimits(i);
        }
    }

    /**
     * Tell the GUI what the motion limits are for a given motor.
     * @param gmi The motor to update.
     * @throws InterruptedException if this command is aborted.
     */
    private void displayMotorLimits(int gmi) throws InterruptedException {
        if (isValidMotor(gmi)) {
            MotorInfo mi = getMotorInfo(gmi);
            if (mi != null) {
                int max = mi.getMaxlimit();
                int min = mi.getMinlimit();
                sendToGui("displayMotorLimits "
                          + gmi + " " + min + " " + max);
            }
        }
    }

    /**
     * Do double-sided indexing for the PZT motor module.
     * PZT only permits this when the motor "status" has bit 0 set (0x1)
     * meaning that the driver is calibrated for this motor.
     * The motor is left at its starting location, if possible.
     * @param gmi The motor to index.
     * @return True if indexing was successful.
     * @throws InterruptedException If this command is aborted.
     */
    public boolean doubleIndexMotor(int gmi) throws InterruptedException {
        boolean rtn = false;
        m_isIndexing = true;
        m_master.displayStatus(STATUS_INDEXING);
        int flag = readFlag(gmi);
        if (flag == 0) {
            writeFlag(gmi, 1); // Don't get into endless loop reading position
        }
        int p0 = readPosition(gmi);
        setReindexing(gmi, 0);
        // NB: -400000 is a magic number for the PZT to do double indexing.
        step(gmi, -400000, false);
        m_isIndexing = false;
        if (!getStepError()) {
            // Go to original position
            p0 -= getReindexing(gmi);
            int p1 = readPosition(gmi);
            step(gmi, p0 - p1, false);
            rtn = true;
        }
        m_master.displayStatus(STATUS_PREVIOUS);
        return rtn;
    }

    /**
     * Initialize the absolute motor position.  Runs the motor in
     * one direction until it hits a stop.
     * This becomes position 0.
     * The motor is then returned to its starting position by moving
     * the same number of steps in the other direction.
     * <br>
     * @param gmi The Global Motor Index number of the motor.
     * @return True for successful completion, false if no stop was found.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean indexMotor(int gmi) throws InterruptedException {
        // NB: -200000 is a magic number for the PZT to do indexing.
        int steps = 200000;

        if (!isValidMotor(gmi)) {
            // No channel uses this motor -- and it may not even exist.
            return false;
        }

        MotorInfo mi = getMotorInfo(gmi);
        if (mi == null) {
            return false;
        }
        m_isIndexing  = true;
        m_master.displayStatus(STATUS_INDEXING);
        int flag = readFlag(gmi);
        if (flag == 0) {
            writeFlag(gmi, 1); // Don't get into endless loop reading position
        }
        mi.setInitialized(true);
        // Direction of indexing
        int dir = mi.getIndexDirection();
        if (isPZT()) {
            // Force negative dir for PZT. Does not accept a non-zero "setpos".
            dir = -1;
        }
        boolean freeRunIndexing = mi.getIndexDistance() != 0;
        if (freeRunIndexing) {
            steps = mi.getIndexDistance();
        } else {
            steps = steps * dir;
        }

        // Position to be set at hard stop
        // NB: maxlimit is assumed less than max hard stop by the same
        // amount that minlimit is greater than 0, so the top hard stop
        // position is maxlimit + minlimit. (The min hard stop is
        // defined as the 0 position.)
        int maxlimit = mi.getMaxlimit();
        int minlimit = mi.getMinlimit();
        int setpos = (steps > 0) ? (maxlimit + minlimit) : 0;
        // NB: Fix to allow negative motor limits w/ top hard stop at 0
        if (maxlimit <= 0 && minlimit < 0) {
            setpos = (steps > 0) ? 0 : (maxlimit + minlimit);
        }

        boolean done = false;
        int p0 = readPosition(gmi);
        Messages.postDebug("IndexMotor","setpos=" + setpos + ", p0=" + p0);
        step(gmi, steps, false); // Go until it hits a stop or gives up
        m_isIndexing = false;

        // Wait for it to settle to get a good reading
        // NB: May not be necessary for PZT?
        Thread.sleep(1000);
        int p1 = readPosition(gmi);
        Messages.postDebug("IndexMotor","p1=" + p1);
        int indexChange = setpos - p1;
        String msg = "Index position for motor " + gmi
                + " changed by " + indexChange + " steps";
        Messages.postInfo(msg);
        sendToMotor("setPosition " + gmi + " " + setpos);

        flag = (int)(System.currentTimeMillis() / 1000);
        writeFlag(gmi, flag);

        // Go back to the original position
        int btol = 3;
        int posn = p0 + indexChange;
        if (freeRunIndexing) {
            // Can't get back to original position with free-run indexing
            // so leave it near the end it's at
            posn = steps <= 0 ? mi.getMinlimit() : mi.getMaxlimit();
        }
        posn = Math.min(posn, mi.getMaxlimit());
        posn = Math.max(posn, mi.getMinlimit());
        if (moveMotorToPosition(gmi, posn, btol)) {
            done = true;
        } else {
            // Failed to move to the requested position. The motor
            // is stuck, either at the end or due to a broken capacitor
            int mtrbox = gmi / 2;
            int ldrive = ((gmi % 2) == 1)? 1 : 2;
            String mtr_des;
            if (isPZT()) {
                mtr_des = "motor #" + gmi;
            } else {
                mtr_des = (mtrbox == 0)
                ? "Master module, Drive " + ldrive
                        : "Dual module #" + mtrbox + ", Drive " + ldrive;
            }
            Messages.postError("Motor stuck, cannot index " + mtr_des);
            String title = "Motor_Stuck";
            String dirstr = (steps > 0) ? "positive"
                    : ((steps < 0) ? "negative" : "unknown");
            msg = "Tuning shaft for " + mtr_des + " appears to be stuck<br>"
                    + "while moving in a " + dirstr + " direction.<br>"
                    + "Check the tuning knob according to the manual.";
            sendToGui("popup error title " + title + " msg " + msg);
        }
        m_master.displayStatus(STATUS_PREVIOUS);
        return done;
    }

    public boolean checkMotorPositionMemory(int gmi)
                        throws InterruptedException {
        int flag = 0;

        if (m_isIndexing) {
            flag = 1;
        } else if (!isPZT()) {
            flag = readFlag(gmi);
        } else {
            // isPZT
            int status = readStatus(gmi);
            Messages.postDebug("Motor " + gmi + " status= " + status);
            if ((status & PZTSTAT_CALIBRATED) == 0) {
                readModuleId();
                String msg = "Probe Tuning Controller is not calibrated "
                    + "for motor " + gmi + ", module " + getModuleId();
                m_master.exit(1, msg);
            } else if ((status & PZTSTAT_DOUBLE_INDEXED) == 0
                       && (status & PZTSTAT_SINGLE_INDEXED) == 0)
            {
                // Do double-sided indexing
                doubleIndexMotor(gmi);
                status = readStatus(gmi);
                if ((status & PZTSTAT_DOUBLE_INDEXED) == 0) {
                    // indexing failed
                    String msg = "Double-sided indexing of motor # " + gmi
                            + " failed";
                    m_master.exit(1, msg);
                }
                readRange(gmi);
                displayMotorLimits(gmi);
            }
            flag = status & (PZTSTAT_SINGLE_INDEXED | PZTSTAT_DOUBLE_INDEXED);
        }

        if (flag == 0) {
            if (!indexMotor(gmi)) {
                // indexing failed
                String msg = "Single-sided indexing of motor # " + gmi
                        + " failed";
                m_master.exit(1, msg);
            }
        }

        if (flag > 0){
            MotorInfo minfo = getMotorInfo(gmi);
            if (minfo != null) {
                minfo.setInitialized(true);
            }
        }
        return true;
    }

    /**
     * Move a motor to a specified absolute position.
     * Motor is always moved such that the final run is clockwise,
     * to minimize backlash effects.
     * This is not used for closed-loop tuning, as the tuning is not
     * checked.
     * @param gmi Global Motor Index of motor to move.
     * @param dst The destination motor position.
     * @param tol The maximum acceptable error in the positioning.
     * @return True if motor is now in correct position.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean moveMotorToPosition(int gmi, int dst, int tol)
        throws InterruptedException {

        Messages.postDebug("MoveMotorToPosition",
                           "MotorControl.MoveMotorToPosition("
                           + "gmi=" + gmi + ", dst=" + dst
                           + ", tol=" + tol + ")");
        MotorInfo mi = getMotorInfo(gmi);
        if (mi == null || !isConnected()) {
            return false;
        }

        // Make sure requested position is in range of this motor
        dst = Math.max(mi.getMinlimit(), Math.min(mi.getMaxlimit(), dst));
        checkMotorPositionMemory(gmi);
        int current = readPosition(gmi);
        boolean ok = Math.abs(dst - current) <= tol;
        Messages.postDebug("MoveMotorToPosition",
                           "corrected dest=" + dst
                           + ", Initial position=" + current);
        for (int i = 0; i < 5 && !ok; i++) {
            // Insert a delay between retries.
            if (i > 0) {
                Thread.sleep(1000);
            }
            if (current > dst) {
                // First, take negative step past the desired position.
                int bkDst = dst - MotorInfo.MAX_BACKLASH;
                bkDst = Math.max(bkDst, mi.getMinlimit());
                Messages.postDebug("MoveMotorToPosition", "backupDest=" + bkDst
                                   + ", distance=" + (bkDst - current));
                while (current > bkDst) {
                    int stepsize = bkDst - current;
                    if (!isPZT()) {
                        // NB: Since stepsize < 0, this is min absval:
                        stepsize = Math.max(stepsize, -MAX_STEP_SIZE);
                    }
                    Messages.postDebug("MoveMotorToPosition",
                                       "position=" + current + " ,"
                                       + "stepsize=" + stepsize);
                    int prev = current;
                    current = step(gmi, stepsize, false);
                    if (Math.abs(prev - current) <= tol) {
                        break;
                    }
                }
                Messages.postDebug("MoveMotorToPosition",
                                   "position=" + current);
            }
            Messages.postDebug("MoveMotorToPosition",
                               "dest=" + dst + ", distance=" + (dst - current));
            int motorSlop = 5; // Motor tends to overshoot!
            // Go almost all the way first
            int nearDst = dst - motorSlop;
            while (current < nearDst) {
                int stepsize = nearDst - current;
                if (!isPZT()) {
                    // NB: stepsize > 0
                    stepsize = Math.min(stepsize, MAX_STEP_SIZE);
                }
                Messages.postDebug("MoveMotorToPosition",
                                   "position=" + current
                                   + ", stepsize=" + stepsize);
                int prev = current;
                current = step(gmi, stepsize, false);
                if (Math.abs(prev - current) <= tol) {
                    break;
                }
            }
            //step(gmi, dst - current - motorSlop, false);
            //current = mi.getPosition();
            if (dst - current > 0) {
                int stepsize = dst - current;
                Messages.postDebug("MoveMotorToPosition",
                                   "position=" + current
                                   + ", stepsize=" + stepsize);
                step(gmi, stepsize, false);
                current = mi.getPosition();
            }
            Messages.postDebug("MoveMotorToPosition",
                               "MotorControl.moveMotorToPosition: "
                               + "Final position=" + current);
            ok = Math.abs(dst - current) <= tol;
        }
        return ok;
    }

    /**
     * Get the current motor position by querying the motor.
     * @param gmi The Global Motor Index.
     * @return The current position.
     * @throws InterruptedException if this command is aborted.
     */
    public int readPosition(int gmi) throws InterruptedException {
        int positionVal;

        if (m_motorWriter != null) {
            m_waitForPosition.val = true;
            sendToMotor("position " + gmi); // Check current position
            waitForMotor(m_waitForPosition, MOTOR_REPLY_TIME);
            if (m_waitForPosition.val) {
                Messages.postError("MotorControl.readPosition() timed out");
                m_waitForPosition.val = false;
            }
        }
        MotorInfo mi = getMotorInfo(gmi);
        positionVal= mi.getPosition();
        Messages.postDebug("readPosition", "positionVal=" + positionVal);
        return positionVal;
    }

    public int readRange(int gmi) throws InterruptedException {
        int range;

        if (!isPZT()) {
            return 0;
        }
        if (m_motorWriter != null) {
            m_waitForRange.val = true;
            sendToMotor("range " + gmi); // Check current range
            waitForMotor(m_waitForRange, MOTOR_REPLY_TIME);
            if (m_waitForRange.val) {
                Messages.postError("MotorControl.readRange() timed out");
                m_waitForRange.val = false;
            }
        }
        MotorInfo mi = getMotorInfo(gmi);
        range= mi.getPztMaxlimit();
        return range;
    }

    /**
     * Set the flag value in a motor. Also sets the same value in the
     * MotorInfo for that motor.
     * @param gmi The Global motor index on the motor to set.
     * @param value The value to set.
     */
    private void writeFlag(int gmi, int value) throws InterruptedException {
        sendToMotor("setFlag " + gmi + " " + value);
        MotorInfo mi = getMotorInfo(gmi);
        mi.setFlag(value);
    }

    /**
     * Get the current flag value from a motor.
     * A zero value is normally used to indicate the the motor does
     * not know its current absolute position.
     * @param gmi The Global Motor Index.
     * @return The flag value.
     * @throws InterruptedException if this command is aborted.
     */
    public int readFlag(int gmi) throws InterruptedException {
        int flagVal;
        //synchronized (m_motorCommLock) {
            m_waitForFlag.val = true; // Turned off externally when value received
            sendToMotor("getFlag " + gmi); // Check current position
            waitForMotor(m_waitForFlag, MOTOR_REPLY_TIME);
            if (m_waitForFlag.val) {
                Messages.postDebugWarning("Motor \"getFlag " + gmi + "\" "
                                          + "request timed out - assuming OK");
                MotorInfo mi = getMotorInfo(gmi);
                mi.setFlag(1); // Set a default flag value
                m_waitForFlag.val = false;
            }
        //}
        MotorInfo mi = getMotorInfo(gmi);
        flagVal= mi.getFlag();
        return flagVal;
    }

    /**
     * Wait to return until the specified flag is false.
     * If the flag is not released within the specified timeout time,
     * we return anyway. The caller can check for this condition by
     * checking that the flag is false on return.
     * @param flag The flag to wait for.
     * @param timeout The maximum time to wait, in ms.
     * @throws InterruptedException If this command is aborted.
     */
    private void waitForMotor(Flag flag, int timeout)
    throws InterruptedException {

        synchronized (m_motorCommLock) {
            Messages.postDebug("allMotorComm", "waitForMotor has CommLock");
            long giveup = System.currentTimeMillis() + timeout;
            while (System.currentTimeMillis() < giveup && flag.val) {
                throwExceptionOnCancel();
                m_motorCommLock.wait(giveup - System.currentTimeMillis());
            }
        }
        Messages.postDebug("allMotorComm", "waitForMotor released CommLock");
    }

    /**
     * Get the current status value from a motor.
     * @param gmi The Global Motor Index.
     * @return The status integer.
     * @throws InterruptedException if this command is aborted.
     */
    public int readStatus(int gmi) throws InterruptedException {
        m_waitForStatus.val = true; //Turned off externally when status received
        sendToMotor("status " + gmi);
        waitForMotor(m_waitForStatus, MOTOR_REPLY_TIME);
        if (m_waitForStatus.val) {
            Messages.postError("MotorControl.readStatus() timed out");
            m_waitForStatus.val = false;
        }
        MotorInfo mi = getMotorInfo(gmi);
        return mi.getStatus();
    }

    /**
     * Get the ID of the PZT module.
     * @return The ID string.
     * @throws InterruptedException if this command is aborted.
     */
    public String readModuleId() throws InterruptedException {
        m_waitForStatus.val = true; //Turned off externally when status received
        sendToMotor("module_id");
        waitForMotor(m_waitForModuleId, MOTOR_REPLY_TIME);
        if (m_waitForModuleId.val) {
            Messages.postError("MotorControl.readStatus() timed out");
            m_waitForModuleId.val = false;
        }
        return getModuleId();
    }

    /**
     * Steps the indicated motor by the indicated number of steps
     * and checks the tuning to update the position info.
     * May request negative, zero, or positive number of steps; all
     * requests are sent to the motor, even if the requested step is 0.
     * @param gmi The Global Motor Index number of the motor to move.
     * @param n The number of steps to move.
     * @return The position of the motor after stepping.
     * @throws InterruptedException if this command is aborted.
     */
    public int step(int gmi, int n) throws InterruptedException {
        return step(gmi, n, true);
    }

    /**
     * Steps the indicated motor by the indicated number of steps
     * and optionally checks the tuning to update the position info.
     * May request negative, zero, or positive number of steps; all
     * requests are sent to the motor, even if the requested step is 0.
     * @param gmi The Global Motor Index number of the motor to move.
     * @param n The number of steps to move.
     * @param checkData True to check the tuning after moving, false
     * to return as soon as the motor is in position.
     * @return The position of the motor after stepping.
     * @throws InterruptedException if this command is aborted.
     */
    private int step(int gmi, int n, boolean checkData)
        throws InterruptedException {

        int currentPos = readPosition(gmi);
        int finalPos = currentPos;
        MotorInfo mi = getMotorInfo(gmi);
        if (m_motorWriter != null) {
            mi.setPrevPosition(currentPos);
            finalPos = currentPos + n;
            if ((finalPos > mi.getMaxlimit()) && !m_isIndexing) {
                if (n != 0) {
                    mi.setEot(n);
                }
                n = mi.getMaxlimit() - currentPos;
                Messages.postDebug("MotorControl",
                                   "Step requested is above EOT; changed to:"
                                   + " \"MotorControl.step(" + gmi
                                   + ", " + n + ")\"");
            } else if (finalPos < mi.getMinlimit() && !m_isIndexing) {
                if (n != 0) {
                    mi.setEot(n);
                }
                n = mi.getMinlimit() - currentPos;
                Messages.postDebug("MotorControl",
                                   "Step requested is below EOT; changed to:"
                                   + " \"MotorControl.step(" + gmi
                                   + ", " + n + ")\"");
            }
            // if(n==0){
            //     n=1;
            // }
            //if (!m_ok) {
                // Don't even try to move if position readback failed.
            //    return true;
            //}
            //sendToMotor("position " + gmi); // Check current position

            m_checkData = checkData;
            long deltaT = 0; // Step motion time (ms)
            //synchronized (m_motorCommLock) {
                m_waitForStep.val = true; // Return message from motor will set false
                sendToGui("displayMotorMotion " + gmi + " " + n);
                mi.setStepCommanded(n);
                long t0 = System.currentTimeMillis();
                sendToMotor("step " + gmi + " " + n);
                waitForMotor(m_waitForStep, MAX_STEP_TIME);
                Messages.postDebug("MotorControl", "FINISHED STEPPING");
                deltaT = System.currentTimeMillis() - t0;
                if (m_waitForStep.val) {
                    Messages.postError("MotorControl.step() timed out: motor# "
                                    + gmi);
                    m_waitForStep.val = false;
                }
            //}
            finalPos = mi.getPositionAfterLastStep();
            if (DebugOutput.isSetFor("MotorSpeed") && deltaT != 0) {
                int steps = Math.abs(finalPos - currentPos);
                long rpm = (60000 * steps) / (mi.getStepsPerRev() * deltaT);
                Messages.postDebug("Motor speed = " + rpm + " RPM");
            }
        }
        m_master.displayLastStep(gmi, n);
        return finalPos;
    }

    private void updatePositionInfo(int gmi, int nSteps, long when)
        throws InterruptedException {
        Messages.postDebug("UpdatePositionInfo",
                           "MotorControl.updatePositionInfo("
                           + gmi + ", " + nSteps + ")");

        // Update the current channel info.
        ChannelInfo ci = m_master.getChannel();
        Messages.postDebug("UpdatePositionInfo",
                           "MotorControl.updatePositionInfo: Channel #"
                           + ci.getChannelNumber());
        int cmi = ci.getChannelMotorNumber(gmi);
        if (cmi >= 0) {
            ci.remeasureTune(cmi, nSteps, when);
        }
    }

    public boolean isConnected() {
        return m_ok;
    }

    /**
     * Return the info for the i'th motor.
     * @param gmi The Global Motor Index.
     * @return The MotorInfo object for the motor.
     * @throws InterruptedException if this command is aborted.
     */
    public MotorInfo getMotorInfo(int gmi) throws InterruptedException {
        if (gmi < 0 || gmi >= m_motorList.length) {
            return null;
        } else {
            if (m_motorList[gmi] == null) {
                Messages.postDebug("MotorControl",
                                   "getMotorInfo: Create info for " + gmi);
                m_motorList[gmi] = new MotorInfo(m_master, gmi);
                m_eotList[gmi] = 0;
                sendToMotor("version " + gmi);
            }
            return m_motorList[gmi];
        }
    }

    public int getPosition(int gmi) throws InterruptedException {
        MotorInfo mInfo = getMotorInfo(gmi);
        if (mInfo == null) {
            return MOTOR_NO_POSITION;
        } else {
            return mInfo.getPosition();
        }
    }

    public void writePersistence() {
        if (!UPDATING_OFF.equals(m_motorFileUpdating)) {
            for (int i = 0; i < m_motorList.length; i++) {
                if (m_motorList[i] != null) {
                    m_motorList[i].writePersistence();
                }
            }
        }
    }

    /**
     * Deal with the given message from the motor.
     * Usually called from a MessageProcessor thread spawned to deal only with
     * this message. Thus, the processing of a message is allowed to wait for
     * another message to be processed before returning.
     * Messages starting with "rt" (RealTime) are called from the MotorRead
     * thread and must return expeditiously.
     * @param msg The message to process.
     * @param msgTime When the message was received.
     * @throws InterruptedException If this command is aborted.
     */
    protected void processMotorMessage(String msg, long msgTime)
        throws InterruptedException {

        Messages.postDebug("MotorCommands", "Msg from motor: " + msg);
        if (msg == null) {
            Messages.postDebug("MotorComm|AllMotorComm|MotorCommands",
                               "processMotorMessage(): null msg received");
            return;             // No message
        }
        if (msg.startsWith("#")) {
            // Just a comment - ignore
            return;
        }
        // NB: Need "=" as separator because of non-standard messages
        StringTokenizer toker = new StringTokenizer(msg, " =\t");
        int ntokens = toker.countTokens();
        if (ntokens == 0) {
            Messages.postDebug("MotorComm|AllMotorComm|MotorCommands",
                               "processMotorMessage(): empty msg received");
            return;             // No message
        }

        String cmd = toker.nextToken().toLowerCase();
        ntokens--;
        if (cmd.equals("rtposition")) { // NB: Called from MotorReadThread
            parsePositionMsg(msg, toker, ntokens);
        } else if (cmd.equals("step")) {
            if (ntokens != 3) {
                badMotorMessage(msg);
                return;
            }
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                String status = toker.nextToken();
                int position = Integer.parseInt(toker.nextToken());

                m_master.setReceived(msg);

                MotorInfo motorInfo = m_motorList[gmi];
                int nSteps = position - motorInfo.getPrevPosition();
                int reqSteps = motorInfo.getStepCommanded();

                if (DebugOutput.isSetFor("LogStepTimes")) {
                    String logfile = "/vnmr/tmp/ptuneStepTimes";
                    long t1 = System.currentTimeMillis();
                    long t0 = motorInfo.getStepStartTime();
                    String logmsg = nSteps + "  " + (t1 - t0);
                    TuneUtilities.appendLog(logfile, logmsg, "#nSteps    ms");
                }

                if (nSteps != reqSteps) {
                    Messages.postDebug("MotorControl",
                                       "Motor " + gmi + ": requested "
                                       + reqSteps + " steps, got " + nSteps);
                }

                // NB: Workaround for lack of "eot" messages from motor
                // EOT's fixed: Leave this redundant code in for now
                double req = Math.abs(reqSteps);
                double got = Math.abs(nSteps);
                if (req > 9 && got / req < 0.5) {
                    status = "eot";
                }

                // Workaround: need to stick for a long time before
                // we're really at EOT.
                if (status.equals("eot")) {
                    if (++m_eotList[gmi] < 10 && !isPZT()) {
                        Messages.postDebug("MotorControl",
                                           "EOT count for motor " + gmi
                                           + " = " + m_eotList[gmi]);
                        // Doesn't really count
                        status = "ok";
                    }
                } else {
                    m_eotList[gmi] = 0;
                }

                if (m_checkData) {
                    // Wait for settling time
                    long checkTime = msgTime;
                    if (nSteps != 0) {
                        checkTime += MAX_SETTLE_TIME / (nSteps * nSteps);
                        //NB: More time is added by NetworkAnalyzer
                    }
                    updatePositionInfo(gmi, nSteps, checkTime);
                }
                motorInfo.incrementPosition(nSteps);

                motorInfo.setPosition(position, true);
                motorInfo.setPositionAfterLastStep(position);
                sendToGui("displayMotorMotion " + gmi + " 0");
                //sendToGui("displayMotorPosition " + gmi + " " + position);

                setStepError(status.equals("err"));
                if (status.equals("eot") && reqSteps != 0) {
                    motorInfo.setEot(reqSteps); // Report motor at end
                    Messages.postDebug("MotorControl",
                                       "Set EOT on global motor # " + gmi);
                } else {
                    motorInfo.setEot(0); // Motor NOT at end
                }

                // ODOMETER add nSteps to the odometer reading
                long odometer = 0;
                if (nSteps > 0){
                    odometer = motorInfo.getOdometerCW() + nSteps;
                    motorInfo.setOdometerCW(odometer);
                } else {
                    odometer = motorInfo.getOdometerCCW() + nSteps;
                    motorInfo.setOdometerCCW(odometer);
                }
                //the value gets written to the file after every command
                //see execute() in ProbeTune.java


            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }
            notifyMsgReceived(m_waitForStep);

            if (DebugOutput.isSetFor("LogStepStats")) {
                TuneUtilities.appendLog("/vnmr/tmp/ptuneStepStats", msg);
            }
        } else if (cmd.equals("stepstats")) {
            if (DebugOutput.isSetFor("LogStepStats")) {
                TuneUtilities.appendLog("/vnmr/tmp/ptuneStepStats", msg);
            }
        } else if (cmd.equals("position")) {
            int gmi = parsePositionMsg(msg, toker, ntokens);
            MotorInfo motorInfo = getMotorInfo(gmi);
            if (motorInfo != null) {
                int position = motorInfo.getPosition();
                if (Math.abs(position) < -1) {
                    // Suspicious position; check for power cycle.
                    // This could result in result in the motor being reindexed
                    // and another "position" request being sent.
                    // That's why we call it after our (possibly bogus) position
                    // has been recorded; it may get overwritten by the other
                    // request.  The original caller will not proceed until the
                    // final, correct, position is set, because we don't clear
                    // the wait flag until after this.
                    checkMotorPositionMemory(gmi);
                }
            }

            notifyMsgReceived(m_waitForPosition);

        } else if (cmd.equals("status")) {
            if (ntokens != 2) {
                badMotorMessage(msg);
                return;
            }

            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int status = Integer.parseInt(toker.nextToken());
                getMotorInfo(gmi).setStatus(status);
            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }
            notifyMsgReceived(m_waitForStatus);

        } else if (cmd.equals("range")) {
            if (ntokens != 2) {
                badMotorMessage(msg);
                return;
            }

            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int range = Integer.parseInt(toker.nextToken());
                getMotorInfo(gmi).setPztMaxLimit(range);
            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }
            notifyMsgReceived(m_waitForRange);

        } else if (cmd.equals("flag")) {
            if (ntokens != 2) {
                badMotorMessage(msg);
                return;
            }

            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int flag = Integer.parseInt(toker.nextToken());
                MotorInfo motorInfo = getMotorInfo(gmi);
                motorInfo.setFlag(flag);
                m_master.setReceived("flag " + gmi + " " + flag);
            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }

            notifyMsgReceived(m_waitForFlag);

        } else if (cmd.equals("mlstat")) {
            m_watchDogCounter = 0; //Reset counter even if message is error
            if (ntokens != 1) {
                badMotorMessage(msg);
                return;
            }
            int status;
            try {
                status = Integer.parseInt(toker.nextToken());
                if(status != -1) {
                    Messages.postDebug("MotorStatus",
                                       "Magnet Leg is connected");
                } else {
                    Messages.postDebug("MotorStatus",
                                       "Magnet Leg is not connected");
                }
                m_master.displayMLStatus(status);
            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }
        } else if (cmd.equals("tune")) {
            /*
             * Process "tune" response from motor module, which is resulted from
             * a htune/xtune command.
             */
            if ((ntokens != 2)
                || (!toker.nextToken().equalsIgnoreCase("mode:")))
            {
                badMotorMessage(msg);
                return;
            }
            try {
                int band = Integer.parseInt(toker.nextToken());
                switch (band) {
                case 0:
                    Messages.postDebug("MotorStatus", "High band tune mode");
                    break;
                case 1:
                    Messages.postDebug("MotorStatus", "Low band tune mode");
                    break;
                default:
                    band = -1;
                    Messages.postDebug("MotorStatus",
                                       "Unknown tune mode:" + band);
                    break;
                }
                m_master.displayBandSwitch(band);
            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }
        } else if (cmd.equals("motor")) {
            // This is the way the version is sent by ProTune Motor Modules
            // (Allows different versions for different motors)
            // E.g.: motor 2 firmware version PTMM 2.1 2009.01.01
            m_master.setReceived(msg);
            try {
                int gmi = Integer.valueOf(toker.nextToken());
                toker.nextToken(); // Discard "firmware"
                toker.nextToken(); // Discard "version"
                String version = toker.nextToken("").trim();
                if (m_motorList != null && m_motorList[gmi] != null) {
                    m_motorList[gmi].setMotorFWVersion(version);
                }
                if (gmi == 0) {
                    setFirmwareVersion(version);
                }
                if (isPZT() && getModuleId() == null) {
                    sendToMotor("module_id");
                }
            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }
        } else if (cmd.equals("version")) {
            // This is the way the version is sent by PZT
            // (One firmware instance runs all motors)
            // E.g.: version PZT 2.1 2009.01.01
            try {
                setFirmwareVersion(toker.nextToken("").trim());
                if (isPZT() && getModuleId() == null) {
                    // Get the module ID only if we haven't received it yet
                    sendToMotor("module_id");
                }
            } catch (NoSuchElementException nsee) {
                badMotorMessage(msg);
            }
            m_master.setReceived(msg);

        } else if (cmd.equals("firmware")) {
            // TODO: This is how the version is sent in one prototype PZT
            try {
                toker.nextToken(); // Discard "version"
                setFirmwareVersion(toker.nextToken("").trim());
                if (isPZT() && getModuleId() == null) {
                    sendToMotor("module_id");
                }
            } catch (NoSuchElementException nsee) {
                badMotorMessage(msg);
            }
            m_master.setReceived(msg);

        } else if (cmd.equals("module")) {
            // E.g.: "Module ID: 000000000000"
            try {
                toker.nextToken(); // Discard junk token
                setModuleId(toker.nextToken("").trim());
            } catch (NoSuchElementException nsee) {
                badMotorMessage(msg);
            }
            m_master.setReceived(msg);
            notifyMsgReceived(m_waitForModuleId);

        } else if (cmd.equals("reindex")) {
            // E.g.: reindex 2 3456
            try {
                String strGmi = toker.nextToken();
                String strDelta = toker.nextToken();
                int gmi = Integer.parseInt(strGmi);
                int delta = Integer.parseInt(strDelta);
                setReindexing(gmi, delta);
                shiftTorqueData(strGmi, strDelta);
            } catch (NoSuchElementException nsee) {
                badMotorMessage(msg);
            } catch (NumberFormatException nfe) {
                badMotorMessage(msg);
            }

        } else {
            badMotorMessage(msg, "Unknown msg from motor: ");
        }
    }

    /**
     * Parse a "position" or "rtposition" message from the motor and act on it.
     * @param msg The message (may be used to print an error message).
     * @param toker This tokenizer has already read the first token of "msg".
     * @param ntoks The number of tokens remaining to be read.
     * @return The Global Motor Index that this message was for.
     * @throws InterruptedException If this command is aborted.
     */
    private int parsePositionMsg(String msg, StringTokenizer toker, int ntoks)
            throws InterruptedException {
        if (ntoks < 2) {
            badMotorMessage(msg);
            return -1;
        }
        int gmi = 0;
        int posn = 0;

        m_master.setReceived(msg);

        try {
            String strGmi = toker.nextToken();
            gmi = Integer.parseInt(strGmi);
            String strPos = toker.nextToken();
            posn = Integer.parseInt(strPos);
            MotorInfo motorInfo = getMotorInfo(gmi);
            motorInfo.setPosition(posn, false);
            if (ntoks > 2) {
                addTorqueDatum(strGmi, strPos, toker.nextToken());
            }
        } catch (NumberFormatException nfe) {
            badMotorMessage(msg);
        }
        return gmi;
    }

    private void addTorqueDatum(String strGmi,
                                String strPos, String strTorque) {

        if (m_torqueData != null) {
            sendToGui("addTorqueDatum " + strGmi + " " + strPos + " " + strTorque);
            m_torqueData.addTorqueDatum(strGmi, strPos, strTorque);
        }
    }

    private void shiftTorqueData(String strGmi, String strShift) {

        if (m_torqueData != null) {
            sendToGui("shiftTorqueData " + strGmi + " " + strShift);
            m_torqueData.shiftTorqueData(strGmi, strShift);
        }
    }

    /**
     * Handles the "reindexing" message from the PZT module during
     * double-sided indexing. This occurs when the motor has reached
     * the lower stop and is about to reverse direction.
     * The "new" position of the lower stop is always 0, so our idea
     * of positions changes by -delta.
     * @param gmi The Global Motor Number.
     * @param delta The "old" position of the lower stop.
     */
    private void setReindexing(int gmi, int delta) {
        try {
            m_reindexing[gmi] = delta;
            // Change the direction display:
            sendToGui("displayMotorMotion " + gmi + " 1");
        } catch (IndexOutOfBoundsException e) {
            Messages.postError("Bad motor number: " + gmi);
        }
    }

    private int getReindexing(int gmi) {
        try {
            return m_reindexing[gmi];
        } catch (IndexOutOfBoundsException e) {
            Messages.postError("Bad motor number: " + gmi);
        }
        return 0;
    }

    private void notifyMsgReceived(Flag flag) {
        synchronized (m_motorCommLock) {
            flag.val = false;
            m_motorCommLock.notifyAll();
        }
    }

    private void badMotorMessage(String msg) {
        badMotorMessage(msg, "Bad message from motor: ");
    }

    private String getBadMotorMessagesFilepath() {
        return ProbeTune.getVnmrSystemDir() + "/tmp/BadMotorMessages";
    }

    private void badMotorMessage(String msg, String prefix) {
        if (!DebugOutput.isSetFor("SuppressBadMotorMessages")) {
            Messages.postError(prefix + msg);
        }
        if (DebugOutput.isSetFor("LogBadMotorMessages")) {
            Set<String> badMotorMessages = getBadMotorMessagesList();
            if (!badMotorMessages.contains(msg)) {
                badMotorMessages.add(msg);
                String path = getBadMotorMessagesFilepath();
                TuneUtilities.appendLog(path, msg, null);
            }
        }
    }

    private Set<String> getBadMotorMessagesList() {
        if (m_badMotorMessagesList == null) {
            String path = getBadMotorMessagesFilepath();
            m_badMotorMessagesList = new TreeSet<String>();
            BufferedReader in = TuneUtilities.getReader(path);
            if (in != null) {
                try {
                    String line;
                    while ((line = in.readLine()) != null) {
                        m_badMotorMessagesList.add(line);
                    }
                } catch (IOException ioe) {
                } finally {
                    try {
                        in.close();
                    } catch (Exception e) {}
                }
            }
        }
        return m_badMotorMessagesList;
    }

    private void setStepError(boolean err) {
        m_lastStepError = err;
    }

    private boolean getStepError() {
        return m_lastStepError;
    }

    private void setFirmwareVersion(String version) {
        m_firmwareVersion = version;
    }

    public static String getFirmwareVersion() {
        return m_firmwareVersion;
    }

    private void setModuleId(String id) {
        m_moduleId = id;
    }

    public static String getModuleId() {
        return m_moduleId;
    }

    public int getNumberOfMotors() {
        return m_motorList.length;
    }

    /**
     * See if this is a PZT Module.
     * @return True if this is a PZT.
     */
    public static boolean isPZT() {
        return (m_firmwareVersion != null
                && (m_firmwareVersion.startsWith("PZT")
                    || m_firmwareVersion.startsWith("TP")));
    }

    /**
     * See if this is a later model ProTune Motor Module;
     * i.e., firmware dated 2009 or later.
     * @return True if this is a Motor Module with "recent" firmware.
     */
    public static boolean isPTMM() {
        return (m_firmwareVersion != null
                && m_firmwareVersion.startsWith("PTMM"));
    }

    public static boolean isAutoUpdateOk() {
        return UPDATING_ON.equals(m_motorFileUpdating);
    }

    /**
     * Command the motors to stop immediately.
     * This is a no-op for ProTune Motor Modules with older
     * firmware (pre-2009), because the abort command breaks
     * their command parser.
     * @throws InterruptedException If this command is interrupted
     */
    public void abort() throws InterruptedException {
        if (isPZT() || isPTMM()) {
            Messages.postDebugWarning("Sending 'abort' to motor");
            sendToMotor("abort");
        }
    }

    /**
     * Send a string to the motor module.
     * @param command The string to send.
     * @throws InterruptedException if this command is aborted.
     */
    public void sendToMotor(String command)
        throws InterruptedException {

        if (!command.startsWith("tune")) {
            Messages.postDebug("MotorCommands",
                               "sendToMotor: \"" + command + "\"");
        }
        if (m_motorWriter != null) {
            // Check if this motor needs to be initialized
            StringTokenizer toker = new StringTokenizer(command);
            if (toker.countTokens() >= 2) {
                String key = toker.nextToken();
                if ((key.equals("step") || key.equals("position"))
                    && !m_isIndexing)
                {
                    try {
                        int gmi = Integer.parseInt(toker.nextToken());
                        MotorInfo minfo = getMotorInfo(gmi);
                        if (!minfo.isInitialized()) {
                            if (!checkMotorPositionMemory(gmi)) {
                                // Can't figure out where this motor is
                                // Generate a fake reply w/ illegal position
                                int posn = -minfo.getPosition();
                                String reply = "step " + gmi + " err " + posn;

                                // "receive" the fake reply
                                long now = System.currentTimeMillis();
                                m_execPool.execute(new MessageProcessor(reply,
                                                                        now));
                                //processMotorMessage(reply, now);
                                return; // Don't send the command
                            }
                            if (!key.equals("position")) {
                                readPosition(gmi);
                            }
                        }
                        if (key.equals("step")) {
                            // NB: Workaround for motor firmware bug where
                            // commands to take 0 steps are not acknowledged.
                            // Catch commands to take 0 steps
                            int nsteps = Integer.parseInt(toker.nextToken());
                            if (nsteps == 0) {
                                // Generate a fake reply
                                String reply = "step " + gmi;
                                int posn = minfo.getPosition();
                                MotorInfo motorInfo
                                        = m_master.getMotorInfo(gmi);
                                if (posn == motorInfo.getMaxlimit()) {
                                    reply += " eot ";
                                    motorInfo.setStepCommanded(1);
                                } else if (posn == motorInfo.getMinlimit()) {
                                    reply += " eot ";
                                    motorInfo.setStepCommanded(-1);
                                } else {
                                    reply += " ok ";
                                }
                                reply += posn;
                                long now = System.currentTimeMillis();
                                // "receive" the fake reply
                                m_execPool.execute(new MessageProcessor(reply,
                                                                        now));
                                //processMotorMessage(reply, now);
                                return; // Don't send the command
                            }
                        }
                    } catch (NumberFormatException nfe) {
                        return;
                    }
                }
            } else if (command.equalsIgnoreCase("getmlstat")) {
                // The motor command "getmlstat" is used as a trigger for
                // incrementing watchdog counter
                m_watchDogCounter++;
            }

            //try {
            //    m_inputStream.available();
            //} catch (IOException ioe) {
            //    ProbeTune.printlnErrorMessage(3, "IO exception in sendToMotor: " + ioe);
            //    initializeSocket();
            //}

            synchronized (m_motorWriter) {
                if (command.startsWith("tune ")
                        || command.startsWith("getmlstat"))
                {
                    Messages.postDebug("AllMotorComm",
                                       "  To Motor > \"" + command + "\"");
                } else {
                    Messages.postDebug("MotorComm|AllMotorComm",
                                       "  To Motor > \"" + command + "\"");
                }

                long readyTime = m_lastMessageTime + MOTOR_DIGEST_TIME;
                while (System.currentTimeMillis() < readyTime) {
                    int delay = (int)(readyTime - System.currentTimeMillis());
                    Messages.postDebug("AllMotorComm", "delay=" + delay);
                    Thread.sleep(delay);
                }
                Messages.postDebug("AllMotorComm",
                                   "Last msg sent at: " + m_lastMessageTime);
                m_motorWriter.println(command);
                m_lastMessageTime  = System.currentTimeMillis();
                Messages.postDebug("AllMotorComm",
                                   "    Current time: " + m_lastMessageTime);
            }

            //boolean err = m_motorWriter.checkError();
            //if (err || !m_motorSocket.isConnected()) {
            //  ProbeTune.printlnErrorMessage(3, "Error on Motor connection");
            //}
        }
    }

    public boolean isIndexing() {
        return m_isIndexing;
    }

    /**
     * @throws InterruptedException If the cancel flag has been set.
     */
    private void throwExceptionOnCancel() throws InterruptedException {
        if (ProbeTune.isCancel()) {
            throw new InterruptedException("Aborted");
        }
    }


    private class MotorReadThread extends Thread {
        private boolean m_quit = false;
        private boolean m_forceQuit = false;
        private BufferedReader m_reader;
        private int m_timeoutCounter = 0;

        private static final int MAX_TIMEOUT_COUNT = 6;

        MotorReadThread(BufferedReader r) {
            m_reader = r;
            setName("MotorReadThread");
        }

        public void quit(int ms) {
            m_quit = true;
            m_forceQuit = true;
            interrupt();
            try {
            	join(ms);
            } catch (InterruptedException e) {
            	e.printStackTrace();
            }
        }

        public void run() {
            m_watchDogCounter = 0;
            m_timeoutCounter = 0;
            synchronized (m_readerLock) {
                while (!m_quit) {
                    String msg;
                    try {
                        Messages.postDebug("AllMotorComm", "Waiting for motor msg"
                                           + ", thread=" + this.getName());
                        msg = m_reader.readLine();
                        if (msg == null) {
                            throw new NullPointerException();
                        }
                        if (msg.startsWith("mlstat")) {
                            Messages.postDebug("AllMotorComm",
                                               "From Motor < \"" + msg + "\"");
                        } else {
                            Messages.postDebug("MotorComm|AllMotorComm",
                                               "From Motor < \"" + msg + "\"");
                        }
                        long now = System.currentTimeMillis();
                        if (msg.startsWith("rt")) {
                            // Process "real-time" messages in this thread
                            // NB: This call must not wait for any motor replies
                            processMotorMessage(msg, now);
                        } else {
                            m_execPool.execute(new MessageProcessor(msg, now));
                        }
                        m_gotMotorComm = true;
                        m_timeoutCounter = 0;
                    } catch (NullPointerException npe) {
                        Messages.postDebug("AllMotorComm", "From Motor < NULL");
                        Messages.postDebugWarning("Null message from motor -- "
                                                  + "reconnect...");
                        m_quit = true;
                        connectionReset(m_forceQuit);
                    } catch (SocketTimeoutException ste) {
                        Messages.postDebug("AllMotorComm", "Motor reader timeout");
                        m_timeoutCounter++;
                        if ((m_timeoutCounter > MAX_TIMEOUT_COUNT) &&
                                (m_watchDogCounter > MAX_WD_COUNT)) {
                            Messages.postDebugWarning("Motor watchdog timed out --"
                                                      + " reconnect...");
                            //m_quit = true;
                            connectionReset(m_forceQuit);
                        }
                    } catch (IOException ioe) {
                        Messages.postDebugWarning("Motor reader IO exception: "
                                                  + ioe.getMessage());
                        m_quit = true;
                        connectionReset(m_forceQuit);
                    } catch (Exception e) {
                        Messages.postDebugWarning("Motor reader exception: "
                                                  + e.toString() + "; "
                                                  + e.getMessage());
                        m_quit = true;
                        connectionReset(m_forceQuit);
                    }
                }
            }
        }
    }


    class MessageProcessor implements Runnable {
        String mm_message;
        long mm_msgTime;

        public MessageProcessor(String message, long time) {
            mm_message = message;
            mm_msgTime = time;
        }

        public void run() {
            try {
                processMotorMessage(mm_message, mm_msgTime);
                m_gotMotorComm = true;
            } catch (Exception e) {
                Messages.postError("MotorControl.MessageProcessor.run: "
                                   + "Error processing motor message: " + e);
                Messages.postStackTrace(e);
            }
        }
    }


    class Flag {
        public boolean val;

        public Flag() {
            val = false;
        }
    }


    public void setTorqueDataDir(String torqueDataDir) {
        m_torqueData = new TorqueData(torqueDataDir);
    }

}
