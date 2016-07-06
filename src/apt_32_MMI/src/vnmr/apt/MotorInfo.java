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

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.util.*;
import javax.swing.Timer;

import vnmr.apt.ChannelInfo.Limits;

import static vnmr.apt.AptDefs.*;


public class MotorInfo implements Serializable {

    /**
     * Holds info about a motor attached to a particular tune knob.
     */

    /** We know (assume) the backlash is always smaller than this. */
    public static final int MAX_BACKLASH = 300;

    /** Number of steps to take as we are approaching overcoming backlash. */
    public static final int BACKLASH_STEP = 3;

    /** The change in frequency to know we have overcome backlash. */
    public static final double CHANGE_IN_FREQ = .07E6;

    /** The change in match to know we have overcome backlash. */
    public static final double CHANGE_IN_MATCH = .01;

    /** We know (assume) the backlash is always larger than this. */
    public static final int MIN_BACKLASH = 20;

    /** How many steps we need to take to get a good calibration baseline. */
    public static final int CAL_STEP = 5;

    /** Time in ms between position updates in interface */
    private static final int POSITION_UPDATE_DELAY = 500;


    /**
     * Steps per revolution of this motor.
     * In general, we only deal in steps, not revolutions.  This is
     * only here so that it gets saved in probe info files.
     */
    private int m_stepsPerRev = 200;

    /**
     * The minimum step that should be used for sensitivity calibration.
     * A smaller number would give too much uncertainty in the actual
     * amount of capacitor motion.
     */
    private int m_minCalStep = 0;

    /** The master control for this process. */
    private ProbeTune m_master;

    /** The number of this motor, starting with 0. */
    private int m_gmi;

    /**
     * Current position of the motor.  This is just a count of the steps
     * taken, and does not take backlash into account.
     */
    private int m_position = 0;

    /** A memory of where we were before moving. */
    private int m_prevPosition;

    /**
     * Current flag value for the motor.  This is downloaded to the motor
     * by the host.  It gets reset (in the motor) on a power cycle.
     */
    private int m_flag = 0;

    /**
     * Motor status word (PZT only).
     */
    private int m_status = 0;

    /**
     * Track the most recent step we've taken.  Needed because of backlash.
     */
    private int m_lastStep = 0;

    /**
     * Keeps track of how we were last commanded to move. Use to check
     * if we are stuck.
     */
    private int m_stepCommanded = 0;

    /**
     * Keeps track of when we were last commanded to move.
     * May be used to check validity of reported motor positions.
     */
    private long m_stepStartTime = 0;

    /**
     * Track how far we've gone in this direction.  Used to see when we
     * have taken up all backlash.
     */
    private int m_distanceThisDirection = 0;

    /** Backlash in this motor */
    //private double m_backlash = MAX_BACKLASH; /*CMP*/
    private double m_backlash = 100;

    /** Backlash in this motor */
    private double m_defaultBacklash = MAX_BACKLASH;

    /** Maximum backlash in this motor */
    private double m_maxBacklash = Double.NaN;

    /** Minimum backlash in this motor */
    private double m_minBacklash = Double.NaN;

    /** Whether this motor has been initialized */
    private boolean m_initialized = false;

    private int m_eotStatus = 0;

    /** How many steps we have tried to take without actually moving */
    private int m_eotCount = 0;

    /** Min legal position for this motor (steps). */
    private int m_minLimit = 0;

    /** Max legal position for this motor (steps). */
    private int m_maxLimit = 20000;

    /**
     * Reported hard stop position for PZT motor (steps).
     * Zero if this is not a PZT (hard stop position not measured).
     */
    private int m_pztMaxLimit = 0;

    /** Odometer reading from this motor */
    private long m_odometerCW = 0;
    private long m_odometerCCW = 0;

    /** Minimum software version required to operate on this motor info file */
    private String m_minSWVersion = "0.0";

    /** Direction of positive rotation, depends on the gear configuration */
    private int m_indexDirection = 0;

    /** Motor module firmware version */
    private String m_motorFWVersion = "Unknown";

    /** The position of the motor at the end of the last step command. */
    private int m_lastStepResult;

    private List<Integer> m_backlashGroup = new ArrayList<Integer>();

    private long m_positionUpdateTime = 0;

    private Timer m_positionDisplayTimer;

    /** Set non-zero to index with free-running instead of hard stop. */
    private int m_indexDistance = 0;


