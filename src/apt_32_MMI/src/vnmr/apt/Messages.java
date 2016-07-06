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


package vnmr.apt;


/**
 * This class prints debuging and error messages.
 */
public class Messages implements AptDefs {

    /** The current debug level. */
    private static int m_debugLevel = 0;
    private static String m_lastWarning;
    private static String m_lastError;
    private static String m_lastDebugWarning;
    private static String m_lastDebugError;


    /**
     * Unconditionally print a debug message.
     * @param msg The message.
     */
    public static void postDebug(String msg) {
        System.err.println("<APT>: " + msg);
    }

    /**
     * Print a warning message to the debug log.
     * @param msg The message.
     */
    public static void postDebugWarning(String msg) {
        if (!msg.equals(m_lastDebugWarning)) {
            m_lastDebugWarning = msg;
            System.err.println("<APT-WARNING>: " + msg);
        }
    }

    /**
     * Print an info message to the debug log.
     * @param msg The message.
     */
    public static void postDebugInfo(String msg) {
        System.err.println("<APT-INFO>: " + msg);
    }

    /**
     * Print an error message to the debug log.
     * @param msg The message.
     */
    public static void postDebugError(String msg) {
        if (!msg.equals(m_lastDebugError)) {
            m_lastDebugError = msg;
            System.err.println("<APT-ERROR>: " + msg);
        }
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
        if (DebugOutput.isSetFor(category)) {
            postDebug(msg, DebugOutput.isDetailed(category));
        }
    }

    public static void printDebug(int level, String msg) {
        boolean printit = (m_debugLevel >= (DEBUG_LEVEL_MASK & level));
        if (printit || (~DEBUG_LEVEL_MASK & level) != 0) {
            System.out.print(msg);
        }
    }

    /**
     * Post an informational message and record it in the debug log.
     * @param msg The message.
     */
    public static void postInfo(String msg) {
        postDebugInfo(msg);  // Also send to debug log
        msg = msg.replaceAll("'", "\"");
        ProbeTune.sendToVnmr(VJ_INFO_MSG + "write('line3', '" + msg + "')");
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
        if (msg == null || msg.length() == 0) {
            return;
        }
        if (!msg.equals(m_lastWarning)) {
            m_lastWarning = msg;
            postDebugWarning(msg);  // Also send to debug log
            msg = "Warning: " + msg;
            msg = msg.replaceAll("'", "\"");
            if (wantBeep) {
                ProbeTune.sendToVnmr(VJ_ERR_MSG + "write('error', '" + msg + "')");
            } else {
                ProbeTune.sendToVnmr(VJ_INFO_MSG + "write('line3', '" + msg + "')");
            }
        }
    }

    /**
     * Print an error message.
     * @param msg The message to display.
     */
    public static void postError(String msg) {
        if (msg == null || msg.length() == 0) {
            return;
        }
        if (!msg.equals(m_lastError)) {
            m_lastError = msg;
            postDebugError(msg);  // Also send to debug log
            if (!ProbeTune.isCancel()) {
                String vjmsg = msg.replaceAll("'", "\"");
                ProbeTune.sendToVnmr(VJ_ERR_MSG
                                     + "write('error', '" + vjmsg + "')");
            }
        }
    }

    /**
     * Post the stack trace for the given exception to the debug log.
     * @param e The exception.
     */
    static public void postStackTrace(Exception e) {
        StackTraceElement[] trace = e.getStackTrace();
        for (StackTraceElement elem : trace) {
            Messages.postDebugError("   " + elem);
        }
    }

}
