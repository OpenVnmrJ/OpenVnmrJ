/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;

import java.util.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.text.*;

import javax.swing.SwingUtilities;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.ui.shuf.*;


/********************************************************** <pre>
 * Summary: "Message central" utility class.
 *
 * - processes Vnmrbg "line3" and "error" messages (thru ExpPanel)
 * - processes vnmrj java class messages
 *
 *   vnmrj API methods
 *
 *   1. static void postMessage(int type, String msg)
 *
 *       type=GROUP|TYPE|OUPUT
 *
 *       GROUP specifies the message category:
 *          ACQ      Acquisition messages
 *          OTHER    Other messages (default)
 *
 *       TYPE specifies the message type:
 *          ERROR    an error (critical)
 *          WARN     an error (non-critical)
 *          INFO     general information
 *
 *       OUPTUT routes messages to one or more of the following:
 *          MBOX     HardwareBar MessageBox
 *          CLI      Command Line area
 *          STDOUT   vnmrj startup command window
 *          STDERR   Console window
 *          LOG      vnmrj log file (~/vnmrsys/VnmrjMsgLog)
 *
 *
 *   Convienience Methods
 *
 *   2. static void postError(String msg)
 *          calls postMessage(OTHER|ERROR|MBOX|LOG,msg)
 *
 *   3. static void postWarning(String msg)
 *          calls postMessage(OTHER|WARN|MBOX|LOG,msg)
 *
 *   4. static void postInfo(String msg)
 *          calls postMessage(OTHER|MBOX,msg)
 *
 *   5. static void postDebug(String s)
 *          calls postMessage(OTHER|DEBUG|LOG|STDOUT,s)
 *
 *   6. static void writeStackTrace(Exception e, String str)
 *          Outputs str to VnmrjMsgLog log file followed by
 *          e.printStackTrace to the log file.
 *
 *   7. static void writeStackTrace(Exception e)
 *          Outputs e.printStackTrace to the VnmrjMsgLog log file.
 *
 *   vnmrjBg API methods
 *
 *   1. static void postMessage(String msg)  <see method description>
 *
 </pre> **********************************************************/
public class  Messages
{
    // code masks passed to postMessage

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

    private  static String logfilename = "VnmrjMsgLog";
    private static SimpleDateFormat dateFmt
        = new SimpleDateFormat("MMM d, yyyy HH:mm:ss");

    private static Hashtable<String, Integer> mcodes = null;

    protected static java.util.List<MessageListenerIF> messageListenerList
         = Collections.synchronizedList(new LinkedList<MessageListenerIF>());

    /**
     * Set the name of the log file
     * @param s The name.
     */
    public static void setLogFileName(String s){
        logfilename=s;
    }

    public static String getLogFileName()
    {
        return logfilename;
    }

    private static VLabelResource messageResource = null;
    public static String getMessageString(String str) {
        if(messageResource == null) messageResource = new VLabelResource("messageResources");

        if(messageResource == null) return str;
        else return messageResource.getString(str);
    }


    /**<pre> message handler for "line3" (Vnmrbg) messages.
     *  - decodes a two character type String from a message to
     *    generate a Message type value.
     *
     *   syntax: postMessage(String str)
     *   String str=code+" "+msg
     *
     *   code     Message type
     *   -------------------------
     *     ai     CLI|MBOX|ACQ|INFO
     *     aw     CLI|MBOX|ACQ|WARN
     *     ae     CLI|MBOX|ACQ|ERROR|LOG
     *     oi     CLI|MBOX|OTHER|INFO
     *     ow     CLI|MBOX|OTHER|WARN
     *     oe     CLI|MBOX|OTHER|ERROR|LOG
     *     oc     CLI|     OTHER|ERROR|LOG
     *   -------------------------</pre>
     * @param s The message.
     */
    public static void postMessage(String s){
        s = s.trim();
        if(s == null || s.length() == 0)
            return;

        if(mcodes==null){
            initCodeTable();
        }
        if (s.indexOf("\\n") >= 0) {
            // Expand explicit newlines
            s = s.replaceAll("\\\\n", "\n");
        }
        StringTokenizer tok = new StringTokenizer(s);
        if(tok.hasMoreTokens()) {
            String key=tok.nextToken();
            if(mcodes.containsKey(key)){
                Integer itype = mcodes.get(key);
                postMessage(itype.intValue(),s.substring(key.length()));
            }
        }
    }

