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

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.StringTokenizer;
import java.util.TreeMap;

import vnmr.util.Fmt;
import vnmr.util.QuotedStringTokenizer;


/**
 * Holds the criterion for successful tuning.
 * A tune criterion instance is immutable, but the static value giving
 * the default
 */
public class TuneCriterion {
    private double m_target_db; // Negative
    private Double m_accept_db = null; // Negative
    private String m_name;
    private String m_acceptName = null;

    static private double sm_defaultMargin_db = 0;
    static private Map<String,TuneCriterion> sm_stdCriteria
            = new TreeMap<String,TuneCriterion>();

    // Define the default standard criteria.
    final static private double COARSE_DB = percentToDb(5);
    final static private double MEDIUM_DB = percentToDb(2);
    final static private double FINE_DB = percentToDb(0.5);
    final static private String COARSE = "Coarse";
    final static private String MEDIUM = "Medium";
    final static private String FINE = "Fine";


    static {
        // Initialize the list (map) of standard criteria
        addStandardCriterion(COARSE, COARSE_DB, null);
        addStandardCriterion(MEDIUM, MEDIUM_DB, COARSE);
        addStandardCriterion(FINE, FINE_DB, MEDIUM);
        // Possible modifications or additions from configuration file
        addStandardCriteria();
    }


    /**
     * Construct a new match criterion.
     * The dB values are forced to be negative and are rounded to 0.1 dB.
     * @param name The name of the criterion, e.g., "fine".
     * @param target_db The target reflection in dB.
     * @param acceptName The name of the fallback criterion, e.g., "medium".
     */
    private TuneCriterion(String name, double target_db, String acceptName) {
        m_target_db = -absRound(target_db);
        m_name = name == null ? getMatch_str(m_target_db) : name;
        m_acceptName = acceptName == null ? name : acceptName;
    }

    /**
     * Construct a new tune criterion.
     * The dB values are forced to be negative and are rounded to 0.1 dB.
     * @param name The name of the criterion, e.g., "fine".
     * @param target_db The target reflection in dB.
     * @param accept_db The acceptable, fallback reflection in dB.
     */
    private TuneCriterion(String name, double target_db, double accept_db) {
        m_target_db = -absRound(target_db);
        m_accept_db = Math.max(-absRound(accept_db), m_target_db);
        m_name = name == null ? getMatch_str(m_target_db) : name;
        m_acceptName = m_accept_db + " dB";
    }

    /**
     * Construct a new tune criterion with
     * an accept level higher than the target by the value of
     * sm_defaultMargin_db.
     * The name is set to the match target in dB.
     * The dB value is rounded to the nearest 0.1 dB and forced to be negative.
     * @param target_db The target reflection in dB.
     */
    public TuneCriterion(double target_db) {
        m_target_db = -absRound(target_db);
        m_accept_db = m_target_db + sm_defaultMargin_db;
        m_name = m_target_db + " dB";
        m_acceptName = m_accept_db + " dB";
    }

    /**
     * Check for "TuneCriteria" files with custom tuning thresholds.
     * Values are read from either $vnmruser/tune/TuneCriteria or
     * from $vnmrsystem/tune/TuneCriteria, with the former taking precedence.
     * Values may either introduce new criteria to supplement the "Coarse",
     * "Medium", "Fine" choices, or these standard choices may be modified.
     * Each line in the file is a specification in the form:
     * <pre>
     * name match_db fallbackName
     * </pre>
     * The "name" and "match_db" are required; the "fallbackName" if present
     * is the name of another, looser tune criterion which, if met by the
     * final tuning will produce a successful result status, even if the
     * "match_db" criterion is not met.
     */
    private static void addStandardCriteria() {
        String sysPath = ProbeTune.getVnmrSystemDir() + "/tune/TuneCriteria";
        String usrPath = ProbeTune.getVnmrUserDir() + "/tune/TuneCriteria";
        String path = usrPath;
        if (!new File(path).canRead()) {
            path = sysPath;
        }
        if (new File(path).canRead()) {
            BufferedReader in = TuneUtilities.getReader(path);
            if (in != null) {
                try {
                    String line;
                    while ((line = in.readLine()) != null) {
                        Messages.postDebug("TuneCriterion",
                                           "tuneCriterion line: " + line);
                        addStandardCriterion(line);
                    }
                } catch (IOException ioe) {
                } finally {
                    try { in.close(); } catch (Exception e) {}
                }
            }

        }
    }

