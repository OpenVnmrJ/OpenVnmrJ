/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.util.*;
import java.awt.event.*;

import javax.swing.SwingUtilities;

import org.omg.CORBA.ORB;
import org.omg.PortableServer.*;
import msaccess.*;
import vnmr.util.*;


/**
 * This class handles communication with the Bear Mass Spectrometer
 */
public class MsCorbaClient extends CorbaClient implements Callback {

    public final static String MS_REF_NAME = "ms1200";

    /** Max number of queries PML can handle as one line */
    private final static int PML_MAX_CMDS = 1;

    /** Interval between status queries to PML (ms) */
    private final static int MS_STATUS_RATE = 2000;

    /** Interval between polling for PML replies (ms) */
    private final static int MS_POLL_RATE = 200;

    private Bear1200 m_accessor = null;
    private String m_msDataFile = null;
    private int m_numScans = 0;
    private double m_minRetentionTime = 0;
    private double m_maxRetentionTime = 0;
    private boolean m_pmlListenerStarted = false;
    private PmlPollingThread m_pollingThread;
    private Status1200Thread statusThread = null;
    private int m_iStatusCmd = 0;
    private long m_sendTime_ms = 0;
    private int m_corbaErrorType = ERROR_OK;
    private int m_retries = 0;
    private int m_maxRetries = 10;
    private String m_sSrcPath = "";
    private String m_sDstPath = "";
    private String m_sUser = "";
    private String m_sPasswd = "";
    private String m_sIpAddr = "";

    /**
     * Constructor only calls the base class constructor.
     * Does not validate communication in any way.
     */
    public MsCorbaClient(ButtonIF vif) {
        super(vif);
        initAccessor();
        statusThread = new Status1200Thread(MS_STATUS_RATE);
        statusThread.start();
        vif.sendToVnmr("msStatusUpdates='y'");
    }

    public void disconnect() {
        setMsStatusUpdating(false);
    }

    public void setMsStatusUpdating(boolean b) {
        boolean ison = statusThread != null && statusThread.isAlive();
        if (b != ison) {
            if (b) {
                statusThread = new Status1200Thread(MS_STATUS_RATE);
                statusThread.start();
            } else {
                statusThread.setQuit(true);
            }
        }
    }

    protected void corbaOK() {
        if (m_corbaErrorType != ERROR_OK) {
            Messages.postInfo("... CORBA to MS1200 is OK.");
            m_corbaErrorType = 0;
        }
    }

    protected void corbaError(int type, String msg) {
        if (m_corbaErrorType != type) {
            m_corbaErrorType = type;
            String sType = "";
            switch (type) {
            case ERROR_COMM_FAILURE:
                sType = "Communication failure";
                break;
            case ERROR_NO_ACCESSOR:
                sType = "Initialization failure";
                break;
            case ERROR_MISC_FAILURE:
                sType = "Unknown failure";
                break;
            }
            Messages.postError("CORBA to MS1200: " + sType + " "
                               + msg + " ...");
        }
    }

