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
import java.nio.charset.Charset;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

import vnmr.util.Complex;
import vnmr.util.Fmt;
import static java.lang.Math.*;
import static vnmr.apt.AptDefs.*;


public class MtuneControl extends SweepControl {


    protected static final double FREQ_RESOLUTION = 25e3; // Hz
    protected static final double FREQ_OFFSET = FREQ_RESOLUTION / 2;

    /**
     * Approximate maximum number of points to acquire in a single sweep
     * in normal operation (non-calibration sweeps).
     */
    protected static final int MAX_PTS = 1000;


    // NB: Use "/vnmr/tmp" as temporary directory.
    // The "/tmp" directory permissions cause problems on Linux systems.
    private static final String VNMRDIR = ProbeTune.getVnmrSystemDir();
    private static final String INFOPATH = VNMRDIR + "/tmp/ptuneInfo";
    private static final String INFOPATHTMP = INFOPATH + "tmp";
    private static final String BASE_CAL_PARENTDIR = "tune";
    private static final String BASE_CAL_SUBDIR = "tunecal";
    private static final String BASE_CAL_DIR = VNMRDIR 
					     + "/" + BASE_CAL_PARENTDIR
					     + "/" + BASE_CAL_SUBDIR;
   
    private static final int DATA_TIMEOUT = 30000; // ms before giving up
    private static final int RETRY_RATE = 25; // ms between tries

    /** The max number of data blocks it could take to get fresh data. */
    //private static final int MAX_DATA_BLOCKS = 2;
    private static final int MAX_DATA_BLOCKS = 10;

    /** Number of initial FIDs to ignore to get fresh data. */
    private static final int MAX_FIDS_TO_DISCARD = 2;

    /** Max time it should take Vnmr to restart experiment w/ new parameters. */
    private static final long SEQUENCE_RESTART_TIME = 10000; // ms

    /** Records when last data was acquired. */
    private static long m_lastDataTime = 0;
    /** Time of last bad data seen. */
    private static long m_badDataTime = 0;

    /**
     * List of sweeps that we have requested from Vnmr.
     * The first in the list is the sweep we are currently receiving.
     * If the first element is null, we have not requested a sweep yet.
     */
    private LinkedList<FidData> m_requestedSweeps = new LinkedList<FidData>();

    /** Number of data blocks seen since we started waiting for fresh data. */
    private static int m_nDataBlocksChecked;

    /** Path to the FID file. */
    private String m_fidPath = null;

    // Calibration info
    private Calibration m_cal = null;

//    /** Where the last calibration was retrieved from. */
//    private static String m_calDirectory = null;

    // Variables to handle tune mode on the magnet leg
    protected static int TUNE_TIMEOUT = DATA_TIMEOUT;
    protected static int TUNE_CHECK_TIMEOUT = 2000; //Increased to 2 secs
    protected static int MLSTAT_UPDATE_RATE = 3; //2*TUNE_CHECK_TIMEOUT
    protected int m_tuneTimeout = TUNE_TIMEOUT;
    protected int m_tuneCheckTimeout = TUNE_CHECK_TIMEOUT;
    protected long m_dataFileDate = 0;
    protected boolean m_isTuneModeDisplayed = false;
    protected boolean m_displayedTuneMode;
    protected boolean m_inTuneMode;
    protected long m_tuneOnTime = 0; // If tune is on, this is in the past
    protected long m_tuneOffTime = 0; // If tune is on, this is in the future
    protected long m_sweepResetDate = 0; // When we sent a new sweep to Vnmr

    /** The fixed value of the quad phase: 0 - 3; -1 ==> not set. */
    protected int m_fixedQuadrant = -1;

    private static boolean m_firstTime= true; //First time measure tune
    private TuneModeThread m_tuneModeThread = null;

    private MotorControl m_motorControl = null;
    private ChannelInfo m_channelInfo = null;
    private int m_rfchan = 0;
    private Map<String,QuadPhase> m_phaseMap = new TreeMap<String,QuadPhase>();

    /**
     * Minimum ratio of best fit of raw data to reference, over next best fit
     * at another phase correction.
     */
//    private double m_minPhaseRatio;

    /** external off switch */
    private static boolean m_run = true;
    
    private static DateFormat m_tinyDateFormat
        = new SimpleDateFormat("yyyyMMdd'T'HHmm-");


    public MtuneControl(ProbeTune control, boolean usePhaseData) {
        super(control);
        init();
    }

    public MtuneControl(ProbeTune control, boolean usePhaseData,
                        double start, double stop, int np) {
        super(control, start, stop, np);
        init();
    }

    // cleanup after test to support unit testing
    public static void reset() {
        m_lastDataTime = 0;
        m_badDataTime = 0;
        m_firstTime = true;
        m_run = true;
        TUNE_TIMEOUT = DATA_TIMEOUT;
        TUNE_CHECK_TIMEOUT = 2000;
        MLSTAT_UPDATE_RATE = 3;
    }
    
    public static void close() {
    	m_run = false;
    }
    
    private void init() {
        if (isTuneModeSwitchable()) {
            startTuneModeThread();
        }
        m_requestedSweeps.add(null);
        if (!isQuadPhaseProblematic()) {
            m_fixedQuadrant = 0;
            setQuadPhase(0, true);
        }
    }

    /**
     * Read a String from a "file" (AKA entry) inside a Zip file.
     * Will return the first non-blank line of the file
     * that does not start with "#".
     * @param path The path to the Zip file.
     * @param name The name of the entry inside the Zip file.
     * @return The relevant line.
     */
    private static String readCalString(String path, String name) {
        String value = null;
        ZipFile zipFile = null;
        BufferedReader reader = null;
        try {
            zipFile = new ZipFile(path);
            ZipEntry entry = zipFile.getEntry(name);
            InputStream in = zipFile.getInputStream(entry);
            reader = new BufferedReader(new InputStreamReader(in));

            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (!line.startsWith("#") && line.length() > 0) {
                    value = line;
                    break;
                }
            }
        } catch (Exception e) {
            Messages.postDebugWarning("Cannot read line from: " + path
                                      + ", entry: " + name);
        } finally {
            try { reader.close(); } catch (Exception e) {}
            try { zipFile.close(); } catch (Exception e) {}
        }
        return value;
    }

    /**
     * Read a double value from a "file" (AKA entry) inside a Zip file.
     * The entry file should have a line with a single number on it.
     * Any other lines should start with "#" or be blank.
     * @param path The path to the Zip file.
     * @param name The name of the entry inside the Zip file.
     * @return The double value, or NaN on error.
     */
    private static double readCalDouble(String path, String name) {
        double value = Double.NaN;
        ZipFile zipFile = null;
        BufferedReader reader = null;
        try {
            zipFile = new ZipFile(path);
            ZipEntry entry = zipFile.getEntry(name);
            InputStream in = zipFile.getInputStream(entry);
            reader = new BufferedReader(new InputStreamReader(in));

            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (!line.startsWith("#") && line.length() > 0) {
                    value = Double.valueOf(line);
                    break;
                }
            }
        } catch (Exception e) {
            Messages.postDebugWarning("Cannot read double value from: " + path
                                      + ", entry: " + name);
        } finally {
            try { reader.close(); } catch (Exception e) {}
            try { zipFile.close(); } catch (Exception e) {}
        }
        return value;
    }

    /**
     * Read the value of the probeDelay from the specified calibration
     * Zip file.
     * @param path The path to the Zip file.
     * @return The probeDelay in ns.
     */
    private static double readCalProbeDelay(String path) {
        double delay_ns = readCalDouble(path, DELAY_CAL_NAME);
//        if (Double.isNaN(delay_ns)) {
//            delay_ns = 5.5;
//        }
        return delay_ns;
    }

    private static Interval readCalDipInterval(String path) {
        String str_interval = readCalString(path, DIP_REGION_CAL_NAME);
        return new Interval(str_interval);
    }

