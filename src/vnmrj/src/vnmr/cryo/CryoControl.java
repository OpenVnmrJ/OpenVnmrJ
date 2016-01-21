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


package vnmr.cryo;

import java.awt.Frame;
import java.awt.Window;
import java.io.File;
import java.util.HashMap;
import java.util.StringTokenizer;

import vnmr.ui.ExpPanel;
import vnmr.ui.StatusManager;
import vnmr.util.ButtonIF;
import vnmr.util.FileUtil;
import vnmr.util.Messages;

import static vnmr.cryo.CryoPort.*;


public class CryoControl implements MessageIF {

    private static final int INIT = 1;
    private static final int OPEN_CRYO = 2;
    private static final int CONNECT_CRYO = 3;
    private static final int CONNECT_TEMP = 4;

    private ButtonIF m_vif = null;
    private HashMap<String, Integer> m_commandTable
       = new HashMap<String, Integer>();
    public CryoDiagnostics m_cryoDiagnostics = null;
    private CryoSocketControl m_cryobay= null;
    private CryoSocketControl m_dataport= null;
    private CryoSocketControl m_tempcontroller= null;
    private MessageIF m_cryoMsg;
    private String m_dataDir;
    private String m_logDir;


    public CryoControl(ButtonIF vif) {
        m_vif = vif;
        m_cryoMsg = this;
        m_cryoMsg.postDebug("cryocmd", "CryoControl.<init>");

        m_dataDir = FileUtil.usrdir() + "/cryo/data";
        new File(m_dataDir).mkdirs();
        m_logDir = FileUtil.sysdir() + "/cryo/data";
        File fileLogDir = new File(m_logDir);
        fileLogDir.mkdirs();
        // NB: Of course, this only works for vnmr1:
        fileLogDir.setWritable(true, false); // Make directory writable for all

        makeCommandTable();
        maybeCreateCryoDiag(m_cryoMsg);

        m_cryoMsg.postDebug("cryocmd", "CryoControl.<init>: DONE");
    }

    private void makeCommandTable() {
        m_commandTable.clear();
        for (int i = 0; i < CMD_TABLE.length; i++) {
            m_commandTable.put(((String)CMD_TABLE[i][0]).toLowerCase(),
                             (Integer)CMD_TABLE[i][1]);
        }
    }

    public boolean execCommand(String str) {
        m_cryoMsg.postDebug("cryocmd", "execCommand(" + str + ")");
        StringTokenizer tok = new StringTokenizer(str, " ,\n");
            String cmd = tok.nextToken().trim().toLowerCase();
            Integer nCmd = m_commandTable.get(cmd);
            if (nCmd == null) {
                StringTokenizer toker2 = new StringTokenizer(str);
                if (toker2.countTokens() > 1) {
                    toker2.nextToken();        // Eat the "lccmd" token
                    str = toker2.nextToken("");
                }
                m_cryoMsg.postError("Unknown \"cryocmd\": " + str);
                return false;
            } else {
                return execCommand(cmd, nCmd, tok);
            }
    }

    public boolean execCommand(String cmd, int iCmd, StringTokenizer args) {

        boolean rtn = true;
        switch (iCmd) {
        case INIT:
            // No special action; we are now initialized
            break;
        case OPEN_CRYO:
            if (m_cryobay == null) {
                new CryoBayConnector(args).start();
            }
            if (m_tempcontroller == null) {
                new TempControlConnector(args).start();
            }
            m_cryoMsg.postDebug("cryocmd", "Open Cryo Panel");
            startCryoDiag();
            if(m_cryobay!=null && m_cryoDiagnostics!=null){
            }
            break;
        case CONNECT_CRYO:
            new CryoBayConnector(args).start();
            break;
        case CONNECT_TEMP:
            new TempControlConnector(args).start();
            break;
        default:
            rtn = false;
        }
        return rtn;
    }

//    /**
//     * Send a status string to ExpPanel - as if from Infostat.
//     * General format is:
//     * <br>"Tag Value <units>"
//     * <br>E.g.: "lklvl 82.3"
//     * <br>or "spinval 0 Hz"
//     * @param msg Status string to send.
//     */
//    public void sendStatusMessage(String msg) {
//        ((ExpPanel)m_vif).processStatusData(msg);
//    }

