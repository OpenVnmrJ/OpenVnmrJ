/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import vnmr.util.Fmt;

public class TuneTarget {
    
    private String nucleus;
    private double freq_hz;
    private int rfchan;
    private TuneCriterion criterion;

    public TuneTarget(String nuc, double freq, int rfchan, String criterion) {
        this.nucleus = nuc;
        this.freq_hz = freq;
        this.rfchan = rfchan;
        this.criterion = TuneCriterion.getCriterion(criterion);
    }

    public TuneTarget(String nuc, double freq,
                      int rfchan, TuneCriterion criterion) {
        this.nucleus = nuc;
        this.freq_hz = freq;
        this.rfchan = rfchan;
        this.criterion = criterion;
    }

    public TuneTarget(double freq, TuneCriterion criterion) {
        this.nucleus = "";
        this.freq_hz = freq;
        this.rfchan = 0;
        this.criterion = criterion;
    }

    /**
     * Get a string describing the target frequency. If there is a nucleus
     * name for this target, that is returned. Otherwise, returns a string
     * giving the frequency with a "MHz" label.
     * @return A name for the target frequency.
     */
    public String getFrequencyName() {
        String rtn = getNucleus();
        if (rtn == null || rtn.trim().length() == 0) {
            rtn = getFreq_str();
        }
        return rtn;
    }

    /**
     * @return the nucleus
     */
    public String getNucleus() {
        return nucleus;
    }

    /**
     * @param nucleus the nucleus to set
     */
    public void setNucleus(String nucleus) {
        this.nucleus = nucleus;
    }

    /**
     * @return the freq_hz
     */
    public double getFreq_hz() {
        return freq_hz;
    }

    /**
     * @return the frequency in MHz as a string
     */
    public String getFreq_str() {
        return Fmt.f(3, getFreq_hz() / 1e6) + " MHz";
    }

    /**
     * @param freqHz the freq_hz to set
     */
    public void setFreq_hz(double freqHz) {
        freq_hz = freqHz;
    }

    /**
     * @return the rfchan
     */
    public int getRfchan() {
        return rfchan;
    }

    /**
     * @param rfchan the rfchan to set
     */
    public void setRfchan(int rfchan) {
        this.rfchan = rfchan;
    }

    /**
     * @return the criterion
     */
    public TuneCriterion getCriterion() {
        return criterion;
    }

    /**
     * @param criterion the criterion to set
     */
    public void setCriterion(TuneCriterion criterion) {
        this.criterion = criterion;
    }

    public double getTarget_db() {
        return criterion == null ? Double.NaN : criterion.getTarget_db();
    }

    public double getTarget_pwr() {
        return criterion == null ? Double.NaN : criterion.getTarget_pwr();
    }

    public double getTarget_refl() {
        return criterion == null ? Double.NaN : criterion.getTarget_refl();
    }

    public double getAccept_db() {
        return criterion == null ? Double.NaN : criterion.getAccept_db();
    }

    public double getAccept_pwr() {
        return criterion == null ? Double.NaN : criterion.getAccept_pwr();
    }

    public double getAccept_refl() {
        return criterion == null ? Double.NaN : criterion.getAccept_refl();
    }

    public String getCriterionName() {
        return criterion == null ? null : criterion.getName();
    }

    public String getAcceptName() {
        return criterion == null ? null : criterion.getAcceptName();
    }

}
