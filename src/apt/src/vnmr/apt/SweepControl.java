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


public abstract class SweepControl {

    protected boolean m_ok;       // Whether control is operational
    protected double m_startFreq = 280e6;
    protected double m_stopFreq = 320e6;
    protected int m_np = 255;
    protected double m_minFreq = 0;
    protected double m_maxFreq = Double.MAX_VALUE;
    protected boolean m_settingSweep = false;
    protected ProbeTune m_master;

    public SweepControl(ProbeTune master) {
        m_master = master;
        m_ok = getConnection();
    }

    public SweepControl(ProbeTune master, double start, double stop, int np) {
        m_master = master;
        m_ok = getConnection();
        m_startFreq = start;
        m_stopFreq = stop;
        m_np = np;
    }

    public boolean isConnected() {
        return m_ok;
    }

    /**
     * Get reflection data, after a specified delay.
     * If given a non-zero target frequency, tries to adjust the sweep
     * to be appropriate to tuning to that target.
     * @param chanInfo The channel to measure the reflection on.
     * @param delay_ms The time to wait before making the measurement.
     * @param targetFreq The target frequency we would like to tune to (Hz).
     * @return The reflection measurement.
     * @throws InterruptedException if this command is aborted.
     */
    public ReflectionData getData(ChannelInfo chanInfo,
                                  int delay_ms, double targetFreq)
        throws InterruptedException {

        //TuneInfo estTune = chanInfo.getEstimatedTunePosition();
        //if (estTune != null) {
        //    double center = estTune.getFrequency();
        //    setCenter(center);
        //}
        ReflectionData data = getData(chanInfo, delay_ms);
        if (data == null) {
            return null;
        }

        m_master.displayData(data);
        //double dipIndex = data.getDipIndex();
        double origSpan = m_stopFreq - m_startFreq;
        double span = origSpan;
        if (targetFreq != 0 && !data.isDipDetected()) {
            //KPL: set full sweep instead of channel limts! since nothing has been seen
            // No dip detected.  Look at whole frequency range of this channel
            //span = chanInfo.getMaxFreq() - chanInfo.getMinFreq();
            //center = (chanInfo.getMaxFreq() + chanInfo.getMinFreq()) / 2;
            //setSweep(center, span, m_np);
            chanInfo.setFullSweep();
            span = chanInfo.getMaxSweepFreq() - chanInfo.getMinSweepFreq();
            //setSpan(span);
            //setCenter(center);
            data = getData(chanInfo, 0);
            m_master.displayData(data);
            //dipIndex = data.getDipIndex();
        }
        if (targetFreq != 0 && data.isDipDetected()) {
            // Make sure we have appropriate resolution to reach the target.
            double dipFreq = data.getDipFreq();
            double step_dfreq = 0.1 * Math.abs(dipFreq - targetFreq);
            double data_dfreq = span / (m_np - 1);

            // How far from resonance can we be and still have good match?
            // r^2 = curve * x^2 < matchTol
            // ==> x < sqrt(matchTol / curve)
            double curve = Math.abs(data.getCurvature());
            double match_dfreq = m_master.getTargetReflection() / curve;
            match_dfreq *= data_dfreq; // Points to Hz conversion

            // Is data resolution enough to see dip width?
            // What is the distance in the reflection plane between
            //  data points at the target frequency?
            double deltaRefl = data.getDRefl_DIdx(targetFreq);
            double targetRefl = m_master.getTargetReflection();
            boolean matchResOk = deltaRefl <= 2 * targetRefl;
            //boolean matchResOk = data_dfreq <= match_dfreq;

            // Is data resolution bigger than a motor step?
            boolean stepResOk = data_dfreq <= step_dfreq;

            // Are we already showing max resolution?
            boolean maxRes = data_dfreq < getMinFreqStep() * 1.5;

            if (!maxRes && !stepResOk && !matchResOk) {
                // Need better resolution to figure out how far to go
                //setCenter(targetFreq);
                double span1 = 2.5 * Math.abs(targetFreq - dipFreq);
                double span2 = match_dfreq * m_np / 2; // TODO: Questionable
                //setSpan(Math.max(span1, span2));
                int rfchan = chanInfo.getRfchan();
                setSweep(targetFreq, Math.max(span1, span2), m_np, rfchan);
                data = getData(chanInfo, 0);
                m_master.displayData(data);/*CMP*/
            }
        }
        //for (int i = 0; i < 5 && !data.isDipDetected(); i++) {
        //    span *= 4;
        //    setSpan(span);
        //    data = getData(chanInfo, 0);
        //    m_master.displayData(data);/*CMP*/
        //    dipIndex = data.getDipIndex();
        //}
        //double deltaFreq = 0.5 * span;
        //for (int i = 0; i < 5 && dipIndex > data.size - endPad; i++) {
        //    center = (m_startFreq + m_stopFreq) / 2;
        //    setCenter(center + deltaFreq);
        //    data = getData(chanInfo, 0);
        //    //m_master.displayData(data);/*CMP*/
        //    dipIndex = data.getDipIndex();
        //}
        //for (int i = 0; i < 5 && dipIndex < endPad; i++) {
        //    center = (m_startFreq + m_stopFreq) / 2;
        //    setCenter(center - deltaFreq);
        //    data = getData(chanInfo, 0);
        //    //m_master.displayData(data);/*CMP*/
        //    dipIndex = data.getDipIndex();
        //}
        return data;
    }