    public void updateStatusMessage(String msg){
    	ExpPanel.getStatusValue(msg);
    	m_cryoDiagnostics.updateStatusMessage(msg);
    	//m_cryoMsg.postDebug("cryocmd", "Cryocontrol.updateStatusMessage " + msg);
    }
    
    public StatusManager getStatusManager() {
        return (StatusManager)m_vif;
    }

    /**
     * Connect to the CryoBay
     */
    private void connectCryoBay(StringTokenizer toker){
        String name = "cryo";
        String host = "V-CryoBay";
        //int port= CryoSocketControl.CRYOBAY_PORT;
        //DateFormat fmt = new SimpleDateFormat("yyyyMMMdd", Locale.US);
        //String date = fmt.format(new Date());

        m_cryoMsg.postDebug("cryocmd", "CryoControl.connectCryoBay()");
        if (m_cryobay != null) {
            m_cryoMsg.postDebug("cryocmd", "... disconnecting cryoport");
            m_cryobay.disconnect();
            m_cryobay = null;
        }
        if (toker.hasMoreTokens()) {
            name= toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            host= toker.nextToken(" \t:"); // Allow "hostname:port"
        }
        //if (toker.hasMoreTokens()) {
        //    port= Integer.parseInt(toker.nextToken());
        //}
        if (name.equals("cryo")) {
            m_cryoMsg.postDebug("cryocmd", "... new cryobay: host=" + host);
            m_cryobay = new CryoSocketControl(host,
                                              CRYOBAY,
                                              m_cryoMsg,
                                              m_dataDir,
                                              m_logDir, (StatusManager)m_vif
                                              );
//            m_cryobay.setCryoPanel(m_cryoDiagnostics);
            if (m_cryoDiagnostics != null) {
                m_cryoDiagnostics.setCryobay(m_cryobay);
            }
            if (m_dataport != null) {
                m_cryoMsg.postDebug("cryocmd", "... disconnecting dataport");
                m_dataport.disconnect();
                m_dataport = null;
            }
            m_dataport = new CryoSocketControl(host,
                                               DATA,
                                               m_cryoMsg,
                                               m_dataDir,
                                               m_logDir, (StatusManager)m_vif
                                               );
            if (m_cryoDiagnostics != null) {
                m_cryoDiagnostics.setDataport(m_dataport);
            }
//            m_dataport.setCryoPanel(m_cryoDiagnostics);
        }
    }

    /**
     * Connect to the TempController
     */
    private void connectTemp(StringTokenizer toker){
        if (true){
            String name = "temp";
            String host = "V-CryoBay";
            //int port= CryoSocketControl.TEMP_CTL_PORT;
                
            m_cryoMsg.postDebug("cryocmd", "CryoControl.connectTemp()");
           if (m_tempcontroller != null) {
                m_cryoMsg.postDebug("cryocmd", "... calling disconnect");
                m_tempcontroller.disconnect();
                m_tempcontroller = null;
            }

            if (name.equals("temp")) {
                m_cryoMsg.postDebug("cryocmd", "... new tempcontroller: host="
                                    + host);
                m_tempcontroller = new CryoSocketControl(host,
                                                         TEMPCTRL,
                                                         m_cryoMsg,
                                                         m_dataDir,
                                                         m_logDir, (StatusManager)m_vif
                );
//                m_tempcontroller.setCryoPanel(m_cryoDiagnostics);
                if (m_cryoDiagnostics != null) {
                    m_cryoDiagnostics.setTempcontroller(m_tempcontroller);
                }
            }
        }
    }
    

    /**
     * Start up the Cryo Diagnostics
     */
    private void startCryoDiag() {
        m_cryoMsg.postDebug(//"cryocmd",
                           "startcryoDiag:  popup");
        maybeCreateCryoDiag(m_cryoMsg);
        m_cryoDiagnostics.setState(Frame.NORMAL); // Uniconify if necessary
        m_cryoDiagnostics.setVisible(true);       // Bring to top or show
        //m_cryoDiagnostics.setAdvanced(false);
    }

