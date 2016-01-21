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

import java.net.*;
import java.io.*;

public class NetworkAnalyzer extends SweepControl {

    private static final int ANALYZER_PORT = 5025; // Port # for socket
    private static final int SOCKET_CONNECT_TIMEOUT = 5000;
    private static final int SOCKET_READ_TIMEOUT = 60000; // Really long time

    // Network Analyzer parameters:
    private BufferedReader naReader;
    private PrintWriter naWriter;
    private String m_sweepIp;
    private int m_traceNumber = 1;
    /** Where replies to SCPI commands are stored. */
    private String m_scpiDataFilePath = "./ScpiOutput";

    public NetworkAnalyzer(ProbeTune control, String sweepIp) {
        super(control);
        m_sweepIp = sweepIp;
        m_ok = getConnection();
    }

    protected boolean getConnection() {
        if (m_sweepIp == null) {
            return false;
        }

        Socket naSocket = null;
        SocketAddress naSocketAddress = null;

        boolean noErrors = true;
        try {
            Messages.postDebug("NA", "NA: trying connection (waiting "
                               + SOCKET_CONNECT_TIMEOUT + " ms)");
            InetAddress inetAddr = InetAddress.getByName(m_sweepIp);
            InetSocketAddress inetSocketAddr;
            inetSocketAddr = new InetSocketAddress(inetAddr, ANALYZER_PORT);
            naSocket = new Socket();
            naSocket.connect(inetSocketAddr, SOCKET_CONNECT_TIMEOUT);
            naSocket.setSoTimeout(SOCKET_READ_TIMEOUT);
        } catch (IOException ioe) {
            if (ioe instanceof SocketTimeoutException) {
                Messages.postError("Timeout connecting to NA socket \""
                                   + m_sweepIp + ":" + ANALYZER_PORT + "\"");
            } else {
                Messages.postError("IOException connecting to NA socket \""
                                   + m_sweepIp + ":" + ANALYZER_PORT + "\"");
            }
            noErrors = false;
        }

        if (noErrors) {
            Messages.postDebug("NA", "got NA socket, getting address...");
            naSocketAddress = naSocket.getRemoteSocketAddress();

            if (naSocketAddress == null) {
                Messages.postError("null NA socket address");
                noErrors = false;
            } else {
                Messages.postDebug("NA", "NA socket addr = " + naSocketAddress);
            }
        }

        if (noErrors) {
            try {
                naReader = new BufferedReader(new InputStreamReader
                                              (naSocket.getInputStream()));
                naWriter = new PrintWriter(naSocket.getOutputStream(), true);
            } catch (IOException ioe) {
                Messages.postError("IOException connecting to NA");
                noErrors = false;
            }
        }

        if (noErrors) {
            // The network analyzer must have Telnet control enabled from the
            // instrument panel:
            // "System / Misc Setup / Network Setup / Telnet Server"
            // TODO: Does this enable Telnet on the NA?
//            Messages.postDebug("NA", "Sending N.A. control enable");
//            sendToNA(":sens:mult1:stat ON");
//            sendToNA(":sens:mult2:stat ON");

            Messages.postDebug("NA", "Sending start frequency " + m_startFreq);
            sendToNA(":sens" + getTraceNumber() + ":freq:star " + m_startFreq);
            Messages.postDebug("NA", "Sending stop frequency " + m_stopFreq);
            sendToNA(":sens" + getTraceNumber() + ":freq:stop " + m_stopFreq);
            showSweepLimitsInGui();
            setForAPT();
        }

        return noErrors;
    }

    /**
     * Get data in real,imaginary format
     */
    private void setForAPT() {
        sendToNA(":FORMAT:DATA ASCII");
    }

    protected ReflectionData getData(long when, double probeDelay_ns)
        throws InterruptedException {

        Messages.postDebug("NA", "NA.getData: delay=" + probeDelay_ns);

        if (naWriter == null) {
            return null;
        }
        // TODO: Only need this delay only if motor just moved
        when = Math.max(System.currentTimeMillis() + 1000, when);
        waitUntil(when);
        //if (cancel){
        //    return null;
        //}
        //long t0 = System.currentTimeMillis();
        int trace = getTraceNumber();
        sendToNA(":calc" + trace + ":par" + trace + ":sel");
        sendToNA(":calc" + trace + ":form pol");
        sendToNA(":calc" + trace + ":data:fdat?");
        String sData = readFromNA();
        ReflectionData data = null;
        if (sData != null && sData.length() > 0) {
            data = new ReflectionData(sData, m_startFreq, m_stopFreq);
            data.correctForProbeDelay_ns(probeDelay_ns);
        }
        if (data != null && data.isZero()) {
            data = null;
        }
        //long t2 = System.currentTimeMillis();
        /*Messages.postDebug("NA", "Data read time: " + (t2 - t0) + " = "
                           + (t1 - t0) + " + " + (t2 - t1));*/
        return data;
    }

