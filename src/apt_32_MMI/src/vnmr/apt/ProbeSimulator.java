/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.StringTokenizer;

import vnmr.apt.ChannelInfo.Limits;
import vnmr.apt.Mtune.CoilParams;
import vnmr.util.Complex;
import vnmr.util.NLFit;
import vnmr.util.LinFit.PolyBasisFunctions;
import vnmr.util.LinFit.SvdFitResult;

import static java.lang.Double.NaN;

//import static vnmr.apt.ChannelInfo.Limits;
import static vnmr.apt.ChannelInfo.SwitchPosition;
import static vnmr.apt.ChannelInfo.TunePosition;


/**
 * This class simulates tuning data for a probe defined
 * by its ProTune persistence files.
 */
class ProbeSimulator {
    /** The resonances to be simulated. */
    Collection<RefResonance> m_dips = new ArrayList<RefResonance>();

    /** The legal limits of travel of the motors. */
    private Limits[] m_motorLimits;


    /**
     * Creates an object that simulates tuning data for a probe defined
     * by the persistence files for the specified probe name.
     * @param probe The name of the probe to use.
     * @param sysdir The Vnmr system directory, maybe "/vnmr".
     * @param chanName If non-null this is the name of the one
     * tune channel to use.
     */
    public ProbeSimulator(String probe, String sysdir, String chanName) {
        // Get a list of the tune channel files to use.
        String dirpath = sysdir + "/tune/" + probe;
        File[] chanFiles = null;
        if (chanName != null) {
            // Use only a specified channel
            File[] f = {new File(dirpath, chanName)};
            chanFiles = f;
            if (!f[0].canRead()) {
                System.err.println("Cannot read channel file \""
                                   + f[0].getPath() + "\"");
            }
        } else {
            // Use all the channels in the directory
            chanFiles = new File(dirpath).listFiles(new ChanFileFilter());
            if (chanFiles == null) {
                System.err.println("Cannot read directory \"" + dirpath + "\"");
            } else if (chanFiles.length == 0) {
                System.err.println("No channel files in \"" + dirpath + "\"");
            }
        }

        // Get the motor limits
        String path = sysdir + "/tmp/ptuneMotorLimits";
        m_motorLimits = getMotorLimits(path).toArray(new Limits[0]);

        // Create ChannelInfos for all the channels.
        for (File chanFile : chanFiles) {
            // Make a RefResonance for channel (dip simulator)
            ChannelInfo ci = new ChannelInfo(chanFile.getPath());
            TunePosition tp = ci.getPositionAt(Double.NaN, null, 0, 0, 0, true);
            if (tp != null) {
                // Call calcMotorSensitivity for tune and match on resonance
                ArrayList<Integer> motorArray = new ArrayList<Integer>();
                for (int i = 0, j = 0; (j = ci.getMotorNumber(i)) >= 0; i++) {
                    motorArray.add(j);
                }
                int[] motors = new int[motorArray.size()];
                for (int i = 0; i < motorArray.size(); i++) {
                    motors[i] = motorArray.get(i);
                }

                // Hack for variable "switch" motors that "assist" tune motor
                int tuneIdx = ci.getTuneMotorChannelIndex();
                int mult = 1;
                mult += ci.getFrequencyDependentSwitches().length;
                if (mult > 1) {
                    tp.setDfDp(tuneIdx, tp.getDfDp(tuneIdx) * mult);
                }

                m_dips.add(new RefResonance(tp, motors, 100e-9, 0.5, ci));
            }
        }
    }

    /**
     * Read the limits on the motor positions (including backlash)
     * from the MotorLimits file.
     * @param path The path to the file.
     * @return A list of the Limits, indexed by global motor index (gmi).
     */
    private List<Limits> getMotorLimits(String path) {
        List<Limits> limits = new ArrayList<Limits>();
        String buf = TuneUtilities.readFile(path, true);
        StringTokenizer toker = new StringTokenizer(buf);
        while (toker.hasMoreTokens()) {
            String line = toker.nextToken();
            StringTokenizer toker2 = new StringTokenizer(line, ": ");
            if (toker2.countTokens() == 2) {
                try {
                    int min = Integer.parseInt(toker2.nextToken());
                    int max = Integer.parseInt(toker2.nextToken());
                    limits.add(new Limits(min, max));
                } catch (NumberFormatException nfe) {
                    limits.add(new Limits(NaN, NaN));
                }
            }
        }
        return limits;
    }