    public int getNp() {
        return m_np;
    }

    public void setNp(int np) {
        m_np = np;
    }

    public double getSpan() {
        return m_stopFreq - m_startFreq;
    }

    public void setSpan(double span_hz) {
        double center = getCenter();
        m_startFreq = center - span_hz / 2;
        m_stopFreq = m_startFreq + span_hz;
    }

    public double getCenter() {
        return (m_stopFreq + m_startFreq) / 2;
    }

    public void setCenter(double center_hz) {
        double span = getSpan();
        m_startFreq = center_hz - span / 2;
        m_stopFreq =  m_startFreq + span;
    }


    /**
     * This method may be overridden in subclasses for efficiency.
     * @param center Center frequency in Hz.
     * @param span Frequency span in Hz.
     * @param np Number of points to acquire.
     * @param rfchan The rf channel to use (from 1).
     */
    public void setSweep(double center, double span, int np, int rfchan) {
        setSweep(center, span, np, rfchan, false);
    }

    /**
     * This method may be overridden in subclasses for efficiency.
     * @param center Center frequency in Hz.
     * @param span Frequency span in Hz.
     * @param np Number of points to acquire.
     * @param rfchan Which RF channel to use, or 0 for automatic.
     * @param force If true, send sweep change even if it seems pointless.
     */
    public void setSweep(double center, double span, int np,
                         int rfchan, boolean force) {
        setCenter(center);
        setSpan(span);
        setNp(np);
    }

    public void setExactSweep(double center, double span, int np, int rfchan) {
        setSweep(center, span, np, rfchan);
    }

    public ReflectionData getData(ChannelInfo chanInfo, long when)
        throws InterruptedException 
    {
        //chanInfo.setRfRouting();
        ReflectionData data = getData(when, chanInfo.getProbeDelay());
        if (data != null) {
            chanInfo.setData(data);
        }
        return data;
    }

    /**
     * Adjusts the sweep parameters to something that the hardware
     * may prefer. This default implementation just returns the same
     * values passed in.
     * @param center Center frequency in Hz.
     * @param span Frequency span in Hz.
     * @param np Number of points to acquire.
     * @param force If true, change sweep even if it seems pointless.
     * @return An array of 3 doubles giving the modified center, span, and
     * number of points, respectively.
     */
    public double[] adjustSweep(double center, double span,
                                int np, boolean force) {
        // TODO: Implement "force" argument?
        double[] rtn = new double[3];
        rtn[0] = center;
        rtn[1] = span;
        rtn[2] = np;
        return rtn;
    }

    protected void waitUntil(long when) throws InterruptedException {
        long delay;
        while ((delay = when - System.currentTimeMillis()) > 0) {
            Thread.sleep(delay);
        }
        return;
    }

    /**
     * Set the sweep range, and acquire data.
     * @param center The desired center frequency of the data (Hz).
     * @param span The desired frequency span of the data (Hz).
     * @param np The desired number of points in the data.
     * @param rfchan TODO
     */
    protected ReflectionData getData(double center, double span,
                                     int np, int rfchan, double probeDelay_ns)
        throws InterruptedException {

        //setCenter(center);
        //setSpan(span);
        //setNp(np);
        setSweep(center, span, np, rfchan);
        m_settingSweep = true;
        ReflectionData data = getData(0, probeDelay_ns);
        m_settingSweep = false;
        return data;
    }

    public void setLimits(double minFreq, double maxFreq) {
        m_minFreq = minFreq;
        m_maxFreq = maxFreq;
    }

    public double getMinLimit() {
        return m_minFreq;
    }

    public double getMaxLimit() {
        return m_maxFreq;
    }

    public double getMinFreqStep() {
        return 0;
    }

    public double getFreqStepOffset() {
        return 0;
    }

    public void clearCal() {
    }

    /**
     * Override in implementation, if appropriate.
     * @param type "open", etc.
     * @return True if successful.
     */
    public boolean saveCalData(String type) {
        return true;
    }

    protected void showSweepLimitsInGui() {
        m_master.sendToGui("showSweepLimits " + getCenter() + " " + getSpan());
    }

    protected abstract boolean getConnection();

    /**
     * Get data that were acquired at or after the given date and time.
     * Implementations must look at the m_settingSweep flag; if it is true,
     * data are not valid unless center, span, and np match
     * the member values m_startFreq, m_stopFreq, and m_np.
     * @param date The desired time in ms since 1970.
     */
    protected abstract ReflectionData getData(long date, double probeDelay_ns)
        throws InterruptedException ;
}
