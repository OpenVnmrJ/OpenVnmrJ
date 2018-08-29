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

import java.util.*;
import java.io.*;

import vnmr.apt.MotorInfo;
import vnmr.util.Fmt;
import vnmr.util.NLFit;
import static java.lang.Math.*;
import static vnmr.apt.MotorControl.MOTOR_NO_POSITION;
//import static vnmr.apt.MotorInfo.*;


/**
 * Holds information about a tune channel--both current and historical.
 * Where a probe is "switchable" for different frequencies, there is a
 * ChannelInfo for each choice.
 * <p>
 * The sensitivity to each motor of the tune frequency and match is
 * kept here rather than in MotorInfo.  It depends on both the motor
 * and the channel the motor is affecting.  I.e., we do not assume
 * that a motor affects only its primary channel.
 */
public class ChannelInfo implements Serializable, AptDefs {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    /**
     * How old tune data can be before we think it may no longer be valid.
     * The sample may change, or the tune may be changed manually.
     * In milliseconds.
     */
    private static final int REMEASURE_TIME = 5000; // ms

    private static final int STUCK_MOTOR_RETRIES = 15;

    private static final double TUNE_POSITION_TOL_HZ = 2.0e6;

    private static final double MAX_VALID_REFLECTION = 0.75;

    protected static final double FREQ_RESOLUTION = 25e3; // Hz

    protected static final double GOOD_CAL_S2N = 30;
    protected static final double MIN_CAL_S2N = 5;

    /**
     * Frequencies closer than this will use the same refRangeList.
     */
    protected static final double MAX_REF_RANGE_FREQ_DIFF = 1e6;

    /** Default step size for sensitivity calibration. */
    protected static final int DEFAULT_CAL_DEG = 18;

    /** Where to write the sensitivity log. */
    private static final String SENSITIVITY_LOG
            = ProbeTune.getVnmrSystemDir() + "/tmp/ptuneSensitivity";

    private static String m_channelFileUpdating;

    /** How many SDVs away to consider a point bad data. */
    private static double m_outlierFactor;

    /** The ProbeTune instance used to contol sweeps. */
    private transient ProbeTune m_master;

    /** The number of this channel */
    private int m_channelNumber;

    /** The probe name. */
    private String m_probeName;

    /** The user persistence directory name. */
    private String m_usrProbeName;

    /** The channel name. */
    private String m_channelName;

    /** Min frequency reached by this channel (Hz). */
    private double m_minFreq = 280e6;

    /** Max frequency reached by this channel (Hz). */
    private double m_maxFreq = 320e6;

    /** Min design frequency for this channel (Hz). */
    private double m_minChanFreq = m_minFreq;

    /** Max design frequency for this channel (Hz). */
    private double m_maxChanFreq = m_maxFreq;

    /**
     * The group delay of the probe relative to the probe input port (ns).
     * Used to correct the "phase wrapping" of the reflection data.
     */
    private double m_probeDelay_ns;

    /** List of motors used as switches on this channel */
    private List<SwitchPosition> m_switchList = new ArrayList<SwitchPosition>();

    /** List of manual motors on this channel */
    private List<ManualMotor> m_manMotorList = new ArrayList<ManualMotor>();

    /** Distance to the optimum tune position in steps. */
    private double m_tuneStepDistance = Double.NaN;

    /**
     * List of motors slaved to other motors on this channel.
     * A motor may have 0 or 1 slaves. A non-negative entry for the
     * i'th element in the slaveList is the motor number for the slave
     * of the i'th motor. The slave will be kept in (roughly) the same
     * postition as the master.
     */
    private List<Integer> m_slaveList = new ArrayList<Integer>();

    /**
     * List of backlash values for motors on this channel.
     * The i'th entry in the list is the backlash for the i'th
     * channel motor.
     * For master/slave groups, this is the total backlash, the
     * sum of the individual backlashes for each motor.
     * (The individual backlashes are not measured.)
     */
    private List<Integer> m_backlashList = new ArrayList<Integer>();

    /**
     * List of motors that affect tuning on this channel.
     */
    private int[] m_motorList = null;

    /**
     * Index of the match motor in the list.
     */
    private int m_matchMotor = 1; // Default value

    /**
     * Last data collected for this channel.
     */
    private ReflectionData m_data = null;

    /**
     * Last measured tune position.
     */
    private TuneInfo m_measuredTune = null;

    /**
     * Estimated tune position.
     */
    private TuneInfo m_estimatedTune = null;

    /** Tune sensitivity: Hz / step, df/ds, for each motor. */
    private double[] m_hzPerStep = null;

    /**
     * Match sensitivity: dr/ds at match, for each motor.  This is a
     * signed quantity: negative if the origin is inside the loop,
     * positive if it's outside (small loop:+, big loop:-).
     */
    private double[] m_rPerStep = null;

    /** Tune (frequency) sensitivity averaged over good data. */
    private double[] m_fSdv = null;

    /** Match (reflection) sensitivity averaged over good data. */
    private double[] m_rSdv = null;

    /** Number of Frequency data points (good and bad) */
    private int[] m_nFreqSamples = null;

    /** Number of Match data points (good and bad) */
    private int[] m_nReflSamples = null;

    /** Tune (frequency) sensitivity averaged over all recent data. */
    private double[] m_fSensAll = null;

    /** Match (reflection) sensitivity averaged over all recent data. */
    private double[] m_rSensAll = null;

    /** Tune (frequency) sensitivity standard deviation for all recent data. */
    private double[] m_fSdvAll = null;

    /** Match (reflection) standard deviation for all recent data. */
    private double[] m_rSdvAll = null;

    /** Min and Max limits on the frequency sensitivities */
    private Limits[] m_freqSensLimits = null;

    /** Min and Max limits on the reflection sensitivities */
    private Limits[] m_reflSensLimits = null;

    /**
     * Fallback value for motor sensitivity.
     */
    private double[] m_defaultHzPerStep = null;

    /**
     * Fallback value for motor sensitivity.
     */
    private double[] m_defaultRPerStep = null;

    /**
     * Keep counts of how many times each motor doesn't seem to change
     * anything.
     */
    private int[] m_motorStuck = null;

    private boolean m_calibrating = false;

    private boolean[] m_isAtLimit = null;

    /**
     * The tolerance in the match for this channel.
     * This is the maximum squared reflection coefficient that will
     * terminate tuning.
     */
    //private double m_matchTolerance = 1e-4;

    /**
     * List of known tune positions
     */
    private List<TunePosition> m_tuneList = new ArrayList<TunePosition>();

    /**
     * System list of known tune positions
     */
    private List<TunePosition> m_sysTuneList = new ArrayList<TunePosition>();

    /**
     * List of tune position ranges needed to reach certain frequencies
     */
    private List<TuneRange> m_refRangeList = new ArrayList<TuneRange>();

    /**
     * List of predefined tune positions
     */
    private List<TunePosition> m_predefinedTuneList
            = new ArrayList<TunePosition>();

    private boolean m_channelExists = true;

    /** Minimum software version required to operate on this channel info file */
    private String m_minSWVersion = "0.0";

    /**
     * Size of motor steps to take for sensitivity calibration.
     * Array giving step size for each channel motor.
     */
    private int[] m_calStepsize;

    /** The quad phase correction (used by MtuneControl). */
    private QuadPhase m_quadPhase = null;

    /** Path to the calibration file (used by MtuneControl). */
    private String m_calPath = null;

    private boolean m_isUpdateDisplayed = true;

    private int m_rfchan = 0; // 0 ==> automatic

    private boolean m_readCalStepEntry = false;

    private double m_targetFreq = 0;

    /**
     * A "static" map with motor number as the key; the entry is the number
     * of a channel that uses the motor.
     */
    private static Map<Integer, Integer> sm_motorMap = null;


    /**
     * Initialize the info for a channel.
     * The "sweepFlag" is currently always false (5/21/08).
     * @param master The ProbeTune instance used to contol sweeps.
     * @param iChan The number of the channel, from 0.
     * @param probeName The name, or ID, of the probe.  This is used to
     * initialize the channel info from a file.
     * @param usrProbeName The name of the user channel info directory.
     * @param channelName The name, or ID, of the channel.  This is used to
     * initialize the channel info from a file.  It is specific to the
     * channel (and nucleus?) being tuned.
     * @param range A frequency range (Mhz) that overrides the range
     * in the channel file, or null.
     */
    public ChannelInfo(ProbeTune master,
                       int iChan,
                       String probeName,
                       String usrProbeName,
                       String channelName,
                       Limits range) {
        try {
            init(master, iChan, probeName, usrProbeName, channelName, range);
        } catch (InterruptedException e) {
            m_channelExists = false;
        }
    }

    /**
     * Constructor for the Mtune probe simulator.
     * @param chanFilePath The path to the chan#X file for this channel.
     */
    public ChannelInfo(String chanFilePath) {
        boolean isSysDir = true;
        readPersistence(chanFilePath, isSysDir, null);
        File pathfile = new File(chanFilePath);
        m_channelName = pathfile.getName();
        m_channelNumber = Integer.parseInt(m_channelName.substring(5));
        m_probeName = pathfile.getParentFile().getName();
        m_usrProbeName = m_probeName;
    }

    public static void setOptions() {
        // Initialize permission for updating channel files.
        String key = "apt.channelFileUpdating";
        m_channelFileUpdating  = TuneUtilities.getProperty(key, UPDATING_ON);
    }

    /**
     * Initialize this channel from persistence files.
     * If there are no files for this channel, the m_channelExists flag
     * is set false.
     * @param master The (single) instance of ProbeTune.
     * @param iChan The channel index number (i starts from 0).
     * @param probeName The probe name, used for the name of the system
     * channel info directory.
     * @param usrProbeName The name of the user channel info directory.
     * @param channelName The channel name ("chan#i").
     * @param freqRange TODO
     */
    private void init(ProbeTune master,
                      int iChan,
                      String probeName,
                      String usrProbeName,
                      String channelName,
                      Limits freqRange) throws InterruptedException {
        m_master = master;
        m_channelNumber = iChan;
        m_probeName = probeName;
        m_usrProbeName = usrProbeName;
        m_channelName = channelName;
        
        if (!ProbeTune.isQuadPhaseProblematic(master.getConsoleType())) {
            m_quadPhase = new QuadPhase(0, 9);
        }
//        for (int i = 0; i < MotorControl.MAX_MOTORS; i++) {
//            m_slaveList.add(-1);
//        }
        // Default correction for probe length
        setProbeDelay(TuneUtilities.getDoubleProperty("apt.probeDelay", 5));

        int nMotors = 0;
        // First, read the system persistence file to get backup values
        if (!readPersistence(true, freqRange)) {
            m_channelExists = false;
            return;
        } else {
            // Set the backup motor sensitivities
            nMotors = m_motorList.length;
            m_defaultHzPerStep = new double[nMotors];
            m_defaultRPerStep = new double[nMotors];
            for (int i = 0; i < nMotors; i++) {
                m_defaultHzPerStep[i] = m_hzPerStep[i];
                m_defaultRPerStep[i] = m_rPerStep[i];
            }
        }

        maybeSetSensitivityLimits(); // Based only on system values

        // Now, read the user persistence file
        // ... create it if it doesn't exist
        File sysFile = new File(getPersistenceFilePath(true));
        File userFile = new File(getPersistenceFilePath(false));
        if (!userFile.exists()) {
            try {
                userFile.getParentFile().mkdirs();
                TuneUtilities.copy(sysFile, userFile);
            } catch (Exception e) {
                Messages.postError("Cannot write user channel file: \""
                                   + userFile.getPath() + "\"");
            }
        }
        readPersistence(false, freqRange);

        // Initialize sensitivity tracking
        m_outlierFactor = 3;
        m_fSdv = new double[nMotors];
        m_rSdv = new double[nMotors];
        m_fSensAll = new double[nMotors];
        m_rSensAll = new double[nMotors];
        m_fSdvAll = new double[nMotors];
        m_rSdvAll = new double[nMotors];
        m_nFreqSamples = new int[nMotors];
        m_nReflSamples = new int[nMotors];
        for (int i = 0; i < nMotors; i++) {
            m_fSdv[i] = Double.NaN;
            m_rSdv[i] = Double.NaN;
            m_fSensAll[i] = Double.NaN;
            m_rSensAll[i] = Double.NaN;
            m_fSdvAll[i] = Double.NaN;
            m_rSdvAll[i] = Double.NaN;
            m_nFreqSamples[i] = 0;
            m_nReflSamples[i] = 0;
        }


        nMotors = m_motorList.length;
        m_motorStuck = new int[nMotors];
        m_isAtLimit = new boolean[nMotors];
    }

    /**
     * See if this is a valid channel.
     * @return False if there is no channel file for this channel.
     */
    public boolean exists() {
        return m_channelExists;
    }

    /**
     * Get the number of independent motors for tuning / matching this channel.
     * @return The number of motors.
     */
    public int getNumberOfMotors() {
        int len = 0;
        if (m_motorList != null) {
            len = m_motorList.length;
        }
        return len;
    }

    /**
     * Get the number of independent motors on this channel that are not
     * at their limits of travel. (A motor is considered at EOT only if
     * is both at one end of its range, and the last step calculation found
     * that it needed to move beyond that end.
     * @return The number of usefully movable motors.
     */
    public int getNumberOfMovableMotors() {
        int n = m_motorList.length;
        for (int cmi = n - 1; cmi >= 0; cmi--) {
            if (isAtLimit(cmi)) {
                n--;
            }
        }
        return n;
    }

    public void setFullSweep() {
        /*Messages.postDebug("ChannelInfo.setFullSweep: "
                           + "MaxSweepFreq="
                           + Fmt.f(3, getMaxSweepFreq() / 1e6)
                           + ", MinSweepFreq="
                           + Fmt.f(3, getMinSweepFreq() / 1e6));/*DBG*/
        m_master.setSweep(((getMaxSweepFreq() + getMinSweepFreq()) / 2),
                          (getMaxSweepFreq() - getMinSweepFreq()), getRfchan());
    }

    public void setSweepLimits() {
        m_master.setSweepLimits(getMinSweepFreq(), getMaxSweepFreq());
    }

    /**
     * Set up to tune on this channel.
     * @param setSweepFlag If true, set the sweep to the full range
     * of this channel.
     * @throws InterruptedException if this command is aborted.
     */
    public void setChannel(boolean setSweepFlag) throws InterruptedException {
        setSweepLimits();
        if (setSweepFlag) {
            Messages.postDebug("TuneAlgorithm|ChannelInfo",
                               "Setting full sweep on channel "
                               + getChannelNumber());
            setFullSweep();
        }
        setRfRouting();
        setProbeChannel();
    }

    /**
     * The group delay of the probe relative to the probe input port (ns).
     * Used to correct the "phase wrapping" of the reflection data.
     * @param delay_ns The round-trip delay in ns.
     */
    public void setProbeDelay(double delay_ns) {
        m_probeDelay_ns = delay_ns;
        m_master.displayProbeDelay(delay_ns);
    }

    /**
     * The group delay of the probe relative to the probe input port (ns).
     * Used to correct the "phase wrapping" of the reflection data.
     * @return The round-trip delay in ns.
     */
    public double getProbeDelay() {
        return m_probeDelay_ns;
    }

    /**
     * Get the name of the persistence file for this channel.
     * This is something like "chan#0".
     * @return The file name.
     */
    private String getChannelName() {
        return m_channelName;
    }

    /**
     * Get the last two components of the persistence file path.
     * These are the probeName/chan#x.
     * @param sysFlag If true, return the system path, otherwise the user path.
     * @return The last two components of the path.
     */
    public String getPersistenceFileNames(boolean sysFlag) {
        if (sysFlag) {
            return m_probeName + "/" + getChannelName();
        } else {
            return m_usrProbeName + "/" + getChannelName();
        }
    }

    /**
     * Get the full file path to the persistence file for this channel.
     * @param sysFlag If true, get the path to the system persistence file,
     * otherwise get the path to the user persistence file.
     * @return The file path.
     */
    public String getPersistenceFilePath(boolean sysFlag) {
        if (sysFlag) {
            return m_master.getSysTuneDir() + "/" + getChannelName();
        } else {
            return m_master.getUsrTuneDir() + "/" + getChannelName();
        }
    }

//    public void restorePersistence(int channelNumber){
//        File sysFile;
//        File userFile;
//
//        String sysName= getPersistenceFileName(true);
//        String userName= getPersistenceFileName(false);
//
//        if (ProbeTune.useProbeId()) {
//            sysFile = ProbeTune.getProbeFile(sysName, true);
//            userFile = ProbeTune.getProbeUserFile(userName);
//        } else {
//            sysFile= new File(ProbeTune.getVnmrSystemDir() + "/tune/" + sysName);
//            userFile= new File(ProbeTune.getVnmrUserDir() + "/tune/" + userName);
//        }
//        Messages.postDebug("Persistence",
//                           "System persistence file: " + sysFile.getPath());
//        Messages.postDebug("Persistence",
//                           "User persistence file: " + userFile.getPath());
//
//        try{
//            TuneUtilities.copy(sysFile, userFile);
//            readPersistence(false);
//        } catch (Exception e){
//            Messages.postError("Cannot restore chan#" + channelNumber);
//        }
//    }

    static public String freqToString(double freq) {
        String rtn = Fmt.f(3, freq / 1e6) + "e6";
        return rtn;
    }

    public void writePersistence() {
        if (UPDATING_OFF.equalsIgnoreCase(m_channelFileUpdating)) {
            return;
        }
        StringBuffer out = new StringBuffer();
        String nl = "\n";

        // Counts per motor revolution for this system:
        out.append("Steps/rev " + MotorControl.getCountsPerRev()).append(nl);

        //Minimum software version required
        if (!m_minSWVersion.equalsIgnoreCase("0.0"))
            out.append("ReqSWVersion ").append(m_minSWVersion).append(nl);
        // Max frequency range:
        out.append("Range ").append(freqToString(m_minChanFreq))
                .append(" ").append(freqToString(m_maxChanFreq)).append(nl);
        // Current settings:
        //out.append("Center ").append(m_master.getCenter()).append(nl);
        //out.append("Span ").append(m_master.getSpan()).append(nl);
        // Freq we want to tune to:
        double freq = getTargetFreq();
        if (freq > 0) {
            out.append("Target ").append(freqToString(freq));
            out.append(nl);
        }
        // Manual motors:
        for (ManualMotor mtr : m_manMotorList) {
            out.append("ManualMotor ").append(mtr.getGmi());
            out.append(" ").append(mtr.getLabel()).append(nl);
        }
        // The autoX switch motors:
        for (int i = 0; i < m_switchList.size(); i++) {
            SwitchPosition sp = m_switchList.get(i);
            out.append("Switch ").append(sp.toString()).append(nl);
        }
        for (int i = 0; i < m_slaveList.size(); i++) {
            int slave = m_slaveList.get(i);
            if (slave >= 0) {
                out.append("SlaveMotor ").append(slave).append(" ")
                        .append(i).append(nl);
            }
        }
        // Motor sensitivities:
        int nMotors = m_motorList.length;

        for (int iMotor = 0; iMotor < nMotors; iMotor++) {
            double[] sensitivity = getMotorSensitivities(iMotor, null);
            out.append("Motor ").append(getMotorNumber(iMotor)).append(" ")
                    .append(Fmt.e(4, sensitivity[0])).append(" ")
                    .append(Fmt.e(4, sensitivity[1])).append(nl);
        }

        // Backlashes:
        for (int cmi = 0; cmi < m_backlashList.size(); cmi++) {
            Integer backlash = m_backlashList.get(cmi);
            if (backlash != null) {
                int gmi = getMotorNumber(cmi);
                out.append("TotalBacklash ").append(gmi)
                        .append(" ").append(backlash).append(nl);
            }
        }

        // Calibration stepsizes:
        if (m_readCalStepEntry) {
            out.append("CalStep");
            for (int i = 0; i < m_calStepsize.length; i++) {
                out.append(" " + m_calStepsize[i]);
            }
            out.append(nl);
        }

        // Known tune ranges
        int nRanges = m_refRangeList.size();
        for (int i = 0; i < nRanges; i++) {
            TuneRange tr = m_refRangeList.get(i);
            out.append(tr.toString());
            out.append(nl);
        }

        // Known tune positions
        int nPosns = m_tuneList.size();
        for (int i = 0; i < nPosns; i++) {
            TunePosition tp = m_tuneList.get(i);
            out.append("Ref ").append(freqToString(tp.getFreq())).append(" ")
                    .append(Fmt.e(3, tp.getRefl()));
            for (int iMotor = 0; iMotor < nMotors; iMotor++) {
                double[] sensitivity = getMotorSensitivities(iMotor, tp);
                out.append(" ").append(tp.getPosition(iMotor)).append(" ")
                        .append(Fmt.e(4, sensitivity[0])).append(" ")
                        .append(Fmt.e(4, sensitivity[1]));
            }
            out.append(nl);
        }

        // Known predefined tune positions
        int npdtPosns = m_predefinedTuneList.size();
        Messages.postDebug("PredefinedTune",
                           "Predefined tune list size: " + npdtPosns);

        for (int i = 0; i < npdtPosns; i++) {
            TunePosition tp = m_predefinedTuneList.get(i);
            try {
                out.append("Sample ").append(tp.getSampleName() + " ")
                .append(Fmt.e(6, tp.getFreq())).append(" ")
                .append(Fmt.e(3, tp.getRefl()));
                for (int iMotor = 0; iMotor < nMotors; iMotor++) {
                    double[] sensitivity = getMotorSensitivities(iMotor, tp);
                    out.append(" ").append(tp.getPosition(iMotor)).append(" ")
                    .append(Fmt.e(4, sensitivity[0])).append(" ")
                    .append(Fmt.e(4, sensitivity[1]));
                }
                out.append(nl);

            } catch(Exception e){
                Messages.postDebugWarning("Predefined tune list exception: "
                                          + e);
                Messages.postDebugWarning("... name: " + tp.getSampleName());
            }
        }

        String path = getPersistenceFilePath(false);
        TuneUtilities.writeFile(path, out.toString());
    }

    /**
     * Get the motor sensitivities for a given reference Tune Position.
     * @param cmi The channel index of the motor.
     * @param tp The reference Tune Position.
     * @return Sensitivities of tune (Hz/step) and match (reflection/step).
     */
    private double[] getMotorSensitivities(int cmi, TunePosition tp) {
        double dfdp = Double.NaN;
        double drdp = Double.NaN;
        if (tp != null && cmi >= 0 && cmi < tp.getNumberOfMotors()) {
            dfdp = m_freqSensLimits[cmi].clip(tp.getDfDp(cmi));
            drdp = m_reflSensLimits[cmi].clip(tp.getDrDp(cmi));
        }
        if (Double.isNaN(dfdp) || Double.isNaN(drdp)) {
            dfdp = m_hzPerStep[cmi];
            drdp = m_rPerStep[cmi];
            if (Double.isNaN(dfdp) || Double.isNaN(drdp)) {
                dfdp = m_defaultHzPerStep[cmi];
                drdp = m_defaultRPerStep[cmi];
            }
        }
        double[] rtn = {dfdp, drdp};
        return rtn;
    }

    public int getMasterOf(int slave) {
        int master = -1;
        for (int i = 0; i < m_slaveList.size(); i++) {
            int s = m_slaveList.get(i);
            if (s == slave) {
                master = i;
                break;
            }
        }
        return master;
    }

    private boolean readPersistence(boolean sysFlag, Limits range)
        throws InterruptedException {

        String path = getPersistenceFilePath(sysFlag);
        boolean ok = readPersistence(path, sysFlag, range);
        if (!ok) {
            if (!sysFlag) {
                Messages.postWarning("Initializing channel " + m_channelNumber
                                     + " from system file");
                return readPersistence(sysFlag = true, range);
            }
        }
        return ok;
    }