    /**
     * Get a list of CoilParams objects -- one for each active resonance.
     * All based on the current motor positions: Switch motors are used to
     * decide if a channel is "active" and should have a resonance generated
     * for it. "Motor" (tune and match) motors are used to determine
     * @param motorPositions A list of the motor positions by gmi.
     * @return The list of CoilParams.
     */
    public CoilParams[] getCoilParams(int[] motorPositions) {
        ArrayList<CoilParams> coilParams = new ArrayList<CoilParams>();
        for (RefResonance ref : m_dips) {
            if (ref.isResonanceActive(motorPositions, m_motorLimits)) {
                coilParams.add(ref.getCoilParams(motorPositions));
            }
        }

        CoilParams[] cp = coilParams.toArray(new CoilParams[0]);
        return cp;
    }

    /**
     * Adjust the tune capacitance in the given coil parameters to move
     * the resonance to the given frequency.
     * This is an approximate adjustment, based on the tune sensitivity.
     * If the given tune sensitivity is null, it is estimated at the
     * current location.
     * @param freq The target frequency.
     * @param datum The resonance center for the input coil parameters.
     * @param coilParams The starting coil parameters.
     * @param tuneSens The estimated tune sensitivity, or null.
     * @return The same CoilParams object, with updated cMatch.
     */
    public static CoilParams calcTuneCap(double freq,
                                         ReflectionDatum datum,
                                         CoilParams coilParams,
                                         CapSensitivity tuneSens) {
        double f0 = datum.getFreq();
        if (tuneSens == null) {
            tuneSens = calcTuneSensitivity(coilParams, f0);
        }
        double dTune = (freq - f0) / tuneSens.getDfreqDcap();
        coilParams.cTune += dTune;
        return coilParams;
    }

    /**
     * Adjust the match capacitance in the given coil parameters to move
     * the center of the resonance to zero reflection.
     * This is an approximate adjustment, based on the given match sensitivity.
     * If the given match sensitivity is null, it is estimated at the
     * current location.
     * @param datum The resonance center for the input coil parameters.
     * @param coilParams The starting coil parameters.
     * @param matchSens The estimated match sensitivity, or null.
     * @return The same CoilParams object, with updated cMatch.
     */
    public static CoilParams calcMatchCap(SignedReflectionDatum datum,
                                          CoilParams coilParams,
                                          CapSensitivity matchSens) {
        double freq = datum.getFreq();
        double refl = datum.signedRefl();
        if (matchSens == null) {
            matchSens = calcMatchSensitivity(coilParams, freq);
        }
        double dMatch = -refl / matchSens.getDreflDcap();
        coilParams.cMatch += dMatch;
        return coilParams;
    }

    /**
     * Calculate the sensitivity of the tuning resonance (frequency and
     * reflection at the dip) to the tune capacitor.
     * @param coilParams The coil parameters of the resonance.
     * @param freq The (approximate) frequency of the resonance (Hz).
     * @return The capacitor sensitivities.
     */
    private static CapSensitivity calcTuneSensitivity(CoilParams coilParams,
                                                      double freq) {
        CapSensitivity tuneSens;
        CoilParams cp = coilParams.clone();
        double dTune = coilParams.cTune / 100.0;
        cp.cTune -= dTune / 2;
        SignedReflectionDatum datum1 = calcDipFreqAndRefl(freq, cp);
        double freq1 = datum1.getFreq();
        double refl1 = datum1.signedRefl();
        cp.cTune += dTune;
        SignedReflectionDatum datum2 = calcDipFreqAndRefl(freq, cp);
        double freq2 = datum2.getFreq();
        double refl2 = datum2.signedRefl();
        tuneSens = new CapSensitivity((freq2 - freq1) / dTune,
                                       (refl2 - refl1) / dTune);
        return tuneSens;
    }

    /**
     * Calculate the sensitivity of the tuning resonance (frequency and
     * reflection at the dip) to the match capacitor.
     * @param coilParams The coil parameters of the resonance.
     * @param freq The (approximate) frequency of the resonance (Hz).
     * @return The capacitor sensitivities.
     */
    private static CapSensitivity calcMatchSensitivity(CoilParams coilParams,
                                                       double freq) {
        CapSensitivity matchSens;
        double dMatch = coilParams.cMatch / 50.0;
        coilParams.cMatch -= dMatch / 2;
        SignedReflectionDatum datum1 = calcDipFreqAndRefl(freq, coilParams);
        double freq1 = datum1.getFreq();
        double refl1 = datum1.signedRefl();
        coilParams.cMatch += dMatch;
        SignedReflectionDatum datum2 = calcDipFreqAndRefl(freq, coilParams);
        double freq2 = datum2.getFreq();
        double refl2 = datum2.signedRefl();
        matchSens = new CapSensitivity((freq2 - freq1) / dMatch,
                                       (refl2 - refl1) / dMatch);
        coilParams.cMatch -= dMatch / 2; // Restore original coil params
        return matchSens;
    }