    /**
     * @param cryoMsgs The object that handles displaying of messages,
     * and other miscellaneous cryo interface chores.
     */
    private void maybeCreateCryoDiag(MessageIF cryoMsgs) {
        if (m_cryoDiagnostics == null) {
            m_cryoMsg.postDebug(//"cryocmd",
                               "maybeCreateCryoDiag");
            m_cryoDiagnostics = new CryoDiagnostics(m_vif,
                                                    m_cryobay,
                                                    m_tempcontroller,
                                                    m_dataport,
                                                    m_cryoMsg,
                                                    m_dataDir);
            if (m_cryobay != null) {
                m_cryoDiagnostics.setCryobay(m_cryobay);
//                m_cryobay.setCryoPanel(m_cryoDiagnostics);
            }
            if (m_dataport != null) {
//                m_dataport.setCryoPanel(m_cryoDiagnostics);
            }
            if (m_tempcontroller != null) {
                m_cryoDiagnostics.setTempcontroller(m_tempcontroller);
//                m_tempcontroller.setCryoPanel(m_cryoDiagnostics);
            }
        }
    }

    public void sendToVnmr(String str) {
        if (str != null) {
            m_vif.sendToVnmr(str);
        }
    }

    /**
     * List of possible cryo commands and their Integer codes.
     */
    private static final Object[][] CMD_TABLE = {
        {"init",                  INIT},
        {"openDiagnostics",       OPEN_CRYO},
        {"connectCryoBay",        CONNECT_CRYO},
        {"connectTempController", CONNECT_TEMP},
    };

    @Override
    public void postDebug(String msg) {
        Messages.postDebug(msg);
    }

    @Override
    public void postDebug(String category, String msg) {
        Messages.postDebug(category, msg);
    }

    @Override
    public void postError(String msg) {
        String pfx;
        if (msg.contains(":")) {
            pfx = "Cryobay error ";
        } else {
            pfx = "Cryobay error: ";
        }
        msg = pfx + msg.replaceAll("^\n*", "");
        Messages.postError(msg);
    }

    @Override
    public void postInfo(String msg) {
        Messages.postInfo(msg);
    }

    @Override
    public void postWarning(String msg) {
        Messages.postWarning(msg);
    }

    @Override
    public void processStatusData(String msg) {
    }

    @Override
    public void writeStackTrace(Exception e) {
        Messages.writeStackTrace(e);
    }

    @Override
    public void writeAdvanced(String msg) {
        m_cryoDiagnostics.writeAdvanced(msg);
    }

    @Override
    public void setDetachEnabled(boolean state) {
        m_cryoDiagnostics.setDetach(state);
    }

    @Override
    public void setStartEnabled(boolean state) {
        m_cryoDiagnostics.setStart(state);
    }

    @Override
    public void setStopEnabled(boolean state) {
        m_cryoDiagnostics.setStop(state);
    }

    @Override
    public void stopNMR() {
        m_vif.sendToVnmr("aa");
        m_cryoMsg.postError("Cryobay Error.  Stopping Acquisition");
    }

    @Override
    public void setVacPurgeEnabled(boolean state) {
        m_cryoDiagnostics.setVacPurge(state);
    }

    @Override
    public void setPumpProbeEnabled(boolean state) {
        m_cryoDiagnostics.setProbePump(state);
    }

    @Override
    public void setThermCycleEnabled(boolean state) {
        m_cryoDiagnostics.setThermCycle(state);
    }

    @Override
    public void setCryoSendEnabled(boolean state) {
        m_cryoDiagnostics.setCryoSend(state);
    }

    @Override
    public void setThermSendEnabled(boolean state) {
        m_cryoDiagnostics.setThermSend(state);
    }

    @Override
    public Window getFrame() {
        return m_cryoDiagnostics;
        
    }

    @Override
    public CryoSocketControl getCryobay() {
        return m_cryoDiagnostics.getCryobay();
    }

    @Override
    public void setDateFormat(String dateFormat) {
        m_cryoDiagnostics.setDateFormat(dateFormat);
    }

    @Override
    public String getDateFormat() {
        return m_cryoDiagnostics.getDateFormat();
    }

    @Override
    public void setCryobay(CryoSocketControl cryoSocketControl) {
    }

    @Override
    public void setStatusManager(StatusManager statusManager) {
    }

    @Override
    public void setUploadCount(int i) {
        m_cryoDiagnostics.setUploadCount(i);
    }


    class CryoBayConnector extends Thread {
        private StringTokenizer m_args;

        public CryoBayConnector(StringTokenizer args) {
            m_args = args;
        }

        public void run() {
            connectCryoBay(m_args);
        }
    }


    class TempControlConnector extends Thread {
        private StringTokenizer m_args;

        public TempControlConnector(StringTokenizer args) {
            m_args = args;
        }

        public void run() {
            connectTemp(m_args);
        }
    }
}