    /**
     * Create a MotorInfo for the specified motor number.
     * The sensitivity of each channel to a given motor is kept in
     * ChannelInfo.
     * @param master The ProbeTune object controlling this process.
     * @param motorNumber Which motor this is.
     * @throws InterruptedException If this command is interrupted.
     */
    public MotorInfo(ProbeTune master, int motorNumber) throws InterruptedException {
        m_master = master;
        m_gmi = motorNumber;

        ActionListener positionTimerAction = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                displayPosition();
            }
        };
        m_positionDisplayTimer = new Timer(POSITION_UPDATE_DELAY,
                                           positionTimerAction);
        m_positionDisplayTimer.setRepeats(false);

        readPersistence(true); // Read persistence from system file
        m_defaultBacklash = getBacklash();
        readPersistence(false);
    }

    /**
     * Minimal constructor for the motor simulator.
     * @param motorNumber The GMI of this motor.
     * @param persistenceDir The directory with the motor files.
     */
    public MotorInfo(int motorNumber, String persistenceDir) {
        m_gmi = motorNumber;
        String path = persistenceDir + "/" + getMotorName();
        readPersistence(path);
    }

    protected void sendToGui(String msg) {
        m_master.sendToGui(msg);
    }

    /**
     * Get the name of the persistence file for this motor.
     * This is something like "motor#0".
     * @return The file name.
     */
    public String getMotorName() {
        return "motor#" +  m_gmi;
    }

    /**
     * Get the path to the persistence file.
     * @param sysFlag If true, return the system path, otherwise the user path.
     * @return The path.
     */
    public String getPersistenceFilePath(boolean sysFlag) {
        if (sysFlag) {
            return m_master.getSysTuneDir() + "/" + getMotorName();
        } else {
            return m_master.getUsrTuneDir() + "/" + getMotorName();
        }
    }