    /**
     * Set up to get data from a particular file on the Mass Spec machine.
     */
    public boolean openMsFile(String fname) {
        if (!initAccessor()) {
            return false;
        }

        m_msDataFile = fname;
        //String path = "C:/Varian1200/data/my files/" + fname;
        try {
            if (! m_accessor.setFileNameCORBA(fname)) {
                return false;
            }
            m_numScans = m_accessor.getNumberOfScansCORBA();
            m_minRetentionTime = m_accessor.getRetentionTimeCORBA(1);
            m_maxRetentionTime = m_accessor.getRetentionTimeCORBA(m_numScans);
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_COMM_FAILURE, "opening MS data file");
            //Messages.writeStackTrace(e);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
            m_msDataFile = null;
            return false;
        }
        return true;
    }

    private boolean initAccessor() {
        m_accessor = (Bear1200)getAccessor(MS_REF_NAME);
        if (m_accessor == null) {
            // NB: Already wrote a specific message in getAccessor().
            //corbaError(ERROR_NO_ACCESSOR, "initializing communication");
            return false;
        }
        if (! m_pmlListenerStarted) {
            // TODO: This passive listener mechanism failed under Linux/Nirvana
            //initListener();

            // TODO: This is the polling workaround for listener failure
            m_pollingThread = new PmlPollingThread(MS_POLL_RATE);
            m_pollingThread.start();
            m_pmlListenerStarted = true;

            /*Messages.postDebug("m_pmlListenerStarted="
                               + m_pmlListenerStarted);/*CMP*/
        }
        return m_pmlListenerStarted;
    } 

    private boolean initListener() {
        if (m_pmlListenerStarted) {
            return true;
        }
        ORB o = getOrb();
        if (o == null) {
            return false;
        }
        if (m_accessor == null) {
            return false;
        }
        POA rootPOA;
        try {
            rootPOA = POAHelper.narrow(o.resolve_initial_references("RootPOA"));
        } catch (org.omg.CORBA.ORBPackage.InvalidName in) {
            Messages.postDebug("MsCorbaClient: " + in);
            return false;
        } catch (Exception e) {
            Messages.writeStackTrace(e);
            return false;
        }
        PmlListenerImpl pmlListenerImpl = new PmlListenerImpl(this);
        try {
            rootPOA.activate_object(pmlListenerImpl);
        } catch (org.omg.PortableServer.POAPackage.ServantAlreadyActive saa) {
            Messages.postDebug("MsCorbaClient: " + saa);
            return false;
        } catch (org.omg.PortableServer.POAPackage.WrongPolicy wp) {
            Messages.postDebug("MsCorbaClient: " + wp);
            return false;
        } catch (Exception e) {
            Messages.writeStackTrace(e);
            return false;
        }
        PmlListener pmlListener = pmlListenerImpl._this(o);
        try {
            rootPOA.the_POAManager().activate();
        } catch (org.omg.PortableServer.POAManagerPackage.AdapterInactive ai) {
            Messages.postDebug("MsCorbaClient: " + ai);
            return false;
        } catch (Exception e) {
            Messages.writeStackTrace(e);
            return false;
        }
        try {
            m_accessor.registerPmlListener(pmlListener);
            corbaOK();
        } catch (org.omg.CORBA.COMM_FAILURE cfe) {
            corbaError(ERROR_COMM_FAILURE, "registering listener");
            //Messages.writeStackTrace(cfe);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
            return false;
        } catch (Exception e) {
            corbaError(ERROR_COMM_FAILURE, "registering listener");
            Messages.writeStackTrace(e);
            return false;
        }
        /*Messages.postDebug("PML Listener registered");/*CMP*/
        m_pmlListenerStarted = true;

        return true;
    }

    /**
     * Get a CORBA accessor given the CORBA Object made from the
     * reference string.
     * @param refName The base name of the reference string.  Not used
     * in this implementation.
     * @param corbaObj The CORBA Object for the reference string.
     * @return The object containing methods to call, or null on error.
     */
    protected Object getAccessor(String refName,
                                 org.omg.CORBA.Object corbaObj) {
        /*Messages.postDebug("MsCorbaClient.getAccessor("
                           + refName + ", " + corbaObj);/*CMP*/
        Object accessor = null;
        try {
            accessor = Bear1200Helper.narrow(corbaObj);
        } catch (org.omg.CORBA.BAD_PARAM cbp) {
            Messages.postDebug("Cannot narrow MS CORBA accessor: " + cbp);
        } catch (org.omg.CORBA.SystemException cse) {
            Messages.postDebug("MsCorbaClient.getAccessor: " + cse);
            //Messages.writeStackTrace(e);
        }
        return accessor;
    }

    /**
     * Set the file to be used to retrieve scans by index.
     * @param filename The name of the file (without the path).
     * @return The time in minutes.
     */
    public boolean setFileName(String filename) {
        if (!initAccessor()) {
            return false;
        }
        boolean rtn = false;
        try {
            rtn = m_accessor.setFileNameCORBA(filename);
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_COMM_FAILURE, "setting data file name on PC");
            //Messages.writeStackTrace(cse);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
        }
        return rtn;
    }

    /**
     * Get the retention time for a given MS scan.
     * @param index The number of the scan (starting with 1).
     * @return The time in minutes.
     */
    public double getTimeOfScan(int index) {
        double rtn = -1;
        if (!initAccessor()) {
            return rtn;
        }
        try {
            rtn = m_accessor.getRetentionTimeCORBA(index) / 60000.0;
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_COMM_FAILURE, "getting a scan time");
            //Messages.writeStackTrace(cse);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
        }
        return rtn;
    }

    /**
     * Find the index of the mass spectrum that is the
     * closest match to the given time.
     */
    public int getClosestIndex(int t) {
        /*
         * We implicitly assume a non-linear relationship, or else a
         * simple linear lookup would be faster.  This uses bisection.
         */
        int idx = 0;
        Messages.postDebug("msPlot",
                           "getClosestIndex(" + t
                           + ") = " + (t / 60000.0) + " min");
        Messages.postDebug("msPlot",
                           "min=" + m_minRetentionTime
                           + ", max=" + m_maxRetentionTime);
        if (!initAccessor() || m_msDataFile == null) {
            idx = -1;
        } else if (t >= m_maxRetentionTime) {
            idx = m_numScans;
        } else if (t <= m_minRetentionTime) {
            idx = 1;
        } else {
            int bot = 1;
            int top = m_numScans;
            double tbot = m_minRetentionTime;
            double ttop = m_maxRetentionTime;
            while ((idx = (top + bot) / 2) > bot) {
                double ti = m_accessor.getRetentionTimeCORBA(idx);
                Messages.postDebug("msPlot",
                                   "... idx=" + idx + ", ti=" + ti);
                if (t > ti) {
                    bot = idx;
                    tbot = ti;
                } else if (t < ti) {
                    top = idx;
                    ttop = ti;
                } else {
                    return idx;
                }
            }
            idx = ((t - tbot) > (ttop - t)) ? top : bot;
        }
        Messages.postDebug("msPlot",
                           "... ClosestIndex=" + idx);
        return idx;
    }

    /**
     * Get mass spectrum for a given index (from 1).
     */
    public int[] getData(int index) {
        int[] rtn = null;
        if (!initAccessor() || m_msDataFile == null) {
            return rtn;
        }
        try {
            rtn = m_accessor.getDataCORBA(index);
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_COMM_FAILURE, "getting a spectrum");
            //Messages.writeStackTrace(cse);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
        }
        return rtn;
    }

    /**
     * Get the current mass spectrum.
     */
    public Scan getData() {
        Scan rtn = null;
        if (!initAccessor()) {
            return rtn;
        }
        try {
            rtn = m_accessor.getDisplayScanCORBA();
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_COMM_FAILURE, "getting current spectrum");
            //Messages.writeStackTrace(cse);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
        }
        return rtn;
    }

    public boolean setScanOn(boolean on) {
        boolean rtn = false;
        if (!initAccessor()) {
            return rtn;
        }
        try {
            m_accessor.setScanSaveCORBA(on);
            rtn = true;
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_COMM_FAILURE, "initializing MS data collection");
            //Messages.writeStackTrace(cse);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
        }
        return rtn;
    }

    public boolean setAnalogOut(int chan,
                                String type,
                                String command,
                                double mass,
                                double scale) {
        boolean rtn = true;

        Messages.postDebug("msCmd", "setAnalogOut(chan=" + chan
                           + ", type=" + type + ", cmd=" + command
                           + ", mass=" + mass + ", scale=" + scale + ")");

        if (!initAccessor()) {
            return false;
        }
        if (type == null) {
            return false;
        }
        type = type.toLowerCase();
        int nType = 0;
        if (type.equals("off")) {
            nType = Bear1200.OFF;
        } else if (type.equals("tic")) {
            nType = Bear1200.TIC;
        } else if (type.equals("area")) {
            nType = Bear1200.AREA;
        } else if (type.equals("user")) {
            nType = Bear1200.USER;
        } else {
            Messages.postDebug("MsCorbaClient.setAnalogOut: "
                               + "Bad type: \"" + type + "\"");
            return false;
        }
        Messages.postDebug("msCmd", "Calling setAnalogOut" + chan + "CORBA("
                           + nType + ", \"" + command + "\", " + mass
                           + ", " + scale + ")");
        try {
            if (chan == 0) {
                rtn = m_accessor.setAnalogOut1CORBA(nType,
                                                    command, mass, scale);
            } else if (chan == 1) {
                rtn = m_accessor.setAnalogOut2CORBA(nType,
                                                    command, mass, scale);
            } else {
                Messages.postDebug("MsCorbaClient.setAnalogOut: "
                                   + "Bad channel number: " + chan);
                return false;
            }
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_COMM_FAILURE, "setting MS analog output");
            //Messages.writeStackTrace(cse);
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
        }
        Messages.postDebug("msCmd", "setAnalogOut() returning " + rtn);
        return rtn;
    }

    /**
     * Get a file from the PC.  Implemented by FTP of the file from
     * the PC to us, so we need to supply all the info FTP needs for
     * the transaction.  (PCs do not normally have an FTP server.)
     * @param sSrcPath Full path of file on PC. May use "/" for
     * file path separators.
     * @param sDstPath Full path of file on Sun.
     * @param sUser User name to use on Sun.
     * @param sPasswd Password for that user.
     * @param sIpAddr IP address of the Sun.
     */
    public boolean getFile(String sSrcPath, String sDstPath,
                           String sUser, String sPasswd, String sIpAddr) {
        if (!initAccessor()) {
            return false;
        }
        boolean ok;
        Messages.postDebug("msCmd", "MsCorbaClient.getFile(" + sSrcPath
                           + ", " + sDstPath + ", " + sUser
                           + ", " + sPasswd + ", " + sIpAddr + ")");
        ok = m_accessor.requestFileTransferCORBA(sSrcPath, sDstPath,
                                                  sUser, sPasswd, sIpAddr);
        Messages.postDebug("msCmd", "... returned " + ok);
        Messages.postDebug("msCmd", "... Retries=" + m_retries
                           + ", MaxRetries=" + m_maxRetries);
        if (ok || m_retries >= m_maxRetries) {
            m_retries = 0;
        } else if (!ok) {
            // Set up to try again
            // TODO: Make retries reentrant, let user choose max retries.
            ++m_retries;
            m_sSrcPath = sSrcPath;
            m_sDstPath = sDstPath;
            m_sUser = sUser;
            m_sPasswd = sPasswd;
            m_sIpAddr = sIpAddr;
            Messages.postDebug("msCmd", "Starting retry timer");
            ActionListener action = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                    retryGetFile();
                }
            };
            javax.swing.Timer timer
                    = new javax.swing.Timer(10000, action); // Delay in ms
            timer.setRepeats(false);
            timer.start();
        }
        return ok;
    }

    private void retryGetFile() {
        getFile(m_sSrcPath, m_sDstPath, m_sUser, m_sPasswd, m_sIpAddr);
    }

    /**
     * Sends PML commands to the MS.
     * @param cmd String to send to PML
     * @return true if servant reference is not null
     */
    public boolean sendCommand(String cmd) {
        Messages.postDebug("PML", "MsCorbaClient.sendCommand(" + cmd + ")");
        boolean rtn = false;
        if (!initAccessor()) {
            return rtn;
        }
        try {
            m_accessor.dispatchToPmlCORBA(cmd);
            corbaOK();
        } catch (org.omg.CORBA.COMM_FAILURE cfe) {
            corbaError(ERROR_COMM_FAILURE, "sending command");
            invalidateAccessor(MS_REF_NAME, ERROR_COMM_FAILURE);
            rtn = false;
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(ERROR_MISC_FAILURE, "sending command");
            //Messages.writeStackTrace(cse);
            invalidateAccessor(MS_REF_NAME, ERROR_MISC_FAILURE);
            rtn = false;
        }
        m_sendTime_ms = System.currentTimeMillis();
        return rtn;
    }

    // Implementation of Callback
    /**
     * Receives replies from the Kodiak, reformats them into standard
     * status strings, and forwards those "key value" strings to ExpPanel
     * to be sent to all the status listeners.
     * The Kodiak string may have many key value pairs, with tokens
     * separated by "|" characters:
     * <pre>
     * |key|value|key|value|key|value|...
     * </pre>
     * Splits this into individual "key value" pairs, and sends the
     * pairs one-by-one to ExpPanel.
     * @param action Not used.
     * @param text A reply from the Kodiak.
     */
    public void retrieveMessage(String action, String text) {
        Messages.postDebug("PML", "retrieveMessage(" + text + ")");
        Messages.postDebug("PML",
                           "Response time="
                           + (System.currentTimeMillis() - m_sendTime_ms)
                           + " ms");
        if (text == null) {
            return;
        }
        StringTokenizer toker = new StringTokenizer(text,"|");
        while (toker.countTokens() >= 2) {
            String key = toker.nextToken();
            String value = toker.nextToken();
            /*
             * NB: Kodiak puts a <SP> betweent the mantissa and exponent
             * of numbers in E notation.  Need to remove it for Java to
             * be able to parse the number properly.
             */
            value = value.replaceAll(" ", ""); // Remove spaces from numbers

            MsStatus.Record rec = (MsStatus.Record)MsStatus.get(key);
            if (rec != null && !value.equals(rec.getValueString())) {
                // Only distribute the value if it has changed
                m_expPanel.processStatusData(key + " " + value);
            }
        }
    }

    protected int checkAccessStatus(int status) {
        return 0;
    }

    class PmlPollingThread extends Thread {
        private int m_rate;
        private volatile boolean m_quit = false;
        private List<String> m_msgList
            = Collections.synchronizedList(new LinkedList<String>());


        /**
         * @param rate Number of milliseconds between status queries.
         */
        PmlPollingThread(int rate) {
            m_rate = rate;
            setName("MsPmlReader");
        }

        public void setQuit(boolean quit) {
            m_quit = quit;
        }

        public void setRate(int rate) {
            m_rate = rate;
        }

        public synchronized void run() {
            /*
             */
            while (!m_quit) {
                while (true) {
                    String msg = m_accessor.getPmlReplyCORBA();
                    if (msg == null || msg.length() == 0) {
                        break;
                    }
                    m_msgList.add(msg);
                    // Start up processData(), if necessary.
                    // NB: If list size is >1, processMessages has already been
                    // queued up. We may occasionally restart it here
                    // unnecessarily, but that's OK.
                    // We rely on the other threads only removing items
                    // from the list.
                    if (m_msgList.size() == 1) {
                        // Queue up data processing in the Event Thread
                        SwingUtilities.invokeLater(new Runnable() {
                                public void run() { processMessages(); } }
                                                   );
                    }
                }

                try {
                    Thread.sleep(m_rate);
                } catch (InterruptedException ie) {
                    // This is OK
                }
            }
        }

        /**
         * This method is run only in the Event-Dispatching Thread and
         * processes all the messages in the message list.
         * Process messages in batches so interface change occurs all
         * at once.
         */
        private void processMessages() {
            // Process the message list until it's empty.
            // We assume other threads only add to the list.
            boolean empty = (m_msgList.size() == 0);
            while (!empty) {
                try {
                    String msg = (String)m_msgList.remove(0);
                    empty = (m_msgList.size() == 0);
                    retrieveMessage("", msg);
                } catch (Exception e) {
                    Messages.writeStackTrace(e, "Error processing PML msg: ");
                }
            }
        }
    }


    class Status1200Thread extends Thread {
        private int m_rate = 0;
        private volatile boolean m_quit = false;
        private String[] m_queryStrings = null;

        /**
         * @param rate Number of milliseconds between status queries.
         */
        Status1200Thread(int rate) {
            m_rate = rate;
            setName("MsStatusPoller");

            /*
             * NB: If too many queries are put in one command line, the
             * Kodiak just ignores the command.  Has been observed to
             * work with 4 and fail with 5.  Don't know if it's just the
             * string length or something else.
             *
             * That's why we break up the queries into however many
             * strings it takes to have, at most, PML_MAX_CMDS queries
             * per command.
             */
            // TODO: Optimize PML command lengths and rate
            // Make status query strings
            ArrayList<String> queryStrings = new ArrayList<String>();
            Iterator itr = MsStatus.values().iterator();
            while (itr.hasNext()) {
                StringBuffer sbQueryString = new StringBuffer("send_to_nmr:");
                for (int i = 0; i < PML_MAX_CMDS && itr.hasNext(); i++) {
                    MsStatus.Record rec = (MsStatus.Record)itr.next();
                    sbQueryString.append("\"|");
                    sbQueryString.append(rec.key);
                    sbQueryString.append("|\":");
                    sbQueryString.append(rec.cmd);
                    sbQueryString.append(":");
                }
                sbQueryString.append("cr");
                queryStrings.add(sbQueryString.toString());
            }
            m_queryStrings = queryStrings.toArray(new String[0]);
        }

        public void setQuit(boolean quit) {
            m_quit = quit;
        }

        public void setRate(int rate) {
            m_rate = rate;
        }

        public synchronized void run() {
            /*
             * NB: Each status query string sent ties up the Kodiak for
             * 1 - 2 s.  (Pretty much independent of how many queries are
             * in the string.)  Queueing up a bunch of commands ties up
             * the Kodiak until they have all been dealt with.  Could
             * be tens of seconds to update all the basic status.
             *
             * That's why we only send one string at a time, with a
             * long pause in between so we leave the Kodiak free to
             * respond quickly most of the time.  But getting status
             * still adds 1-2 s to the latency time for Kodiak commands
             * to get executed.
             *
             * Update time for status depends on how many values are
             * being watched, but should be about a minute or less.
             */
            while (!m_quit) {
                if (m_iStatusCmd < m_queryStrings.length) {
                    sendCommand(m_queryStrings[m_iStatusCmd]);
                }
                ++m_iStatusCmd;
                if (m_iStatusCmd >= m_queryStrings.length) {
                    m_iStatusCmd = 0;
                }
                //for (int i = 0; i < m_queryStrings.length; i++) {
                //    sendCommand(m_queryStrings[i]);
                //}
                try {
                    Thread.sleep(m_rate);
                } catch (InterruptedException ie) {
                    // This is OK
                }
            }
        }
    }



}