    /**
     * Calculate the dip for the given coil parameters.
     * @param freq A hint for the frequency of the dip.
     * @param pars The coil parameters defining the resonance.
     * @return The actual frequency and reflection at the resonance.
     */
    public static SignedReflectionDatum calcDipFreqAndRefl(double freq,
                                                           CoilParams pars) {
        // Check reflection at different frequencies until we go through dip
        ArrayList<ReflectionDatum> reflData = new ArrayList<ReflectionDatum>();
        reflData.add(new ReflectionDatum(freq, pars));
        double minRefl = reflData.get(0).absRefl();
        //double maxRefl = reflData.get(0).absRefl();
        double deltaFreq = 0.05e6;

        // Sample increasing frequencies until reflection increases:
        double f = freq;
        double absR = minRefl;
        do {
            f += deltaFreq;
            reflData.add(new ReflectionDatum(f, pars));
            int mx = reflData.size() - 1;
            absR = reflData.get(mx).absRefl();
            minRefl = Math.min(minRefl, absR);
        } while (absR <= minRefl);
        // Sample decreasing frequencies until reflection increases:
        f = freq;
        do {
            f -= deltaFreq;
            reflData.add(0, new ReflectionDatum(f, pars));
            absR = reflData.get(0).absRefl();
            minRefl = Math.min(minRefl, absR);
        } while (absR <= minRefl);

        // Find index of datum nearest minimum
        int dipIdx;
        for (dipIdx = 1; reflData.get(dipIdx).absRefl() != minRefl; dipIdx++);


        // Get reflections at 3 frequencies around dip
        Complex[] r = new Complex[3];
        double[] frq = new double[3];
        for (int i = dipIdx - 1, j = 0; i <= dipIdx + 1; i++, j++) {
            r[j] = reflData.get(i).getRefl();
            frq[j] = reflData.get(i).getFreq();
        }

        // Calculate circle through the 3 points
        double[] circle = vnmr.util.NLFit.getCircle(r[0].real(), r[0].imag(),
                                                    r[1].real(), r[1].imag(),
                                                    r[2].real(), r[2].imag());

        // Fit frequency to zeta = tan(theta/2) from center of fit circle
        // theta = 0 is at the center of the dip (point nearest origin)
        double theta0 = Math.atan2(0 - circle[1], 0 - circle[0]);
        // Calculate the zetas
        double[] zeta = new double[3];
        for (int i = 0; i < 3; i++) {
            double theta = Math.atan2(r[i].imag() - circle[1],
                                      r[i].real() - circle[0]);
            zeta[i] = Math.tan((theta - theta0) / 2);
        }

        // Do the linear fit: frq = a[0] + a[1] * zeta
        PolyBasisFunctions poly = new PolyBasisFunctions(1);
        SvdFitResult ans = vnmr.util.LinFit.svdFit(zeta, frq, null, poly);
        double[] a = ans.getCoeffs();
        // a[0] is the fit to frq at zeta=0
        SignedReflectionDatum rtn = new SignedReflectionDatum(a[0], pars,
                                                              circle);
        return rtn;
    }


    /**
     * A container class holding the frequency and complex reflection
     * of a resonance.
     */
    static class ReflectionDatum {
        /** Frequency of the resonance (Hz). */
        private double freq;

        /** Complex reflection at the resonant frequency. */
        private Complex refl;


        /**
         * Create a ReflectionDatum using the given frequency and CoilParams.
         * The given frequency is assumed to be at the center of the
         * resonance for the given coil parameters.
         * @param freq The frequency of the resonance (Hz).
         * @param pars The CoilParams to use to calculate the reflection.
         */
        public ReflectionDatum(double freq, CoilParams pars) {
            this.freq = freq;
            this.refl = Mtune.calculateReflection(freq, pars);
        }

        /**
         * Get the absolute value of the reflection in this datum.
         * @return The modulus of the complex reflection.
         */
        double absRefl() {
            return getRefl().mod();
        }

        /**
         * @return The resonant frequency (Hz).
         */
        public double getFreq() {
            return freq;
        }

        /**
         * @return The reflection at the resonant frequency.
         */
        public Complex getRefl() {
            return refl;
        }
    }


    /**
     * A container class holding the frequency and complex reflection
     * of a resonance, as well as the sign of the modulus of the reflection.
     */
    static class SignedReflectionDatum extends ReflectionDatum {
        /** The sign of the modulus of the reflection. */
        private double sign;

