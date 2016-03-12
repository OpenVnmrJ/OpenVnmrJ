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
//-->import java.nio.channels.Channels;
//-->import java.nio.channels.ClosedByInterruptException;
import java.util.ArrayList;
import java.util.NoSuchElementException;
import java.util.Random;
import java.util.StringTokenizer;

import vnmr.util.Complex;
import vnmr.util.Fmt;

import static java.lang.Math.*;
//import static vnmr.apt.AptDefs.*;

/**
 * This class simulates Vnmr's swept tune function. Generates fake reflection
 * data and puts it in a FID file. Also, writes ptuneInfo in /vnmr/tmp with
 * extra "header" information for the FID. Also, reads ptuneCmds to adjust
 * sweep appropriately.
 */
public class Mtune {

    private static final Complex COMPLEX_ONE = new Complex(1, 0);
    private static final Complex COMPLEX_MINUS_ONE = new Complex(-1, 0);
    private static final Complex COMPLEX_ZERO = new Complex(0, 0);
    private static final Complex COMPLEX_50 = new Complex(50, 0);

    private static String sm_vnmrSystemDir = "/vnmr";
    private static String sm_fidpath;
    private static String sm_commandpath;
    private static String sm_infopath;
    private static String sm_cappath;
    private static String sm_motorpath;
    private static String sm_probeName = null;
    private static String sm_chanName = null;
    private static ProbeSimulator sm_probeSimulator = null;

    private static Random sm_random = new Random();

    private long m_start = 389962500; // One pad point at beginning
    private long m_stop = 410037500; // One pad point at end
    private int m_np = 804;
    private int m_power;        // Transmitter power
    private int m_gain;         // Receiver gain
    private int m_tchan;        // RF Channel

    protected static enum Termination {PROBE, OPEN, LOAD, SHORT};
    protected Termination m_termination = Termination.PROBE;
    protected static double m_offset_hz = 0;

    /** The thread that writes the data. */
    private DataWriter m_writer = null;

    /** The thread that reads the console */
    private ConsoleReader m_reader = null;
    
    private double m_prevTune = -1;/*DBG*/
    private double m_prevMatch = -1;/*DBG*/


    public static void main(String[] args) {
        setup(args);
    }

    // setup performs basic setup and returns an instance of Mtune
    // breaking this out facilitates unit testing.
    public static Mtune setup(String[] args) {
        int len = args.length;
        for (int i = 0; i < len; i++) {
            if (args[i].equalsIgnoreCase("-vnmrsystem") && i + 1 < len) {
                sm_vnmrSystemDir = args[++i];
            } else if (args[i].equalsIgnoreCase("-probe") && i + 1 < len) {
                sm_probeName = args[++i];
            } else if (args[i].equalsIgnoreCase("-chanFile") && i + 1 < len) {
                sm_chanName = args[++i];
            }
        }
        setGlobalPaths();
        return new Mtune();
    }
    
    private static void setGlobalPaths() {
        String basedir = sm_vnmrSystemDir + "/tmp";
        sm_fidpath = basedir + "/fid";
        sm_commandpath = basedir + "/ptuneCmds";
        sm_infopath = basedir + "/ptuneInfo";
        sm_cappath = basedir + "/ptuneCapPositions";
        sm_motorpath = basedir + "/ptuneMotorPositions";
        System.out.println("basedir=" + basedir);
    }

    public Mtune() {
        if (sm_probeName != null) {
            sm_probeSimulator = new ProbeSimulator(sm_probeName,
                                                   sm_vnmrSystemDir,
                                                   sm_chanName);
        }
        m_writer = new DataWriter();
        m_writer.start();
        m_reader = new ConsoleReader();
        m_reader.start();
    }

    public void quit(int ms) throws InterruptedException, IOException {
    	m_writer.quit(ms);
    	m_reader.quit(ms);
    }
    
