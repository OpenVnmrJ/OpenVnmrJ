/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.lc;

import java.util.*;
import java.text.*;
import java.awt.Color;

import vnmr.util.*;

/**
 * A map for dealing with the MS status.  There is only one map, accessed
 * by static methods in this class.  The keys are Strings with values like
 * "MSQ3" -- all keys start with "MS" and are the same as the "infoproc"
 * style keys passed to ExpPanel.processStatusData(msg).
 * <br>
 * Each entry contains another copy of the key, and the following items:
 * <br> The color to use to display the value on the MS status picture.
 * This is a VJ color name ("beige", "bisque", etc) or a VJ DisplayOptions
 * color name, or a hex color string starting with "0x" (e.g., 0xff00ff).
 * <br> The X and Y positions of the displayed string on the status picture,
 * measured from the top-left of the picture to the left-baseline of the
 * text, relative to a picture size of 1000x600.
 * <br> The command string to send to the MS to get the value back,
 * for example, "?qoffset(1):cr".
 * <br> The value of the status as a double.  For boolean status items,
 * test for value != 0.
 */
public class MsStatus {
    private static Map<String, Record> table = new HashMap<String, Record>();

    /**
     * Put in a key,command pair with no display specified.
     */
    public static void put(String key, String cmd, String fmt) {
        put(key, cmd, "", 0, 0, fmt);
    }

    /**
     * Put in a key,command pair with display specifications.
     */
    public static void put(String key, String cmd,
                           String color, int x, int y, String fmt) {
        table.put(key, new Record(key, cmd, color, x, y, fmt));
    }

    public static Record get(String key) {
        return table.get(key);
    }

    /**
     * Returns true if the table contains the given key.
     */
    public static boolean containsKey(String key) {
        return table.containsKey(key);
    }

    public static Collection<Record> values() {
        return table.values();
    }

    public static double getValue(String key) {
        double value = Double.NaN;
        Record rec = get(key);
        if (rec != null) {
            value = rec.value;
        }
        return value;
    }

    public static boolean setValue(String key, double value) {
        Record rec = get(key);
        if (rec == null) {
            return false;
        } else {
            rec.setValue(value);
            return true;
        }
    }

    public static String getText(String key) {
        String rtn = null;
        Record rec = get(key);
        if (rec != null) {
            rtn = rec.text;
        }
        return rtn;
    }

    /**
     * Parses a "<key> <value>" status string to put the "value" in
     * the "key" entry of the MsStatus table.
     * All the "values" are numerical.
     */
    public static void setStatusValueInTable(String text) {
        StringTokenizer toker = new StringTokenizer(text);
        if (toker.countTokens() >= 2) {
            String key = toker.nextToken();
            String value = toker.nextToken();
            MsStatus.Record rec = MsStatus.get(key);
            if (rec != null) {
                double dVal = 0;
                try {
                    dVal = Double.parseDouble(value);
                } catch (NumberFormatException nfe) {
                    Messages.postDebug("MsCorbaClient.retrieveMessage(): "
                                       + "Key \"" + key
                                       + "\" has non-numeric value: " + value);
                }
                rec.setValue(dVal);
                rec.setValueString(value);
            }
        }
    }

    public static class Record {
        public String key;
        public String cmd;
        public Color color;
        public double x;
        public double y;
        public DecimalFormat fmt = null;
        public String strFmt = null;
        public double value;
        public String valueString;
        public String text = null;

        public Record(String key, String cmd,
                      String color, int x, int y, String strFmt) {
            this.key = key;
            this.cmd = cmd;
            this.color = DisplayOptions.getColor(color);
            this.x = x;
            this.y = y;
            this.value = 0;
            this.strFmt = strFmt;
            int idx;
            if (strFmt != null) {
                if (strFmt.startsWith(";")) {
                    // This is an index list of ";" separated labels;
                    // if value is n, label is n'th token.
                    this.fmt = null;
                } else {
                    if ((idx = strFmt.indexOf('-')) >= 0) {
                        // Need to make separate positive and negative patterns
                        // to handle the minus sign.
                        StringBuffer sbFmt = new StringBuffer(strFmt);
                        sbFmt.append(";");
                        sbFmt.append(strFmt);
                        sbFmt.deleteCharAt(idx);
                        strFmt = sbFmt.toString();
                        Messages.postDebug("MsStatusFormat",
                                           "key=" + key
                                           + ", pattern=" + strFmt);
                    }
                    try {
                        this.fmt = new DecimalFormat(strFmt);
                    } catch (IllegalArgumentException iae) {
                        Messages.postDebug("Bad format pattern:" + strFmt);
                    }
                }
                setText();
            }
        }

        public void setValue(double value) {
            this.value = value;
            setText();
            Messages.postDebug("MsStatusFormat",
                               "key=" + key + ", text=" + text);
        }

        public void setValueString(String v) {
            this.valueString = v;
        }

        public String getValueString() {
            return valueString;
        }

        private void setText() {
            if (fmt == null) {
                if (strFmt == null) {
                    text = "***";
                } else {
                    int iVal = (int)value;
                    StringTokenizer toker = new StringTokenizer(strFmt, ";");
                    /*Messages.postDebug("*** iVal=" + iVal
                                       + ", strFmt=" + strFmt
                                       + ", nTokens="
                                       + toker.countTokens());/*CMP*/
                    if (iVal < 1 || iVal > toker.countTokens()) {
                        text = "***";
                    } else {
                        for (int i = 0; i < iVal; ++i) {
                            text = toker.nextToken();
                        }
                    }
                }
            } else {
                text = fmt.format(value);
                // Sometimes suffix gets put in twice (Java bug?)  Fix it.
                // Example: fmt="0.0E0 Torr" text="1.23e-6 Torr Torr"
                String sfx = fmt.getPositiveSuffix();
                if (sfx.length() > 0) {
                    int i = text.indexOf(sfx);
                    int j = 0;
                    if (i >= 0 && (j = text.indexOf(sfx, i + 1)) > 0
                        && j + sfx.length() == text.length())
                    {
                        text = text.substring(0, j);
                    }
                }
                Messages.postDebug("MsStatusFormat",
                                   "key=" + key
                                   + ", text=" + this.text
                                   + ", suffix='"
                                   + this.fmt.getPositiveSuffix() + "'");
            }
        }
    }
}