//    public void restorePersistence(int channelNumber){
//        File sysFile;
//        File userFile;
//
//        String sysName= getPersistenceFileNames(true);
//        String userName= getPersistenceFileNames(false);
//        if (ProbeTune.useProbeId()) {
//            sysFile = ProbeTune.getProbeFile(sysName, true);
//            userFile = ProbeTune.getProbeUserFile(userName);
//        } else {
//            sysFile= new File(m_master.getSysTuneDir(), getPersistenceFileName());
//            userFile= new File(ProbeTune.getVnmrUserDir() + "/tune/" + userName);
//        }
//        Messages.postDebug("PredefinedTune",
//                           "System motor file: " + sysFile.getPath());
//        Messages.postDebug("PredefinedTune",
//                           "User motor file: " + userFile.getPath());
//
//        try{
//                TuneUtilities.copy(sysFile, userFile);
//                readPersistence(false);
//        } catch (Exception e){
//                Messages.postError("Cannot restore motor#" + channelNumber);
//        }
//
//    }

    public void writePersistence() {
        StringBuffer sb = new StringBuffer();
        if (!m_minSWVersion.equalsIgnoreCase("0.0")) {
            sb.append("ReqSWVersion " + m_minSWVersion + NL);
        }
        sb.append("Steps/rev " + m_stepsPerRev + NL);
        if (m_indexDirection != 0) {
            sb.append("IndexDirection " + m_indexDirection + NL);
        }
        if (m_indexDistance != 0) {
            sb.append("IndexDistance " + m_indexDistance + NL);
        }
        sb.append("Limit " + m_minLimit + " " + m_maxLimit + NL);
        sb.append("Backlash " + getBacklash() + NL);

        if (!Double.isNaN(m_minBacklash)) {
            sb.append("BacklashLimits " + (int)m_minBacklash);
            if (!Double.isNaN(m_maxBacklash)) {
                sb.append(" " + (int)m_maxBacklash);
            }
            sb.append(NL);
        }

        sb.append("Odom " + m_odometerCW + " "+ m_odometerCCW + NL);
        if (m_minCalStep != 0) {
            sb.append("MinCalStep " + m_minCalStep + NL);
        }

        String path = getPersistenceFilePath(false);
        TuneUtilities.writeFile(path, sb.toString());
    }

    private boolean readPersistence(boolean sysFlag) {
        String path = getPersistenceFilePath(sysFlag);
        boolean ok = readPersistence(path);
        if (!ok && sysFlag) {
            Messages.postError("Cannot read persistence file: " + path);
        }
        return ok;
    }

    public boolean readPersistence(String path) {
        BufferedReader input = TuneUtilities.getReader(path);

        String line = null;
        boolean rtn = true;
        try {
            while ((line = input.readLine()) != null) {
                StringTokenizer toker = new StringTokenizer(line);
                if (toker.hasMoreTokens()) {
                    String key = toker.nextToken().toLowerCase();
                    if (key.equals("backlash")) {
                        double backlash = Double.parseDouble(toker.nextToken());
                        //backlash = Math.max(MIN_BACKLASH, backlash);/*CMP*/
                        //backlash = Math.min(MAX_BACKLASH, backlash);/*CMP*/
                        setBacklash(backlash);
                        m_backlashGroup .clear();
                        while(toker.hasMoreTokens()) {
                            m_backlashGroup.add(new Integer(toker.nextToken()));
                        }
                    } else if (key.equals("backlashlimits")) {
                        m_maxBacklash = Double.parseDouble(toker.nextToken());
                        if (toker.hasMoreTokens()) {
                            m_minBacklash = Double.parseDouble(toker.nextToken());
                        }
                    } else if (key.equals("steps/rev")) {
                        m_stepsPerRev = Integer.parseInt(toker.nextToken());
                    }  else if (key.equals("limit")){
                        m_minLimit = Integer.parseInt(toker.nextToken());
                        if (toker.hasMoreTokens()) {
                            m_maxLimit = Integer.parseInt(toker.nextToken());
                        }

                    } else if (key.equals("odom")){
                        m_odometerCW = Long.parseLong(toker.nextToken());
                        m_odometerCCW = Long.parseLong(toker.nextToken());

                    } else if (key.equals("mincalstep")) {
                        m_minCalStep = Integer.valueOf(toker.nextToken());
                    } else if (key.equals("indexdirection")){
                        int dir = Integer.parseInt(toker.nextToken());
                        m_indexDirection = dir > 0 ? 1 : -1;
                    } else if (key.equals("indexdistance")){
                        m_indexDistance = Integer.parseInt(toker.nextToken());
                    } else if (key.equals("reqswversion")) {
                        // Verify version of software and persistence file
                        m_minSWVersion = toker.nextToken();
                        if (!ProbeTune.isSwVersionAtLeast(m_minSWVersion)) {
                            String msg = "Require software version "
                                + m_minSWVersion + "<br>"
                                + " to operate on the file " + path;
                            Messages.postError(msg);
                            String title = "Incompatible_Motor_File";
                            m_master.sendToGui("popup error title " + title
                                               + " msg " + msg);
                        }
                    }
                }
            }
        } catch (NumberFormatException nfe) {
            Messages.postError("Bad number in persistence file: " + path
                              + ", line: \"" + line + "\"");
            rtn = false;
        } catch (NoSuchElementException nsee) {
            Messages.postError("Bad line in persistence file: " + path
                               + ", line: \"" + line + "\"");
            rtn = false;
        } catch (Exception e) {
            rtn = false;
        } finally {
            try {
                input.close();
            } catch (Exception e) {}
        }
        return rtn;
    }

    /**
     * Remember the distance of the current step command
     * and the time of this call.
     * @param n The step distance currently being requested.
     */
    public void setStepCommanded(int n) {
        m_stepCommanded = n;
        m_stepStartTime = System.currentTimeMillis();
    }

    /**
     * Get the current step command.
     * @return The last step distance requested.
     */
    public int getStepCommanded() {
        return m_stepCommanded;
    }

    /**
     * Get the time of the last step command
     * as remembered by the call to <code>setStepCommanded</code>.
     * @return The time in ms since 1970.
     */
    public long getStepStartTime() {
        return m_stepStartTime;
    }

    /**
     * Update the motor position, based on increment from last position.
     * Also updates the backlash tracking.
     * @param n The number of steps from the previous position.
     */
    public void incrementPosition(int n) {
        Messages.postDebug("BacklashTracking",
                           "incrementPosition(" + n + ") called");
        if (n != 0) {
            m_position += n;
            if (!isSameDirection(n)) {
                m_distanceThisDirection = 0; // Reversed direction
            }
            m_distanceThisDirection += Math.abs(n);
            if (n != 0) {
                m_lastStep = n;
            }
        }
    }

    /**
     * Whether the indicated step continues in the same direction as
     * the last step on this motor. If the indicated step is 0, return
     * true, since we don't need to worry about backlash.
     * If the previous direction is unknown, return false.
     * @param dir The (signed) number of steps we're thinking of taking.
     * @return True if this is the same direction as this motor's last step.
     */
    public boolean isSameDirection(int dir) {
        return (dir == 0 || dir * m_lastStep > 0);
    }

    /**
     * Find if stepping in a given direction will be free of backlash.
     * @param dir The proposed direction.
     * @return True if there is no backlash to be taken up in that direction.
     */
    public boolean isBacklashOk(int dir) {
        return isSameDirection(dir) && m_distanceThisDirection > getMaxlash();
    }

    /**
     * See how far we have gone in which direction since last going
     * the other direction.
     * @return The (signed) distance.
     */
    public int getCurrentDistance() {
        return m_distanceThisDirection * (int)Math.signum(m_lastStep);
    }

    /**
     * Revert to the backup backlash value.
     * @throws InterruptedException If this command is interrupted.
     */
    public void resetBacklash() throws InterruptedException {
        setBacklash(m_defaultBacklash);
    }

    public double getDefaultBacklash() {
        return m_defaultBacklash;
    }

    public Limits getBacklashLimits() {
        double min = (m_defaultBacklash * 3) / 4;
        double max = m_defaultBacklash * 2;
        if (!Double.isNaN(m_minBacklash)) {
            min = m_minBacklash;
            max = m_maxBacklash; // NB: may be NaN (==> no upper limit)
        }
        return new Limits(min, max);
    }

    /**
     * Update the motor position. (Does not move the motor.)
     * @param n The position (in steps).
     * @param force Display new position immediately.
     */
    public void setPosition(int n, boolean force) {
        m_position = n;
        long now = System.currentTimeMillis();
        if (force || now - m_positionUpdateTime > POSITION_UPDATE_DELAY) {
            displayPosition();
            m_positionDisplayTimer.stop();
            m_positionUpdateTime = now;
        } else {
            m_positionDisplayTimer.start();
        }
    }

    /**
     * Display the current motor position.
     */
    protected void displayPosition() {
        long now = System.currentTimeMillis();
        sendToGui("displayMotorPosition " + m_gmi + " " + m_position);
        m_positionDisplayTimer.stop();
        m_positionUpdateTime = now;
    }

    /**
     * Return the current motor position.
     * @return The position (in steps).
     */
    public int getPosition() {
        return m_position;
    }

    /** Set a remembered position - used before taking a step.
     * @param position The position to remember.
     */
    public void setPrevPosition(int position) {
        m_prevPosition = position;
    }

    /** Get the position last set by <code>setPrevPosition</code>.
     * @return The position.
     */
    public int getPrevPosition() {
        return m_prevPosition;
    }

    /**
     * Update the motor flag. (Does not move the motor.)
     * @param n The flag value.
     */
    public void setFlag(int n) {
        m_flag = n;
    }

    /**
     * Return the current motor flag.
     * @return The flag value.
     */
    public int getFlag() {
        return m_flag;
    }

    /**
     * Update the motor status. (Does not move the motor.)
     * @param n The motor status.
     */
    public void setStatus(int n) {
        m_status = n;
    }

    /**
     * Get the current motor status.
     * @return The motor status.
     */
    public int getStatus() {
        return m_status;
    }