    protected void readSweepParameters() {
        BufferedReader in = TuneUtilities.getReader(sm_commandpath);
        if (in != null) {
            // Got commands to read
            try {
                double center = (m_start + m_stop) / 2;
                double span = m_stop - m_start;
                String line;
                while ((line = in.readLine()) != null) {
                    try {
                        StringTokenizer toker = new StringTokenizer(line, " =");
                        String key = toker.nextToken();
                        if (key.equals("sfrq")) {
                            center = Double.parseDouble(toker.nextToken());
                            center *= 1e6; // sfrq is in MHz
                        } else if (key.equals("tunesw")) {
                            span = Double.parseDouble(toker.nextToken());
                        } else if (key.equals("np")) {
                            m_np = Integer.parseInt(toker.nextToken()) / 2;
                        } else if (key.equals("tupwr")) {
                            m_power = Integer.parseInt(toker.nextToken());
                        } else if (key.equals("gain")) {
                            m_gain = Integer.parseInt(toker.nextToken());
                        } else if (key.equals("tchan")) {
                            m_tchan = Integer.parseInt(toker.nextToken());
                        }
                    } catch (NoSuchElementException nsee) {
                    }
                }
                m_start = (long)(center - span / 2);
                m_stop = (long)(center + span / 2);
                Messages.postDebug("Mtune.readSweepParameters: start="
                                   + Fmt.f(6, m_start / 1e6)
                                   + ", stop=" + Fmt.f(6, m_stop / 1e6)
                                   + ", np=" + m_np
                                   + "\n                           "
                                   + "power=" + m_power
                                   + " gain=" + m_gain
                                   + " tchan=" + m_tchan
                                   );/*DBG*/
            } catch (IOException ioe) {
            }

            try {
                in.close();
            } catch (IOException ioe) {
            }

            // Delete command file after using it.
            new File(sm_commandpath).delete();

            try {
                Thread.sleep(2000); // Simulate slow sweep change
            } catch (InterruptedException ie) {
            }
        }
    }


    protected void writeHeaderInfo(long crc) {
        // Open a temp info file, write the info, and rename the file
        String tmppath = sm_infopath + "tmp";
        File tmpfile = new File(tmppath);
        PrintWriter out = null;
        try {
            out = new PrintWriter
                    (new BufferedWriter(new FileWriter(tmpfile)));
            //out.println("checksum " + crc);
            //out.println("gain 0");
            //out.println("power 10");
            out.println("fidpath " + sm_fidpath);
            //out.println("start " + m_start);
            //out.println("stop " + m_stop);
            //out.println("np " + m_np);
        } catch (IOException ioe) {
            Messages.postError("Mtune.writeHeaderInfo: Error writing "
                               + tmppath + ": " + ioe);
        } finally {
            out.close();
        }
        // Move the temp file to standard place
        File file = new File(sm_infopath);
        if (!tmpfile.renameTo(file)) {
            file.delete();
            if (!tmpfile.renameTo(file)) {
                Messages.postError("Unable to rename " + tmppath
                                   + " to " + sm_infopath);
            }
        }
    }

    /**
     * It may be possible for other processes to read this file while
     * it is being written, resulting in getting corrupted data. The
     * checksum may be used to detect this event.
     */
    protected long writeFidData() {
        long crc = 0;
        ByteArrayOutputStream outBytes = new ByteArrayOutputStream();
        DataOutputStream outBuf = new DataOutputStream(outBytes);
        OutputStream out = null;

        try {
            // Write data to internal buffer
            // NB: Writing data to outBuf puts it in outBytes' buffer.
            writeFileHeader(outBuf, m_np);
            writeBlockHeader(outBuf);
            writeFidData(outBuf);

            // Calculate CRC of data file
            crc = TuneUtilities.getCrc32(outBytes.toByteArray());

            // Open the data file, and save the data
            out = new BufferedOutputStream(new FileOutputStream(sm_fidpath));
            outBytes.writeTo(out);
        } catch (IOException ioe) {
            Messages.postError("Error writing data to " + sm_fidpath
                               + ": " + ioe);
        } finally {
            try {
                out.close();
            } catch (Exception e) {}
        }

        return crc;
    }

