/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.text.*;

/**
 * Class has functions (i.e., static methods) that serve as
 * general utilities.
 *
 */
public class Fmt {
    /*
     * Does not keep a class scope NumberFormat, because of the
     * the possibility of conflict between different threads.
     * Is it faster to create a local instance of NumberFormat (as here)
     * or to synchronize the method?
     */
    private static Locale locale = Locale.getDefault();

    public static void setLocale(Locale newLocale) {
        locale = newLocale;
    }

    /**
     * F format with "comma grouping" - print trailing zeros.
     */
    public static String f(int precision, double value) {
        return f(precision, value, true);
    }

    /**
     * F format with optional "comma grouping" - print trailing zeros.
     */
    public static String f(int precision, double value, boolean grouping) {
        return f(precision, value, grouping, true);
    }

    /**
     * Compact F format with no "comma grouping" - trim trailing zeros.
     */
    public static String fg(int precision, double value) {
        return f(precision, value, false, false);
    }

    /**
     * F format with optional "comma grouping" - optional trailing zeros.
     */
    public static String f(int precision, double value,
                           boolean grouping, boolean rightPad) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        NumberFormat fmt = NumberFormat.getNumberInstance(locale);
        fmt.setMaximumFractionDigits(precision);
        if (rightPad) {
            fmt.setMinimumFractionDigits(precision);
        } else {
            fmt.setMinimumFractionDigits(0);
        }
        fmt.setGroupingUsed(grouping);
        return fmt.format(value);
    }

    /**
     * All values in an array in F format.
     */
    public static String f(int precision, double[] values,
                           boolean grouping, boolean rightPad) {
        if (values == null) {
            return "NULL";
        } else {
            StringBuffer sbRtn = new StringBuffer("[");
            int len = values.length;
            for (int i = 0; i < len; i++) {
                sbRtn.append(f(precision, values[i], grouping, rightPad));
                if (i == len - 1) {
                    sbRtn.append("]");
                } else {
                    sbRtn.append(", ");
                }
            }
            return sbRtn.toString();
        }
    }

    /**
     * F format with minimum field width and optional "comma grouping"
     * - prints trailing zeros.
     */
    public static String f(int width, int precision,
                           double value, boolean grouping) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        NumberFormat fmt = NumberFormat.getNumberInstance(locale);
        fmt.setMaximumFractionDigits(precision);
        fmt.setMinimumFractionDigits(precision);
        fmt.setGroupingUsed(grouping);
        String str = fmt.format(value);
        int len = str.length();
        if (len < width) {
            char[] pfx = new char[width - len];
            for (int i = 0; i < pfx.length; i++) {
                pfx[i] = ' ';
            }
            str = new String(pfx) + str;
        }
        return str;
    }

    /**
     * Format a value as a decimal integer string.
     * @param width The minumum width in characters.
     * @param value The value to format.
     * @param grouping If true, use commas (or default grouping char) to
     * delimit thousands, millions, etc.
     * @param fill The character used to pad leading unused character positions.
     * @return The formatted result.
     */
    public static String d(int width, long value, boolean grouping, char fill) {
        String str = d(width, value, grouping);
        if (fill != '0') {
            char[] chars = new char[str.length()];
            str.getChars(0, str.length(), chars, 0);
            for (int i = 0; i < chars.length - 1; i++) {
                if (chars[i] == '0') {
                    chars[i] = fill;
                } else {
                    break;
                }
            }
            str = new String(chars);
        }
        return str;
    }

    /**
     * Format a value as a decimal integer string.
     * @param digits The minumum number of digits (pads with leading zeros
     * if appropriate).
     * @param value The value to format.
     * @param grouping If true, use commas (or default grouping char) to
     * delimit thousands, millions, etc.
     * @return The formatted result.
     */
    public static String d(int digits, long value, boolean grouping) {
        NumberFormat fmt = NumberFormat.getIntegerInstance(locale);
        digits = Math.max(1, digits);
        fmt.setMinimumIntegerDigits(digits);
        fmt.setGroupingUsed(grouping);
        return fmt.format(value);
    }

    /**
     * Format a value as a decimal integer string.  No grouping is done.
     * @param digits The minumum width of the field.  Pads with blanks
     * if appropriate; makes the field longer if the number is too long.
     * @param value The value to format.
     * @return The formatted result.
     */
    public static String d(int digits, long value) {
        return d(digits, value, false, ' ');
    }

    /**
     * Format a value in scientific notation
     * @param digits Number of digits after the decimal point.
     * @param value The number to format.
     * @return The string representation of the number.
     */
    public static String e(int digits, double value) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        NumberFormat fmt = NumberFormat.getNumberInstance(locale);
        if (fmt instanceof DecimalFormat) {
            DecimalFormat dfmt = (DecimalFormat)fmt;
            dfmt.applyPattern("0.0E0");
            dfmt.setMaximumFractionDigits(digits);
        }
        return fmt.format(value);
    }
 
    /**
     * Format a value in decimal or scientific notation, whichever is
     * more compact.
     * No grouping commas are used.
     * @param digits Number of significant digits.
     * @param value The number to format.
     * @return The string representation of the number.
     */
    public static String g(int digits, double value) {
        if (Double.isNaN(value)) {
            return "NaN";
        }
        NumberFormat fmt = NumberFormat.getNumberInstance(locale);
        fmt.setGroupingUsed(false);
        if (fmt instanceof DecimalFormat) {
            DecimalFormat dfmt = (DecimalFormat)fmt;
            dfmt.applyPattern("0.0E0");
            dfmt.setMaximumFractionDigits(digits - 1);
            String str1 = dfmt.format(value);

            dfmt.applyPattern("0.0");
            double log = Math.log(Math.abs(value)) / Math.log(10);
            int ilog = (int)log;
            if (ilog >= 1) {
                digits -= ilog;
            } else if (ilog <= -1) {
                digits -= ilog;
            }
            dfmt.setMaximumFractionDigits(digits);
            String str2 = dfmt.format(value);

            return str1.length() < str2.length() ? str1 : str2;
        }
//            double log = Math.log(Math.abs(value)) / Math.log(10);
//            if (log > (3 + digits) || -log > (1 + digits)) {
//                dfmt.applyPattern("0.0E0");
//                dfmt.setMaximumFractionDigits(digits - 1);
//            } else {
//                dfmt.applyPattern("0.0");
//                int ilog = (int)log;
//                if (ilog >= 1) {
//                    digits -= ilog;
//                } else if (ilog <= -1) {
//                    digits -= ilog;
//                }
//                dfmt.setMaximumFractionDigits(digits);
//            }
        return fmt.format(value);
    }

    /**
     * Replaces all control characters in an ASCII string with printable
     * escapes. ASCII Ctl-C is replaced with "^C", etc.
     * The output is meant read by a human, not a program; it is not
     * possible to reconstruct the input from the output.
     * @param msg A string with (perhaps) control characters in it.
     * @return A string with only printable characters.
     */
    public static String safeAscii(String msg) {
        StringBuffer sb = new StringBuffer(msg);
        for (int i = sb.length() - 1; i >= 0; i--) {
            char c = sb.charAt(i);
            if (c < 0x20 || c == 0x7f) {
                if (c == 0x7f) {
                    sb.setCharAt(i, '~');
                } else {
                    sb.setCharAt(i, (char)(c + 0x40)); // Make char printable
                }
                sb.insert(i, '^'); // Prepend a caret
            } else if (c > 0x7f) {
                c = (char)(c & 0x7f);
                if (c < 0x20) {
                    c = (char)(c + 0x40);
                } else if (c == 0x7f) {
                    c = '~';
                }
                sb.setCharAt(i, c);
            }
        }
        return sb.toString();
    }
 

    /**
     * For testing.
     */
    public static void main(String[] args) {
        if (args.length > 0) {
            double v = Double.parseDouble(args[0]);
            System.out.println(g(2, v));
        } else {
            for (double x = 123456; x > 1e-10; x /= 10) {
                System.out.println(g(2, x));
            }
            for (double x = -123456; x < -1e-10; x /= 10) {
                System.out.println(g(2, x));
            }
        }
    }
}