    private static void initCodeTable() {
        mcodes = new Hashtable<String, Integer>();
        mcodes.put("ai", new Integer(CLI|MBOX|ACQ|INFO));
        mcodes.put("aw", new Integer(CLI|MBOX|ACQ|WARN));
        mcodes.put("ae", new Integer(CLI|MBOX|ACQ|ERROR|LOG));
        mcodes.put("oi", new Integer(CLI|MBOX|OTHER|INFO));
        mcodes.put("ow", new Integer(CLI|MBOX|OTHER|WARN));
        mcodes.put("oe", new Integer(CLI|MBOX|OTHER|ERROR|LOG));
        mcodes.put("oc", new Integer(CLI|OTHER|ERROR|LOG));
    }

    public static void setLogWarnings(boolean b) {
        if (mcodes == null) {
            initCodeTable();
        }
        if (b) {
            mcodes.put("aw", new Integer(CLI|MBOX|ACQ|WARN|LOG));
            mcodes.put("ow", new Integer(CLI|MBOX|OTHER|WARN|LOG));
        } else {
            mcodes.put("aw", new Integer(CLI|MBOX|ACQ|WARN));
            mcodes.put("ow", new Integer(CLI|MBOX|OTHER|WARN));
        }
    }

    public static void setLogInfos(boolean b) {
        if (mcodes == null) {
            initCodeTable();
        }
        if (b) {
            mcodes.put("ai", new Integer(CLI|MBOX|ACQ|INFO|LOG));
            mcodes.put("oi", new Integer(CLI|MBOX|OTHER|INFO|LOG));
        } else {
            mcodes.put("ai", new Integer(CLI|MBOX|ACQ|INFO));
            mcodes.put("oi", new Integer(CLI|MBOX|OTHER|INFO));
        }
    }

    /**
     * General message handler for vnmrj messages
     * @param type Bitmap specfying where to show the message.
     * @param s The message.
     */
    public static void postMessage(int type, String s){
        s = s.trim();
        if(s == null || s.length() == 0)
            return;

        s = getMessageString(s);

        synchronized(messageListenerList){
            VMessage m=new VMessage(s,type);

            // During exiting, do not write to MBOX, it is not stable.
            boolean exiting = ExitStatus.exiting();
            if((type & MBOX) != 0 && exiting)
                type |= STDERR;
            else {
                if((type & MBOX) != 0) {
                    if(messageListenerList.size()>0)
                        updateMessageListenersInEventThread(m);
                    else {
                        // If no MBox, output to STDERR
                        type |= STDERR;
                    }
                }
            }
            String mstr=messageString(m);
            if((type & STDOUT) != 0)
                System.out.println(mstr);
            if((type & STDERR) != 0)
                System.err.println(mstr);
            if((type & LOG) != 0)
                MLogger.write(mstr);
        }
    }

    /** convienience routine for posting vnmrj errors
     * @param s The message
     */
    public static void postError(String s){
        postMessage(OTHER|ERROR|MBOX|LOG,s);
        if(DebugOutput.isSetFor("StackTrace")) {
            postMessage(OTHER|DEBUG|STDOUT,s);
        }
        // If no listerner yet and not in managedb, try delaying once.
        if(messageListenerList.size() == 0 && !FillDBManager.managedb) {
            postDelayedError(s);
        }

    }

    public static void postErrorAfterDelay(String s){
        // Don't log again
        postMessage(OTHER|ERROR|MBOX,s);
        if(DebugOutput.isSetFor("StackTrace")) {
            postMessage(OTHER|DEBUG|STDOUT,s);
        }
    }
    public static void postWarningAfterDelay(String s){
        // Don't log again
        postMessage(OTHER|WARN|MBOX,s);
        if(DebugOutput.isSetFor("StackTrace")) {
            postMessage(OTHER|DEBUG|STDOUT,s);
        }
    }

    /** convienience routine for loging vnmrj errors
     * @param s The message
     */
    public static void logError(String s){
        postMessage(OTHER|ERROR|LOG,s);
        if(DebugOutput.isSetFor("StackTrace")) {
            postMessage(OTHER|DEBUG|STDOUT,s);
        }
    }