    /**
     * @param out The DataOutputStream to write to.
     * @param npts The number of complex points in the FID.
     */
    private void writeFileHeader(DataOutputStream out, int npts)
        throws IOException {

        int nbheaders = 1;
        int nblocks = 1;
        int ntraces = 1;
        int np = npts * 2;      // Twice number of complex points
        int ebytes = 4;         // Bytes per number (element)
        int tbytes = np * ebytes; // Bytes per trace
        int bbytes = ntraces * tbytes + nbheaders * 28;
        short vers_id = 1;      // VNMR version number ???
        short status = 0x8;     // Data is Float format

        out.writeInt(nblocks);
        out.writeInt(ntraces);
        out.writeInt(np);
        out.writeInt(ebytes);
        out.writeInt(tbytes);
        out.writeInt(bbytes);
        out.writeShort(vers_id);
        out.writeShort(status);
        out.writeInt(nbheaders);
    }

    /**
     * @param out The DataOutputStream to write to.
     */
    private void writeBlockHeader(DataOutputStream out) throws IOException {

        short scale = 1;        // Not used
        short status = 0x8;     // Data is Float format
        short index = 0;
        short mode = 0;         // Not used
        int ctcount = 1;        // Number of FIDs summed in data
        float lpval = 0;        // Not used
        float rpval = 0;        // Not used
        float lvl = 0;          // Not used
        float tlt = 0;          // Not used

        out.writeShort(scale);
        out.writeShort(status);
        out.writeShort(index);
        out.writeShort(mode);
        out.writeInt(ctcount);
        out.writeFloat(lpval);
        out.writeFloat(rpval);
        out.writeFloat(lvl);
        out.writeFloat(tlt);
    }

    /**
     * Write a block of swept tune "FID" data.
     * @param out The DataOutputStream to write to.
     */
    /*
    private void writeFidData(DataOutputStream out) throws IOException {
        // Prelim version: Write 3*PI/2 worth of data w/ no resonance
        final double NOISE = 0.005;
        for (int i = 0; i < m_np; i++) {
            double theta = (3 * PI / 2) * ((double)i / (m_np - 1));
            double randy =  NOISE * sm_random.nextGaussian();
            out.writeFloat((float)(randy + cos(theta)));
            randy =  NOISE * sm_random.nextGaussian();
            out.writeFloat((float)(randy + sin(theta)));
        }
    }
    */

    /**
     * Write a block of swept tune "FID" data.
     * @param out The DataOutputStream to write to.
     */
    private void writeFidData(DataOutputStream out) throws IOException {
        final double NOISE = 0.005;

        // The gain factor in amplitude:
        double gain = pow(10, (m_power + m_gain + 50) / 20.0);
        double noise = 1e2 * NOISE / sqrt(gain); // NOISE=noise at gain=1e4
        CoilParams[] coilParams = null;
        Termination termination = m_termination;

        if (termination == Termination.PROBE){
            // Only need to calculate this if we are simulating the probe
            if (sm_probeName != null) {
                coilParams = getCurrentCoilParamsForProbe();
            } else {
                coilParams = getCurrentCoilParamsForDefaultProbe();
            }
        }

        // Calculate reflection for all freqs in range
        for (int i = 0; i < m_np; i++) {
            double f = m_start + ((double)i / (m_np - 1)) * (m_stop - m_start);
            Complex r = null;
            switch (termination) {
            default:
                // Normal case: use the simulated probe resonances
                r = calculateReflection(f, coilParams);
                break;
            case OPEN:
                r = COMPLEX_ONE;
                break;
            case SHORT:
                r = COMPLEX_MINUS_ONE;
                break;
            case LOAD:
                r = COMPLEX_ZERO;
                break;
            }

            // Increment the phase a bit w/ frequency
            // How fast to wind the reflection when far from real resonance:
            // probeDelay is round-trip time from port to coil (seconds)
            double probeDelay = 5.1e-9;
            double dtheta_dfreq = 2 * PI * probeDelay;
            double modulus = r.mod();
            double theta = r.arg();
            if (termination == Termination.PROBE) {
                theta -= dtheta_dfreq * f;
            }
            // NB: Put out FID data like Vnmr's, so sign of imag is
            // reversed as per NMR convention.
            theta = -theta;
            double x = modulus * cos(theta);
            double y = modulus * sin(theta);

            double randy = noise * sm_random.nextGaussian();
            out.writeFloat((float)((randy + x) * gain));
            randy = noise * sm_random.nextGaussian();
            out.writeFloat((float)((randy + y) * gain));
        }
    }