//    private static double readCalQuadPhase() {
//        return readProbeDouble(PHASE_CAL_NAME);
//    }

    private static String getQuadPhaseFilepath() {
        String probe = System.getProperty("apt.probeName");
        String filepath = getCalDir(probe);
        if (filepath != null) {
            filepath += "/" + PHASE_CAL_NAME;
        }
        return filepath;
    }

    /**
     * Starts up a Thread to periodically check that the magnet leg
     * is in Tune mode.
     */
    protected void startTuneModeThread() {
        if (m_master != null) {
            m_tuneModeThread = new TuneModeThread();
            m_tuneModeThread.start();
        }
    }

    public void stopTuneModeThread(int ms) throws InterruptedException {
    	if (m_tuneModeThread != null) {
    		m_run = false;
    		m_tuneModeThread.interrupt();
    		m_tuneModeThread.join(ms);
    	}
    }

    /**
     * Required by the abstract SweepControl, but we don't need to do
     * anything to initiate the connection.
     * @return Always returns true.
     */
    protected boolean getConnection() {
        // NB: Could check date of ptune... file to see if we are getting data
        return true;
    }

    /**
     * On pre-Nirvana systems MtuneControl needs to talk to the ProTune
     * motor modules to set the Magnet Leg in or out of Tune Mode.
     * This gives us a handle to use to talk to the motors.
     * @param motorControl The handle to the motor controller.
     */
    public void setMotorController(MotorControl motorControl) {
        m_motorControl = motorControl;
    }

    /**
     * Checks if the console should be in tune mode, and, if so,
     * puts (or keeps) it in tune mode.
     * Not actually needed on Nirvana systems.
     */
    protected void checkTuneMode() throws InterruptedException {
        long dataDate = getDataFileDate();
        long now = System.currentTimeMillis();
        long dataAge = now - dataDate;
        if (dataDate > m_dataFileDate && dataAge < m_tuneTimeout) {
            // Tune acquisition is running - stay in tune mode
            if (!m_inTuneMode) {
                Messages.postDebug("Turning on Tune mode");
            }
            setTuneMode(m_tuneTimeout - dataAge);
            m_dataFileDate = dataDate;
        } else if (dataAge >= m_tuneTimeout) {
            if (isSweepOk()) {
                Messages.postWarning("Blocksize processing inactive for "
                        + (dataAge / 1000) + " seconds");
            }
            setTuneMode(0);
        }
        if (updateTuneModeDisplay()) {
            updateDataConnectionDisplay();
        }
        Messages.postDebug("TuneMode",
                           "checkTuneMode: gettingData=" + m_inTuneMode);
    }

    /**
     * Sends a message to the GUI to display the current data connection status.
     * Currently, assumes it's the same as the Tune Mode status.
     */
    private void updateDataConnectionDisplay() {
        String msg = m_inTuneMode ? "sweepOk yes" : "sweepOk no";
        m_master.sendToGui(msg);
    }

    /**
     * Updates the display of tune mode (off or on).
     * @return True if the display needed to be changed.
     */
    private boolean updateTuneModeDisplay() {
        boolean rtn = false;
        if (!m_isTuneModeDisplayed
            || (m_displayedTuneMode ^ m_inTuneMode))
        {
            rtn = true;
            m_isTuneModeDisplayed = true;
            m_displayedTuneMode = m_inTuneMode;
            int mode = m_inTuneMode ? 1 : 0;
            m_master.sendToGui("displayTuneMode " + mode);
        }
        return rtn;
    }

    /**
     * Ask the ProTune Motor Module to send back the status of the
     * magnet leg.
     */
    private void updateMLStatus() throws InterruptedException {
        /*
         * Special values to turn off magnet leg status checking
         */
        if (m_motorControl.isIndexing()) {
            return;
        }
        if (m_master.getDebugLevel() > 9000) {
            return;
        }
        if (m_motorControl != null) {
            m_motorControl.sendToMotor("getmlstat");
            m_motorControl.sendToMotor("version");
        }
    }

    private void setTuneBand(double freq) throws InterruptedException {
        if (m_motorControl != null) {
            String cmd = isLowband(freq) ? "xtune" : "htune";
            m_motorControl.sendToMotor(cmd);
        }
    }

    private boolean isLowband(double freq) {
        double freq_MHz = freq / 1e6;
        String key = "apt.H1freq";
        String value = System.getProperty(key);
        double bdy = 0.8 * Double.parseDouble(value);
        boolean isLowband = freq_MHz < bdy;
        return isLowband;
    }

    /**
     * Tells the ProTune Motor Module to turn Tune Mode on or off in
     * the magnet leg.
     * This only applies to pre-Nirvana (VnmrS) systems; later systems
     * select Tune Mode from the pulse sequence.
     * @param timeout Keep the system in tune mode for this many ms;
     * or specify 0 to turn off Tune Mode immediately.
     */
    private void setTuneMode(long timeout) throws InterruptedException {
        if (m_motorControl != null) {
            int time = (int)((timeout + 999) / 1000);
            time = max(0, time);
            if (isTuneModeSwitchable()) {
                m_motorControl.sendToMotor("tune " + time);
            }
            if (time == 0) {
                if (m_inTuneMode) {
                    Messages.postDebugWarning("Turning off Tune mode");
                }
                m_inTuneMode = false;
                //m_tuneOffTime = System.currentTimeMillis();
            } else {
                if (!m_inTuneMode) {
                    m_inTuneMode = true;
                    m_tuneOnTime = System.currentTimeMillis();
                    //ProbeTune.printlnDebugMessage(4, m_tuneOnTime
                    //                              + ": turn on TuneMode");
                }
                m_tuneOffTime = m_tuneOnTime + 1000 * time;
            }
            if (updateTuneModeDisplay()) {
                updateDataConnectionDisplay();
            }
        }
    }

    /**
     * Get the filepath to the current FID file.
     * @param verbose If true, extra debug and error messages are printed.
     * @return The absolute file path.
     */
    private String getFidFilePath(boolean verbose) {
        getPtuneInfo(verbose);
        return m_fidPath;
    }

    /**
     * Get the RF power level used for the given frequency.
     * @return The RF power level in dB (same units as tpwr).
     */
    private int getPowerAt(double freq) {
        int pwr = 20;
        String key = null;
        String value = null;
        try {
            key = "apt.lobandPower";
            value = System.getProperty(key);
            int lbpwr = Integer.parseInt(value);
            key = "apt.hibandPower";
            value = System.getProperty(key);
            int hbpwr = Integer.parseInt(value);
            pwr = isLowband(freq) ? lbpwr : hbpwr;
        } catch (NumberFormatException e) {
            Messages.postDebug("Properties", "Property has invalid value: "
                               + key + "=\"" + value + "\"");
        } catch (NullPointerException e) {
            Messages.postDebug("Properties", "Property not set: " + key);
        }
        return pwr;
    }

    /**
     * Get the gain level used for the given frequency.
     * @return The gain in dB (same units as gain parameter).
     */
    private int getGainAt(double freq) {
        int gain = 0;
        try {
            int lbgain;
            lbgain = Integer.parseInt(System.getProperty("apt.lobandGain"));
            int hbgain;
            hbgain = Integer.parseInt(System.getProperty("apt.hibandGain"));
            gain = isLowband(freq) ? lbgain : hbgain;
        } catch (NumberFormatException e) {
            // Property has invalid value
        } catch (NullPointerException e) {
            // Property not set
        }
        return gain;
    }

    private void getPtuneInfo(boolean verbose) {
        BufferedReader in = TuneUtilities.getReader(INFOPATH);
        if (in == null) {
            if (verbose) {
                Messages.postDebug("No info file: " + INFOPATH);
            }
            m_fidPath = null;
        } else {
            try {
                String line;
                while ((line = in.readLine()) != null) {
                    try {
                        StringTokenizer toker = new StringTokenizer(line);
                        String key = toker.nextToken();
                        if (key.equals("fidpath")) {
                            m_fidPath = toker.nextToken();
                        }
                    } catch (NoSuchElementException nsee) {
                    }
                }
            } catch (IOException ioe) {
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }
        }
    }


    /*
    private boolean isTuneOn() {
        return m_currentTuneMode && System.currentTimeMillis() < m_tuneOffTime;
    }
    */

    public boolean isQuadPhaseProblematic() {
        String console = System.getProperty("apt.console");
        return ProbeTune.isQuadPhaseProblematic(console);
    }

    public boolean isTuneModeSwitchable() {
        String console = System.getProperty("apt.console");
        return ProbeTune.isTuneModeSwitchable(console);
    }

    public boolean isTuneBandSwitchable() {
        String console = System.getProperty("apt.console");
        return ProbeTune.isTuneBandSwitchable(console);
    }

    public boolean isSweepOk() {
        return m_inTuneMode;
    }

    private static long getDataFileDate() {
        long infotmpDate = new File(INFOPATHTMP).lastModified();
        long infoDate = new File(INFOPATH).lastModified();
        long date = max(infotmpDate, infoDate);
        Messages.postDebug("MtuneData|TuneMode",
                           "INFOPATH=" + INFOPATH
                           + ", age=" + (System.currentTimeMillis() - date)
                           + "ms");
        return date;
    }

    /**
     * Get data obtained after a given time, from the given input reader.
     * @param when Clock time in ms of oldest acceptable data.
     * @param input The full path to the file with the data.
     * @return The data, or null if the data in the file is too old.
     */
    private static ReflectionData readRawData(long when, BufferedReader input) {
        if (input == null) {
            Messages.postError("ProTune readRawData: null Reader");
            return null;
        }

        ReflectionData data = null;
        StringBuffer sbData = new StringBuffer();
        sbData.append("scaled ");
        try {
            String line;
            boolean ok = false;
            while ((line = input.readLine()) != null) {
                // Add line from file to the StringBuffer
                sbData.append(line).append(" ");

                // If the Date line in the file is not OK, stop reading.
                if (line.startsWith("acqDate") ) {
                    StringTokenizer toker = new StringTokenizer(line);
                    toker.nextToken();
                    String strTime = toker.nextToken();
                    long acqTime = Long.parseLong(strTime);
                    if (acqTime >= when) {
                        ok = true;
                    } else {
                        // Data is too old
                        if (m_badDataTime != acqTime) {
                            // This is a new data block, but older than we want
                            m_nDataBlocksChecked++;
                            Messages.postDebug("MtuneData",
                                     + ((when - acqTime) / 1000.0)
                                     + " s too old"
                                     + ", m_nDataBlocksChecked="
                                     + m_nDataBlocksChecked);
                            m_badDataTime = acqTime;
                            if (m_nDataBlocksChecked > MAX_DATA_BLOCKS) {
                                // We've waited long enough
                                Messages.postDebug("MtuneData",
                                                   "ReadRawData: ... have seen "
                                                   + MAX_DATA_BLOCKS
                                                   + " data blocks"
                                                   + " -- use this data");
                                ok = true;
                            } else {
                                // Hold out for newer data (just return null)
                                ok = false;
                                break;
                            }
                        }
                    }
                }
            }
            if (ok) {
                data = new ReflectionData(sbData.toString());
                Messages.postDebug("MtuneData",
                                   "Got " + data.size + " data points");
            }
        } catch (IOException ioe) {
            Messages.postError("IO error in readRawData: " + ioe);
        } catch (NumberFormatException nfe) {
            Messages.postError("Error parsing number in readRawData()");
        } finally {
            try {
                input.close();
            } catch (Exception e) {}
        }
        return data;
    }

    /**
     * Get data from the fid file in the current experiment.
     * If we have asked for a new sweep range, waits for data giving
     * the new range.
     * Otherwise, waits for data that was <I>acquired</I> after this
     * method was called.  Currently, this is done by waiting for the
     * data in the fid file to change two times, and then assuming the
     * data is OK.
     * @param verbose If true, extra debug and error messages are printed.
     * @return The data, or null if unsuccessful.
     */
    private ReflectionData readFidfileData(boolean verbose)
        throws InterruptedException {

        // RELIABLE FID means we read FID file twice and got same data
        //
        // If m_nextSweep != null, first RELIABLE data with that NP is good
        // If m_currSweep is ALSO null, we have not set the sweep yet, so do it
        // Otherwise, wait for the NP in m_currSweep;
        //   if we can't check a timestamp, accept third RELIABLE FID
        //
        // If we get an NP different from m_currSweep AND m_nextSweep,
        //   we have a serious problem. (EXIT? Set new sweep?)

        Messages.postDebug("FidData",
                           "MtuneControl.readFidfileData: STARTING");
        if (m_requestedSweeps.getFirst() == null) {
            // We have never set the sweep, so request new sweep
            m_master.setFullSweep();
        }
        long acqTimeLowerBound = System.currentTimeMillis();
        String fidFilePath;
        if ((fidFilePath = getFidFilePath(verbose)) == null) {
            if (verbose) {
                Messages.postDebug("MtuneControl.readFidfileData: "
                                   + "No path to FID file");
            }
            return null;
        }

        boolean printDebug = true;
        boolean resentSweeps = false;
        //long lastNpErrorDate = 0;
        int nfids;
        FidData fid1;
        for (nfids = 1, fid1 = parseFidFile(fidFilePath, verbose);
             nfids <= MAX_FIDS_TO_DISCARD;
             )
        {
            if (printDebug) {
                Messages.postDebug("FidData",
                                   "MtuneControl.readFidfileData: nfids="
                                   + nfids
                                   + ", fid1 OK is " + (fid1 != null));
                printDebug = false;
            }
            if (fid1 == null) {
                //lastNpErrorDate = now;
                FidData nextSweep = m_requestedSweeps.getLast();
                if (m_requestedSweeps.size() < 2 && printDebug) {
                    // np is wrong and may not have a sweep request pending
                    Messages.postDebug("FidData",
                                       "MtuneControl.readFidfileData: "
                                       + "wait for correct NP in sweep:"
                                       + " want " + nextSweep.getNRawPoints());
                    printDebug = false;
                } else {
                    // np is not recognized, but a sweep request is pending
                    // Very next fid (with correct np) should be good
                    long now = System.currentTimeMillis();
                    if (now - nextSweep.dateStamp > SEQUENCE_RESTART_TIME) {
                        if (resentSweeps) {
                            Messages.postDebug("Waiting for new sweep data");
                            Thread.sleep(RETRY_RATE); // Try later
                            break;
                        } else {
                            Messages.postDebug("FidData",
                                               "MtuneControl.readFidfileData: "
                                               + "waiting for raw np = "
                                               + nextSweep.getNRawPoints());
                            resentSweeps = true;
                            // Been waiting a long time --
                            // send another sweep resetting command
                            //Messages.postDebug("MtuneControl.readFidfileData:"
                            //                   + "resetting sweep");
                            //double center = (m_sentStart + m_sentStop) / 2;
                            //double span = m_sentStop - m_sentStart;
                            //NB:setSweep(center, span, m_sentNPoints, true);
                        }
                    }
                }
                nfids = MAX_FIDS_TO_DISCARD;
                fid1 = parseFidFile(fidFilePath, false);
            } else {
                FidData fid2 = parseFidFile(fidFilePath, false);
                if (!fid1.equals(fid2)) {
                    // Got different data; make sure it's stable.
                    printDebug = true;
                    Messages.postDebug("FidData",
                                       "MtuneControl.readFidfileData: "
                                       + "fid1 != fid2");
                    nfids++;
                    fid1 = parseFidFile(fidFilePath, false);
                    if (fid1 == null || !fid1.equals(fid2)) {
                        Messages.postDebug("FidData",
                                           "MtuneControl.readFidfileData: "
                                           + "FID NOT stable");
                        // Must have been changing when fid2 was read
                        // Read once more
                        fid2 = fid1;
                        fid1 = parseFidFile(fidFilePath, false);
                        if (fid1 == null || !fid1.equals(fid2)) {
                            Messages.postDebug("FidData",
                                               "MtuneControl.readFidfileData: "
                                               + "GIVE UP");
                            return null;
                        }
                        nfids++;
                    }
                    Messages.postDebug("FidData",
                                       "MtuneControl.readFidfileData: "
                                       + "FID stable");
                }
            }
            Thread.sleep(RETRY_RATE);
        }

        ReflectionData rtn = null;
        if (fid1 != null) {
            if (DebugOutput.isSetFor("FidData")) {
                Messages.postDebug("fid1 raw max=" + fid1.getMaxAbsval());
            }
            Messages.postDebug("rfchan",
                               "readFidFileData: rfchan=" + fid1.getRfchan());
            rtn = new ReflectionData(fid1.real, fid1.imag,
                                     acqTimeLowerBound,
                                     fid1.start_hz, fid1.stop_hz,
                                     fid1.getPower(), fid1.getGain(),
                                     fid1.getRfchan());
        }
        Messages.postDebug("FidData", "MtuneControl.readFidfileData: DONE");
        return rtn;
    }

    /**
     * Read a Vnmr FID file and extract the sweep data.
     * The number of points is read from the FID file header.
     * Frequency limits, power, and gain are assumed to be what we
     * have requested for a scan with that number of points.
     * The returned data is guaranteed to be complete.
     * Failure implies that the FID file could not be read, or that
     * the number of points in the scan is unexpected (so we cannot
     * interpret the data).
     * Any "padding" data points that were collected are discarded,
     * so the returned object includes only the desired data.
     * (See {@link #sendSweepToVnmr}.)
     * @param pathname The full path and name of the fid file.
     * @param verbose If true, extra debug and error messages are printed.
     * @return The FidData object representing the scan, or null on failure.
     */
    private FidData parseFidFile(String pathname, boolean verbose) {
        FidData fidData = null;
        DataInputStream in = null;
        try {
            in = new DataInputStream
                    (new BufferedInputStream
                     (new FileInputStream(pathname)));

            // File Header
            in.skipBytes(8);
            int np = in.readInt() / 2; // Number of raw, complex data points
            in.skipBytes(14);
            short status = in.readShort();
            in.skipBytes(4);

            /*Messages.postError("parseFidFile: raw np=" + np);/*DBG*/

            boolean isFloat = (status & 0x8) != 0;
            boolean isSP = (status & 0xC) == 0x0;

            // Block Header (nothing useful here -- ct is always 1)
            in.skipBytes(28);

            // Initialize fidData with sweep values corresponding to this np
            fidData = getFidInstanceForNp(np);
            cleanRequestedSweepList(np);

            if (fidData == null) {
                // Unexpected number of points; return null FidData
            } else {
                // Parse Data
                int bytesPerDatum = isSP ? 4 : 8; // Complex datum is two words
                in.skipBytes(bytesPerDatum * fidData.getLeadingPadding());
                int npts = fidData.getNGoodDataPoints();
                /*Messages.postError("parseFidFile: cooked npts=" + npts);/*DBG*/

                // NB: NMR convention reverses the sign of the imag channel,
                // so we undo that here.
                double[] real = new double[npts];
                double[] imag = new double[npts];
                if (isFloat) {
                    for (int i = 0;  i < npts; i++) {
                        real[i] = in.readFloat();
                        imag[i] = -in.readFloat();
                    }
                } else if (isSP) {
                    for (int i = 0;  i < npts; i++) {
                        real[i] = in.readShort();
                        imag[i] = -in.readShort();
                    }
                } else {
                    for (int i = 0;  i < npts; i++) {
                        real[i] = in.readInt();
                        imag[i] = -in.readInt();
                    }
                }

                /*for (int i = 0; i < npts; i++) {
                  System.out.println(real[i] + " \t " + imag[i]);
                  }/*DBG*/

                fidData.setData(real, imag);
            }
            //System.out.println("MtuneControl.parseFidFile: OK");/*DBG*/

        } catch (IOException ioe) {
            if (verbose) {
                Messages.postError("Cannot parse FID file: " + pathname);
                Messages.postDebugWarning("MtuneControl.parseFidFile: Error: "
                                          + ioe);
            }
            fidData = null;
        } finally {
            try {
                in.close();
            } catch (Exception e) {}
        }

        return fidData;
    }

    /**
     * Get data that was acquired after a specified time. Will time out
     * if no data is obtained within a fixed number of seconds.
     * Calibration corrections will be applied, if available.
     * @see #DATA_TIMEOUT
     * @param when Clock time in ms of oldest acceptable data. Use 0
     * to accept any data, or -1 to accept any data newer than the data
     * last read.
     * @return The data, or null if none obtained when timeout occurs.
     */
    protected ReflectionData getData(long when, double probeDelay_ns)
        throws InterruptedException {

        Messages.postDebug("GetData", "MtuneControl.getData(" + when
                           + "ms, " + probeDelay_ns + "ns)");
        boolean retryFlag = false;
        long timeout = DATA_TIMEOUT;
        boolean rawDataFlag = m_master.isRawDataMode();
        ReflectionData data = null;

        if (m_firstTime) {
            m_firstTime = false;
            timeout = ProbeTune.get1stSweepTimeout();
        } else if (!m_inTuneMode) {
            // No sweep communication, don't get data!
            Messages.postError("MtuneControl.getData: No sweep communication");
            return null;
        }

        // Try until we get good data
        long dataReqTime = System.currentTimeMillis();
        if (when == -1) {
            when = m_lastDataTime + 1;
        }
        boolean isPreviousSweepWrong = false;
        boolean isSweepErrorPrinted = false;
        double start = 0;
        double stop = 0;
        int np = 0;
        m_nDataBlocksChecked = 0;
        long now;
        Messages.postDebug("MtuneData", "getData: Enter data checking loop");
        while (data == null
               && (now = System.currentTimeMillis()) - dataReqTime < timeout)
        {
            if (ProbeTune.isCancel()) {
                return null;
            }
            checkTuneMode(); // Update the tune mode
            // Try for data only if we're in tune mode
            Messages.postDebug("AllMtuneData",
                               "getData: inTuneMode=" + m_inTuneMode);
            if (m_inTuneMode) {
                when = max(m_tuneOnTime, when);
                boolean verbose = DebugOutput.isSetFor("MtuneData");
                Messages.postDebug("AllMtuneData",
                                   "Try getting new data ("
                                   + m_tuneOnTime + ", " + when + ", "
                                   + now + ")");
                data = readFidfileData(verbose);
            }
            if (data == null) {
                Thread.sleep(RETRY_RATE);
                isSweepErrorPrinted = false;
            } else {
                start = data.getStartFreq();
                stop = data.getStopFreq();
                np = data.getSize();
                double res = (stop - start) / (np - 1);
                if (!rawDataFlag && m_requestedSweeps.size() == 1) {
                    // May have to request new sweep to match cal data
                    double rem1 = start
                            - FREQ_RESOLUTION * (int)(start / FREQ_RESOLUTION);
                    double rem2 = stop
                            - FREQ_RESOLUTION * (int)(stop / FREQ_RESOLUTION);
                    double rem3 = res
                            - FREQ_RESOLUTION * (int)(res / FREQ_RESOLUTION);
                    double align = FREQ_RESOLUTION / 2;
                    if (rem1 != align || rem2 != align || rem3 != 0) {
                        // Invalid sweep - let setSweep() adjust it
                        setSweep((stop + start) / 2, stop - start,
                                 np, getRfchan());
                        data = null; // Trash this data
                        String msg = "ERROR? MtuneControl.getData: "
                            + "Data not aligned with calibration data"
                            + "\n... Got start=" + Fmt.f(4, start)
                            + ", stop=" + Fmt.f(4, stop)
                            + ", np=" + np
                            + "\n... getSpan=" + Fmt.f(4, getSpan())
                            + ", getCenter=" + Fmt.f(4, getCenter());
                        Messages.postDebug("MtuneData", msg);
                    }
                }
                Messages.postDebug("MtuneControl",
                                   "data: start="+ Fmt.f(4, start/1e6)
                                   + ", stop=" + Fmt.f(4, stop/1e6)
                                   + ", np=" + np);
                // Allow for possibly getting some extra points
                // NB: This test is obsolete. NP is tested to match in
                // parseFidFile.
                if (m_requestedSweeps.size() > 1
                    && (start > m_startFreq
                        || (m_startFreq - start) / res > 2
                        || stop < m_stopFreq
                        || (stop - m_stopFreq) / res > 2
                        || abs(np - m_np) > 5))
                {
                    // Don't want this data
                    data = null;
                    if (!isSweepErrorPrinted) {
                        Messages.postDebug("FidData",
                                 "MtuneControl.getData: wrong sweep range"
                                 + "\n want " + m_startFreq
                                 + " to " + m_stopFreq + ", np=" + m_np
                                 + "; got " + start + " to " + stop
                                 + ", np=" + np);
                        isSweepErrorPrinted = true;
                    }
                    isPreviousSweepWrong = true;
                    Thread.sleep(RETRY_RATE);
                } else {
                    // Sweep has been set
                    //m_settingSweep = false;
                    if (isPreviousSweepWrong) {
                        Messages.postDebug("FidData",
                                 "MtuneControl.getData: sweep range correct"
                                 + ": " + start + " to " + stop + ", np=" + np);
                        isPreviousSweepWrong = false;
                        isSweepErrorPrinted = false;
                    }
                }
            }
            if (data == null && (now -  dataReqTime > timeout / 2)) {
                // Persistently failing to get correct data.
                if (!retryFlag) {
                    // Try resending the sweep request to Vnmr.
                    retryFlag = true;
                    sendSweepToVnmr(getCurrentCenter(),
                                    getCurrentSpan(),
                                    getCurrentNp(),
                                    getCurrentPower(),
                                    getCurrentGain(),
                                    getCurrentRfchan());
                    Messages.postDebug("Re-sent sweep request to Vnmr");
                }
            }
        }
        if (data == null) {
            Messages.postDebug("FidData", "MtuneControl.getData() failed");
            //m_settingSweep = false; // So we don't lock up
        } else {
            //data.setPhasedDataUsed(false);
            m_lastDataTime = data.getAcqTime();

            if (!rawDataFlag) {
                // Do calibration corrections, if available
                if (!correctData(data)) {
                    data.m_calFailed = true;
                    // ... otherwise, normalize to max reflection of 1
                    data.scaleToMaxAbsval();
                }
                data.correctForProbeDelay_ns(probeDelay_ns);
            }
        }
        if (data == null) {
            m_master.exit("failed - Could not get correct sweep data");
        }
        return data;
    }

    /**
     * Get the calibration corrections appropriate to the given data
     * and apply them.
     * @param data The raw data to be corrected.
     * @return True on success; false if there is no suitable calibration.
     */
    public boolean correctData(ReflectionData data) {
        if (m_cal != null && m_cal.isGoodFor(data)) {
            Messages.postDebug("RFCal", "correctData: Use existing correction");
            if (m_cal.isPhaseMeasurementNeeded()) {
                String calPath = m_cal.getFilePath();
                double delay = readCalProbeDelay(calPath);
                m_cal.setPhase(getPhaseFromRefScan(calPath, data, delay));
            }
            m_master.displayPhase(m_cal.getQuadPhaseValue());
            correctData(data, m_cal);
        } else {
            m_cal = getCalibrationFor(data);
            //m_cal = setCalibrations(data); // Superfluous?
            if (m_cal == null) {
                return false;
            }

            if (m_cal.isPhaseKnown()) {
                boolean fixed = !isQuadPhaseProblematic();
                m_master.displayPhase(m_cal.getQuadPhaseValue(), fixed);
                correctData(data, m_cal);
            } else {
                // Need to calculate the quad phase shift between calibration
                // data and our current data.
                data = determineQuadPhase(data);
            }
        }
        return true;
    }

    /**
     * Determine what phase shift is needed to correct this data with
     * the given calibration.
     * @return The same data, modified to corrected values.
     */
    protected ReflectionData determineQuadPhase(ReflectionData rawData) {
        Messages.postDebugWarning("Using old determineQuadPhase method");
        double[] sortedBases = new double[4];
        QuadPhase bestPhase = new QuadPhase();
        if (m_fixedQuadrant >= 0) {
            bestPhase.setPhase(m_fixedQuadrant);
        } else {
            double[][] reflection2 = new double[4][];
            double[] basetop = new double[4];
            QuadPhase testPhase = new QuadPhase();
            for (int phase = 0; phase < 4; phase++) {
                testPhase.setPhase(phase);
                setCalibrations(rawData, testPhase);
                reflection2[phase] = getAbsvalArray(rawData, m_cal);
                Arrays.sort(reflection2[phase]);
                // Look at 95th percentile point
                int idx = (int)(0.95 * reflection2[phase].length);
                basetop[phase] = reflection2[phase][idx];
                sortedBases[phase] = reflection2[phase][idx];
            }
            Arrays.sort(sortedBases);
            if (sortedBases[3] < 1.2) {
                // All phases give OK correction, so assume 0
                bestPhase.setPhase(0);
            } else {
                for (int i = 0; i < 4; i++) {
                    if (basetop[i] == sortedBases[0]) {
                        bestPhase.setPhase(i);
                    }
                    Messages.postDebug("QuadPhase",
                                       "MtuneControl.determineQuadPhase: "
                                       + "base[" + i + "]=" + basetop[i]);
                }
            }
            Messages.postDebug("QuadPhase", "MtuneControl.determineQuadPhase: "
                               + "bestPhase=" + bestPhase);
        }
        m_master.displayPhase(bestPhase.getPhase(), m_fixedQuadrant >= 0);
        m_cal = null;
        setCalibrations(rawData, bestPhase);
//         if (sortedBases[0] < 1.2 * 1.2 && sortedBases[1] > 2 * 2) {
//             // Clear winner
//             m_cal.isPhaseKnown = true;
//             m_knownPhaseList.put(m_cal.baseFilePath, bestPhase);
//             Messages.postError("MtuneControl.determineQuadPhase: "
//                                + "phase is well known");
//         }
        ReflectionData correctedData = rawData;
        correctData(correctedData, m_cal);
        if (m_fixedQuadrant < 0 && sortedBases[1] < 1.2) {
            m_cal = null;       // FORCE PHASE RECALCULATION ON NEXT SWEEP
        }
        return correctedData;
    }