    private boolean readPersistence(String path, boolean sysFlag, Limits range) {
        int fileCountsPerRev = TuneUtilities.getCountsPerRev(path);
        double myCountsPerRev = MotorControl.getCountsPerRev();
        double stepFactor = myCountsPerRev / fileCountsPerRev;

        BufferedReader input = TuneUtilities.getReader(path);
        if (input == null) {
            return false;
        }

        List<TunePosition> tuneList = sysFlag ? m_sysTuneList : m_tuneList;
        List<String> motorInfoList = new ArrayList<String>(2);
        m_backlashList = new ArrayList<Integer>(2); // Initialize backlash list
        m_backlashList.add(null); // ...with values for both motors = null
        m_backlashList.add(null);
        tuneList.clear();
        m_refRangeList.clear();
        m_switchList.clear();
        for (int i = 0; i < MotorControl.MAX_MOTORS; i++) {
            if (m_slaveList.size() <= i) {
                m_slaveList.add(-1);
            } else {
                m_slaveList.set(i, -1);
            }
        }
        String line = null;
        boolean ok = true;
        try {
            while ((line = input.readLine()) != null) {
                StringTokenizer toker = new StringTokenizer(line);
                String lowerLine = line.toLowerCase();
                if (lowerLine.startsWith("motor ")) {
                    motorInfoList.add(line);
                } else if (lowerLine.startsWith("totalbacklash ")) {
                    toker.nextToken(); // Discard key
                    int gmi = Integer.parseInt(toker.nextToken());
                    int backlash = Integer.parseInt(toker.nextToken());
                    backlash *= stepFactor;
                    int cmi = getChannelMotorNumber(gmi);
                    if (cmi >= 0 && cmi < getNumberOfMotors()) {
                        setBacklash(cmi, backlash);
                    }
                } else if (lowerLine.startsWith("target ")) {
                    if (toker.countTokens() != 2) {
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key
                    setTargetFreq(Double.parseDouble(toker.nextToken()));
                } else if (lowerLine.startsWith("range ")) {
                    if (toker.countTokens() != 3) {
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key
                    // NB: minFreq is real value; minChanFreq saved in chan file
                    m_minChanFreq = Double.parseDouble(toker.nextToken());
                    m_minFreq = m_minChanFreq;
                    m_maxChanFreq = Double.parseDouble(toker.nextToken());
                    m_maxFreq = m_maxChanFreq;
                    if (range != null && range.isSet()) {
                        m_minFreq = range.getMin();
                        m_maxFreq = range.getMax();
                    }
                    String cutoff = System.getProperty("apt.upperCutoff");
                    if (cutoff != null) {
                        double upperCutoff = Double.parseDouble(cutoff);
                        m_maxFreq = min(m_maxFreq, upperCutoff);
                    }
                    Messages.postDebug("Persistence",
                                       "m_minFreq=" + Fmt.f(3, m_minFreq / 1e6)
                                       + ", m_maxFreq="
                                       + Fmt.f(3, m_maxFreq / 1e6));
                } else if (lowerLine.startsWith("refrange ")) {
                    // Get RefRange line with format:
                    //   refrange freq_hz gmi minPosition maxPosition
                    int ntoks = toker.countTokens();
                    if (ntoks < 3) {
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key
                    double freq = Double.parseDouble(toker.nextToken());
                    int gmi = Integer.parseInt(toker.nextToken());
                    int minPosition = MOTOR_NO_POSITION;
                    int maxPosition = MOTOR_NO_POSITION;
                    if (ntoks >= 5) {
                        minPosition = Integer.parseInt(toker.nextToken());
                        minPosition *= stepFactor;
                        maxPosition = Integer.parseInt(toker.nextToken());
                        maxPosition *= stepFactor;
                    }

                    //ProbeTune.printlnDebugMessage(DEBUG_PERSISTENCE,
                    Messages.postDebug("Persistence",
                                       "RefRange: " + minPosition
                                       + " " + maxPosition);

                    m_refRangeList.add(new TuneRange(freq, gmi,
                                                     minPosition, maxPosition));
                } else if (lowerLine.startsWith("ref ")) {
                    int ntoks = toker.countTokens();
                    if (ntoks <= 3 || ntoks % 3 != 0) {
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key
                    int nMotors = (ntoks - 3) / 3;
                    if (nMotors < 1 || nMotors > 2) {
                        Messages.postError("Wrong # of motors in 'Ref' line: "
                                           + line);
                        break;
                    }
                    double freq = Double.parseDouble(toker.nextToken());
                    double refl = Double.parseDouble(toker.nextToken());
                    int[] posn = new int[2];
                    double[] dfdp = new double[2];
                    double[] drdp = new double[2];
                    for (int i = 0; i < nMotors; i++) {
                        posn[i] = Integer.parseInt(toker.nextToken());
                        posn[i] *= stepFactor;
                        dfdp[i] = Double.parseDouble(toker.nextToken());
                        dfdp[i] /= stepFactor;
                        drdp[i] = Double.parseDouble(toker.nextToken());
                        drdp[i] /= stepFactor;
                    }
                    tuneList.add(new TunePosition
                                 (freq, refl,
                                  posn[0], dfdp[0], drdp[0],
                                  posn[1], dfdp[1], drdp[1]));
                } else if (lowerLine.startsWith("switch ")) {
                    if (toker.countTokens() < 2) {
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key
                    int motor = Integer.parseInt(toker.nextToken());
                    int n = toker.countTokens();
                    double[] coeffs = new double[n];
                    for (int i = 0; i < n; i++) {
                        coeffs[i] = Double.parseDouble(toker.nextToken());
                        coeffs[i] *= stepFactor;
                    }
                    m_switchList.add(new SwitchPosition(motor, coeffs));
                } else if (lowerLine.startsWith("manualmotor ")) {
                    if (toker.countTokens() < 2) {
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key
                    int motor = Integer.parseInt(toker.nextToken());
                    String label = "";
                    if (toker.hasMoreTokens()) {
                        label = toker.nextToken("").trim();
                    }
                    ManualMotor manMtr = new ManualMotor(motor, label);
                    if (!m_manMotorList.contains(manMtr)) {
                        m_manMotorList.add(manMtr);
                    }
                } else if (lowerLine.startsWith("slavemotor ")) {
                    if (toker.countTokens() != 3) {
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key
                    int slaveMotor = Integer.parseInt(toker.nextToken());
                    int masterMotor = Integer.parseInt(toker.nextToken());
                    m_slaveList.set(masterMotor, slaveMotor);
                } else if (lowerLine.startsWith("calstep ")) {
                    m_readCalStepEntry = true;
                    toker.nextToken(); // Discard key
                    int n = toker.countTokens();
                    m_calStepsize = new int[n];
                    for (int i = 0; i < n; i++) {
                        m_calStepsize[i] = Integer.valueOf(toker.nextToken());
                        m_calStepsize[i] *= stepFactor;
                        //m_calStepsize[i] = clipCalStep(m_calStepsize[i], i);
                    }
                } else if (lowerLine.startsWith("sample ")) {
                    toker.nextToken(); // Discard key

                    String sampleName = new String(toker.nextToken());

                    double freq = Double.parseDouble(toker.nextToken());
                    double refl = Double.parseDouble(toker.nextToken());

                    int ntoks = toker.countTokens();

                    if (ntoks <= 3 || ntoks % 3 != 0) {
                        ok = false;
                        break;
                    }

                    int nMotors = (ntoks) / 3;
                    if (nMotors < 1 || nMotors > 2) {
                        Messages.postError("Wrong # of motors in 'Ref' line: "
                                           + line);
                        break;
                    }

                    int[] posn = new int[2];
                    double[] dfdp = new double[2];
                    double[] drdp = new double[2];
                    for (int i = 0; i < nMotors; i++) {
                        posn[i] = Integer.parseInt(toker.nextToken());
                        posn[i] *= stepFactor;
                        dfdp[i] = Double.parseDouble(toker.nextToken());
                        dfdp[i] /= stepFactor;
                        drdp[i] = Double.parseDouble(toker.nextToken());
                        drdp[i] /= stepFactor;
                    }
                    Messages.postDebug("PredefinedTune",
                                       "Setting predefined tune position: "
                                       + sampleName);
                    //save predefined tune
                    TunePosition newTP = new TunePosition
                    (sampleName, freq, refl,
                            posn[0], dfdp[0], drdp[0],
                            posn[1], dfdp[1], drdp[1]);

                    savePredefinedTunePosition(newTP);


                } else if (lowerLine.startsWith("reqswversion ")) {
                    if (toker.countTokens() != 2) {
                        Messages.postError("Bad line in persistence file: "
                                           + path + ", line: \"" + line + "\"");
                        ok = false;
                        break;
                    }
                    toker.nextToken(); // Discard key

                    // Verify version of software and persistence file
                    m_minSWVersion = toker.nextToken();
                    if (!ProbeTune.isSwVersionAtLeast(m_minSWVersion)) {
                        String msg = "Require software version "
                                + m_minSWVersion + "<br>"
                                + " to operate on the file " + path;
                        Messages.postError(msg);
                        String title = "Incompatible_Channel_File";
                        m_master.sendToGui("popup error title " + title
                                           + " msg " + msg);
                    }
                } else if (lowerLine.startsWith("phase ")) {
                    toker.nextToken();
                    int phase = Integer.parseInt(toker.nextToken());
                    QuadPhase qPhase = new QuadPhase(phase, 10);
                    setQuadPhase(qPhase);
                }
            }

            // Initialize member values
            int nMotors = motorInfoList.size();
//            if (nMotors == 0) {
//                // TODO: Is there any point in this?
//                motorInfoList.add("Motor 0 NaN NaN");
//                motorInfoList.add("Motor 1 NaN NaN");
//                nMotors = motorInfoList.size();
//            }
            try {
                m_motorList = new int[nMotors];
                m_hzPerStep = new double[nMotors];
                m_rPerStep = new double[nMotors];
                if (m_freqSensLimits == null || m_reflSensLimits == null) {
                    m_freqSensLimits = new Limits[nMotors];
                    m_reflSensLimits = new Limits[nMotors];
                    for (int i = 0; i < nMotors; i++) {
                        m_freqSensLimits[i] = new Limits(Double.NaN, Double.NaN);
                        m_reflSensLimits[i] = new Limits(Double.NaN, Double.NaN);
                    }
                }
                for (int i = 0; i < nMotors; i++) {
                    line = motorInfoList.get(i);
                    StringTokenizer toker = new StringTokenizer(line);
                    toker.nextToken(); // "motor" label
                    m_motorList[i] = Integer.parseInt(toker.nextToken());
                    m_hzPerStep[i] = Double.parseDouble(toker.nextToken());
                    m_hzPerStep[i] /= stepFactor;
                    m_rPerStep[i] = Double.parseDouble(toker.nextToken());
                    m_rPerStep[i] /= stepFactor;
                    if (toker.countTokens() >= 4) {
                        double min = Double.parseDouble(toker.nextToken());
                        min /= stepFactor;
                        double max = Double.parseDouble(toker.nextToken());
                        max /= stepFactor;
                        m_freqSensLimits[i] = new Limits(min, max);
                        min = Double.parseDouble(toker.nextToken());
                        min /= stepFactor;
                        max = Double.parseDouble(toker.nextToken());
                        max /= stepFactor;
                        m_reflSensLimits[i] = new Limits(min, max);
                    }
                    m_hzPerStep[i] = m_freqSensLimits[i].clip(m_hzPerStep[i]);
                    m_rPerStep[i] = m_reflSensLimits[i].clip(m_rPerStep[i]);
                }
            } catch (Exception e) {
                Messages.postError("Error reading channel persistence file: "
                        + e);
                Messages.postStackTrace(e);
            }
            int nStepsizes = m_calStepsize == null ? 0 : m_calStepsize.length;
            if (nStepsizes < nMotors) {
                int[] stepsize = new int[nMotors];
                for (int i = 0; i < nStepsizes; i++) {
                    stepsize[i] = m_calStepsize[i];
                }
                for (int i = nStepsizes; i < nMotors; i++) {
                    stepsize[i] = (int)round(DEFAULT_CAL_DEG * myCountsPerRev
                                             / 360);
                    //stepsize[i] = clipCalStep(stepsize[i], i);
                }
                m_calStepsize = stepsize;
            }
            m_matchMotor = getMatchMotorChannelIndex();

        } catch (NumberFormatException nfe) {
            Messages.postError("Bad number in persistence file line"
                               + " for probe \""
                               + m_probeName
                               + "\", channel "
                               + m_channelNumber
                               + ", line: \""
                               + line
                               + "\"");
            ok = false;
        } catch (NoSuchElementException nsee) {
            Messages.postError("Not enough fields in persistence file line"
                               + " for probe \"" + m_probeName
                               + "\", channel " + m_channelNumber
                               + ", line: \"" + line + "\"");
            ok = false;
        } catch (IOException ioe) {
            Messages.postError("Cannot read persistence file for probe: "
                               + m_probeName + ", channel: " + getChannelName());
            ok = false;
        } finally {
            try {
                input.close();
            } catch (Exception e) {}
        }
        return ok;
    }

    /**
     * Make sure a proposed calibration step is large enough.
     * @param calStep The calibration step size to check.
     * @param cmi The Channel Motor Index this is used for.
     * @return The corrected calibration step
     */
    private int clipCalStep(int calStep, int cmi) throws InterruptedException {
        int minStep = getMinCalStep(cmi);
        if (calStep == 0) {
            Messages.postError("clipCalStep: calibration step was 0");
            calStep = minStep;
        } else if (abs(calStep) < minStep) {
            calStep = minStep * (int)signum(calStep);
        }
        return calStep;
    }

    /**
     * If sensitivity limits have not been set in the channel file,
     * set what limits we can figure out.
     */
    public void maybeSetSensitivityLimits() {
        int cmi = getTuneMotorChannelIndex();
        if (cmi >= 0 && ! m_freqSensLimits[cmi].isSet()) {
            // No sign change on tune sensitivity
            m_freqSensLimits[cmi] = new Limits(m_hzPerStep[cmi] / 4,
                                               m_hzPerStep[cmi] * 4);
            m_reflSensLimits[cmi] = new Limits(-m_rPerStep[cmi] * 4,
                                               m_rPerStep[cmi] * 4);
        }
        cmi = getMatchMotorChannelIndex();
        if (cmi >= 0 && ! m_reflSensLimits[cmi].isSet()) {
            m_freqSensLimits[cmi] = new Limits(-m_hzPerStep[cmi] * 4,
                                               m_hzPerStep[cmi] * 4);
            // No sign change on match sensitivity
            m_reflSensLimits[cmi] = new Limits(m_rPerStep[cmi] / 4,
                                               m_rPerStep[cmi] * 4);
        }
        Messages.postDebug("Persistence",
                           "ChannelInfo.maybeSetSensitivityLimits: "
                           + "Sensitivity limits set");
    }

    public void setMaster(ProbeTune master) {
        m_master = master;
    }

    /**
     * Remember the RF reflection data for this channel.
     * @param data The Reflection Data to set as the current data.
     */
    public void setData(ReflectionData data) {
        //NB: Redundant(?) check so we don't assign wrong data to this chan
        if (data == null
            || data.getStartFreq() > m_maxFreq
            || data.getStopFreq() < m_minFreq)
        {
            m_data = null;
        } else {
            m_data = data;
        }
    }

    /**
     * Check the current match at the target frequency against the target
     * maximum match value.
     * @param target The target frequency and match.
     * @return True if the match criterion is met.
     */
    public boolean isMatchBetterThan(TuneTarget target) {
        if (m_data == null) {
            return false;
        }
        double actualMatch = m_data.getMatchAtFreq(target.getFreq_hz());
        double targetmatch = target.getCriterion().getTarget_pwr();
        return actualMatch <= targetmatch;
    }

    public double getMatchAtRequestedFreq(TuneTarget target) {
        double freq = target.getFreq_hz();
        double match = m_data.getMatchAtFreq(freq);
        return match;
    }

    /**
     * Set the motor sensitivities and backlash to safe values.  In
     * this implementation, these come from the values in the system
     * channel file.  We multiply the raw sensitivity values by 2 to
     * make the calculated steps smaller (more conservative).
     * @throws InterruptedException if this command is aborted.
     */
    public void resetSensitivity() throws InterruptedException {
        int nMotors = min(m_defaultHzPerStep.length,
                               m_defaultRPerStep.length);
        m_hzPerStep = new double[nMotors];
        m_rPerStep = new double[nMotors];
        for (int i = 0; i < nMotors; i++) {
            m_hzPerStep[i] = m_defaultHzPerStep[i];
            m_rPerStep[i] = m_defaultRPerStep[i];
            updateSensitivityDisplay(i);
        }
        m_matchMotor = getMatchMotorChannelIndex();
        MotorInfo mInfo;
        for (int i = 0; i < nMotors && (mInfo = getMotorInfo(i)) != null; i++) {
            mInfo.resetBacklash();
        }
    }

    /**
     * Initialize the info for the current state of this channel. This is the
     * sensitivity of the tune and match to each of the motors.
     * <p>
     * The match (reflection) is a signed quantity, positive if it is outside
     * the tuning loop (loop too small), negative if it inside (loop too big).
     * We take the motor with the greatest match sensitivity relative to
     * its tune sensitivity as the "match" motor.
     *
     * @throws InterruptedException if this command is aborted.
     */
    public void initializeChannel() throws InterruptedException {
        if (!m_master.isMotorOk()) {
            Messages.postError("No communication with tune motors");
            return;
        }
        m_calibrating = true;
        // Get sensitivities of each motor
        int nMotors = m_motorList.length;
        for (int i = 0; i < nMotors; i++) {
            initializeMotorSensitivities(i);
        }

        // MatchMotor is the one with relatively largest match sensitivity.
        m_matchMotor = getMatchMotorChannelIndex();
        writePersistence();
        m_calibrating = false;
    }

    public void setEotValues(double[] steps) throws InterruptedException {
        MotorInfo mi;
        for (int cmi = 0; (mi = getMotorInfo(cmi)) != null; cmi++) {
            if (steps == null || steps.length <= cmi) {
                setAtLimit(cmi, false);
            } else {
                setAtLimit(cmi, mi.isAtLimit(steps[cmi]));
            }
        }
    }

    public boolean isAtLimit(int cmi) {
        return m_isAtLimit[cmi];
    }

    public void setAtLimit(int cmi, boolean atLimit) {
        m_isAtLimit[cmi] = atLimit;
    }

    public boolean isPhasedDataUsed() {
        return m_data != null && m_data.isPhasedDataUsed();
    }

    public boolean isDipDetected() {
        return (m_data != null && m_data.isDipDetected());
    }

    public boolean isSignalDetected() {
        return (m_data != null && m_data.getMaxAbsval() > 0.1);
    }

//    public boolean isDipInChannelRange() {
//        boolean rtn = false;
//        TuneInfo ti = getMeasuredTunePosition();
//        if (ti != null) {
//            double freq = ti.getFrequency();
//            rtn = freq >= getMinTuneFreq() && freq <= getMaxTuneFreq();
//        }
//        return rtn;
//    }

    /**
     * Check if a channel is usable.
     * @return True if this channel was successfully initialized.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean isChannelReady() throws InterruptedException {
        boolean ready = true;
        for (int cmi = 0; ready && getMotorNumber(cmi) >= 0; cmi++) {
            // TODO: Does this make sense?
            ready = (getMotorInfo(cmi) != null);
        }
        return ready;
    }

//    public boolean initializeChannelFromScratch() throws InterruptedException {
//        m_calibrating = true;
//        boolean ok = true;
//        ok &= findDipFromScratch();
//        ok &= determineMatchMotorGivenDip();
//        ok &= getDipBelow(0.05);
//        m_calibrating = false;
//        return ok;
//    }
//
//    private boolean determineMatchMotorGivenDip() throws InterruptedException {
//        boolean ok = true;
//
//        //double[] dipSDV = measureDipPositionSDV();
//        for (int cmi = 0; ok && getMotorNumber(cmi) >= 0; cmi++) {
//            ok = measureMotorSensitivities(cmi, 0.5);
//        }
//        if (ok) {
//            m_matchMotor = getMatchMotorChannelIndex();
//        }
//        return ok;
//    }

    private double[] measureDipPositionSDV() throws InterruptedException {
        final int N_MEASUREMENTS = 10;
        double freq = 0;
        double refl = 0;
        List<Double> freqList = new ArrayList<Double>();
        List<Double> reflList = new ArrayList<Double>();
        for (int i = 0; i < N_MEASUREMENTS; i++) {
            TuneInfo ti = measureTune(-1, true);
            freq += ti.getFrequency();
            refl += ti.getReflection();
            freqList.add(ti.getFrequency());
            reflList.add(ti.getReflection());
        }
        freq /= N_MEASUREMENTS;
        refl /= N_MEASUREMENTS;
        double freqSDV = 0;
        double reflSDV = 0;
        for (int i = 0; i < N_MEASUREMENTS; i++) {
            double df = freqList.get(i) - freq;
            freqSDV += df * df;
            double dr = reflList.get(i) - refl;
            reflSDV += dr * dr;
        }
        freqSDV = sqrt(freqSDV / (N_MEASUREMENTS - 1));
        reflSDV = sqrt(reflSDV / (N_MEASUREMENTS - 1));
        double[] rtn = {freqSDV, reflSDV};
        return rtn;
    }

//    private boolean getDipBelow(double maxReflection) {
//
//        return false;
//    }

//    private boolean findDipFromScratch() throws InterruptedException {
//        TuneInfo ti = measureTune(0, true);
//        if (ti == null || !ti.isValid()) {
//            // FIXME: Search for a dip
//        }
//        return (ti != null && ti.isValid());
//    }


    /**
     * Routine to move past the backlash when going in a particular direction
     * @param cmi Motor that is moving beyond the backlash
     * @return direction Direction to keep moving so won't continue to be in backlash
     * @throws InterruptedException if this command is aborted.
     */
    public int movePastBacklash(int cmi) throws InterruptedException {
        // NB: Used only by measureMotorSensitivities().
        // TODO: Fix reliability and accuracy


        // Step in direction of farther end of sweep.
        // (Prefer same direction as last motion)
        TuneInfo t0;
        double f0;
        double r0;
        double forig;
        double rorig;
        double fDiff;
        double rDiff;


        //step in direction of farther end of sweep
        t0= getMeasuredTunePosition();
        f0 = t0.getFrequency();
        r0= t0.getReflection();
        forig= f0;
        rorig= r0;

        // Where is f0 within the sweep range?
        // Which way is it best to go?
        MotorInfo mInfo = getMotorInfo(cmi);
        int direction = 1;
        int currentDistance = mInfo.getCurrentDistance();
        if (currentDistance != 0) {
            direction = (int)signum(currentDistance); // -1 or +1
        }

        Messages.postDebug("Backlash", "Direction to move: " + direction);

        fDiff = abs(f0-forig);
        rDiff = abs(r0-rorig);
        while ( true ){
            step(cmi, direction * MotorInfo.BACKLASH_STEP);
            t0= getMeasuredTunePosition();
            f0=t0.getFrequency();
            r0=t0.getReflection();
            fDiff= abs(f0-forig);
            rDiff= abs(r0-rorig);
            Messages.postDebug("Backlash", "Checking freq0: " + f0
                               + " forig: " + forig + " fDiff " + fDiff);
            Messages.postDebug("Backlash", "Checking ref0: " + r0 +
                               " rorig: " + rorig + " rDiff " + rDiff);
            if(rDiff>MotorInfo.CHANGE_IN_MATCH){
                Messages.postDebug("Backlash", "Match backlash here");
                direction=1;
                step(cmi, MotorInfo.MAX_BACKLASH);
                break;
            }
            if(fDiff>MotorInfo.CHANGE_IN_FREQ){

                if(direction<0){
                    if( (getMaxTuneFreq()-f0) > (getMaxTuneFreq()-forig) ){
                        //done
                        Messages.postDebug("Backlash", "Freq backlash here");
                        break;
                    }
                } else {
                    if( (f0-getMinTuneFreq()) > (forig-getMaxTuneFreq()) ){
                        //done
                        Messages.postDebug("Backlash", "Freq backlash here");
                        break;
                    }
                }
            }

        }
        return direction;

    }

    /**
     * Initialize info about tuning sensitivity and backlash of a motor.
     * @param cmi The index of the motor on this channel, from 0.
     */
    private boolean measureSensitivities(int cmi) throws InterruptedException {

        final double MEASURE_SDV = 0.1;
        boolean ok = true;
        MotorInfo mInfo = getMotorInfo(cmi);
        if (mInfo == null) {
            return false;
        }

        // p0 = initial motor position
        int p0 = getMotorGroupPosition(cmi);

        getFreshTunePosition(true);
        TuneInfo t1 = getMeasuredTunePosition();
        if (t1 == null) {
            return false;
        }
        double[] dipSDV = measureDipPositionSDV();

        // Choose a good direction to start with
        int stepDir = getSafeTuneDirection(cmi, t1.getFrequency());
        m_master.setFullSweep();

        // Measure sensitivities going in one direction
        SensitivityData sens1 = measureMotorSensitivities(cmi,
                                                          dipSDV,
                                                          MEASURE_SDV,
                                                          stepDir,
                                                          null);

        // Set these to help calculate when we're back to initial position
        m_hzPerStep[cmi] = sens1.getFreqSensitivity();
        m_rPerStep[cmi] = sens1.getReflSensitivity();

        // Measure sensitivities going back to original position
        SensitivityData sens2 = measureMotorSensitivities(cmi,
                                                          dipSDV,
                                                          MEASURE_SDV,
                                                          -stepDir,
                                                          t1);
        if (DebugOutput.isSetFor("Sensitivity")) {
            Messages.postDebug("Freq Sens 1="
                               + Fmt.g(3, sens1.getFreqSensitivity())
                               + ", Freq Sens 2="
                               + Fmt.g(3, sens2.getFreqSensitivity())
                               );
            Messages.postDebug("Refl Sens 1="
                               + Fmt.g(3, sens1.getReflSensitivity())
                               + ", Refl Sens 2="
                               + Fmt.g(3, sens2.getReflSensitivity())
                               );
        }

        // Update the sensitivities - averaging measurements
//        sens1.setDeletionAllowed(false);
        sens1.add(sens2);
        if (DebugOutput.isSetFor("Sensitivity")) {
            sens1.dumpSensitivityData(cmi);
        }
        m_hzPerStep[cmi] = sens1.getFreqSensitivity();
        if (!m_freqSensLimits[cmi].contains(m_hzPerStep[cmi])) {
            String type = "frequency";
            String knob = cmi == getMatchMotorChannelIndex() ? "match" : "tune";
            postSensitivityWarning(cmi, type, knob);
        }
        m_rPerStep[cmi] = sens1.getReflSensitivity();
        if (!m_reflSensLimits[cmi].contains(m_rPerStep[cmi])) {
            String type = "reflection";
            String knob = cmi == getMatchMotorChannelIndex() ? "match" : "tune";
            postSensitivityWarning(cmi, type, knob);
        }
        updateSensitivityDisplay(cmi);

        // Get the mean frequency and reflection in measurements
        double f = sens1.getMeanFreq();
        double r = sens1.getMeanRefl();
        // Calculated positions (and SDVs) at those values:
        double[] f1posn = sens1.calcPlusPositionAtFreq(f, m_hzPerStep[cmi]);
        double[] f2posn = sens1.calcMinusPositionAtFreq(f, m_hzPerStep[cmi]);
        double[] r1posn = sens1.calcPlusPositionAtRefl(r, m_rPerStep[cmi]);
        double[] r2posn = sens1.calcMinusPositionAtRefl(r, m_rPerStep[cmi]);
        if (DebugOutput.isSetFor("Sensitivity")) {
            Messages.postDebug("f1posn=" + Fmt.g(3, f1posn[0]) + " +/- "
                               + Fmt.g(3, f1posn[1]));
            Messages.postDebug("f2posn=" + Fmt.g(3, f2posn[0]) + " +/- "
                               + Fmt.g(3, f2posn[1]));
            Messages.postDebug("r1posn=" + Fmt.g(3, r1posn[0]) + " +/- "
                               + Fmt.g(3, r1posn[1]));
            Messages.postDebug("r2posn=" + Fmt.g(3, r2posn[0]) + " +/- "
                               + Fmt.g(3, r2posn[1]));
        }
        // Calculate shifts in position and their standard deviations
        double shiftF = f1posn[0] - f2posn[0];
        double sdvF = sqrt(f1posn[1] * f1posn[1] + f2posn[1] * f2posn[1]);
        double shiftR = r1posn[0] - r2posn[0];
        double sdvR = sqrt(r1posn[1] * r1posn[1] + r2posn[1] * r2posn[1]);
        // Calculate mean shift
        double wgtF = 1 / (sdvF * sdvF);
        double wgtR = 1 / (sdvR * sdvR);
        double sdv = sqrt(1 / (wgtF + wgtR));
        double shift = (shiftF * wgtF + shiftR * wgtR) / (wgtF + wgtR);

        if (shift > 3 * sdv) { // NB: Valid backlash is positive
            // Save maximum plausible shift as the total backlash
            setBacklash(cmi, shift + 3 * sdv);
            // And display it
            displayMotorBacklash(cmi);
            Messages.postInfo("Backlash for motor " + getMotorNumber(cmi)
                              + ": " + round(shift) + ", sigma="
                              + Fmt.f(1, sdv));
        } else {
            Messages.postWarning("Backlash measurement for motor "
                                 + getMotorNumber(cmi)
                                 + " not significant at 3-sigma"
                                 + " (" + round(shift) + ", sigma="
                                 + Fmt.f(1, sdv) + ")");
        }

        // Go back to where we started
        step(cmi, p0 - getMotorGroupPosition(cmi));

        return ok;
    }

    /**
     * Display the backlash values for the tune and match motors.
     * For master/slave groups, we display the same mean backlash for
     * each motor in the group.
     * @throws InterruptedException If this command is aborted.
     */
    public void displayAllBacklashes() throws InterruptedException {
        int n = getNumberOfMotors();
        for (int cmi = 0; cmi < n; cmi++) {
            displayMotorBacklash(cmi);
        }
    }

    /**
     * Display the backlash value for the given motor.
     * For master/slave groups, we display the same mean backlash for
     * each motor in the group.
     * @param cmi The Channel Motor Index.
     * @throws InterruptedException If this command is aborted.
     */
    private void displayMotorBacklash(int cmi) throws InterruptedException {
        if (cmi < 0 || cmi >= getNumberOfMotors()) {
            m_master.exit("Displaying backlash on non-existent channel motor: "
                          + cmi);
        } else {
            int backlash = getMeanBacklash(cmi);
            for (int gmi=getMotorNumber(cmi); gmi >= 0; gmi=getSlaveOf(gmi)) {
                m_master.sendToGui("displayMotorBacklash "
                                   + gmi + " " + backlash);
            }
        }
    }

    /**
     * @param cmi The Channel Motor Index the backlash is for.
     * @param backlash The backlash value.
     */
    private void setBacklash(int cmi, double backlash) {
        if (!Double.isNaN(backlash)) {
            if (cmi < 0 || cmi >= getNumberOfMotors()) {
                m_master.exit("Setting backlash on non-existent channel motor: "
                              + cmi);
            } else {
                m_backlashList.set(cmi, (int)round(backlash));
            }
        }
    }

    private SensitivityData measureMotorSensitivities(int cmi,
                                                      double[] dipSDV,
                                                      double resultSDV,
                                                      int stepDir,
                                                      TuneInfo finalPosition)
                                              throws InterruptedException {

        if (ProbeTune.isCancel()) {
            return null;
        }

        MotorInfo mInfo = getMotorInfo(cmi);
        if (mInfo == null) {
            return null;
        }
        //double backlash = mInfo.getBacklash(); // Gets old backlash value

        // p0 = initial motor position
        int p0 = getMotorGroupPosition(cmi);
        // t0 = initial tune position
        //TuneInfo t0 = getFreshTunePosition(REMEASURE_TIME);
        //ProbeTune.printlnDebugMessage(5, "ChannelInfo.initializeMotor: "
        //                   + "f=" + t0.getFrequency()
        //                   + ", r=" + t0.getReflection());
        int p1 = p0;

        TuneInfo t1 = getFreshTunePosition(true);
        if (t1 == null) {
            return null;
        }
        double f1 = t1.getFrequency();
        double r1 = t1.getReflection();
        TuneInfo t2 = t1;

        SensitivityData sensitivity = new SensitivityData(dipSDV);
        //sensitivity.addDipDatum(new DipPositionDatum(p1, f1, r1));
        // NB: abs(stepSize) is always guaranteed >= getMinCalStep(cmi)
        int stepSize = getCalStepSize(cmi) * stepDir;
        int prevStepSize = stepSize;
        step(cmi, -stepDir * getMaxBacklash(cmi));
        step(cmi, stepDir * getMaxBacklash(cmi));
        TuneInfo t3;
        while ((t3 = getFreshTunePosition(true)) == null
                || Double.isNaN(t3.getFrequency()))
        {
            step(cmi, stepSize);
        }

        while (!isDone(cmi, sensitivity, stepSize, t2, finalPosition)) {
            if (ProbeTune.isCancel()) {
                return null;
            }
            if (stepSize * prevStepSize < 0) {
                // Reversed direction -- shake out backlash
                stepDir = -stepDir;
                step(cmi, -stepDir * getMaxBacklash(cmi));
                step(cmi, stepDir * getMaxBacklash(cmi));
            }
            // Take a step for calibration
            step(cmi, stepSize);
            prevStepSize = stepSize;
            int p2 = getMotorGroupPosition(cmi);
            int nSteps = p2 - p1;
            t2 = getMeasuredTunePosition();
            double f2 = t2.getFrequency();
            double r2 = t2.getReflection();
            if (sensitivity.add(nSteps, (f2 - f1), (r2 - r1), resultSDV, cmi)) {
                sensitivity.addDipDatum(stepSize,
                                        new DipPositionDatum(p2, f2, r2));
            }
            stepSize = sensitivity.correctStepSize(stepSize, nSteps,
                                                   (f2 - f1), (r2 - r1),
                                                   resultSDV, cmi);
            p1 = p2;
            f1 = f2;
            r1 = r2;
        }

        return sensitivity;
    }

    /**
     * Initialize info about tuning sensitivity and backlash of a motor.
     * @param cmi The index of the motor on this channel, from 0.
     */
    private boolean initializeMotorSensitivities(int cmi)
        throws InterruptedException {

        return measureSensitivities(cmi);
        //return measureMotorSensitivities(cmi, 0.1);
    }

//    private boolean measureMotorSensitivities(int cmi, double sdv)
//        throws InterruptedException {
//
//        /*
//         * NB: If either sensitivity is measured, we measure the other
//         * too.  We get the backlash as a byproduct.
//         */
//        MotorInfo mInfo = getMotorInfo(cmi);
//        if (mInfo == null) {
//            return false;
//        }
//        //double backlash = mInfo.getBacklash();
//        double backlash = getTotalBacklash(cmi); // Accounts for slave motors
//
//        // p0 = initial motor position
//        int p0 = getMotorGroupPosition(cmi);
//        // t0 = initial tune position
//        //TuneInfo t0 = getFreshTunePosition(REMEASURE_TIME);
//        //ProbeTune.printlnDebugMessage(5, "ChannelInfo.initializeMotor: "
//        //                   + "f=" + t0.getFrequency()
//        //                   + ", r=" + t0.getReflection());
//
//        if (ProbeTune.isCancel()) {
//            return false;
//        }
//
//        int p1 = getMotorGroupPosition(cmi);
//        getFreshTunePosition(true);
//        TuneInfo t1 = getMeasuredTunePosition();
//        if (t1 == null) {
//            return false;
//        }
//        double f1 = t1.getFrequency();
//        double r1 = t1.getReflection();
//
//        // Make sure we have enough sweep width to make the measurements
//        int freqDir = getSafeTuneDirection(cmi, f1);
//        m_master.setFullSweep();
//
//        double[] dipSDV = measureDipPositionSDV();
//        SensitivityData sensitivity = new SensitivityData(cmi, dipSDV);
//        TuneInfo t2 = null;
//        // NB: abs(stepSize) is always guaranteed >= getMinCalStep(cmi)
//        int stepSize = getCalStepSize(cmi) * freqDir;
//        while (!sensitivity.isMeasured()) {
//            if (ProbeTune.isCancel()) {
//                return false;
//            }
//            // Take a step for calibration
//            step(cmi, stepSize);
//            int p2 = getMotorGroupPosition(cmi);
//            int nSteps = p2 - p1;
//            t2 = getMeasuredTunePosition();
//            double f2 = t2.getFrequency();
//            double r2 = t2.getReflection();
//            sensitivity.add(nSteps, (f2 - f1), (r2 - r1), sdv, cmi);
//            stepSize = sensitivity.correctStepSize(stepSize, nSteps,
//                                                   (f2 - f1), (r2 - r1),
//                                                   sdv, cmi);
//            p1 = p2;
//            f1 = f2;
//            r1 = r2;
//        }
//
//        m_hzPerStep[cmi] = sensitivity.getFreqSensitivity();
//        if (!m_freqSensLimits[cmi].contains(m_hzPerStep[cmi])) {
//            String type = "frequency";
//            String knob = cmi == getMatchMotorChannelIndex() ? "match" : "tune";
//            postSensitivityWarning(cmi, type, knob);
//        }
//        m_rPerStep[cmi] = sensitivity.getReflSensitivity();
//        if (!m_reflSensLimits[cmi].contains(m_rPerStep[cmi])) {
//            String type = "reflection";
//            String knob = cmi == getMatchMotorChannelIndex() ? "match" : "tune";
//            postSensitivityWarning(cmi, type, knob);
//        }
//
//        updateSensitivityDisplay(cmi);
//
//        Messages.postDebug("Backlash|Sensitivity",
//                           "measureMotorSensitivity(): Getting backlash... ");
//
//        /*SDM CHANGE BACKLASH ROUTINE
//
//        // Go back to Point #1 to get backlash
//        TuneInfo t3 = t2; // NB: Don't really need to save t2 here
//        double nextStep = getDistance(t3, t1, cmi); // nextStep < 0
//        //double nextStep = (f1 - f3) / hzPerStep; // nextStep < 0
//        while (!m_master.isCancel() && nextStep < 0) {
//            int n = (int) round(0.8 * nextStep);
//            if (n == 0) {
//                break;
//            }
//            step(cmi, n);
//            //CMP ProbeTune.printlnDebugMessage(5, "------ step(" + (nextStep) + ")");
//            t3 = getMeasuredTunePosition();
//            nextStep = getDistance(t3, t1, cmi);
//        }
//        //t3 = measureTune(500, true); // Get accurate reading
//        int p3 = getMotorGroupPosition(cmi);
//        backlash = p1 - p3;
//        SDM Change Backlash Routine*/
//
//
//        TuneInfo t0;
//        double f0;
//        double r0;
//        double forig;
//        double rorig;
//        double fDiff;
//        double rDiff;
//        int direction;
//        String property = "match";
//
//        direction = movePastBacklash(cmi);
//        direction = -direction;        //reverse direction
//
//      //step in direction of farther end of sweep
//        t0= getMeasuredTunePosition();
//        f0 = t0.getFrequency();
//        r0= t0.getReflection();
//        forig= f0;
//        rorig= r0;
//        backlash=0;             //reset backlash
//
//        Messages.postDebug("Sensitivity", "Direction to move: " + direction);
//
//        fDiff= abs(f0-forig);
//        rDiff= abs(r0-rorig);
//        int p3a= mInfo.getPosition();
//        while ( true ){
//                step(cmi, direction*MotorInfo.BACKLASH_STEP);
//                t0= getMeasuredTunePosition();
//                f0=t0.getFrequency();
//                r0=t0.getReflection();
//                fDiff= abs(f0-forig);
//            rDiff= abs(r0-rorig);
//            if(rDiff>MotorInfo.CHANGE_IN_MATCH){
//                property= "match";
//                break;
//            }
//            if(fDiff>MotorInfo.CHANGE_IN_FREQ){
//                property= "freq";
//                break;
//            }
//        }
//        int p3 = mInfo.getPosition();
//        backlash=abs(p3-p3a);
//        Messages.postDebug("Backlash", property + " backlash=" + backlash);
//
//        backlash = max(MIN_BACKLASH, min(MAX_BACKLASH, backlash));
//
//        // NB: Compare result against System motor file values,
//        Limits limits = mInfo.getBacklashLimits();
//        if (!limits.contains(backlash)) {
//            popupBacklashWarning(mInfo, backlash, limits);
//            // If backlash outside limit, use system value
//            backlash = mInfo.getDefaultBacklash();
//            Messages.postWarning("Backlash outside limits. Keep old: "
//                                 + backlash);
//        }
//        mInfo.setBacklash(backlash);
//        int tuneCmi = getTuneMotorChannelIndex();
//        String motor = (cmi == tuneCmi) ? "tune" : "match";
//        m_master.displayBacklash(backlash, motor);
//
//        // Go back to where we started
//        step(cmi, p0 - getMotorGroupPosition(cmi));
//
//        if (ProbeTune.isCancel()) {
//            return false;
//        }
//
//        // Display results
//        //m_master.displayCalibration();
//
//        return (!Double.isNaN(m_hzPerStep[cmi])
//                && !Double.isNaN(m_rPerStep[cmi])
//                && !Double.isNaN(backlash));
//    }

    /**
     * Get the safer direction to go to keep the dip in the frequency
     * range of the channel.
     * Chooses the direction which is farther in frequency from the end
     * of the channel frequency range.
     * @param cmi The Channel Motor Index of the motor to move.
     * @param f1 The current dip location (Hz).
     * @return Direction to step: 1 or -1; or 0 if it cannot be determined.
     */
    public int getSafeTuneDirection(int cmi, double f1) {
        double fmin = getMinFreq();
        double fmax = getMaxFreq();
        double lowerRange = f1 - fmin;
        double upperRange = fmax - f1;
        int stepDir = 1;
        if (lowerRange > upperRange) {
            stepDir = -1;
        }
        if (m_hzPerStep != null && !Double.isNaN(m_hzPerStep[cmi])) {
            stepDir *= signum(m_hzPerStep[cmi]);
        } else if (m_defaultHzPerStep != null
                   && !Double.isNaN(m_defaultHzPerStep[cmi]))
        {
            stepDir *= signum(m_defaultHzPerStep[cmi]);
        } else {
            stepDir = 0;
        }
        return stepDir;
    }

    /**
     * See if the current sensitivity measurement is done.
     * I.e., if it has either reached the destination position, or
     * (if there is no destination) the sensitivity is well measured.
     * @param cmi The Channel Motor Index of the motor to move.
     * @param sensitivity The accumulated data.
     * @param step The proposed step.
     * @param position The current tune position.
     * @param dst The destination tune position, or null.
     * @return True if this step would take us away from the destination.
     * If the destination is null, returns false.
     */
    private boolean isDone(int cmi, SensitivityData sensitivity, int step,
                           TuneInfo position, TuneInfo dst) {
        boolean rtn = sensitivity.isMeasured();
        if (dst != null) {
            // Scale so that delta of 10 MHz in freq = delta of 1 in reflection
            // TODO: scale according to SDV's of freq and refl measurements?
            double dFreq = dst.getFrequency() - position.getFrequency();
            dFreq /= 1e7;
            double dRefl = dst.getReflection() - position.getReflection();
            double stepFreq = getFreqDelta(cmi, step);
            stepFreq /= 1e7;
            double stepRefl = getReflDelta(cmi, step);
            // Check the dot product
            // TODO: Check if expected dst closer than current position?
            rtn = rtn && (dFreq * stepFreq + dRefl * stepRefl) <= 0;
        }
        return rtn;
    }

    /**
     * For a given step, returns the expected change in frequency.
     * @param cmi Channel Motor Index of the motor to step.
     * @param step The proposed step.
     * @return The predicted change in frequency, or 0 if we don't know.
     */
    private double getFreqDelta(int cmi, int step) {
        double freqDelta = 0;
        if (m_hzPerStep != null && !Double.isNaN(m_hzPerStep[cmi])) {
            freqDelta = step * m_hzPerStep[cmi];
        } else if (m_defaultHzPerStep != null
                   && !Double.isNaN(m_defaultHzPerStep[cmi]))
        {
            freqDelta = step * m_defaultHzPerStep[cmi];
        }
        return freqDelta;
    }

    /**
     * For a given step, returns the expected change in reflection.
     * @param cmi Channel Motor Index of the motor to step.
     * @param step The proposed step.
     * @return The predicted change in reflection, or 0 if we don't know.
     */
    private double getReflDelta(int cmi, int step) {
        double reflDelta = 0;
        if (m_rPerStep != null && !Double.isNaN(m_rPerStep[cmi])) {
            reflDelta = step * m_rPerStep[cmi];
        } else if (m_defaultRPerStep != null
                   && !Double.isNaN(m_defaultRPerStep[cmi]))
        {
            reflDelta = step * m_defaultRPerStep[cmi];
        }
        return reflDelta;
    }

    public int getSafeMatchDirection(int cmi, double refl) {
        int matchDir = -(int)signum(refl);
        if (m_rPerStep != null && !Double.isNaN(m_rPerStep[cmi])) {
            matchDir *= signum(m_rPerStep[cmi]);
        } else if (m_defaultRPerStep != null
                   && !Double.isNaN(m_defaultRPerStep[cmi]))
        {
            matchDir *= signum(m_defaultRPerStep[cmi]);
        } else {
            matchDir = 0;
        }
        return matchDir;
    }

    /**
     * Display a pop-up window to warn of inconsistency in sensitivity
     * calibration. To be called if the measured sensitivity is outside
     * the limits allowed by the system channel file.
     * @param cmi The Channel Motor Index of the offender.
     * @param type Which measurement: "frequency" or "reflection".
     * @param knob Which motor: "tune" or "match".
     */
    private void postSensitivityWarning(int cmi, String type, String knob) {
        String name= getChannelName();
        String chanFilePath = m_master.getSysTuneDir() + "/" + name;
        // String chanFile = ProbeTune.getVnmrSystemDir() + "/tune/" + name
        double value;
        Limits limits;
        if ("frequency".equals(type)) {
            value = m_hzPerStep[cmi];
            limits = m_freqSensLimits[cmi];
        } else if ("reflection".equals(type)) {
            value = m_rPerStep[cmi];
            limits = m_reflSensLimits[cmi];
        } else {
            Messages.postError("Unknown type \"" + type
                               + "\" in call to postCalibrationError()");
            return;
        }
        String msg = "Error setting new " + type
                + " sensitivity for the " + knob + " motor.<br>"
                + "Measured value: " + Fmt.g(3, value) + "<br>"
                + "Limits from system: " + limits.toString(3, "g") + "<br>"
                + "System channel file: " + chanFilePath;
        String title = "Sensitivity_Calibration_Warning";
        m_master.sendToGui("popup warn title " + title + " msg " + msg);
    }

//    /**
//     * Display a pop-up window to warn of inconsistency in backlash
//     * calibration. To be called if the measured backlash is outside
//     * the limits allowed by the system motor file.
//     */
//    private void popupBacklashWarning(MotorInfo mi,
//                                      double value, Limits limits) {
//        String name= mi.getMotorName();
//        String motorFilePath = m_master.getSysTuneDir() + "/" + name;
//        String msg = "Error setting new backlash value for " + name + ".<br>"
//                + "Measured value: " + Fmt.f(0, value) + "<br>"
//                + "Limits from system motor file: " + limits.toString(0, "f")
//                + "<br>"
//                + "System motor file: " + motorFilePath;
//        String title = "Backlash_Calibration_Warning";
//        m_master.sendToGui("popup warn title " + title + " msg " + msg);
//    }

    private int getCalStepSize(int cmi) throws InterruptedException {
        return clipCalStep(m_calStepsize[cmi], cmi);
    }

    /**
     * Get the sum of the positions of a given motor plus all its slaves.
     * @param cmi The Channel Motor Index of the motor.
     * @return The sum of the positions.
     */
    private int getMotorGroupPosition(int cmi) throws InterruptedException {
        int position = 0;
        int gmi = getMotorNumber(cmi);
        if (gmi >= 0) {
            for (int motor = gmi; motor >= 0; motor = getSlaveOf(motor)) {
                position += getGlobalMotorInfo(motor).getPosition();
            }
        }
        return position;
    }

//    /**
//     * Get the number of motors controlled by a given channel motor.
//     * This is the master motor plus all of its slaves.
//     * @param cmi
//     * @return
//     */
//    private int getMotorGroupSize(int cmi) {CalibrationError
//        int size = 0;
//        int gmi = getMotorNumber(cmi);
//        if (gmi >= 0) {
//            for (int motor = gmi; motor >= 0; motor = getSlaveOf(motor)) {
//                size++;
//            }
//        }
//        return size;
//    }
//

    public void displaySensitivities() {
        // Clear match display if no match motor
        if (getNumberOfMotors() == 1) {
            m_master.displayHzPerStep(0, "match");
            m_master.displayMatchPerStep(0, "match");
        }
        for (int cmi = 0; cmi < getNumberOfMotors(); cmi++) {
            updateSensitivityDisplay(cmi);
        }
    }

    private void updateSensitivityDisplay(int cmi) {
        int tuneCmi = getTuneMotorChannelIndex();
        String motor = (cmi == tuneCmi) ? "tune" : "match";
        m_master.displayHzPerStep(m_hzPerStep[cmi], motor);
        m_master.displayMatchPerStep(m_rPerStep[cmi], motor);
    }

    /**
     * Measure the current tune position on this channel.
     * The result is stored as the Measured Tune Position and as the
     * Estimated Tune Position, as well as being returned to the caller.
     * @param when Wait for data acquired after this date, in ms.
     * @param display If true, the sweep for the measurement is
     * sent to the GUI.
     * @return The frequency and reflection at the dip.
     * @throws InterruptedException if this command is aborted.
     */
    public TuneInfo measureTune(long when, boolean display)
        throws InterruptedException {

        return m_master.measureTune(this, when, display);
    }

    /**
     * Get distance in steps between two tuning points.  Assumes that
     * turning only the indicated motor will be able to get there.
     * @param src Tuning where we start.
     * @param dst Tuning where we want to get.
     * @param cmi The motor to use to get there.
     * @return The number of steps on the given motor.
     */
    public double getDistance(TuneInfo src, TuneInfo dst, int cmi) {
        double nSteps = Double.NaN;
        if (m_hzPerStep[cmi] != 0) {
            double fdst = dst.getFrequency();
            double fsrc = src.getFrequency();
            nSteps = (fdst - fsrc) / m_hzPerStep[cmi];
        }
        if (m_rPerStep[cmi] != 0) {
            double rdst = dst.getReflection();
            double rsrc = src.getReflection();
            double[] nStepsR = new double[4];
            nStepsR[0] = (rdst - rsrc) / m_rPerStep[cmi];
            if (Double.isNaN(nSteps)) {
                return nStepsR[0];
            }
            //nStepsR[1] = (-rdst - rsrc) / m_rPerStep[cmi];
            //nStepsR[2] = -nStepsR[0];
            //nStepsR[3] = -nStepsR[1];
            //double nSteps1 = nStepsR[0];
            //double diff = abs(nSteps1 - nSteps);
            //for (int i = 1; i < 4; i++) {
            //    if (abs(nStepsR[i] - nSteps) < diff) {
            //        nSteps1 = nStepsR[i];
            //        diff = abs(nSteps1 - nSteps);
            //    }
            //}
            //double wf = abs(m_hzPerStep[cmi] / src.getFrequencySdv());
            //double wr = abs(m_rPerStep[cmi] / src.getReflectionSdv());
            //double xxx = nSteps;
            //nSteps = (nSteps * wf + nSteps1 * wr) / (wf + wr);
            //ProbeTune.printlnDebugMessage(5, "Motor=" + cmi
            //                   + ", wf=" + Fmt.e(2, wf)
            //                   + ", wr=" + Fmt.e(2, wr)
            //                   + ", nSteps=" + Fmt.f(1, xxx)
            //                   + ", nSteps1=" + Fmt.f(1, nSteps1)
            //                   + ", avg=" + Fmt.f(1, nSteps));
        }
        return nSteps;
    }

    /**
     * Returns the last measured tune position.
     * @return The Tune Position.
     */
    public TuneInfo getMeasuredTunePosition() {
        return m_measuredTune;
    }

    /**
     * Returns a guess at the current tune state. Used after changing the
     * tune position, before running a frequency sweep.  Useful to know
     * where and how wide to make the sweep.
     * @return The Tune Position.
     */
    public TuneInfo getEstimatedTunePosition() {
        return m_estimatedTune;
    }

    /**
     * Estimate where the dip will be after a given motor step.
     * @param oldTinfo The current dip position.
     * @param cmi The motor (Channel Motor Index) that we propose to move.
     * @param nSteps The distance we propose to move the motor.
     * @return The predicted tune position.
     */
    private TuneInfo predictTune(TuneInfo oldTinfo, int cmi, int nSteps) {
        TuneInfo newTinfo = oldTinfo;
        double oldFreq = oldTinfo.getFrequency();
        double oldRefl = oldTinfo.getReflection();
        double dFreq = getHzPerStep(cmi) * nSteps;
        double dMatch = getReflectionPerStep(cmi) * nSteps;
        double fEst1 = oldFreq + dFreq;
        double mEst1 = oldRefl + dMatch;
        if (!Double.isNaN(fEst1) && !Double.isNaN(fEst1)) {
            newTinfo = new TuneInfo(fEst1);
            newTinfo.setReflection(mEst1);
        }
        return newTinfo;
    }

    public void setMeasuredTunePosition(TuneInfo tinfo) {
        m_measuredTune = tinfo;
        m_estimatedTune = tinfo;
    }

    /**
     * Clear the stuck motor counts.
     * This does not try to fix the motors, it just clears the record of
     * any suspected stuck motor.
     * A stuck motor is one that is not changing the data.
     * The stuck count is just the number of consecutive times the tune
     * didn't change when we expected it to.
     */
    public void clearStuckMotors() {
        for (int i = 0; i < m_motorStuck.length; i++) {
            m_motorStuck[i] = 0;
        }
    }

    /**
     * Check if any motor is stuck.  If more than one is stuck, only
     * one is reported.
     * @see ChannelInfo#clearStuckMotors()
     * @return The number of any stuck motor, or -1 if all OK.
     */
    public int getStuckMotorNumber() {
        for (int i = 0; i < m_motorStuck.length; i++) {
            if (m_motorStuck[i] > STUCK_MOTOR_RETRIES) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Clear End-Of-Travel flags for tune/match motors on this channel.
     * @throws InterruptedException if this command is aborted.
     */
    public void clearEotMotors() throws InterruptedException {
        for (int i = 0; i < m_motorList.length; i++) {
            getMotorInfo(i).setEot(0);
            this.setAtLimit(i, false);
        }
    }

    public int getEotMotorNumber() throws InterruptedException {
        int nMotors = m_motorList.length;
        for (int i = 0; i < nMotors; i++) {
            if (getMotorInfo(i).isAtLimit(0)) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Update the dip position, due to a motor motion.
     *
     * @param cmi Channel Motor Index of the motor.
     * @param nSteps Number of steps it moved.
     * @param when Earliest time to collect new data.
     * @throws InterruptedException if this command is aborted.
     */
    public void remeasureTune(int cmi, int nSteps, long when)
        throws InterruptedException {

        // TODO: Check the logic in this method!
        if (cmi < 0) {
            return;
        }
        double currentFreq = Double.NaN;
        double currentRefl = Double.NaN;
        if (m_measuredTune != null) {
            currentFreq = m_measuredTune.getFrequency();
            currentRefl = m_measuredTune.getReflection();
        }
        if (Double.isNaN(currentFreq) && m_estimatedTune != null) {
            currentFreq = m_estimatedTune.getFrequency();
        }
        if (Double.isNaN(currentRefl) && m_estimatedTune != null) {
            currentRefl = m_estimatedTune.getReflection();
        }
        Messages.postDebug("RemeasureTune",
                           "ChannelInfo.remeasureTune(): "
                           + "currentFreq=" + Fmt.f(6, currentFreq/1e6)
                           + ", currentRefl=" + Fmt.f(6, currentRefl));
        if (m_measuredTune == null
            || currentFreq < m_minFreq
            || currentFreq > m_maxFreq
            || currentRefl > 0.2)
        {
            setMeasuredTunePosition(measureTune(when, true));
        } else {
            // Dead reckoning from the last measurement or estimate
            TuneInfo currentTune = new TuneInfo(currentFreq, currentRefl);
            m_estimatedTune = predictTune(currentTune, cmi, nSteps);
            double fEst1 = m_estimatedTune.getFrequency();
            double rEst1 = m_estimatedTune.getReflection();
            m_measuredTune.setOutOfDate(true);

            // Take a new measurement
            double sweepCenter = m_master.getCenter();
            double sweepSpan = m_master.getSpan();
            if (abs(fEst1 - sweepCenter) > 0.5 * sweepSpan / 1.3) {
                m_master.setCenter(fEst1);
            }
            TuneInfo ti = measureTune(when, isUpdateDisplayed());
            if (ti == null) {
                return;
            }

            // See if the dip is moving as much as we think we should
            // TODO: Allow for backlash properly in expected motion?
            // Actual motions:
            double df = ti.getFrequency() - currentFreq;
            double dr = ti.getReflection() - currentRefl;
            // Exptected motions:
            double dFreq = fEst1 - currentFreq;
            double dRefl = rEst1 - currentRefl;

            double matchSdv = ti.getReflectionSdv();
            double freqSdv = ti.getFrequencySdv();
            boolean useRefl = (matchSdv != 0
                               && abs(dRefl / matchSdv) > 4);
            boolean useFreq = (freqSdv != 0
                               && abs(dFreq / freqSdv) > 4);
            if ((useRefl || useFreq) // Should see significant motion, but...
                && (!useRefl || abs(dr / dRefl) < 0.05) // none on refl
                && (!useFreq || abs(df / dFreq) < 0.05)) // none on freq
            {
                // Cable may be loose
                ++m_motorStuck[cmi];
                Messages.postDebug("ChannelInfo",
                                   "Motor #" + cmi + ": stick count = "
                                   + m_motorStuck[cmi]);
            } else if (useRefl || useFreq) {
                // Seems OK; decrement the count, but not below 0.
                m_motorStuck[cmi] = max(--m_motorStuck[cmi], 0);
            }
        }
    }

    public boolean isUpdateDisplayed() {
        return m_isUpdateDisplayed;
    }

    public void setUpdateDisplayed(boolean b) {
        m_isUpdateDisplayed = b;
    }

    public boolean isTuneInfoOld() {
        TuneInfo tuneInfo = getMeasuredTunePosition();
        return tuneInfo == null || tuneInfo.getAge() > REMEASURE_TIME;
    }

    public TuneInfo getFreshTunePosition(boolean display)
        throws InterruptedException {

        return getFreshTunePosition(REMEASURE_TIME, display);
    }

    public TuneInfo getFreshTunePosition(long maxAge)
        throws InterruptedException {

        return getFreshTunePosition(maxAge, true);
    }

    public TuneInfo getFreshTunePosition(long maxAge,boolean display)
        throws InterruptedException {

        TuneInfo tuneInfo = getMeasuredTunePosition();
        if (tuneInfo == null || tuneInfo.getAge() > maxAge) {
            Messages.postDebug("AllTuneAlgorithm", "Get a new measured tune");
            tuneInfo = measureTune(00, display);
        }
        return tuneInfo;
    }

    /**
     * Determine how to get to the desired tune position.
     * NOTE: only handles two motors per channel--any more would
     * overdetermine the problem.
     * @param freq_hz The frequency we want to get to.
     * @return The i'th element in the returned array is the number of
     * steps required on the i'th motor of the channel.
     * @throws InterruptedException if this command is aborted.
     */
    public double[] calcStepsToTune(double freq_hz)
    throws InterruptedException {

        int n = getNumberOfMotors();
        TuneInfo start = getMeasuredTunePosition();
        double dfreq = freq_hz - start.getFrequency();
        double drefl = 0 - start.getReflection();

        double[] rtn = new double[n];

        if (n <= 0 || n > 2) {
            // Can't currently handle more than 2 motors
            Messages.postError(n + " motors defined in channel file? "
                               + getChannelName());
            return rtn;
        }

        double[][] eqns;
        int[] limitedMotors = new int[0];
        int nMovableMotors = n;
        if (nMovableMotors == 2) {
            // Normal case -- motors for tune and match
            // But we may find that a motor is at its limit, and we'll end
            // up doing this as n==1.
            // Put equations into this augmented matrix:
            eqns = new double[2][3];
            eqns[0][0] = getHzPerStep(0);
            eqns[0][1] = getHzPerStep(1);
            eqns[0][2] = dfreq;
            eqns[1][0] = getReflectionPerStep(0);
            eqns[1][1] = getReflectionPerStep(1);
            eqns[1][2] = drefl;
            rtn = NLFit.solve(eqns);
            // printlnDebugMessage(5, "steps=" + Fmt.f(2, rtn, false, false));
            // TODO: If motor is not at limit, but cannot go as far as we want,
            //       recalculate requested step for other motor?
            limitedMotors = getLimitedMotors(rtn);
            nMovableMotors -= limitedMotors.length;
        }

        // We recalculate if not all the calculated steps are possible
        if (nMovableMotors < 2) {
            // Only the movable motor will get a non-zero step value
            for (int i = 0; i < n; i++) {
                rtn[i] = 0;
            }
            int cmi = -1;
            for (int i = 0; i < n; i++) {
                if (Arrays.binarySearch(limitedMotors, i) < 0) {
                    cmi = i;
                    break;
                }
            }
            if (cmi >= 0) {
                rtn[cmi] = calcSingleMotorStepsToTune(cmi, freq_hz);
            }
        }
        Messages.postDebug("AllTuneAlgorithm", "calcStepsToTune:"
                           + " steps=" + Fmt.f(2, rtn, false, false));
        double d = setTuneStepDistance(rtn);
        // NB: Added check for NaN - 6/13/2008
        if (Double.isNaN(d)) {
            if (DebugOutput.isSetFor("TuneAlgorithm")) {
                Messages.postWarning("calcStepsToTune() failed:");
                Messages.postStackTrace(new Exception());
            }
            return null;
        } else {
            return rtn;
        }
    }

    public boolean retrieveDip() throws InterruptedException {
        // Where we think we are
        TuneInfo estPosn = getEstimatedTunePosition();
        double estFreq = estPosn.getFrequency();
        if (estFreq - getMinFreq() < 1e6 || getMaxFreq() - estFreq < 1e6) {
            int tuneCmi = getTuneMotorChannelIndex();
            int tuneDir = getSafeTuneDirection(tuneCmi, estFreq);
            for (int i = 0; i < 100 && !isDipDetected(); i++) {
                step(tuneCmi, tuneDir * m_calStepsize[tuneCmi]);
            }
        }
        double estRefl = estPosn.getReflection();
        if (abs(estRefl) > 0.25) {
            int matchCmi = getMatchMotorChannelIndex();
            int matchDir = getSafeMatchDirection(matchCmi, estRefl);
            for (int i = 0; i < 100 && !isDipDetected(); i++) {
                step(matchCmi, matchDir * m_calStepsize[matchCmi]);
            }
        }

        return isDipDetected();
    }

    /**
     * Given an array of desired motor motions, return a list (array) of the
     * motors that cannot move in the requested direction because of
     * being at their end-of-travel.
     * @param stepList The desired motion of each motor.
     * @return The Channel Motor Indices (cmi's) of the limited motors.
     */
    private int[] getLimitedMotors(double[] stepList)
        throws InterruptedException {

        List<Integer> limitedList = new ArrayList<Integer>(0);
        if (stepList != null) {
            int n = getNumberOfMotors();
            for (int cmi = 0; cmi < n; cmi++) {
                MotorInfo mInfo = getMotorInfo(cmi);
                if (mInfo.isAtLimit(stepList[cmi])) {
                    limitedList.add(cmi);
                }
            }
        }
        int listLen = limitedList.size();
        int[] intList = new int[listLen];
        for (int i = 0; i < listLen; i++) {
            intList[i] = limitedList.get(i);
        }
        return intList;
    }

    /**
     * Calculate the number of steps from the current tune position to
     * the given target, moving only the given motor.
     * If it's the tune motor, we will optimize the frequency,
     * if the match motor, we will minimize the reflection.
     * @param cmi Channel Motor Index of the motor to adjust.
     * @param freq_hz The target tune frequency.
     * @return The number of steps to the target position.
     */
    protected double calcSingleMotorStepsToTune(int cmi, double freq_hz) {
        TuneInfo start = getMeasuredTunePosition();
        double steps = 0;
        if (cmi == getTuneMotorChannelIndex()) {
            double dfreq = freq_hz - start.getFrequency();
            // TODO: Don't let the match get so bad that we can't see the dip
            steps = dfreq / getHzPerStep(cmi);
        } else if (cmi == getMatchMotor()) {
            double drefl = 0 - start.getReflection();
            steps = drefl / getReflectionPerStep(cmi);
        }
        return steps;
    }


    /**
     * Sets the estimated distance from the current position to the perfectly
     * tuned position, in motor steps. If more than one motor needs to be
     * moved to tune, this is sqrt(X^2 + Y^2).
     * @param steps The distance from tune on each motor (tune and match).
     * @return The distance or NaN. Member variable m_tuneStepDistance
     * is also set to this value.
     */
    public double setTuneStepDistance(double[] steps) {
        double rtn = Double.NaN;
        if (steps != null) {
            int len = steps.length;
            rtn = 0;
            for (int i = 0; i < len; i++) {
                rtn += steps[i] * steps[i];
            }
            rtn = sqrt(rtn);
        }
        return setTuneStepDistance(rtn);
    }

    /**
     * Sets the estimated distance from the current position to the optimum
     * tuned position, in motor steps. If more than one motor needs to be
     * moved to tune, this is sqrt(X^2 + Y^2).
     * @param steps The distance in motor steps.
     * @return The steps value.
     */
    public double setTuneStepDistance(double steps) {
        Messages.postDebug("AllTuneAlgorithm", "ChannelInfo.setTuneStepDistance("
                           + Fmt.f(1, steps) + ")");
        m_tuneStepDistance = steps;
        return steps;
    }

    /**
     * Gets the estimated distance from the current position to the perfectly
     * tuned position, in motor steps. If more than one motor needs to be
     * moved to tune, this is sqrt(X^2 + Y^2).
     * This is used to determine if we are as close as practically possible
     * to the target.
     * @return The distance in steps.
     */
    public double getTuneStepDistance() {
        return m_tuneStepDistance;
    }

    /**
     * Set up the RF routing so the sweep gets the signal for this channel.
     * Currently a NO-OP.
     * @return Always true.
     */
    public boolean setRfRouting() {
        Messages.postDebug("ChannelInfo", "Route to chan " + m_channelNumber);
        boolean ok = true;
        return ok;
    }

    /**
     * Get a list of the fixed switches on this channel
     * (non frequency-dependent).
     * @return The list as an array. May be empty, but never null.
     */
    public SwitchPosition[] getFixedSwitches() {
        List<SwitchPosition> alist = new ArrayList<SwitchPosition>();
        for (SwitchPosition sp : m_switchList) {
            if (sp.isFixedPosition()) {
                alist.add(sp);
            }
        }
        return alist.toArray(new SwitchPosition[0]);
    }

    /**
     * Get a list of the frequency dependent switches on this channel.
     * @return The list as an array. May be empty, but never null.
     */
    public SwitchPosition[] getFrequencyDependentSwitches() {
        List<SwitchPosition> alist = new ArrayList<SwitchPosition>();
        for (SwitchPosition sp : m_switchList) {
            if (sp.isFrequencyDependent()) {
                alist.add(sp);
            }
        }
        return alist.toArray(new SwitchPosition[0]);
    }

    /**
     * Move any "switch" motors on this tuning channel to the positions
     * appropriate for the given frequency.
     * Will not move motors with fixed switch positions.
     * @param freq The frequency (Hz).
     * @return True if motor(s) moved successfully to position(s).
     * @throws InterruptedException if this command is aborted.
     *
     */
    public boolean setSwitchToFreq(double freq) throws InterruptedException {
        Messages.postDebug("ChannelInfo", "setSwitchToFreq(" + freq + ")");
        boolean ok = true;
        SwitchPosition[] switchList = getFrequencyDependentSwitches();
        for (SwitchPosition sp : switchList) {
            int position = sp.getPosition(freq);
            int motor = sp.getMotor();
            int tol = sp.getTolerance();
            MotorInfo mi = m_master.getMotorInfo(motor);
            if (position > (mi.getMaxlimit()-3*mi.getStepsPerRev())) {
                position = mi.getMaxlimit();
                Messages.postDebug("ChannelInfo", "Move to upper limit: "
                                   + position);
            } else if (position < (mi.getMinlimit()
                    + 10 * mi.getStepsPerRev()))
            {
                position = mi.getMinlimit();
                Messages.postDebug("ChannelInfo", "Move to lower limit: "
                                   + position);
            }
            ok &= moveMotorToPosition(motor, position, tol);

        }
        return ok;
    }

    /**
     * Switch the probe to this channel. (For auto-switchable probes.)
     * Only set non-frequency-dependent switches.
     * @return True if switches were successfully set.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean setProbeChannel() throws InterruptedException {
        boolean ok = true;

        Messages.postDebug("ChannelInfo",
                           "setProbeChannel to " + m_channelNumber);

        // Set the probe channel
        SwitchPosition[] switchList = getFixedSwitches();
        for (SwitchPosition sp : switchList) {
            int position = sp.getPosition();
            int motor = sp.getMotor();
            int tol = sp.getTolerance();
            ok = moveMotorToPosition(motor, position, tol);
        }
        return ok;
    }

    /**
     * Given a distance to be stepped by a set of motors, calculate how
     * far each should step to keep all the motors as nearly in the same
     * position as possible. No motors will be moved backwards to get
     * the group together.
     * @param step The distance to be stepped (the sum of the
     * distances moved by all the motors).
     * @param positions The current positions of the motors.
     * @return List giving the distance to be stepped by each motor.
     */
    protected static int[] calculateGroupOfSteps(int step,
                                                 int[] positions,
                                                 int[] limits) {
        int n = min(limits.length, positions.length);
        int[] steps = new int[n];
        int totalSteps = step;
        if (n == 1) {
            steps[0] = totalSteps;
        } else {
            // Which motor is farthest in the direction we are stepping?
            int sign = step >= 0 ? 1 : -1;
            int extremePosition = positions[0];
            for (int i = 1; i < n; i++) {
                if (sign * extremePosition < sign * positions[i]) {
                    extremePosition = positions[i];
                }
            }

            // Try to get everybody up to at least the extreme position
            //  (unless there aren't enough totalSteps to do the job)
            totalSteps = lineUpMotors(positions, limits, steps,
                                      extremePosition, totalSteps);

            // Now move all the motors about an equal distance
            while (totalSteps != 0) {
                totalSteps = moveEqually(steps, positions, limits, totalSteps);
            }
        }
        return steps;
    }

    private static int moveEqually(int[] steps, int[] positions,
                                   int[] limits, int totalSteps) {
        int n = min(limits.length, positions.length);
        int nImmovable = 0;
        for (int i = 0; i < n; i++) {
            nImmovable += (limits[i] == positions[i]) ? 1 : 0;
        }
        int nMovable = n - nImmovable;
        for (int i = 0; totalSteps != 0 && i < n; i++) {
            if (limits[i] != positions[i]) {
                int meanStep = totalSteps / nMovable;
                if (abs(meanStep) > abs(totalSteps)) {
                    meanStep = totalSteps;
                }
                if (totalSteps < 0) {
                    meanStep = max(meanStep, limits[i] - positions[i]);
                } else {
                    meanStep = min(meanStep, limits[i] - positions[i]);
                }
                steps[i] += meanStep;
                positions[i] += meanStep;
                totalSteps -= meanStep;
                nMovable--;
            }
        }
        return (nMovable == 0) ? 0 : totalSteps;
    }

    private static int lineUpMotors(int[] positions, int[] limits, int[] steps,
                                    int extremePosition, int totalSteps) {
        int n = min(limits.length, positions.length);
        for (int i = 0; i < n; i++) {
            int target = (totalSteps < 0)
                    ? max(extremePosition, limits[i])
                    : min(extremePosition, limits[i]);
            steps[i] = target - positions[i];
            if (abs(steps[i]) > abs(totalSteps)) {
                steps[i] = totalSteps;
            }
            positions[i] += steps[i];
            totalSteps -= steps[i];
        }
        return totalSteps;
    }

//    private static int getShortStep(int step, int[] positions, int[] limits) {
//        if (step == 0) {
//            return 0;
//        }
//        int n = min(limits.length, positions.length);
//        int shortStep = step / n;
//        for (int i = 0; i < n; i++) {
//            if (step < 0) {
//                shortStep = max(shortStep, limits[i] - positions[i]);
//            } else {
//                shortStep = min(shortStep, limits[i] - positions[i]);
//            }
//        }
//        return shortStep;
//    }

    /**
     * Move motor "cmi" by "nSteps" steps.  The cmi number means
     * the cmi'th motor on this channel, starting from 0.
     * Data is redisplayed after the step.
     * @param cmi Channel Motor Index of the motor.
     * @param nSteps Number of steps to move.
     * @return True if this step was in the same direction as the previous one.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean step(int cmi, int nSteps) throws InterruptedException {
        return step(cmi, nSteps, true);
    }

    /**
     * Move motor "cmi" by "nSteps" steps.  The cmi number means
     * the cmi'th motor on this channel, starting from 0.
     * @param cmi Channel Motor Index of the motor.
     * @param nSteps Number of steps to move.
     * @param display If true, data is redisplayed after the step.
     * @return True if this step was in the same direction as the previous one.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean step(int cmi, int nSteps, boolean display)
    throws InterruptedException {

        boolean sameDirection = false;

        if (ProbeTune.isCancel()) {
            return false;
        }

        setUpdateDisplayed(display);
        // Translate channel motor number into absolute motor number.
        int gmi = getMotorNumber(cmi);
        if (gmi >= 0) {
            // Fresh measure of current position
            TuneInfo t0 = getFreshTunePosition(REMEASURE_TIME);
            MotorControl motorCtl = m_master.getMotorControl();

            List<Integer> gmiList = new ArrayList<Integer>();
            for (int motor = gmi; motor >= 0; motor = getSlaveOf(motor)) {
                gmiList.add(motor);
            }

            // Track how many steps the motor(s) actually moved:
            int prevPosition = 0;
            int newPosition = 0;

            int n = gmiList.size();
            if (n == 1) {
                // Only have one motor to move
                MotorInfo mi = motorCtl.getMotorInfo(gmi);
                prevPosition = mi.getPosition();
                newPosition = motorCtl.step(gmi, nSteps);
            } else {
                // Move both this motor and slaves
                int[] positions = new int[n];
                int[] limits = new int[n];
                for (int i = 0; i < n; i++) {
                    MotorInfo mi = motorCtl.getMotorInfo(gmiList.get(i));
                    positions[i] = mi.getPosition();
                    prevPosition += positions[i];
                    if (nSteps < 0) {
                        limits[i] = mi.getMinlimit();
                    } else {
                        limits[i] = mi.getMaxlimit();
                    }
                }
                int[] steps = calculateGroupOfSteps(nSteps, positions, limits);
                // NB: Step motors in reverse order (refresh after master moves)
                newPosition = 0;
                for (int i = n - 1; i >= 0; --i) {
                    int motor = gmiList.get(i);
                    newPosition += motorCtl.step(motor, steps[i]);
                }
            }
            // May have End-Of-Travel without getting message from motor.
            // This check gives a quicker failure when we are against the limit
            // than the one in MotorControl.processMotorMessage.
            // (If we're already at limit, no step command is sent.)
            MotorInfo mi = motorCtl.getMotorInfo(gmi);
            Messages.postDebug("AllTuneAlgorithm",
                               "prevPosition=" + prevPosition
                               + ", deltaPosition="
                               + (newPosition - prevPosition));
            if (newPosition == prevPosition) {
                mi.addEotDatum(nSteps); // Will set EOT if we accumulate enough
            } else {
                mi.setEot(0); // Not at EOT; clears the EOT step count
            }
            if (DebugOutput.isSetFor("LogDipMotion")) {
                TuneInfo t1 = getMeasuredTunePosition();
                String msg = newPosition + " " + t1.getFrequency()
                + " " + t1.getReflection();
                String hdr = "#POSITION\tFREQUENCY\tREFLECTION";
                TuneUtilities.appendLog("ptuneDipMotion" + gmi, msg, hdr);
            }
            // NB: If a motor has slaves, the recorded backlash is the amount
            // the master has to move to take up the backlash.
            // If we have one slave, only half the steps taken go to moving
            // the master motor, so divide nSteps by the number of motors.
            sameDirection = getMotorInfo(cmi).isBacklashOk(nSteps / n); // TODO: Check this
            if (sameDirection && t0 != null) {
                int actualSteps = newPosition - prevPosition;
                double absSteps = abs(actualSteps);
                double absDiff = abs(actualSteps - nSteps);
                if (absDiff < 4
                    && (absDiff / absSteps) < 0.1
                    && nSteps * actualSteps > 1)
                {
                    TuneInfo t1 = getMeasuredTunePosition();
                    updateSensitivities(cmi, actualSteps, t0, t1);
                }
            }
        }
        return sameDirection;
    }


    public void updateSensitivities(int cmi, int nSteps,
                                    TuneInfo t0, TuneInfo t1)
        throws InterruptedException {

        if (m_calibrating) {
            return;
        }
        double f0 = t0.getFrequency();
        double f1 = t1.getFrequency();
        double r0 = t0.getReflection();
        double r1 = t1.getReflection();

        // No updating if we're out of the ballpark for this channel
        // NB: !(x < y) is not equivalent to (x >= y) when x = NaN
        if (!(min(f0, f1) > m_minFreq && max(f0, f1) < m_maxFreq)) {
            Messages.postDebug("Sensitivity", "No sensitivity updating: "
                               + "Out of frequency range or no dip");
            return;
        }
        if (!(max(abs(r0), abs(r1)) <= MAX_VALID_REFLECTION)) {
            Messages.postDebug("Sensitivity", "No sensitivity updating: "
                               + "Dip reflection too high");
            return;
        }

        //final int MIN_STEPS_USED = 1;
        //final int MIN_STEPS_GOOD = 100;
        //final double MAX_WEIGHT = 0.37; // Empirically adjusted
        //final double MAX_R_WEIGHT = 0.45; // Empirically adjusted

        // The weighting for running avg of sensitivities.
        // This is the weight, w, for the old avg for a stepsize of 1.
        // For stepsize=N, w = baseWtOld**N, and:
        //  newAvg = w * oldAvg + (1 - w) * newValue
        double baseWtOld = 1 - 0.005;
        String key = "apt.sensitivitySmoothing";
        baseWtOld = TuneUtilities.getDoubleProperty(key, baseWtOld);

        // The maximum weight we allow on the new measurement:
        double maxWtNew = 0.5;
        key = "apt.sensitivitySmoothingLimit";
        maxWtNew = TuneUtilities.getDoubleProperty(key, maxWtNew);

        double absSteps = abs(nSteps);
        // wtNew is the weight for the NEW value
        double wtNew = 1 - pow(baseWtOld, absSteps);
        wtNew = min(wtNew, maxWtNew);

        // Measure frequency sensitivity
        double sensOld = m_hzPerStep[cmi];
        double df = f1 - f0;
        double measured = df / nSteps;
        updateMean(cmi, measured, wtNew,
                   m_fSensAll, m_fSdvAll,
                   m_hzPerStep, m_fSdv, m_nFreqSamples);
        m_hzPerStep[cmi] = m_freqSensLimits[cmi].clip(m_hzPerStep[cmi]);

        if (DebugOutput.isSetFor("LogSensitivity")) {
            int position = getMotorInfo(cmi).getPosition();
            int gmi = getMotorNumber(cmi);
            String msg = nSteps + " " + Fmt.f(0, sensOld, false) + " "
                    + Fmt.f(0, measured, false)
                    + " " + Fmt.f(0, m_hzPerStep[cmi], false)
                    + "  " + position
                    + "  " + Fmt.f(3, (f0 + f1) / 2e6, false) // Freq_MHz
                    //+ " df=" + df + ", nSteps=" + nSteps
                    ;
            String hdr = "#steps Hz/step(old) _(measured) _(new) position f(MHz)";
            TuneUtilities.appendLog(SENSITIVITY_LOG + "Tune" + gmi, msg, hdr);
        }

        // Measure reflection sensitivity
        // NB: Use same weight as for freq.
        //wtNew = min(MAX_R_WEIGHT * absSteps / MIN_STEPS_GOOD,
        //                 MAX_R_WEIGHT);
        sensOld = m_rPerStep[cmi];
        double dr = r1 - r0;
        measured = dr / nSteps;
        updateMean(cmi, measured, wtNew,
                   m_rSensAll, m_rSdvAll,
                   m_rPerStep, m_rSdv, m_nReflSamples);
        m_rPerStep[cmi] = m_reflSensLimits[cmi].clip(m_rPerStep[cmi]);

        if (DebugOutput.isSetFor("LogSensitivity")) {
            double refl = (r1 + r0) / 2;
            int position = (getMotorInfo(cmi).getPosition()
                    + getMotorInfo(cmi).getPrevPosition()) / 2;
            int gmi = getMotorNumber(cmi);
            String msg = nSteps + " " + Fmt.f(6, sensOld) + " "
                    + Fmt.f(6, measured) + " " + Fmt.f(6, m_rPerStep[cmi])
                    + "  " + Fmt.f(3, refl) + "  " + position;
            String hdr = "#steps R/step(old) _(measured) _(new)  refl position";
            TuneUtilities.appendLog(SENSITIVITY_LOG + "Match" + gmi, msg, hdr);
        }

        updateSensitivityDisplay(cmi);
    }

    /**
     * Average a new point into a data stream.
     * Two mean values are maintained, "avg" and "fullAvg";
     * "avg" includes only the "good" data, and "fullAvg" includes all data.
     * The new value is rejected if it is more than "m_outlierFactor"
     * standard deviations away from the current mean value,
     * but it is still averaged into "fullAvg".
     * @param cmi Which element to operate on in the input arrays.
     * @param newValue The new value to average in.
     * @param newWeight The weight to give to the new value (0 < weight < 1).
     * @param fullAvg The running mean of all data, good and bad.
     * @param fullSdv The running standard deviation of all data.
     * @param avg The running "best estimate" of the mean.
     * @param sdv The running standard deviation of the good data.
     * @param nSamples The number of values averaged so far.
     * @return True if the new value was averaged in.
     */
    protected static boolean updateMean(int cmi,
                                        double newValue, double newWeight,
                                        double[] fullAvg, double[] fullSdv,
                                        double[] avg, double[] sdv,
                                        int[] nSamples) {
        boolean rtn = false;

        if (Double.isNaN(newValue)) {
            return false;
        }
        nSamples[cmi]++;
        if (Double.isNaN(fullAvg[cmi])) {
            fullAvg[cmi] = newValue;
        }
        if (Double.isNaN(avg[cmi])) {
            avg[cmi] = newValue;
            return true;
        }

        double oldWeight = 1 - newWeight;
        double absDiff = abs(newValue - avg[cmi]);
        if (Double.isNaN(sdv[cmi])) {
            // Just getting started -- no sdv yet
            if (Double.isNaN(absDiff)) {
                // Didn't even have a valid starting value
                avg[cmi] = newValue;
                rtn = true;
            } else {
                // TODO: Don't set sdv until we get some larger # of points?
                sdv[cmi] = fullSdv[cmi] = absDiff; // Pretty rough estimate!
                sdv[cmi] = max(sdv[cmi], newValue / 10); // ...so constrain it
                avg[cmi] = oldWeight * avg[cmi] + newWeight * newValue;
                rtn = true;
            }
        } else {
            fullAvg[cmi] = oldWeight * fullAvg[cmi] + newWeight * newValue;
            fullSdv[cmi] = sqrt(oldWeight * fullSdv[cmi] * fullSdv[cmi]
                                   + newWeight * absDiff * absDiff);
            if (absDiff < m_outlierFactor * sdv[cmi]) {
                // This looks like a good measurement
                double goodAvg = oldWeight * avg[cmi] + newWeight * newValue;
                if (abs(fullAvg[cmi] - avg[cmi])
                    < abs(goodAvg - avg[cmi]))
                {
                    // We've gotten big diffs of opposite sign that cancel out
                    avg[cmi] = fullAvg[cmi];
                } else {
                    avg[cmi] = goodAvg;
                }
                sdv[cmi] = sqrt(oldWeight * sdv[cmi] * sdv[cmi]
                                     + newWeight * absDiff * absDiff);
                rtn = true;
            }
        }
        return rtn;
    }

//    public boolean goToSavedPosition(double freq, TuneInfo ti)
//        throws InterruptedException {
//
//        return goToSavedPosition(freq, ti, Double.NaN, Double.NaN, 0.0);
//    }

    /**
     * Go to the remembered position closest to the given frequency.
     * @param freq The frequency we want to go to.
     * @param ti The current tune position.
     * @param avoidFreqLow The minimum frequency to use
     * when trying to locate a previous reference position. This is
     * mainly use to avoid cutoff frequencies.
     * @param avoidFreqHigh The maximum frequency to use
     * when trying to locate a previous reference position. This is
     * mainly use to avoid cutoff frequencies.
     * @param freqTooClose Frequency range that considered to be too
     * close to the cutoff(s) and must move the dip away from it.
     * @param tol How close motors must be to the remembered position.
     * @return True if we moved to a remembered position to within
     * the specified tolerance.
     * @throws InterruptedException  if this command is aborted.
     */
    public boolean goToSavedPosition(double freq,
                                     TuneInfo ti,
                                     double avoidFreqLow,
                                     double avoidFreqHigh,
                                     double freqTooClose,
                                     int tol) throws InterruptedException {
        boolean isSys = false;
        boolean atRef = goToSavedPosition(freq,
                                                   ti,
                                                   avoidFreqLow,
                                                   avoidFreqHigh,
                                                   200e3,
                                                   tol,
                                                   isSys);
        ti = measureTune(00, true);
        if (ti == null) {
            // Go to system saved position
            atRef = goToSavedPosition(freq,
                                      ti,
                                      avoidFreqLow,
                                      avoidFreqHigh,
                                      200e3,
                                      1,
                                      isSys = true);
            measureTune(00, true);
        }
        return atRef;
    }

    /**
     * Go to the remembered position closest to the given frequency.
     * @param freq The frequency we want to go to.
     * @param ti The current tune position.
     * @param avoidFreqLow The minimum frequency to use
     * when trying to locate a previous reference position. This is
     * mainly use to avoid cutoff frequencies.
     * @param avoidFreqHigh The maximum frequency to use
     * when trying to locate a previous reference position. This is
     * mainly use to avoid cutoff frequencies.
     * @param freqTooClose Frequency range that considered to be too
     * close to the cutoff(s) and must move the dip away from it.
     * @param tol How close motors must be to the remembered position.
     * @param isSys TODO
     * @return True if we moved to a remembered position to within
     * the specified tolerance.
     * @throws InterruptedException  if this command is aborted.
     */
    public boolean goToSavedPosition(double freq,
                                     TuneInfo ti,
                                     double avoidFreqLow,
                                     double avoidFreqHigh,
                                     double freqTooClose,
                                     int tol,
                                     boolean isSys)
                                  throws InterruptedException {

        boolean rtn = false;
        TunePosition tp = getPositionAt(freq,
                                        ti,
                                        avoidFreqLow,
                                        avoidFreqHigh,
                                        freqTooClose,
                                        isSys);

        rtn = moveMotorToSavedPosition(tp, freq, tol);

        return rtn;
    }

    /**
     * Go to the remembered position closest to the given frequency.
     * @param freq The frequency we want to go to (Hz).
     * @param freqTol If freqTol > 0, only move if the ref position is within
     * "freqTol" Hz.
     * @return True if we moved to a remembered position.
     * @throws InterruptedException  if this command is aborted.
     */
    public boolean goToSavedPosition(double freq, double freqTol)
                                     throws InterruptedException {

        boolean rtn = false;
        TunePosition tp = getPositionAt(freq);
        if (tp != null) {
            if (freqTol <= 0 || abs(tp.getFreq() - freq) < freqTol) {
                int motorTol = 3;
                rtn = moveMotorToSavedPosition(tp, freq, motorTol);
            }
        }
        return rtn;
    }

    /**
     * Go to the specified position.
     * @param tp The tune position to go to.
     * @param freq The frequency we want to go to.
     * @param tol How close motors must be to the remembered position.
     * @return True if we moved to the position to within
     * the specified tolerance.
     * @throws InterruptedException  if this command is aborted.
     */
    public boolean moveMotorToSavedPosition(TunePosition tp,
                                            double freq, int tol)
        throws InterruptedException {

        boolean rtn = false;

        if (tp != null) {
            rtn = true;
            // Get the new sweep ready
            m_master.setSweepToShow(tp.getFreq(), freq);
            //m_master.setSweepToShow(freq - 5e6, freq + 5e6);
            // Go to the remembered approximate position
            for (int i = 0; i < m_motorList.length; i++) {
                Messages.postDebug("PredefinedTune",
                                   "BEFORE ChannelInfo.goToSavedPosition: "
                                   + "motor list length" + m_motorList.length
                                   + " " + i
                                   + "motor #" + getMotorNumber(i)
                                   + " to " + tp.getPosition(i)
                                   + ", hzPerStep=" + Fmt.e(3, tp.getDfDp(i))
                                   + ", rflPerStep=" + Fmt.e(3, tp.getDrDp(i))
                );
                rtn &= moveMotorToPosition(getMotorNumber(i),
                                           tp.getPosition(i), tol);
                // Update sensitivities
                //m_hzPerStep[i] = tp.getDfDp(i);
                m_hzPerStep[i] = m_freqSensLimits[i].clip(tp.getDfDp(i));

                //m_rPerStep[i] = tp.getDrDp(i);
                m_rPerStep[i] = m_reflSensLimits[i].clip(tp.getDrDp(i));

                updateSensitivityDisplay(i);
                Messages.postDebug("PredefinedTune",
                                   "ChannelInfo.goToSavedPosition: "
                                   + "motor #" + getMotorNumber(i)
                                   + " to " + tp.getPosition(i)
                                   + ", hzPerStep=" + Fmt.e(3, tp.getDfDp(i))
                                   + ", rflPerStep=" + Fmt.e(3, tp.getDrDp(i))
                );
            }
        }
        return rtn;
    }

    /**
     * Get the Global Motor Number used to control a given switch on this channel.
     * @param iSwitch The index of the switch.
     * @return The Global Motor Number.
     */
    public int getSwitchMotorNumber(int iSwitch) {
        if (iSwitch < 0 || iSwitch >= m_switchList.size()) {
            return -1;
        }
        return m_switchList.get(iSwitch).getMotor();
    }

    /**
     * Get a manual motor on this channel.
     * @param n The index into the list of manual motors.
     * @return The ManualMotor object, or null.
     */
    public ManualMotor getManualMotor(int n) {
        if (n < 0 || n >= m_manMotorList.size()) {
            return null;
        }
        return m_manMotorList.get(n);
    }

    /**
     * Get the global motor number corresponding to the i'th motor
     * on this channel.  If i is less than 0 or greater than or equal to the
     * number of channel motors, returns -1.
     * @param cmi The channel motor number.
     * @return The global motor number, or -1 if there is none.
     */
    public int getMotorNumber(int cmi) {
        int rtn = -1;
        if (m_motorList != null && cmi < m_motorList.length && cmi >= 0) {
            rtn = m_motorList[cmi];
        }
        return rtn;
    }

    /**
     * Return info for a motor associated with this channel.
     * @param cmi The Channel Motor Index, starting with 0;
     * <em>must</em> be a valid index.
     * @return The MotorInfo for the indicated motor, or <em>null</em>
     * if the requested motor number does not exist.
     * @throws InterruptedException if this command is aborted.
     */
    public MotorInfo getMotorInfo(int cmi) throws InterruptedException {
        return getGlobalMotorInfo(getMotorNumber(cmi));
    }

    public MotorInfo getGlobalMotorInfo(int gmi) throws InterruptedException {
        return m_master.getMotorInfo(gmi);
    }

    /**
     * Get the channel motor number corresponding to the i'th motor
     * in the global list.  If i >= number of motors, returns -1.
     * @param gmi The global motor number.
     * @return The channel motor number, or -1 if there is none.
     */
    public int getChannelMotorNumber(int gmi) {
        int idx = -1;
        if (m_motorList != null) {
            for (int i = 0; i < m_motorList.length; i++) {
                if (m_motorList[i] == gmi) {
                    idx = i;
                    break;
                }
            }
        }
        return idx;
    }

    /**
     * @param cmi Channel Motor Index of the motor.
     * @return Frequency sensitivity (Hz / step).
     */
    public double getHzPerStep(int cmi) {
        return m_hzPerStep[cmi];
    }

    /**
     * @param cmi Channel Motor Index of the motor.
     * @return Reflection sensitivity (reflection / step).
     */
    public double getReflectionPerStep(int cmi) {
        return m_rPerStep[cmi];
    }

    /**
     * @return The channel number of this tuning channel.
     */
    public int getChannelNumber() {
        return m_channelNumber;
    }

    /**
     * The last frequency target used on this channel.
     * @return The target frequency (MHz).
     */
    public double getTargetFreq() {
        return m_targetFreq;
    }

    /**
     * The last frequency target used on this channel.
     * @param freq The target frequency (MHz).
     */
    public void setTargetFreq(double freq) {
        m_targetFreq = freq;
    }

    /**
     * The minimum frequency we can see on this channel.
     * @return Minimum frequency in Hz;
     */
    public double getMinFreq() {
        return m_minFreq;
    }

    /**
     * The maximum frequency we can see on this channel.
     * @return Maximum frequency in Hz;
     */
    public double getMaxFreq() {
        return m_maxFreq;
    }

    /**
     * The minimum frequency we can tune to on this channel.
     * We can tune to a little beyond what we can see.
     * @return Minimum frequency in Hz;
     */
    public double getMinTuneFreq() {
        return m_minFreq - FREQ_RESOLUTION;
    }

    /**
     * The maximum frequency we can tune to on this channel.
     * We can tune to a little beyond what we can see.
     * @return Maximum frequency in Hz;
     */
    public double getMaxTuneFreq() {
        return m_maxFreq + FREQ_RESOLUTION;
    }

    /**
     * The center frequency we can tune to on this channel.
     * We can tune to a little beyond what we can see.
     * @return Center frequency in Hz;
     */
    public double getCenterTuneFreq() {
        return (getMaxTuneFreq() + getMinTuneFreq()) / 2;
    }

    /**
     * The span of frequency we can tune to on this channel.
     * We can tune to a little beyond what we can see.
     * @return Span delta frequency in Hz;
     */
    public double getSpanTuneFreq() {
        return getMaxTuneFreq() - getMinTuneFreq();
    }

    /**
     * @return The Channel Motor Index of the match motor.
     */
    public int getMatchMotor() {
        return m_matchMotor;
    }

    /**
     * Returns the Channel Motor Index (CMI) of the "match" motor for this
     * channel.
     * It there is only one motor, it is assumed to be a tune motor.
     * @return The CMI of the match motor, or -1 if none found.
     */
    public int getMatchMotorChannelIndex() {
        if (m_motorList.length < 2) {
            return -1;
        }
        return TuneUtilities.getIndexOfMaxAbsRatioR(m_rPerStep, m_hzPerStep);
    }

    public boolean isMatchMotor(int cmi) {
        return cmi == getMatchMotorChannelIndex();
    }

    /**
     * Returns the Channel Motor Index (CMI) of the "tune" motor for this
     * channel.
     * It there is only one motor, it is assumed to be a tune motor.
     * @return The CMI of the tune motor, or -1 if none found.
     */
    public int getTuneMotorChannelIndex() {
        return TuneUtilities.getIndexOfMaxAbsRatio(m_hzPerStep, m_rPerStep);
    }

    public boolean isTuneMotor(int cmi) {
        return cmi == getTuneMotorChannelIndex();
    }

    /**
     * Get the minimum frequency we should set on this channel.
     * @return The minimum legal frequency for this channel (Hz)
     */
    public double getMinSweepFreq() {
        // Used to put in a fudge factor to allow sweeping outside the
        // range for this channel. --Not good for tuning through the
        // H-F combiner, with its sharp frequency cutoffs.
        return (m_minFreq);
    }

    /**
     * Get the maximum frequency we should set on this channel.
     * @return The maximum legal frequency for this channel (Hz)
     */
    public double getMaxSweepFreq() {
        // Used to put in a fudge factor to allow sweeping outside the
        // range for this channel. --Not good for tuning near the PTS620
        // cutoff or tuning through the H-F combiner, with its sharp
        // frequency cutoffs.
        return (m_maxFreq);
    }

    /**
     * Set the tolerance on the match.
     * Units of square of the reflection coefficient.
     */
    //public void setMatchTolerance(double thresh) {
    //    m_matchTolerance = thresh;
    //}

    /**
     * Get the tolerance on the match.
     * Units of square of the reflection coefficient.
     */
    //public double getMatchTolerance() {
    //    return m_matchTolerance;
    //}

    /**
     * Find the range of motor positions that may be needed to get the
     * resonance near a specified frequency.
     * @param wantFreq The frequency we want to go to.
     * @return The TuneRange for (roughly) this frequency.
     */
    protected TuneRange getMotorRangeForFreq(double wantFreq) {
        TuneRange refRange = null;
        int len = m_refRangeList.size();
        for (int i = 0; i < len; i++) {
            TuneRange range = m_refRangeList.get(i);
            double freqDiff = abs(wantFreq - range.getFreq());
            if (refRange != null) {
                // See if this is better than previous OK refRange
                double refFreqDiff = abs(wantFreq - refRange.getFreq());
                if (freqDiff < refFreqDiff) {
                    refRange = range;
                }
            } else {
                // Is this close enought to be a refRange?
                if (freqDiff <= MAX_REF_RANGE_FREQ_DIFF) {
                    refRange = range;
                }
            }
        }
        return refRange;
    }

//    /**
//     * Find the remembered position closest to the given frequency.
//     * @param wantFreq The frequency we want to go to.
//     * @param ti The current tune position.
//     * @return The remembered tune position, or null if there is no remembered
//     * position on this channel better than the current tune position.
//     */
//    public TunePosition getPositionAt(double wantFreq, TuneInfo ti) {
//        return getPositionAt(wantFreq, ti, Double.NaN, Double.NaN, 0.0);
//    }

    /**
     * Find the remembered position closest to the given frequency.
     * Specify the frequency Double.NaN to return the first TunePosition.
     * @param wantFreq The frequency we want to go to.
     * @param ti The current tune position.
     * @param avoidFreqLow The minimum frequency to use
     * when trying to locate a previous reference position. This is
     * mainly use to avoid cutoff frequencies.
     * @param avoidFreqHigh The maximum frequency to use
     * when trying to locate a previous reference position. This is
     * mainly use to avoid cutoff frequencies.
     * @param freqTooClose Frequency range that considered to be too
     * close to the cutoff(s) and must move the dip away from it.
     * @param isSys If true, read from the system tune positions,
     * otherwise use the user tune positions.
     * @return The remembered tune position, or null if there is no
     * remembered position on this channel better than the current
     * tune position.
     */
    public TunePosition getPositionAt(double wantFreq,
                                      TuneInfo ti,
                                      double avoidFreqLow,
                                      double avoidFreqHigh,
                                      double freqTooClose,
                                      boolean isSys) {
        List<TunePosition> tuneList = isSys ? m_sysTuneList : m_tuneList;
        if (Double.isNaN(wantFreq)) {
            if (tuneList.size() > 0) {
                return tuneList.get(0);
            } else {
                return null;
            }
        }
        double curRefl = 999;   // Don't use current position
        double curFreq = 0;
        if (ti != null) {
            curRefl = abs(ti.getReflection());
            curFreq = ti.getFrequency();
        }
        double tolRefl = m_master.getTargetReflection();
        double dFreq = Double.MAX_VALUE; // Current distance from desired freq
        if (curRefl < tolRefl * 2) {
            if ( (Double.isNaN(avoidFreqLow)
                  || ((curFreq - avoidFreqLow) > freqTooClose))
                 && (Double.isNaN(avoidFreqHigh)
                     || ((avoidFreqHigh - curFreq) > freqTooClose)) )
            {
                dFreq = abs(curFreq - wantFreq);
            }
        }
        int bestIndex = -1;
        int nPosns = tuneList.size();
        double minFreq = avoidFreqLow + 1e6;
        if (!(minFreq > getMinFreq())) {
            minFreq = getMinFreq();
        }
        double maxFreq = avoidFreqHigh - 1e6;
        if (!(maxFreq < getMaxFreq())) {
            maxFreq = getMaxFreq();
        }
        for (int i = 0; i < nPosns; i++) {
            TunePosition tp = tuneList.get(i);
            double freq = tp.getFreq();
            if (freq >= minFreq && freq <= maxFreq) {
                double df = abs(freq - wantFreq);
                if (dFreq > df) {
                    dFreq = df;
                    bestIndex = i;
                    Messages.postDebug("TuneAlgorithm",
                                       "Found reference position for: "
                                       + Fmt.f(3, freq / 1e6) + " MHz");
                }
            }
        }
        if ( bestIndex < 0) {
            // Better off where we are
            return null;
        } else {
            return tuneList.get(bestIndex);
        }
    }

    /**
     * Find the remembered position closest to the given frequency.
     * Looks first in the user ref list. Only if nothing is found there
     * for this channel, looks in the system ref list.
     * @param wantFreq The frequency we want to go to.
     * @return The remembered tune position, or null if there is no
     * remembered position on this channel.
     */
    private TunePosition getPositionAt(double wantFreq) {
        List<List<TunePosition>> lists = new ArrayList<List<TunePosition>>();
        lists.add(m_tuneList);
        lists.add(m_sysTuneList);
        double dFreq = Double.MAX_VALUE;
        int bestIndex = -1;
        for (List<TunePosition> tuneList : lists) {
            int nPosns = tuneList.size();
            for (int i = 0; i < nPosns; i++) {
                double freq = tuneList.get(i).getFreq();
                double df = abs(freq - wantFreq);
                if (dFreq > df) {
                    dFreq = df;
                    bestIndex = i;
                    Messages.postDebug("TuneAlgorithm",
                                       "Found reference position for: "
                                       + Fmt.f(3, freq / 1e6) + " MHz");
                }
            }
            if (bestIndex >= 0) {
                return tuneList.get(bestIndex);
            }
        }
        return null;
    }

    /**
     * Update any motor position TuneRanges that we are tracking.
     * If we go to a new frequency that is far from any we are tracking,
     * we won't record it; only update ranges that we are already tracking.
     * More than one refRange may get updated, because we could be
     * close to more than one existing refRange frequency.
     * @param newTp The Tune Position to consider in updating the refRange.
     * @throws InterruptedException if this command is aborted.
     */
    public void updateTuneRange(TunePosition newTp)
        throws InterruptedException {

        double newFreq = newTp.getFreq();
        int nRanges = m_refRangeList.size();
        Messages.postDebug("RefRange",
                           "ChannelInfo.updateTuneRange: nRanges=" + nRanges);
        for (int i = 0; i < nRanges; i++) {
            // Update this one
            TuneRange tr = m_refRangeList.get(i);
            double freq = tr.getFreq();
            Messages.postDebug("RefRange", "range#" + i + "=" + tr);
            if (abs(newFreq - freq) < MAX_REF_RANGE_FREQ_DIFF) {
                int gmi = tr.getGmi();
                MotorInfo mInfo = m_master.getMotorInfo(gmi);
                int posn = mInfo.getPosition();
                Messages.postDebug("RefRange", "current position of motor #"
                                   + gmi + " = " + posn);
                int minPosn = tr.getMinPosition();
                if (posn < minPosn || minPosn == MOTOR_NO_POSITION) {
                    tr.setMinPosition(posn);
                }
                int maxPosn = tr.getMaxPosition();
                if (posn > maxPosn || maxPosn == MOTOR_NO_POSITION) {
                    tr.setMaxPosition(posn);
                }
            }
        }
    }

    public void saveTunePosition(TunePosition newTp) {
        double newFreq = newTp.getFreq();
        int nPosns = m_tuneList.size();
        // Go through list backwards so we can remove stuff
        for (int i = nPosns - 1; i >= 0; --i) {
            TunePosition tp = m_tuneList.get(i);
            double freq = tp.getFreq();
            if (abs(freq - newFreq)  < TUNE_POSITION_TOL_HZ) {
                // TODO: Update other tune positions??
                m_tuneList.remove(i);
            }
        }
        m_tuneList.add(0, newTp); // Add to the front of the list
    }

    /**
     * Saves/updates a Ref position and, if applicable,
     * updates RefRange entries.
     * @throws InterruptedException if this command is aborted.
     */
    // TODO: No longer used????
    public void saveCurrentTunePosition() throws InterruptedException {
        Messages.postDebug("PredefinedTune", "ChannelInfo: saving current tune position ");
        TuneInfo ti = getMeasuredTunePosition();
        double freq = ti.getFrequency();
        // TODO: Check if freq is in or near this channel?
        // TODO: Avoid writing invalid (NaN) entries?
        if (!Double.isNaN(freq)) {
            MotorControl motorCtl = m_master.getMotorControl();
            int nMotors = m_motorList.length;
            int[] p = new int[nMotors];
            double[] dfdp = new double[nMotors];
            double[] drdp = new double[nMotors];
            for (int cmi = 0; cmi < nMotors; cmi++) {
                MotorInfo mi = getMotorInfo(cmi);
                p[cmi] = mi.getPosition();
                if(p[cmi] == 0){
                    if(mi.getFlag()==0){
                        if(motorCtl.readFlag(getMotorNumber(cmi))!=0){
                            //read position from motor for p[i]
                            p[cmi]= motorCtl.readPosition(getMotorNumber(cmi));
                        }
                    }
                }
                if (!mi.isSameDirection(1)) {
                    // Got here going in minus direction; add backlash estimate
                    // NB: Use mean backlash for motors in slave group
                    p[cmi] += getMeanBacklash(cmi);
                }
                dfdp[cmi] = m_hzPerStep[cmi];
                drdp[cmi] = m_rPerStep[cmi];
            }
            TunePosition tp = new TunePosition(freq, ti.getReflection(),
                                               p, dfdp, drdp);
            saveTunePosition(tp);
            updateTuneRange(tp);
        }
    }


    public void savePredefinedTunePosition(TunePosition newTp) {
        //double newFreq = newTp.getFreq();
        int nPosns = m_predefinedTuneList.size();
        // Go through list backwards so we can remove stuff
        for (int i = nPosns - 1; i >= 0; --i) {
            TunePosition tp = m_predefinedTuneList.get(i);
            String sampleName = tp.getSampleName();
            if (sampleName.equals(newTp.getSampleName())) {
                // TODO: Update other tune positions
                m_predefinedTuneList.remove(i);
            }
        }
        m_predefinedTuneList.add(0, newTp); // Add to the front of the list
    }

    /**
     * Saves/updates a tune position
     * @param sampleName The name to assign to the position.
     * @throws InterruptedException if this command is aborted.
     */
    public void saveCurrentTunePosition(String sampleName)
        throws InterruptedException {

        TuneInfo ti = getMeasuredTunePosition();
        double freq = ti.getFrequency();
        // TODO: Check if freq is in or near this channel?
        // TODO: Avoid writing invalid (NaN) entries?
        if (!Double.isNaN(freq)) {
            MotorControl motorCtl = m_master.getMotorControl();
            int nMotors = m_motorList.length;
            int[] p = new int[nMotors];
            double[] dfdp = new double[nMotors];
            double[] drdp = new double[nMotors];
            for (int cmi = 0; cmi < nMotors; cmi++) {
                MotorInfo mi = getMotorInfo(cmi);
                p[cmi] = mi.getPosition();
                if(p[cmi] == 0){
                    if(mi.getFlag()==0){
                        if(motorCtl.readFlag(getMotorNumber(cmi))!=0){
                            //read position from motor for p[i]
                            p[cmi]= motorCtl.readPosition(getMotorNumber(cmi));
                        }
                    }
                }
                if (!mi.isSameDirection(1)) {
                    // Got here going in minus direction; add backlash estimate
                    // NB: Use mean backlash for motors in slave group
                    p[cmi] += getMeanBacklash(cmi);
                }
                dfdp[cmi] = m_hzPerStep[cmi];
                drdp[cmi] = m_rPerStep[cmi];
            }
            if(sampleName==null){
                Messages.postDebug("TuneAlgorithm",
                                   "ChannelInfo.saveCurrentTunePosition");
                TunePosition tp = new TunePosition(freq, ti.getReflection(),
                                                   p, dfdp, drdp);
                saveTunePosition(tp);
                updateTuneRange(tp);
            } else {
                Messages.postDebug("PredefinedTune",
                                   "SavePredefinedTune: " + sampleName);

                TunePosition tp = new TunePosition(sampleName, freq, ti.getReflection(),
                                                   p[0], dfdp[0], drdp[0], p[1], dfdp[1], drdp[1]);
                savePredefinedTunePosition(tp);
            }


        }
    }

    /**
     * Find the position at the predefined tuning point with sampleName.
     * @param sampleName Text name of sample in persistence file
     * @return The Tune Position for the given name.
     */
    public TunePosition getPositionAt(String sampleName) {

        Messages.postDebug("PredefinedTune",
                           "Getting position at: " + sampleName);
        Messages.postDebug("PredefinedTune",
                           "Predefined list length: "
                           + m_predefinedTuneList.size());

        if(m_predefinedTuneList.size() <= 0){
            Messages.postDebug("PredefinedTune",
                               "Predefined list length not valid.");
            return null;
        }

        for (TunePosition tp : m_predefinedTuneList) {
            if (tp.m_sampleName.equalsIgnoreCase(sampleName)) {
                return tp;
            }
        }
        return null;
    }

    /**
     * Make a map of the motors used by all the ChannelInfo objects.
     * The map can take a gmi and return the index of the channel that uses the
     * motor.
     *
     * @param master  The ProbeTune instance.
     * @throws InterruptedException If this command is aborted.
     */
    public static void initializeMotorToChannelMap(ProbeTune master)
        throws InterruptedException {

        sm_motorMap = new HashMap<Integer, Integer>();
        List<ChannelInfo> chanInfos = master.getChannelInfos();
        for (ChannelInfo ci : chanInfos) {
            int iChan = ci.getChannelNumber();
            // Put in the tuning motors
            for (int cmi = 0; true; cmi++) {
                int gmi = ci.getMotorNumber(cmi);
                if (gmi >= 0) {
                    sm_motorMap.put(gmi, iChan);
                } else {
                    break;
                }
            }
            // Put in the switch motors
            for (int iSwitch = 0; true; iSwitch++) {
                int gmi = ci.getSwitchMotorNumber(iSwitch);
                if (gmi >= 0) {
                    sm_motorMap.put(gmi, iChan);
                } else {
                    break;
                }
            }
            // Put in the slave motors
            for (int iMaster = 0;
            iMaster < MotorControl.MAX_MOTORS;
            iMaster++)
            {
                int gmi = ci.getSlaveOf(iMaster);
                if (gmi >= 0) {
                    sm_motorMap.put(gmi, iChan);
                }
            }
            // Put in the manual motors
            for (int n = 0; true; n++) {
                ManualMotor manMtr = ci.getManualMotor(n);
                if (manMtr != null) {
                    sm_motorMap.put(manMtr.getGmi(), iChan);
                } else {
                    break;
                }
            }
        }
    }

    /**
     * Determine which channel uses a given motor.
     * @param master The ProbeTune instance.
     * @param gmi The Global Motor Index of the motor we're interested in.
     * @return The index of the channel that uses the motor, or -1 if it is not used.
     * @throws InterruptedException if this command is aborted.
     */
    public static int getChannelForMotor(ProbeTune master, int gmi)
        throws InterruptedException {

        if (sm_motorMap == null) {
            initializeMotorToChannelMap(master);
        }
        Integer channel = sm_motorMap.get(gmi);
        return (channel == null) ? -1 : channel.intValue();
    }

    /**
     * Get the number of any slave motor to a given motor.
     * @param gmi The global motor index of the master motor.
     * @return The global motor index of the slave, or -1 if there is no slave.
     */
    public int getSlaveOf(int gmi) {
        return m_slaveList.get(gmi);
    }

    /**
     * Move a given motor -- and it's slave motor, if any --
     * to a given position.
     * @param gmi The global motor index of the motor to move.
     * @param dst The destination position of the motor.
     * @param tol The tolerance in the position. (degrees)
     * @return True if the motor(s) is/are within tol of the desired position.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean moveMotorToPosition(int gmi, int dst, int tol)
        throws InterruptedException {

        Messages.postDebug("MoveMotorToPosition",
                           "ChannelInfo.MoveMotorToPosition("
                           + "gmi=" + gmi + ", dst=" + dst
                           + "tol=" + tol + ")");
        boolean ok = true;
        MotorControl motorCtl = m_master.getMotorControl();
        for ( ; gmi >= 0; gmi = getSlaveOf(gmi)) {
            ok &= motorCtl.moveMotorToPosition(gmi, dst, tol);
        }
        return ok;
    }

    /**
     * Tries to find a dip (resonance) in this channel by moving the tune motor.
     * Calls searchForDipInRefRange() and searchForDipInChannel().
     * @param target The target we're trying to tune to.
     *
     * @return True if the dip is already present or successfully found.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean searchForDip(TuneTarget target) throws InterruptedException {
        //boolean gotDip = isDipInChannelRange();
        boolean gotDip = isDipDetected();
        // Note that searchFor... isn't called if gotDip is already true
        gotDip = gotDip || searchForDipInRefRange(target);
        gotDip = gotDip || searchForDipInChannel(target);
        return gotDip;
    }

    /**
     * Searches over the RefRange (from the channel file) to try to move the
     * dip into the range of this channel. Moves only the motor specified in
     * the RefRange line.
     * Does nothing if there is no RefRange for the target frequency
     * (within 1 MHz).
     * @param target The target we're trying to tune to.
     *
     * @return True if the dip is already present or successfully found.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean searchForDipInRefRange(TuneTarget target)
        throws InterruptedException {

        final double MIN_FREQ_STEP = 1e6;

        if (!isDipDetected()) {
            // Don't have a visible dip yet
            double targetFreq = target.getFreq_hz();
            // Use relevant RefRange from channel file
            TuneRange refRange = getMotorRangeForFreq(targetFreq);
            int gmi = -1;
            int cmi = -1;
            if (refRange != null
                && (gmi = refRange.getGmi()) >= 0
                && (cmi = getChannelMotorNumber(gmi)) >= 0
                && refRange.getMinPosition() != MOTOR_NO_POSITION
                && refRange.getMaxPosition() != MOTOR_NO_POSITION)
            {
                // We have a refRange to work with
                // Remember starting motor position
                MotorInfo mInfo = m_master.getMotorInfo(gmi);
                int initialPosition = mInfo.getPosition();

                //double refFreq = refRange.getFreq();
                // Set |DS| = 1/2 dist from target to nearest channel edge
                double dfreq = abs(getHzToChannelEdge(targetFreq)) / 2;
                if (dfreq < MIN_FREQ_STEP) {
                    dfreq = MIN_FREQ_STEP;
                }
                double absHzPerStep = abs(getHzPerStep(cmi));
                int dstep = (int)round(dfreq / absHzPerStep);

                // First search in direction of position farthest from
                // our current position.
                // Put limits of search 10 MHz beyond previously seen limits.
                int maxPosition = refRange.getMaxPosition();
                int minPosition = refRange.getMinPosition();
                int limit1 = maxPosition + (int)(10e6 / absHzPerStep);
                int limit2 = minPosition - (int)(10e6 / absHzPerStep);
                if (initialPosition - minPosition
                    > maxPosition - initialPosition)
                {
                    // We'll search in negative motor direction first
                    dstep = -dstep;
                    int tmp = limit1;
                    limit1 = limit2;
                    limit2 = tmp;
                }

                // Move DS steps, up to first limit, or until dip seen
                // NB: dstep could be 0 in pathological case?
                while (!isDipDetected()
                       && dstep * (mInfo.getPosition() - limit1) < 0)
                {
                    throwExceptionOnCancel();
                    step(cmi, dstep);
                }

                if (!isDipDetected()) { // If still no dip
                    // Return to starting motor position
                    moveMotorToPosition(gmi, initialPosition, 10);
                    dstep = -dstep;
                    // Move -DS steps, up to second limit, or until dip seen
                    while (!isDipDetected()
                           && dstep * (mInfo.getPosition() - limit2) < 0)
                    {
                        throwExceptionOnCancel();
                        step(cmi, dstep);
                    }
                }
                if (!isDipDetected()) { // If still no dip
                    // Return to starting motor position
                    moveMotorToPosition(gmi, initialPosition, 10);
                }
            }
        }
        boolean foundDip = isDipDetected();
        if (!foundDip) {
            Messages.postDebug("TuneAlgorithm", "Dip not found in ref range");
        }
        return foundDip;
    }

    /**
     * @throws InterruptedException If the cancel flag has been set.
     */
    private void throwExceptionOnCancel() throws InterruptedException {
        if (ProbeTune.isCancel()) {
            throw new InterruptedException("Aborted");
        }
    }

    /**
     * Searches over the range of the tune motor to try to move the
     * dip into the range of this channel.
     * Moves only the tune motor (as implied by the channel file
     * tune vs. match sensitivities).
     * @param target The target we're trying to tune to.
     *
     * @return True if the dip is already present or successfully found.
     * @throws InterruptedException if this command is aborted.
     */
    public boolean searchForDipInChannel(TuneTarget target)
        throws InterruptedException {

        final double MIN_FREQ_STEP = 1e6;

        if (!isDipDetected()) {
            int cmi = getTuneMotorChannelIndex(); // Move the tune motor
            MotorInfo mInfo = getMotorInfo(cmi);
            int initialMotorPosition = mInfo.getPosition();

            // Set |DS| = 1/2 dist from target to nearest channel edge
            // Set direction to go FROM channel edge nearest to target
            // TODO: Guess direction of dip from baseline slope
            double targetFreq = target.getFreq_hz();
            double dfreq = -getHzToChannelEdge(targetFreq) / 2;
            if (abs(dfreq) < MIN_FREQ_STEP) {
                dfreq = (dfreq < 0) ? -MIN_FREQ_STEP : MIN_FREQ_STEP;
            }
            int dstep = (int)round(dfreq / getHzPerStep(cmi));

            // Move DS steps, up to EOT, or until dip seen
            int gmi = getMotorNumber(cmi); // Global Motor Index
            while (!isDipDetected() && !mInfo.isAtLimit(dstep)) {
                throwExceptionOnCancel();
                step(cmi, dstep);
            }

            if (!isDipDetected()) { // If still no dip
                // Return to starting motor position
                moveMotorToPosition(gmi, initialMotorPosition, 10);
                dstep = -dstep;
                // Move -DS steps, up to EOT, or until dip seen
                while (!isDipDetected() && !mInfo.isAtLimit(dstep)) {
                    throwExceptionOnCancel();
                    step(cmi, dstep);
                }
            }

            if (!isDipDetected()) { // If still no dip
                // Return to starting motor position
                moveMotorToPosition(gmi, initialMotorPosition, 10);
            }
        }
        boolean foundDip = isDipDetected();
        if (!foundDip) {
            Messages.postDebug("TuneAlgorithm",
                               "Dip not found in channel range");
        }
        return foundDip;
    }

    /**
     * Returns the distance (in Hz) from a given valid tune frequency
     * to the nearest edge of this channel.
     * The sign of the answer implies which edge is nearer.
     * In order too avoid ambiguity when the given frequency is exactly on
     * the edge, the absolute value of the answer is incremented by 1 Hz.
     * Thefore, the answer is never 0 for a frequency that is a valid
     * value to tune to.
     * (However, the input frequency is not validated.)
     * @return Signed distance-to-nearest-tuning-limit-plus-one (Hz).
     */
    protected double getHzToChannelEdge(double freq) {
        double d1 = getMinTuneFreq() - 1 - freq;
        double d2 = getMaxTuneFreq() + 1 - freq;
        return (abs(d1) < abs(d2)) ? d1 : d2;
    }

    /**
     * Get the minimum step that should be used for sensitivity calibration.
     * A smaller number would give too much uncertainty in the actual
     * amount of capacitor motion.
     * If this motor has slaves, the minimum step is large enough to make
     * all the motors move (roughly) their minimum cal step.
     * @param cmi The Channel Motor Index of the motor to check.
     * @return The minimum step size.
     * @throws InterruptedException if this command is aborted.
     */
    public int getMinCalStep(int cmi) throws InterruptedException {
        int minStep = 0;
        int gmi = getMotorNumber(cmi);
        if (gmi >= 0) {
            for (int motor = gmi; motor >= 0; motor = getSlaveOf(motor)) {
                minStep += getGlobalMotorInfo(motor).getMinCalStep();
            }
        }
        return minStep;
    }

    /**
     * If we take a given step, will the dip still be visible and have
     * a reasonable match. If the current dip is already outside the good
     * range in frequency or match, we just check if it will get better.
     * @param cmi The Channel Motor Index of the motor we want to move.
     * @param stepSize The proposed step.
     * @return True if we expect a good dip after taking the proposed step.
     */
    public boolean isPredictedPositionInRange(int cmi, int stepSize) {
        boolean ok = true;
        Limits freqLimits = new Limits(getMinSweepFreq(), getMaxSweepFreq());
        Limits reflLimits = new Limits(-0.5, 0.5);
        TuneInfo predictTune = predictTune(m_estimatedTune, cmi, stepSize);
        ok &= freqLimits.contains(predictTune.getFrequency());
        ok &= reflLimits.contains(predictTune.getReflection());
        return ok;
    }

    /**
     * Gets the sum of the backlashes for a motor and all it's slaves.
     * Uses the value measured and stored for this channel if it exists.
     * Otherwise, sums the backlashes of all the motors in this motor group.
     * This is the sum of the differences in position for all motors in the
     * group when a cap position is approached from above vs. below.
     * @param cmi The Channel Motor Index of the (master) motor.
     * @return The mean backlash in steps.
     * @throws InterruptedException if this command is aborted.
     */
    public int getTotalBacklash(int cmi) throws InterruptedException {
        int backlash = 0;
        try {
            Integer val = m_backlashList.get(cmi);
            if (val != null) {
                backlash = val;
            }
        } catch (Exception e) {}
        if (backlash == 0) {
            for (int gmi=getMotorNumber(cmi); gmi >= 0; gmi=getSlaveOf(gmi)) {
                backlash += getGlobalMotorInfo(gmi).getBacklash();
            }
        }
        return backlash;
    }

    /**
     * Gets the mean backlash for a motor or motor group.
     * This is the difference in position for <i>each motor</i>
     * when a cap position is approached from above vs. below.
     * @param cmi The Channel Motor Index of the motor.
     * @return The mean backlash in steps.
     * @throws InterruptedException if this command is aborted.
     */
    public int getMeanBacklash(int cmi) throws InterruptedException {
        return getTotalBacklash(cmi) / getSizeOfMotorGroup(cmi);
    }

    public int getTotalGraylash(int cmi) throws InterruptedException {
        return getTotalBacklash(cmi) / 2;
    }

    public int getMaxBacklash(int cmi) {
        return MotorInfo.MAX_BACKLASH * getSizeOfMotorGroup(cmi);
    }

    /**
     * Get the size of the group this motor belongs to. The motor group is
     * this motor, its slave, its slave's motor, etc.
     * @param cmi The Channel Motor Index of the motor to check.
     * @return The number of motors in the group.
     */
    private int getSizeOfMotorGroup(int cmi) {
        int gmi = getMotorNumber(cmi);
        int n = 0;
        for (int motor = gmi; motor >= 0; motor = getSlaveOf(motor)) {
            n++;
        }
        return n;
    }

    public static boolean isAutoUpdateOk() {
        return UPDATING_ON.equalsIgnoreCase(m_channelFileUpdating);
    }



    /**
     * Container class for holding the range of positions that a motor may
     * need to have to tune to a given frequency.
     */
    public class TuneRange {
        private double m_frequency;
        private int m_gmi;
        private int[] m_positions = new int[2];

        public TuneRange(double freq, int gmi, int minPosn, int maxPosn) {
            m_frequency = freq;
            m_gmi = gmi;
            m_positions[0] = minPosn;
            m_positions[1] = maxPosn;
        }

        public double getFreq() {
            return m_frequency;
        }

        public int getGmi() {
            return m_gmi;
        }

        public int getMinPosition() {
            return m_positions[0];
        }

        public void setMinPosition(int position) {
            Messages.postDebug("RefRange",
                               "Set min motor #" + m_gmi + " position="
                               + position);
            m_positions[0] = position;
        }

        public int getMaxPosition() {
            return m_positions[1];
        }

        public void setMaxPosition(int position) {
            Messages.postDebug("RefRange",
                               "Set max motor #" + m_gmi + " position="
                               + position);
            m_positions[1] = position;
        }

        public String toString() {
            return "RefRange " + m_frequency + " " + m_gmi + " "
                    + m_positions[0] + " " + m_positions[1];
        }
    }


    /**
     * Container class for holding motor positions to tune to a given
     * frequency, along with the tune & match sensitivities at that
     * position.
     */
    static public class TunePosition {
        private double m_frequency;
        private double m_reflection;
        private int[] m_position;
        private double[] m_dfdp;
        private double[] m_drdp;
        private String m_sampleName;

        public TunePosition(double freq, double refl,
                            int[] p, double[] dfdp, double[] drdp) {
            int len = min(p.length, min(dfdp.length, drdp.length));
            m_frequency = freq;
            m_reflection = refl;
            m_position = new int[len];
            m_dfdp = new double[len];
            m_drdp = new double[len];
            for (int i = 0; i < len; i++) {
                m_position[i] = p[i];
                m_dfdp[i] = dfdp[i];
                m_drdp[i] = drdp[i];
            }
        }

        public TunePosition(double freq, double refl,
                            int p0, double dfdp0, double drdp0,
                            int p1, double dfdp1, double drdp1) {
            m_frequency = freq;
            m_reflection = refl;
            m_position = new int[2];
            m_position[0] = p0;
            m_position[1] = p1;
            m_dfdp = new double[2];
            m_dfdp[0] = dfdp0;
            m_dfdp[1] = dfdp1;
            m_drdp = new double[2];
            m_drdp[0] = drdp0;
            m_drdp[1] = drdp1;
        }

        public TunePosition(String sampleName, double freq, double refl,
                        int p0, double dfdp0, double drdp0,
                        int p1, double dfdp1, double drdp1) {
                m_sampleName= sampleName;
                m_frequency = freq;
                m_reflection = refl;
                m_position = new int[2];
                m_position[0] = p0;
                m_position[1] = p1;
                m_dfdp = new double[2];
                m_dfdp[0] = dfdp0;
                m_dfdp[1] = dfdp1;
                m_drdp = new double[2];
                m_drdp[0] = drdp0;
                m_drdp[1] = drdp1;
        }


        public double getFreq() {
            return m_frequency;
        }

        public int getNumberOfMotors() {
            // FIXME: Always length = 2 from persistence file, even if 1 motor
            return min(m_position.length, min(m_dfdp.length,
                                                        m_drdp.length));
        }

        public double getRefl() {
            return m_reflection;
        }

        public int getPosition(int cmi) {
            return m_position[cmi];
        }

        public double getDfDp(int cmi) {
            return m_dfdp[cmi];
        }

        public double getDrDp(int cmi) {
            return m_drdp[cmi];
        }

        public String getSampleName(){
            return m_sampleName;
        }

        public void setDfDp(int cmi, double value) {
            m_dfdp[cmi] = value;
        }
    }


    /**
     * This class holds a "Manual Motor" specification, which includes
     * the motor number and a label.
     */
    static public class ManualMotor {
        private int mm_gmi;
        private String mm_label;

        public ManualMotor(int gmi, String label) {
            mm_gmi = gmi;
            mm_label = label;
        }

        public int getGmi() {
            return mm_gmi;
        }

        public String getLabel() {
            return mm_label;
        }

        /**
         * Two ManualMotors are equal if they have the same motor number.
         */
        public boolean equals(Object obj) {
            return obj != null
                    && obj instanceof ManualMotor
                    && mm_gmi == ((ManualMotor) obj).getGmi();
        }
    }


    /**
     * This class holds a switch position, which may depend on frequency.
     */
    static public class SwitchPosition {
        private int mm_motor;
        private double[] mm_coeffs;
        /** The tolerance in the position of the switch motor. */
        private int mm_tolerance = 10; // degrees


        /**
         * Constructor for a SwitchPosition for either a fixed or a
         * frequency dependent switch.
         * A fixed switch has a coefficient array of length one.
         * @param motor The Global Motor Index (gmi) of the motor controlling
         * this switch.
         * @param coeffs The coefficients of the polynomial specifying the
         * position of the motor vs. frequency (in MHz).
         */
        public SwitchPosition(int motor, double[] coeffs) {
            mm_motor = motor;
            mm_coeffs = coeffs;
        }

        /**
         * Get the motor number for this switch.
         * @return The global motor index (GMI).
         */
        public int getMotor() {
            return mm_motor;
        }

        public int getTolerance() {
            return mm_tolerance;
        }

        public int getPosition() {
            return (int)round(mm_coeffs[0]);
        }

        public int getPosition(double freq) {
            freq *= 1e-6;           // Put freq in MHz
            int n = mm_coeffs.length;
            double posn = mm_coeffs[--n];
            while (n > 0) {
                posn = posn * freq + mm_coeffs[--n];
            }
            return (int)round(posn);
        }

        public boolean isFixedPosition() {
            return (mm_coeffs.length == 1);
        }

        public boolean isFrequencyDependent() {
            return (mm_coeffs.length > 1);
        }

        public String toString() {
            StringBuffer sb = new StringBuffer();
            sb.append(mm_motor);
            int n = mm_coeffs.length;
            for (int i = 0; i < n; i++) {
                sb.append(" ");
                sb.append(mm_coeffs[i]);
            }
            return sb.toString();
        }
    }


    /**
     * This class holds the minimum and maximum values that some double
     * is allowed to have.
     * Either or both limits may be NaN, which is treated as no restriction.
     * Has a method to clip a value to satisfy the limits.
     */
    static public class Limits {
        private double mm_min;
        private double mm_max;

        /**
         * Create a new Limits object with the given limits.
         * @param a Either min or max limit.
         * @param b The other limit.
         */
        public Limits(double a, double b) {
            init(a, b);
        }

        public double getMax() {
            return mm_max;
        }

        public double getMin() {
            return mm_min;
        }

        /**
         * Create a new Limits object with the limits specified as Strings.
         * @param limit1 Either min or max limit.
         * @param limit2 The other limit.
         */
        public Limits(String limit1, String limit2) {
            double a;
            double b;
            try {
                a = Double.parseDouble(limit1);
            } catch (NumberFormatException nfe) {
                a = Double.NaN;
            }
            try {
                b = Double.parseDouble(limit2);
            } catch (NumberFormatException nfe) {
                b = Double.NaN;
            }
            init(a, b);
        }

        /**
         * Initialize the limit settings.
         * If either argument is NaN, the first is considered the lower limit,
         * otherwise the smaller of the two is the lower limit.
         * @param a Either min or max limit.
         * @param b The other limit.
         */
        private void init(double a, double b) {
            if (Double.isNaN(a) || Double.isNaN(b)) {
                mm_min = a;
                mm_max = b;
            } else {
                mm_min = min(a, b);
                mm_max = max(a, b);
            }
        }

        /**
         * See if a given value is within the limits.
         * A limit that is NaN is treated as no limit at all.
         * @param value The value to check.
         * @return True if the value is within the limits.
         */
        public boolean contains(double value) {
            return !(value < mm_min) && !(value > mm_max);
        }

        /**
         * See if these limits have been set.
         * @return True if both min and max are set, otherwise false.
         */
        public boolean isSet() {
            return (!Double.isNaN(mm_min) && !Double.isNaN(mm_max));
        }

        /**
         * Clip the given value so that it is not outside the limits.
         * A limit of NaN is no limit at all.
         * @param value The value to clip.
         * @return The clipped value.
         */
        public double clip(double value) {
            if (mm_min > value) {
                return mm_min;
            } else if (mm_max < value) {
                return mm_max;
            } else {
                return value;
            }
        }

        public String toString(int digits, String format) {
            if ("f".equals(format)) {
                return Fmt.f(digits, mm_min) + " to " + Fmt.f(digits, mm_max);
            }
            return Fmt.g(digits, mm_min) + " to " + Fmt.g(digits, mm_max);
        }

        /**
         * Return a string representing these limits, in for format "min:max".
         * If the limits are integral, the value strings will be parsable as
         * integers.
         * @return The string for these limits.
         */
        public String toString() {
            if ((int)mm_min == mm_min && (int)mm_max == mm_max) {
                return Fmt.f(0, mm_min, false) + ":" + Fmt.f(0, mm_max, false);
            } else {
                return mm_min + ":" + mm_max;
            }
        }
    }



    class SensitivityData {
        private static final int MIN_MEASURES = 5;

        private double mm_dipFreqSDV;
        private double mm_dipReflSDV;
        private List<Double> mm_freqSensList = new ArrayList<Double>();
        private List<Double> mm_reflSensList = new ArrayList<Double>();
        private List<Integer> mm_stepList = new ArrayList<Integer>();
        private DipPositionData mm_positionListPlus = new DipPositionData();
        private DipPositionData mm_positionListMinus = new DipPositionData();
        private double mm_freqSens = Double.NaN;
        private double mm_reflSens = Double.NaN;
//        private double mm_freqSensSDV = Double.NaN;
//        private double mm_reflSensSDV = Double.NaN;
//        private int mm_freqPtsUsed = 0;
//        private int mm_reflPtsUsed = 0;
        private boolean mm_outOfDate = false;
        private boolean mm_isMeasured = false;

//        private boolean mm_isDeletionAllowed = true;


        public SensitivityData(double[] dipSDV) {
            mm_dipFreqSDV = dipSDV[0];
            mm_dipReflSDV = dipSDV[1];
        }

        public void add(SensitivityData sens2) {
            mm_freqSensList.addAll(sens2.mm_freqSensList);
            mm_reflSensList.addAll(sens2.mm_reflSensList);
            mm_stepList.addAll(sens2.mm_stepList);
            mm_positionListPlus.addAll(sens2.mm_positionListPlus);
            mm_positionListMinus.addAll(sens2.mm_positionListMinus);
            mm_outOfDate = true;
        }

//        public void setDeletionAllowed(boolean b) {
//            mm_isDeletionAllowed = b;
//        }

        public double getMeanFreq() {
            updateCalculation();
            double sum = 0;
            for (DipPositionDatum datum : mm_positionListPlus) {
                sum += datum.freq;
            }
            return sum / mm_positionListPlus.size();
        }

        public double getMeanRefl() {
            updateCalculation();
            double sum = 0;
            for (DipPositionDatum datum : mm_positionListPlus) {
                sum += datum.refl;
            }
            return sum / mm_positionListPlus.size();
        }

        private int size() {
            // NB: Class is responsible for keeping all lists the same size
            return mm_stepList.size();
        }

        private void updateCalculation() {
            if (mm_outOfDate) {
                //mm_isMeasured = calculateSensitivities();
                mm_isMeasured = calculateLinearSensitivities();
                mm_outOfDate = false;
            }
        }

//        /**
//         * Remove points from the beginning of the given list.
//         * @param nTrailing The number of points to leave in the list.
//         * @param list The list.
//         */
//        private void trimTo(int nTrailing, DipPositionData list) {
//            int npts = list.size() - nTrailing;
//            for (int i = 0; i < npts; i++) {
//                list.remove(0);
//            }
//        }

//        private int deleteLeadingLowPoints(List<Double> data, boolean debug) {
//            if (!mm_isDeletionAllowed ) {
//                return 0;
//            }
//            int nDeleted = 0;
//            int size = data.size();
//            if (size >= MIN_MEASURES) {
//                // Get the average of all the trailing points
//                double mean = 0; // Mean value of the trailing points
//                int n = 0;
//                for (int i = size - 1; i >= 0; --i) {
//                    n++;
//                    mean = (mean * (n - 1) + data.get(i)) / n;
//                    if (n >= 3 && i > 0) {
//                        // Get Standard Deviation for these trailing points
//                        double sdv = 0;
//                        for (int j = size - 1; j >= i; --j) {
//                            double diff = mean - data.get(j);
//                            sdv += diff * diff;
//                        }
//                        sdv = sqrt(sdv / n - 1);
//
//                        // See if the next point looks low
//                        double nextPoint = data.get(i - 1);
//                        double diff = nextPoint - mean;
//                        boolean isLower = abs(nextPoint) < abs(mean);
//                        if (isLower && abs(diff) > 3 * sdv) {
//                            // Point i-1 looks low
//                            // How many leading low points are there?
//                            for (int j = 0; j < i; j++) {
//                                double datum = data.get(i - 1);
//                                double delta = datum - mean;
//                                boolean isLow = abs(datum) < abs(mean);
//                                if (isLow && abs(delta) > 3 * sdv) {
//                                    nDeleted++;
//                                } else {
//                                    break;
//                                }
//                            }
//                            break;
//                        }
//                    }
//                }
//                // Delete all low points from front
//                if (nDeleted > 0 && debug) {
//                    List<Double> dels = data.subList(0, nDeleted);
//                    Messages.postDebug("Sensitivity", "Deleted: " + dels);
//                }
//                for (int j = 0; j < nDeleted; j++) {
//                    data.remove(0);
//                }
//            }
//            return nDeleted;
//        }

//        /**
//         * Calculate linear fits of x=freq,y=position and
//         * x=reflection,y=position.
//         * Must be called after <code>calculateSensitivities()</code>,
//         * as that removes the bad points from the data.
//         */
//        private void calculateLinFits() {
//            int n = mm_positionList.size();
//            double[] positions = new double[n];
//            double[] freqs = new double[n];
//            double[] refls = new double[n];
//            for (int i = 0; i < n; i++) {
//                DipPositionDatum datum = mm_positionList.get(i);
//                positions[i] = datum.position;
//                freqs[i] = datum.freq;
//                refls[i] = datum.refl;
//            }
//            mm_posnVsFreq = TuneUtilities.polyFit(freqs, positions, 1);
//            mm_posnVsRefl = TuneUtilities.polyFit(refls, positions, 1);
//        }

        /**
         * Write this sensitivity data to a log file.
         * @param cmi The channel motor index this data belongs to.
         */
        private void dumpSensitivityData(int cmi) {
            StringBuffer sb;
            sb = new StringBuffer("#POSITION\tFREQUENCY\tREFLECTION\n");
            for (DipPositionDatum datum : mm_positionListPlus) {
                sb.append(datum.position).append(" ");
                sb.append(datum.freq).append(" ");
                sb.append(datum.refl).append("\n");
            }
            TuneUtilities.writeFile(SENSITIVITY_LOG + cmi + "Plus",
                                    sb.toString());

            sb = new StringBuffer("#POSITION\tFREQUENCY\tREFLECTION\n");
            for (DipPositionDatum datum : mm_positionListMinus) {
                sb.append(datum.position).append(" ");
                sb.append(datum.freq).append(" ");
                sb.append(datum.refl).append("\n");
            }
            TuneUtilities.writeFile(SENSITIVITY_LOG + cmi + "Minus",
                                    sb.toString());
        }

        /**
         * Calculate the mean and standard deviation of the position
         * for a given frequency when moving in the plus direction.
         * All the measured (frequency,position) pairs are considered.
         * With position considered as the dependent (y) variable,
         * each position is regressed to the value it would have at
         * the given frequency, assuming the given sensitivity.
         * The standard deviation is the scatter of the individual values
         * about the mean divided by sqrt(n - 2).
         * @param freq The reference frequency for the position calculation.
         * @param dfdp The frequency sensitivity (delta Hz / delta position).
         * @return Array of 2 doubles: mean and standard deviation, respectively.
         */
        public double[] calcPlusPositionAtFreq(double freq, double dfdp) {
            updateCalculation();
            int n = mm_positionListPlus.size();
            double[] positions = new double[n];
            for (int i = 0; i < n; i++) {
                DipPositionDatum datum = mm_positionListPlus.get(i);
                double deltaPos = (datum.freq - freq) / dfdp;
                positions[i] = datum.position - deltaPos;
            }
            double rtn[] = TuneUtilities.getMean(positions);
            if (n > 2) {
                rtn[1] /= sqrt(n - 2);
            }
            return rtn;
        }

        /**
         * Calculate the mean and standard deviation of the position
         * for a given frequency when moving in the minus direction.
         * All the measured (frequency,position) pairs are considered.
         * With position considered as the dependent (y) variable,
         * each position is regressed to the value it would have at
         * the given frequency, assuming the given sensitivity.
         * The standard deviation is the scatter of the individual values
         * about the mean divided by sqrt(n - 2).
         * @param freq The reference frequency for the position calculation.
         * @param dfdp The frequency sensitivity (delta Hz / delta position).
         * @return Array of 2 doubles: mean and standard deviation, respectively.
         */
        public double[] calcMinusPositionAtFreq(double freq, double dfdp) {
            updateCalculation();
            int n = mm_positionListMinus.size();
            double[] positions = new double[n];
            for (int i = 0; i < n; i++) {
                DipPositionDatum datum = mm_positionListMinus.get(i);
                double deltaPos = (datum.freq - freq) / dfdp;
                positions[i] = datum.position - deltaPos;
            }
            double rtn[] = TuneUtilities.getMean(positions);
            if (n > 2) {
                rtn[1] /= sqrt(n - 2);
            }
            return rtn;
        }

        /**
         * Calculate the mean and standard deviation of the position for a
         * given reflection coefficient when moving in the plus direction.
         * All the measured (reflection,position) pairs are considered.
         * With position considered as the dependent (y) variable,
         * each position is regressed to the value it would have at
         * the given reflection, assuming the given sensitivity.
         * The standard deviation is the scatter of the individual values
         * about the mean divided by sqrt(n - 2).
         * @param refl The reference frequency for the position calculation.
         * @param drdp The frequency sensitivity (delta refl / delta position).
         * @return Array of 2 doubles: mean and standard deviation, respectively.
         */
        public double[] calcPlusPositionAtRefl(double refl, double drdp) {
            updateCalculation();
            int n = mm_positionListPlus.size();
            double[] positions = new double[n];
            for (int i = 0; i < n; i++) {
                DipPositionDatum datum = mm_positionListPlus.get(i);
                double deltaPos = (datum.refl - refl) / drdp;
                positions[i] = datum.position - deltaPos;
            }
            double rtn[] = TuneUtilities.getMean(positions);
            if (n > 2) {
                rtn[1] /= sqrt(n - 2);
            }
            return rtn;
        }

        /**
         * Calculate the mean and standard deviation of the position for a
         * given reflection coefficient when moving in the minus direction.
         * All the measured (reflection,position) pairs are considered.
         * With position considered as the dependent (y) variable,
         * each position is regressed to the value it would have at
         * the given reflection, assuming the given sensitivity.
         * The standard deviation is the scatter of the individual values
         * about the mean divided by sqrt(n - 2).
         * @param refl The reference frequency for the position calculation.
         * @param drdp The frequency sensitivity (delta refl / delta position).
         * @return Array of 2 doubles: mean and standard deviation, respectively.
         */
        public double[] calcMinusPositionAtRefl(double refl, double drdp) {
            updateCalculation();
            int n = mm_positionListMinus.size();
            double[] positions = new double[n];
            for (int i = 0; i < n; i++) {
                DipPositionDatum datum = mm_positionListMinus.get(i);
                double deltaPos = (datum.refl - refl) / drdp;
                positions[i] = datum.position - deltaPos;
            }
            double rtn[] = TuneUtilities.getMean(positions);
            if (n > 2) {
                rtn[1] /= sqrt(n - 2);
            }
            return rtn;
        }

        private Sensitivity getSensitivities(DipPositionData data) {
            Sensitivity sensitivity = new Sensitivity();
            int size = data.size();
            double[] pos = new double[size];
            double[] freq = new double[size];
            double[] refl = new double[size];
            for (int i = 0; i < size; i++) {
                pos[i] = data.get(i).position;
                freq[i] = data.get(i).freq;
                refl[i] = data.get(i).refl;
            }

            double[] linfit = TuneUtilities.linFit(pos, freq, null);
            sensitivity.freqSens = linfit[1];
            sensitivity.freqSensSDV = linfit[3];

            linfit = TuneUtilities.linFit(pos, refl, null);
            sensitivity.reflSens = linfit[1];
            sensitivity.reflSensSDV = linfit[3];

            return sensitivity;
        }

        private boolean calculateLinearSensitivities() {
            boolean ok = false;
            Sensitivity plusSensitivity = new Sensitivity();
            Sensitivity minusSensitivity = new Sensitivity();
            if (mm_positionListPlus.size() > MIN_MEASURES) {
                // Fit line to data; get sensitivities and sdv's
                plusSensitivity = getSensitivities(mm_positionListPlus);
            }
            if (mm_positionListMinus.size() > MIN_MEASURES) {
                // Fit line to data; get sensitivities and sdv's
                minusSensitivity = getSensitivities(mm_positionListMinus);
            }
            Sensitivity sensitivity = new Sensitivity();
            sensitivity.average(plusSensitivity, minusSensitivity);
            mm_freqSens = sensitivity.freqSens;
            double freqMeanSDV = sensitivity.freqSensSDV;
            mm_reflSens = sensitivity.reflSens;
            double reflMeanSDV = sensitivity.reflSensSDV;
            Messages.postDebug("Sensitivity",
                               "mm_freqSens=" + Fmt.g(3, mm_freqSens)
                               + " +/- " + Fmt.g(3, freqMeanSDV)
                               + ", mm_reflSens=" + Fmt.g(3, mm_reflSens)
                               + " +/- " + Fmt.g(3, reflMeanSDV)
                               );
            // Want GOOD_CAL_S2N; after many measures, settle for MIN_CAL_S2N
            int n = mm_positionListPlus.size() + mm_positionListMinus.size();
            double m = (MIN_CAL_S2N - GOOD_CAL_S2N) / (20 - MIN_MEASURES);
            double targetS2N = GOOD_CAL_S2N + (n - MIN_MEASURES) * m;
            ok = freqMeanSDV < abs(mm_freqSens) / targetS2N
                   || reflMeanSDV < abs(mm_reflSens) / targetS2N;
            return ok;
        }

//        private boolean calculateSensitivities() {
//            //final int MIN_TOTAL_WEIGHT = 100;
//
//            boolean ok = false;
//            boolean debug = DebugOutput.isSetFor("Sensitivity");
//            int ndelFreq = deleteLeadingLowPoints(mm_freqSensList, debug);
//            Messages.postDebug("Sensitivity",
//                               ndelFreq + " leading points deleted from freq");
//            int ndelRefl = deleteLeadingLowPoints(mm_reflSensList, debug);
//            Messages.postDebug("Sensitivity",
//                               ndelRefl + " leading points deleted from refl");
//            int pointsLeft = min(mm_freqSensList.size(),
//                                      mm_reflSensList.size());
//            trimTo(pointsLeft, mm_positionListPlus);
//            TuneMean.Stats fStats = TuneMean.getFilteredMean(mm_freqSensList,
//                                                             debug);
//            TuneMean.Stats rStats = TuneMean.getFilteredMean(mm_reflSensList,
//                                                             debug);
//            if (fStats.nPointsUsed >= MIN_MEASURES
//                    && rStats.nPointsUsed >= MIN_MEASURES)
//            {
//                mm_freqSens = fStats.mean;
//                mm_reflSens = rStats.mean;
//                mm_freqPtsUsed = fStats.nPointsUsed;
//                mm_reflPtsUsed = rStats.nPointsUsed;
//                mm_freqSensSDV = fStats.sdv;
//                mm_reflSensSDV = rStats.sdv;
//                double freqMeanSDV = mm_freqSensSDV / sqrt(mm_freqPtsUsed);
//                double reflMeanSDV = mm_reflSensSDV / sqrt(mm_reflPtsUsed);
//                if (isTuneMotor(mm_cmi)) {
//                    ok = freqMeanSDV < abs(mm_freqSens) / GOOD_CAL_S2N;
//                } else if (isMatchMotor(mm_cmi)) {
//                    ok = reflMeanSDV < abs(mm_reflSens) / GOOD_CAL_S2N;
//                } else {
//                    ok = freqMeanSDV < abs(mm_freqSens) / MIN_CAL_S2N
//                       || reflMeanSDV < abs(mm_reflSens) / MIN_CAL_S2N;
//                }
//            }
//            if (debug) {
//                if (!ok) {
//                    Messages.postDebug("SensitivityMeasurement: NOT READY");
//                } else {
//                    String msg = "SensitivityMeasurement: cmi=" + mm_cmi + NL
//                    + "...FreqSensitivity=" + Fmt.g(3, mm_freqSens)
//                    + ", SDV=" + Fmt.g(3, mm_freqSensSDV)
//                    + ", nUsed=" + fStats.nPointsUsed
//                    + ", nOmitted=" + fStats.nPointsOmitted + NL
//                    + "   " + mm_freqSensList + NL
//                    + "...ReflSensitivity=" + Fmt.g(3, mm_reflSens)
//                    + ", SDV=" + Fmt.g(3, mm_reflSensSDV)
//                    + ", nUsed=" + rStats.nPointsUsed
//                    + ", nOmitted=" + rStats.nPointsOmitted + NL
//                    + "   " + mm_reflSensList
//                    ;
//                    Messages.postDebug(msg);
//                }
//            }
//            return ok;
//        }

//        private boolean remove(int idx) {
//            if (idx >= 0 && idx < size()) {
//                mm_freqSensList.remove(idx);
//                mm_reflSensList.remove(idx);
//                mm_stepList.remove(idx);
//                mm_outOfDate = true;
//                return true;
//            } else {
//                return false;
//            }
//        }

        /**
         * Add a measurement to the sensitivity lists. The points are not added
         * if the signal to noise of the measurement is less than 1/"sdv".
         * @param nSteps Number of motor steps used in this meaurement.
         * @param dFreq Change in frequency (Hz) over nSteps.
         * @param dRefl Change in reflection over nSteps.
         * @param sdv Measurement acceptance threshold.
         * @param cmi The Channel motor index of the motor being used.
         * @return True if the measurement was accepted.
         */
        public boolean add(int nSteps,
                           double dFreq, double dRefl, double sdv, int cmi) {
            boolean ok = false;
            if (!Double.isNaN(dFreq) && !Double.isNaN(dRefl)) {
                double sigToNoise = calcMaxSigToNoise(dFreq, dRefl);
                if (!(sigToNoise < 1 / sdv)) {
                    mm_freqSensList.add(dFreq / nSteps);
                    mm_reflSensList.add(dRefl / nSteps);
                    mm_stepList.add(nSteps);
                    mm_outOfDate = true;
                    ok = true;
                }
            }
            return ok;
        }

        public void clear() {
            mm_freqSensList.clear();
            mm_reflSensList.clear();
            mm_stepList.clear();
        }

        public boolean isMeasured() {
            updateCalculation();
            return mm_isMeasured;
        }

        public double getFreqSensitivity() {
            updateCalculation();
            return mm_freqSens;
        }

        public double getReflSensitivity() {
            updateCalculation();
            return mm_reflSens;
        }

        public double getFreqSensitivity(int i) {
            i = (i < 0) ? size() + i : i;
            try {
                return mm_freqSensList.get(i);
            } catch (IndexOutOfBoundsException ioobe) {
                return Double.NaN;
            }
        }

        public double getReflSensitivity(int i) {
            i = (i < 0) ? size() + i : i;
            try {
                return mm_reflSensList.get(i);
            } catch (IndexOutOfBoundsException ioobe) {
                return Double.NaN;
            }
        }

        public int getStep(int i) {
            i = (i < 0) ? size() + i : i;
            try {
                return mm_stepList.get(i);
            } catch (IndexOutOfBoundsException ioobe) {
                return 0;
            }
        }

        /**
         * Calculates an adjusted calibration step based on results of
         * the last one tried.
         * @param stepSize Previous requested cal step.
         * @param actualStep Actual step performed.
         * @param dFreq Change in frequency from that step.
         * @param dRefl Change in reflection from that step.
         * @param sdv The standard deviation we want for the sensitivity.
         * @param cmi Which motor we are dealing with.
         * @return The corrected/adjusted step size.
         * @throws InterruptedException if this command is aborted.
         */
        public int correctStepSize(int stepSize, int actualStep,
                                   double dFreq, double dRefl,
                                   double sdv, int cmi)
            throws InterruptedException {

            final double STEP_SIZE_INCREMENT = 1.5;
            final double S_TO_N_PAD = 1.4;

            int stepError = abs(stepSize) - abs(actualStep);
            if (stepError > 4 || stepSize * actualStep <= 0) {
                // Assume EOT
                // TODO: Check for repeated EOT? Channel should know?
                Messages.postDebugWarning("correctStepSize: Reversing direction"
                                          + ": actual step - requested step = "
                                          + stepError);
                stepSize = -stepSize;
            } else {
                double signalToNoise = calcSigToNoise(dFreq, dRefl, cmi);
                if (signalToNoise < 5) {
                    // Capacitor may not have moved (because of backlash)
                    stepSize *= STEP_SIZE_INCREMENT;
//                } else if (signalToNoise < MIN_S_TO_N) {
//                    double factor = MIN_S_TO_N / signalToNoise;
//                    factor = min(2, factor);
//                    stepSize = (int)(stepSize * factor) + 1;
                } else {
                    double factor = (S_TO_N_PAD / sdv) / signalToNoise;
                    factor = min(2, factor);
                    double minFactor = getMinCalStep(cmi) / (double)stepSize;
                    minFactor = abs(minFactor);
                    factor = max(minFactor, factor);
                    stepSize *= factor;
                }
            }
            stepSize = clipCalStep(stepSize, cmi);
            // If we're near the end of the channel, reverse direction
            if (!isPredictedPositionInRange(cmi, stepSize)) {
                Messages.postDebugWarning("correctStepSize: Reversing direction"
                                          + ": about to leave the sweep range");
                stepSize = -stepSize;
            }
            Messages.postDebug("Sensitivity", "stepSize=" + stepSize);
            return stepSize;
        }

        /**
         * Calculate an overall signal to noise to characterize given
         * delta(frequency) and delta(reflection) measurements.
         * (Uses global variables giving the expected standard deviation
         * in the two types of measurement.)
         * For a tune motor, we look at the delta(frequency), and for a
         * match motor, at the delta(reflection). If the motor is neither
         * identified as tune or match, the maximum signal-to-noise of
         * the two is returned.
         * @param dFreq The frequency change (Hz).
         * @param dRefl The reflection change.
         * @param cmi Channel motor index of the motor being evaluated.
         * @return The signal-to-noise ratio.
         */
        protected double calcSigToNoise(double dFreq, double dRefl, int cmi) {
            double sigToNoise;
            if (isTuneMotor(cmi)) {
                sigToNoise = abs(dFreq / mm_dipFreqSDV);
            } else if (isMatchMotor(cmi)) {
                sigToNoise = abs(dRefl / mm_dipReflSDV);
            } else {
                sigToNoise = calcMaxSigToNoise(dFreq, dRefl);
            }
            return sigToNoise;
        }

        /**
         * Calculate an overall signal to noise to characterize given
         * delta(frequency) and delta(reflection) measurements.
         * (Uses global variables giving the expected standard deviation
         * in the two types of measurement.)
         * The maximum signal-to-noise of
         * the two  measurements is returned.
         * @param dFreq The frequency change (Hz).
         * @param dRefl The reflection change.
         * @return The signal-to-noise ratio.
         */
        protected double calcMaxSigToNoise(double dFreq, double dRefl) {
            return max(abs(dFreq / mm_dipFreqSDV),
                            abs(dRefl / mm_dipReflSDV));
        }

        protected double calcSDV(List<Double>freqList,
                                 List<Double>reflList, int cmi) {
            double sdv;
            if (isTuneMotor(cmi)) {
                sdv = calcSDV(freqList);
            } else if (isMatchMotor(cmi)) {
                sdv = calcSDV(reflList);
            } else {
                sdv = max(calcSDV(freqList), calcSDV(reflList));
            }
            return sdv;
        }

        private double calcSDV(List<Double> list) {
            double sdv = Double.NaN;
            double mean = 0;
            int n = 0;
            for (Double x : list) {
                if (x != null && !Double.isInfinite(x) && !Double.isNaN(x)) {
                    n++;
                    mean += x;
                }
            }
            if (n > 3) {
                mean /= (n - 1);
                sdv = 0;
                for (Double x : list) {
                    if (x != null && !Double.isInfinite(x) && !Double.isNaN(x)) {
                        double diff = x - mean;
                        sdv += diff * diff;
                    }
                }
                sdv = sqrt(sdv / (n - 1));
            }
            return sdv;
        }


        public boolean addDipDatum(int dir, DipPositionDatum datum) {
            if (!Double.isNaN(datum.freq) && !Double.isNaN(datum.refl)) {
                if (dir > 0) {
                    return mm_positionListPlus.add(datum);
                } else {
                    return mm_positionListMinus.add(datum);
                }
            }
            return false;
        }
    }


    /**
     * Holds the frequency and reflection sensitivities to motor position
     * and their standard deviations.
     */
    static class Sensitivity {
        /** Frequency sensitivity (Hz/step). */
        public double freqSens = 0;

        /** SDV of frequency sensitivity (Hz/step) */
        public double freqSensSDV = Double.POSITIVE_INFINITY;

        /** Reflection sensitivity (amplitude/step). */
        public double reflSens = 0;

        /** SDV of reflection sensitivity (amplitude/step) */
        public double reflSensSDV = Double.POSITIVE_INFINITY;

        public void average(Sensitivity sens1, Sensitivity sens2) {
            double w1 = 1 / (sens1.freqSensSDV * sens1.freqSensSDV);
            double w2 = 1 / (sens2.freqSensSDV * sens2.freqSensSDV);
            freqSens = (sens1.freqSens * w1 + sens2.freqSens * w2) / (w1 + w2);
            freqSensSDV = sqrt(1 / (w1 + w2));

            w1 = 1 / (sens1.reflSensSDV * sens1.reflSensSDV);
            w2 = 1 / (sens2.reflSensSDV * sens2.reflSensSDV);
            reflSens = (sens1.reflSens * w1 + sens2.reflSens * w2) / (w1 + w2);
            reflSensSDV = sqrt(1 / (w1 + w2));
        }

    }


    /**
     * Container class holds a (motor) position, a frequency, and
     * a reflection value.
     */
    static class DipPositionDatum {
        public int position;
        public double freq;
        public double refl;

        /**
         * Construct a DipPositionDatum with the given data.
         * @param position Position of the motor.
         * @param freq Frequency (Hz).
         * @param refl Reflection amplitude.
         */
        public DipPositionDatum(int position, double freq, double refl) {
            this.position = position;
            this.freq = freq;
            this.refl = refl;
        }
    }


    /**
     * An ArrayList of DipPositionDatums.
     */
    static class DipPositionData extends ArrayList<DipPositionDatum> {

        /**
         * 
         */
        private static final long serialVersionUID = 1L;
    }


    public QuadPhase getQuadPhase() {
        return m_quadPhase ;
    }

    public void setQuadPhase(QuadPhase phase) {
        m_quadPhase = phase;
    }

    public String getCalPath() {
        return m_calPath;
    }

    public void setCalPath(String path) {
        m_calPath  = path;
    }

    public static boolean isProbeSupported(String dir) {
        File[] files = new File(dir).listFiles(CHAN_FILE_FILTER);
        return files != null && files.length > 0;
    }
    public void setRfchan(int rfchan) {
        m_rfchan  = rfchan;
    }

    public int getRfchan() {
        return m_rfchan;
    }

    /**
     * Make a ChannelInfo for every channel file in the system tune directory.
     * @param master The ProbeTune instance requesting the map.
     * @return A map of all the ChannelInfos, keyed by channel number.
     * May be empty, but never null.
     */
    public static Map<Integer, ChannelInfo> getChanInfos(ProbeTune master) {
        String sysdir = master.getSysTuneDir();
        File[] chanFiles = new File(sysdir).listFiles(new ChanFileFilter());
        if (chanFiles == null) {
            System.err.println("Cannot read directory \"" + sysdir + "\"");
        } else if (chanFiles.length == 0) {
            System.err.println("No channel files in \"" + sysdir + "\"");
        }
        return getChanInfos(master, chanFiles, null);
    }

    public static Map<Integer, ChannelInfo> getChanInfos(ProbeTune master,
                                                         String modeName) {
        if (modeName == null) {
            return getChanInfos(master);
        }

        String sysdir = master.getSysTuneDir();
        TuneMode mode = new TuneMode(sysdir, modeName);
        List<ChannelSpec> chanSpecs = mode.getChanList();
        if (chanSpecs.size() == 0) {
            master.exit("Could not get any channels from mode file: "
                        + "mode#" + modeName);
        }
        List<File> chanFiles = new ArrayList<File>();
        for (ChannelSpec cs : chanSpecs) {
            String path = sysdir + "/mode#" + cs.getChanNum();
            chanFiles.add(new File(path));
        }
        Map<Integer, ChannelInfo> chanMap = getChanInfos(master, chanSpecs);
        return chanMap;
    }

    /**
     * Make a ChannelInfo for every channel in the given list.
     * @param master The ProbeTune instance requesting the map.
     * @param specs The list of ChannelSpecs.
     * @return A map of all the ChannelInfos, keyed by channel number.
     * May be empty, but never null.
     */
    public static Map<Integer,ChannelInfo>
    getChanInfos(ProbeTune master, List<ChannelSpec> specs) {
        String sysdir = master.getSysTuneDir();
        int len = specs.size();
        File[] chanFiles = new File[len];
        Limits[] chanRanges = new Limits[len];
        for (int i = 0; i < len; i++) {
            ChannelSpec cs = specs.get(i);
            String path = sysdir + "/chan#" + cs.getChanNum();
            chanFiles[i] = new File(path);
            chanRanges[i] = cs.getFreqRange_MHz();
        }
        return getChanInfos(master, chanFiles, chanRanges);
    }

    /**
     * Given a list of channel persistence files, construct a map of the
     * corresponding ChannelInfos.
     * @param master The ProbeTune instance requesting the map.
     * @param chanFiles The array of chan#N files.
     * @param chanRanges TODO
     * @return A map of all the ChannelInfos, keyed by channel number.
     * May be empty, but never null.
     */
    private static Map<Integer, ChannelInfo> getChanInfos(ProbeTune master,
                                                          File[] chanFiles,
                                                          Limits[] chanRanges) {
        Arrays.sort(chanFiles, new PFileNameComparator<File>());
        Map<Integer, ChannelInfo> chanMap = new TreeMap<Integer, ChannelInfo>();
        int n = chanFiles.length;
        if (chanRanges == null) {
            chanRanges = new Limits[n];
            Limits noLimits = new Limits(Double.NaN, Double.NaN);
            for (int i = 0; i < n; i++) {
                chanRanges[i] = noLimits;
            }
        }
        for (int i = 0; i < n; i++) {
            File file = chanFiles[i];
            String name = file.getName();
            int iChan = ChanFileFilter.getChanNumber(name);
            ChannelInfo ci = new ChannelInfo(master,
                                             iChan,
                                             master.getProbeName(),
                                             master.getUsrProbeName(),
                                             name,
                                             chanRanges[i]
                                             );
            if (ci.exists()) {
                chanMap.put(iChan, ci);
            } else {
                master.exit("No valid channel file for channel #"
                            + iChan);
            }
        }
        return chanMap;
    }
}
