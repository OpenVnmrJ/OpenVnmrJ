/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */
package vnmr.vjclient;

/**
 * This class prints debugging and error messages.
 * Written by Chris Price as part of ProTune.  Extricated to become
 * part of a general library for use with ProbeID.
 */
public class Messages implements VnmrIO {
    /** The current debug level. */
    private static int m_debugLevel = 0;

    /**
     * Unconditionally print a debug message.
     * @param msg The message.
     */
    public static void postDebug(String msg) {
        System.err.println(msg);
    }

    /**
     * Unconditionally print a debug message, optionally prepending the
     * current time.
     * @param msg The message to print.
     * @param isDetailed If true, the time in ms is prepended to the message.
     */
    public static void postDebug(String msg, boolean isDetailed) {
        if (isDetailed) {
            msg = addDetailTo(msg);
        }
        postDebug(msg);
    }

    /**
     * Prepend the appropriate detail to the given message.
     * @param msg The plain message.
     * @return The fancy message.
     */
    private static String addDetailTo(String msg) {
        return System.currentTimeMillis() + ": " + msg;
    }

    /**
     * Print a debug message if the debug level is set high enough.
     * @param level Bitmap of msg types that this message matches.
     * @param msg The message.
     */
    public static void postDebug(int level, String msg) {
        boolean printit = (m_debugLevel >= (DEBUG_LEVEL_MASK & level));
        if (printit || (~DEBUG_LEVEL_MASK & level) != 0) {
            postDebug(msg);
        }
    }

    /**
     * Print a debug message if the given debug category is set.
     * @param category The category of this message.
     * @param msg The message.
     */
    public static void postDebug(String category, String msg) {
        //if (DebugOutput.isSetFor(category)) {
            postDebug(msg, DebugOutput.isDetailed(category));
        //}
    }

    public static void printDebug(int level, String msg) {
        boolean printit = (m_debugLevel >= (DEBUG_LEVEL_MASK & level));
        if (printit || (~DEBUG_LEVEL_MASK & level) != 0) {
            System.out.print(msg);
        }
    }
    
    /**
     * Print an error message to the debug log.
     * @param msg The message.
     */
    public static void postDebugError(String msg) {
        System.err.println("<APT-ERROR>: " + msg);
    }

    /**
     * Post an informational message and record it in the debug log.
     * @param msg The message.
     */
    public static void postInfo(String msg) {
        postDebug(msg);  // Also send to debug log
        msg = msg.replaceAll("'", "\"");
        VnmrProxy.sendToVnmr(VJ_INFO_MSG + "write('line3', '" + msg + "')");
    }

    /**
     * Post a warning message and record it in the debug log.
     * @param msg The message.
     */
    public static void postWarning(String msg) {
        postWarning(false, msg);
    }

    /**
     * Post a warning message and record it in the debug log.
     * @param wantBeep If true, VnmrJ will beep if beeping is enabled.
     * @param msg The message.
     */
    public static void postWarning(boolean wantBeep, String msg) {
        msg = "Warning: " + msg;
        postDebug(msg);  // Also send to debug log
        msg = msg.replaceAll("'", "\"");
        if (wantBeep) {
            VnmrProxy.sendToVnmr(VJ_ERR_MSG + "write('error', '" + msg + "')");
        } else {
            VnmrProxy.sendToVnmr(VJ_INFO_MSG + "write('line3', '" + msg + "')");
        }
    }

    /**
     * Unconditionally print an error message.
     * @param msg The message to display.
     */
    public static void postError(String msg) {
        System.err.println(msg);
        msg = msg.replaceAll("'", "\"");
        VnmrProxy.sendToVnmr(VJ_ERR_MSG + "write('error', '" + msg + "')");
    }

    /**
     * Post the stack trace for the given exception to the debug log.
     * @param e The exception.
     */
    static public void postStackTrace(Exception e) {
        StackTraceElement[] trace = e.getStackTrace();
        for (StackTraceElement elem : trace) {
            postDebugError("   " + elem);
        }
    }
}
