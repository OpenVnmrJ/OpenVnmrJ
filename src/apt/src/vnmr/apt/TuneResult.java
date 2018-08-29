/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.util.Collection;
import java.util.Set;
import java.util.TreeSet;

import vnmr.util.Fmt;

/**
 * This class represents the result of a tuning attempt.
 * Two TuneResult objects are "equal" if they have the same
 * requested frequency.
 */
public class TuneResult {
    private double m_actualMatch_db = Double.NaN;
    private Set<Integer> m_eotMotors = new TreeSet<Integer>();
    /** tuneResult = "ok", "warning", or "failed" */
    private String m_tuneResult;
    private String m_failureMessage = null;
    private Object m_note = null;
    private TuneTarget m_target = null;


    public TuneResult(String result) {
        setTuneResult(result);
    }

    public TuneResult(String result, String failureMessage) {
        setTuneResult(result);
        setFailureMessage(failureMessage);
    }

    public TuneResult() {
    }

    /**
     * Get a one-line string expressing the result.
     * Equivalent to <code>toString("percent")</code>.
     * @see #toString(String)
     * @return The status string.
     */
    public String toString() {
        return toString("percent");
    }

    /**
     * Get a one-line string expressing the result.
     * The first word is "ok" or "failed".
     * If tuning was attempted, gives the match at the requested frequency.
     * The match is expressed in dB or percent of optimum pulse width
     * depending on whether "fmt" is "dB" or "percent", respectively.
     * The default format is "percent".
     * Any motors at End-Of-Travel are reported.
     * @param fmt The format of the match: "dB" or "percent".
     * @return The status string.
     */
    public String toString(String fmt) {
        Messages.postDebug("TuneResult","TuneResult.toString(" + fmt + ")");
        String result = getTuneResult();
        String rtn = result;
        if (getFailureMessage() != null) {
            if (!Double.isNaN(getRequestedFreq())) {
                rtn += " " + Fmt.f(3, getRequestedFreq() / 1e6, false)
                        + " MHz";
            }
            rtn += " " + getFailureMessage();
        } else {
            if (!Double.isNaN(getRequestedFreq())) {
                double actualMatch = getActualMatch_db();
                double reqMatch = getRequestedMatch_db();
                double okMatch = getAcceptableMatch_db();
                Messages.postDebug("TuneResult",
                                   "actualMatch=" + actualMatch
                                   + ", reqMatch=" + reqMatch
                                   + ", okMatch=" + okMatch);
                rtn += " - tuning to " + m_target.getFrequencyName();
                String targetName = m_target.getCriterionName();
                String acceptName = m_target.getAcceptName();
                if ("dB".equalsIgnoreCase(fmt) || targetName == null) {
                    // Not tuning to a standard criterion
                    rtn += ", match is " + Fmt.f(1, getActualMatch_db())
                            + " dB";
                } else if (actualMatch <= okMatch) {
                    // NB: reqCriterion != null
                    if (actualMatch <= reqMatch) {
                        rtn += " meets " + targetName + " criterion";
                    } else if (acceptName != null) {
                        rtn += " meets " + acceptName + " criterion";
                    } else {
                        rtn += " at " + TuneCriterion.getMatch_str(actualMatch);
                    }
                    if (actualMatch > reqMatch) {
                        rtn += " - failed to reach " + targetName + " criterion";
                    }
                } else { // Failed
                    if (actualMatch > reqMatch) {
                        rtn += " - failed to reach " + targetName;
                        if (okMatch > reqMatch) {
                            rtn += " or " + acceptName;
                        }
                        rtn += " criterion";
                    }
                }
            }
            for (int gmi : getEotMotors()) {
                rtn += " - Motor " + gmi + " is at EOT";
            }
        }
        rtn += getNote();
        return rtn;
    }

    /**
     * Returns the word (token) that is to prefix the VnmrJ info message
     * of the tuning outcome.
     * Checks the current m_tuneResult, which is "ok",
     * "warning", or "failed".
     * If it is "warning", "Warning:" is returned, which causes the message
     * to be printed as a warning.
     * Otherwise, just returns m_tuneResult.
     * @return The first word of the info message.
     */
    public String getTuneResult() {
        if ("warning".equals(m_tuneResult)) {
            return "Warning:";
        } else {
            return m_tuneResult;
        }
    }

    /**
     * Set the result status for this TuneResult. 
     * @param result The (case-insensitive) result name.
     * @throws IllegalArgumentException If "result" is not one of
     * "ok", "warning", or "failed".
     */
    public void setTuneResult(String result)
    throws IllegalArgumentException {
        result = result.toLowerCase();
        if (!"ok".equals(result)
            && !"warning".equals(result)
            && !"failed".equals(result))
        {
            throw new IllegalArgumentException("Bad tune result: \""
                                               + result + "\"");
        }
        m_tuneResult = result;
    }

    public double getRequestedFreq() {
        if (m_target != null) {
            return m_target.getFreq_hz();
        } else {
            return Double.NaN;
        }
    }

//    public void setRequestedFreq(double freq) {
//        m_requestedFreq_hz = freq;
//    }

    public double getRequestedMatch_db() {
        if (m_target != null) {
            return m_target.getCriterion().getTarget_db();
        } else {
            return Double.NaN;
        }
    }

//    public void setRequestedMatch_db(double match_db) {
//        m_requestedMatch_db = match_db;
//    }

    public double getAcceptableMatch_db() {
        if (m_target != null) {
            return m_target.getCriterion().getAccept_db();
        } else {
            return Double.NaN;
        }
    }

//    public void setAcceptableMatch_db(double match_db) {
//        m_acceptableMatch_db = match_db;
//    }

    public double getActualMatch_db() {
        return m_actualMatch_db;
    }

    public void setActualMatch_db(double match_db) {
        m_actualMatch_db = match_db;
    }

    public Set<Integer> getEotMotors() {
        return m_eotMotors;
    }

    public void setEotMotors(Collection<Integer> gmiList) {
        m_eotMotors.clear();
        m_eotMotors.addAll(gmiList);
    }

    public void setEotMotor(int gmi) {
        m_eotMotors.add(gmi);
    }

    public String getFailureMessage() {
        return m_failureMessage;
    }

    public void setFailureMessage(String message) {
        m_failureMessage = message;
    }

    public void setNote(String note) {
        m_note = note;
    }

    public String getNote() {
        String rtn = "";
        if (m_note != null) {
            rtn = " -- " + m_note;
        }
        return rtn;
    }

    public boolean equals(Object obj) {
        return getRequestedFreq() == ((TuneResult)obj).getRequestedFreq();
    }

    public void setTarget(TuneTarget target) {
        m_target  = target;
    }

}