//    public Backlash getBacklashInfo() {
//        Backlash backlash = new Backlash();
//        backlash.setValue(getBacklash());
//        backlash.setLimits(getBacklashLimits());
//        backlash.setGroup(m_backlashGroup);
//        return backlash;
//    }

    /**
     * Return the mean backlash value.
     * @return The backlash (in steps).
     */
    public double getBacklash() {
        return m_backlash;
    }

    /**
     * Set the backlash value.
     * @param b The backlash (in steps).
     */
    public void setBacklash(double b) {
        m_backlash = b;
        if (m_master != null) {
            sendToGui("displayMotorBacklash " + m_gmi + " " + (int)b);
        }
    }

    /**
     * Return the maximum uncertainty in the backlash for this motor.
     * @return The uncertainty (in steps).
     */
    public double getGraylash() {
        return getBacklash() / 2;
    }

    /**
     * Return an upper limit to the backlash for this motor.
     * I.e., we are sure that it's not larger than this.
     * @return The maximum backlash (in steps).
     */
    public double getMaxlash() {
        return getBacklash() + getGraylash();
    }

    /**
     * Return the maximum limit for this motor.
     * For PZT, it is the position of the top hard stop minus the
     * "buffer" distance specified by <code>getMinLimit</code>
     * (based on the "motor" file).
     * For Classic ProTune, the maximum limit is read from the motor file.
     * @return The maximum legal position.
     */
    public int getMaxlimit() {
        if (m_pztMaxLimit > 0) {
            return m_pztMaxLimit - m_minLimit;
        } else {
            return m_maxLimit;
        }
    }

    /**
     * Return the minimum position for this motor, as read from
     * the "motor" file.
     * Since the lower hard stop is defined as the "zero" position,
     * this is numerically equal to the distance between the lower hard
     * stop and the minimum position.
     * @return The minimum legal position.
     */
    public int getMinlimit() {
        return m_minLimit;
    }

    public boolean isPztRangeInitialized() {
        return m_pztMaxLimit != 0;
    }

    /**
     * This is the position of the PZT's hard stop, as determined by
     * the PZT itself.
     * @param maxLimit The hard-stop position.
     */
    public void setPztMaxLimit(int maxLimit) {
        m_pztMaxLimit = maxLimit;
    }

    /**
     * For PZT motors, returns the position of the upper hard stop for this
     * motor as determined by the PZT "range" command.
     * @return Range of PZT motor, or 0 if not a PZT.
     */
    public int getPztMaxlimit() {
        return m_pztMaxLimit;
    }

    public void addEotDatum(int nSteps) {
        m_eotCount += nSteps;
        Messages.postDebug("AllTuneAlgorithm",
                           "Motor " + m_gmi + " EOT count=" + m_eotCount);
        if (Math.abs(m_eotCount) > 5) {
            setEot(m_eotCount);
        }
    }

    /**
     * Set flag indicating if this motor is at the end of it's travel.
     * Typically called after making a motion of "step" motor steps.
     * A positive value of "step" indicates are at the positive EOT.
     * A negative value of "step" indicates are at the negative EOT.
     * A zero value for "step" indicates are not at EOT.
     * @param step Flag that we are at EOT for steps in this direction.
     */
    public void setEot(int step) {
        m_eotStatus = step;
        if (step == 0) {
            m_eotCount = 0;
        } else if (DebugOutput.isSetFor("TuneAlgorithm")){
            Messages.postDebug("Setting EOT=" + step + " on motor " + m_gmi);
        }
    }

    /**
     * See if this motor is known to be at one end of it's range of travel.
     * <br>
     * If "direction" is zero, returns true if motor is at either limit
     * of travel.
     * <br>
     * If "direction" is non-zero, returns true only if it is at the
     * limit for stepping in that direction.
     * <br>
     * If "direction" is NaN, returns false.
     * @param direction The proposed direction of motion.
     * @return True if the motor can be moved at all in the requested direction.
     */
    public boolean isAtLimit(double direction) {
        return ((direction == 0 && m_eotStatus != 0)
                || (direction * m_eotStatus > 0));
    }

    /**
     * Returns -1 if direction in motor file is <=0 or unspecified,
     * otherwise 1.
     * @return The direction to index in: +1 for clockwise, -1 for CCW.
     */
    public int getIndexDirection() {
        return m_indexDirection > 0 ? 1 : -1;
    }

    public int getStepsPerRev() {
        return m_stepsPerRev;
    }

    /**
     * Get the minimum step that should be used for sensitivity calibration.
     * A smaller number would give too much uncertainty in the actual
     * amount of capacitor motion.
     * @return The minimum step size.
     */
    public int getMinCalStep() {
        return m_minCalStep < 1 ? CAL_STEP : m_minCalStep;
    }

    public void setInitialized(boolean b) {
        m_initialized = b;
    }

    public boolean isInitialized() {
        return m_initialized;
    }

    public void setOdometerCW(long odomCW){
        m_odometerCW= odomCW;
    }

    public void setOdometerCCW(long odomCCW){
        m_odometerCCW= odomCCW;
    }

    public long getOdometerCW(){
        return m_odometerCW;
    }

    public long getOdometerCCW(){
        return m_odometerCCW;
    }

    /**
     * @return Returns the m_motorFWVersion.
     */
    public String getFirmwareVersion() {
        return m_motorFWVersion;
    }

    /**
     * @param version The m_motorFWVersion to set.
     */
    public void setMotorFWVersion(String version) {
        m_motorFWVersion = version;
    }

    /**
     * Set the motor position returned by the last step command.
     * "Position" should be the value for the position returned by the
     * motor's reply to the last "step" command.
     * @param position Motor position in steps.
     */
    public void setPositionAfterLastStep(int position) {
        m_lastStepResult = position;
    }

    /**
     * Get the motor position returned by the last step command.
     * Should contain the value for the position returned by the
     * motor's reply to the last "step" command, not the position from
     * any "position" queries.
     * @return The position.
     */
    public int getPositionAfterLastStep() {
        return m_lastStepResult;
    }

    public static boolean isProbeSupported(String dir) {
        File[] files = new File(dir).listFiles(MOTOR_FILE_FILTER);
        boolean ok = (files != null && files.length > 0);
        return ok;
    }

    public int getIndexDistance() {
        return m_indexDistance;
    }
}
