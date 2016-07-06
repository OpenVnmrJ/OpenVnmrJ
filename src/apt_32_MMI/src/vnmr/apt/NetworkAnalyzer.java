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
    private static final int TIMEOUT = 5000;
    private static final int TRACE_NUMBER = 1;

    // Network Analyzer parameters:
    private BufferedReader naReader;
    private PrintWriter naWriter;
    private String m_sweepIp;

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
                               + TIMEOUT + " ms)");
            InetAddress inetAddr = InetAddress.getByName(m_sweepIp);
            InetSocketAddress inetSocketAddr;
            inetSocketAddr = new InetSocketAddress(inetAddr, ANALYZER_PORT);
            naSocket = new Socket();
            naSocket.connect(inetSocketAddr, TIMEOUT);
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
        sendToNA(":calc" + getTraceNumber() + ":form pol");
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
        sendToNA(":calc" + getTraceNumber() + ":data:fdat?");
        String sData = "";
        try {
            sData = naReader.readLine();
        } catch (IOException ioe) {
            Messages.postError("IO exception reading NA");
        }
        Messages.postDebug("NA", "NA.getData: sData=" + sData);
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

    public void setNp(int np) {
        np = Math.max(np, 255);
        super.setNp(np);
        sendToNA(":sens" + getTraceNumber() + ":swe:poin " + np);
    }

    private int getTraceNumber() {
        return TRACE_NUMBER;
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
}