    /**
     * Calculate the coil parameters for the current motor positions
     * using the probe behavior specified in the ProTune channel files.
     * The motor positions are read from the "ptuneMotorPositions" file.
     * @return The coil parameters.
     */
    private CoilParams[] getCurrentCoilParamsForProbe() {
        // Read the motor positions
        int[] motorPositions = getAllMotorPositions();
        return sm_probeSimulator.getCoilParams(motorPositions);
    }

    /**
     * Calculate the coil parameters for the current capacitor positions
     * using the default probe.
     * The capacitor positions are read from the "ptuneCapPositions" file.
     * @return The coil parameters.
     */
    private CoilParams[] getCurrentCoilParamsForDefaultProbe() {
        double f0 = 400e6;      // Calc. nominal values for this frequency
        double w = 2 * PI * f0; // omega
        double lCoil = 100e-9;
        double rCoil = 0.5;
        double abs2Coil = rCoil * rCoil + w * w * lCoil * lCoil;
        double react = sqrt((50 * abs2Coil - 50 * 50 * rCoil) / rCoil);
        double cMatch = 1 / (react * w); // Nominal match capacitance
        double cTune = 1 / (w * w * lCoil); // Nominal tune capacitance

        // Check the motor positions and tweak the cap values
        double tune1 = cTune;
        double match1 = cMatch;

        CapPositions[] capPositions = getCapPositions();
        if (capPositions.length > 0) {
            tune1 *= (1 + capPositions[0].tune);
            match1 *= (1 + 5 * capPositions[0].match);
        }

        // Log values that change
        if (m_prevTune != tune1) {
            m_prevTune = tune1;
            System.out.println("tune = " + Fmt.g(5, tune1 * 1e12) + " pF");
        }
        if (m_prevMatch != match1) {
            m_prevMatch = match1;
            System.out.println("match = " + Fmt.g(5, match1 * 1e12) + " pF");
        }

        CoilParams[] cpars = {new CoilParams(lCoil, rCoil, tune1, match1)};
        return cpars;
    }

    /**
     * Calculates the tune and match capacitances needed to tune our model
     * coil to the given frequency with given coil inductance and resistence.
     * This is an approximate calculation, just to get the cap values in
     * the ballpark. The match is fairly good; the frequency can be off by
     * more than 10 MHz.
     * @param freq The frequency to tune to (Hz).
     * @param coilParams The coil parameters, with only rCoil and lCoil set.
     * @return The same CoilParams object, with cTune and cMatch set.
     * respectively (F).
     */
    public static CoilParams calculateNominalCaps(double freq,
                                                CoilParams coilParams) {
        double rCoil = coilParams.rCoil;
        double lCoil = coilParams.lCoil;
        double w = 2 * PI * freq; // omega
        double abs2Coil = rCoil * rCoil + w * w * lCoil * lCoil;
        double react = sqrt((50 * abs2Coil - 50 * 50 * rCoil) / rCoil);
        coilParams.cMatch = 1 / (react * w); // Nominal match capacitance
        coilParams.cTune = 1 / (w * w * lCoil); // Nominal tune capacitance
        return coilParams;
    }

    /**
     * Calculates the complex reflection for the given resonant coil
     * at the given frequency.
     * @param freq The frequency to calculate the reflection for (Hz).
     * @param coilParams The coil parameters to use.
     * @return The complex reflection.
     */
    public static Complex calculateReflection(double freq,
                                              CoilParams coilParams) {
        CoilParams[] paramsArray = {coilParams};
        return calculateReflection(freq, paramsArray);
    }