    /** convienience routine for posting vnmrj warnings
     * @param s The message
     */
    public static void postWarning(String s){
        postMessage(OTHER|WARN|MBOX|LOG,s);
        // If no listerner yet and not in managedb, try delaying once.
        if(messageListenerList.size() == 0 && !FillDBManager.managedb) {
            postDelayedWarning(s);
        }
    }

    /** convienience routine for loging vnmrj warnings
     * @param s The message
     */
    public static void logWarning(String s){
        postMessage(OTHER|WARN|LOG,s);
    }

    /** convienience routine for posting vnmrj info messages
     * @param s The message
     */
    public static void postInfo(String s){
        postMessage(OTHER|INFO|MBOX,s);
    }

     /** convienience routine for posting vnmrj debug messages
     * @param s The message
     */
    public static void postDebug(String s){
        postMessage(OTHER|DEBUG|LOG|STDOUT,s);
    }

     /** convienience routine for posting vnmrj debug messages to log only
     * @param s The message
     */
    public static void postLog(String s){
        postMessage(OTHER | DEBUG | LOG, s);
    }

    /**
     * Write a message only if the specified debug category is on.
     * The message is prefixed with word "DEBUG" followed by the
     * date and time to the nearest second.
     * @param category The debug category.
     * @param s The message.
     * @see DebugOutput#isSetFor(String)
     */
    public static void postDebug(String category, String s) {
        if (DebugOutput.isSetFor(category)) {
            postMessage(OTHER|DEBUG|LOG|STDOUT, s);
        }
    }

    /**
     * Log a message only if the specified debug category is on.
     * The message is prefixed with word "DEBUG" followed by the
     * date and time to the nearest second.
     * @param category The debug category.
     * @param s The message to write.
     * @see DebugOutput#isSetFor(String)
     */
    public static void postLog(String category, String s) {
        if (DebugOutput.isSetFor(category)) {
            postMessage(OTHER | DEBUG | LOG, s);
        }
    }

    /** Prepend a message in front of a stack trace, then write the trace
     * @param e The exception associated with the trace.
     * @param str The message
     */
    static public void writeStackTrace(Exception e, String str) {
        postMessage(OTHER|ERROR|LOG, str);
        if(DebugOutput.isSetFor("StackTrace")) {
            postMessage(OTHER|DEBUG|STDOUT,str);
        }

        writeStackTrace(e);
    }

    /** write the stack trace to the log file
     * @param e The exception associated with the trace.
     */
    static public void writeStackTrace(Exception e) {
        FileWriter fileWriter;
        PrintWriter printWriter;
        String dir;
        String filepath = "";
        boolean append = true;

        Class<? extends Exception> classType = e.getClass();
        String className = classType.getName();
        if(className.indexOf("SocketException") != -1) {
            // If SocketException, don't try to output any errors.  We are
            // probably shutting down, but none the less, we will likely
            // fail at outputting the error giving just more errors.
            return;
        }



        try {
            // Create the log file dir path and name
            dir = FileUtil.usrdir();
            filepath = new String(dir + File.separator + logfilename);

            UNFile file = new UNFile(filepath);

            // If the file does not exist, this will create it.
            // If it exists, it will be opened for appending.
            fileWriter = new FileWriter(file, append);
        }
        catch (Exception ex) {
            // ** Do Not Call the standard error handling which in turn
            //    calls this Logger.  Else we will get into an endless loop.
            System.out.println("ERROR: Problem creating or opening\n    " +
                               filepath);
            ex.printStackTrace();
            System.err.println("We had the above error while trying to print" +
                               " out the following error");
            e.printStackTrace();
            return;
        }

        try {
            printWriter = new PrintWriter(fileWriter);
            e.printStackTrace(printWriter);
            printWriter.close();
        }
        catch (Exception ex) {
            // ** Do Not Call the standard error handling which in turn
            //    calls this Logger.  Else we will get into an endless loop.
            System.err.println("ERROR: Problem writing to\n    " + filepath);
            ex.printStackTrace();
            System.err.println("We had the above error while trying to print" +
                               " out the following error");
            e.printStackTrace();

            return;
        }

        // Allow programmers to have all stack traces come out in the
        // startup window.
        if(DebugOutput.isSetFor("StackTrace")) {
            System.err.println("** Debug Stack Trace **");
            e.printStackTrace();
        }

    }