    /**
     * Add or modify a standard tune criterion based on the given specification
     * line.
     * @param line The text line containing the specification
     * for the new criterion.
     * @see #addStandardCriteria()
     */
    private static void addStandardCriterion(String line) {
        StringTokenizer toker = new QuotedStringTokenizer(line);
        String fallbackName = null;
        try {
            String name = toker.nextToken();
            double thresh_db = Double.parseDouble(toker.nextToken());
            if (toker.hasMoreTokens()) {
                fallbackName = toker.nextToken();
            }
            addStandardCriterion(name, thresh_db, fallbackName);
        } catch (NumberFormatException nfe) {

        } catch (NoSuchElementException nsee) {
        }
    }

    /**
     * Add or modify a standard tune criterion based on the given
     * specifications.
     * @param name The name for the new criterion.
     * @param threshDb The target match threshold in dB.
     * @param acceptName The name of a fallback criterion, or null.
     * @see #addStandardCriteria()
     */
    private static void addStandardCriterion(String name, double threshDb,
                                             String acceptName) {

        sm_stdCriteria.put(name.toLowerCase(),
                           new TuneCriterion(name, threshDb, acceptName));
    }

    /**
     * Determine if a given name belongs to a standard criterion.
     * @param name The given name.
     * @return True if the name corresponds to a standard criterion.
     */
    static public boolean isStandardCriterion(String name) {
        return sm_stdCriteria.containsKey(name.toLowerCase());
    }

    /**
     * Get the standard tune criterion with the given name.
     * Standard names are "Fine", "Medium", and "Coarse" -- case insensitive.
     * @param name The given name.
     * @return The corresponding Tune Criterion, or null.
     */
    static public TuneCriterion getCriterion(String name) {
        return sm_stdCriteria.get(name.toLowerCase());
    }

    static public String getMatch_str(double db) {
        return Fmt.f(0, db) + " dB";
    }

//    static public TuneCriterion getCriterionByDb(double db) {
//        Messages.postDebug("TuneCriterion", "getCriterionByDb(" + db + ")");
//        TuneCriterion rtn = null;
//        TuneCriterion[] criteria = TuneCriterion.getStandardCriteria();
//        for (TuneCriterion criterion : criteria) {
//            double criterion_db = criterion.getTarget_db();
//            Messages.postDebug("TuneCriterion",
//                               "getCriterionByDb: criterion=" + criterion_db);
//            if (Math.abs(db - criterion_db) < 0.5) {
//                rtn = criterion;
//            }
//        }
//        return rtn;
//    }

//    /**
//     * Given a tune level in dB, returns the next looser tune criterion,
//     * or null;
//     * @param db The given tune level.
//     * @return A standard TuneCriterion looser than the given level.
//     */
//    static public TuneCriterion getLooseCriterionByDb(double db) {
//        TuneCriterion rtn = null;
//        TuneCriterion[] criteria = TuneCriterion.getStandardCriteria();
//        // NB: Start with finest criterion; end with second coarsest
//        for (int i = criteria.length - 1; i > 0; --i) {
//            double thisDb = criteria[i].getTarget_db();
//            if (Math.abs(db - thisDb) < 0.5) {
//                rtn = criteria[i - 1];
//                break;
//            }
//        }
//        return rtn;
//    }

//    /**
//     * Given a standard TuneCriterion, returns the next looser standard
//     * tune criterion, or this same criterion if there is none looser.
//     * @param target The given Tune Criterion.
//     * @return The next looser standard Tune Criterion.
//     */
//    static public TuneCriterion getLooserStdCriterion(TuneCriterion target) {
//        TuneCriterion rtn = target;
//        TuneCriterion[] criteria = TuneCriterion.getStandardCriteria();
//        // NB: Start with finest criterion; end with second coarsest
//        for (int i = criteria.length - 1; i > 0; --i) {
//            if (criteria[i] == target) {
//                rtn = criteria[i - 1];
//                break;
//            }
//        }
//        return rtn;
//    }