//    /**
//     * Determine what phase shift is needed to correct this data with
//     * the given calibration.
//     * @return The same data, modified to corrected values.
//     */
//    protected int measureQuadPhase(ReflectionData rawData) {
//        Messages.postDebug("WARNING: using old measureQuadPhase method");
//        double[] sortedBases = new double[4];
//        int bestPhase = 0;
//        if (m_fixedQuadrant >= 0) {
//            bestPhase = m_fixedQuadrant;
//        } else {
//            double[][] reflection2 = new double[4][];
//            double[] basetop = new double[4];
//            for (int quadPhase = 0; quadPhase < 4; quadPhase++) {
//                setCalibrations(rawData, quadPhase);
//                reflection2[quadPhase] = getAbsvalArray(rawData, m_cal);
//                Arrays.sort(reflection2[quadPhase]);
//                // Look at 95th percentile point
//                int idx = (int)(0.95 * reflection2[quadPhase].length);
//                basetop[quadPhase] = reflection2[quadPhase][idx];
//                sortedBases[quadPhase] = reflection2[quadPhase][idx];
//            }
//            Arrays.sort(sortedBases);
//            if (sortedBases[3] < 1.2) {
//                // All phases give OK correction, so assume 0
//                bestPhase = 0;
//            } else {
//                for (int i = 0; i < 4; i++) {
//                    if (basetop[i] == sortedBases[0]) {
//                        bestPhase = i;
//                    }
//                    Messages.postDebug("QuadPhase",
//                                       "MtuneControl.measureQuadPhase: "
//                                       + "base[" + i + "]=" + basetop[i]);
//                }
//            }
//            Messages.postDebug("QuadPhase", "MtuneControl.measureQuadPhase: "
//                               + "bestPhase=" + bestPhase);
//        }
//        m_master.displayPhase(bestPhase, m_fixedQuadrant >= 0);
//        //if (m_fixedQuadrant < 0 && sortedBases[1] < 1.2) {
//        //    m_cal = null;       // FORCE PHASE RECALCULATION ON NEXT SWEEP
//        //}
//        return bestPhase;
//    }

    /**
     * Compute the absvals of the given raw data implied by the given
     * calibration. The raw data itself is not changed.
     * @return Array of reflection^2 data.
     */
    protected double[] getAbsvalArray(ReflectionData rawData, Calibration cal) {
        int size = rawData.getSize();
        double[] reflection2 = new double[size];

        for (int i = 0; i < size; i++) {
            Complex zm = new Complex(rawData.real[i], rawData.imag[i]);
            reflection2[i] = cal.correctDatum(zm, i).mod2();
        }
        return reflection2;
    }

    /**
     * Apply the given calibration correction to the given data.
     */
    protected boolean correctData(ReflectionData data, Calibration cal) {
        int size = data.getSize();
        double maxAbs = 0;
        Messages.postDebug("MtuneCal", "correctData: using " + cal.toString());
        for (int i = 0; i < size; i++) {
            Complex zm = new Complex(data.real[i], data.imag[i]);
            Complex za = cal.correctDatum(zm, i);
            double x = data.real[i] = za.real();
            double y = data.imag[i] = za.imag();
            data.reflection2[i] = x * x + y * y;
            if (maxAbs < data.reflection2[i]) {
                maxAbs = data.reflection2[i];
            }
        }
        //data.stringData = null;
        data.setMax(maxAbs);
        data.setPhasedDataUsed(true);
        return true;
    }

    protected void setQuadPhase(int quadPhase, boolean isFixed) {
        Messages.postDebug("QuadPhase", "MtuneControl.setQuadPhase("
                           + quadPhase + ", isFixed=" + isFixed);
        if (abs(quadPhase) > 45) {
            quadPhase /= 90;
        }
        m_fixedQuadrant = quadPhase;
        m_master.displayPhase(quadPhase, isFixed);
        m_cal = null;
    }

    public int getQuadPhase() {
        return m_fixedQuadrant;
    }

    protected void saveQuadPhase() { // TODO: Eliminate this
        int phase = m_fixedQuadrant;
        if (phase < 0 && m_cal != null) {
            phase = m_cal.quadPhase.getPhase();
        }
        boolean ok = true;
        String path = getQuadPhaseFilepath();
        if (phase >= 0) {
            ok = TuneUtilities.writeFile(path, phase + NL);
        } else {
            ok = TuneUtilities.writeFile(path, "NaN" + NL);
        }
        new File(path).setWritable(true, false); // Set writable by everybody
        if (!ok) {
            Messages.postWarning("Could not write ProTune phase file: \""
                                 + path + "\"");
        }
    }

    /**
     * Checks that the current calibration correction matches the parameters
     * of this data.
     * All of "startFreq", "stopFreq", "np", "power", and "gain" must match.
     * @param data The data we want to correct.
     * @return True if the current correction matches the parameters of "data".
     */
    protected boolean isCalibrationSetFor(ReflectionData data) {
        long newDate = getLatestCalDate();
        return (m_cal != null
                && m_cal.date >= newDate
                && m_cal.start == data.getStartFreq()
                && m_cal.stop == data.getStopFreq()
                && m_cal.np == data.getSize()
                && m_cal.pwr == data.getPower()
                && m_cal.gain == data.getGain());
    }

    /**
     * Get the latest modification time of any calibration file for the
     * current probe.
     * @return The time in ms.
     */
    public static long getLatestCalDate() {
        long date = 0;
        File dir = null;
        String probe = System.getProperty("apt.probeName");
        String path = getCalDir(probe);
        if (path != null) {
            dir = new File(path);
        }

        if (dir != null) {
            File[] files = dir.listFiles();
            if (files != null) {
                for (File file : files) {
                    date = max(date, file.lastModified());
                }
            }
        }
        return date;
    }

