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

import vnmr.util.Complex;
import vnmr.util.Fmt;
import vnmr.util.LinFit;
import vnmr.util.NLFit;

/**
 * A data structure class for data from one frequency sweep.
 */
public class ReflectionData implements Serializable {


    private static Integer m_dataMarginPct;
    private static Integer m_dataMargin;
    private static boolean m_checkFitRange;
    private static double m_minCircleSizeS2N;
    private static Double m_minCircleSize;
    private static double m_maxCircleR;
    private static double m_maxCircleBulge;
    private static double m_minCircleS2N;
    private static int m_minFwdPct;
    private static int m_maxBack;
    private static double m_minFreqFitChisqReduce;

    public long birthtime = 0;
    public int size = 0;
    public double[] real = null;
    public double[] imag = null;
    public double[] reflection2 = null; // Square of reflection coef
    public boolean[] deviants = null; // Flag points to ignore in noise calc
    public double startFreq;
    public double stopFreq;
    public String stringData;

    private int m_power = -1;
    private int m_gain = -1;
    private int m_rfchan = 0;
    private long m_acqTime = 0;
    private boolean m_usePhaseData = true;
    private double m_maxReflection2 = 0;
    private double m_minReflection2 = Double.MAX_VALUE;

    // Values that can be calculated for the data
    private boolean m_failed = false;
    private double dipIndex = Double.NaN;
    private double dipValue = Double.NaN; // The linear reflection at the dip
    private double dipValueSdv = Double.NaN;
    private double dipFreq = Double.NaN;
    private double dipFreqSdv = Double.NaN;
    private Dip[] m_dips = new Dip[1];
    private double m_curvatureValue = Double.NaN;
    private double m_noise = Double.NaN;
//    private List<Double> sortedValues = null;
//    private double[] m_x = new double[3];
//    private double[] m_y = new double[3];
    private boolean m_dipCalculated = false;
    /** The correction applied for probeDelay time. */
    private double m_probeDelayCorrection_ns = 0;
    public boolean m_calFailed = false;
    private double m_dipPhase = Double.NaN;
    private double m_meanAbsval = Double.NaN;

    private static final long serialVersionUID = 42L;

    static {
        setOptions();
    }

    public ReflectionData(String s, double start, double stop) {
        this(s, start, stop, "phase");
    }

    /**
     * Construct ReflectionData from a Comma Delimited String of
     * real/imaginary coordinates.  Pairs of data points are assumed
     * to be equally spaced in frequency. There is an optional first
     * token giving the sign of the reflection.
     * @param s The Comma Delimited String.
     * @param start The frequency at the first data point.
     * @param stop The frequency at the last data point.
     * @param type "absval" if the phase information is to be ignored.
     */
    public  ReflectionData(String s, double start, double stop, String type) {
        /*ProbeTune.printlnErrorMessage(5, "ReflectionData.<init>: start="
                           + start + ", stop=" + stop);/*DBG*/
        //ProbeTune.printlnErrorMessage(5, "ReflectionData<init>: " + s);/*DBG*/
        m_acqTime = birthtime = System.currentTimeMillis();
        StringTokenizer toker = new StringTokenizer(s, ", \t");
        size = toker.countTokens();
        if ((size & 1) == 1) {
            // Odd number; first token should be sign
            toker.nextToken(); // Ignore it
        }
        if (type == null) {
            // default behavior
        } else if (type.equals("absval")) {
            m_usePhaseData = false;
        }
        size /= 2;
        startFreq = start;
        stopFreq = stop;
        real = new double[size];
        imag = new double[size];
        reflection2 = new double[size];
        deviants = new boolean[size];
        for (int i = 0; i < size; i++) {
            double x = 0;
            double y = 0;
            try {
                x = Double.parseDouble(toker.nextToken());
                y = Double.parseDouble(toker.nextToken());
            } catch (NumberFormatException nfe) {
            }
            real[i] = x;
            imag[i] = y;
            reflection2[i] = x * x + y * y;
            if (m_maxReflection2 < reflection2[i]) {
                m_maxReflection2 = reflection2[i];
            } else if (m_minReflection2 > reflection2[i]) {
                m_minReflection2 = reflection2[i];
            }
        }
    }

    /**
     * Construct ReflectionData from a string containing "meta" info
     * and data.  Tokens are delimited by commas, spaces, tags,
     * returns, or linefeeds, which are all equivalent. The format of
     * the data is a keyword followed by zero or more numeric fields,
     * followed by more keyword fields. If an unknown keyword is
     * found, it is ignored along with any following numeric tokens;
     * the next non-numeric token is taken as the next keyword.  The
     * "data" keyword should be the last keyword in the string.  Data
     * are equally spaced in frequency.
     * <br>
     * Recognized keywords are:
     * <br>
     * start <value>: The frequency of the first point, in Hz. (Required)
     * <br>
     * stop <value>: The frequency of the last point, in Hz. (Required)
     * <br>
     * absval: The data are the absolute values of the reflection
     * at every frequency. (Required, if that's what it is)
     * <br>
     * complex: The data are pairs of values giving the real and
     * imaginary reflection at every frequency. (Optional: the default)
     * <br>
     * sign <value>: For "absval" data, this can indicate the sign of
     * the reflection. The sign is taken as negative iff value<0.  If
     * "sign" is present for "complex" data, the complex data will be
     * used only to construct the absval data, and no other
     * algorithmic use will be made of it.  (Optional: default is
     * positive)
     * <br>
     * date <value>: The system time in ms when the data was acquired.
     * (Optional)
     * <br>
     * size <value>: The number of frequency points. (Optional:
     * if not present, counts the number of tokens following the "data"
     * keyword)
     * <br>
     * data <value> ... : The data values.
     * @param s The string containing the reflection data.
     */
    public  ReflectionData(String s) {
        /*ProbeTune.printlnErrorMessage(5, "ReflectionData.<init>: start="
                            + start + ", stop=" + stop);/*DBG*/
        //ProbeTune.printlnErrorMessage(5, "ReflectionData<init>: " + s);/*DBG*/
        //stringData = s;
        int skip = 0;
        boolean absvalFlag = false;
        boolean scaledFlag = true;
        m_acqTime = birthtime = System.currentTimeMillis();
        StringTokenizer toker = new StringTokenizer(s, ", \t\r\n");
        while (toker.hasMoreTokens()) {
            String key = toker.nextToken().toLowerCase();
            if (key.equals("start")) {
                startFreq = Double.parseDouble(toker.nextToken());
            } else if (key.equals("stop")) {
                stopFreq = Double.parseDouble(toker.nextToken());
            } else if (key.equals("power")) {
                m_power = Integer.parseInt(toker.nextToken());
            } else if (key.equals("gain")) {
                m_gain = Integer.parseInt(toker.nextToken());
            } else if (key.equals("absval")) {
                absvalFlag = true;
            } else if (key.equals("complex")) {
                absvalFlag = false;
            } else if (key.equals("scaled")) {
                scaledFlag = true;
            } else if (key.equals("nophase")) {
                m_usePhaseData = false;
            } else if (key.equals("sign")) {
                toker.nextToken(); // Ignore it
                m_usePhaseData = false;
            } else if (key.equals("acqdate")) {
                m_acqTime = Long.parseLong(toker.nextToken());
            } else if (key.equals("size")) {
                size = Integer.parseInt(toker.nextToken());
            } else if (key.equals("calfailed")) {
                m_calFailed = true;
            } else if (key.equals("skip")) {
                // Number of initial points to skip
                skip = Integer.parseInt(toker.nextToken());
            } else if (key.equals("data")) {
                if (size == 0) {
                    size = toker.countTokens();
                    if (!absvalFlag) {
                        size /= 2;
                    }
                }
                for (int i = 0; i < skip; i++) {
                    toker.nextToken(); // Skip bad point
                    if (!absvalFlag) {
                        toker.nextToken(); // Skip bad imaginary part
                    }
                }
                // This puts the data in the class member buffers
                parseData(toker, size, absvalFlag);
            }
        }
        if (!scaledFlag) {
            scaleToMaxAbsval();
        }
    }

    /**
     * Constructor used when we get data from FID file.
     * @param realData The real data as an array.
     * @param imagData The imaginary data as an array.
     * @param acqTime The clock time that the data was read.
     * @param start The start frequency (Hz).
     * @param stop The stop frequency (Hz).
     * @param power The RF power.
     * @param gain The receiver gain.
     * @param rfchan The RF channel.
     */
    public  ReflectionData(double[] realData, double[] imagData,
                           long acqTime,
                           double start, double stop,
                           int power, int gain, int rfchan) {
        real = realData;
        imag = imagData;
        size = Math.min(real.length, imag.length);
        startFreq = start;
        stopFreq = stop;
        m_acqTime = acqTime;
        m_power = power;
        m_gain = gain;
        m_rfchan = rfchan;
        m_usePhaseData = true;
        reflection2 = new double[size];
        for (int i = 0; i < size; i++) {
            reflection2[i] = real[i] * real[i] + imag[i] * imag[i];
            if (m_maxReflection2 < reflection2[i]) {
                m_maxReflection2 = reflection2[i];
            } else if (m_minReflection2 > reflection2[i]) {
                m_minReflection2 = reflection2[i];
            }
        }
    }

    /**
     * Initialize the criteria for recognizing a resonance.
     */
    public static void setOptions() {
        // Initialize various criteria for accepting a dip as a real resonance.
        // Read System properties to get user customizations.

        // How far _inside_ the data range the dip needs to be.
        // By default, the dip can be _outside_ the data range.
        final int BAD_MARGIN = 99987678;
        String key = "apt.dataMarginPct";
        m_dataMarginPct = TuneUtilities.getIntProperty(key, BAD_MARGIN);
        if (m_dataMarginPct == BAD_MARGIN) {
            m_dataMarginPct = null;
        }
        key = "apt.dataMargin";
        int margin = TuneUtilities.getIntProperty(key, BAD_MARGIN);
        if (margin == BAD_MARGIN) {
            m_dataMargin = null;
        } else {
            m_dataMargin = margin;
        }

        key = "apt.checkFitRange";
        m_checkFitRange = TuneUtilities.getBooleanProperty(key, true);

        key = "apt.minCircleSizeS2N";
        m_minCircleSizeS2N = TuneUtilities.getDoubleProperty(key, 3.0);
        key = "apt.minCircleSize";
        double minSize = TuneUtilities.getDoubleProperty(key, Double.NaN);
        if (Double.isNaN(minSize)) {
            m_minCircleSize = null;
        } else {
            m_minCircleSize = minSize;
        }

        key = "apt.maxCircleCenterR";
        m_maxCircleR = TuneUtilities.getDoubleProperty(key, 1.0);
        key = "apt.maxCircleExtentR";
        m_maxCircleBulge = TuneUtilities.getDoubleProperty(key, 2.0);

        key = "apt.minCircleFitSignalToNoise";
        m_minCircleS2N = TuneUtilities.getDoubleProperty(key, 20);

        key = "apt.minFreqsMonotonicPct";
        m_minFwdPct = TuneUtilities.getIntProperty(key, 90);
        key = "apt.maxFreqsNonMonotonic";
        m_maxBack = TuneUtilities.getIntProperty(key, 2);

        // NB: The following factor should really depend on degrees
        // of freedom:
        //  use Student's t-Distribution F(t) (actually F(t, df))
        //  to find t such that F(t)=0.9995 for the degrees of freedom
        //  we have. (A reasonable answer is t ~= 4 if df > 10.)
        //  Is it reasonable to pull in a t-Distribution calculation
        //  just to get a better answer? For stat functions, see e.g.:
        //  http://dsd.lbl.gov/~hoschek/colt/
        //  Look under the feature "statistics - probability".
        key = "apt.minFreqChisqReduction";
        m_minFreqFitChisqReduce = TuneUtilities.getDoubleProperty(key, 4.0);
    }