        /**
         * Create a SignedReflectionDatum using the given frequency, CoilParams,
         * and Circle fit to the data that defines the dip.
         * The given frequency is assumed to be at the center of the
         * resonance for the given coil parameters.
         * @param freq The frequency of the resonance (Hz).
         * @param pars The CoilParams to use to calculate the reflection.
         * @param circle The circle fit to the resonance -- used only to
         * determine the sign of the reflection.
         */
        public SignedReflectionDatum(double freq,
                                     CoilParams pars, double[] circle) {
            super(freq, pars);
            double circleRadius = Math.hypot(getRefl().real() - circle[0],
                                             getRefl().imag() - circle[1]);
            double originDist = Math.hypot(circle[0], circle[1]);
            this.sign = originDist >= circleRadius ? 1 : -1;
        }


        /**
         * Get the signed value of the reflection in this datum.
         * Negative if it's inside the fit circle, otherwise positive.
         * @return Plus or minus the modulus of the complex reflection.
         */
        double signedRefl() {
            return getRefl().mod() * sign;
        }
    }


    /**
     * This is a container for the sensitivity of the dip frequency and
     * reflection at the resonant frequency to a given capacitor.
     */
    static class CapSensitivity {
        /** Sensitivity of frequency to capacitance (Hz/F). */
        private double dfdc;

        /** Sensitivity of reflection to capacitance (reflection/F). */
        private double drdc;

        /**
         * Construct a CapSensitivity instance with the given sensitivities
         * to a capacitor.
         * @param dfdc The frequency sensitivity (Hz/F).
         * @param drdc The sensitivity of the reflection amplitude (1/F).
         */
        public CapSensitivity(double dfdc, double drdc) {
            this.dfdc = dfdc;
            this.drdc = drdc;
        }

        /**
         * @return The sensitivity of the resonant frequency
         * to capacitance (Hz/F).
         */
        public double getDfreqDcap() {
            return dfdc;
        }

        /**
         * @return The sensitivity of reflection to capacitance at the
         * resonant frequency (reflection/F).
         */
        public double getDreflDcap() {
            return drdc;
        }
    }


    /**
     * This class represents a resonance with a particular reference
     * frequency. Its coil parameters produce a resonance at that frequency.
     */
    public static class RefResonance {
        /** The coil parameters at the reference frequency. */
        private CoilParams mm_coilParams;

        /** The sensitivity to the tune capacitor. */
        private CapSensitivity mm_tuneSensitivity;

        /** The sensitivity to the match capacitor. */
        private CapSensitivity mm_matchSensitivity;
        
        /** The sensitivity of the coil parameters to motor positions. */
        private List<CoilParamSensitivity> mm_coilParamSensitivities
            = new ArrayList<CoilParamSensitivity>();
        
        /** The tune channel this resonance was made for. */
        private ChannelInfo mm_channelInfo = null;


        /**
         * Construct a reference resonance at the given frequency with the
         * given coil inductance and resistance.
         * @param tp The reference frequency and motor sensitivities.
         * @param motors Ordered list of the gmi's of the tuning motors.
         * @param lCoil The coil inductance (H).
         * @param rCoil The coil resistance (Ohms).
         * @param chanInfo The tune channel that this resonance is for.
         */
        public RefResonance(TunePosition tp, int[] motors, double lCoil,
                            double rCoil, ChannelInfo chanInfo) {
            double freq = tp.getFreq();
            CoilParams coilParams = new CoilParams(lCoil, rCoil, 0, 0);
            coilParams = Mtune.calculateNominalCaps(freq, coilParams);
            SignedReflectionDatum datum = calcDipFreqAndRefl(freq, coilParams);
            double f0 = datum.getFreq();
            CapSensitivity matchSens = calcMatchSensitivity(coilParams, f0);
            CapSensitivity tuneSens = calcTuneSensitivity(coilParams, f0);
            for (int i = 0; i < 10; i++) {
                coilParams = calcMatchCap(datum, coilParams, matchSens);
                matchSens = calcMatchSensitivity(coilParams, f0);
                datum = calcDipFreqAndRefl(freq, coilParams);
                coilParams = calcTuneCap(freq, datum, coilParams, tuneSens);
                tuneSens = calcTuneSensitivity(coilParams, f0);
                datum = calcDipFreqAndRefl(freq, coilParams);
                if (Math.abs(datum.getFreq() - freq) < 1
                    && datum.absRefl() < 1e-4)
                {
                    break;
                }
                f0 = datum.getFreq();
            }
            mm_coilParams = coilParams;
            mm_tuneSensitivity = tuneSens;
            mm_matchSensitivity = matchSens;
            mm_channelInfo = chanInfo;
            for (int i = 0; i < motors.length; i++) {
                int refPosition = tp.getPosition(i);
                calcMotorSensitivity(motors[i], refPosition,
                                     tp.getDfDp(i), tp.getDrDp(i));
            }
        }