    /**
     * Calculates the complex reflection for the given resonant coils
     * at the given frequency.
     * Each coil gives a different resonance.
     * @param freq The frequency to calculate the reflection for (Hz).
     * @param coilParams The parameters of all the coils of interest.
     * @return The complex reflection.
     */
    public static Complex calculateReflection(double freq,
                                              CoilParams[] coilParams) {
        freq -= m_offset_hz;
        Complex r = COMPLEX_ONE;
        for (CoilParams params : coilParams) {
            double w = 2 * PI * freq; // omega; circular freq
            Complex zMatch = new Complex(0, -1 / (w * params.cMatch));
            Complex zTune = new Complex(0, -1 / (w * params.cTune));
            Complex zCoil = new Complex(params.rCoil, w * params.lCoil);
            // z = zMatch + 1 / (1/zTune + 1/zCoil)
            Complex z = zMatch.plus((zTune.inv().plus(zCoil.inv())).inv());
            Complex z0 = COMPLEX_50; // 50 Ohm load
            // reflection = (z - z0) / (z + z0)
            r = r.times(z.minus(z0).div(z.plus(z0)));
        }
        return r;
    }

    /**
     * Get all the current motor positions from a file.
     * These positions are corrected to the virtual capacitor positions,
     * which include simulated backlash.
     * @return The list of positions, indexed by gmi.
     */
    private int[] getAllMotorPositions() {
        ArrayList<Integer> motorPositions = new ArrayList<Integer>();

        BufferedReader in = TuneUtilities.getReader(sm_motorpath);
        if (in == null) {
            Messages.postError("Failed to get motor position file: "
                               + sm_motorpath);
        } else {
            try {
                String line;
                while ((line = in.readLine()) != null) {
                    try {
                        int position = Integer.parseInt(line);
                        motorPositions.add(position);
                    } catch (NumberFormatException nfe) {
                        motorPositions.add(0);
                    }
                }
            } catch (IOException ioe) {
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }
        }
        int[] rtn = new int[motorPositions.size()];
        for (int i = 0; i < motorPositions.size(); i++) {
            rtn[i] = motorPositions.get(i);
        }
        return rtn;
    }

    /**
     * This reads "capacitor positions" from a file.
     * (Legacy support for simulating a single, hard-coded resonance.)
     * @return The array of cap positions.
     */
    private CapPositions[] getCapPositions() {
        ArrayList<CapPositions> capPositions = new ArrayList<CapPositions>();

        BufferedReader in = TuneUtilities.getReader(sm_cappath);
        if (in == null) {
            Messages.postError("Failed to get cap position file: " + sm_cappath);
        } else {
            try {
                String line;
                while ((line = in.readLine()) != null) {
                    //Messages.postDebug("Mtune: getCapPositions line=" + line);
                    try {
                        StringTokenizer toker = new StringTokenizer(line);
                        if (toker.countTokens() == 3) {
                            String key = toker.nextToken();
                            int idx = Integer.parseInt(toker.nextToken());
                            double val = Double.parseDouble(toker.nextToken());

                            // Make sure we have a place to put the value
                            while (idx >= capPositions.size()) {
                                capPositions.add(null);
                            }
                            CapPositions thesePositions = capPositions.get(idx);
                            if (thesePositions == null) {
                                thesePositions = new CapPositions();
                                capPositions.set(idx, thesePositions);
                            }

                            // Set the appropriate value
                            if (key.equals("tune")) {
                                thesePositions.tune = val;
                            } else if (key.equals("match")) {
                                thesePositions.match = val;
                            }
                        }
                    } catch (NoSuchElementException nsee) {
                        Messages.postError(nsee.toString());
                    }
                }
            } catch (IOException ioe) {
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }
        }
        //rtn[0].match = -0.2;
        //rtn[0].tune = -0.04;
        return capPositions.toArray(new CapPositions[0]);
    }