    /**
     * Calculates an estimate of the variance in the amplitude
     * for each frequency point.
     * @return An array of variance values for each frequency point.
     */
    protected double[] measureNoiseVsFreq() {
        // Number of points averaged to get a variance estimate
        final int nAvg = Math.min(9, size - 1) | 1; // Must be odd

        double[] variance = new double[size];
        double rSum = 0;
        double r2Sum = 0;

        // Calculate first point
        for (int i = 0; i < nAvg; i++) {
            rSum += Math.sqrt(reflection2[i]);
            r2Sum += reflection2[i];
        }
        variance[nAvg / 2] = (r2Sum - rSum * rSum / nAvg) / (nAvg - 1);

        // First nAvg/2 points are the same
        for (int i = 0; i < nAvg / 2; i++) {
            variance[i] = variance[nAvg / 2];
        }

        // Calculate the middle (size - nAvg) points
        for (int i = nAvg / 2 + 1; i < size - nAvg / 2; i++) {
            int j = i + nAvg / 2; // Add this point to sum
            int k = i - nAvg / 2 - 1; // Remove this point from sum
            rSum += Math.sqrt(reflection2[j]) - Math.sqrt(reflection2[k]);
            r2Sum += reflection2[j] - reflection2[k];
            variance[i] = (r2Sum - rSum * rSum / nAvg) / (nAvg - 1);
        }

        // Last nAvg/2 points are the same
        for (int i = size - nAvg / 2; i < size; i++) {
            variance[i] = variance[size - nAvg / 2 - 1];
        }
        return variance;
    }

    /**
     * Fits line "phase = a + b*freq" to the data.
     * @param dip A known dip to ignore.
     * @param sigma The relative standard deviation of a measurement.
     * @return Array of {a, b, sigma(a), sigma(b)}.
     */
    public double[] measurePhaseVsFreq(Dip dip, double[] sigma) {
        // Assign weights according to noise
        double[] weights = new double[size];
        for (int i = 0; i < size; i++) {
            weights[i] = 1 / (sigma[i] * sigma[i]);
            //if (weights[i] < 10) {
            //    weights[i] = 0;
            //}
        }

        // Exclude region around dip
        Interval exclude = null;
        if (dip != null) {
            exclude = dip.getPerturbedInterval();
            for (int i = exclude.start; i <= exclude.stop; i++) {
                weights[i] = 0;
            }
        }

        // TODO: Check for too few points w/ non-zero weight


        // Build angle (phase) and frequency arrays
        double[] phases = new double[size];
        double[] freqs = new double[size];
        double df = (stopFreq - startFreq) / (size - 1);
        for (int i = 0; i < size; i++) {
            phases[i] = Math.atan2(imag[i], real[i]);
            freqs[i] = startFreq  + i * df;
        }

        if (dip != null) {
            // Correct phases for dip
            for (int i = 0; i < size; i++) {
                phases[i] -= dip.getDeltaPhi(i);
            }
        }
        wrapAngles(phases, exclude); // Eliminates any 2*PI discontinuities
        if (DebugOutput.isSetFor("MeasureProbeDelay")) {
            String path = ProbeTune.getVnmrSystemDir() + "/tmp/angleVsFreq";
            new File(path).delete();
            String header = "# frequency   theta   theta*weight  weight";
            double start = startFreq;
            double dfreq = getDeltaFreq();
            for (int i = 0; i < phases.length; i++) {
                double freq = start + i * dfreq;
                double phase = (weights[i] == 0) ? 0 : phases[i];
                String buffer = freq + "  " + phases[i] + "  "
                        + phase + "   " + weights[i];
                TuneUtilities.appendLog(path, buffer, header);
            }
        }

        // Calculate the fit
        double[] linFit = TuneUtilities.linFit(freqs, phases, weights);

        return linFit;
    }

    /**
     * Undoes the phase wrapping caused by the probe delay
     * (the round-trip group delay time between the probe port and the
     * resonant circuit).
     * @param probeDelay_ns The new delay value (ns).
     */
    public void correctForProbeDelay_ns(double probeDelay_ns) {
        // Account for any previous correction
        double correction =  probeDelay_ns - m_probeDelayCorrection_ns;
        if (correction != 0) {
            double dtheta_dfreq = 2 * Math.PI * correction * 1e-9;
            double freq = startFreq;
            double step = (stopFreq - startFreq) / (size - 1);
            for (int i = 0; i < size; i++, freq += step) {
                double theta = Math.atan2(imag[i], real[i]);
                double r = Math.hypot(real[i], imag[i]);
                theta += dtheta_dfreq * freq;
                imag[i] = r * Math.sin(theta);
                real[i] = r * Math.cos(theta);
            }
        }
        m_probeDelayCorrection_ns = probeDelay_ns;
    }

    public void scaleToMaxAbsval() {
        if (m_maxReflection2 > 0) {
            double scale2 = 1 / m_maxReflection2;
            double scale = Math.sqrt(scale2);
            for (int i = 0; i < size; i++) {
                real[i] *= scale;
                imag[i] *= scale;
                reflection2[i] *= scale2;
            }
            m_maxReflection2 = 1;
        }
    }

    /**
     * Given a StringTokenizer with the string representations on the
     * reflection amplitude at n frequencies, sets member variables that
     * record the data.
     * For complex data, sets real, imag, and reflection2 (r^2) for each point.
     * For absolute value data, only sets reflection2.
     * In either case, calculates m_maxReflection2 and m_minReflection2.
     * @param toker Contains all the tokens of the data string.
     * @param n The number of frequencies.
     * @param absvalFlag True if this is complex data.
     */
    private void parseData(StringTokenizer toker, int n, boolean absvalFlag) {
        reflection2 = new double[n];
        deviants = new boolean[n];
        if (!absvalFlag) {
            real = new double[n];
            imag = new double[n];
        }

        if (absvalFlag) {
            for (int i = 0; i < n; i++) {
                try {
                    double r = Double.parseDouble(toker.nextToken());
                    reflection2[i] = r * r;
                    if (m_maxReflection2 < reflection2[i]) {
                        m_maxReflection2 = reflection2[i];
                    } else if (m_minReflection2 > reflection2[i]) {
                        m_minReflection2 = reflection2[i];
                    }
                } catch (NumberFormatException nfe) {
                }
            }
        } else {
            for (int i = 0; i < n; i++) {
                try {
                    double x = Double.parseDouble(toker.nextToken());
                    double y = Double.parseDouble(toker.nextToken());
                    real[i] = x;
                    imag[i] = y;
                    reflection2[i] = x * x + y * y;
                    if (m_maxReflection2 < reflection2[i]) {
                        m_maxReflection2 = reflection2[i];
                    } else if (m_minReflection2 > reflection2[i]) {
                        m_minReflection2 = reflection2[i];
                    }
                } catch (NumberFormatException nfe) {
                }
            }
        }
    }

    /**
     * Whether we want to use data phase in the tune algorithm.
     * @return True if phase information is to be used.
     */
    public boolean isPhasedDataUsed() {
        return m_usePhaseData;
    }

    /**
     * Check if data are all zero.
     * @return True if all data are zero.
     */
    public boolean isZero() {
        boolean rtn = true;
        for (int i = 0; i < size; i++) {
                if (real[i] != 0 || imag[i] != 0) {
                        rtn = false;
                        break;
                }
        }
        return rtn;
    }

    /**
     * Whether we want to use data phase in the tune algorithm.
     * @param b True if phase information is to be used.
     */
    public void setPhasedDataUsed(boolean b) {
        m_usePhaseData = b;
    }

    public Dip[] getDips() {
        return m_dips;
    }

    public void setDips(Dip[] dips) {
        if (dips == null) {
            m_dips = new Dip[1];
        } else {
            m_dips = dips;
        }
    }

    public double getDipIndex() {
        if (Double.isNaN(dipIndex)) {
            calculateDip();
        }
        return dipIndex;
    }

    public double getDipFreq() {
        if (Double.isNaN(dipFreq)) {
            calculateDip();
        }
        return dipFreq;
    }

    public double getDipFreqSdv() {
        if (Double.isNaN(dipFreqSdv)) {
            calculateDip();
        }
        return dipFreqSdv;
    }

    public double getDipValue() {
        if (Double.isNaN(dipValue)) {
            calculateDip();
        }
        return dipValue;
    }

    public double getDipValueSdv() {
        if (Double.isNaN(dipValueSdv)) {
            calculateDip();
        }
        return dipValueSdv;
    }

    public double getDipPhase() {
        if (Double.isNaN(m_dipPhase)) {
            calculateDip();
        }
        return m_dipPhase;
    }

//    /**
//     * @return Absolute value of dr/df (fraction reflected / Hz).
//     */
//    public double getDipDrDf() {
//        double rtn = 1e-6;     // Ballpark guess is default (approx 1 / FWHM)
//        if (m_dips != null && m_dips.length > 0) {
//            rtn = m_dips[0].getDrDf();
//        }
//        return rtn;
//    }

    /**
     * The curvature is the coefficient of x^2 in the fit to the r^2 data.
     * The reflection ,"r", ranges from 0 to 1, and x is the frequency
     * change in data points (i.e., if the diff between points is 1 kHz, the
     * units of x is kHz).
     * @return The curvature coefficient.
     */
    public double getCurvature() {
        if (Double.isNaN(m_curvatureValue)) {
            calculateDip();
        }
        return m_curvatureValue;
    }

    public double getMatchAtFreq(double freq) {
        if (Double.isNaN(freq)) {
            return Double.NaN;
        }
        double[] r = null;
        // See if this freq is within a calculated dip
        double dIx = getIndexAtFrequency(freq);
        int ix = (int)Math.round(dIx); // The index for the requested freq
        Dip[] dips = getDips();
        for (Dip dip : dips) {
            if (dip != null) {
                Interval dipRange = dip.getInterval();
                if (dipRange.start == 0) {
                    dipRange.start = -2;
                }
                if (dipRange.stop == size - 1) {
                    dipRange.stop = size + 1;
                }
                if (dipRange.contains(ix)) {
                    r = dip.getReflectionAt(freq);
                }
            }
        }
        if (r == null) {
            r = getComplexReflectionAtFreq(freq);
        }
        return r[0] * r[0] + r[1] * r[1];
    }

    /**
     * Get the real and imaginary values of the reflection at a
     * given frequency.  (Used to plot the target frequency on the
     * polar plot of the reflection.)
     * @param freq The frequency of interest in Hz.
     * @return An array of two doubles, giving the real and imaginary
     * parts of the reflection, respectively.
     */
    public double[] getComplexReflectionAtFreq(double freq) {
        double[] rtn = {Double.NaN, Double.NaN};
        if (Double.isNaN(freq)) {
            return rtn;
        }

        double dIx = getIndexAtFrequency(freq);
        int ix = (int)Math.round(dIx);
        if (ix < -1 || ix > size) {
            return rtn;
        }
        if (ix < 2) {
            ix = 2;
        }
        if (ix > size - 3) {
            ix = size - 3;
        }
        if (ix < 2) {
            return rtn;
        }

        double[] x = new double[5];
        double[] y = new double[5];
        double[] z = new double[5];
        int j = ix - 2;
        for (int i = 0; i < 5; i++) {
            x[i] = i;
            y[i] = real[j];
            z[i] = imag[j++];
        }
        double[] fit = TuneUtilities.polyFit(x, y, 2);
        rtn[0] = TuneUtilities.polyValue(dIx - ix + 2, fit);
        fit = TuneUtilities.polyFit(x, z, 2);
        rtn[1] = TuneUtilities.polyValue(dIx - ix + 2, fit);
        return rtn;
    }