//    /**
//     * Reads in calibrations for the specified data from files.
//     * Calculates the correction coefficients and puts them in m_cal.
//     * @param data The data for which we want the calibration. This is
//     * used just for the center, span, np, and gain of the scan.
//     * @return The Calibration, null if no calibration is available
//     * for these data.
//     */
//    private Calibration setCalibrations(ReflectionData data) {
//        return setCalibrations(data, -99);
//    }

    /**
     * Reads in calibrations for the specified data from the best cal file.
     * Calculates the correction coefficients and puts them in m_cal.
     * @param data The data for which we want the calibration. This is
     * used just for the center, span, np, and gain of the scan.
     * @return True on success, false if no calibration is available
     * for this data.
     */
    private Calibration getCalibrationFor(ReflectionData data) {
        // Check if calibration is already up to date
        if (m_cal != null && m_cal.isGoodFor(data)) {
            return m_cal;
        }

        // Get the raw material for the calibrations
        Calibration cal = null;
        double start_hz = data.getStartFreq();
        double stop_hz = data.getStopFreq();
        int npts = data.getSize();
        int power = data.getPower();
        int gain = data.getGain();
        int rfchan = data.getRfchan();

        String calPath = null;
        QuadPhase quadPhase = null;
        if (m_channelInfo != null) {
            // Use cal file of current channel, if possible
            // These are set when a channel is initialized
            calPath = m_channelInfo.getCalPath();
            quadPhase = m_channelInfo.getQuadPhase();
            Messages.postDebug("MtuneCal", "getCalibrationFor:"
                               + " got cal for channel "
                               + m_channelInfo.getChannelNumber()
                               + ": phase=" + quadPhase
                               + ", calPath=" + calPath);
        }
        if (!calFileIsCompatible(calPath, data)) {
            Messages.postDebug("MtuneCal", "getCalibrationFor:"
                               + "channel cal file is not compatible");
            // Get a cal file that works, but don't make it default for channel
            // Probably got here through a SetCalSweep command
            calPath = getBestCalFilePath(power, gain, rfchan, start_hz, stop_hz);
            quadPhase = null;
        }
        if (calPath == null) {
            Messages.postWarning("No RF calibration for "
                                 + Fmt.f(1, start_hz / 1e6) + " to "
                                 + Fmt.f(1, stop_hz / 1e6) + " MHz");
            return null;
        }
        Complex[][] calData = readCalData(calPath, start_hz, stop_hz, npts);
        if (calData == null) {
            Messages.postDebugWarning("getCalibrationFor: got a bad file: "
                                      + calPath);
            return null;
        }

        if (m_fixedQuadrant >= 0) {
            quadPhase = new QuadPhase();
            quadPhase.setPhase(m_fixedQuadrant);
        }
        if (quadPhase == null || !quadPhase.isGoodQuality()) {
            double delay = readCalProbeDelay(calPath);
            quadPhase = getPhaseFromRefScan(calPath, data, delay);
        }

        if (quadPhase != null) {
            calData = rotateCalibration(calData, quadPhase.getPhase());
        }
        cal = new Calibration(data, calData, calPath, m_channelInfo);
        cal.setPhase(quadPhase);
        return cal;
    }

    private boolean calFileIsCompatible(String calPath, ReflectionData data) {
        if (calPath == null) {
            return false;
        }
        String pattern = getCalNameTemplate(data.getPower(), data.getGain(),
                                            data.getRfchan(), null, null);
        String name = new File(calPath).getName();
        boolean ok = name.matches(pattern);
        double[] range = getFreqRangeOfCalFile(name);
        return ok && range[0] <= data.startFreq && range[1] > data.stopFreq;
    }

    /**
     * Reads in calibrations for the specified data from the best cal file.
     * Calculates the correction coefficients and puts them in m_cal.
     * @param data The data for which we want the calibration. This is
     * used just for the center, span, np, and gain of the scan.
     * @return The calibration, m_cal, null if no calibration is available
     * for these data.
     */
    private Calibration setCalibrations(ReflectionData data,
                                        QuadPhase phase) {
        // Check if this calibration is already set
        Messages.postDebug("MtuneCal" , "setCalibratons: phase=" + phase);
        if (isCalibrationSetFor(data)) {
            //if (quadrant < 0 || m_cal.quadPhase == quadrant) {
            //    Messages.postDebug("MtuneCal",
            //                       "Mtune.setCalibrations(): m_cal already set");
                return m_cal;
            //}
        }

        // Get the raw material for the calibrations
        double start_hz = data.getStartFreq();
        double stop_hz = data.getStopFreq();
        int npts = data.getSize();
        int power = data.getPower();
        int gain = data.getGain();
        int rfchan = data.getRfchan();

        // If calibration is for same channel, use the same calPath
        String calPath = null;
        ChannelInfo calChannel = (m_cal == null) ? null : m_cal.getChannel();
        if (calChannel != null && calChannel == m_channelInfo) {
            calPath = m_cal.getFilePath();
        }
        if (calPath == null) {
            calPath = getBestCalFilePath(power, gain, rfchan, start_hz, stop_hz);
        }
        Complex[][] calData = readCalData(calPath, start_hz, stop_hz, npts);
        if (calData == null) {
            return null;
        }

        if (m_fixedQuadrant >= 0) {
            phase = new QuadPhase(m_fixedQuadrant, QuadPhase.MAX_QUALITY);
            Messages.postDebugWarning("setCalibrations: using fixedQuadrant="
                                      + phase);
        }
        if (phase.getPhase() > 0) {
            calData = rotateCalibration(calData, phase.getPhase());
        }

        m_cal = new Calibration(data, calData, calPath, m_channelInfo);
        if (phase.getPhase() >= 0) {
            m_cal.setPhase(phase);
        }
        return m_cal;
    }

    /**
     * Rotate the Complex data in this calibration through a multiple of PI/2.
     * This puts new Complex numbers in the same data arrays that are
     * passed in.
     *
     * @param data Three arrays of Complex numbers: data[3][len], where
     * "len" is the length of the arrays.
     * @param quadrant What angle to rotate by: 0=no change, 1=PI/2,
     * 2=PI, 3=3*PI/2. Interpreted mod(4).
     * @return Same data array with new Complex numbers, or the same numbers
     * unchanged if quadrant%4=0.
     */
    protected Complex[][] rotateCalibration(Complex[][] data, int quadrant) {
        if ((quadrant % 4) != 0) {
            for (int i = 0; i < 3; i++) {
                Complex.rotateData(data[i], quadrant);
            }
        }
        return data;
    }

    private static int getRfChanFromCalFiles(ChannelInfo ci) {
        double start = ci.getMinSweepFreq();
        double stop = ci.getMaxSweepFreq();
        String calPath = getBestCalFilePath(null, null, null, start, stop);
        return getRfchanFromCalPath(calPath);
    }

    /**
     * Get the RF channel number implied by the name of the calibration
     * file. If it is an old-style calibration with no explicit RF channel,
     * 0 is returned.
     * @param calPath The name or path of the cal file.
     * @return The RF channel number.
     */
    private static int getRfchanFromCalPath(String calPath) {
        int rfchan = 0;
        if (calPath != null) {
            String name = new File(calPath).getName();
            String[] toks = name.split("_");
            if (toks.length == 6) {
                rfchan = Integer.parseInt(toks[3]);
            }
        }
        return rfchan;
    }

    /**
     * Gets the path of the best calibration file for the given sweep range.
     * Looks for the calibration file that
     * covers the frequency range of the data and has the
     * fewest "extra" calibration points off the ends.
     * If the power and/or gain are specified (non-null),
     * they are required to match those of the calibration.
     * If power or gain is not specified, and there are duplicate calibrations
     * with identical "best" ranges, which is returned is indeterminate.
     * <p>
     * The calibration file names are of the form:
     * "cal_pwr_gain_start_stop.zip" where
     * "pwr" and "gain" are the (integer) power and gain used in
     * the calibration sweep (dB), and "start" and "stop" are the
     * (ddd.dddd format) beginning and end of the calibrated range (MHz).
     * @param power The required power setting, or null.
     * @param gain The required gain setting, or null.
     * @param rfchan The RF channel to use, or 0 for "any".
     * @param startHz The start frequency of the data (Hz).
     * @param stopHz The stop frequency of the data (Hz).
     * @return The path to the calibration file, e.g.:
     * "/vnmr/tune/tunecal_myprobe/cal_20_0_252.9875_337.0125.zip",
     * or null if there is no suitable calibration file.
     */
    protected static String getBestCalFilePath(Integer power, Integer gain,
                                               Integer rfchan,
                                               double startHz, double stopHz) {
        // NB: All the "double" frequencies have integral values

        // List files that may contain valid calibrations for this data
        String probe = System.getProperty("apt.probeName");
        String path = getCalDir(probe);
        if (path == null) {
            return null;
        }
        File dir = new File(path);

        String[] patterns = new String[2];
        patterns[0] = getCalNameTemplate(power, gain, rfchan, null, null);
        // NB: patterns[1] looks for old style Cal files w/o rfchan specified
        patterns[1] = getCalNameTemplate(power, gain, -1, null, null);
        File[] files = null;
        File calFile = null;
        Messages.postDebug("MtuneCal", "Mtune.getBestCalFilePath: "
                           + "startHz=" + startHz + ", stopHz=" + stopHz);
        for (int j = 0; calFile == null && j < patterns.length; j++) {
            double bestStartMarg = -1;
            double bestStopMarg = -1;
            double bestMarg = bestStartMarg + bestStopMarg;
            String pattern = patterns[j];
            FilenameFilter filter = new RegexFileFilter(pattern);
            files = dir.listFiles(filter);

            // Look for the file with best frequency range
            for (int i = 0;  files != null && i < files.length; i++) {
                String name = files[i].getName();
                Messages.postDebug("MtuneCal",
                                   "Mtune.getBestCalFilePath: Try: " + name);
                double[] range = getFreqRangeOfCalFile(name);
                double startMarg = startHz - range[0];
                double stopMarg = range[1] - stopHz;
                if (DebugOutput.isSetFor("Mtune.cal")
                        && startMarg >= 0 && stopMarg >= 0)
                {
                    Messages.postDebug("MtuneCal", "Mtune.getBestCalFilePath: "
                                       + "This one will work: " + name);
                }
                if (startMarg >= 0 && stopMarg >= 0
                        && isValidCalFile(files[i])
                        // This one will work; is it the best so far?
                        && ((bestStartMarg < 0 || bestStopMarg < 0) // Only one
                            || (startMarg + stopMarg < bestMarg) // Best
                            || ((startMarg + stopMarg == bestMarg)
                                && (startMarg < bestStartMarg)) // Tie breaker
                        )
                )
                {
                    Messages.postDebug("MtuneCal", "Mtune.getBestCalFilePath: "
                                       + "Best so far is: " + name);
                    bestStartMarg = startMarg;
                    bestStopMarg = stopMarg;
                    bestMarg = bestStartMarg + bestStopMarg;
                    calFile = files[i];
                }
            }
        }

        if (calFile == null) {
            // No files with required power, gain, and frequency range
            String strPwr = "";
            if (power != null) {
                strPwr = "power=" + power + ", ";
            }
            String strGain = "";
            if (gain != null) {
                strGain = "gain=" + gain + ", ";
            }
            String strChan = "";
            if (rfchan != null) {
                strChan = "rfchan=" + rfchan + ", ";
            }
            Messages.postDebugWarning("No cal file for " + strPwr + strGain
                                      + strChan
                                      + Fmt.f(1, startHz / 1e6) + " - "
                                      + Fmt.f(1, stopHz / 1e6) + " MHz "
                                      + "in " + dir.getAbsolutePath());
            return null;
        }
        return calFile.getPath();
    }

    /**
     * Deduce the range of frequencies in a calibration file from its name
     * or path.
     * @param name The name of the calibration file.
     * @return The start and stop frequencies, respectively (Hz).
     */
    private static double[] getFreqRangeOfCalFile(String name) {
        double[] range = new double[2];
        int idx = name.lastIndexOf(".");
        int idx2 = name.lastIndexOf("_");
        range[1] = Double.parseDouble(name.substring(idx2+1, idx));
        range[1] = rint(range[1] * 1e6);
        idx = idx2;
        idx2 = name.lastIndexOf("_", idx-1);
        range[0] = Double.parseDouble(name.substring(idx2+1, idx));
        range[0] = rint(range[0] * 1e6);
        return range;
    }

    /**
     * Gets calibration file path for exactly the given sweep range.
     * Names are like: "cal_pwr_gain_rfchan_start_stop.zip" where
     * "pwr" and "gain" are the power and gain used in
     * the calibration sweep, and "start" and "stop" are the beginning
     * and end of the calibration sweep in MHz.
     * Looks for calibration files that have the same frequency range
     * that we specify.
     *
     * @param startHz The start frequency of the data (Hz).
     * @param stopHz The stop frequency of the data (Hz).
     * @param rfchan TODO
     * @return The path to the calibration file or null, e.g.:
     * "/vnmr/tune/tunecal_myprobe/cal_20_0_1_252.9875_337.0125.zip".
     */
    protected static String getExactCalFilePath(double startHz,
                                                double stopHz, int rfchan) {
        // NB: The "double" frequencies have exact integral values
        String filepath = null;
        String probe = System.getProperty("apt.probeName");
        String dir = getCalDir(probe);
        String pattern = getCalNameTemplate(null, null, rfchan, startHz, stopHz);
        FilenameFilter filter = new RegexFileFilter(pattern);
        File[] fileList = new File(dir).listFiles(filter);
        if (fileList != null && fileList.length > 0) {
            filepath = fileList[0].getAbsolutePath();
        }
        return filepath;
    }

    /**
     * Delete (back up) all the files in the given
     * directory that match the given regular expression.
     * @see #moveFileToBackup(String)
     * @param dir The directory to look in.
     * @param nameRegex The regex to match the filenames.
     */
    public static void deleteCalFiles(String dir, String nameRegex) {
        FilenameFilter filter = new RegexFileFilter(nameRegex);
        String[] paths = new File(dir).list(filter);
        for (String path : paths) {
            moveFileToBackup(dir + "/" + path);
        }
    }

    /**
     * Delete (move to backup directory) the calibration file for this band.
     * @param calBand The calibration band
     * @return True if successfully deleted (backed up).
     */
    public static boolean deleteCalFile(CalBand calBand) {
        boolean ok = false;
        String path = getExactCalFilePath(calBand.getStart(), calBand.getStop(),
                                          calBand.getRfchan());
        if (path != null) {
            ok = moveFileToBackup(path);
        }
        return ok;
    }

    protected static boolean isValidCalFile(File file) {
        return file.canRead();
    }

    /**
     * Extract the calibration reference points from a given data set.
     * Get only points for the given frequency range and resolution.
     * @param calData The raw data.
     * @param nDataPts The number of data points to extract.
     * @param start_hz The frequency of the first point to extract.
     * @param stop_hz The frequency of the last point to extract.
     * @param exclude A range of data points to ignore, or null.
     * @return A complex array of nDataPts calibration points, or null
     * on failure.
     */
    protected static Complex[] extractCalData(ReflectionData calData,
                                              int nDataPts,
                                              double start_hz,
                                              double stop_hz,
                                              Interval exclude) {

        Messages.postDebug("CalData", "extractCalData(calData, "
                           + nDataPts + ", start=" + Fmt.f(4, start_hz/1e6)
                           + ", stop=" + Fmt.f(4, stop_hz/1e6) + ")");
        if (calData == null) {
            Messages.postDebugWarning("extractCalData: calData=null");
            return null;
        }

        // Extract the points that we need
        double step = (stop_hz - start_hz) / (nDataPts - 1);

        int calSize = calData.getSize();
        double calStart = calData.getStartFreq();
        double calStop = calData.getStopFreq();
        double calStep = (calStop - calStart) / (calSize - 1);

        int idx0 = (int)round((start_hz - calStart) / calStep);
        int idxInc = (int)round(step / calStep);

        if (idx0 < 0 || idx0 + (nDataPts - 1) * idxInc > calSize) {
            Messages.postDebugWarning("extractCalData: idx0=" + idx0 + ", last="
                                      + (idx0 + (nDataPts - 1) * idxInc)
                                      + ", size=" + calSize);
            return null;
        }

        Complex[] cal = new Complex[nDataPts];
        for (int i = 0, j = idx0; i < nDataPts; i++, j += idxInc) {
            if (exclude != null && exclude.contains(j)) {
                cal[i] = new Complex(0, 0);
            } else {
                cal[i] = new Complex(calData.real[j], calData.imag[j]);
            }
        }

        return cal;
    }

    /**
     * Gets calibration data for sweep range in a data set
     * from a Zip format calibration file.
     * File name is like: "cal_pwr_gain_start_stop.zip" where
     * "pwr" and "gain" are the power and gain used in
     * the calibration sweep, and "start" and "stop" are the beginning
     * and end of the calibration sweep in MHz.
     * Returns data from the "Short", "Load", and "Open" entries in the
     * Zip file.
     *
     * @return Three arrays of Complex values, giving the data in
     * "Short", "Load", and "Open", respectively, Returns
     * null if any of the three entries is unavailable or faulty.
     */
    private static Complex[][] readCalData(String path, ReflectionData data) {
        double start = data.startFreq;
        double stop = data.stopFreq;
        int np = data.size;
        return readCalData(path, start, stop, np);
    }

    /**
     * Gets calibration data for sweep range in a data set
     * from a Zip format calibration file.
     * File name is like: "cal_pwr_gain_start_stop.zip" where
     * "pwr" and "gain" are the power and gain used in
     * the calibration sweep, and "start" and "stop" are the beginning
     * and end of the calibration sweep in MHz.
     * Returns data from the "Short", "Load", and "Open" entries in the
     * Zip file.
     *
     * @return Three arrays of Complex values, giving the data in
     * "Short", "Load", and "Open", respectively, Returns
     * null if any of the three entries is unavailable or faulty.
     */
    protected static Complex[][] readCalData(String calPath,
                                             double start_hz,
                                             double stop_hz,
                                             int npts) {
        if (calPath == null) {
            return null;
        }

        // Read the calibration files
        Complex[][] cals = new Complex[3][];
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(calPath);
            String[] entries = {"CalOpen", "CalLoad", "CalShort"};
            for (int i = 0; i < 3; i++) {
                ZipEntry entry = zipFile.getEntry(entries[i]);
                InputStream in = zipFile.getInputStream(entry);
                BufferedReader reader;
                reader = new BufferedReader(new InputStreamReader(in));
                ReflectionData calData = readRawData(0, reader);
                cals[i] = extractCalData(calData, npts, start_hz, stop_hz, null);
                if (cals[i] == null) {
                    Messages.postError("Invalid calibration entry \""
                                       + entries[i] + "\" in: " + calPath);
                    return null;
                }
            }
        } catch (IOException e) {
            Messages.postError("Error reading calibration in \""
                               + calPath + "\": " + e);
        } finally {
            try { zipFile.close(); } catch (Exception e) {}
        }
        return cals;
    }

    /**
     * Write out calibration file into the system tunecal directory for
     * the current probe.
     * Requires the three calibration scans
     * and the {@link CalBand#CalBand(String) CalBand} specification string.
     * @param data Array of 5 calibration scans:
     * open, load, short, probe, and sigma, respectively
     * @param spec The CalBand specification.
     * @param probeDelay The probeDelay for this calibration (ns).
     * @param exclude A range of data points to ignore, or null.
     * @return True if successful.
     */
    public boolean writeCalData(ReflectionData[] data, String spec,
                                double probeDelay, Interval exclude) {
        String probe = System.getProperty("apt.probeName");
        File dir = new File(getCalDir(probe));
        if (!dir.exists()) {
            dir.mkdirs();
        }
        if (!dir.canWrite()) {
            Messages.postError("MtuneControl.writeCalData: cannot write to \""
                               + dir + "\" directory");
            return false;
        }

        // Construct the output filename from data parameters
        int power = data[0].getPower();
        int gain = data[0].getGain();
        int rfchan = data[0].getRfchan();
        double start = data[0].getStartFreq();
        double stop = data[0].getStopFreq();
        String basename = "cal" + "_" + power + "_" + gain + "_" + rfchan
                          + "_" + getCalFreqName(start)
                          + "_" + getCalFreqName(stop);
        String path = dir.getPath() + File.separator + basename;
        return writeCalData(path, data, spec, probeDelay, exclude);
    }

    /**
     * Converts the start or stop frequency in Hz into a string giving the
     * frequency in MHz.
     * This is the string used in the name of the calibration file.
     * @param freq The frequency (Hz).
     * @return The string for the file name.
     */
    public static String getCalFreqName(double freq) {
        return Fmt.fg(6, freq * 1e-6);
    }

    /**
     *
     * @param power Transmitter power or "null".
     * @param gain Receiver gain or "null".
     * @param rfchan The RF channel to use, or null or 0 for "any".
     * @param start Sweep start frequency or "null" (Hz).
     * @param stop Sweep stop frequency or "null" (Hz).
     * @return The regular expression as a String.
     */
    public static String getCalNameTemplate(Integer power, Integer gain,
                                            Integer rfchan,
                                            Double start, Double stop) {
        String name = "cal_";
        if (power == null) {
            name += "-?[0-9]+_";
        } else {
            name += power + "_";
        }
        if (gain == null) {
            name += "-?[0-9]+_";
        } else {
            name += gain + "_";
        }
        if (rfchan == null || rfchan == 0) {
            name += "[0-9]+_"; // Cal file specifies any RF chan
        } else if (rfchan < 0) {
            // Look for an "old" cal file w/ no rfchan specified
        } else {
            name += rfchan + "_";
        }
        if (start == null) {
            name += "[0-9]*\\.[0-9]*_";
        } else {
            start = adjustCalSweepStart(start);
            name += getCalFreqName(start) + "_";
        }
        if (stop == null) {
            name += "[0-9]*\\.[0-9]*";
        } else {
            stop = adjustCalSweepStop(stop);
            name += getCalFreqName(stop);
        }
        name += ".zip";
        return name;
    }

    /**
     * Saves the given calibration data to the given file path.
     * Will only work for the Vnmr administrator, unless the permissions
     * on the tunecal directory have been customized.
     * Data will be written into a Zip file, with a separate entry for each
     * of the 3 data sets (open, load, and short).
     * @param path The path to write the data to (without "zip" extension).
     * @param data The 5 reflection data sets to save in the calibration,
     * open, load, short, probe, and sigma, respectively.
     * @param spec The Band Specification string that will construct
     * the corresponding {@link CalBand#CalBand(String) CalBand} object.
     * @param probeDelay The probe delay in ns.
     * @param exclude A range of data points to ignore, or null.
     * @return True if successful.
     */
    public boolean writeCalData(String path, ReflectionData[] data, String spec,
                                double probeDelay, Interval exclude) {
        ZipOutputStream out = null;
        path += ".zip";
        File file = new File(path);
        if (file.exists()) {
            if (!moveFileToBackup(path)) {
                String title = "Calibration_Not_Saved";
                String msg = "Could not back up old calibration data in<br>"
                        + path
                        + "<br>New calibration was not saved.";
                m_master.sendToGui("popup error title " + title + " msg " + msg);
                return false;
            }
        }

        // Delete (back up) all cal files with this freq range
        double start = data[0].startFreq;
        double stop = data[0].stopFreq;
        int rfchan = data[0].getRfchan();
        String nameRegex = getCalNameTemplate(null, null, rfchan, start, stop);
        Messages.postDebug("MtuneCal",
                "Delete files matching regex: " + nameRegex);
        deleteCalFiles(file.getParent(), nameRegex);

        boolean isError = false;
        try {
            out = new ZipOutputStream(new FileOutputStream(path));
            zipCalData(data[0], OPEN_CAL_NAME, out);
            zipCalData(data[1], LOAD_CAL_NAME, out);
            zipCalData(data[2], SHORT_CAL_NAME, out);
            zipCalData(data[3], PROBE_CAL_NAME, out);
            zipCalData(data[4], SIGMA_CAL_NAME, out);
            zipString(spec, SPEC_CAL_NAME, out);
            zipString(probeDelay + "", DELAY_CAL_NAME, out);
            zipString(exclude.toString(), DIP_REGION_CAL_NAME, out);
            String v = ProbeTune.getSWVersion() + ": " + ProbeTune.getSWDate();
            zipString(v, VERSION_CAL_NAME, out);
            out.finish();
            QuadPhase phase = new QuadPhase();
            String key = new File(path).getName();
            m_phaseMap.put(key, phase);
            QuadPhase.writePhase(path, phase);
        } catch (FileNotFoundException e) {
            Messages.postDebugWarning("Cannot write RF cal file: " + path);
            isError = true;
        } catch (IOException e) {
            Messages.postDebugWarning("Error writing RF cal file: " + path);
            isError = true;
        } catch (NullPointerException e) {
            Messages.postDebugWarning("Null pointer writing: \"" + path + "\"");
            isError = true;
        } finally {
            try { out.close(); } catch (Exception e) {}
        }
        if (isError) {
            String title = "Calibration_Not_Saved";
            String msg = "Could not save calibration data in<br>" + path;
            m_master.sendToGui("popup error title " + title + " msg " + msg);

        }
        return (!isError);
    }

    /**
     * Saves the calibration data to the given Zip stream.
     * @param data The reflection data to save as the calibration.
     * @param entryName The name of the entry to write into the Zip file.
     * @param out The ZipOutputStream to write to.
     */
    private boolean zipCalData(ReflectionData data,
                               String entryName, ZipOutputStream out) {
        return zipString(data.getStringData("", NL), entryName, out);
    }

    /**
     * Writes given string to the given Zip stream with a given entry name.
     * @param data The string to write.
     * @param name The name of the entry in the Zip stream.
     * @param out The ZipOutputStream to write to.
     */
    private boolean zipString(String data, String name, ZipOutputStream out) {
        // Write given raw data to the appropriate cal file entry
        boolean ok = false;

        // The input data as a byte array (using UTF-8 encoding):
        byte[] byteData = data.getBytes(Charset.forName("UTF-8"));

        // The output entry name comes from the specified "type"
        ZipEntry entry = new ZipEntry(name);
        entry.setSize(byteData.length);
        CRC32 crc = new CRC32();
        crc.reset();
        crc.update(byteData);
        entry.setCrc(crc.getValue());
        try {
            out.putNextEntry(entry);
            out.write(byteData, 0, byteData.length);
        } catch (IOException e) {
            Messages.postError("Cannot write " + name + " entry to Zip file");
        }
        return ok;
    }

    /**
     * Return the (static) instance of the DateFormat used to add a date
     * stamp to a file name.
     */
    protected static DateFormat getTinyDateFormat() {
        return m_tinyDateFormat;
    }

    /**
     * Move the given file to a subdirectory named "bak", with a date stamp
     * prepended to the file name.
     * @param filepath The full path to the existing file.
     */
    protected static boolean moveFileToBackup(String filepath) {
        boolean ok = true;
        File infile = new File(filepath);
        if (infile.exists()) {
            ok = false;
            try {
                File indir = infile.getParentFile();
                File outdir = new File (indir + File.separator + "bak");
                outdir.mkdirs();
                outdir.setWritable(true, false); // Give all write permission
                long date = infile.lastModified();
                String strDate = getTinyDateFormat().format(new Date(date));
                File  outfile = new File(outdir, strDate + infile.getName());
                ok = infile.renameTo(outfile);
            } catch (SecurityException se) {
            }
        }
        return ok;
    }

    /**
     * Adjusts the sweep parameters to something that is compatible with
     * what is in the calibration files. The number of points will be set
     * arbitrarily to some reasonable number!
     * @param center Desired center frequency (Hz).
     * @param span Desired span (Hz).
     * @param np Not used.
     * @param force If true, send sweep change even if it seems pointless.
     * @return An array of 3 doubles giving the modified center, span, and
     * number of points, respectively.
     */
    public double[] adjustSweep(double center, double span,
                                int np, boolean force) {
        Messages.postDebug("MtuneControl",
                           "adjustSweep(" + Fmt.f(6, center/1e6)
                           + ", " + Fmt.f(6, span/1e6) + ", " + np + ")");

        int minPts = MAX_PTS / 2; // Minimum number of points to put in a sweep
        if (force) {
            minPts = 32;
        }
        if (span > m_maxFreq - m_minFreq) {
            minPts = (int)((m_maxFreq - m_minFreq) / FREQ_RESOLUTION);
        }

        // Get a legal start/stop range
        double start = center - span / 2;
        double stop = center + span / 2;
        double minSpan = max(span, FREQ_RESOLUTION * minPts);
        minSpan = min(minSpan, m_maxFreq - m_minFreq);
        double extraSpan = minSpan - (stop - start);
        if (extraSpan > 0) {
            start -= extraSpan / 2;
            stop += extraSpan / 2;
        }
        if (start < m_minFreq) {
            start = m_minFreq;
            stop = max(stop, m_minFreq + minSpan);
            stop = min(stop, m_maxFreq);
        } else if (stop > m_maxFreq) {
            stop = m_maxFreq;
            start = min(start, m_maxFreq - minSpan);
            start = max(start, m_minFreq);
        }

        // Outer limits of start/stop frequencies -- offset by +FREQ_OFFSET Hz
        double startOff = start + FREQ_OFFSET;
        double stopOff = stop + FREQ_OFFSET;

        Messages.postDebug("MtuneControl", "adjustSweep: "
                           + "m_minFreq=" + Fmt.g(6, m_minFreq/1e6)
                           + ", m_maxFreq=" + Fmt.g(6, m_maxFreq/1e6)
                           + "; startOff=" + Fmt.f(6, startOff/1e6)
                           + ", stopOff=" + Fmt.f(6, stopOff/1e6));

        // Position of start/stop frequencies in units of FREQ_RESOLUTION
        double nStart = ceil(startOff / FREQ_RESOLUTION);
        double nStop = floor(stopOff / FREQ_RESOLUTION);

        Messages.postDebug("MtuneControl", "adjustSweep: nStart="
                           + Fmt.f(0, nStart) + ", nStop=" + Fmt.f(0, nStop));

        // Number of data points before clumping
        int npts = (int)round(nStop - nStart + 1);
        // Number of FREQ_RESOLUTION units we need between data points
        int clump = max(1, (npts - 1 + (MAX_PTS - 2)) / (MAX_PTS - 1));
        // Number of pts left out of final sweep range at ends
        int discards = (npts - 1) % clump;
        // Number of data points after clumping
        npts = 1 + (npts - 1) / clump;
        // Correct start/stop positions for clumping
        nStart += discards / 2;
        nStop -= discards - discards / 2;

        // Final, adjusted start/stop range
        start = nStart * FREQ_RESOLUTION - FREQ_OFFSET;
        stop = nStop * FREQ_RESOLUTION - FREQ_OFFSET;

        // Return answer in terms of center and span
        double[] rtn = new double[3];
        rtn[0] = (stop + start) / 2;
        rtn[1] = stop - start;
        rtn[2] = npts;

        Messages.postDebug("MtuneControl", "adjustSweep to: "
                           + "start=" + Fmt.f(6, start / 1e6)
                           + ", stop=" + Fmt.f(6, stop / 1e6)
                           + ", np=" + npts);
        return rtn;
    }

    /**
     * Calculates the number of extra data points needed to make a
     * unique raw "np" -- different from any np in the list of
     * requested scans.
     * @param nRawPoints The number of points we want.
     * @return The number of additional points we need ask for to avoid
     * conflicting with a previous request.
     */
    private int getPaddingForUniqueNp(int nRawPoints) {
        SortedSet<Integer> npCollection = new TreeSet<Integer>();
        for (FidData fidData : m_requestedSweeps) {
            if (fidData != null) {
                npCollection.add(fidData.getNRawPoints());
            }
        }
        int np = nRawPoints;
        while (npCollection.contains(np)) {
            np++;
        }
        return np - nRawPoints;
    }

    /**
     * Change the power and gain of the current sweep.
     * The center, span, and np values are kept the same as the last sweep;
     * @param power The desired transmitter power (dB).
     * @param gain The desired receiver gain (dB).
     * @param rfchan TODO
     * @return True if the request was sent successfully; this does not
     * necessarily guarantee that Vnmr will respond to the request.
     */
    public boolean setPowerAndGain(int power, int gain, int rfchan) {
        Messages.postDebug("Autogain", "setPowerAndGain: "
                           + "old values: pwr=" + getCurrentPower()
                           + ", gain=" + getCurrentGain()
                           + ", new values: pwr=" + power + ", gain=" + gain);
        FidData prevFid = m_requestedSweeps.getLast();
        if (prevFid == null) {
            return false;
        }
        return sendSweepToVnmr(getCurrentCenter(),
                               getCurrentSpan(),
                               getCurrentNp(),
                               power, gain, rfchan);
    }

    /**
     * Sends a request to Vnmr to set parameters of the scan to
     * specified values.
     * The scan request sent to Vnmr may be "padded" with extra points
     * for 2 reasons:
     * <OL>
     * <LI> The digital filtering on VnmrS consoles makes it hard to
     * guarantee that the first and last data points in a scan are valid,
     * so we add one extra point to the beginning and another to the end
     * of the scan.
     * <LI> Since the only information in the FID files about the scan
     * parameters is "np", we want to make the number of points in the
     * FID file different from what we're currently getting (and also
     * different from what is in any pending requests) so we can tell
     * when the data for this request starts coming in. This may result
     * in one or more additional padding points being added to the end
     * of the scan.
     * </OL>
     *
     * @param center The center frequency of the scan in Hz.
     * @param span The the width of the scan in Hz.
     * @param np The number of frequencies to sample in the scan.
     * @param power The transmitter power (tupwr) to use.
     * @param gain The receiver gain (gain) to use.
     * @param rfchan TODO
     * @return True if the request was sent successfully; this does not
     * necessarily guarantee that Vnmr will respond to the request.
     */
    private boolean sendSweepToVnmr(double center, double span,
                                    int np, int power, int gain, int rfchan) {
        showSweepLimitsInGui();
        double start_hz = center - (span / 2);
        double stop_hz = center + (span / 2);

        int nLeadingPadPoints = 1;
        int nTrailingPadPoints = 1;
        int nRawPoints = np + nLeadingPadPoints + nTrailingPadPoints;
        int extraPadding = getPaddingForUniqueNp(nRawPoints);
        nTrailingPadPoints += extraPadding;
        nRawPoints += extraPadding;
        //if (m_currSweep != null && nRawPoints == m_currSweep.getNRawPoints()) {
        //    nTrailingPadPoints++;
        //    nRawPoints++;
        //}
        Messages.postDebug(//"MtuneControl",
                           "MtuneControl.sendSweepToVnmr("
                           + "(center=" + Fmt.f(6, center/1e6)
                           + ", span=" + Fmt.f(6, span/1e6)
                           + ", npts=" + np
                           + ", power=" + power
                           + ", gain=" + gain
                           + ", rfchan=" + rfchan
                           + ", rawPts=" + nRawPoints
                           + ")");
        double step = span / (np - 1);
        span += (nLeadingPadPoints + nTrailingPadPoints) * step;
        center += (nTrailingPadPoints - nLeadingPadPoints) * (step / 2);

        boolean ok = m_master.queueToVnmr("sfrq=" + (center / 1e6) + NL
                                          + "tunesw=" + span + NL
                                          + "np=" + (2 * nRawPoints) + NL
                                          + "tupwr=" + power + NL
                                          + "gain=" + gain + NL
                                          + "tchan=" + rfchan + NL
                                          );

        if (ok) {
            // Key the parameters of the data (start, stop, etc.) with the
            // number of points in the FID we receive. There is nothing in
            // the FID file to specify these parameters.
            m_requestedSweeps.remove(null); // Remove possible null placeholder
            m_sweepResetDate = System.currentTimeMillis();
            m_requestedSweeps.add(new FidData(nRawPoints,
                                              nLeadingPadPoints,
                                              nTrailingPadPoints,
                                              start_hz, stop_hz,
                                              power, gain, rfchan));
            Messages.postDebug("FidData", "sendSweepToVnmr requested sweep: "
                               + "np=" + nRawPoints + ", rfchan=" + rfchan);
        }

        if (isTuneBandSwitchable()) {
            try {
                setTuneBand(center);
            } catch (InterruptedException e) {}
        }
        return ok;
    }

    public void setSweep(double center, double span, int np,
                         int rfchan, boolean force) {

        Messages.postDebug("MtuneControl",
                           "MtuneControl.setSweep(" + center + ", " + span
                           + ", " + np + ", " + force + ")");
        if (np < 1) {
            np = getNp();
        }
        // NB: For now, adjustSweep ignores the input np and assigns a value.
        double[] sweep = adjustSweep(center, span, np, force);
        double newCenter = sweep[0];
        double newSpan = sweep[1];
        int newNp = (int)sweep[2];

        FidData prevFid = m_requestedSweeps.getLast();
        double prevStart_hz = prevFid == null ? 0 : prevFid.start_hz;
        double prevStop_hz = prevFid == null ? 0 : prevFid.stop_hz;
        double prevNp = prevFid == null ? 0 : prevFid.getNGoodDataPoints();
        int prevRfchan = prevFid == null ? 0 : prevFid.getRfchan();

        // Unless force is specified, don't change the sweep unless it
        // will potentially give more information.
        // When checking resolution, we use the requested, unadjusted span
        // and np, because that indicates the resolution we think we need.
        double prevResolution = (prevStop_hz - prevStart_hz) / (prevNp - 1);
        double newStart = newCenter - newSpan / 2;
        double newStop = newCenter + newSpan / 2;
        if (force
            || prevStart_hz < m_minFreq
            || prevStop_hz > m_maxFreq
            || newStart < prevStart_hz - 1
            || newStop > prevStop_hz + 1
            || rfchan != prevRfchan
            || (span / (np - 1) < prevResolution
                && newSpan / (newNp - 1) < prevResolution))
        {
            // We're asking for more range or more resolution or using force
            // ... so do it
            super.setSpan(newSpan);
            super.setCenter(newCenter);
            super.setNp(newNp);

            // Default power and gain:
            int power = getPowerAt(newCenter);
            int gain = getGainAt(newCenter);
            Messages.postDebug("PowerAndGain","setSweep: Default power="
                               + power + ", gain=" + gain);

            // See if we have a power and gain for this sweep range
            int[] pwrAndGain = getPowerAndGainFor(newStart, newStop, rfchan);
            Messages.postDebug("PowerAndGain","setSweep: pwrAndGain="
                               + pwrAndGain);
            if (pwrAndGain != null) {
                power = pwrAndGain[0];
                gain = pwrAndGain[1];
                Messages.postDebug("PowerAndGain","setSweep: power="
                                   + power + ", gain=" + gain);
            }
            sendSweepToVnmr(newCenter, newSpan, newNp, power, gain, rfchan);
        }
    }

    /**
     * Get the power and gain values for the calibration covering a given range.
     * Picks the calibration "best" fitting the given range.
     * @param startHz Beginning of the range (Hz).
     * @param stopHz End of the range (Hz).
     * @param rfchan TODO
     * @return Array of 2 ints giving power and gain, respectively.
     */
    private int[] getPowerAndGainFor(double startHz, double stopHz,
                                     int rfchan) {
        boolean ok = false;
        int[] powerAndGain = new int[2];
        String path = null;
        if (m_channelInfo != null) {
            path = m_channelInfo.getCalPath();
        }
        if (path == null) {
            path = getBestCalFilePath(null, null, rfchan, startHz, stopHz);
        }
        if (path != null) {
            String name = new File(path).getName();
            String[] tokens = name.split("_");
            try {
                powerAndGain[0] = Integer.parseInt(tokens[1]);
                powerAndGain[1] = Integer.parseInt(tokens[2]);
                ok = true;
            } catch (NumberFormatException nfe) {
                Messages.postDebugWarning("getPowerAndGainFor: bad cal file: \""
                                   + name + "\"");
            } catch (IndexOutOfBoundsException ioobe) {
                Messages.postDebugWarning("getPowerAndGainFor: Bad cal file: \""
                                   + name + "\"");
            }
        }
        Messages.postDebug("PowerAndGain",
                           "getPowerAndGainFor: "
                           + "start=" + Fmt.f(4, startHz / 1e6)
                           + ", stop=" + Fmt.f(4, stopHz / 1e6)
                           + ", pwr=" + powerAndGain[0]
                           + ", gain=" + powerAndGain[1]
                           + ", path=" + path);
        if (!ok) {
            powerAndGain = null;
            //powerAndGain[0] = getCurrentPower();
            //powerAndGain[1] = getCurrentGain();
        }
        return powerAndGain;
    }

    /**
     * Get the last span sent to Vnmr.
     * @return The span (Hz), or NaN if unknown.
     */
    public double getCurrentSpan() {
        FidData prevFid = m_requestedSweeps.getLast();
        if (prevFid == null) {
            return Double.NaN;
        }
        return (prevFid.stop_hz - prevFid.start_hz);
    }

    /**
     * Get the center frequency sent to Vnmr.
     * @return The center frequency (Hz), or NaN if unknown.
     */
    public double getCurrentCenter() {
        FidData prevFid = m_requestedSweeps.getLast();
        if (prevFid == null) {
            return Double.NaN;
        }
        return (prevFid.start_hz + prevFid.stop_hz) / 2;
    }

    /**
     * Get the last np sent to Vnmr.
     * @return The number of complex points, or 0 if unknown.
     */
    public int getCurrentNp() {
        FidData prevFid = m_requestedSweeps.getLast();
        return (prevFid == null) ? 0 : prevFid.getNGoodDataPoints();
    }

    /**
     * Get the last power sent to Vnmr.
     * @return The power (tupwr parameter, dB), or Integer.MIN_VALUE if unknown.
     */
    public int getCurrentPower() {
        FidData prevFid = m_requestedSweeps.getLast();
        return (prevFid == null) ? Integer.MIN_VALUE : prevFid.getPower();
    }

    /**
     * Get the last gain sent to Vnmr.
     * @return The gain (gain parameter, dB), or Integer.MIN_VALUE if unknown.
     */
    public int getCurrentGain() {
        FidData prevFid = m_requestedSweeps.getLast();
        return (prevFid == null) ? Integer.MIN_VALUE : prevFid.getGain();
    }

    /**
     * Get the last gain sent to Vnmr.
     * @return The gain (gain parameter, dB), or Integer.MIN_VALUE if unknown.
     */
    public int getCurrentRfchan() {
        FidData prevFid = m_requestedSweeps.getLast();
        return (prevFid == null) ? Integer.MIN_VALUE : prevFid.getRfchan();
    }

    /**
     * Get the RF channel.
     * @return The current RF channel, or 0 if unknown.
     */
    public int getRfchan() {
        return m_rfchan;
    }

    /**
     * Set the RF channel.
     * @param rfchan The RF Channel, or 0 if unknown.
     */
    public void setRfchan(int rfchan) {
        m_rfchan = rfchan;
    }

    public void setExactSweep(double center, double span, int np, int rfchan) {
        Messages.postDebug("MtuneControl",
                           "setExactSweep(" + center + ", " + span
                           + ", " + np + ")");
        if (np < 1) {
            np = getNp();
        }
        if (rfchan < 1) {
            double start = center - span / 2;
            double stop = center + span / 2;
            String calPath = getBestCalFilePath(null, null, null, start, stop);
            rfchan = getRfchanFromCalPath(calPath);
        }

        boolean changed = false;
        if (span != m_stopFreq - m_startFreq) {
            super.setSpan(span);
            changed = true;
        }
        if (center != getCenter()) {
            super.setCenter(center);
            changed = true;
        }
        if (np != getNp()) {
            super.setNp(np);
            changed = true;
        }
        if (rfchan != getRfchan()) {
            setRfchan(rfchan);
            changed = true;
        }

        if (changed) {
            // Default power and gain:
            int power = getPowerAt(center);
            int gain = getGainAt(center);

            // See if we have a power and gain for this sweep range
            double start = center - span / 2;
            double stop = center + span / 2;
            int[] pwrAndGain = getPowerAndGainFor(start, stop, rfchan);
            if (pwrAndGain != null) {
                power = pwrAndGain[0];
                gain = pwrAndGain[1];
            }

            // Write out the command file
            sendSweepToVnmr(center, span, np, power, gain, rfchan);
            //m_settingSweep = true;
        }
    }


    public void setSpan(double span) {
        if (span != getSpan()) {
            super.setSpan(span);
            double center = getCenter();
            int np = getNp();
            setSweep(center, span, np, getRfchan());
        }
    }

    public void setCenter(double center) {
        if (center != getCenter()) {
            super.setCenter(center);
            double span = getSpan();
            int np = getNp();
            setSweep(center, span, np, getRfchan());
        }
    }

    public void setNp(int np) {
        if (np != getNp()) {
            super.setNp(np);
            double center = getCenter();
            double span = getSpan();
            setSweep(center, span, np, getRfchan());
        }
    }

    public static double getFreqResolution() {
        return FREQ_RESOLUTION;
    }

    public double getMinFreqStep() {
        return getFreqResolution();
    }

    public double getFreqStepOffset() {
        return getFreqResolution() / 2;
    }

    /**
     * Gets the list of directories that might have calibration files
     * for the given probe.
     * Directories are listed in order of priority, highest first.
     * <br>
     * Currently, the length of the list will be 1 or 0.
     * @param probeName The probe name (Vnmr "probe" parameter).
     * @return The list of directories.
     */
    public static ArrayList<File> getCalDirs(String probeName) {
        ArrayList<File> dirs = new ArrayList<File>();
        String path = getCalDir(probeName);
        if (path != null) {
            dirs.add(new File(path));
        }
        return dirs;
    }