        /**
         * Calculate the sensitivity of the tune and match capacitors
         * to a motor. We internally store the change in tune and match
         * capacitance necessary to simulate one step of the motor.
         * These are the sensitivities when the resonance is tuned at the
         * reference frequency.
         * @param gmi The index number of the motor (Global Motor Index).
         * @param refPosition The motor position at the reference frequency.
         * @param dfdp The change in dip frequency per motor step (Hz).
         * @param drdp The change in reflection amplitude per motor step.
         */
        public void calcMotorSensitivity(int gmi, int refPosition,
                                         double dfdp, double drdp) {
            double[][] eqns = new double[2][3];
            eqns[0][0] = getTuneSensitivity().getDfreqDcap();
            eqns[0][1] = getMatchSensitivity().getDfreqDcap();
            eqns[0][2] = dfdp;
            eqns[1][0] = getTuneSensitivity().getDreflDcap();
            eqns[1][1] = getMatchSensitivity().getDreflDcap();
            eqns[1][2] = drdp;
            double[] deltaCaps = NLFit.solve(eqns);

            CoilParamSensitivity cps = new CoilParamSensitivity();
            cps.gmi = gmi;
            cps.refPosition = refPosition;
            cps.dtdp = deltaCaps[0];
            cps.dmdp = deltaCaps[1];
            mm_coilParamSensitivities.add(cps);
        }

        /**
         * Get the coil parameters appropriate for the given motor positions.
         * @param mtrPositions Motor positions, indexed by gmi.
         * @return A modified copy of the reference coil parameters.
         */
        public CoilParams getCoilParams(int[] mtrPositions) {
            CoilParams coilParams = mm_coilParams.clone();
            for (CoilParamSensitivity cps : mm_coilParamSensitivities) {
                int gmi = cps.gmi;
                double deltaPosition = mtrPositions[gmi] - cps.refPosition;
                // If there are any slaves to this motor, they contribute
                // to the deltaPosition.
                while ((gmi = mm_channelInfo.getSlaveOf(gmi)) >= 0) {
                    deltaPosition += mtrPositions[gmi] - cps.refPosition;
                }
                double deltaTune = deltaPosition * cps.dtdp;
                double deltaMatch = deltaPosition * cps.dmdp;
                coilParams.cTune += deltaTune;
                coilParams.cMatch += deltaMatch;
            }
            return coilParams;
        }

        /**
         * Get the sensitivity of the dip's frequency to the capacitance.
         * @return Frequency sensitivity (Hz/F).
         */
        public CapSensitivity getTuneSensitivity() {
            return mm_tuneSensitivity;
        }

        /**
         * Get the sensitivity of the dip's reflection at minimum
         * to the capacitance.
         * @return Reflection sensitivity (1/F).
         */
        public CapSensitivity getMatchSensitivity() {
            return mm_matchSensitivity;
        }

        /**
         * Determine if this resonance is active, given the specified
         * motor positions. The switch motors turn resonances on and off.
         * The motor limits are needed because some switch positions are
         * specified in the channel files at positions beyond the range
         * of travel (like 0). Since the motor can't get to that position,
         * the expected switch positions are clipped to legal values.
         * @param mtrPositions Motor positions, indexed by gmi.
         * @param mtrLimits The legal limits of motor positions.
         * @return True if this resonance is active.
         */
        public boolean isResonanceActive(int[] mtrPositions,
                                         Limits[] mtrLimits) {
            SwitchPosition[] switchList = mm_channelInfo.getFixedSwitches();
            for (SwitchPosition sp : switchList) {
                int gmi = sp.getMotor();
                int posn = sp.getPosition();
                posn = (int)mtrLimits[gmi].clip(posn);
                if (gmi >= mtrPositions.length || posn != mtrPositions[gmi]) {
                    return false;
                }
            }
            return true;
        }


        /**
         * This class contains information about how the virtual probe's
         * parameters (tune and match capacitances) depend on the position
         * of one motor.
         * Only used by the RefResonance class.
         */
        private static class CoilParamSensitivity {
            /** The motor number these values apply to. */
            public int gmi;

            /** The reference position of the motor. */
            public int refPosition;

            /** Variation of virtual tune capacitor with motor position. */
            public double dtdp;

            /** Variation of virtual match capacitor with motor position. */
            public double dmdp;
        }
    }
}