    /**
     * Returns true iff there is a valid resonance in this data.
     * @return True if a dip was detected.
     */
    public boolean isDipDetected() {
//        if (m_minReflection2 < 0.666) {
//            return true;
//        }

        boolean rtn = false;
        Dip[] dips = getDips();
        for (int i = 0; i < dips.length; i++) {
            if (dips[i] != null && dips[i].isResonance()) {
                rtn = true;
                break;
            }
        }
        return rtn;
    }

    /**
     * Calculates the noise in the baseline reflection.
     * @return The estimated noise (sigma).
     */
    public double getBaselineNoise() {
        double sigma = 0;
        int chunks = 10;
        int minChunkSize = 30;
        if (/*Double.isNaN(m_noise) && !m_failed*/ true) {
            SortedSet<Double> sigmas = new TreeSet<Double>();
            int n = size / chunks; // # of points in each sigma estimate
            if (n < minChunkSize) {
                n = minChunkSize;
            }
            int begin = 0;
            while (begin < size - n) {
                int end = begin + n;
                if (end > size) {
                    end = size;
                }
                double v;
                v = TuneUtilities.rootPolyResidual(reflection2, begin, end, 2);
                sigmas.add(v * v);
                //ProbeTune.printlnDebugMessage(5, "m_noise=" + Fmt.f(8, v));
                begin = end;
            }
            if (sigmas.size() == 0) {
                Messages.postDebugWarning("No values for baseline noise");
            } else {
                // Put sorted sigmas into an array.
                // Sigmas near end of array will be too large because of
                // resonance and / or other features in data.
                // Average leading "i" sigmas until the next sigma deviates
                // too much from the average.
                // Note: there are n-3 degrees of freedom in each sigma sample.
                Double[] arraySigmas = sigmas.toArray(new Double[0]);
                sigma = arraySigmas[0];
                double sampleSizeFactor = Math.sqrt(n - 3);
                for (int i = 1; i < arraySigmas.length; i++) {
                    // Calculate next mean sigma; previous one has "i" samples
                    double tst = (sigma * i + arraySigmas[i]) / (i + 1);
                    if (arraySigmas[i] - tst > 4 * tst / sampleSizeFactor) {
                        break;
                    }
                    sigma = tst;
                }
            }
        }
        /*ProbeTune.printlnDebugMessage(5, "ReflectionData.getNoise: m_noise="
                           + Fmt.f(8, m_noise));*/
        m_noise = Math.sqrt(sigma);
        return m_noise;
    }

    /**
     * Returns the noise in the baseline reflection^2.
     * TODO: getNoise is not currently used.
     * @param exclude An array of boolean the same length as the data set.
     * If an element is "true", the corresponding data point is ignored.
     * @return The standard deviation.
     */
    public double getNoise(boolean[] exclude) {

        int nExcluded = 0;
        for (int i = 0; i < size; i++) {
            if (exclude[i]) {
                nExcluded++;
            }
        }
        Messages.postDebug("DipFind", "getNoise: nExcluded = " + nExcluded);

        if (!Double.isNaN(m_noise)) {
            return m_noise;
        }
        if (exclude.length != size) {
            Messages.postDebugWarning("ReflectionData.getNoise: "
                                      + "bad exclude list");
            return -1;
        }

        double mean = 0;
        int n = 0;
        for (int i = 0; i < size; i++) {
            if (!exclude[i]) {
                double diff = reflection2[i];
                mean += diff;
                n++;
            }
        }
        if (n < 2) {
            Messages.postDebugWarning("ReflectionData.getNoise: "
                                      + "too few points");
            return -1;
        }
        mean /= n;
        Messages.postDebug("DipFind", "getNoise: mean=" + mean);

        // Calculate Standard Deviation
        double sdv = 0;
        for (int i = 0; i < size; i++) {
            if (!exclude[i]) {
                double diff = reflection2[i] - mean;
                sdv += diff * diff;
            }
        }
        sdv = Math.sqrt(sdv / (n - 1));
        Messages.postDebug("DipFind", "n=" + n + ", sdv=" + sdv );
        /******************************/
        double tol = 3.0 * sdv;
        /******************************/

        // Find deviant points
        if (deviants == null) {
            deviants = new boolean[size]; // Initially all false
        }
        int nDeviant = 0;
        for (int i = 0; i < size; i++) {
            if (exclude[i]) {
                deviants[i] = true;
            } else {
                double diff = reflection2[i] - mean;
                if (Math.abs(diff) > tol) {
                    deviants[i] = true;
                    nDeviant++;
                }
            }
        }
        Messages.postDebug("DipFind", "getNoise: nDeviant=" + nDeviant
                           + ", nExcluded=" + nExcluded);

        if (nDeviant > 0) {
            sdv = getNoise(deviants);
        }

        return sdv;
    }

    /**
     * Calculate the sum of the difference between this data and a given
     * array of Complex data. This is the mean distance between
     * corresponding data points divided by the mean absval in this
     * ReflectionData.
     * @param otherData A Complex array equal in size to this data set.
     * @param exclude An interval to be excluded from the calculation.
     * @return The normalized mean difference between data points.
     */
    public double getNormalizedDiff(Complex[] otherData, Interval exclude) {
        double diff = 0;
        if (otherData.length != size) {
            Messages.postDebugWarning("ReflectionData.getDiff:"
                                      + "otherData is different length");
        } else {
            int n = 0;
            for (int i = 0; i < size; i++) {
                // Exclude points where "otherData" is zero
                if (!(otherData[i].mod() == 0)) {
                    Complex val = new Complex(real[i], imag[i]);
                    diff += val.minus(otherData[i]).mod();
                    n++;
                }
            }
            n = Math.max(n, 1);
            diff /= n * getMeanAbsval();
        }
        return diff;
    }

//    public boolean[] getDeviants() {
//        if (deviants == null) {
//            getBaselineNoise();
//        }
//        return deviants;
//    }

//    /**
//     * Get the 95th percentile of the squared reflection. That is, 5% of
//     * the data points have a larger reflection.
//     * @return The reflection**2 of the data point 5% from the top.
//     */
//    public double getBaseline() {
//        if (sortedValues == null) {
//            calculateSortedValues();
//        }
//        // Return 95th percentile value
//        int idx = (int)((size - 1) * 0.95);
//        return sortedValues.get(idx);
//    }
//
//    private void calculateSortedValues() {
//        sortedValues = new ArrayList<Double>();
//        for (int i = 0; i < size; i++) {
//            sortedValues.add(reflection2[i]);
//        }
//        Collections.sort(sortedValues);
//        //ProbeTune.printlnDebugMessage(5, sortedValues);
//    }

    public Dip findResonanceNear(int idx) {
        final int MIN_PTS_IN_FIT = 7;
        // Accept no dip with FWHM > this (Hz):
        final double MAX_DIP_WIDTH = 10e6;

        CircleFit circleFit = null;
//        CircleFit bestFit = null;
        int npts;
        int nptsUsed = 0;
        Messages.postDebug("DipFind", "\nfindResonanceNear()");
        for (npts = MIN_PTS_IN_FIT / 2; npts < size / 2; npts++) {
            if (npts > 10) {
                npts *= 1.1;
            }
            double arcWidth_Hz = npts * 2 * getDeltaFreq();
            if (arcWidth_Hz > MAX_DIP_WIDTH) {
                // We won't accept a dip this wide; stop wasting time
                Messages.postDebug("DipFind",
                                   "Dip is too wide ("
                                   + Fmt.f(1, arcWidth_Hz / 1e6)
                                   + " > " + Fmt.f(1, MAX_DIP_WIDTH / 1e6)
                                   + " MHz)");
                break;
            }
            int startIndex = Math.max(0, idx - npts);
            int endIndex = Math.min(size - 1, startIndex + 2 * npts);
            startIndex = Math.min(startIndex, endIndex - 2 * npts);
            startIndex = Math.max(0, startIndex);
            int n = endIndex - startIndex + 1;
            if (n > nptsUsed) {
                nptsUsed = n;
            } else {
                Messages.postDebug("DipFind", "Dip is as wide as the channel");
                break;
            }
            Messages.postDebug("DipFind",
                               "findResonanceNear(): nptsUsed=" + nptsUsed);

            circleFit = fitCircle(startIndex, endIndex);
            // FIXME: Improve criteria for how many points to include in the fit
            // When dip is "off the edge", can fit too few points and get
            // a very inaccurate fit. -- The "maxAngle" check is problematic.
            if (circleFit != null) {
                double[] theta = circleFit.getArcExcursions();
                double arcLength = circleFit.getArcLength();
                double maxAngle = Math.abs(circleFit.getMaxArcExcursion());
                boolean dipWithinArc = theta[0] * theta[1] < 0;
                if (arcLength > Math.PI * 1 || maxAngle > Math.PI * 0.7) {
                    Messages.postDebug("DipFind",
                                       "Dip fit region extends far enough ("
                                       + "maxAngle=" + Fmt.f(2, maxAngle)
                                       + "  from dip)");
                    break;
//                } else if (circleFit.betterThan(bestFit)) {
//                    /*System.err.println("findResonanceNear: bestFit set: npts="
//                                       + npts);/*DBG*/
//                    bestFit = circleFit;
                } else if (!dipWithinArc && arcLength > Math.PI * 0.7) {
                    Messages.postDebug("DipFind",
                                       "Dip outside arc and arcLen > 0.7*PI ("
                                       + Fmt.f(2, arcLength) + ")"
                                       + ", npts="
                                       + (endIndex - startIndex + 1));
                    break;
                    // TODO: Don't try to fit so much data if it is wiggly
                //} else if (circleFit.muchWorseThan(bestFit)) {
                //    break;
                } else if (npts * getDeltaFreq() > MAX_DIP_WIDTH / 2) {
                    // We won't accept a dip this wide; stop wasting time
                    Messages.postDebug("DipFind",
                                       "Dip is too wide ("
                                       + Fmt.f(1, npts * 2 * getDeltaFreq() / 1e6)
                                       + " > " + Fmt.f(1, MAX_DIP_WIDTH / 1e6)
                                       + " MHz)");
                    break;
                }
            }
        }

        Dip dip = null;
        if (circleFit != null) {
            dip = new Dip(circleFit);
        }
        //Dip dip = (bestFit != null) ? new Dip(bestFit) : new Dip(circleFit);
            /*if (dip != null) {
                System.out.println("findResonanceNear: bestDip.isResonance()="
                                   + dip.isResonance());
            }/*DBG*/
            //if (dip != null && !dip.isResonance()) {
            //    dip = null;
            //}
        m_dips[0] = dip;
        return m_dips[0];
    }

