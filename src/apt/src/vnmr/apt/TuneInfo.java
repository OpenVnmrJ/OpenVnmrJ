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

public class TuneInfo implements Serializable {

    /**
     * Holds a tune state.  This can be either a measured state, or
     * a target frequency (with match = 0) to tune to.
     */

    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    /** When this tune position was measured.  In milliseconds since 1970. */
    private long m_acqTime;

    /** The frequency (Hz). */
    private double m_freq;

    /**
     * The (signed) reflection at the resonance frequency.
     * Positive means the origin is "out of the loop".
     * Zero if this is a target frequency.
     */
    private double m_reflection = 0;

    /**
     * The uncertainty in the frequency.
     * Zero if this is a target frequency.
     */
    private double m_freqSdv = 0;

    /**
     * The uncertainty in the reflection.
     * Zero if this is just a target frequency.
     */
    private double m_reflectionSdv = 0;

    /**
     * The solvent when this tune point was measured.
     * The empty string if this is just a target frequency.
     */
    private String m_solvent = "";

    /**
     * Whether the tune may have been changed since
     * last measurement. Only used if TuneInfo represents a measurement.
     */
    private boolean m_outOfDate;

    private boolean m_signSet = false;

    /**
     * Construct a tune state from measured data.
     * @param data The data.
     */
    public TuneInfo(ReflectionData data) {
        this(data, "");
    }

    /**
     * Construct a tune state from measured data and solvent.
     * @param data The data.
     * @param solvent The solvent name.
     */
    public TuneInfo(ReflectionData data, String solvent) {
        m_acqTime = data.getAcqTime();
        m_freq = data.getDipFreq();
        m_freqSdv = data.getDipFreqSdv();

        m_reflection = data.getDipValue();
        m_reflectionSdv = data.getDipValueSdv();

        //m_complexReflection = data.getPhasedReflection();

        m_solvent = solvent;
    }

    /**
     * Construct a desired tune state.  The frequency is specified, and
     * the reflection is set to 0.
     * @param freq The frequency in Hz of the desired state.
     */
    public TuneInfo(double freq) {
        m_freq = freq;
    }

    /**
     * Construct a tune state for a given frequency and reflection.
     * @param freq The frequency in Hz of the desired state.
     * @param refl The linear reflection.
     */
    public TuneInfo(double freq, double refl) {
        m_freq = freq;
        m_reflection = refl;
    }

    /**
     * Get the age of this tune measurement in ms.  For a tune target,
     * a 0 age is reported.
     * @return The age (ms).
     */
    public long getAge() {
        if (m_acqTime == 0) {
            return 0;
        } else {
            return System.currentTimeMillis() - m_acqTime;
        }
    }

    /**
     * Mark whether this measurement is out-of-date.  E.g., the motor has
     * moved or the sample has changed since the measurement.
     * @param b If true, out-of-date.
     */
    public void setOutOfDate(boolean b) {
        m_outOfDate = b;
    }

    /**
     * See if this measurement is considered current.
     * @return If true, out-of-date.
     */
    public boolean isOutOfDate() {
        return m_outOfDate;
    }

    /**
     * @return True if we have a valid tune specification.
     */
    public boolean isValid() {
        return !Double.isNaN(m_freq);
    }

    /**
     * @return The frequency of the dip (Hz).
     */
    public double getFrequency() {
        return m_freq;
    }

    /**
     * Set the frequency of the dip.
     * @param freq The frequency of the dip (Hz).
     */
    public void setFrequency(double freq) {
        m_freq = freq;
    }

    /**
     * Return the uncertainty in the frequency of the dip.
     * @return The standard deviation (Hz).
     */
    public double getFrequencySdv() {
        return m_freqSdv;
    }

    /**
     * Set the uncertainty in the frequency of the dip.
     * @param freqSdv The standard deviation (Hz).
     */
    public void setFrequencySdv(double freqSdv) {
        m_freqSdv = freqSdv;
    }

    /**
     * Return the reflection coefficient at the dip.
     * @return The reflection amplitude.
     */
    public double getReflection() {
        return m_reflection;
    }

    /**
     * Set the reflection coefficient at the dip.
     * @param reflection The reflection amplitude.
     */
    public void setReflection(double reflection) {
        m_reflection = reflection;
    }

    /**
     * Return the uncertainty in the reflection coefficient at the dip.
     * @return The standard deviation.
     */
    public double getReflectionSdv() {
        return m_reflectionSdv;
    }

    /**
     * Set the uncertainty in the reflection coefficient at the dip.
     * @param reflectionSdv The standard deviation.
     */
    public void setReflectionSdv(double reflectionSdv) {
        m_reflectionSdv = reflectionSdv;
    }

    /**
     * Set the sign of the match to the sign of "s".  No effect if s=0.
     * @param s Negative, zero, or positive.
     */
    public void setMatchSign(double s) {
        if (m_reflection * s < 0) {
            m_reflection *= -1;
        }
        if (s != 0) {
            m_signSet = true;
        }
        Messages.postDebug("MatchSign", "setMatchSign(" + s + "): m_reflection="
                           + m_reflection);
    }

    public boolean isMatchSigned() {
        return m_signSet;
    }

    public String getSolvent() {
        return m_solvent;
    }
}
