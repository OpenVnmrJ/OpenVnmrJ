/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;


/**
 * Holds one LC datum, a value for a particular channel (trace) and run-time.
 */
public class LcDatum {

    /** Which channel (trace number) the datum applies to. */
    private int m_trace;

    /** The run-time of the datum in ms. */
    private long m_runTime_ms;

    /** The clock-time of the datum in ms. */
    private long m_clockTime_ms;

    /** The value. */
    private double m_value;

    /**
     * @param c Which channel (trace number) the datum applies to.
     * @param t The run-time of the datum in ms.
     * @param clockTime The time-of-day in ms since 1970.
     * @param v The value.
     */
    public LcDatum(int c, long t, long clockTime, double v) {
        m_trace = c;
        m_runTime_ms = t;
        m_clockTime_ms = clockTime;
        m_value = v;
    }

    /** Return the trace number (first trace is 0). */
    public int getChannel() {
        return m_trace;
    }

    public long getRunTime() {
        return m_runTime_ms;
    }

    public long getClockTime() {
        return m_clockTime_ms;
    }

    public double getValue() {
        return m_value;
    }
}