    /**
     * Starting at a given index position in the reflection data, look for
     * a valid resonance dip.
     * Successive searches starting at dip.getEndIndex()
     * will eventually go through all the data.
     * @param startIndex Start at this data index to look for dips.
     * @return The resonance, or null if none found.
     */
    public Dip findNextResonance(int startIndex) {
        /*System.err.println("ReflectionData.findNextResonance("
                           + startIndex + ")");/*DBG*/
        Dip dip = null;
        int idx = startIndex;
        while (idx < size) {
            // Fit circle to data from current index to as far as fits well
            CircleFit circleFit = fitCircleAt(idx);
            dip = new Dip(circleFit);
            double dipIdx = dip.getIndex();
            int startFitIndex = dip.getStartIndex();
            int endFitIndex = dip.getEndIndex();
            if (dip.isResonance()
                && ((dipIdx < 0 && startIndex == 0)
                    || dipIdx >= size
                    || (dipIdx >= startFitIndex && dipIdx < endFitIndex)))
            {
                break;      // Found a resonance in this fit range
            } else {
                dip = null;
            }
            idx = endFitIndex;
        }
        return dip;
    }

    private CircleFit fitCircleAt(int idx) {
        /*System.err.println("ReflectionData.fitCircleAt(" + idx + ")");/*DBG*/
        if (idx >= size) {
            return null;        // Beyond end of data
        }
        CircleFit result = null;
        // First fit as many points as give good fit from here forward
        result = extendFit(idx, idx, size - 1, result);
        // Add as many previous points as possible w/o degrading fit
        result = extendFit(idx, result.getEndIndex() - 1, 0, result);
        return result;
    }

    private CircleFit extendFit(int idxStart,
                                int idxEnd,
                                int idxLimit,
                                CircleFit previousResult) {
        Messages.postDebug("CircleFit",
                           "ReflectionData.extendFit(start=" + idxStart
                           + ", end=" + idxEnd
                           + ", limit=" + idxLimit
                           + ")");
//         if (idxLimit >= idxStart && idxLimit <= idxEnd) {
//             return previousResult; // Nothing to be done
//         }
        CircleFit result = previousResult;
        // Do this if we're extending to smaller indices
        while(idxLimit < idxStart) {
            --idxStart;
            CircleFit tmpResult = fitCircle(idxStart, idxEnd);
            double[] arcRange = tmpResult.getArcExcursions();
            double arcLength = Math.abs(arcRange[0] - arcRange[1]);
            if (arcLength > Math.PI * 1.1) {
                break;      // Done
            } else if (tmpResult.muchWorseThan(result)) {
                break;      // Done
            } else {
                result = tmpResult;
            }
        }

        // Do this if we're extending to larger indices
        while (idxLimit > idxEnd) {
            idxEnd++;
            CircleFit tmpResult = fitCircle(idxStart, idxEnd);
            double[] arcRange = tmpResult.getArcExcursions();
            double arcLength = Math.abs(arcRange[0] - arcRange[1]);
            if (arcLength > Math.PI * 1.1) {
                break;      // Done
            } else if (tmpResult.muchWorseThan(result)) {
                break;      // Done
            } else {
                result = tmpResult;
            }
        }
        return result;
    }

    /**
     * Fit a circle to the data between given indices.
     * @param iStart Index of first data point to fit.
     * @param iEnd Index of last data point to fit.
     */
    private CircleFit fitCircle(int iStart, int iEnd) {
        Messages.postDebug("CircleFit",
                           "ReflectionData.fitCircle(start=" + iStart
                           + ", end=" + iEnd
                           + ")");
        final int MIN_POINTS_IN_CIRCLE_FIT = 5;
        final double FIT_TOL = 1e-3;

        int n = iEnd - iStart + 1;
        CircleFit result = new CircleFit(iStart, n);
        if (n >= MIN_POINTS_IN_CIRCLE_FIT) {
            double[] x = new double[n];
            double[] y = new double[n];
            for (int i = 0; i < n; i++) {
                x[i] = real[iStart + i];
                y[i] = imag[iStart + i];
            }

            // NB: DEBUG print fit input
            /*
            System.err.print("input points:\nx={");
            for (int i = 0; i < Math.min(6, n); i++) {
                System.err.print(", " + vnmr.util.Fmt.f(3, x[i]));
            }
            System.err.println("}");
            System.err.print("input points:\ny={");
            for (int i = 0; i < Math.min(6, n); i++) {
                System.err.print(", " + vnmr.util.Fmt.f(3, y[i]));
            }
            System.err.println("}");
            /*DBG*/


            // TODO: Implement a WeightedCircleFunc extending CircleFunc
            // that takes an array with a weight for each (x, y) point.
            // WeightedCircleFunc.evaluateBasisFunctions() would calculate
            // differences that are the distance of data point from the
            // circle times the weight of the point.
            // (Or pass in Sigma of each data point (or pseudo-sigma),
            // equivalent to weight = 1 / Sigma**2.)
            // (Or just provide a CircleFunc constructor that includes
            // weights (or sigmas)).
            // Might want the weight to be roughly the distance from the
            // dip in frequency (squared?).
            NLFit.CircleFunc func = new NLFit.CircleFunc(x, y);
            double[] a = func.getEstimatedParameters();
            /*System.err.println("Estimated params: x=" + a[0] + ", y=" + a[1]
                               + ", r=" + a[2]);/*DBG*/
            NLFit.NLFitResult fitResult = NLFit.lmdif1(func, a, FIT_TOL);
            result.setCircleFunc(func);
            result.setFitResult(fitResult);


            // NB: DEBUG print fit result
            /*
            a = fitResult.getCoeffs();
            System.err.println("x=" + a[0] + ", y=" + a[1]
                               + ", r=" + a[2]);
            System.err.println("Result code: " + fitResult.getInfo());
            System.err.println("Number of evaluations: "
                               + fitResult.getNumberEvaluations());
            System.err.println("Sigma: " + Math.sqrt(fitResult.getChisq()));
            /*DBG*/
        }
        return result;
    }

    public boolean calculateDip() {
        if (m_failed) {
            return false;       // Already tried; give up
        } else if (m_dipCalculated) {
            return true;
        }
        if (size < 5) {
            m_failed = true;
            return false;
        }

        //double[] min = getDataMinimum();
        double[] min = getDip();
        if (m_dips == null || m_dips.length < 1 || m_dips[0] == null) {
            m_failed = true;
            return false;
        }
        m_dipCalculated = true;
        dipIndex = m_dips[0].getIndex();
        dipFreq = min[0];
        dipValue = min[1];
        dipFreqSdv = min[2];
        dipValueSdv = min[3];
        // TODO: m_drDf = min[4];
        m_curvatureValue = 1;
        Dip dip = m_dips[0];
        double[] circle = dip.getCircleFit().getFitResult().getCoeffs();
        m_dipPhase  = Math.atan2(circle[1], circle[0]);

        return true;
    }

    /**
     * Find the position of the minimum value in the reflection data.
     * @return Five doubles are returned as an array.  The first
     * element is the frequency at the minimum. The
     * second element is the interpolated reflection amplitude at that
     * index. The third element is the SDV of the frequency, and the
     * fourth is the SDV of the minimum data value, and the fifth is
     *  ...
     */
    public double[] getDip() {
        final int NPOINTS = 5;  // Min number of points to fit for minimum
        double[] rtn = new double[5];
        rtn[0] = rtn[1] = rtn[2] = rtn[3] = rtn[4] = Double.NaN;
        if (size < NPOINTS) {
            return rtn;
        }

        // Find lowest data point
        double[] data = reflection2;
        int minIndex = 0;
        double minReflection = data[0];
        for (int i = 1; i < size; i++) {
            if (minReflection > data[i]) {
                minReflection = data[i];
                minIndex = i;
            }
        }

        // Fit circle to data to find x,y at minimum.
        Dip dip = findResonanceNear(minIndex);

        /*
        if (dip != null) {
            //double[] theta = dip.getCircleFit().getArcRange();
            double[] theta = dip.getArcRange();
            System.err.println("ReflectionData.getDip: df0/df1="
                               + Fmt.f(2, dip.freqVariesSlowlyAtDip())
                               + ", res=" + dip.isResonance()
                               + ", near=" + dip.isNearData()
                               + ", theta=" + Fmt.f(2, theta[0])
                               + " to " + Fmt.f(2, theta[1])
                               + ", angleCorr=" + dip.freqCorrelatedWithAngle()
                               );
        }/*DBG*/

        if (dip.isResonance()) {
            rtn[0] = dip.getFrequency();
            rtn[1] = dip.getReflection();
            rtn[2] = dip.getFrequencySdv();
            rtn[3] = dip.getReflectionSdv();
            rtn[4] = dip.getDrDf();
        } else {
            m_dips[0] = null;
        }
        /*{
            double f = startFreq + (stopFreq - startFreq) * rtn[0] / (size - 1);
            double df = (stopFreq - startFreq) * rtn[2] / (size - 1);
            double r = Math.sqrt(Math.abs(rtn[1]));
            double dr = rtn[3] / (2 * r);
            double approxDr = Math.sqrt(covar[0][0]) / (2 * r);
            ProbeTune.printlnDebugMessage(5, "getDataMinimum: x=" + f
                               + " +/- " + df
                               + ", y=" + r + " +/- " + dr
                               + ", chisq=" + fit.getChisq());
        }/*DBG*/
        return rtn; // TODO: just return the Dip
    }

    /**
     * Find the position of the minimum value in the reflection data.
     * @param sigma The
     * @return The Dip, or null.
     */
    public Dip getDip(double[] sigma) {
        final int NPOINTS = 5;  // Min number of points to fit for minimum
        if (size < NPOINTS) {
            return null;
        }

        // Find lowest data point
        // How many standard deviations the reflection could be off by:
        double noiseMargin = 5;
        double[] data = reflection2;
        int minIndex = 0;
        double minReflection = data[0];
        double noiseMargin2 = noiseMargin * noiseMargin;
        for (int i = 1; i < size; i++) {
            // The maximum this might be, given the sigma at this frequency
            double maxLevel = data[i] + sigma[i] * sigma[i] * noiseMargin2;
            if (minReflection > maxLevel) {
                minReflection = maxLevel;
                minIndex = i;
            }
        }

        // Fit circle to data to find x,y at minimum.
        Dip dip = findResonanceNear(minIndex);

        /*
        if (dip != null) {
            //double[] theta = dip.getCircleFit().getArcRange();
            double[] theta = dip.getArcRange();
            System.err.println("ReflectionData.getDip: df0/df1="
                               + Fmt.f(2, dip.freqVariesSlowlyAtDip())
                               + ", res=" + dip.isResonance()
                               + ", near=" + dip.isNearData()
                               + ", theta=" + Fmt.f(2, theta[0])
                               + " to " + Fmt.f(2, theta[1])
                               + ", angleCorr=" + dip.freqCorrelatedWithAngle()
                               );
        }/*DBG*/

        if (!dip.isResonance()) { // TODO: Too stringent condition?
            dip = null;
        }
        return dip;
    }

//    /**
//     * Get the minimum and maximum squared reflection in this data set.
//     * @return Min and max values, respectively.
//     */
//    public double[] getMinMax() {
//        double[] minmax = new double[2];
//        minmax[0] = reflection2[0];
//        minmax[1] = reflection2[0];
//        for (int i = 0; i < size; i++) {
//            if (minmax[0] > reflection2[i]) {
//                minmax[0] = reflection2[i];
//            } else if (minmax[1] < reflection2[i]) {
//                minmax[1] = reflection2[i];
//            }
//        }
//        return minmax;
//    }