//    /**
//     * Gets the dirpath to look in for the calibration data.
//     * @return Returns m_calDirectory.
//     */
//    public static String getCalDir() {
//        return m_calDirectory;
//    }

    /**
     * Gets the dirpath to look in for the calibration data.
     * Returns the path to the probe-specific directory if it exists
     * or can be created (and it is created).
     * @param probeName The probe name.
     * @return The full path of the directory, or null.
     */
    public static String getCalDir(String probeName) {
        String dir = null;
        if (ProbeTune.useProbeId()) {
            String subdir = BASE_CAL_PARENTDIR + File.separator
            + BASE_CAL_SUBDIR + "_" + probeName;
            File blob = ProbeTune.getProbe().blobWrite(".",
                                                       subdir, false, false);
            return blob == null ? null : blob.getPath();
        }
        if (probeName != null && probeName.length() > 0) {
            dir = BASE_CAL_DIR + "_" + probeName;
            File fDir = new File(dir);
            fDir.mkdirs();
            fDir.setWritable(true, false); // Give everyone write permission
            if (!fDir.isDirectory()) {
                dir = null;
            }
        }
        return dir;
    }

    public void clearCal() {
        m_cal = null;
    }

    /**
     * Get a FidData instance with no data, but with header info expected
     * for a FID acquired with the given number of points.
     * The sweep info is based on what has been sent to Vnmr.
     * @param np The number of complex points that the raw FID was
     * acquired with.
     * @return A FidData with everything initialized except the data.
     * Or null, if the specified np is unexpected.
     */
    private FidData getFidInstanceForNp(int np) {
        FidData rtn = null;
        for (FidData fidData : m_requestedSweeps) {
            if (fidData != null && fidData.getNRawPoints() == np) {
                rtn = new FidData(np,
                                  fidData.getLeadingPadding(),
                                  fidData.getTrailingPadding(),
                                  fidData.start_hz,
                                  fidData.stop_hz,
                                  fidData.getPower(),
                                  fidData.getGain(),
                                  fidData.getRfchan());
                Messages.postDebug("FidData", "getFidInstanceForNp(" + np
                                   + "): rfchan=" + fidData.getRfchan());
                break;
            }
        }
        return rtn;
    }

    /**
     * Remove obsolete entries from the list of sweeps sent to Vnmr.
     * Any entries sent before the one with the specified "np" number
     * of raw data points are removed. If there is no entry with the
     * specified "np" value, the list is unchanged.
     * @param np The specified number of raw data points.
     * @return True if the list was changed, otherwise false.
     */
    private boolean cleanRequestedSweepList(int np) {
        boolean haveNp = false;

        // First, see if np is in the list
        for (FidData fidData : m_requestedSweeps) {
            if (fidData != null && fidData.getNRawPoints() == np) {
                haveNp = true;
                break;
            }
        }

        if (haveNp) {
            Iterator<FidData> itr = m_requestedSweeps.iterator();
            while(itr.hasNext()) {
                FidData fidData = itr.next();
                if (fidData == null || fidData.getNRawPoints() != np) {
                    itr.remove();
                } else {
                    break;
                }
            }
        }
        return haveNp;
    }

    public static double adjustCalSweepStop(double stop) {
        double fstep = getFreqResolution();
        if (fstep > 0) {
            double foff = fstep / 2;
            // Snap beginning freq to a step, rounding up
            int m = (int)ceil((stop - foff) / fstep);
            stop = m * fstep + foff;
        }
        return stop;
    }

    public static double adjustCalSweepStart(double start) {
        double fstep = getFreqResolution();
        if (fstep > 0) {
            double foff = fstep / 2;
            // Snap beginning freq to a step, rounding down
            int m = (int)floor((start - foff) / fstep);
            start = m * fstep + foff;
        }
        return start;
    }

    public static int getCalSweepNp(double start, double stop) {
        double fstep = getFreqResolution();
        return (int)round((stop - start) / fstep) + 1;
    }


    /**
     * Container class to hold FID data.
     */
    public class FidData {

        /**
         * The time this instance was created, either when
         * this new sweep was sent or when this data was read.
         */
        public long dateStamp = System.currentTimeMillis();

        /**
         * The number of complex points that the raw FID was acquired with.
         * Typically greater than the number of data points in this FID,
         * because some points are trimmed from the ends when the FID is read.
         */
        public int nRawPoints;

        /**
         * Number of data points to be stripped off the front of the
         * raw data.
         */
        private int nLeadingPadPoints = 1; // Always 1

        /**
         * Number of data points to be stripped off the end of the
         * raw data.
         */
        private int nTrailingPadPoints = 1; // Always >= 1, usually 1 or 2

        /** The frequency of the first data point in Hz. */
        public double start_hz;

        /** The frequency of the last data point in Hz. */
        public double stop_hz;

//         /** The receiver gain the data was acquired with. */
//         public int gain;

//         /** The transmitter power the data was acquired with. */
//         public int power;

        /** Array of real data for this FID. */
        public double[] real;

        /** Array of imaginary data for this FID. */
        public double[] imag;

        private int mm_gain;

        private int mm_power;

        private int mm_rfchan;


        /**
         * Construct a FidData object with no data and the properties
         * set to the given values.
         * Normal usage is to create this shell of a FidData to keep track
         * of what data we're looking for and fill it with data when we
         * get a FID with the correct nRawPoints.
         * @param nRawPoints The number of complex points that the raw FID
         * was acquired with.
         * @param nLeadingPadPoints Number of data points to be
         * stripped off the front of the raw data.
         * @param nTrailingPadPoints Number of data points to be
         * stripped off the end of the raw data.
         * @param start_hz The frequency of the first data point in Hz.
         * @param stop_hz The frequency of the last data point in Hz.
         * @param power The transmitter power (tupwr).
         * @param gain The receiver gain.
         * @param rfchan The RF channel used to acquire the data.
         */
        public FidData(int nRawPoints,
                       int nLeadingPadPoints, int nTrailingPadPoints,
                       double start_hz, double stop_hz,
                       int power, int gain, int rfchan) {

            this.nRawPoints = nRawPoints;
            this.nLeadingPadPoints = nLeadingPadPoints;
            this.nTrailingPadPoints = nTrailingPadPoints;
            this.start_hz = start_hz;
            this.stop_hz = stop_hz;
            mm_power = power;
            mm_gain = gain;
            mm_rfchan = rfchan;
            //this.gain = getGainAt((start_hz + stop_hz) / 2);
            //this.power = getPowerAt((start_hz + stop_hz) / 2);
        }

        public double getMaxAbsval() {
            double max = 0;
            int size = min(real.length, imag.length);
            for (int i = 0; i < size; i++) {
                double z = real[i]*real[i] + imag[i]*imag[i];
                max = max(max, z);
            }
            return sqrt(max);
        }

        /**
         * Set the real and imaginary data in this FID to the given values.
         * It's up to the caller to ensure that the size of the arrays
         * equals this.getNGoodDataPoints().
         * @param realData The array of real values.
         * @param imagData The array of imaginary values.
         */
        public void setData(double[] realData, double[] imagData) {
            real = realData;
            imag = imagData;
        }

        /**
         * Returns the raw number of data points (before leading and trailing
         * padding has been stripped off).
         * @return The number of complex points.
         */
        public int getNRawPoints() {
            return nRawPoints;
        }

        /**
         * Returns the number of data points to be stripped off the front
         * of the raw data.
         * @return The number of leading, complex pad points.
         */
        public int getLeadingPadding() {
            return nLeadingPadPoints;
        }

        /**
         * Returns the number of data points to be stripped off the end
         * of the raw data.
         * @return The number of trailing, complex pad points.
         */
        public int getTrailingPadding() {
            return nTrailingPadPoints;
        }

        /**
         * Returns the number of data points (after leading and trailing
         * padding has been stripped off).
         * @return The number of complex points of actual data.
         */
        public int getNGoodDataPoints() {
            return nRawPoints - nLeadingPadPoints - nTrailingPadPoints;
        }

        /**
         * Returns the gain setting used to obtain this data.
         * @return The gain in dB (probably 0 to 50).
         */
        public int getGain() {
            return mm_gain;
        }

        /**
         * Returns the power setting used to obtain this data.
         * @return The power in dB (probably -16 to 20).
         */
        public int getPower() {
            return mm_power;
        }

        /**
         * Returns the RF channel used to obtain this data.
         * @return The RF channel (from 1).
         */
        public int getRfchan() {
            return mm_rfchan;
        }

        /**
         * Returns true if "obj" is a FidData object whose data are all
         * the same values as ours.
         * @param obj The Object to compare with this one.
         * @return True if equal.
         */
        public boolean equals(Object obj) {
            if (obj == null || !(obj instanceof FidData)) {
                return false;
            }
            FidData f = (FidData)obj;
            if (real.length != f.real.length || imag.length != f.imag.length) {
                return false;
            }
            int len = min(real.length, imag.length);
            for (int i = 0; i < len; i++) {
                if (real[i] != f.real[i] || imag[i] != f.imag[i]) {
                    return false;
                }
            }
            return true;
        }
    }


    /**
     * Container class to hold a calibration
     */
    class Calibration {
        /**
         * Modification date of newest of the calibration directories
         * at the time this calibration was loaded.
         * (This calibration will become obsolete if another calabration
         * appears that covers a different range that more closely matches
         * the data we want to correct - so we recheck (and reload) whenever
         * any new calibration appears.)
         */
        public long date;

        public double start;
        public double stop;
        public int np;
        public int pwr;
        public int gain;
        public int rfchan;
        private QuadPhase quadPhase = new QuadPhase(0, 0);
        //public boolean isPhaseKnown = false;
        public String mm_filePath = null;
        public Complex[] d;
        public Complex[] s;
        public Complex[] t1;
        private ChannelInfo mm_channelInfo = null;


        /**
         * Construct an RF calibration correction object, using "open",
         * "load", and "short" scans.
         * @param data The ReflectionData used to specify the scan parameters.
         * @param calData The calibration scans in the order
         * "open", "load", "short".
         * @param calPath Where this calibration came from.
         * @param chanInfo Which channel this was made for.
         */
        public Calibration(ReflectionData data,
                           Complex[][] calData, String calPath, ChannelInfo chanInfo) {

            mm_filePath = calPath;
            mm_channelInfo = chanInfo;
            // Calculate the correction coefficients
            d = calData[1];
            s = new Complex[data.size];
            t1 = new Complex[data.size];
            Complex one = new Complex(1, 0);
            Complex two = new Complex(2, 0);
            for (int i = 0; i < data.size; i++) {
                Complex z50 = d[i];
                Complex z0 = calData[2][i];
                Complex zInf = calData[0][i];

                s[i] = ((two.times(z50)).minus(z0.plus(zInf)))
                           .div(z0.minus(zInf));
                t1[i] = (z50.minus(z0)).times(one.plus(s[i]));
            }
            start = data.getStartFreq();
            stop = data.getStopFreq();
            np = data.getSize();
            pwr = data.getPower();
            gain = data.getGain();
            rfchan = data.getRfchan();
            date = System.currentTimeMillis();
        }

        public boolean isPhaseMeasurementNeeded() {
            return isQuadPhaseProblematic() && !quadPhase.isGoodQuality();
        }

        public int getQuadPhaseValue() {
            return quadPhase.getPhase();
        }

        public QuadPhase getPhase() {
            return quadPhase;
        }

        public void setPhase(QuadPhase phase) {
            quadPhase = phase;
        }
        
        public boolean isPhaseKnown() {
            return (!isQuadPhaseProblematic() || quadPhase.getQuality() > 0);
        }

        /**
         * Construct an empty calibration with only the start and stop
         * values set.
         * These values are derived from the given calibration file name:
         * "cal_pwr_gain_start_stop.zip".
         * @param name The given calibration file name.
         */
        public Calibration(String name) {
            int idx = name.lastIndexOf(".");
            int idx1 = name.lastIndexOf("_");
            double dstop = Double.parseDouble(name.substring(idx1+1, idx));
            dstop = rint(dstop * 1e6);
            int idx2 = name.lastIndexOf("_", idx1-1);
            double dstart = Double.parseDouble(name.substring(idx2+1, idx1));
            dstart = rint(dstart * 1e6);
            start = dstart;
            stop = dstop;
        }

        public ChannelInfo getChannel() {
            return mm_channelInfo;
        }

        public void setChannel(ChannelInfo channelInfo) {
            mm_channelInfo = channelInfo;
        }

        public String getFilePath() {
            return mm_filePath;
        }

        public void setFilePath(String filePath) {
            this.mm_filePath = filePath;
        }

        public Complex correctDatum(Complex raw, int idx) {
            return (raw.minus(d[idx]))
                    .div(t1[idx].plus(s[idx].times(raw.minus(d[idx]))));
        }

        /**
         * Checks that this calibration correction matches the parameters
         * of the given data, and that no new calibration files have appeared.
         * All of "startFreq", "stopFreq", "np", "power", "gain", and "rfchan"
         * must match.
         * @param data The data we want to correct.
         * @return True if this correction matches the parameters of "data".
         */
        public boolean isGoodFor(ReflectionData data) {
            long newDate = getLatestCalDate();
            return (date >= newDate
                    && start == data.getStartFreq()
                    && stop == data.getStopFreq()
                    && np == data.getSize()
                    && pwr == data.getPower()
                    && gain == data.getGain()
                    && rfchan == data.getRfchan());
        }

        public String toString() {
            return "phase=" + quadPhase + ",path=" + getFilePath();
        }
    }


    /**
     * This class just runs an endless loop to check the magnet leg
     * every few seconds to see if it's in tune mode.
     * Not actually needed for Nirvana.
     */
    class TuneModeThread extends Thread {

        /**
         * Endless loop to check whether the tune mode is on.
         */
        public void run() {
            int cnt = 0;
            setName("MtuneControl");
            m_run = true;
            while (m_run && !MotorControl.isPZT()) {
                try {
                    checkTuneMode();
                    if (++cnt == MLSTAT_UPDATE_RATE) {
                        updateMLStatus();
                        cnt = 0;
                    }
                    Thread.sleep(TUNE_CHECK_TIMEOUT);
                } catch (InterruptedException ie) {
                    // Don't stop this thread on interrupt.
                }
                //if (m_master.isCancel()) break;
            }
        }
    }


    /**
     * Determine the quad phase correction for the current sweep region.
     * This should be done only when the channel is first set and the
     * sweep is set to the full range of the channel.
     * @param calPath The path to the calibration Zip file.
     * @param probeDelay The probe delay (ns).
     * @return The phase correction (0-3).
     * @throws InterruptedException If execution is aborted.
     */
    private QuadPhase findPhaseCorrection(String calPath, double probeDelay)
            throws InterruptedException {
        boolean rawDataFlag = m_master.isRawDataMode();
        // Gather raw data for current sweep
        m_master.setRawDataMode(true);
        ReflectionData data = getData(0, probeDelay);
        // Determine best phase from these data
        QuadPhase phase = getPhaseFromRefScan(calPath, data, probeDelay);
        m_master.setRawDataMode(rawDataFlag);
        return phase;
    }

    private QuadPhase getPhaseFromRefScan(String calPath,
                                          ReflectionData data,
                                          double probeDelay) {
        QuadPhase quadPhase = null;
        if (calPath != null) {
            String key = new File(calPath).getName();
            QuadPhase prevPhase = m_phaseMap.get(key);
            Messages.postDebug("QuadPhase", "prevPhase=" + prevPhase);

            Interval exclude = readCalDipInterval(calPath);
            Complex[] referenceData = getRefData(calPath, data, exclude);
            Complex[][] calData = readCalData(calPath, data);
            QuadPhase savedPhase = QuadPhase.readPhase(calPath);
            if (referenceData == null || calData == null) {
                if (prevPhase != null) {
                    return prevPhase;
                } else if (savedPhase != null) {
                    return savedPhase;
                } else {
                    return new QuadPhase(0, 0); // Desperate default
                }
            }

            double[] maxs = new double[4];
            double[] diffs = new double[4];
            double[] dipPhases = new double[4];
            for (int i = 0; i < 4; i++) {
                Calibration cal = new Calibration(data, calData, calPath, null);
                ReflectionData corrData = data.copy();
                correctData(corrData, cal);
                corrData.correctForProbeDelay_ns(probeDelay);
                dipPhases[i] = corrData.getDipPhase();

                maxs[i] = getCalDataMax(data, calData, calPath);

                // Check diff between specified data and rotated reference scan
                // TODO: Ignore dip regions in both this data and ref data?
                diffs[i] = data.getNormalizedDiff(referenceData, exclude);
                Messages.postDebug("QuadPhase", "getPhaseFromRefScan: "
                                   + "max[" + i + "]=" + Fmt.f(4, maxs[i])
                                   + ", diff[" + i + "]=" + Fmt.f(4, diffs[i])
                                   + ", phase[" + i + "]="
                                   + Fmt.f(4, dipPhases[i])
                                   );

                // Set data for next try
                Complex.rotateData(referenceData, 1);
                calData = rotateCalibration(calData, 1);
            }
            double[] sortedMaxs = Arrays.copyOf(maxs, maxs.length);
            Arrays.sort(sortedMaxs);
            int phase0 = -1;
            for (int i = 0; i < maxs.length; i++) {
                if (maxs[i] == sortedMaxs[0]) {
                    phase0 = i;
                    break;
                }
            }
            int quality0 = 1;
            if (sortedMaxs[1] > 5 && sortedMaxs[0] < 1) {
                quality0 = 3;
            } else if (sortedMaxs[1] > 3 && sortedMaxs[0] < 1.2) {
                quality0 = 2;
            } else if (sortedMaxs[1] < 1.5) {
                quality0 = 0;
            }
            Messages.postDebug("QuadPhase", "phase0=" + phase0
                               + ", quality=" + quality0
                               + "  (corrected max reflection)");

            double[] sortedDiffs = Arrays.copyOf(diffs, diffs.length);
            Arrays.sort(sortedDiffs);
            int phase1 = -1;
            for (int i = 0; i < diffs.length; i++) {
                if (diffs[i] == sortedDiffs[0]) {
                    phase1 = i;
                    break;
                }
            }
            double diff0 = sortedDiffs[0];
            double diff13 = abs(sortedDiffs[2] - sortedDiffs[1]);
            double diff90 = (sortedDiffs[1] + sortedDiffs[2]) / 2;
            double diff180 = sortedDiffs[3];
            int phase180 = -1;
            for (int i = 0; i < diffs.length; i++) {
                if (diffs[i] == diff180) {
                    phase180 = i;
                    break;
                }
            }
            int quality1 = 1;
            if ((phase1 + 2) % 4 != phase180) {
                quality1 = 0;
            } else if (diff13 / diff90 < 0.2
                && diff0 / diff90 < 0.5
                && diff180 / diff90 > 1.2
                && diff180 / diff90 < 1.6)
            {
                quality1 = 3;
            } else if (diff13 / diff90 < 0.5
                    && diff0 / diff90 < 0.6
                    && diff180 / diff90 > 1
                    && diff180 / diff90 < 2)
            {
                quality1 = 2;
            }
            Messages.postDebug("QuadPhase", "phase1=" + phase1
                               + ", quality=" + quality1
                               + "  (raw data comparison)");

            double[] sortedPhases = new double[dipPhases.length];
            for (int i = 0; i < dipPhases.length; i++) {
                sortedPhases[i] = abs(dipPhases[i]);
            }
            Arrays.sort(sortedPhases);
            int phase2 = -1;
            for (int i = 0; i < dipPhases.length; i++) {
                if (abs(dipPhases[i]) == sortedPhases[0]) {
                    phase2 = i;
                    break;
                }
            }
            int quality2 = 1;
            if (Double.isNaN(sortedPhases[0]) || phase2 < 0)
            {
                quality2 = 0;
            } else if (Double.isNaN(sortedPhases[1])) {
                quality2 = 2;
            } else if (abs(sortedPhases[0] / sortedPhases[1]) > 0.9) {
                quality2 = 0;
            } else if (abs(sortedPhases[0] / sortedPhases[1]) < 0.2) {
                quality2 = 3;
            } else if (abs(sortedPhases[0] / sortedPhases[1]) < 0.5) {
                quality2 = 2;
            }
            Messages.postDebug("QuadPhase", "phase2=" + phase2
                               + ", quality=" + quality2
                               + "  (dip phase)");

            int phase = 0;
            int quality = 0;
            if (phase0 == phase1 && phase1 == phase2) {
                // A clear winner
                phase = phase0;
                quality = quality0 + quality1 + quality2;
                Messages.postDebug("QuadPhase", "phase0=phase1=phase2: "
                                   + phase);
            }
            // They don't all agree:
            else if (phase0 == phase1 && quality0 + quality1 > quality2) {
                phase = phase0;
                quality = quality0 + quality1 - quality2;
                Messages.postDebug("QuadPhase", "phase0=phase1: "
                                   + phase);
            } else if (phase0 == phase2 && quality0 + quality2 > quality1) {
                phase = phase0;
                quality = quality0 + quality2 - quality1;
                Messages.postDebug("QuadPhase", "phase0=phase2: "
                                   + phase);
            } else if (phase1 == phase2 && quality1 + quality2 > quality0) {
                phase = phase1;
                quality = quality1 + quality2 - quality0;
                Messages.postDebug("QuadPhase", "phase1=phase2: "
                                   + phase);
            }
            // No result has a solid majority
            else if (quality0 > (quality1 + quality2)) {
                phase = phase0;
                quality = quality0 - quality1 - quality2;
                Messages.postDebug("QuadPhase",
                                   "phase0 looks the best: " + phase);
            } else if (quality1 > (quality0 + quality2)) {
                phase = phase1;
                quality = quality1 - quality0 - quality2;
                Messages.postDebug("QuadPhase",
                                   "phase1 looks the best: " + phase);
            } else if (quality2 > (quality0 + quality1)) {
                phase = phase2;
                quality = quality2 - quality0 - quality1;
                Messages.postDebug("QuadPhase",
                                   "phase2 looks the best: " + phase);
            }
            // No one measurement stands out
            else if (quality0 > quality1 && quality0 > quality2) {
                phase = phase0;
                quality = quality0 - quality1 - quality2;
                Messages.postDebug("QuadPhase",
                                   "phase0 looks better: " + phase);
            } else if (quality1 > quality2) {
                phase = phase1;
                quality = quality1 - quality0 - quality2;
                Messages.postDebug("QuadPhase",
                                   "phase1 looks better: " + phase);
            } else {
                phase = phase2;
                quality = quality2 - quality0 - quality1;
                Messages.postDebug("QuadPhase",
                                   "phase2 looks better: " + phase);
            }
            quality = max(0, quality);
            Messages.postDebug("QuadPhase", "phase=" + phase
                               + ", quality=" + quality);
            quadPhase = new QuadPhase(phase, quality);

            int savedQuality = (savedPhase == null)
                ? 0 : min(savedPhase.getQuality(),
                               QuadPhase.MAX_SAVED_QUALITY);
            if (quadPhase.getQuality() >= savedQuality) {
                Messages.postDebug("QuadPhase", "Saving new phase");
                QuadPhase.writePhase(calPath, quadPhase);
                savedQuality = quadPhase.getQuality();
            }

            if (prevPhase != null && prevPhase.getQuality() > quality) {
                Messages.postDebug("QuadPhase", "Using previous phase");
                quadPhase = prevPhase;
            }

//            if (quadPhase.getQuality() < savedQuality) {
//                Messages.postDebug("QuadPhase", "Using saved phase");
//                quadPhase = savedPhase;
//                quadPhase.setQuality(QuadPhase.MAX_SAVED_QUALITY);
//            }
            m_phaseMap.put(key, quadPhase);
        }
        Messages.postDebug("QuadPhase","quadPhase.phase="
                           + quadPhase.getPhase());
        return quadPhase;
    }

    /**
     * @param data The raw data to check.
     * @param calData The calibration data to use.
     * @param calPath The file the calibration data came from.
     * @return The 95'th percentile level of squared amplitude.
     */
    private double getCalDataMax(ReflectionData data, Complex[][] calData,
                                 String calPath) {
        Calibration cal = new Calibration(data, calData, calPath, null);
        double[] reflection2 = getAbsvalArray(data, cal);
        Arrays.sort(reflection2);
        // Look at 95th percentile point
        int idx = (int)(0.95 * reflection2.length);
        return reflection2[idx];
    }

    private static Complex[] getRefData(String refPath,
                                        ReflectionData data, Interval exclude) {
        double start = data.startFreq;
        double stop = data.stopFreq;
        int np = data.size;
        return getRefData(refPath, start, stop, np, exclude);
    }