    /**
     * Get a type code String
     * @param m The VMessage to examine.
     * @return A String expressing the type of the given message.
     */
    public static String typeString(VMessage m){
        String s="";
        if((m.type & GROUP)==ACQ)
            s="ACQ ";
        if((m.type & TYPE)==ERROR)
            s+="ERROR ";
        else if((m.type & TYPE)==WARN)
            s+="WARNING ";
        else if((m.type & TYPE)==DEBUG)
            s+="DEBUG ";
        else
            s+="INFO  ";
        return s;
    }

    /**
     * Set the precision of the date/time string.  Supports printing
     * to 1 ms, to 1 s, or to 1 minute precision.
     * @param precision The approximate desired precision in ms.
     */
    public static void setTimePrecision(int precision) {
        if (precision < 1000) {
            dateFmt = new SimpleDateFormat("MMM d, yyyy HH:mm:ss.SSS");
        } else if (precision < 1000 * 60) {
            dateFmt = new SimpleDateFormat("MMM d, yyyy HH:mm:ss");
        } else {
            dateFmt = new SimpleDateFormat("MMM d, yyyy HH:mm");
        }
    }

    /** return message String including type code String and date
     * @param m The VMessage to process.
     * @return The message in string form.
     */
    public static String messageString(VMessage m){
        String s=typeString(m);
        // Format the date
        s += dateFmt.format(m.date);
        s+=" ";
        s+=m.msg;
        return s;
    }