    /**
     * Get the maximum absolute value of the reflection in this data set.
     * @return Maximum value (>= 0).
     */
    public double getMaxAbsval() {
        return Math.sqrt(m_maxReflection2);
    }

    /**
     * Get the mean absolute value of the reflection in this data set.
     * @return Mean value (>= 0).
     */
    public double getMeanAbsval() {
        if (Double.isNaN(m_meanAbsval)) {
            double sum = 0;
            for (int i = 0; i < size; i++) {
                sum += Math.hypot(real[i], imag[i]);
            }
            m_meanAbsval = sum / size;
        }
        return m_meanAbsval;
    }

    /**
     * Get the reflection data in string form, prefixed with a given string.
     *
     * @param prefix String to be prepended to the data string.
     * @param separator The string used to separate "lines" within
     * the data string.
     * @return The data string.
     */
    public String getStringData(String prefix, String separator) {
        StringBuffer sb = new StringBuffer();
        if (prefix != null && prefix.length() > 0) {
            sb.append(prefix).append(separator);
        }
        sb.append("acqDate ").append(m_acqTime).append(separator);
        sb.append("gain ").append(m_gain).append(separator);
        sb.append("power ").append(m_power).append(separator);
        sb.append("start ").append(startFreq).append(separator);
        sb.append("stop ").append(stopFreq).append(separator);
        sb.append("size ").append(size).append(separator);
        sb.append("setPpf 1").append(separator);
        sb.append("scaled").append(separator);
        sb.append("data").append(separator);
        for (int i = 0; i < size; i++) {
            sb.append(Fmt.g(6, real[i])).append(" ");
            sb.append(Fmt.g(6, imag[i])).append(separator);
        }
        return sb.toString();
    }

    public double getIndexAtFrequency(double freq) {
        return (freq - startFreq) * (size - 1) / (stopFreq - startFreq);
    }

    public double getFrequencyAtIndex(int idx) {
        return startFreq + idx * (stopFreq - startFreq) / (size - 1);
    }

    /**
     * Compute an average of a given group of data sets.
     * The first n data sets in the given array are averaged.
     * All input data sets must have the same size.
     * The average is the unweighted mean of the data sets.
     * @param data The array of ReflectionData's to be averaged.
     * @param n The number of data sets to average.
     * @return A new ReflectionData containing the computed average.
     * @throws Exception If there are not enough data sets or the data
     * sets are different sizes.
     */
    public static ReflectionData average(ReflectionData[] data, int n)
            throws Exception {
        int size = data[0].getSize();
        long time = data[0].getAcqTime();
        double start = data[0].getStartFreq();
        double stop = data[0].getStopFreq();
        int power = data[0].getPower();
        int gain = data[0].getGain();
        int rfchan = data[0].getRfchan();
        double[] real = new double[size];
        double[] imag = new double[size];

        for (int j = 0; j < n; j++) {
            ReflectionData reflData = data[j];
            for (int i =0; i < size; i++) {
                real[i] += reflData.real[i];
                imag[i] += reflData.imag[i];
            }
        }
        for (int i =0; i < size; i++) {
            real[i] /= n;
            imag[i] /= n;
        }
        ReflectionData rtn = new ReflectionData(real, imag, time, start, stop,
                                                power, gain, rfchan);
        return rtn;
    }

    /**
     * Make a data-only, deep copy of this ReflectonData object.
     * Dip calculations, etc. are not copied, but left uninitialized.
     * @return The copy.
     */
    public ReflectionData copy() {
        int size = getSize();
        long time = getAcqTime();
        double start = getStartFreq();
        double stop = getStopFreq();
        int power = getPower();
        int gain = getGain();
        int rfchan = getRfchan();
        double[] real = new double[size];
        double[] imag = new double[size];

        for (int i =0; i < size; i++) {
            real[i] = this.real[i];
            imag[i] = this.imag[i];
        }
        ReflectionData rtn = new ReflectionData(real, imag, time, start, stop,
                                                power, gain, rfchan);
        return rtn;
    }

    /**
     * Compute the noise in amplitude from a given group of data sets.
     * The first n data sets in the given array are analyzed,
     * and the standard deviation of each frequency point is calculated.
     * All input data sets must have the same size.
     * The standard deviation is put in the "real" part of the resulting
     * data set; the contents of the "imaginary" part is undefined.
     * @param data The array of ReflectionData's to be analyzed.
     * @param n The number of scans in the analysis.
     * @return A new ReflectionData containing the computed relative sigma.
     */
    public static ReflectionData getRelSigma(ReflectionData[] data, int n) {
        int size = data[0].getSize();
        long time = data[0].getAcqTime();
        double start = data[0].getStartFreq();
        double stop = data[0].getStopFreq();
        int power = data[0].getPower();
        int gain = data[0].getGain();
        int rfchan = data[0].getRfchan();
        double[] real = new double[size];
        double[] imag = new double[size];

        for (int i =0; i < size; i++) {
            double mean = 0;
            double[] abs = new double[n];
            for (int j = 0; j < n; j++) {
                ReflectionData rd = data[j];
                double y2 = rd.real[i] * rd.real[i] + rd.imag[i] * rd.imag[i];
                abs[j] = Math.sqrt(y2);
                mean += abs[j];
            }
            mean /= n;
            for (int j = 0; j < n; j++) {
                double diff = abs[j] - mean;
                real[i] += (diff * diff);
            }
            real[i] = Math.sqrt(real[i] / (n - 1));
            real[i] /= mean; // Make it relative
        }
        ReflectionData rtn = new ReflectionData(real, imag, time, start, stop,
                                                power, gain, rfchan);
        return rtn;
    }

    /**
     * Calculates the difference between two angles (in radians).
     * The difference returned is in the range -PI to +PI, that is,
     * the angle is as small as possible.
     * @return The difference theta1 - theta2 in radians.
     */
    protected static double angleDifference(double theta1, double theta2) {
        double rtn = theta1 - theta2;
        rtn = Math.IEEEremainder(rtn, 2 * Math.PI);
        return rtn;
    }

    /**
     * Adjusts a given angle by a multiple of 2*PI to make it close to
     * another angle.
     * @param theta1 The angle to adjust (radians).
     * @param theta2 The reference angle to get close to (radians).
     * @return The adjusted value of theta1 (radians).
     */
    protected static double wrapAngle(double theta1, double theta2) {
        while (Math.abs(theta1 - theta2) > Math.PI) {
            if (theta1 > theta2) {
                /*System.err.println("theta1: " + Fmt.f(2, theta1) + " --> "
                                   + Fmt.f(2, theta1 - 2 * Math.PI));/*DBG*/
                theta1 -= 2 * Math.PI;
            } else {
                /*System.err.println("theta1: " + Fmt.f(2, theta1) + " --> "
                               + Fmt.f(2, theta1 + 2 * Math.PI));/*DBG*/
                theta1 += 2 * Math.PI;
            }
        }
        return theta1;
    }

    /**
     * Adjusts elements of an array of angles by multiples of 2*PI to make
     * the array (more or less) monotonic in angle.
     * The array is assumed to be already roughly monotonic except for
     * discontinuities of +/- 2*PI.
     * @param angleArray The array of angles to adjust (radians).
     * @param refIndex The index of the angle that is not adjusted.
     * @param exclude A range of indices to ignore in the wrapping.
     */
    protected static void wrapAngles(double[] angleArray,
                                     int refIndex, Interval exclude) {
        if (exclude == null) {
            exclude = new Interval(-1, -1); // A null interval
        }
        int len = angleArray.length;
        for (int i = refIndex + 1; i < len; i++) {
            if (exclude.contains(i)) {
                angleArray[i] = angleArray[i - 1];
            } else {
                angleArray[i] = wrapAngle(angleArray[i], angleArray[i - 1]);
            }
        }
        for (int i = refIndex - 1; i >= 0; --i) {
            if (exclude.contains(i)) {
                angleArray[i] = angleArray[i + 1];
            } else {
                angleArray[i] = wrapAngle(angleArray[i], angleArray[i + 1]);
            }
        }
    }

    /**
     * Adjusts elements of an array of angles by multiples of 2*PI to make
     * the array (more or less) monotonic in angle.
     * The angle in the array closest to 0 will not be changed;
     * other angles are adjusted in reference to it.
     * The array is assumed to be already roughly monotonic except for
     * discontinuities of +/- 2*PI.
     * @param angles The array of angles to adjust (radians).
     * @param exclude A range of indices to ignore in the wrapping.
     */
    protected static void wrapAngles(double[] angles, Interval exclude) {
        if (exclude == null) {
            exclude = new Interval(-1, -1); // A null interval
        }
        int len = angles.length;
        if (len > 1) {
            // Find the index of the angle closest to 0
            int minIdx = 0;
            double minAngle = Double.MAX_VALUE;
            for (int i = 0; i < len; i++) {
                if (!exclude.contains(i) && minAngle > Math.abs(angles[i])) {
                    minAngle = Math.abs(angles[i]);
                    minIdx = i;
                }
            }
            // Do the wrapping
            wrapAngles(angles, minIdx, exclude);
        }
    }


    /*
     * Simple accessors
     */
    public double getStopFreq() {
        return stopFreq;
    }

    public double getStartFreq() {
        return startFreq;
    }

    public double getDeltaFreq() {
        return (stopFreq - startFreq) / (size - 1);
    }

    /**
     * Get the current probeDelay.
     * @return The probeDelay (ns).
     */
    public double getProbeDelay() {
        return m_probeDelayCorrection_ns;
    }

    public int getPower() {
        return m_power;
    }

    public int getGain() {
        return m_gain;
    }

    public int getSize() {
        return size;
    }

    public long getAcqTime() {
        return m_acqTime;
    }

    public void setMax(double value) {
        m_maxReflection2 = value;
    }


    /**
     * The CircleFit class holds the "CircleFunc" necessary for fitting a
     * circle to data, and the parameters of the resulting fit.
     * The data is held in the parent ReflectionData object, and this
     * object points to the subset of the ReflectionData in the circle.
     * That subset of the data is also copied into the CircleFunc
     * contained by this object.
     */
    public class CircleFit {
        private NLFit.CircleFunc mm_circleFunc;
        private NLFit.NLFitResult mm_fitResult;
        private int mm_indexOffset;
        private int mm_nDataPoints = 0;

        /**
         * An ordered array giving the angle (in radians) from the center
         * of the fit circle to each data point. The angles are "wrapped" so
         * that there are no discontinuities of 2*PI in the array.
         * Thus, the difference between the angles of any two data points
         * is not (necessarily) the shortest arc between the two points,
         * but an arc that goes by the intermediate points.
         */
        private double[] mm_theta = null;

        CircleFit(int indexOffset, int size) {
            setIndexOffset(indexOffset);
            mm_nDataPoints = size;
            mm_circleFunc = null;
            mm_fitResult = null;
        }

        public boolean muchWorseThan(CircleFit otherFit) {
            boolean rtn = false;
            if (mm_fitResult == null
                || otherFit == null
                || otherFit.getFitResult() == null)
            {
                return false;   // No comparison!
            }
            double relativeChisq = (mm_fitResult.getChisq()
                                    / otherFit.getFitResult().getChisq());
            rtn = (relativeChisq > 10);
            /*
            if (rtn == true) {
                System.err.println("muchWorseThan: relativeChisq="
                                   + relativeChisq);
            }/*DBG*/
            return rtn;
        }

        /**
         * Get the index of the first point relative to the original data array.
         * This is just the indexOffset passed to the constructor.
         * @return The index of the first data point.
         */
        public int getStartIndex() {
            return getIndexOffset();
        }

