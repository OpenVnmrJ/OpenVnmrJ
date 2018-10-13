/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.lc;


import java.awt.event.*;
import java.lang.reflect.*;
import java.util.*;

/**
 * A standalone alternative to using the VnmrJ "Messages" class.
 * If the vnmr.util.Messages class is present, uses reflection to call
 * the appropriate methods in that class.
 * Otherwise, just writes messages to stdout or stderr.
 */
public class  LcMsg {

    /** These are the same as in vnmr.util.Messages. */
    public final static int TYPE        = 0x000f;
    public final static int INFO        = 0x0001;
    public final static int WARN        = 0x0002;
    public final static int ERROR       = 0x0004;
    public final static int DEBUG       = 0x0008;

    public final static int GROUP       = 0x00f0;
    public final static int ACQ         = 0x0010;
    public final static int OTHER       = 0x0020;

    public final static int OUTPUT      = 0xff00;
    public final static int LOG         = 0x0100;
    public final static int MBOX        = 0x0200;
    public final static int CLI         = 0x0400;
    public final static int STDOUT      = 0x0800;
    public final static int STDERR      = 0x1000;


    private static Method m_isDebugSetFor = null;
    private static Method m_postMessage = null;
    private static Method m_writeStackTrace = null;
    private static boolean m_standalone;
    private static Set<String> m_categoryList = new TreeSet<String>();

    static {
        m_standalone = System.getProperty("sysdir") == null;
        if (!m_standalone) {
            try {
                // Should always be able to get these
                Class strClass = Class.forName("java.lang.String");
                Class[] argStr = {strClass};
                Class[] argIntStr = {Integer.TYPE, strClass};
                Class[] argException = {Class.forName("java.lang.Exception")};

                // Should usually get these
                Class debugOutput = Class.forName("vnmr.util.DebugOutput");
                m_isDebugSetFor = debugOutput.getMethod("isSetFor", argStr);

                // Will sometimes get these
                Class messages = Class.forName("vnmr.util.Messages");
                m_postMessage = messages.getMethod("postMessage", argIntStr);
                m_writeStackTrace = messages.getMethod("writeStackTrace",
                                                       argException);
            } catch (ClassNotFoundException cnfe) {
                m_standalone = true;
            } catch (NoSuchMethodException nsme) {
                m_standalone = true;
            }
        }
    }


    /** General routine for posting messages. */
    public static void postMessage(int type, String s) {
        if (m_standalone) {
            if ((type & (WARN | ERROR | STDERR)) != 0) {
                System.err.println(s);
            } else {
                System.out.println(s);
            }
        } else {
            Object[] args = {new Integer(type), s};
            try {
                m_postMessage.invoke(null, args);
            } catch (IllegalAccessException iae) {
                m_standalone = true;
            } catch (InvocationTargetException ite) {
                m_standalone = true;
            }
        }
    }

    public static void addCategory(String category) {
        m_categoryList.add(category);
    }

    public static void removeCategory(String category) {
        m_categoryList.remove(category);
    }

    public static boolean isSetFor(String category) {
        boolean printit = m_categoryList.contains(category);
        if (m_isDebugSetFor != null) {
            String[] args = {category};
            try {
                Object prnt = m_isDebugSetFor.invoke(null, (Object[])args);
                printit = ((Boolean)prnt).booleanValue();
            } catch (IllegalAccessException iae) {
                m_standalone = true;
            } catch (InvocationTargetException ite) {
                m_standalone = true;
            }
        }
        return printit;
    }


    /** convienience routine for loging errors */
    public static void logError(String s){
        postMessage(OTHER|ERROR|LOG,s);
    }

    /** convienience routine for posting errors */
    public static void postError(String s){
        postMessage(OTHER|ERROR|MBOX|LOG,s);
    }

    /** convienience routine for posting warnings */
    public static void postWarning(String s){
        postMessage(OTHER|WARN|MBOX|LOG,s);
    }

    /** convienience routine for loging warnings */
    public static void logWarning(String s){
        postMessage(OTHER|WARN|LOG,s);
    }

    /** convienience routine for posting info messages */
    public static void postInfo(String s){
        postMessage(OTHER|INFO|MBOX,s);
    }

     /** convienience routine for posting debug messages */
    public static void postDebug(String s){
        postMessage(OTHER|DEBUG|LOG|STDOUT,s);
    }

     /** convienience routine for posting debug messages to log only */
    public static void postLog(String s){
        postMessage(OTHER | DEBUG | LOG, s);
    }

    /**
     * Write a message only if the specified debug category is on.
     * The message is prefixed with word "DEBUG" followed by the
     * date and time to the nearest second.
     * @param category The debug category.
     * @param s The message to write.
     * @see vnmr.util.DebugOutput#isSetFor(String)
     */
    public static void postDebug(String category, String s) {
        if (LcMsg.isSetFor(category)) {
            postDebug(s);
        }
    }

    /**
     * Log a message only if the specified debug category is on.
     * The message is prefixed with word "DEBUG" followed by the
     * date and time to the nearest second.
     * @param category The debug category.
     * @param s The message to write.
     * @see vnmr.util.DebugOutput#isSetFor(String)
     */
    public static void postLog(String category, String s) {
        if (LcMsg.isSetFor(category)) {
            postLog( s);
        }
    }

   /** Prepend a message in front of a stack trace, then write the trace */
    static public void writeStackTrace(Exception e, String str) {
        postError(str);
        writeStackTrace(e);
    }

    /** write the stack trace to the log file */
    static public void writeStackTrace(Exception e) {
        if (m_standalone) {
            e.printStackTrace();
        } else {
            Object[] args = {e};
            try {
                m_writeStackTrace.invoke(null, args);
            } catch (IllegalAccessException iae) {
                m_standalone = true;
            } catch (InvocationTargetException ite) {
                m_standalone = true;
            }
        }
    }

    /**
     * Not implemented.
     */
    public static void setTimePrecision(int precision) {
    }

    /**
     * Print an error message iff it is not canceled within a specified time.
     * Returns a javax.swing.Timer object that controls the message.
     * Call timer.stop() to cancel the message.
     * <br>
     * The intended usage is like:<pre>
     * timer = LcMsg.setTimeout(200, "Something is hung up");
     * [ ... do "something" ... ]
     * timer.stop();
     * </pre>
     * Note that this is good for debugging (maybe), but not great for
     * production code, as the thread remains hung up.
     * @param delay How long to wait before printing the message (ms).
     * @param msg The message.
     */
    public static javax.swing.Timer setTimeout(int delay, String msg) {
        final String message = msg;
        ActionListener msgPrinter = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                postError(message);
            }
        };
        javax.swing.Timer timer;
        timer = new javax.swing.Timer(delay, msgPrinter);
        timer.setRepeats(false);
        timer.start();
        return timer;
    }

}