    /**
     * Print a warning message iff it is not canceled before a specified time.
     * Returns a javax.swing.Timer object that controls the message.
     * Call timer.stop() to cancel the message.
     * <br>
     * The intended usage is like:<pre>
     * timer = Messages.setTimeout(200, "Something is hung up");
     * [ ... do "something" ... ]
     * timer.stop();
     * </pre>
     * Note that this is good for debugging, but not great for
     * production code, as the thread remains hung up.
     * @param delay How long to wait before printing the message (ms).
     * @param msg The message.
     * @return The timer that will trigger the printing of the message.
     */
    public static javax.swing.Timer setTimeout(int delay, String msg) {
        final String message = msg;
        ActionListener msgPrinter = new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                postWarning(message);
            }
        };
        javax.swing.Timer timer;
        timer = new javax.swing.Timer(delay, msgPrinter);
        timer.setRepeats(false);
        timer.start();
        return timer;
    }

    /**
     * Send new message to all listeners. Offloads task to
     * Event Thread if necessary.
     */
    private static void updateMessageListenersInEventThread(VMessage m) {
        if (EventQueue.isDispatchThread()) {
            updateMessageListeners(m);
        } else {
            SwingUtilities.invokeLater(new GuiUpdater(m));
        }
    }

    /**
     * Send new message to all listeners
     */
    protected static void updateMessageListeners(VMessage m) {
        synchronized(messageListenerList) {
            Iterator<MessageListenerIF> itr = messageListenerList.iterator();
            while (itr.hasNext()) {
                MessageListenerIF item = itr.next();
                item.newMessage(m.type,m.msg);
            }
        }
    }

    /** register a message listener
     * @param item The listener to add.
     */
    public static void addMessageListener(MessageListenerIF item){
        if (!messageListenerList.contains(item))
            messageListenerList.add(item);
    }

    /** unregister a message listener
     * @param item The listener to add.
     */
    public static void removeMessageListener(MessageListenerIF item){
        if (messageListenerList.contains(item))
            messageListenerList.remove(item);
    }


   /**********************************************************
    * private static class MLogger
    * Summary: Append to and maintain a log file for errors, warnings etc.
    *
    * - Prepend the date and time to the message
    * - Open the log file for appending and append the message
    * - Check log file size to see if it is too big
    * - If too big, keep only the newest part of the file,
    *   trimming it by at least a factor of 2.
    * - Create a new file
    * - Read the last part of the original file
    * - Write the last part to the new file
    * - Rename to the new file to the original files name thus replacing it
    **********************************************************/
  static class MLogger {

    static public void write(String msg) {
        FileWriter fileWriter;
        FileReader fileReader;
        String dir;
        String filepath = "";
        boolean append = true;
        int numCharRead;
        int charRead;

        try {
            // Create the log file dir path and name
            dir = FileUtil.usrdir();
            filepath = new String(dir + "/" + logfilename);
            UNFile file = new UNFile(filepath);

            // If the file does not exist, this will create it.
            // If it exists, it will be opened for appending.
            fileWriter = new FileWriter(file, append);
        }
        catch (IOException e) {
            // ** Do Not Call the standard error handling which in turn
            //    calls this Logger.  Else we will get into an endless loop.
            System.out.println("ERROR: Problem creating or opening " +filepath);
            return;
        }

        try {
             fileWriter.write("## "+msg +"\n");
             fileWriter.close();

             int sizeLimit;
             if(DebugOutput.isSetFor("bigLogFile"))
                 sizeLimit = 10000000;
             else
                 sizeLimit = 1000000;
            // Check the size of the log file
            File file = new File(filepath);
            long length = file.length();
            // If too big, trim down to less than the limit
            if (length > sizeLimit) {
                File newfile = new File(filepath + ".new");
                sizeLimit = (int)Math.min(sizeLimit, length / 2);
                long skipLength = length - sizeLimit + 3;
                fileWriter.close();
                // Create a new file to hold the part we'll keep
                fileWriter = new FileWriter(newfile);

                // Open the log file for reading
                fileReader = new FileReader(filepath);
                // Skip over the first part of the file
                fileReader.skip(skipLength);
                // create a buffer to read into
                char[] buf = new char[sizeLimit];

                // Read in characters until we reach the beginning of a
                // message as designated by ##.  Then start filling the
                // buffer with the rest of the file.
                while(true) {
                    charRead = fileReader.read();
                    if(charRead == '#') {
                        charRead = fileReader.read();
                        if(charRead == '#') {
                            // Found 2 # signs in a row.  This must be the
                            // beginning of a new message.
                            // Write the 2 # to the new file, then continue
                            // with reading and writing from the old file to
                            // the new one.
                            fileWriter.write('#');
                            fileWriter.write('#');
                            break;
                        }
                    }
                    // If end of file, continue.
                    if(charRead == -1)
                        break;
                }
                // read last part into buffer
                numCharRead = fileReader.read(buf, 0, sizeLimit);
                // Write whatever was read, into new file
                fileWriter.write(buf, 0, numCharRead);

                fileReader.close();
                fileWriter.close();

                // Move the new trimmed file in place of the original one
                newfile.renameTo(file);
            }
        }
        catch (IOException e) {
            // ** Do Not Call the standard error handling which in turn
            //    calls this Logger.  Else we will get into an endless loop.
            System.out.println("ERROR: Problem writing to " + filepath +
                               ".\n" + e);
        }
    }
  } // class MLogger



    public static void postDelayedError(String errString) {

        DelayedErrorThread det = new DelayedErrorThread(errString);
        det.setName("Delayed Error Output");
        det.start();

    }

    public static void postDelayedWarning(String errString) {

        DelayedWarningThread det = new DelayedWarningThread(errString);
        det.setName("Delayed Warning Output");
        det.start();

    }
} // class Messages



class DelayedErrorThread extends Thread {
    String errString;

    public DelayedErrorThread(String errString) {
        this.errString = errString;
    }

    public void run() {

        try {
            sleep(20000);
            Messages.postErrorAfterDelay(errString);
        }
        catch (Exception e) {}
    }
}

class DelayedWarningThread extends Thread {
    String errString;

    public DelayedWarningThread(String errString) {
        this.errString = errString;
    }

    public void run() {

        try {
            sleep(25000);
            Messages.postWarningAfterDelay(errString);
        }
        catch (Exception e) {}
    }
}

/**
 * Sends a message to everyone in the messageListenerList.
 * Work is done in the Event Thread.
 */
class GuiUpdater implements Runnable {
    VMessage mm_msg;

    GuiUpdater(VMessage msg) {
        mm_msg = msg;
    }

    public void run() {
        Messages.updateMessageListeners(mm_msg);
    }
}