        /**
         * The index is relative to the original data array.
         * @return One more than the index of the last data point.
         */
        public int getEndIndex() {
            int rtn = getIndexOffset();
            if (mm_nDataPoints > 0) {
                rtn += mm_nDataPoints;
            } else if (mm_circleFunc != null) {
                rtn += mm_circleFunc.getYLength();
            }
            return rtn;
        }

        /**
         * Returns the absolute angle to the origin from the center of
         * the circle. Will be adjusted by +/- 2*PI*n to make the angle
         * agree with the range of angles in the fit; that is, it will
         * be within the range of the fit arc, or as close to it as
         * possible.
         * @return Direction in radians.
         */
        public double getDirectionToOrigin() {
            final double PI2 = Math.PI * 2;
            double[] circle = getFitResult().getCoeffs();
            double direction = Math.atan2(-circle[1], -circle[0]);

            // Now adjust by 2*PI*n
            double startArc = getDirectionToArcStart();
            double endArc = getDirectionToArcEnd();
            double minArc = Math.min(startArc, endArc);
            double maxArc = Math.max(startArc, endArc);
            double d1 = direction;
            double d2 = direction;
            double diff1 = 0;
            double diff2 = 0;

            while (direction < minArc) {
                direction += PI2;
            }
            // direction is now in the arc range or above it

            while (direction > maxArc) {
                direction -= PI2;
            }
            // direction is now in the arc range or just below it
            if (direction < minArc) {
                d1 = direction;
                diff1 = Math.abs(angleDifference(minArc, direction));
            }

            while (direction < minArc) {
                direction += PI2;
            }
            // direction is now in the arc range or just above it
            if (direction > maxArc) {
                d2 = direction;
                diff2 = Math.abs(angleDifference(maxArc, direction));
            }

            if (Math.abs(diff1) < Math.abs(diff2)) {
                direction = d1;
            } else {
                direction = d2;
            }
            return direction;
        }

        /**
         * Returns the absolute angle to the start of the fitted data
         * relative to the center of the circle.
         * @return Direction in radians.
         */
        public double getDirectionToArcStart() {
            double[] theta = getAngleArray();
            return theta[0];
        }

        /**
         * Returns the absolute angle to the end of the fitted data
         * relative to the center of the circle.
         * @return Direction in radians.
         */
        public double getDirectionToArcEnd() {
            double[] theta = getAngleArray();
            return theta[theta.length - 1];
        }

        /**
         * Gets the directions to the two ends of the fitted arc, relative
         * to the direction to the origin, in radians.
         * @return Array of two doubles, giving the angles to the two arc ends.
         */
        public double[] getArcExcursions() {
            double pi2 = Math.PI * 2;
            double[] rtn = new double[2];
            double theta0 = getDirectionToOrigin();
            rtn[0] = getDirectionToArcStart() - theta0;
            rtn[0] = Math.IEEEremainder(rtn[0], pi2); // TODO: Best we can do?
            rtn[1] = getDirectionToArcEnd() - theta0;
            rtn[1] = Math.IEEEremainder(rtn[1], pi2);

            // Debug stuff:
            double[] circ = getFitResult().getCoeffs();
            double radius = circ[2];
            double dist = Math.hypot(circ[0], circ[1]);
            Messages.postDebug("DipFind",
                               "center at: (r=" + Fmt.f(2, dist)
                               + ", theta=" + Fmt.f(2, theta0)
                               + "), radius=" + Fmt.f(2, radius)
                               + ", arc start: " + Fmt.f(2, getDirectionToArcStart())
                               + ", arc end: " + Fmt.f(2, getDirectionToArcEnd()));
            // End debug stuff

            return rtn;
        }

        /**
         * Returns the maximum angle between data and the dip, as measured from
         * the center of the fit circle.
         *
         * @return The (signed) angle (in radians) that is largest in absolute
         *         value.
         */
        public double getMaxArcExcursion() {
            double[] x = getArcExcursions();
            if (Math.abs(x[0]) >= Math.abs(x[1])) {
                return x[0];
            } else {
                return x[1];
            }
        }

        public double[] getAngleArray() {
            if (mm_theta == null) {
                double[] circle = getFitResult().getCoeffs();
                double[][] data = getCircleFunc().getDataCoordinates();
                int n = data.length;
                mm_theta = new double[n];
                for (int i = 0; i < n; i++) {
                    mm_theta[i] = Math.atan2(data[i][1] - circle[1],
                                             data[i][0] - circle[0]);
                }
                wrapAngles(mm_theta, null); // Eliminates any 2*PI discontinuities

            }
            return mm_theta;
        }

        /**
         * Calculates the absolute value of the length of the arc
         * that is used to obtain this circle fit.
         * @return Arc length in radians.
         */
        public double getArcLength() {
            double[] theta = getAngleArray();
            int n = theta.length;
            return Math.abs(theta[0] - theta[n - 1]);
        }

        public NLFit.CircleFunc getCircleFunc() {
            return mm_circleFunc;
        }

        public void setCircleFunc(NLFit.CircleFunc circleFunc) {
            mm_circleFunc = circleFunc;
        }

        public NLFit.NLFitResult getFitResult() {
            return mm_fitResult;
        }

        public void setFitResult(NLFit.NLFitResult fitResult) {
            mm_fitResult = fitResult;
        }

        public int getIndexOffset() {
            return mm_indexOffset;
        }

        public void setIndexOffset(int indexOffset) {
            mm_indexOffset = indexOffset;
        }

    }


    /**
     * This class holds information about a dip (possible resonance) in
     * the data. It is based on a CircleFit to the data at this location and
     * also contains fits of the frequency vs angle in the circle.
     * <p>
     * Various criteria are used by the <code>isResonance</code> method
     * to decide whether a dip is a real resonance.
     * To see why a dip is being rejected as a real resonance, turn on the
     * "DipFind" debug dategory.
     * <p>
     * Except as noted below, all criteria must be met for a dip to be
     * considered a resonance.
     * The default criteria can be changed by defining System properties:
     * <ul>
     * <li> apt.dataMargin -- How far the dip center must be from either end of
     * the reflection data range. If negative, the dip can be outside the
     * range of the data. If this is defined, it takes precedence over
     * "dataMarginPct", below.
     * Units: number of points.
     * Default: not defined -- use dataMarginPct.
     *
     * <li> apt.dataMarginPct -- The data margin (above) expressed as a
     * percentage of the number of points in the CircleFit for the dip.
     * Units: percent.
     * Default: -50%. (I.e., the dip may be a little beyond the data.)
     *
     * <li> apt.checkFitRange -- Whether the dip center must be within the
     * range of points in the CircleFit. Or, if beyond the range of data,
     * the CircleFit is at the end of the data range.
     * Units: true or false.
     * Default: true.
     *
     * <li> apt.minCircleSizeS2N -- The minimum radius of the CircleFit.
     * Units: baseline noise (sigma) in the reflection amplitude.
     * Default: 3.0 (i.e., radius at least 3 times the baseline noise).
     *
     * <li> apt.minCircleSize -- The minimum radius of the CircleFit.
     * If defined, this takes precedence over minCircleSizeS2N.
     * Units: reflection amplitude.
     * Default: not defined -- use minCircleSizeS2N.
     *
     * <li> apt.maxCircleCenterR -- The maximum distance of the center of the
     * CircleFit from the origin.
     * Units: reflection amplitude.
     * Default: 1.0.
     *
     * <li> apt.maxCircleExtentR -- The maximum distance any part of the
     * circle defined by the CircleFit can be from the origin.
     * Units: reflection amplitude.
     * Default: 2.0.
     *
     * <li> apt.minFreqsMonotonicPct -- The frequencies of the points in
     * the CircleFit are checked to see if they are monotonic with angle.
     * This specifies the minimum percentage of points that must go in
     * the same direction.
     * The dip need pass only one of the tests minFreqsMonotonicPct,
     * maxFreqsNonMonotonic, and minFreqChisqReduction.
     * Units: percent.
     * Default: 90%.
     *
     * <li> apt.maxFreqsNonMonotonic -- See minFreqsMonotonicPct, above.
     * This specifies the maximum number of points that can go in
     * the "wrong" direction.
     * The dip need pass only one of the tests minFreqsMonotonicPct,
     * maxFreqsNonMonotonic, and minFreqChisqReduction.
     * Units: number of points.
     * Default: 2.
     *
     * <li> apt.minFreqChisqReduction -- The minimum factor by which the
     * fit of frequency to angle must reduce the frequency chi-square.
     * The dip need pass only one of the tests minFreqsMonotonicPct,
     * maxFreqsNonMonotonic, and minFreqChisqReduction.
     * Units: dimensionless ratio.
     * Default: 4.0.
     *
     * <li> apt.minCircleFitSignalToNoise -- The minimum quality of the fit
     * of the CircleFit to the reflection data. This is the ratio of the
     * circle radius to the standard deviation of the distance of the
     * points from the circle.
     * Units: dimensionless ratio.
     * Default: 20.0.
     */
    public class Dip {
        /** The interpolated/extrapolated data index of the dip minimum. */
        private double mm_dipIndex;

        /** The standard deviation of the amplitude at the dip minimum. */
        private double mm_dipSdv;

        /** The fit of a circle to the data for this dip. */
        private CircleFit mm_circleFit;

        /**
         * The angles of the data points from the center of the fit circle.
         * The angle is in radians, with 0 being the direction
         * from the center of the circle to the origin (the direction
         * of the dip minimum).
         */
        private double[] mm_theta;

        /**
         * Re-parameterization of theta: zeta = tan(theta/2).
         * Note that abs(theta) must be < PI for this to make sense.
         * Frequency varies linearly with zeta.
         */
        private double[] mm_zeta;

        /** The frequencies of the data points. */
        private double[] mm_freq;

        /** The pseudo std deviation of the data points: 1/weight. */
        private double[] mm_sigma;

        /** A linear fit to the frequency vs. zeta data. */
        private LinFit.SvdFitResult mm_linearFreqFit = null;

        /**
         * Flag indicating whether this dip is a resonance or not.
         * Null until the calculation has been done.
         */
        private Boolean mm_isResonance = null;


        /**
         * The frequency near the dip minimum (Hz) as a function of
         * angle from the minimum (in radians) as seen from the center
         * of the fit circle.
         * Expect this to be normally a cubic fit.
         * An array of polynomial coefficients.  The first coefficient,
         * mm_dipFreq[0], is the frequency at the dip minimum.
         */
        private double[] mm_dipFreq;

        /** The standard deviation of the frequency at the dip minimum (Hz). */
        private double mm_dipFreqSdv;


        /**
         * Construct a Dip from a given CircleFit to some data.
         * @param circleFit The CircleFit to use.
         */
        public Dip(CircleFit circleFit) {
            mm_circleFit = circleFit;
            mm_dipSdv = Math.sqrt(circleFit.getFitResult().getChisq());
            mm_dipFreq = fitFreqsToAngle(mm_circleFit);
            mm_dipIndex = getIndexAtFrequency(mm_dipFreq[0]);
        }

        public boolean isFreqInRangeOfDipData(double freq_hz) {
            int n1 = mm_freq.length - 1;
            double fmin = Math.min(mm_freq[0], mm_freq[n1]);
            double fmax = Math.max(mm_freq[0], mm_freq[n1]);
            double step = (fmax - fmin) / n1;
            return freq_hz > fmin - step && freq_hz < fmax + step;
        }