//    private Complex[] getRefData(ChannelInfo ci, ReflectionData data) {
//        Complex[] refData = new Complex[data.size];
//        String calPath = ci.getCalPath();
//        return null;
//    }

    /**
     * Read the probe data from the given cal file, getting just the
     * data for the specified points.
     * @param calPath The path to the calibration Zip file.
     * @param start_hz The frequency of the first point.
     * @param stop_hz The frequency of the last point.
     * @param npts The number of data points.
     * @param exclude Interval to exclude by zeroing the data.
     * @return The probe reflection data as a Complex array.
     */
    protected static Complex[] getRefData(String calPath,
                                          double start_hz,
                                          double stop_hz,
                                          int npts,
                                          Interval exclude) {
        if (calPath == null) {
            return null;
        }

        // Read the calibration files
        Complex[] refData = new Complex[npts];
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(calPath);
            ZipEntry entry = zipFile.getEntry("CalProbe");
            InputStream in = zipFile.getInputStream(entry);
            BufferedReader reader;
            reader = new BufferedReader(new InputStreamReader(in));
            ReflectionData calData = readRawData(0, reader);
            refData = extractCalData(calData, npts, start_hz, stop_hz, exclude);
            if (refData == null) {
                Messages.postError("Invalid calibration entry \""
                                   + "CalProbe\" in: " + calPath);
                refData = null;
            }
        } catch (Exception e) {
            Messages.postError("Error reading calibration in \""
                               + calPath + "\": " + e);
            refData = null;
        } finally {
            try { zipFile.close(); } catch (Exception e) { }
        }
        return refData;
    }

    public QuadPhase findPhaseCorrection(ChannelInfo chanInfo)
            throws InterruptedException {
        QuadPhase phase = null;
        phase = chanInfo.getQuadPhase(); // See if phase is already set
        if (phase == null || phase.getQuality() < 9) {
            // Make sure sweep is set correctly for detecting phase ...
            // ... findPhaseCorrection will acquire data
            chanInfo.setFullSweep();

            double delay = chanInfo.getProbeDelay();
            QuadPhase phase1;
            phase1 = findPhaseCorrection(chanInfo.getCalPath(), delay);
            if (phase == null || phase.getQuality() < phase1.getQuality()) {
                phase = phase1;
            }
        }
        return phase;
    }

    /**
     * Measure the probe delay from the given calibration data.
     * This is the effective round-trip delay time between the probe port
     * and the coil. The "open", "load", "short" scans are done at the
     * probe port; the "probe" scan is reflected from the coil. Therefore
     * the reflected probe data has a frequency dependent phase shift.
     * The correct probe delay is calculated to make the phase frequency
     * independent and equal to 0.
     * If there is a dip in the probe data, the probeDelay is fine-tuned to
     * put the dip at phase = 0.
     * @param calData ReflectionData for "open", "load", "short", "probe",
     * and "sigma" respectively.
     * @return The probe calibration scan, with the dip and probeDelay set.
     * @throws InterruptedException If aborted.
     */
    public ReflectionData measureProbeDelay(ReflectionData[] calData)
    throws InterruptedException {
        // Correct "probe" scan using "open/load/short" scans

        // Set calibration
        Complex[][] cals = new Complex[3][];
        int npts = calData[0].size;
        double start_hz = calData[0].startFreq;
        double stop_hz = calData[0].stopFreq;
        for (int i = 0; i < 3; i++) {
            cals[i] = extractCalData(calData[i], npts, start_hz, stop_hz, null);
            if (cals[i] == null) {
                Messages.postError("Invalid calibration scan");
                return null;
            }
        }
        Calibration cal = new Calibration(calData[0], cals, null, null);

        // Correct the "probe" scan
        ReflectionData probeData = calData[3].copy(); // Don't change original
        correctData(probeData, cal); // modifies "probeData"

        // Get the array of angles with 0 probe delay
        probeData.correctForProbeDelay_ns(0); // Make sure correction is 0
        double[] theta = new double[npts];
        for (int i = 0; i < npts; i++) {
            theta[i] = atan2(probeData.imag[i], probeData.real[i]);
        }

        // Brute force: find a delay that make most angles near 0
        double deltaHz = probeData.getDeltaFreq();
        List<Double> delays = new ArrayList<Double>();
        List<Double> sigmas = new ArrayList<Double>();
        double delayInc = 0.05; // (ns)
        double maxDelay = 15; // Maximum conceivable probe delay (ns)
        for (double delay = 0; delay < maxDelay; delay += delayInc) {
            double sigma = calcSigmaForDelay(start_hz, deltaHz, theta, delay);
            delays.add(delay);
            sigmas.add(sigma);
            Messages.postDebug("ProbeDelay", "Delay=" + delay
                               + ", sigma=" + sigma);
        }
        double minSigma = Double.MAX_VALUE;
        int bestIdx = 0;
        for (int i = 0; i < sigmas.size(); i++) {
            double sigma = sigmas.get(i);
            if (minSigma > sigma) {
                minSigma = sigma;
                bestIdx = i;
            }
        }
        double delay = delays.get(bestIdx);
        double bestDelay = golden(start_hz,
                                  deltaHz,
                                  theta,
                                  delay - delayInc,
                                  delay,
                                  delay + delayInc,
                                  minSigma);
        Messages.postDebug("ProbeDelay", "BestDelay=" + bestDelay);

        if (DebugOutput.isSetFor("ProbeDelay")) {
            // Show if sigma has improved
            minSigma = calcSigmaForDelay(start_hz, deltaHz, theta, bestDelay);
            Messages.postDebug("BestDelay=" + bestDelay
                               + ", sigma=" + minSigma);
            showDelayFit(start_hz, deltaHz, theta, minSigma, bestDelay);
        }

        m_master.setProbeDelay(bestDelay);
        probeData.correctForProbeDelay_ns(bestDelay);
        probeData.setDips(null);

        m_master.displayData(probeData);
        return probeData;
    }

    /**
     * Find the probe delay that minimizes the angles of the data.
     * Uses a Golden section search starting from a triple of delays that
     * bracket the minimum.
     * @param start_hz The frequency of the first data point (Hz).
     * @param deltaHz The increment in frequency between data points (Hz).
     * @param theta The angles of the data for 0 delay (radians).
     * @param x0 A delay that is less than optimum (ns).
     * @param x1 A delay between x0 and x3 that is better than either (ns).
     * @param x3 A delay that is greater than optimum (ns).
     * @param y1 The standard deviation of the angles from 0 for the x1 delay.
     * @return The optimum delay, within 0.001 ns.
     */
    private double golden(double start_hz, double deltaHz, double[] theta,
                          double x0, double x1, double x3, double y1) {
        final double GR = 0.61803399;
        final double GRC = 1 - GR;
        final double TOL = 0.001;

        double x2 = x1 + GRC * (x3 - x1);
        double y2 = calcSigmaForDelay(start_hz, deltaHz, theta, x2);
        while (abs(x3 - x0) > TOL) {
            if (y2 < y1) {
                x0 = x1;
                x1 = x2;
                x2 = GR * x1 + GRC * x3;
                y1 = y2;
                y2 = calcSigmaForDelay(start_hz, deltaHz, theta, x2);
            } else {
                x3 = x2;
                x2 = x1;
                x1 = GR * x2 + GRC * x0;
                y2 = y1;
                y1 = calcSigmaForDelay(start_hz, deltaHz, theta, x1);
            }
        }
        return (y1 < y2) ? x1 : x2;
    }

    /**
     * Dump data for the angles for a given delay (for debugging).
     * Data is written to "/vnmr/tmp/ptuneAngleWrap".
     * @param startHz Frequency of first data point (Hz).
     * @param deltaHz Frequency increment between points (Hz).
     * @param theta Array of angles of data points (radians).
     * @param cutoff Ignore points with angle greater than this (radians).
     * @param delay The probe delay (ns).
     */
    private void showDelayFit(double startHz, double deltaHz,
                              double[] theta, double cutoff,
                              double delay) {
        StringBuffer sb = new StringBuffer("#FREQ\tANGLE\n");
        int npts = theta.length;
        double dtheta_dfreq = 2 * Math.PI * delay * 1e-9;
        for (int i = 0; i < npts; i++) {
            double freq = startHz + i * deltaHz;
            double t = theta[i] + dtheta_dfreq * freq;
            t = ReflectionData.wrapAngle(t, 0);
            if (abs(t) < cutoff) {
                sb.append(freq).append(" ").append(t).append("\n");
            } else {
                System.err.println("reject i=" + i);
            }
        }
        String path = "/vnmr/tmp/ptuneAngleWrap";
        TuneUtilities.writeFile(path, sb.toString());
    }

    /**
     * @param start_hz Frequency of first data point (Hz).
     * @param deltaHz Frequency increment between points (Hz).
     * @param theta Array of angles of data points (radians).
     * @param delay The probe delay (ns).
     * @return Standard deviation of angle from expected value of 0.
     */
    private double calcSigmaForDelay(double start_hz, double deltaHz,
                                     double[] theta, double delay) {
        int npts = theta.length;
        double dtheta_dfreq = 2 * PI * delay * 1e-9;
        double sigma = 0;
        for (int i = 0; i < npts; i++) {
            double freq = start_hz + i * deltaHz;
            double t = theta[i] + dtheta_dfreq * freq;
            t = ReflectionData.wrapAngle(t, 0);
            sigma += t * t;
        }
        sigma = sqrt(sigma / (npts - 1));
        return sigma;
    }