    public double getTarget_db() {
        return m_target_db;
    }

    public double getTarget_refl() {
        return Math.pow(10, m_target_db / 20);
    }

    public double getTarget_pwr() {
        return Math.pow(10, m_target_db / 10);
    }

    public String getName() {
        return m_name;
    }

    /**
     * Get the worst acceptable match. This is the fallback target, if
     * present, otherwise the target match.
     * @return The target or fallback match in dB.
     */
    public double getAccept_db() {
        TuneCriterion accept = null;
        if (m_accept_db != null) {
            return m_accept_db;
        } else if (m_acceptName != null
                && ((accept = getCriterion(m_acceptName)) != null))
        {
            return Math.max(accept.getTarget_db(), getTarget_db());
        } else {
            return getTarget_db();
        }
    }

    public double getAccept_refl() {
        return Math.pow(10, getAccept_db() / 20);
    }

    public double getAccept_pwr() {
        return Math.pow(10, getAccept_db() / 10);
    }

    /**
     * Get the name of the largest acceptable reflection, like "Medium".
     * @return The name.
     */
    public String getAcceptName() {
        return m_acceptName;
    }

    /**
     * Get a new version of this TuneCriterion with a new, specified margin.
     * @param margin_db The new margin.
     * @return The new Tune Criterion.
     */
    public TuneCriterion setMargin(double margin_db) {
        margin_db = absRound(margin_db);
        double accept_db = getTarget_db() + margin_db;
        return new TuneCriterion(getName(), getTarget_db(), accept_db);
    }

    /**
     * The the default match margin, used when new tune criteria are
     * created. The margin is forced to be positive and is rounded to 0.1 dB.
     * This is the amount by which the actual reflection can exceed the
     * target reflection before the tuning fails.
     * @param margin The new default margin (dB).
     */
    static public void setDefaultMargin_db(double margin) {
        sm_defaultMargin_db = absRound(margin);
    }

    /**
     * Get the absolute value of the given number, rounded to the nearest
     * 0.1.
     * @param val The given number.
     * @return The rounded, non-negative result.
     */
    static public double absRound(double val) {
        return Math.round(Math.abs(val) * 10) / 10.0;
    }

    /**
     * Convert percent increase in pulse width over perfect match
     * to match level in dB.
     * @param percent The percent increase in pulse width.
     * @return The match in dB.
     */
    public static double percentToDb(double percent) {
        double relativeWidth = 1 + 0.01 * percent;
        double db = 10 * Math.log10(1 - 1 / (relativeWidth * relativeWidth));
        return db;
    }

    /**
     * Convert match level in dB to percent
     * increase in pulse width over perfect match.
     * @param matchDb The match level in dB (a negative number).
     * @return The percentage.
     */
    public static double dbToPercent(double matchDb) {
        //relpw90 = 100*(-1 + 1/sqrt(1 - 10**(0.1 * dblvl)))
        matchDb = Math.max(-50, Math.min(-0.1, matchDb));
        double relReflPower = Math.pow(10, 0.1 * matchDb);
        double relXmitAmplitude = Math.sqrt(1 - relReflPower);
        double relPwPct = Math.max(0.1, 100 * ((1 / relXmitAmplitude) - 1));
        return relPwPct;
    }

    /**
     * Convert match level in dB to percent
     * increase in pulse width over perfect match.
     * @param matchDb The match level in dB (a negative number).
     * @return The percentage as a string.
     */
    public static String str_dbToPercent(double matchDb) {
        return Fmt.f(1, dbToPercent(matchDb));
    }

    public String toString() {
        String name = getName();
        if (isStandardCriterion(name)) {
            return name;
        } else {
            return String.valueOf(getTarget_db());
        }
    }
}