        /**
         * Use the given CircleFit to data and the known frequencies
         * of the data to get a linear fit of frequency vs zeta in
         * the circle. (Zeta = tan(theta/2).)
         * This is a linear function, unlike frequency vs. theta.
         * Also initializes the arrays theta, zeta, freq, and sigma -- the
         * data that is the basis for the fit.
         * The angle is in radians, with 0 being the direction
         * from the center of the circle to the origin (the direction
         * of the dip minimum).
         * @param circleFit An object containing a previously fitted circle.
         * @return The coefficients of the linear fit of frequency to zeta.
         * (The first element is the frequency at the dip minimum,
         *  the second is the variation: d freq / d zeta.)
         */
        public double[] fitFreqsToAngle(CircleFit circleFit) {
            // NB: circle[0] = Xc, circle[1] = Yc, circle[2] = r
            double[] circle = circleFit.getFitResult().getCoeffs();
            // Direction from center of circle to origin:
            double theta0 = Math.atan2(-circle[1], -circle[0]);

            double[][] data = circleFit.getCircleFunc().getDataCoordinates();
            int n = data.length;
            mm_theta = new double[n];
            mm_zeta = new double[n];
            mm_freq = new double[n];
            mm_sigma = new double[n];
            int idxOffset = circleFit.getStartIndex();
            // Track which point is closest to origin direction
            int minIdx = 0;
            mm_theta[0] = Math.atan2(data[0][1] - circle[1],
                                    data[0][0] - circle[0]);
            mm_theta[0] = angleDifference(mm_theta[0], theta0);
            double minAngle = Math.abs(mm_theta[0]);
            mm_freq[0] = getFrequencyAtIndex(idxOffset);
            for (int i = 1; i < n; i++) {
                // For each point:
                // Get angle between point and origin
                mm_theta[i] = Math.atan2(data[i][1] - circle[1],
                                        data[i][0] - circle[0]);
                mm_theta[i] = angleDifference(mm_theta[i], theta0);
                if (minAngle > Math.abs(mm_theta[i])) {
                    minAngle = Math.abs(mm_theta[i]);
                    minIdx = i;
                }
                // Get frequency of point
                mm_freq[i] = getFrequencyAtIndex(idxOffset + i);
            }
            wrapAngles(mm_theta, minIdx, null);

            // Give more weight to points near the dip
            double sumwt = 0;
            for (int i = 0; i < n; i++) {
                mm_zeta[i] = Math.tan(mm_theta[i] / 2);
                // NB: wt function and min value are optimized empirically;
                // "mm_sigma" has nothing to do with the data quality,
                // but gives greater weight to points near the dip center.
                double wt = mm_zeta[i] * mm_zeta[i];
                double min = 0.25;
                mm_sigma[i] = Math.max(Math.abs(wt), min);
                sumwt += 1 / (mm_sigma[i] * mm_sigma[i]);
            }
            for (int i = 0; i < n; i++) {
                // Normalize weights to sum to the number of points
                mm_sigma[i] *= (sumwt / n);
            }

            if (DebugOutput.isSetFor("AngleFitData")) {
                String path = ProbeTune.getVnmrSystemDir() + "/tmp/angleFit";
                new File(path).delete();
                String header = "# theta  frequency";
                for (int i = 0; i < n; i++) {
                    String buffer = mm_theta[i] + "  " + mm_freq[i];
                    TuneUtilities.appendLog(path, buffer, header);
                }
                path = ProbeTune.getVnmrSystemDir() + "/tmp/tanAngleFit";
                new File(path).delete();
                header = "# tan(theta/2)  frequency";
                //String header = "# theta  frequency";
                for (int i = 0; i < n; i++) {
                    String buffer = mm_zeta[i] + "  " + mm_freq[i];
                    //String buffer = mm_theta[i] + "  " + mm_freq[i];
                    TuneUtilities.appendLog(path, buffer, header);
                }
            }

            //for (int i = 0; i < m_theta.length; i++) {
            //    System.err.print(Fmt.f(2, m_theta[i]) + " ");
            //} System.err.println(" END");/*DBG*/

            LinFit.SvdFitResult freqFit = getLinearFreqFit();
            double[] freqCoeffs = freqFit.getCoeffs();

            mm_dipFreqSdv = Math.sqrt(freqFit.getChisq() / (n - 2));

            return freqCoeffs;
        }

        /**
         * Gets the parameters of the circle that fits the data near
         * this dip when the reflection is plotted in the complex
         * plane.
         * @return An array of 5 doubles for
         * (1) X coordinate of center (reflection units),
         * (2) Y coordinate of center,
         * (3) raduis,
         * (4) angle to start of fitted arc (radians),
         * (5) length of fitted arc (radians).
         */
        public double[] getCircleArc() {
            double[] circleParams = getCircleFit().getFitResult().getCoeffs();
            double theta0 = Math.atan2(-circleParams[1], -circleParams[0]);
            double[] fitArcRange = getArcRange();
            double[] circleArc = new double[5];
            circleArc[0] = circleParams[0];
            circleArc[1] = circleParams[1];
            circleArc[2] = circleParams[2];
            circleArc[3] = fitArcRange[0] + theta0;
            circleArc[4] = fitArcRange[1] - fitArcRange[0];
            return circleArc;
        }

        /**
         * Get the complex reflection at the given frequency.
         * If the given frequency is within the range fit by this dip,
         * returns the reflection implied by the fit.
         * @param freq The given frequency in Hz.
         * @return An array of two doubles giving the real and imaginary
         * components of the reflection, respectively.
         */
        public double[] getReflectionAt(double freq) {
            double[] reflect = null;

            // arcRange defines the "range" of this dip
            double[] arcRange = {-3 * Math.PI / 2, 3 * Math.PI / 2};
            double theta = Double.NaN;
            if (isFreqInRangeOfDipData(freq)) {
                double[] freqFit = getLinearFreqFit().getCoeffs();
                double zeta = TuneUtilities.invPoly(freq, freqFit, arcRange);
                theta = 2 * Math.atan(zeta);
            } else {
                if (freq < startFreq || freq > stopFreq) {
                    // Outside range of ReflectionData!
                    // Use last 2 points to get d(freq)/d(theta)
                    int n = mm_theta.length;
                    boolean isFrontEnd = (Math.abs(mm_theta[0])
                                          < Math.abs(mm_theta[n-1]));

                    double deltaFreq = isFrontEnd
                            ? freq - mm_freq[0]
                            : freq - mm_freq[n - 1];
                    double deltaStep = deltaFreq / (mm_freq[1] - mm_freq[0]);
                    if (Math.abs(deltaStep) < 2) {
                        double dthetaDstep = isFrontEnd
                                ? mm_theta[1] - mm_theta[0]
                                : mm_theta[n - 1] - mm_theta[n - 2];
                        theta = isFrontEnd
                                ? mm_theta[0] + deltaStep * dthetaDstep
                                : mm_theta[n - 1] + deltaStep * dthetaDstep;
                    }
                }
            }
            if (!Double.isNaN(theta)) {
                double[] circle = getCircleFit().getFitResult().getCoeffs();
                double theta0 = Math.atan2(-circle[1], -circle[0]);
                theta += theta0;
                reflect = new double[2];
                reflect[0] = circle[0] + circle[2] * Math.cos(theta);
                reflect[1] = circle[1] + circle[2] * Math.sin(theta);
            } else {
                // This freq is outside the range of this dip, but within
                // the range of the reflection data.
                reflect = getComplexReflectionAtFreq(freq);
            }
            return reflect;
        }

        /**
         * Tries to figure out if this dip is a real resonance.
         * Checks that there is a strong correlation of frequency vs angle
         * (not just looking at noise).
         * If the cubic term in the fit is significant, it should be such
         * that the frequency varies slowly with angle at the dip.
         * @return True if this Dip seems to be a valid resonance.
         */
        synchronized public boolean isResonance() {
            // FIXME: This is currently one value to specify 2 different items:
            // 1) Is this definitely a real resonance?
            // 2) Can we get a well-defined dip location from it?
            // Should first check for #1, if it's OK then check for #2.
            // #2 might be modified to just be the standard deviation
            // of the dip position measurement.
            //

            if (mm_isResonance != null) {
                return mm_isResonance;
            }

            boolean ok = true; // The return value
            String reason = "none";

            // Index of first dip datum relative to full ReflectionData:
            int idxDipStart = getStartIndex();
            int idxDipEndPlus1 = getEndIndex(); // One after last datum
            int nDipPoints = idxDipEndPlus1 - idxDipStart;
            double idxDip = getIndex(); // Location of dip minimum on full data
            int dataSize = getSize();

            // How far _inside_ the data range the dip needs to be.
            // By default, the dip can be _outside_ the data range.
            // (I.e., dataMargin is negative.)
            int dataMargin = -1;
            if (m_dataMarginPct != null) {
                dataMargin = (nDipPoints * m_dataMarginPct) / 100;
            }
            if (m_dataMargin != null) {
                dataMargin = m_dataMargin;
            }
            if (ok && idxDip <= dataMargin) {
                // Dip is too far before beginning of data
                String idx = Fmt.f(6, idxDip);
                reason = "Dip is too far left (idx=" + idx + ")"
                        + ", nFitPoints=" + nDipPoints;
                dipCheckDebug(reason);
                ok = false;
            }
            if (ok && idxDip >= dataSize - dataMargin - 1) {
                // TODO: This criterion may be too crude
                // dip is too far off end of data
                String idx = Fmt.f(6, idxDip - dataSize + 1);
                reason = "Dip is too far right (idx=end+" + idx + ")"
                              + ", nFitPoints=" + nDipPoints;
                dipCheckDebug(reason);
                ok = false;
            }

            // Don't let the dip be outside the fit data
            if (m_checkFitRange) {
                if (ok && idxDip <= 0 && idxDipStart > 0) {
                    // Dip before start of data, but dip fit not at beginning
                    reason = "Dip too far left of fit region";
                    dipCheckDebug(reason);
                    ok = false;
                }
                if (ok && idxDip >= dataSize - 1 && idxDipEndPlus1 < dataSize) {
                    // Dip after end of data, but dip fit not at end
                    reason = "Dip too far right of fit region";
                    dipCheckDebug(reason);
                    ok = false;
                }
                if (ok && idxDip > 0 && idxDip < dataSize - 1) {
                    // Dip minimum is within ReflectionData range,
                    // so it should also be within dip data range.
                    if (idxDip < idxDipStart || idxDip >= idxDipEndPlus1) {
                        reason = "Min of fit outside dip data range";
                        dipCheckDebug(reason);
                        ok = false;
                    }
                    if (ok && !isDipInsideFitRange()) {
                        // Dip is within data range, but not within fit range
                        reason = "Dip min is not within fit arc";
                        dipCheckDebug(reason);
                        ok = false;
                    }
                }
            }

            double[] circ = getCircleFit().getFitResult().getCoeffs();
            // Radial coord of center of fit circle:
            double r0 = Math.hypot(circ[0], circ[1]);
            double r = circ[2]; // Radius of fit circle

            double minCircleSize;
            if (m_minCircleSize != null) {
                minCircleSize = m_minCircleSize;
            } else {
                minCircleSize = getBaselineNoise() * m_minCircleSizeS2N;
            }
            if (ok && r < minCircleSize) {
                // Circle is not much larger than the noise
                reason = "Dip fit circle too small for the noise";
                dipCheckDebug(reason);
                ok = false;
                Messages.postDebug("CircleSize", reason);
            }

            if (ok && r0 > m_maxCircleR) {
                // Center of fit circle is outside the unit circle
                reason = "Dip fit centered outside unit circle";
                dipCheckDebug(reason);
                ok = false;
            }

            if (ok && r0 + r > m_maxCircleBulge) {
                // Fit circle bulges too far outside the unit circle
                reason = "Dip fit goes far outside unit circle";
                dipCheckDebug(reason);
                ok = false;
            }

            double absReflection = Math.abs(getReflection());
            // NB: Doesn't the absReflection effectively have to be > 0?
            if (ok && !freqCorrelatedWithAngle() && absReflection > 0) {
                reason = "Dip fit frequency uncorrelated with angle";
                dipCheckDebug(reason);
                ok = false;
            }
            if (ok) {
                double sdv = 0;
                for (int i = idxDipStart; i < idxDipEndPlus1; i++) {
                    double diff = Math.hypot(real[i] - circ[0],
                                             imag[i] - circ[1]) - r;
                    sdv += diff * diff;
                }
                double sumsq = sdv * nDipPoints / (nDipPoints - 3.0);
                sdv = Math.sqrt(sdv / (nDipPoints - 3));
                double chisq = getCircleFit().getFitResult().getChisq();
                Messages.postDebug("CircleSdv", "r=" + Fmt.g(3, r)
                                   + ", sdv=" + Fmt.g(3, sdv)
                                   + ", sumsq=" + Fmt.g(3, sumsq)
                                   + ", chisq=" + Fmt.g(3, chisq)
                                   + ", r/sdv=" + Fmt.f(3, r / sdv)
                                   + ", n=" + nDipPoints
                                   );
                double minS2N; // Multiply base S2N limit by reflection at dip
                minS2N = Math.max(2, m_minCircleS2N * Math.abs(r0 - r));
                if (r / sdv < minS2N) {
                    reason = "Ratio of fit circle radius to Std Dev of fit  "
                            + minS2N + " (" + Fmt.f(3, r / sdv) + ")";
                    dipCheckDebug(reason);
                    ok = false;
                }
            }

            if (ok) {
                Messages.postDebug("IsResonance", "isResonance=true");
            } else {
                Messages.postDebug("IsntResonance",
                                   "isResonance=false: " + reason);
            }
            // Cache the result
            mm_isResonance = ok;
            dipCheckDebug("isResonance=" + mm_isResonance);
            return ok;
        }

