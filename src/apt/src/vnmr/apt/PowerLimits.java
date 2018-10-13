/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

/**
 * Holds the limits of transmitter power and receiver gain.
 */
public class PowerLimits {
    private static final double DATA_OVERFLOW_LEVEL = 32767;

    /**
     * The limits for the transmitter power.
     * First index is type of limit: Max, Min, Step for index = 0, 1, 2.
     * Second index is over the RF Channels: index = 0 for Channel 1.
     */
    private static int[][] m_powerLimits = new int[3][];

    /**
     * The limits for the receiver gain.
     * Limit type is Max, Min, Step for index = 0, 1, 2.
     */
    private static int[] m_gainLimits = new int[3];

    /**
     * The maximum acceptable signal level in the received data.
     */
    private static double m_maxOkSignal;

    /**
     * The minimum acceptable signal level in the received data, as
     * a fraction of the maximum acceptable signal.
     */
    private static double m_minOkRelativeSignal = 0.2;


    static {
        String[] apars = {"maxPower", "minPower", "stepPower"};
        String[] adefaults = {"20 20 20 20 20",
                              "-16 -16 -16 -16 -16",
                              "1 1 1 1 1"};
        for (int i = 0; i < apars.length; i++) {
            String par = apars[i];
            String key = "apt." + par;
            String value = System.getProperty(key, adefaults[i]);
            try {
                String[] tokens = value.split(" ");
                m_powerLimits[i] = new int[tokens.length];
                for (int j = 0; j < tokens.length; j++) {
                    m_powerLimits[i][j] = (int)Double.parseDouble(tokens[j]);
                }
            } catch (NumberFormatException nfe) {
                Messages.postDebugWarning("PowerLimits: Bad property value: "
                                          + key + "=\"" + value + "\"");
            } catch (NullPointerException npe) {
                Messages.postDebugWarning("PowerLimits: Property not set: "
                                          + key);
            }
        }

        String[] pars = {"maxGain", "minGain", "stepGain"};
        String[] defaults = {"60", "0", "2"};
        for (int i = 0; i < pars.length; i++) {
            String par = pars[i];
            String key = "apt." + par;
            String value = System.getProperty(key, defaults[i]);
            try {
                m_gainLimits[i] = (int)Double.parseDouble(value);
            } catch (NumberFormatException nfe) {
                Messages.postDebugWarning("PowerLimits: Bad property value: "
                                          + key + "=\"" + value + "\"");
            } catch (NullPointerException npe) {
                Messages.postDebugWarning("PowerLimits: Property not set: "
                                          + key);
            }
        }

        String key = "apt.maxSignalForAutogain";
        double dflt = DATA_OVERFLOW_LEVEL / 2;
        m_maxOkSignal = TuneUtilities.getDoubleProperty(key, dflt);
    }

    private static int getPowerLimit(int rfChan, int type) {
        rfChan = Math.max(rfChan, 1);
        if (type < 0 || type >= m_powerLimits.length) {
            throw new IndexOutOfBoundsException("Bad PowerLimit type");
        }
        if (rfChan > m_powerLimits[type].length) {
            String msg = "Bad RF Channel # (" + rfChan + ");"
                + " Legal values are between 1 and "
                + m_powerLimits[type].length;
            throw new IndexOutOfBoundsException(msg);
        }
        return m_powerLimits[type][rfChan - 1];
    }

    public static int getMaxPower(int rfChan) {
        return getPowerLimit(rfChan, 0);
    }

    public static int getMinPower(int rfChan) {
        return getPowerLimit(rfChan, 1);
    }

    public static int getStepPower(int rfChan) {
        return getPowerLimit(rfChan, 2);
    }

    private static int getGainLimit(int type) {
        if (type < 0 || type >= m_gainLimits.length) {
            throw new IndexOutOfBoundsException("Bad GainLimit type");
        }
        return m_gainLimits[type];
    }

    public static int getMaxGain() {
        return getGainLimit(0);
    }

    public static int getMinGain() {
        return getGainLimit(1);
    }

    public static int getStepGain() {
        return getGainLimit(2);
    }

    public static double getMaxOkSignal() {
        return m_maxOkSignal;
    }

    public static double getMinOkSignal() {
        return m_maxOkSignal * m_minOkRelativeSignal;
    }
}
