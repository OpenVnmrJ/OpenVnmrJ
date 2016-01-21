/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;


/**
 * This class has functions (i.e., static methods) that serve as
 * general utilities for operating on Strings.
 *
 */
public class StrUtil {

    /**
     * Maximum length of Vnmr strings.
     * One less than Vnmr's string buffer size in C.
     */
    public final static int MAXSTR = 255;


    /**
     * A private constructor; nobody can make an instance.
     */
    private StrUtil() {}

    /**
     * Split a String into pieces of length "maxlen".
     * @param input The string to split up.
     * @param maxlen The maximum length of each piece of the result.
     * @return An array of strings, each no longer than "maxlen".
     */
    public static String[] split(String input, int maxlen) {
        if (input.length() == 0) {
            return new String[0];
        }
        // Number of pieces of output:
        int n = (input.length() + maxlen - 1) / maxlen;
        String[] rtn = new String[n];
        for (int i = 0; i < n; i++) {
            int begin = i * maxlen;
            int end = Math.min(input.length(), begin + maxlen);
            rtn[i] = input.substring(begin, end);
        }
        return rtn;
    }

    /**
     * Escape all problematic characters in a string.
     * Problematic characters are defined as all characters with codes
     * outside the range 32-127, plus "&", "'" (single quote),
     * "\"" (double quote), "\\" (backslash), and ";".
     * (The semicolon is included because VnmrBG uses it to
     * separate array elements when sending arrayed string parameters
     * to VJ.)  The escape sequence is "&#ddd.", where ddd is the
     * decimal representation of the characters's Unicode value. This
     * is like the HTML/XML escape sequence, except that it is
     * terminated with "." instead of ";" (see note above about
     * semicolons).
     * @param text The string to be cleaned up.
     * @return An equivalent string with problematic characters replaced
     * with escape sequences.
     */
    public static String insertEscapes(String text) {
        StringBuffer sbText = new StringBuffer(text);
        boolean isSpecialChars = false;
        for (int i = sbText.length() - 1; i >= 0; --i) {
            char ch = sbText.charAt(i);
            if (ch == '&' || ch == '\'' || ch == '"' || ch == '\\' || ch == ';'
                || ch < 32 || ch >= 127)
            {
                sbText.replace(i, i+1, "&#" + (int)ch + ".");
                isSpecialChars = true;
            }
        }
        if (isSpecialChars) {
            text = sbText.toString();
        }
        return text;
    }

    /**
     * Decode all escaped characters in a string.
     * @param text The text to be decoded in a StringBuffer.
     * @return The interpreted text as a String.
     * @see #insertEscapes(String)
     */
    public static String decodeEscapes(StringBuffer text) {
        int k = text.lastIndexOf("&#");
        if (k > 0) {
            for ( ; k > 0; k = text.lastIndexOf("&#", k - 1)) {
                int j = text.indexOf(".", k);
                int i = k + 2;
                if (j > i) {
                    int chr = Integer.parseInt(text.substring(i, j));
                    text.replace(k, j + 1, String.valueOf((char)chr));
                }
            }
        }
        return text.toString();
    }
}