    /**
     * Read the next line from the network analyzer.
     * @return The string sent by the network analyzer.
     */
    private String readFromNA() {
        String sData = "";
        try {
            sData = naReader.readLine();
        } catch (SocketTimeoutException stoe) {
            Messages.postError("Timeout reading from NA");
        } catch (IOException ioe) {
            Messages.postError("IO exception reading NA");
        }
        Messages.postDebug("NA", "NA.getData: sData=" + sData);
        return sData;
    }

    public void setSpan(double span) {
        if (!(span >= 5e6)) { // NB: checks for span is NaN
            // Minimum allowed span
            span = 5e6;
        }
        if (span != m_stopFreq - m_startFreq) {
            super.setSpan(span);
            sendToNA(":sens" + getTraceNumber() + ":freq:span " + span);
            showSweepLimitsInGui();

            try { // TODO: Need wait here?
                Messages.postDebug("NA", "Wait for span");
                Thread.sleep(1000);
            } catch (InterruptedException ie) {
                return;
            }
            //Messages.postDebug("NA", "requesting response ... response = ...");
            //sendToNA(":sens" + getTraceNumber() + ":freq:span?");
            //Messages.postDebug("NA", "... " + naReader.readLine());

            // First data set read is often junk
            //sendToNA(":calc" + getTraceNumber() + ":data:fdat?");
            //try {
            //    naReader.readLine();
            //} catch (IOException ioe) {
            //    Messages.postError("IO exception reading NA");
            //}
        }
    }

    public void setCenter(double center) {
        if (center != getCenter()) {
            super.setCenter(center);
            sendToNA(":sens" + getTraceNumber() + ":freq:cent " + center);
            showSweepLimitsInGui();

            try {
                Messages.postDebug("AllNA", "Wait for center");
                Thread.sleep(1000);
            } catch (InterruptedException ie) {
                return;
            }/*CMP*/
            //Messages.postDebug("NA", "requesting response ... response = ...");
            //sendToNA(":sens" + getTraceNumber() + ":freq:cent?");
            //Messages.postDebug("NA", "... " + naReader.readLine());

            // First data-set read is often junk
            //sendToNA(":calc" + getTraceNumber() + ":data:fdat?");
            //try {
            //    naReader.readLine();
            //} catch (IOException ioe) {
            //    Messages.postError("IO exception reading NA");
            //}
        }
    }

    public void setSweep(double center, double span, int np,
                         int rfchan, boolean force) {
        setTraceNumber(rfchan);
        super.setSweep(center, span, np, rfchan, force);
    }

    public void setNp(int np) {
        np = Math.max(np, 255);
        super.setNp(np);
        sendToNA(":sens" + getTraceNumber() + ":swe:poin " + np);
    }

    private void setTraceNumber(int n) {
        m_traceNumber = Math.max(1, n);
    }

    private int getTraceNumber() {
        return m_traceNumber;
    }

    public boolean isConnected() {
        return naWriter != null;
    }

    private void sendToNA(String s) {
        Messages.postDebug("NA", "sendToNA <<< \"" + s + "\"");
        if (naWriter != null) {
            naWriter.println(s);
        }
    }

    /**
     * Execute a SCPI related command.
     * Possible commands:
     * <ul>
     * <li>CMD xxx (sends the given string to the network analyzer)
     * <li>QUERY xxx (sends the string, waits for reply and writes it to file)
     * <li>FILE path (sets the filepath where future replies are written)
     * <li>PRINT xxx (appends the given string to the reply file)
     * <li>PRINTLINE xxx (like PRINT, but appends a line feed after the string)
     * </ul>
     * @param scpiCmd The SCPI command string, prefaced by a keyword.
     */
    public void execScpi(String scpiCmd) {
        String[] tokens = scpiCmd.split(" +", 2);
        if (tokens.length == 2) {
            String key = tokens[0];
            String cmd = tokens[1];
            if (key.equalsIgnoreCase("CMD")) {
                sendScpiCmd(cmd);
            } else if (key.equalsIgnoreCase("QUERY")) {
                sendScpiCmd(cmd);
                getScpiReply();
            } else if (key.equalsIgnoreCase("FILE")){
                m_scpiDataFilePath = cmd;
            } else if (key.equalsIgnoreCase("PRINT")){
                appendToScpiFile(cmd);
            } else if (key.equalsIgnoreCase("PRINTLINE")){
                appendToScpiFile(cmd + System.getProperty("line.separator"));
            }  else if (key.equalsIgnoreCase("PAUSE")){
                int time_ms = Integer.parseInt(cmd);
                try {
                    Thread.sleep(time_ms);
                } catch (InterruptedException ex) {
                }
            }
        }
    }

    private void sendScpiCmd(String cmd) {
        sendToNA(cmd);
    }

    private void getScpiReply() {
        String msg = readFromNA();
        appendToScpiFile(msg + System.getProperty("line.separator"));
    }

    private void appendToScpiFile(String string) {
        TuneUtilities.appendLogString(m_scpiDataFilePath, string);
    }

}