//    /**
//     * Prepare MtuneControl to deal with the given channel.
//     * Sets MtuneControl's m_channelInfo variable to ci.
//     * If ci is non-null:
//     * Set the channel's calibration file.
//     * Set the probe delay parameter to the calibration file's value.
//     * Determine the quad phase correction, or retrieve it from the
//     * ChannelInfo.
//     * @param ci The ChannelInfo of the new channel, or null.
//     * @return True if the initialization was successful.
//     * @throws InterruptedException If tuning was aborted.
//     */
//    public boolean initializeForChannel(ChannelInfo ci)
//            throws InterruptedException {
//
//        return initializeForChannel(ci, 0);
//    }

    /**
     * Prepare MtuneControl to deal with the given channel.
     * Sets MtuneControl's m_channelInfo variable to ci.
     * If ci is non-null:
     * Set the channel's calibration file.
     * Set the probe delay parameter to the calibration file's value.
     * Determine the quad phase correction, or retrieve it from the
     * ChannelInfo.
     * @param ci The ChannelInfo of the new channel, or null.
     * @param rfchan The RF channel used by this tune channel.
     * @return True if the initialization was successful.
     * @throws InterruptedException If tuning was aborted.
     */
    public boolean initializeForChannel(ChannelInfo ci, int rfchan)
            throws InterruptedException {

        // Initialize ourself to deal with this channel
        m_channelInfo = ci;

        if (ci != null) {
            rfchan = initializeRfChan(ci, rfchan);
            // Set the channel's calibration file
            double startHz = ci.getMinSweepFreq();
            double stopHz = ci.getMaxSweepFreq();
            String calPath = getBestCalFilePath(null, null, rfchan,
                                                startHz, stopHz);
            ci.setCalPath(calPath);
            Messages.postDebug("RFCal", "initializeForChannel: set calPath="
                               + calPath);
            if (calPath == null) {
                return false;
            }

            double probeDelay = readCalProbeDelay(calPath);
            if (Double.isNaN(probeDelay)) {
                double[] range = getFreqRangeOfCalFile(calPath);
                Messages.postError("New RF Calibration is required for "
                                   + Fmt.f(0, range[0] / 1e6) + " to "
                                   + Fmt.f(0, range[1] / 1e6) + " MHz");
                String title = "RF_Calibration_Error";
                String msg = "<HTML>New RF Calibration is required for <BR>"
                        + Fmt.f(0, range[0] / 1e6) + " to "
                        + Fmt.f(0, range[1] / 1e6) + " MHz";
                m_master.sendToGui("popup error title " + title
                                   + " msg " + msg);
                return false;
            }
            ci.setProbeDelay(probeDelay); // Also displays it

            QuadPhase phase = ci.getQuadPhase();
            if (phase == null) { //FIXME: omit this conditional
                // Set the channel's quad phase correction
                phase = findPhaseCorrection(ci); // May take a scan in raw mode
                ci.setQuadPhase(phase);
            }
        }
        return true;
    }

    public int initializeRfChan(ChannelInfo ci, int rfchan) {
        if (rfchan == 0 && (rfchan = ci.getRfchan()) == 0) {
            rfchan = getRfChanFromCalFiles(ci);
        }
        ci.setRfchan(rfchan);
        setRfchan(rfchan);
        return rfchan;
    }

}
