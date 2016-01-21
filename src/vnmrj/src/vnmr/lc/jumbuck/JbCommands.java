/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;

import java.util.*;
import java.io.*;
import java.lang.reflect.InvocationTargetException;
import java.net.SocketTimeoutException;
import javax.imageio.stream.*;
import javax.swing.SwingUtilities;

import vnmr.util.Fmt;
import vnmr.lc.LcMsg;


/**
 * This class holds a list of all the commands and replies for the 335
 * PDA.  The commands are kept in a Map keyed off the command ID
 * codes.  The entry for each command includes the information about
 * parameter ordering that is needed to use the command.  Also
 * contains methods to read and write commands.
 */
public class JbCommands extends HashMap<Integer, JbCommands.Command>
    implements JbReplyListener, JbDefs {

    /** The parameter cache for my values (driver values). */
    private JbParameters m_driverParameters;

    /** The parameter cache for detector values. */
    private JbParameters m_detectorParameters;

    /** I/O */
    private JbIO m_jbIO = new JbIO();

    private JbReplyProcessor m_replyProcessor = null;

    /**
     * The list of replies we're expecting.
     * A reply is expected if we sent a command that should produce that
     * reply, and it has not been received yet.
     */
    private List<Integer> m_expectedReplyList = new ArrayList<Integer>();

    /** Whether to validate the replies, or assume they're correct. */
    private boolean m_validateReplies = false;

    /** Keep track of I/O errors so we don't innundate user with messages. */
    private int m_errorCount = 0;


    /**
     * Test routine ...
     */
    //static public void main(String[] args) {
    //}

    /**
     * Initialize the command table with all the standard commands.
     * This class holds pointers to the parameter tables, which are
     * needed to send and receive messages. 
     */
    public JbCommands(JbParameters driverPars, JbParameters detectorPars) {

        m_driverParameters = driverPars;
        m_detectorParameters = detectorPars;

        // Construct command table
        Object[][] cp = COMMANDS_AND_PARAMETERS;
        int nCmds = cp.length;
        for (int i = 0; i < nCmds; i++) {
            String name = (String)cp[i][0];
            int code = ((Integer)cp[i][1]).intValue();
            int size = ((Integer)cp[i][2]).intValue();
            int npars = cp[i].length - 3;
            String[] pars = new String[npars];
            for (int j = 0; j < npars; j++) {
                pars[j] = (String)cp[i][j + 3];
            }
            put((Integer)cp[i][1], new Command(name, code, size, pars));
        }

        m_jbIO.setReplyListener(this);
    }

    /**
     * Register a listener for messages from instrument.
     */
    public void setReplyProcessor(JbReplyProcessor jbl) {
        m_replyProcessor = jbl;
    }

    /**
     * Connect to the instrument.
     * @return Returns false if the connection cannot be made.
     */
    public boolean connect(String ipAddress) {
        return m_jbIO.openPort(ipAddress);
    }

    /**
     * Disconnect from the instrument.
     */
    public void disconnect() {
        m_jbIO.closePort();
    }

    /** Determine if detector communication is OK. */
    public boolean isConnected() {
        return m_jbIO.isPortOpen();
    }

    /**
     * Copy all parameters used by a given message from one table to another.
     * @param msg The ID code for the message.
     * @param src The parameter table from which parameter values are read.
     * @param dst The parameter table to which parameter values are written.
     */
    public void copyPars(int msg, JbParameters src, JbParameters dst) {
        Command command = (Command)get(new Integer(msg));
        String[] pnames = command.getParameters();
        for (int i = 0; i < pnames.length; i++) {
            String name = pnames[i];
            dst.set(name, src.getParameterValue(name));
        }
    }
    
    /**
     * Send a command to the instrument, using the driver parameter table.
     * @return Returns true unless the connection is bad or the command
     * code is invalid.
     */
    public boolean sendCommand(int code) {
        boolean ok = sendMessage(code, m_driverParameters, m_jbIO.getWriter());
        if (m_validateReplies && ok) {
            synchronized (m_expectedReplyList) {
                int reply = JbInstrument.getReplyFor(code);
                m_expectedReplyList.add(reply);
                if (LcMsg.isSetFor("jbio")) {
                    LcMsg.postDebug("Sent command: 0x"
                                    + Integer.toHexString(code)
                                    + "; Expecting reply: 0x"
                                    + Integer.toHexString(reply));
                }
            }
        }
        return ok;
    }

    /**
     * Send a reply to the driver, using the detector parameter table.
     * @return Returns true unless the connection is bad or the command
     * code is invalid.
     */
    public boolean sendReply(int code, ImageOutputStream out) {
        return  sendMessage(code, m_detectorParameters, out);
    }

    /**
     * Send a message, using a given parameter table.
     * This is synchronized on the output stream to prevent different
     * threads from intermixing output characters.
     * The message may be either a command to the instrument, or
     * a response from the (virtual) instrument.  The ID code implies
     * what parameters are sent with the message.
     * @param code The ID of the message to send.
     * @param parTable The table to use to fill in the message parameters.
     * @param out The output stream to use to send the message.
     * @return Returns true unless the connection is bad or the command
     * code is invalid.
     */
    // NB: Synchronized so status queries cannot get interleaved with others.
    // Assumes that only one thread sets parameters before sending commands.
    // (Status queries have no parameters.)
    // If that's not true, we have to sync from the time first parameter
    // is set to time message is sent.
    // (E.g., synchronize JbControl.setIpAddress(), etc.)
    synchronized public boolean sendMessage(int code, JbParameters parTable,
                               ImageOutputStream out) {

        if (out == null) {
            LcMsg.postError("JbCommands.sendMessage: "
                               + "no communication with 335");
            return false;
        }

        synchronized (out) {
            Command command = (Command)get(new Integer(code));
            if (command == null) {
                LcMsg.postError("Unknown command code: 0x"
                                + Integer.toHexString(code));
                return false;
            }

            int nbytes = 0;
            String[] pnames = command.getParameters();
            for (int i = 0; i < pnames.length; i++) {
                if (!pnames[i].startsWith("*")) {
                    nbytes += parTable.getParameter(pnames[i]).getSize();
                }
            }
            // TODO: Check the command length
            //if (nbytes != command.getSize()) {
            //    LcMsg.postError("Internal error: for command \""
            //                       + command.getName() + "\" ...\n"
            //                       + "      command size = "
            //                       + command.getSize()
            //                       + ", sum of parm sizes = " + nbytes);
            //    return false;
            //}

            // Write out the message
            try {
                // NB: The "METHOD_END" is not a real 335 command. It is
                // just a "METHOD_LINE_ENTRY" with different
                // parameters. Now that we've collected the parameters,
                // change the code so the 335 can handle it.
                if (code == ID_CMD_METHOD_END_335.intValue()) {
                    code = ID_CMD_METHOD_LINE_ENTRY_335.intValue();
                }

                // Do we want debug output for this message?
                boolean dbg = LcMsg.isSetFor("jbCmds");
                if (code == ID_CMD_RETURN_STATUS.intValue()) {
                    dbg = LcMsg.isSetFor("jbstatus");
                } else if (code == ID_CMD_RETURN_SPECTRUM_335.intValue()
                           || code == ID_CMD_RETURN_MULTIPLE_SPECTRA.intValue())
                {
                    dbg = LcMsg.isSetFor("jbCmds+");
                } else if (code == ID_CMD_GET_ERROR_CODE_STRINGS.intValue()) {
                    dbg = LcMsg.isSetFor("jberrstrings");
                }

                out.writeShort(code);
                if (dbg) {
                    LcMsg.postDebug("JbCommands.sendMessage: code="
                                    + Integer.toHexString(code));
                }
                int size = command.getSize();
                out.writeInt(size);
                if (dbg) {
                    LcMsg.postDebug("sendMessage: length=" + size);
                }
                // Write all the parameters (but no more than "size" bytes)
                for (int i = 0; i < pnames.length && size > 0; i++) {
                    String name = pnames[i];
                    if (!name.startsWith("*")) {
                        JbParameters.Parameter p = parTable.getParameter(name);
                        Object oval = p.getValue();
                        int bytes = p.getSize();
                        if (oval instanceof String) {
                            // Get a String that's just the right length
                            String sval = parTable.getPaddedString(name);
                            out.writeBytes(sval);
                            if (dbg) {
                                LcMsg.postDebug("sendMessage: " + sval
                                                + "\t (" + name
                                                + " - " + bytes + " bytes)");
                            }
                        } else if (oval instanceof Byte) {
                            int ival = ((Byte)oval).intValue();
                            out.writeByte(ival);
                            if (dbg) {
                                LcMsg.postDebug("sendMessage: " + (0xff & ival)
                                                + "\t (" + name
                                                + " - " + bytes + " byte)");
                            }
                        } else if (oval instanceof Integer) {
                            int ival = ((Integer)oval).intValue();
                            if (bytes == 1) {
                                out.writeByte(ival);
                            } else if (bytes == 2) {
                                out.writeShort(ival);
                            } else {
                                out.writeInt(ival);
                            }
                            if (dbg) {
                                LcMsg.postDebug("sendMessage: " + ival
                                                + "\t (" + name
                                                + " - " + bytes + " bytes)");
                            }
                        } else if (oval instanceof Float) {
                            float fval = ((Float)oval).floatValue();
                            out.writeFloat(fval);
                            if (dbg) {
                                LcMsg.postDebug("sendMessage: " + fval
                                                + "\t (" + name
                                                + " - " + bytes + " bytes)");
                            }
                        } else if (oval instanceof float[]) {
                            float[] buf = (float[])oval;
                            int count = buf.length;
                            for (int j = 0; j < count; j++) {
                                out.writeFloat(buf[j]);
                            }
                            if (dbg) {
                                LcMsg.postDebug("sendMessage: "
                                + Fmt.e(6, buf[0])
                                + ", " + Fmt.e(6, buf[1])
                                + ", " + Fmt.e(6, buf[2])
                                + ", ... "
                                + Fmt.e(6, buf[count - 1])
                                + "	(" + name + " - "
                                + (count * bytes) + " bytes)");
                            }
                        }
                        size -= bytes;
                    }
                }
                out.flush();        // Actually send it
            } catch (IOException ioe) {
                if (m_errorCount == 0) {
                    LcMsg.postError("Cannot send command to 335: " + ioe);
                }
                m_errorCount++;
                return false;
            }
        }
        if (m_errorCount != 0) {
            LcMsg.postInfo("Communication to 335 OK");
            m_errorCount = 0;
        }
        return true;
    }

    /**
     * Report I/O error.
     */
    private void reportIOError(String msg, Exception e) {
        int errs = m_jbIO.getErrorCount();
        if (errs == 0) {
            LcMsg.postError(msg + ": " + e);
        }
        m_jbIO.setErrorCount(++errs);
        m_jbIO.openPort();      // Try to reopen the socket
    }

    /**
     * Report absence of I/O error.
     */
    private void reportIORecovery() {
        if (m_jbIO.getErrorCount() > 0) {
            m_jbIO.setErrorCount(0);
            LcMsg.postInfo("Communication with 335 PDA is OK.");
        }
    }

    private int validateLength(int code, int len) {
        int l = ((Command)get(new Integer(code))).getSize();
        if (l == -1) {
            // No check possible
        } else if (l < 0) {
            if (len > -l) {
                LcMsg.postError("Variable length 335 reply is "
                                + "too long: " + len
                                + " Max allowed is "
                                + (-l) + " bytes");
                len = -l;
            }
        } else {
            if (len != l) {
                LcMsg.postError("Fixed length 335 reply has "
                                + "wrong length: " + len
                                + " Correct value is "
                                + l + " bytes");
                len = l;
            }
        }
        return len;
    }

    /**
     * Checks to see if we are expecting to get a certain reply.
     * We should never get unexpected messages, but only replies to
     * commands we have sent to the instrument.
     * If there is only one reply outstanding, returns the code for
     * that one. If there is more than one outstanding, looks at the
     * length to decide which it should be.
     * If no reply is expected, it's a serious error--returns code -1.
     * @param code The reply code to check.
     * @param len The length of the reply to check.
     * @return An array of two integers, the first is the corrected code
     * and the second is the corrected length.
     */
    private int[] validateReplyCode(int code, int len) {
        int[] rtn = new int[2];

        // Don't let the list change while we're fiddling with it 
        synchronized (m_expectedReplyList) {
            int n = m_expectedReplyList.size();
            if (n <= 0) {
                LcMsg.postError("Unexpected msg from 335 PDA; code=0x"
                                + Integer.toHexString(code));
                code = -1;
            } else {
                int idx = m_expectedReplyList.indexOf(new Integer(code));
                if (idx >= 0) {
                    // The normal case
                    m_expectedReplyList.remove(idx);
                    len = validateLength(code, len);
                } else if (n == 1) {
                    // Bad code, but correct value is known
                    int c = m_expectedReplyList.remove(0);
                    LcMsg.postError("Bad msg code from 335 PDA: expected 0x"
                                    + Integer.toHexString(c)
                                    + ", got 0x" + Integer.toHexString(code));
                    if (c != 0) { // If c == 0, don't know what we expect
                        code = c;
                    }
                    len = validateLength(code, len);
                } else {
                    // Bad code; maybe we can guess from the length
                    boolean guessed = false;
                    for (int i = 0; i < n; i++) {
                        Integer ic = m_expectedReplyList.get(i);
                        int l = get(ic).getSize();
                        if (l == len) {
                            LcMsg.postError("Bad msg code from 335 PDA: "
                                            + "Got 0x"
                                            + Integer.toHexString(code)
                                            + ", but length is correct for 0x"
                                            + Integer.toHexString(ic));
                            code = ic;
                            m_expectedReplyList.remove(i);
                            guessed = true;
                            break;
                        }
                    }
                    if (!guessed) {
                        LcMsg.postError("Bad msg code from 335 PDA: 0x"
                                        + Integer.toHexString(code)
                                        + "; can't guess what it should be");
                        code = -1;
                        len = 0;
                    }
                }
            }
        }
        rtn[0] = code;
        rtn[1] = len;
        return rtn;
    }

    /**
     * This method can be used by a virtual instrument to read commands.
     */
    public boolean receiveCommand(int code, JbInputStream in)
        throws IOException {

        return receiveMessage(code, in, m_driverParameters);
    }

    /**
     * This method can be used by the driver to read replies.
     */
    public boolean receiveReply(int code, JbInputStream in)
        throws IOException {

        // Read the message; save values in parameters
        boolean ok = receiveMessage(code, in, m_detectorParameters);

        // Do any special processing
        if (m_replyProcessor != null) {
            if (SwingUtilities.isEventDispatchThread()) {
                m_replyProcessor.processReply(code, 0);
            } else {
                try {
                    // NB: Wait for processing to complete before letting the
                    // next reply change any parameters.
                    SwingUtilities.invokeAndWait(new ReplyProcessor(code));
                } catch (InterruptedException ie) {
                    // Maybe should really wait but ... gotta go
                    LcMsg.writeStackTrace(ie, "PDA335 reply processing: ");
                } catch (InvocationTargetException ite) {
                    LcMsg.writeStackTrace(ite, "PDA335 reply processing: ");
                }
            }
        }
        return ok;
    }

    /**
     * This method can be used by a virtual instrument to read commands.
     * This is not thread-safe; it should only be called from one
     * thread (on the same input stream).
     */
    public boolean receiveMessage(int code, JbInputStream in,
                                  JbParameters pars)
        throws IOException {

        boolean ok = true;
        if (in == null) {
            throw new IOException("JbCommands.receiveMessage: "
                               + "no communication with host");
        }

        int nbytes = in.readInt(); // Number of bytes remaining in message


        if (m_validateReplies) {
            // Check if we are expecting this reply (if not, code is wrong)
            int[] codeLen = validateReplyCode(code, nbytes);
            code = codeLen[0];
            nbytes = codeLen[1];
        }

        // Notify that data packet has been received
        if (code == ID_RSP_RETURN_MULTIPLE_SPECTRA.intValue()
            && m_replyProcessor != null)
        {
            m_replyProcessor.dataReceived();
        }

        Command command = (Command)get(new Integer(code));
        String[] pnames = null;
        if (command != null) {
            pnames = command.getParameters();
        } else /* command == null */ {
            // If code=-1, we already put out a message; see validateReplyCode()
            if (code != -1) {
                // Unrecognized command
                LcMsg.postError("JbCommands.receiveMessage: "
                                + "Unknown command code: " + code
                                + " (0x" + Integer.toHexString(code) + ")");
            }
            pnames = new String[0];
            ok = false;
            // We'll just flush the message out of the stream (below).
        }

        // Do we want debug output for this message?
        boolean dbg = LcMsg.isSetFor("jbio");
        if (code == ID_RSP_RETURN_STATUS.intValue()) {
            dbg = LcMsg.isSetFor("jbstatus");
        } else if (code == ID_RSP_GET_ERROR_CODE_STRINGS.intValue()) {
            dbg = LcMsg.isSetFor("jberrstrings");
        }

        // Read in the message
        pars.set("Length", new Integer(nbytes));
        if (dbg) {
            LcMsg.postDebug("receiveMessage: length=" + nbytes
                            + " (0x" + Integer.toHexString(nbytes) + ")");
        }

        if (nbytes > 0 && pnames.length > 0) {
            nbytes = readSubMessage(code, pnames, 0, in, pars, dbg, nbytes);
        }

        // Deal with any leftover bytes
        if (nbytes != 0) {
            if (ok) {
                LcMsg.postError("JbCommands.receiveMessage: "
                                   + "Too many bytes in command");
                ok = false;
            }
            // Flush out excess message bytes
            // NB: Socket timeout was set in JbIO; flush only what is ready
            byte[] buf = new byte[1024];
            int bytesRead = 0;
            int lastChunkSize = 1;
            if (nbytes < 0) {
                nbytes = 0x100000; // Flush a lot of bytes
            }
            while (nbytes > 0 && lastChunkSize > 0) {
                int n = Math.min(buf.length, nbytes);
                try {
                    lastChunkSize = in.read(buf, 0, n);
                } catch (SocketTimeoutException ste) {
                    LcMsg.postDebug("Got SocketTimeoutException flushing msg");
                    break;
                }
                LcMsg.postDebug("Flushed " + lastChunkSize + " bytes");
                bytesRead += lastChunkSize;
                nbytes -= lastChunkSize;
                if (bytesRead < n) {
                    break;
                }
            }
            LcMsg.postError("Total flushed: " + bytesRead
                            + " bytes, expected " + (nbytes + bytesRead));
        }

        return ok;
    }

    private int readSubMessage(int code, String[] pnames, int index,
                               JbInputStream in,
                               JbParameters pars, boolean dbg, int nbytes)
        throws IOException {

        for (int i = index; nbytes > 0 && i < pnames.length; i++) {
            String name = pnames[i];
            if (name.equalsIgnoreCase("*StartLoop")) {
                i++;
                while (nbytes > 0) {
                    nbytes = readSubMessage(code, pnames, i,
                                            in, pars, dbg, nbytes);
                }
                return nbytes;  // NB: Do processing in lowest loop level
            } else if (name.startsWith("Code ")) {
                // This parameter is a index of the name of the next param
                // The next name in the list is just a placeholder
                i++;
                nbytes = readParam("Code", in, pars, dbg, nbytes);
                int idx = pars.getByte("Code");
                name = getMethodItem(idx);
                nbytes = readParam(name, in, pars, dbg, nbytes);
            } else {
                nbytes = readParam(name, in, pars, dbg, nbytes);
            }
        }
        if (index > 0 && m_replyProcessor != null) {
            m_replyProcessor.processReply(code, index);
        }
        return nbytes;
    }

    private int readParam(String name, JbInputStream in,
                          JbParameters pars, boolean dbg, int nbytes)
        throws IOException {

        if (nbytes < 0) {
            LcMsg.postError("JbCommands.receiveMessage: "
                            + "Too few bytes in command");
            return nbytes;
        }
        JbParameters.Parameter p = pars.getParameter(name);
        Object oval = p.getValue();
        int wbytes = p.getSize();
        if (oval instanceof Integer) {
            int val;
            if (wbytes == 4) {
                val = in.readInt();
            } else if (wbytes == 2) {
                val = in.readUnsignedShort();
            } else {
                val = in.readUnsignedByte();
            }
            Integer ival = new Integer(val);
            if (dbg) {
                LcMsg.postDebug("receiveMessage: " + val
                                + " (0x" + Integer.toHexString(val)
                                + ")\t (" + name
                                + " - " + wbytes + " bytes)");
            }
            /*
            if (name.equals("ReturnCode") && ival.intValue() != 0) {
                System.err.println("err =" + ival);
            }
            */
            pars.set(name, ival);
            nbytes -= wbytes;
        } else if (oval instanceof Float) {
            Float fval = new Float(in.readFloat());
            if (dbg) {
                LcMsg.postDebug("receiveMessage: "
                                + fval + "\t (" + name
                                + " - " + wbytes + " bytes)");
            }
            pars.set(name, fval);
            nbytes -= wbytes;
        } else if (oval instanceof float[]) {
            int count = pars.getInt("Count");
            float[] fbuf = new float[count];
            for (int j = 0; j < count; j++) {
                fbuf[j] = in.readFloat();
            }
            if (dbg) {
                LcMsg.postDebug("receiveMessage: "
                                + Fmt.e(6, fbuf[0])
                                + ", " + Fmt.e(6, fbuf[1])
                                + ", " + Fmt.e(6, fbuf[2])
                                + ", ... "
                                + Fmt.e(6, fbuf[count - 1])
                                + "	(" + name + " - "
                                + (count * wbytes) + " bytes)");
            }
            pars.set(name, fbuf);
            nbytes -= wbytes * count;
        } else if (oval instanceof String) {
            // Read a String that's just the right length
            byte[] buf = new byte[wbytes];
            in.readFully(buf);
            String sval = new String(buf);
            if (dbg) {
                LcMsg.postDebug("receiveMessage: "
                                + sval + "\t (" + name
                                + " - " + wbytes + " bytes)");
            }
            pars.set(name, sval);
            nbytes -= wbytes;
        } else if (oval instanceof Byte) {
            int val = in.readUnsignedByte();
            if (dbg) {
                LcMsg.postDebug("receiveMessage: " + val
                                + " (0x" + Integer.toHexString(val)
                                + ")\t (" + name
                                + " - " + wbytes + " bytes)");
            }
            pars.setByte(name, val);
            nbytes -= wbytes;
        }
        return nbytes;
    }

    /**
     * For the Method-Line-Entry, return the name corresponding to a
     * given code number.
     */
    private String getMethodItem(int idx) {
        switch (idx) {
        case 0x10:
            return "Param Wavelength 1";
        case 0x11:
            return "Param Wavelength 2";
        case 0x12:
            return "Param Autozero 1";
        case 0x13:
            return "Param Autozero 2";
        case 0x14:
            return "Param Analog 1 Attn";
        case 0x15:
            return "Param Analog 2 Attn";
        case 0x16:
            return "Param Output Relay 1";
        case 0x17:
            return "Param Output Relay 2";
        case 0x18:
            return "Param Output Relay 3";
        case 0x19:
            return "Param Output Relay 4";
        case 0x1a:
            return "Param Peaksense Relay Mode";
        case 0x1b:
            return "Param Peaksense Peak Width";
        case 0x1c:
            return "Param Peaksense S N Ratio";
        case 0x1d:
            return "Param Time Slice Trigger";
        case 0x1e:
            return "Param Time Slice Period";
        case 0x1f:
            return "Param Level Sense Threshold";
        case 0xf0:
            return "Param Method End";
        default:
            LcMsg.postError("JbCommands.getMethodItem: bad index: " + idx);
            return "";
        }
    };


    class Command {

        /** The command name as a string. */
        private String mm_name;

        /** The command code. */
        private int mm_code;

        /** The command size in bytes, as stored in a message. */
        private int mm_size;

        /** The list of parameters in the message, by parameter name. */
        private String[] mm_parameters;


        public Command(String name, int code, int size, String[] parameters) {
            mm_name = name;
            mm_code = code;
            mm_size = size;
            mm_parameters = parameters;
        }

        public String getName() {
            return mm_name;
        }

        public int getCode() {
            return mm_code;
        }

        public int getSize() {
            return mm_size;
        }

        public void setSize(int size) {
            mm_size = size;
        }

        public String[] getParameters() {
            return mm_parameters;
        }
    }


    /**
     * Interprets a reply. Done as a task in the Event Thread.
     */
    class ReplyProcessor implements Runnable {
        int mm_code;

        ReplyProcessor(int code) {
            mm_code = code;
        }

        public void run() {
            if (m_replyProcessor != null) {
                m_replyProcessor.processReply(mm_code, 0);
            }
        }
    }


    /**
     * Known commands and their parameters.
     * For each command, we need [name, code, length_bytes, parm_1, parm_2, ...]
     * where the parm_N are the parameter names.
     */
    private Object[][] COMMANDS_AND_PARAMETERS = {
        //
        // Commands
        //
        {"Get Inst Identity", ID_CMD_GET_INST_IDENTITY, ZERO},
        {"Method Action", ID_CMD_METHOD_ACTION,
         FOUR,
         "Method Action"
        },
        {"Auto Zero", ID_CMD_AUTO_ZERO, ZERO},
        {"Lamp", ID_CMD_LAMP,
         EIGHT,
         "Lamp",
         "Lamp Action",
        },
        {"Set IP Params", ID_CMD_SET_IP_PARAMS,
         new Integer(28),
         "SetFixedIP",
         "SetSubnetMask",
         "GatewayAddress",
         "BootPsname",
         "Spare Int Parameter",
         "Spare Int Parameter",
         "Spare Int Parameter",
        },
        {"Get IP Params", ID_CMD_GET_IP_PARAMS, ZERO},
        {"Get Status", ID_CMD_RETURN_STATUS, ZERO},
        {"Get Spectrum", ID_CMD_RETURN_SPECTRUM_335, ZERO},
        {"Get Multiple Spectra", ID_CMD_RETURN_MULTIPLE_SPECTRA,
         FOUR,
         "MaxNumberOfSpectra"
        },
        {"Method Header", ID_CMD_METHOD_HEADER_335,
         new Integer(112),
         "Method Version",
         "Method Name",
         "Min Wavelength",
         "Max Wavelength",
         "Slit Width",
         "Analog 1 Source",
         "Analog 1 Peak Ticks",
         "Analog 2 Source",
         "Analog 2 Peak Ticks",
         "Peak Sense Relay Active",
         "Peak Sense Duration",
         "Peak Sense Delay",
         "Analog Time Constant",
         "Bunching Size",
         "Noise Monitor Length",
         "Diagnostic Method Enabled",
         "Spare Int Parameter",
         "Processing Stage Bypass",
         "Bandwidth Filter Size",
        },
        // This inserts a line with all allowed parameters defined
        {"Method Line", ID_CMD_METHOD_LINE_ENTRY_335,
         new Integer(84),
         "Run Time",
         "Code Wavelength 1", "Param Wavelength 1",
         "Code Wavelength 2", "Param Wavelength 2",
         "Code Autozero 1", "Param Autozero 1",
         "Code Autozero 2", "Param Autozero 2",
         "Code Analog 1 Attn", "Param Analog 1 Attn",
         "Code Analog 2 Attn", "Param Analog 2 Attn",
         "Code Output Relay 1", "Param Output Relay 1",
         "Code Output Relay 2", "Param Output Relay 2",
         "Code Output Relay 3", "Param Output Relay 3",
         "Code Output Relay 4", "Param Output Relay 4",
         "Code Peaksense Relay Mode", "Param Peaksense Relay Mode",
         "Code Peaksense Peak Width", "Param Peaksense Peak Width",
         "Code Peaksense S N Ratio", "Param Peaksense S N Ratio",
         "Code Time Slice Trigger", "Param Time Slice Trigger",
         "Code Time Slice Period", "Param Time Slice Period",
         "Code Level Sense Threshold", "Param Level Sense Threshold",
        },
        // This inserts the last line of the method
        {"Method End Line", ID_CMD_METHOD_END_335,
         new Integer(9),
         "Run Time",
         "Code Method End", "Param Method End",
        },
        {"Get Error Codes", ID_CMD_GET_ERROR_CODE_STRINGS,
         FOUR,
         "ReadFlag",
        },
        {"Set Cell Params", ID_CMD_SET_CELL_PARAMS,
         EIGHT,
         "CellRatio",
         "CellType",
        },
        {"Get Cell Params", ID_CMD_GET_CELL_PARAMS,
         ZERO,
        },
        {"Get Globals", ID_CMD_GET_GLOBALS,
         ZERO,
        },


        //
        // Replies
        //
        {"Get Inst Identity", ID_RSP_GET_INST_IDENTITY,
         new Integer(32),
         "Family",
         "Model",
         "Version",
         "Name",
        },
        {"Method Action", ID_RSP_METHOD_ACTION,
         EIGHT,
         "ReturnCode",
         "Method Action",
        },
        {"Auto Zero", ID_RSP_AUTO_ZERO,
         new Integer(-8),
         "ReturnCode",
         "Spare Int Parameter", // Seems to be present on error
        },
        {"Lamp", ID_RSP_LAMP,
         EIGHT,
         "ReturnCode",
         "OrigCommand",
        },
        {"Set IP Params", ID_RSP_SET_IP_PARAMS,
         FOUR,
         "ReturnCode",
        },
        {"Get IP Params", ID_RSP_GET_IP_PARAMS,
         new Integer(32),
         "ReturnCode",
         "SetFixedIP",
         "SetSubnetMask",
         "GatewayAddress",
         "BootPsname",
         "Spare Int Parameter",
         "Spare Int Parameter",
         "Spare Int Parameter",
        },
        {"Method Header", ID_RSP_METHOD_HEADER_335,
         FOUR,
         "ReturnCode",
        },
        {"Method Line", ID_RSP_METHOD_LINE_ENTRY_335,
         EIGHT,
         "ReturnCode",
         "RunTime",
        },
        {"Get Spectrum", ID_RSP_RETURN_SPECTRUM,
         MINUS_ONE,
         "ReturnCode",
         "InstActivityField",
         "InstErrorField",
         "CommsBufferLevelUsed",
         "Status",
         "RunTime",
         "SigMod",
         "StartWL",
         "Count",
         "AbsValue",
        },
        {"Get Multiple Spectra", ID_RSP_RETURN_MULTIPLE_SPECTRA,
         MINUS_ONE,
         "ReturnCode",
         "InstActivityField",
         "InstErrorField",
         "CommsBufferLevelUsed",
         "NumberOfSpectraFollowing",
         "*StartLoop",
         "Status",
         "RunTime",
         "SigMod",
         "StartWL",
         "Count",
         "AbsValue",
        },
        {"Get Status", ID_RSP_RETURN_STATUS,
         new Integer(64),
         "ReturnCode",
         "InstErrorField",
         "D2LampState",
         "VisLampState",
         "Wavelength1",
         "Wavelength2",
         "Absorbance1",
         "Absorbance2",
         "DetectorState",
         "RunTime",
         "InstOnTime",
         "EndTime",
         "InstActivityField",
         "MethodIdNumber",
         "ParamIdNumber",
         "Spare Int Parameter",
        },
        {"Get Error Codes", ID_RSP_GET_ERROR_CODE_STRINGS,
         MINUS_ONE,
         "ReturnCode",
         "MoreFlag",
         "*StartLoop",
         "ErrorCode",
         "StringDescription",
        },
        {"Set Cell Params", ID_RSP_SET_CELL_PARAMS,
         FOUR,
         "ReturnCode",
        },
        {"Get Cell Params", ID_RSP_GET_CELL_PARAMS,
         new Integer(12),
         "ReturnCode",
         "CellRatio",
         "CellType",
        },
        {"Get Globals", ID_RSP_GET_GLOBALS,
         new Integer(52),
         "ReturnCode",
         "InstType",
         "InstConfig",
         "InstFwVersion",
         "InstHwVersion",
         "InstLogicVersion",
         "SyncSignals",
         "EnableReadyIn",
         "LampActionOnError",
         "Normalize9x0",
         "Spare 10 Bytes",
         "FirmwareRevisionLevel",
         "FirmwareIssueNumber",
        },
        
        {"Unknown Command", ZERO,
         MINUS_ONE,
        },
        
    };
}