    /**
     * Class to hold the current values of the capacitor positions
     * in the probe.
     * (Legacy support for simulating a single, hard-coded resonance.)
     */
    static public class CapPositions {
        /**
         * Value of the tune capacitor as a fractional offset from the
         * nominal value.
         * Capacitance is (nominal * (1 + tune)) farads.
         */
        public double tune = 0;

        /**
         * Value of the match capacitor as a fractional offset from the
         * nominal value.
         * Capacitance is (nominal * (1 + match)) farads.
         */
        public double match = 0;


        /**
         * Reset the capacitor positions to their nominal values.
         */
        public void clear() {
            tune = 0;
            match = 0;
        }
    }


    /**
     * Thread to write the data after every "sweep".
     */
    public class DataWriter extends Thread {

        private boolean mm_quit = false;

        public DataWriter() {
            Messages.postDebug("Mtune.DataWriter created");
        }

        public void quit(int ms) throws InterruptedException {
            quit();
            interrupt();
            join(ms);
        }

        public void quit() {
            mm_quit = true;
        }

        public void run() {
            Messages.postDebug("Mtune.DataWriter running");
            setName("Mtune.DataWriter");

            while (!mm_quit) {
                readSweepParameters();
                long crc = writeFidData();
                writeHeaderInfo(crc);
                try {
                    Thread.sleep(100);
                } catch (InterruptedException ie) {
                }
            }
        }
    }


    /**
     * Thread to read text input from the terminal.
     * Currently used only to indicate whether to simulate the probe,
     * or a cable terminated with open/load/short.
     */
    public class ConsoleReader extends Thread {
    	private boolean mm_forceQuit = false;
        private boolean mm_quit = false;
        private BufferedReader mm_reader;

        public ConsoleReader() {
            mm_reader = new BufferedReader(new InputStreamReader(System.in));
            Messages.postDebug("Mtune.ConsoleReader created");
        }

        public void quit(int ms) throws InterruptedException, IOException {
	    mm_forceQuit = true;
            quit();
            interrupt();
            join(ms);
        }
        
        public void quit() {
            mm_quit = true;
        }
        
        public void run() {
            Messages.postDebug("Mtune.ConsoleReader running");
            setName("Mtune.ConsoleReader");
            while (!mm_quit) {
                try {
                    String line = mm_reader.readLine();
                    if (line == null || "quit".equalsIgnoreCase(line)) {
                        quit();
                    } else {
                        line = line.trim();
                        try {
                            m_offset_hz = 1e6 * Double.parseDouble(line);
                        } catch (NumberFormatException nfe) {
                        }
                        if ("open".equalsIgnoreCase(line)) {
                            m_termination = Termination.OPEN;
                        } else if ("load".equalsIgnoreCase(line)) {
                            m_termination = Termination.LOAD;
                        } else if ("short".equalsIgnoreCase(line)) {
                            m_termination = Termination.SHORT;
                        } else {
                            m_termination = Termination.PROBE;
                        }
                        if (m_offset_hz != 0) {
                            Messages.postDebug("Dip shift="
                                               + Fmt.f(2, m_offset_hz / 1e6)
                                               + " MHz");
                        }
                        Messages.postDebug("Termination=" + m_termination);
                    }
                } catch (IOException e) {
                    // ClosedByInterruptException occurs during shutdown
                    if (!mm_forceQuit)
                        e.printStackTrace();
                    quit();
                }
            }
        }
    }


    /**
     * Container class for the values used to calculate the reflection due
     * to a resonance.
     */
    static class CoilParams implements Cloneable {
        /** Coil inductance. */
        public double lCoil;
        /** Coil resistance. */
        public double rCoil;
        /** Tune capacitor value. */
        public double cTune;
        /** Match capacitor value. */
        public double cMatch;

        public CoilParams(double l, double r, double tune, double match) {
            lCoil = l;
            rCoil = r;
            cTune = tune;
            cMatch = match;
        }

        public CoilParams clone() {
            return new CoilParams(lCoil, rCoil, cTune, cMatch);
        }
    }


}