        private boolean isDipInsideFitRange() {
            double[] arcRange = getArcRange(); // End angles relative to origin
            return arcRange[0] * arcRange[1] <= 0;
        }

        /**
         * @return True if the frequency is highly correlated with angle
         * in the circle fit.
         */
        public boolean freqCorrelatedWithAngle() {
            boolean ok = false;
            int n = Math.min(mm_freq.length, mm_theta.length);

            // If freq vs angle is (nearly) monotonic, it's OK
            int nPositive = 0;
            int nNegative = 0;
            for (int i = 0; i < n - 1; i++) {
                double ratio = ((mm_freq[i] - mm_freq[i+1])
                                / (mm_theta[i] - mm_theta[i+1]));
                if (ratio > 0) {
                    nPositive++;
                } else if (ratio < 0) {
                    nNegative++;
                }
            }
            double nTotal = nPositive + nNegative;
            int nFwd = Math.max(nPositive, nNegative);
            int nBack = Math.min(nPositive, nNegative);
            if (nFwd / nTotal > m_minFwdPct / 100.0 || nBack <= m_maxBack) {
                ok = true;
            } else {
                dipCheckDebug("Freq vs angle not monotonic enough "
                              + "(nFwd=" + nFwd + ", nBack=" + nBack + ")");
            }

            // It's also OK if chisq reduction is big enough
            if (!ok && mm_circleFit != null) {
                // Get chisq of freq relative to mean freq
                double freqMean = 0;
                for (int i = 0; i < n; i++) {
                    freqMean += mm_freq[i];
                }
                freqMean /= n;
                double chisq0 = 0;
                for (int i = 0; i < n; i++) {
                    double df = mm_freq[i] - freqMean;
                    chisq0 += df * df;
                }
                double resid0 = Math.sqrt(chisq0 / (n - 1));

                // Get linear fit to data
                LinFit.SvdFitResult fit = getLinearFreqFit();
                //double[] a = fit.getCoeffs();
                //double[][] covar = fit.getCovariance();
                //double slopeSdv = Math.sqrt(covar[1][1]);
                double resid1 = Math.sqrt(fit.getChisq() / (n - 2));
                /*System.err.println("freqCorrelatedWithAngle:"
                                   //+ " fit slope=" + Fmt.f(0, a[1])
                                   //+ " +/- " + Fmt.f(2, slopeSdv)
                                   + ", resid0=" + Fmt.f(0, resid0)
                                   + ", resid1=" + Fmt.f(0, resid1)
                                   + ", n=" + n);/*DBG*/
                //rtn = (Math.abs(a[1]) / covar[1][1] > 100); // Super fit
                double chisqReduc = resid0 / resid1;
                if (chisqReduc >= m_minFreqFitChisqReduce) {
                    ok = true;
                    dipCheckDebug("... but chisq reduction is OK"
                                  + " (" + Fmt.f(1, chisqReduc) + ")");
                } else {
                    dipCheckDebug("... and chisq reduction is too small"
                                  + " (" + Fmt.f(1, chisqReduc) + ")");
                }
            }
            return ok;
        }

        private void dipCheckDebug(String string) {
            Messages.postDebug("DipFind", "DipCheck: " + string);
        }

        /**
         * Get a linear fit of frequency vs angle in the circle.
         * Assumes that the arrays zeta, freq, and sigma have already been
         * initialized.
         * @return The LinFit.SvdFitResult, from which the polynomial
         * coefficients of the fit can be extracted.
         */
        private LinFit.SvdFitResult getLinearFreqFit() {
            if (mm_linearFreqFit == null) {

                /*for (int i = 0; i < m_theta.length; i++) {
                    System.err.print(Fmt.f(2, m_theta[i]) + " ");
                } System.err.println(" LIN");/*DBG*/

                LinFit.PolyBasisFunctions poly1;
                poly1 = new LinFit.PolyBasisFunctions(1);
                mm_linearFreqFit = LinFit.svdFit(mm_zeta, mm_freq,
                                                 mm_sigma, poly1);
            }
            return mm_linearFreqFit;
        }

        /**
         * @return The reflection amplitude at the dip.
         */
        public double getReflection() {
            double rtn = 0;
            if (mm_circleFit != null) {
                double[] a = mm_circleFit.getFitResult().getCoeffs();
                rtn = Math.hypot(a[0], a[1]) - a[2];
            }
            return rtn;
        }

        /**
         * @return The SDV in the reflection amplitude at the dip.
         */
        public double getReflectionSdv() {
            return mm_dipSdv;
        }

        /**
         * Returns the interpolated or extrapolated index of the
         * dip position relative to the original data array.
         * @return The dip index.
         */
        public double getIndex() {
            return mm_dipIndex;           // TODO: Calculate dip index
        }

        /**
         * Returns the interpolated or extrapolated frequency of the dip.
         * @return The dip frequency (Hz).
         */
        public double getFrequency() {
            return mm_dipFreq[0];
        }

        /**
         * Returns the standard deviation in the frequency of the dip.
         * @return The dip frequency SDV (Hz).
         */
        public double getFrequencySdv() {

            return mm_dipFreqSdv;
        }

        /**
         * Returns the approximate slope of the reflection vs. frequency
         * at the dip. This assumes that the reflection at the dip is
         * small compared to the radius of the fit circle.
         * @return The slope (fractionReflected / Hz).
         */
        public double getDrDf() {
            // NB: at x=0, d tan(x) / dx = 1
            double dFdTheta = mm_dipFreq[1]; // dF/dTheta
            // dReflection / dTheta (if circle passed through origin) = radius
            double dRdTheta = mm_circleFit.getFitResult().getCoeffs()[2];
            return dRdTheta / dFdTheta;
        }

        /**
         * The index is relative to the original data array.
         * @return The index of the first data point.
         */
        public int getStartIndex() {
            return mm_circleFit == null ? 0 : mm_circleFit.getStartIndex();
        }

        /**
         * The index is relative to the original data array.
         * @return One more than the index of the last data point.
         */
        public int getEndIndex() {
            return mm_circleFit == null ? 0 : mm_circleFit.getEndIndex();
        }

        public CircleFit getCircleFit() {
            return mm_circleFit;
        }

        /**
         * @return The range of data points fit by this dip.
         */
        public Interval getInterval() {
            int start = getStartIndex();
            int stop = Math.max(0, getEndIndex() - 1);
            return new Interval(start, stop);
        }

        public double[] getArcRange() {
            double[] rtn = new double[2];
            if (mm_theta != null) {
                rtn[0] = mm_theta[0];
                rtn[1] = mm_theta[mm_theta.length - 1];
            }
            return rtn;
        }

        /**
         * Get the amount that this dip affects the phase of data far from
         * the dip.
         * @param idx The index of a data point in ReflectionData.
         * @return The amount this dip perturbs the phase of that point.
         */
        public double getDeltaPhi(int idx) {
            // mm_dipFreq[0] = freq of center of dip
            // mm_dipFreq[1] = dfreq / dzeta (zeta = tan(theta/2)
            double df = getFrequencyAtIndex(idx) - mm_dipFreq[0];
            double theta = 2 * Math.atan(df / mm_dipFreq[1]);
            double r = getCircleFit().getFitResult().getCoeffs()[2];
            double phi = theta > 0
                    ? -(Math.PI - theta) * r : (Math.PI + theta) * r;
            return phi;
        }

        /**
         * Get an estimate of the dip width as a number of data points.
         * This approximates the FWHM of the dip.
         * @return The dip width (number of data points).
         */
        public int getDipWidth() {
            // mm_dipFreq[1] = dfreq / dzeta (zeta = tan(theta/2)
            // at theta = PI/2, zeta=1 ==> dfreq = mm_dipFreq[1]
            return 2 * (int)Math.abs(mm_dipFreq[1] / getDeltaFreq());
        }

        /**
         * Get the interval within the reflection data that this dip
         * perturbs more than can be reliably corrected by the
         * getDeltaPhi method.
         * @return The Interval perturbed by the dip.
         */
        public Interval getPerturbedInterval() {
            final int WING_WIDTH = 3; // See usage below
            int dipIdx = (int)Math.round(getIndex());
            int dipWidth = getDipWidth();
            int idx0 = Math.max(0, dipIdx - dipWidth * WING_WIDTH);
            int idx1 = Math.min(size - 1, dipIdx + dipWidth * WING_WIDTH);
            return new Interval(idx0, idx1);
        }
    }


    public int getRfchan() {
        return m_rfchan;
    }

    /**
     * Get the distance between data points in the complex reflection plane
     * near the given frequency.
     * @param freq The given frequency.
     * @return The distance in reflection units (always positive).
     */
    public double getDRefl_DIdx(double freq) {
        double dblIdx = getIndexAtFrequency(freq);
        int i0 = (int)Math.floor(dblIdx);
        i0 = Math.max(i0, 0);
        i0 = Math.min(i0, size - 2);
        int i1 = i0 + 1;
        double deltaX = real[i1] - real[i0];
        double deltaY = imag[i1] - imag[i0];
        return Math.hypot(deltaX, deltaY);
    }


}
